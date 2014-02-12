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



#include "Scene3D/Components/ParticleEffectComponent.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/LodComponent.h"
#include "Scene3D/Systems/LodSystem.h"
#include "Scene3D/Systems/ParticleEffectSystem.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"


namespace DAVA
{
    
REGISTER_CLASS(ParticleEffectComponent)

ParticleEffectComponent::ParticleEffectComponent()
{
	repeatsCount = -1;
	stopWhenEmpty = false;
	clearOnRestart = true;
	effectDuration = 100.0f;
	playbackSpeed = 1.0f;
	isPaused = false;
	state = STATE_STOPPED;
	effectData.infoSources.resize(1);
	effectData.infoSources[0].size=Vector2(1,1);
	effectRenderObject = new ParticleRenderObject(&effectData);
    effectRenderObject->SetWorldTransformPtr(&Matrix4::IDENTITY); //world transform doesn't effect particle render object drawing - instead particles are generated in corresponding world position
	time = 0;
	desiredLodLevel = 1;
}

ParticleEffectComponent::~ParticleEffectComponent()
{
	ClearCurrentGroups();
	if (state!=STATE_STOPPED)
		GetEntity()->GetScene()->particleEffectSystem->RemoveFromActive(this);
	SafeRelease(effectRenderObject);
	for (int32 i=0, sz = emitters.size(); i<sz; ++i)
		SafeRelease(emitters[i]);
	
}

Component * ParticleEffectComponent::Clone(Entity * toEntity)
{	
	ParticleEffectComponent * newComponent = new ParticleEffectComponent();
	newComponent->SetEntity(toEntity);
	newComponent->repeatsCount = repeatsCount;
	newComponent->stopWhenEmpty = stopWhenEmpty;	
	newComponent->playbackComplete = playbackComplete;
	newComponent->effectDuration = effectDuration;
	int emittersCount = emitters.size();
	newComponent->emitters.resize(emittersCount);
	for (int32 i=0; i<emittersCount; ++i)
		newComponent->emitters[i] = emitters[i]->Clone();
    newComponent->RebuildEffectModifiables();
	return newComponent;
}

void ParticleEffectComponent::SetSortingOffset(uint32 offset)
{
    effectRenderObject->SetSortingOffset(offset);
}

void ParticleEffectComponent::Start()
{
	isPaused = false;
	GlobalEventSystem::Instance()->Event(GetEntity(), EventSystem::START_PARTICLE_EFFECT);	
}

void ParticleEffectComponent::Stop(bool isDeleteAllParticles)
{
	if (state == STATE_STOPPED) return;
	if (isDeleteAllParticles)
	{
		ClearCurrentGroups();		
		effectData.infoSources.resize(1);
		GlobalEventSystem::Instance()->Event(GetEntity(), EventSystem::STOP_PARTICLE_EFFECT);		
	}
	else
	{
		state = STATE_STOPPING;
        SetGroupsFinishing();
	}
}

void ParticleEffectComponent::Pause(bool isPaused /*= true*/)
{	
	this->isPaused = isPaused;
}

bool ParticleEffectComponent::IsStopped()
{
	return state == STATE_STOPPED;
}

bool ParticleEffectComponent::IsPaused()
{
	return isPaused;
}

void ParticleEffectComponent::Step(float32 delta)
{
	GetEntity()->GetScene()->particleEffectSystem->UpdateEffect(this, delta, delta);
}
	
void ParticleEffectComponent::Restart(bool isDeleteAllParticles)
{
	isPaused = false;
	if (isDeleteAllParticles)
		ClearCurrentGroups();
	currRepeatsCont = 0;
	GlobalEventSystem::Instance()->Event(GetEntity(), EventSystem::START_PARTICLE_EFFECT);		
}

void ParticleEffectComponent::StopAfterNRepeats(int32 numberOfRepeats)
{
    repeatsCount = numberOfRepeats;
}

void ParticleEffectComponent::StopWhenEmpty(bool value /*= true*/)
{
	stopWhenEmpty = value;
}

void ParticleEffectComponent::ClearCurrentGroups()
{
	for (List<ParticleGroup>::iterator it = effectData.groups.begin(), e = effectData.groups.end(); it!=e; ++it)
	{
		Particle * current = (*it).head;
		while (current)
		{
			Particle *next = current->next;
			delete current;
			current = next;
		}
		it->layer->Release();
		it->emitter->Release();		
	}
	effectData.groups.clear();
}

void ParticleEffectComponent::SetRenderObjectVisible(bool visible)
{
    if (visible) 
        effectRenderObject->AddFlag(RenderObject::VISIBLE);
    else
        effectRenderObject->RemoveFlag(RenderObject::VISIBLE);

}

void ParticleEffectComponent::SetGroupsFinishing()
{
    for (List<ParticleGroup>::iterator it = effectData.groups.begin(), e = effectData.groups.end(); it!=e; ++it)
    {
        (*it).finishingGroup = true;
    }
}

void ParticleEffectComponent::SetPlaybackCompleteMessage(const Message & msg)
{
	playbackComplete = msg;
}


float32 ParticleEffectComponent::GetPlaybackSpeed()
{
	return playbackSpeed;
}

void ParticleEffectComponent::SetPlaybackSpeed(float32 value)
{
	playbackSpeed = value;
}


void ParticleEffectComponent::SetDesiredLodLevel(int32 level)
{
	desiredLodLevel = level;
	for (List<ParticleGroup>::iterator it = effectData.groups.begin(), e=effectData.groups.end(); it!=e;++it)
	{
		ParticleGroup& group = *it;
		if (!group.emitter->shortEffect)
			group.visibleLod = group.layer->IsLodActive(level);
	}
}


void ParticleEffectComponent::SetExtertnalValue(const String& name, float32 value)
{
	externalValues[name] = value;
	for (MultiMap<String, ModifiablePropertyLineBase *>::iterator it = externalModifiables.lower_bound(name), e=externalModifiables.upper_bound(name); it!=e; ++it)
		(*it).second->SetModifier(value);
}

float32 ParticleEffectComponent::GetExternalValue(const String& name)
{
	Map<String, float32>::iterator it = externalValues.find(name);
	if (it!=externalValues.end())
		return (*it).second;
	else
		return 0.0f;
}

Set<String> ParticleEffectComponent::EnumerateVariables()
{
	Set<String> res;
	for (MultiMap<String, ModifiablePropertyLineBase *>::iterator it = externalModifiables.begin(), e=externalModifiables.end(); it!=e; ++it)
		res.insert((*it).first);
	return res;
}

void ParticleEffectComponent::RegisterModifiable(ModifiablePropertyLineBase *propertyLine)
{
	externalModifiables.insert(std::make_pair(propertyLine->GetValueName(), propertyLine));
	Map<String, float32>::iterator it = externalValues.find(propertyLine->GetValueName());
	if (it!=externalValues.end())
		propertyLine->SetModifier((*it).second);
}
void ParticleEffectComponent::UnRegisterModifiable(ModifiablePropertyLineBase *propertyLine)
{
	for (MultiMap<String, ModifiablePropertyLineBase *>::iterator it = externalModifiables.lower_bound(propertyLine->GetValueName()), e=externalModifiables.upper_bound(propertyLine->GetValueName()); it!=e; ++it)
	{
		if ((*it).second == propertyLine) 
		{
			externalModifiables.erase(it);
			return;
		}
	}
}

void ParticleEffectComponent::RebuildEffectModifiables()
{
	externalModifiables.clear();
	List<ModifiablePropertyLineBase *> modifiables;
	
	for (int32 i = 0, sz = emitters.size(); i < sz; i ++)
	{		
		emitters[i]->GetModifableLines(modifiables);					
	}

	for (List<ModifiablePropertyLineBase *>::iterator it = modifiables.begin(), e=modifiables.end(); it!=e; ++it)
	{
		externalModifiables.insert(std::make_pair((*it)->GetValueName(), (*it)));
		Map<String, float32>::iterator itName = externalValues.find((*it)->GetValueName());
		if (itName!=externalValues.end())
			(*it)->SetModifier((*itName).second);
	}	
}

	
int32 ParticleEffectComponent::GetActiveParticlesCount()
{
	int32 totalActiveParticles = 0;
	for (List<ParticleGroup>::iterator it = effectData.groups.begin(), e=effectData.groups.end(); it!=e; ++it)
		totalActiveParticles+=(*it).activeParticleCount;
	

	return totalActiveParticles;
}

void ParticleEffectComponent::Serialize(KeyedArchive *archive, SerializationContext *serializationContext)
{
	Component::Serialize(archive, serializationContext);
	archive->SetUInt32("pe.version", 1);
	archive->SetBool("pe.stopWhenEmpty", stopWhenEmpty);
	archive->SetFloat("pe.effectDuration", effectDuration);
	archive->SetUInt32("pe.repeatsCount", repeatsCount);
	archive->SetBool("pe.clearOnRestart", clearOnRestart);
	archive->SetUInt32("pe.emittersCount", emitters.size());
	KeyedArchive *emittersArch = new KeyedArchive();	
	for (uint32 i=0; i<emitters.size(); ++i)
	{		
		KeyedArchive *emitterArch = new KeyedArchive();
		String filename = emitters[i]->configPath.GetRelativePathname(serializationContext->GetScenePath());
		emitterArch->SetString("emitter.filename", filename);
        emittersArch->SetVector3("emitter.position", emitters[i]->position);
		emittersArch->SetArchive(KeyedArchive::GenKeyFromIndex(i), emitterArch);
		emitterArch->Release();
	} 
	archive->SetArchive("pe.emitters", emittersArch);
	emittersArch->Release();
}
	

void ParticleEffectComponent::Deserialize(KeyedArchive *archive, SerializationContext *serializationContext)
{
	Component::Deserialize(archive, serializationContext);
	loadedVersion = archive->GetUInt32("pe.version", 0);
	
	if (loadedVersion==1) //new effect - load everything here
	{
		stopWhenEmpty = archive->GetBool("pe.stopWhenEmpty");
		effectDuration = archive->GetFloat("pe.effectDuration");
		repeatsCount = archive->GetUInt32("pe.repeatsCount");
		clearOnRestart = archive->GetBool("pe.clearOnRestart");
		uint32 emittersCount = archive->GetUInt32("pe.emittersCount");		
		KeyedArchive *emittersArch = archive->GetArchive("pe.emitters");
		emitters.resize(emittersCount);
		for (uint32 i=0; i<emittersCount; ++i)
		{		
			emitters[i]=new ParticleEmitter();
            KeyedArchive *emitterArch = emittersArch->GetArchive(KeyedArchive::GenKeyFromIndex(i));
			String filename = emitterArch->GetString("emitter.filename");
			if (!filename.empty())
				emitters[i]->LoadFromYaml(serializationContext->GetScenePath()+filename);			
            emitters[i]->position = emittersArch->GetVector3("emitter.position");
		} 	
        RebuildEffectModifiables();
	}
}

void ParticleEffectComponent::CollapseOldEffect(SerializationContext *serializationContext)
{
	Entity *entity = GetEntity();
	bool lodDefined = false;
	effectDuration = 0;
    Vector3 effectScale = Vector3(1,1,1);
    Entity *currEntity = entity;
    while (currEntity)
    {
        effectScale*=currEntity->GetLocalTransform().GetScaleVector();
        currEntity = currEntity->GetParent();
    }
	for (int32 i=0, sz = entity->GetChildrenCount(); i<sz; ++i)
	{
		Entity *child = entity->GetChild(i);
		PartilceEmitterLoadProxy *emitterProxy = NULL;
		RenderComponent *renderComponent = static_cast<RenderComponent*>(child->GetComponent(Component::RENDER_COMPONENT));
		if (renderComponent)
			emitterProxy = static_cast<PartilceEmitterLoadProxy *>(renderComponent->GetRenderObject());
		if (!emitterProxy) continue;
		ParticleEmitter *emitter = new ParticleEmitter();
		emitter->position = (child->GetLocalTransform().GetTranslationVector())*effectScale;
		if (!emitterProxy->emitterFilename.empty())
		{			
			emitter->LoadFromYaml(serializationContext->GetScenePath()+emitterProxy->emitterFilename);
			if (effectDuration<emitter->lifeTime)
				effectDuration = emitter->lifeTime;
		}
		emitter->name = child->GetName();
		emitters.push_back(emitter);
		if (!lodDefined)
		{
			LodComponent *lodComponent = static_cast<LodComponent *>(child->GetComponent(Component::LOD_COMPONENT));
			if (lodComponent)
			{
				entity->AddComponent(lodComponent->Clone(entity));
				lodDefined = true;
			}
		}
	}

	entity->RemoveAllChildren();	
    RebuildEffectModifiables();
}

int32 ParticleEffectComponent::GetEmittersCount()
{
	return (int32)emitters.size();
}
ParticleEmitter* ParticleEffectComponent::GetEmitter(int32 id)
{
	DVASSERT((id>=0)&&(id<(int32)emitters.size()));
	return emitters[id];
}

void ParticleEffectComponent::AddEmitter(ParticleEmitter *emitter)
{
    SafeRetain(emitter);
	emitters.push_back(emitter);
}


int32 ParticleEffectComponent::GetEmitterId(ParticleEmitter *emitter)
{
    for (int32 i=0, sz=emitters.size(); i<sz; ++i)
        if (emitters[i]==emitter)
            return i;
    return -1;
}

void ParticleEffectComponent::InsertEmitterAt(ParticleEmitter *emitter, int32 position)
{
    Vector<ParticleEmitter*>::iterator it = emitters.begin();
    std::advance(it, Min(position, GetEmittersCount()));
    SafeRetain(emitter);
    emitters.insert(it, emitter);
}

void ParticleEffectComponent::RemoveEmitter(ParticleEmitter *emitter)
{
	Vector<ParticleEmitter *>::iterator it = std::find(emitters.begin(), emitters.end(), emitter);
	DVASSERT(it!=emitters.end());
	emitter->Release();
	emitters.erase(it);	
}

float32 ParticleEffectComponent::GetCurrTime()
{
    return time;
}

}
