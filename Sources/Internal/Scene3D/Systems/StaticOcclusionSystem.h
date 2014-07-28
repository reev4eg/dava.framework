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
#ifndef __DAVAENGINE_SCENE3D_STATIC_OCCLUSION_SYSTEM_H__
#define	__DAVAENGINE_SCENE3D_STATIC_OCCLUSION_SYSTEM_H__

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"
#include "Base/Message.h"

namespace DAVA
{
class Camera;
class RenderObject;
class StaticOcclusion;
class StaticOcclusionData;
class StaticOcclusionDataComponent;

class MessageQueue
{
public:
    MessageQueue();
    
    void DispatchMessages();
    void AddMessage(const Message & message);
private:
    std::queue<Message> messageQueue;
};
    
// System that allow to build occlusion information. Required only in editor.
class StaticOcclusionBuildSystem : public SceneSystem
{
public:
    StaticOcclusionBuildSystem(Scene * scene);
    virtual ~StaticOcclusionBuildSystem();
    
    virtual void AddEntity(Entity * entity);
    virtual void RemoveEntity(Entity * entity);
    virtual void Process(float32 timeElapsed);
    
    inline void SetCamera(Camera * camera);

    void Build();
    void RebuildCurrentCell();
    void Cancel();

    bool IsInBuild() const;
    uint32 GetBuildStatus() const;

private:
    MessageQueue messageQueue;
    
    
    void StartBuildOcclusion(BaseObject * bo, void * messageData, void * callerData);
    void OcclusionBuildStep(BaseObject * bo, void * messageData, void * callerData);
    void FinishBuildOcclusion(BaseObject * bo, void * messageData, void * callerData);
    void SceneForceLod(int32 layerIndex);
    
    Camera * camera;
    Vector<Entity*> entities;
    StaticOcclusion * staticOcclusion;
    StaticOcclusionDataComponent * componentInProgress;
    uint32 activeIndex;
    uint32 buildStepsCount;
    uint32 buildStepRemains;
    uint32 renewIndex;
};
    
// System that allow to use occlusion information during rendering
class StaticOcclusionSystem : public SceneSystem
{
public:
    StaticOcclusionSystem(Scene * scene);
    virtual ~StaticOcclusionSystem();
    
    inline void SetCamera(Camera * camera);
    virtual void AddEntity(Entity * entity);
    virtual void RemoveEntity(Entity * entity);
    virtual void Process(float32 timeElapsed);
    virtual void SceneDidLoaded();

private:
    Camera * camera;
    StaticOcclusionData * activePVSSet;
    uint32 activeBlockIndex;
    
    
    // Final system part
    void ProcessStaticOcclusion(Camera * camera);
    void ProcessStaticOcclusionForOneDataSet(uint32 blockIndex, StaticOcclusionData * data);
    void UndoOcclusionVisibility();
    Vector<StaticOcclusionDataComponent*> staticOcclusionComponents;
    Vector<RenderObject*> indexedRenderObjects;

};
    
inline void StaticOcclusionBuildSystem::SetCamera(Camera * _camera)
{
    camera = _camera;
}

inline void StaticOcclusionSystem::SetCamera(Camera * _camera)
{
    camera = _camera;
}

} // ns

#endif	/* __DAVAENGINE_SCENE3D_STATIC_OCCLUSION_SYSTEM_H__ */

