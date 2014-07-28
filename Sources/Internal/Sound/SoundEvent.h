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



#ifndef __DAVAENGINE_SOUND_EVENT_H__
#define __DAVAENGINE_SOUND_EVENT_H__

#include "Base/BaseTypes.h"
#include "Base/EventDispatcher.h"

namespace DAVA 
{

class SoundEvent : public EventDispatcher
{
public:
    struct SoundEventParameterInfo
    {
        String name;
        float32 maxValue;
        float32 minValue;
    };

    enum eSoundEventCallbackType
    {
        EVENT_END,     /* Called when an event is stopped for any reason. */

        EVENT_COUNT
    };
    
    enum SoundEventCreateFlags
    {
        SOUND_EVENT_CREATE_DEFAULT = 0, //2D; static; no loop
        SOUND_EVENT_CREATE_STREAM = (1 << 0),
        SOUND_EVENT_CREATE_3D = (1 << 1),
        SOUND_EVENT_CREATE_LOOP = (1 << 2)
    };
    
    SoundEvent() : volume(1.f), isDirectional(false) {};

    virtual bool IsActive() const = 0;
    virtual bool Trigger() = 0;
    virtual void Stop() = 0;
    virtual void SetPaused(bool paused) = 0;
    
    virtual void SetVolume(float32 volume) = 0;
    inline float32 GetVolume() const;
    
    virtual void SetPosition(const Vector3 & position) = 0;
    virtual void SetDirection(const Vector3 & direction) = 0;
    virtual void UpdateInstancesPosition() = 0;
    virtual void SetVelocity(const Vector3 & velocity) = 0;
    
    inline bool IsDirectional() const;

    virtual void SetParameterValue(const FastName & paramName, float32 value) = 0;
    virtual float32 GetParameterValue(const FastName & paramName) = 0;
    virtual bool IsParameterExists(const FastName & paramName) = 0;

    virtual void GetEventParametersInfo(Vector<SoundEventParameterInfo> & paramsInfo) const = 0;

    virtual String GetEventName() const = 0;
    virtual float32 GetMaxDistance() const = 0;

protected:
    float32 volume;
    bool isDirectional;
};

inline float32 SoundEvent::GetVolume() const
{
    return volume;
}

inline bool SoundEvent::IsDirectional() const
{
    return isDirectional;
}

};

#endif