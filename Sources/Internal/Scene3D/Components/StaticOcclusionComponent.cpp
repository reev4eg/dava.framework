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
#include "Scene3D/Components/StaticOcclusionComponent.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"

namespace DAVA
{

REGISTER_CLASS(StaticOcclusionComponent)


StaticOcclusionComponent::StaticOcclusionComponent()
{
    xSubdivisions = 2;
    ySubdivisions = 2;
    zSubdivisions = 2;
    boundingBox = AABBox3(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f, 20.0f, 20.0f));
}

Component * StaticOcclusionComponent::Clone(Entity * toEntity)
{
	StaticOcclusionComponent * newComponent = new StaticOcclusionComponent();
	newComponent->SetEntity(toEntity);
    newComponent->SetSubdivisionsX(xSubdivisions);
    newComponent->SetSubdivisionsY(ySubdivisions);
    newComponent->SetSubdivisionsZ(zSubdivisions);
    newComponent->SetBoundingBox(boundingBox);
	return newComponent;
}

void StaticOcclusionComponent::Serialize(KeyedArchive *archive, SerializationContext *serializationContext)
{
	Component::Serialize(archive, serializationContext);

	if(NULL != archive)
	{
        archive->SetVariant("soc.aabbox", VariantType(boundingBox));
        archive->SetUInt32("soc.xsub", xSubdivisions);
        archive->SetUInt32("soc.ysub", ySubdivisions);
        archive->SetUInt32("soc.zsub", zSubdivisions);
	}
}

void StaticOcclusionComponent::Deserialize(KeyedArchive *archive, SerializationContext *serializationContext)
{
	if(NULL != archive)
	{
        boundingBox = archive->GetVariant("soc.aabbox")->AsAABBox3();
        xSubdivisions = archive->GetUInt32("soc.xsub", 1);
        ySubdivisions = archive->GetUInt32("soc.ysub", 1);
        zSubdivisions = archive->GetUInt32("soc.zsub", 1);
    }

	Component::Deserialize(archive, serializationContext);
}

    
    
    
    
StaticOcclusionDataComponent::StaticOcclusionDataComponent()
{
}

StaticOcclusionDataComponent::~StaticOcclusionDataComponent()
{
}

Component * StaticOcclusionDataComponent::Clone(Entity * toEntity)
{
    StaticOcclusionDataComponent * newComponent = new StaticOcclusionDataComponent();
	newComponent->SetEntity(toEntity);
    newComponent->data = data;
    return newComponent;
}
    
void StaticOcclusionDataComponent::Serialize(KeyedArchive *archive, SerializationContext *serializationContext)
{
    Component::Serialize(archive, serializationContext);
    
	if(NULL != archive)
	{
        // VB:
        archive->SetVariant("sodc.bbox", VariantType(data.bbox));
        archive->SetUInt32("sodc.blockCount", data.blockCount);
        archive->SetUInt32("sodc.objectCount", data.objectCount);
        archive->SetUInt32("sodc.subX", data.sizeX);
        archive->SetUInt32("sodc.subY", data.sizeY);
        archive->SetUInt32("sodc.subZ", data.sizeZ);
        archive->SetByteArray("sodc.data", (uint8*)data.data, data.blockCount * data.objectCount / 32 * 4);
    }
}
    
void StaticOcclusionDataComponent::Deserialize(KeyedArchive *archive, SerializationContext *serializationContext)
{
    if(NULL != archive)
	{
        data.bbox = archive->GetVariant("sodc.bbox")->AsAABBox3();
        data.blockCount = archive->GetUInt32("sodc.blockCount", 0);
        data.objectCount = archive->GetUInt32("sodc.objectCount", 0);
        data.sizeX = archive->GetUInt32("sodc.subX", 1);
        data.sizeY = archive->GetUInt32("sodc.subY", 1);
        data.sizeZ = archive->GetUInt32("sodc.subZ", 1);
        
        data.data = new uint32[data.blockCount * data.objectCount / 32];
        DVASSERT(data.blockCount * data.objectCount / 32 * 4 == archive->GetByteArraySize("sodc.data"));
        memcpy(data.data, archive->GetByteArray("sodc.data"), data.blockCount * data.objectCount / 32 * 4);
    }
    
	Component::Deserialize(archive, serializationContext);
}


}