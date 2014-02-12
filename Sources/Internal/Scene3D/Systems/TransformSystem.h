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



#ifndef __DAVAENGINE_TRANSFORM_SYSTEM_H__
#define __DAVAENGINE_TRANSFORM_SYSTEM_H__

#include "Base/BaseTypes.h"
#include "Math/MathConstants.h"
#include "Math/Matrix4.h"
#include "Base/Singleton.h"
#include "Entity/SceneSystem.h"

namespace DAVA 
{

class Entity;
class Transform;

class TransformSystem : public SceneSystem
{
public:
	TransformSystem(Scene * scene);
	~TransformSystem();

    Transform * CreateTransform();

	virtual void ImmediateEvent(Entity * entity, uint32 event);
	virtual void AddEntity(Entity * entity);
	virtual void RemoveEntity(Entity * entity);
    //virtual void SetParent(Entity * entity, Entity * parent);


    void DeleteTransform(Transform * transform);
    void LinkTransform(int32 parentIndex, int32 childIndex);
	void UnlinkTransform(int32 childIndex);

    //void AllocateMatrix();

    virtual void Process(float32 timeElapsed);

private:
    void SortAndThreadSplit();
    
	Vector<Entity*> updatableEntities;
    Vector<Entity*> sendEvent;
    
	void EntityNeedUpdate(Entity * entity);
	void HierahicAddToUpdate(Entity * entity);
    void FindNodeThatRequireUpdate(Entity * entity);
    void TransformAllChildEntities(Entity * entity);
    void RecursiveTransformCheck(Entity * entity);


	void HierahicFindUpdatableTransform(Entity * entity, bool forcedUpdate = false);

	int32 passedNodes;
	int32 multipliedNodes;
};

};

#endif //__DAVAENGINE_TRANSFORM_SYSTEM_H__
