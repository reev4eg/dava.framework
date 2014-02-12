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


#ifndef __DAVAENGINE_SCENE3D_RENDEROBJECT_H__
#define	__DAVAENGINE_SCENE3D_RENDEROBJECT_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Animation/AnimatedObject.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Scene3D/Scene.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{

/*
class RenderCallInstance
{
public:
    VBO *
    IBO *
    NMaterial *
    uint32 start;
    uint32 count;
    uint32 primitiveType;
};
*/
    
/*
    Types of possible render objects
 
    
    - Mesh(Static)
    - Mesh(Skinned)
 
 */
const static uint16 INVALID_STATIC_OCCLUSION_INDEX = (uint16)(-1);

class RenderBatch;
class RenderObject : public AnimatedObject
{
public:
    enum eType
    {
        TYPE_RENDEROBJECT = 0,  // Base Render Object
        TYPE_MESH,              // Normal mesh
        TYPE_SKINNED_MESH,      // Animated mesh for skinned animations
        TYPE_LANDSCAPE,         // Landscape object
        TYPE_CUSTOM_DRAW,       // Custom drawn object
		TYPE_SPRITE,			// Sprite Node
		TYPE_PARTICLE_EMTITTER,  // Particle Emitter
		TYPE_SKYBOX
    };
    
	enum eFlags
	{
		VISIBLE = 1 << 0,
        VISIBLE_AFTER_CLIPPING_THIS_FRAME = 1 << 1,
		ALWAYS_CLIPPING_VISIBLE = 1 << 4,
        VISIBLE_STATIC_OCCLUSION = 1 << 5,
		TREE_NODE_NEED_UPDATE = 1 << 6,
		NEED_UPDATE = 1 << 7,
		MARKED_FOR_UPDATE = 1 << 8,

        CUSTOM_PREPARE_TO_RENDER = 1<<9, //if set, PrepareToRender would be called

        TRANSFORM_UPDATED = 1 << 15,
	};

	static const uint32 VISIBILITY_CRITERIA = VISIBLE | VISIBLE_AFTER_CLIPPING_THIS_FRAME | VISIBLE_STATIC_OCCLUSION;
	const static uint32 CLIPPING_VISIBILITY_CRITERIA = RenderObject::VISIBLE | VISIBLE_STATIC_OCCLUSION;
    static const uint32 SERIALIZATION_CRITERIA = VISIBLE;
protected:
    virtual ~RenderObject();
public:
    RenderObject();
    
    
    inline void SetRemoveIndex(uint32 removeIndex);
    inline uint32 GetRemoveIndex();

	inline void SetTreeNodeIndex(uint16 index);
	inline uint16 GetTreeNodeIndex();
    
    void AddRenderBatch(RenderBatch * batch);
    void AddRenderBatch(RenderBatch * batch, int32 lodIndex, int32 switchIndex);
    void RemoveRenderBatch(RenderBatch * batch);
    void RemoveRenderBatch(uint32 batchIndex);

    void UpdateBatchesSortingTransforms();

    virtual void RecalcBoundingBox();
    
	inline uint32 GetRenderBatchCount() const;
    inline RenderBatch * GetRenderBatch(uint32 batchIndex) const;
	inline RenderBatch * GetRenderBatch(uint32 batchIndex, int32 & lodIndex, int32 & switchIndex) const;

	inline uint32 GetActiveRenderBatchCount() const;
	inline RenderBatch * GetActiveRenderBatch(uint32 batchIndex) const;
    
    inline void SetFlags(uint32 _flags) { flags = _flags; }
    inline uint32 GetFlags() { return flags; }
    inline void AddFlag(uint32 _flag) { flags |= _flag; }
    inline void RemoveFlag(uint32 _flag) { flags &= ~_flag; }
    
    inline void SetAABBox(const AABBox3 & bbox);
    inline void SetWorldAABBox(const AABBox3 & bbox);
    inline void SetBSphere(const Sphere & sphere);
    
    inline const AABBox3 & GetBoundingBox() const;
    inline AABBox3 & GetWorldBoundingBox();
    
    inline void SetWorldTransformPtr(Matrix4 * _worldTransform);
    inline Matrix4 * GetWorldTransformPtr() const;
    
    inline eType GetType() { return (eType)type; }

	virtual RenderObject * Clone(RenderObject *newObject);
	virtual void Save(KeyedArchive *archive, SerializationContext *serializationContext);
	virtual void Load(KeyedArchive *archive, SerializationContext *serializationContext);

    void SetOwnerDebugInfo(const String & str) { ownerDebugInfo = str; };

    virtual void SetRenderSystem(RenderSystem * renderSystem);
	RenderSystem * GetRenderSystem();

	virtual void BakeTransform(const Matrix4 & transform);

	virtual void RecalculateWorldBoundingBox();
    
    inline uint16 GetStaticOcclusionIndex() const;
    inline void SetStaticOcclusionIndex(uint16 index);
	virtual void PrepareToRender(Camera *camera); //objects passed all tests and is going to be rendered this frame - by default calculates final matrix	

	void SetLodIndex(int32 lodIndex);
	void SetSwitchIndex(int32 switchIndex);
    int32 GetMaxLodIndex() const;
    int32 GetMaxSwitchIndex() const;

	uint8 startClippingPlane;
    
protected:
//    eType type; //TODO: waiting for enums at introspection
	RenderSystem * renderSystem;
    uint32 type;

    uint32 flags;
    uint32 debugFlags;
    uint32 removeIndex;
	uint16 treeNodeIndex;
    uint16 staticOcclusionIndex;    
    AABBox3 bbox;
    AABBox3 worldBBox;
    Matrix4 * worldTransform;                    // temporary - this should me moved directly to matrix uniforms	
    String ownerDebugInfo;
	int32 lodIndex;
	int32 switchIndex;
//    Sphere bsphere;
    
	struct IndexedRenderBatch
	{
		IndexedRenderBatch()
		:	renderBatch(0),
			lodIndex(-2),
			switchIndex(-1)
		{}

		RenderBatch * renderBatch;
		int32 lodIndex;
		int32 switchIndex;

		INTROSPECTION(IndexedRenderBatch, 
			MEMBER(renderBatch, "Render Batch", I_SAVE | I_VIEW)
			MEMBER(lodIndex, "Lod Index", I_SAVE | I_VIEW)
			MEMBER(switchIndex, "Switch Index", I_SAVE | I_VIEW)
			);
	};
    
	void UpdateActiveRenderBatches();
    Vector<IndexedRenderBatch> renderBatchArray;
	Vector<RenderBatch*> activeRenderBatchArray;

public:
	INTROSPECTION_EXTEND(RenderObject, AnimatedObject,
        MEMBER(type, "Type", I_SAVE | I_VIEW | I_EDIT)
                         
        MEMBER(flags, "Flags", I_SAVE | I_VIEW | I_EDIT)
        MEMBER(debugFlags, "Debug Flags", I_SAVE | I_VIEW | I_EDIT)
        MEMBER(removeIndex, "Remove index", I_SAVE)
        MEMBER(bbox, "Box", I_SAVE | I_VIEW | I_EDIT)
        MEMBER(worldBBox, "World Box", I_SAVE | I_VIEW | I_EDIT)
        MEMBER(worldTransform, "World Transform", I_SAVE | I_VIEW | I_EDIT)
		MEMBER(lodIndex, "Lod Index", I_VIEW | I_EDIT)
		MEMBER(switchIndex, "Switch Index", I_VIEW | I_EDIT)
                 
        COLLECTION(renderBatchArray, "Render Batch Array", I_SAVE | I_VIEW | I_EDIT)
        COLLECTION(activeRenderBatchArray, "Render Batch Array", I_VIEW)
    );
};

inline uint32 RenderObject::GetRemoveIndex()
{
    return removeIndex;
}
    
inline void RenderObject::SetRemoveIndex(uint32 _removeIndex)
{
    removeIndex = _removeIndex;
}

inline void RenderObject::SetTreeNodeIndex(uint16 index)
{
	treeNodeIndex = index;
}
inline uint16 RenderObject::GetTreeNodeIndex()
{
	return treeNodeIndex;
}
    
inline void RenderObject::SetAABBox(const AABBox3 & _bbox)
{
    bbox = _bbox;
}

inline void RenderObject::SetWorldAABBox(const AABBox3 & _bbox)
{
    worldBBox = _bbox;
}

inline const AABBox3 & RenderObject::GetBoundingBox() const
{
    return bbox;
}

inline AABBox3 & RenderObject::GetWorldBoundingBox()
{
    return worldBBox;
}
    
inline void RenderObject::SetWorldTransformPtr(Matrix4 * _worldTransform)
{
    if (worldTransform == _worldTransform)
        return;
    worldTransform = _worldTransform;
    flags |= TRANSFORM_UPDATED;
    UpdateBatchesSortingTransforms();
}
    
inline Matrix4 * RenderObject::GetWorldTransformPtr() const
{
    return worldTransform;
}

inline uint32 RenderObject::GetRenderBatchCount() const
{
    return (uint32)renderBatchArray.size();
}

inline RenderBatch * RenderObject::GetRenderBatch(uint32 batchIndex) const
{
	DVASSERT(batchIndex < renderBatchArray.size());

    return renderBatchArray[batchIndex].renderBatch;
}

inline RenderBatch * RenderObject::GetRenderBatch(uint32 batchIndex, int32 & _lodIndex, int32 & _switchIndex) const
{
	const IndexedRenderBatch & irb = renderBatchArray[batchIndex];
	_lodIndex = irb.lodIndex;
	_switchIndex = irb.switchIndex;

	return irb.renderBatch;
}

inline uint32 RenderObject::GetActiveRenderBatchCount() const
{
	return (uint32)activeRenderBatchArray.size();
}

inline RenderBatch * RenderObject::GetActiveRenderBatch(uint32 batchIndex) const
{
	return activeRenderBatchArray[batchIndex];
}
    
inline uint16 RenderObject::GetStaticOcclusionIndex() const
{
    return staticOcclusionIndex;
}
inline void RenderObject::SetStaticOcclusionIndex(uint16 _index)
{
    staticOcclusionIndex = _index;
}


} // ns

#endif	/* __DAVAENGINE_SCENE3D_RENDEROBJECT_H__ */

