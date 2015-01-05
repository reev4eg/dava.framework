/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.
 
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
 
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.
 
    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/

#include <base/FunctionTraits.h>
#include <Debug/DVAssert.h>
#include <Base/Atomic.h>
#include <Thread/LockGuard.h>

#include <Network/Base/IOLoop.h>
#include <Network/ServiceRegistrar.h>

#include <Network/Private/ProtoDriver.h>

namespace DAVA
{
namespace Net
{

uint32 ProtoDriver::nextPacketId = 0;

ProtoDriver::ProtoDriver(IOLoop* aLoop, eNetworkRole aRole, const ServiceRegistrar& aRegistrar)
    : loop(aLoop)
    , role(aRole)
    , registrar(aRegistrar)
    , transport(NULL)
    , whatIsSending()
{
    DVASSERT(loop != NULL);
    Memset(&curPacket, 0, sizeof(curPacket));
}

ProtoDriver::~ProtoDriver()
{

}

void ProtoDriver::SetTransport(IClientTransport* aTransport, const uint32* sourceChannels, size_t channelCount)
{
    DVASSERT(aTransport != NULL && sourceChannels != NULL && channelCount > 0);

    transport = aTransport;
    channels.reserve(channelCount);
    for (size_t i = 0;i < channelCount;++i)
    {
        channels.push_back(Channel(sourceChannels[i], this));
    }
}

void ProtoDriver::SendData(uint32 channelId, const void* buffer, size_t length, uint32* outPacketId)
{
    DVASSERT(transport != NULL && buffer != NULL && length > 0);

    Packet packet;
    PreparePacket(&packet, channelId, buffer, length);
    if (outPacketId != NULL)
        *outPacketId = packet.packetId;

    // This method may be invoked from different threads
    if (true == senderLock.TryLock())
    {
        // TODO: consider optimization when called from IOLoop's thread
        curPacket = packet;
        loop->Post(MakeFunction(this, &ProtoDriver::SendCurPacket));
    }
    else
    {
        EnqueuePacket(&packet);
    }
}

void ProtoDriver::SendControl(uint32 code, uint32 channelId, uint32 packetId)
{
    ProtoHeader header;
    proto.EncodeControlFrame(&header, code, channelId, packetId);
    if (true == senderLock.TryLock())   // Control frame can be sent directly without queueing
    {
        curControl = header;
        SendCurControl();
    }
    else
    {
        // No need for mutex locking as control frames are always sent from handlers
        controlQueue.push_back(header);
    }
}

void ProtoDriver::OnConnected(const Endpoint& endp)
{
    if (SERVER_ROLE == role)
    {
        // In SERVER_ROLE only setup remote endpoints
        for (size_t i = 0, n = channels.size();i < n;++i)
        {
            channels[i].remoteEndpoint = endp;
        }
    }
    else
    {
        // In CLIENT_ROLE ask server for services
        for (size_t i = 0, n = channels.size();i < n;++i)
        {
            channels[i].remoteEndpoint = endp;
            channels[i].service = registrar.Create(channels[i].channelId);
            if (channels[i].service != NULL)
            {
                SendControl(TYPE_CHANNEL_QUERY, channels[i].channelId, 0);
            }
        }
    }
}

void ProtoDriver::OnDisconnected()
{
    for (size_t i = 0, n = channels.size();i < n;++i)
    {
        if (channels[i].service != NULL && true == channels[i].confirmed)
        {
            channels[i].confirmed = false;
            channels[i].service->OnChannelClosed(&channels[i]);
        }
    }
    ClearQueues();
    for (size_t i = 0, n = channels.size();i < n;++i)
    {
        if (channels[i].service != NULL)
        {
            registrar.Delete(channels[i].channelId, channels[i].service);
            channels[i].service = NULL;
        }
    }
}

bool ProtoDriver::OnDataReceived(const void* buffer, size_t length)
{
    bool canContinue = true;
    ProtoDecoder::DecodeResult result;
    ProtoDecoder::eDecodeStatus status = ProtoDecoder::DECODE_INVALID;
    do {
        status = proto.Decode(buffer, length, &result);
        if (ProtoDecoder::DECODE_OK == status)
        {
            switch(result.type)
            {
            case TYPE_DATA:
                canContinue = ProcessDataPacket(&result);
                break;
            case TYPE_CHANNEL_QUERY:
                canContinue = ProcessChannelQuery(&result);
                break;
            case TYPE_CHANNEL_ALLOW:
                canContinue = ProcessChannelAllow(&result);
                break;
            case TYPE_CHANNEL_DENY:
                canContinue = ProcessChannelDeny(&result);
                break;
            case TYPE_PING:
                SendControl(TYPE_PONG, 0, 0);
                canContinue = true;
                break;
            case TYPE_PONG:
                // Do nothing as some data have been already arrived
                canContinue = true;
                break;
            case TYPE_DELIVERY_ACK:
                canContinue = ProcessDeliveryAck(&result);
                break;
            }
        }
        DVASSERT(length >= result.decodedSize);
        length -= result.decodedSize;
        buffer = static_cast<const uint8*>(buffer) + result.decodedSize;
    } while (status != ProtoDecoder::DECODE_INVALID && true == canContinue && length > 0);
    canContinue = canContinue && (status != ProtoDecoder::DECODE_INVALID);
    return canContinue;
}

void ProtoDriver::OnSendComplete()
{
    if (SENDING_DATA_FRAME == whatIsSending)
    {
        curPacket.sentLength += curPacket.chunkLength;
        if (curPacket.sentLength == curPacket.dataLength)
        {
            Channel* ch = GetChannel(curPacket.channelId);
            pendingAckQueue.push_back(curPacket.packetId);

            ch->service->OnPacketSent(ch, curPacket.data, curPacket.dataLength);
            curPacket.data = NULL;
        }
    }

    if (true == DequeueControl(&curControl))    // First send control packets if any
    {
        SendCurControl();
    }
    else if (curPacket.data != NULL || true == DequeuePacket(&curPacket))   // Send current packet further or send new packet
    {
        SendCurPacket();
    }
    else
    {
        senderLock.Unlock();    // Nothing to send, unlock sender
    }
}

bool ProtoDriver::ProcessDataPacket(ProtoDecoder::DecodeResult* result)
{
    Channel* ch = GetChannel(result->channelId);
    if (ch != NULL && ch->service != NULL)
    {
        // Send back delivery confirmation
        SendControl(TYPE_DELIVERY_ACK, result->channelId, result->packetId);
        ch->service->OnPacketReceived(ch, result->data, result->dataSize);
        return true;
    }
    DVASSERT(0);
    return false;
}

bool ProtoDriver::ProcessChannelQuery(ProtoDecoder::DecodeResult* result)
{
    DVASSERT(SERVER_ROLE == role);

    Channel* ch = GetChannel(result->channelId);
    if (ch != NULL)
    {
        DVASSERT(NULL == ch->service);
        if (NULL == ch->service)
        {
            ch->service = registrar.Create(ch->channelId);
            uint32 code = ch->service != NULL ? TYPE_CHANNEL_ALLOW
                                              : TYPE_CHANNEL_DENY;
            SendControl(code, result->channelId, 0);
            if (ch->service != NULL)
            {
                ch->confirmed = true;
                ch->service->OnChannelOpen(ch);
            }
            return true;
        }
        return false;
    }
    return true;    // Nothing strange that queried channel is not found
}

bool ProtoDriver::ProcessChannelAllow(ProtoDecoder::DecodeResult* result)
{
    DVASSERT(CLIENT_ROLE == role);

    Channel* ch = GetChannel(result->channelId);
    if (ch != NULL && ch->service != NULL)
    {
        ch->confirmed = true;
        ch->service->OnChannelOpen(ch);
        return true;
    }
    DVASSERT(0);
    return false;
}

bool ProtoDriver::ProcessChannelDeny(ProtoDecoder::DecodeResult* result)
{
    DVASSERT(CLIENT_ROLE == role);

    Channel* ch = GetChannel(result->channelId);
    if (ch != NULL && ch->service != NULL)
    {
        registrar.Delete(ch->channelId, ch->service);
        // Do not call OnChannelClosed as channel hasn't been opened
        ch->service = NULL;
        return true;
    }
    DVASSERT(0);
    return false;
}

bool ProtoDriver::ProcessDeliveryAck(ProtoDecoder::DecodeResult* result)
{
    Channel* ch = GetChannel(result->channelId);
    DVASSERT(ch != NULL && ch->service != NULL);
    DVASSERT(false == pendingAckQueue.empty());
    if (ch != NULL && ch->service != NULL && false == pendingAckQueue.empty())
    {
        uint32 pendingId = pendingAckQueue.front();
        pendingAckQueue.pop_front();
        DVASSERT(pendingId == result->packetId);
        if (pendingId == result->packetId)
        {
            ch->service->OnPacketDelivered(ch, pendingId);
            return true;
        }
    }
    return false;
}

void ProtoDriver::ClearQueues()
{
    if (curPacket.data != NULL)
    {
        Channel* ch = GetChannel(curPacket.channelId);
        ch->service->OnPacketSent(ch, curPacket.data, curPacket.dataLength);
        curPacket.data = NULL;
    }
    for (Deque<Packet>::iterator i = dataQueue.begin(), e = dataQueue.end();i != e;++i)
    {
        Packet& packet = *i;
        Channel* ch = GetChannel(packet.channelId);
        ch->service->OnPacketSent(ch, packet.data, packet.dataLength);
    }
    dataQueue.clear();
    pendingAckQueue.clear();
    controlQueue.clear();
    senderLock.Unlock();
}

void ProtoDriver::SendCurPacket()
{
    DVASSERT(curPacket.sentLength < curPacket.dataLength);

    whatIsSending = SENDING_DATA_FRAME;
    curPacket.chunkLength = proto.EncodeDataFrame(&header, curPacket.channelId, curPacket.packetId, curPacket.dataLength, curPacket.sentLength);

    Buffer buffers[2];
    buffers[0] = CreateBuffer(&header);
    buffers[1] = CreateBuffer(curPacket.data + curPacket.sentLength, curPacket.chunkLength);
    transport->Send(buffers, 2);
}

void ProtoDriver::SendCurControl()
{
    whatIsSending = SENDING_CONTROL_FRAME;

    Buffer buffer = CreateBuffer(&curControl);
    transport->Send(&buffer, 1);
}

void ProtoDriver::PreparePacket(Packet* packet, uint32 channelId, const void* buffer, size_t length)
{
    DVASSERT(buffer != NULL && length > 0);

    packet->channelId = channelId;
    packet->packetId = AtomicIncrement(reinterpret_cast<int32&>(nextPacketId));;
    packet->dataLength = length;
    packet->sentLength = 0;
    packet->chunkLength = 0;
    packet->data = static_cast<uint8*>(const_cast<void*>(buffer));
}

bool ProtoDriver::EnqueuePacket(Packet* packet)
{
    bool queueWasEmpty = false;

    LockGuard<Mutex> lock(queueMutex);
    queueWasEmpty = dataQueue.empty();
    dataQueue.push_back(*packet);
    return queueWasEmpty;
}

bool ProtoDriver::DequeuePacket(Packet* dest)
{
    LockGuard<Mutex> lock(queueMutex);
    if (false == dataQueue.empty())
    {
        *dest = dataQueue.front();
        dataQueue.pop_front();
        return true;
    }
    return false;
}

bool ProtoDriver::DequeueControl(ProtoHeader* dest)
{
    // No need for mutex locking as control packets are always dequeued from handler
    if (false == controlQueue.empty())
    {
        *dest = controlQueue.front();
        controlQueue.pop_front();
        return true;
    }
    return false;
}

}   // namespace Net
}   // namespace DAVA
