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



#ifndef __DAVAENGINE_SCENE3D_PARTICLEEFFECTSYSTEM_H__
#define __DAVAENGINE_SCENE3D_PARTICLEEFFECTSYSTEM_H__

#include "Base/BaseTypes.h"
#include "Scene3D/Systems/BaseProcessSystem.h"
#include "Scene3D/Components/ParticleEffectComponent.h"

namespace DAVA
{

class Component;
class ParticleEffectSystem : public SceneSystem
{
	friend class ParticleEffectComponent;
    friend class UIParticles;    
public:
	ParticleEffectSystem(Scene * scene, bool forceDisableDepthTest = false);

	~ParticleEffectSystem();
	virtual void Process(float32 timeElapsed);		
	virtual void ImmediateEvent(Entity * entity, uint32 event);
    
	virtual void RemoveEntity(Entity * entity);	
	virtual void RemoveComponent(Entity * entity, Component * component);

	void SetGlobalExtertnalValue(const String& name, float32 value);
	float32 GetGlobalExternalValue(const String& name);
	Map<String, float32> GetGlobalExternals();
	
protected:
	void RunEffect(ParticleEffectComponent *effect);	
    void AddToActive(ParticleEffectComponent *effect);
	void RemoveFromActive(ParticleEffectComponent *effect);

	void UpdateEffect(ParticleEffectComponent *effect, float32 time, float32 shortEffectTime);
	Particle* GenerateNewParticle(ParticleEffectComponent *effect, ParticleGroup& group, float32 currLoopTime, const Matrix4 &worldTransform);
	
	void PrepareEmitterParameters(Particle * particle, ParticleGroup &group, const Matrix4 &worldTransform);
	void AddParticleToBBox(const Vector3& position, float radius, AABBox3& bbox);

	void RunEmitter(ParticleEffectComponent *effect, ParticleEmitter *emitter, int32 positionSource = 0);
	

private:
	Map<String, float32> globalExternalValues;
	
	Vector<ParticleEffectComponent *> activeComponents;


private: //materials stuff
	NMaterial *particleRegularMaterial, *particleFrameBlendMaterial;
	Map<uint32, NMaterial *> materialMap;
	NMaterial *GetMaterial(Texture *texture, bool enableFog, bool enableFrameBlend, eBlendMode srcFactor, eBlendMode dstFactor);
    bool forceDisableDepthTest;
};

}

#endif //__DAVAENGINE_SCENE3D_PARTICLEEFFECTSYSTEM_H__