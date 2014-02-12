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


#ifndef __DAVAENGINE_SCENENODE_H__
#define __DAVAENGINE_SCENENODE_H__

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Render/RenderBase.h"
#include "Scene3D/SceneNodeAnimationKey.h"
#include "Entity/Component.h"
#include "FileSystem/KeyedArchive.h"
#include "Base/HashMap.h"
#include "Scene3D/SceneFile/SerializationContext.h"

//#define COMPONENT_STORAGE_STDMAP 1
#include "Scene3D/Components/CustomPropertiesComponent.h"

namespace DAVA
{

class Scene;
class SceneNodeAnimation;
class SceneNodeAnimationKey;
class SceneFileV2;
class DataNode;
class Entity;
class RenderComponent;
class TransformComponent;

/**
    \brief Base class of 3D scene hierarchy. All nodes in our scene graph is inherited from this node.
 */
class Entity : public BaseObject
{
protected:
	virtual ~Entity();
public:	
	Entity();
	
    /**
        \brief Function to set scene for node and it's children. 
        Function goes recursively and set scene for this node, and each child. 
        \param[in] _scene pointer to scene we want to set as holder for this node. 
     */
    virtual void SetScene(Scene * _scene);//TODO: Move SetScene to private
    /**
        \brief Function to return scene of this node. This is virtual function. For Entity's function returns it's scene value. 
        In Scene class function is overloaded and returns self. It required to avoid dynamic casts to find a scene. 
        \returns pointer to the scene that holds this node. 
     */
    virtual Scene * GetScene();
    
    void AddComponent(Component * component);
    void RemoveComponent(Component * component);
    void RemoveComponent(uint32 componentType, uint32 index = 0);

    Component * GetComponent(uint32 componentType, uint32 index = 0) const;
    Component * GetOrCreateComponent(uint32 componentType, uint32 index = 0);
    uint32 GetComponentCount();
    uint32 GetComponentCount(uint32 componentType);
    
    inline uint32 GetAvailableComponentFlags();


	// working with childs
	virtual void	AddNode(Entity * node);
    
    virtual void    InsertBeforeNode(Entity *newNode, Entity *beforeNode);
    
	virtual void	RemoveNode(Entity * node);
    virtual int32   GetChildrenCountRecursive() const;
	virtual void	RemoveAllChildren();
	virtual Entity*	GetNextChild(Entity *child);

    inline  Entity* GetChild(int32 index) const;
	inline  int32   GetChildrenCount() const;

    
	virtual bool FindNodesByNamePart(const String & namePart, List<Entity *> &outNodeList);
    
	/**
        \brief Get string with path by indexes in scenegraph from root node to current node.
        \returns result string.
     */
	String GetPathID(Entity * root);

	/**
        \brief Get Node by pathID, generated in prev function.
        \returns result Entity.
     */
	static Entity * GetNodeByPathID(Entity * root, String pathID);

	/**
        \brief Find node by it's name inside this scene node.
        \param[in] name name of object you want to find. 
        \returns pointer to the object if object with such name available, 0 in case if it's not exists.
     */
	virtual Entity *	FindByName(const String & name);
    /**
        \brief Set name of this particular node.
        \param[in] new name for this node
     */
    virtual void SetName(const String & name);

    /**
        \brief Get name of this particular node.
        \returns name of this node
     */
    inline const String & GetName() const;

    /**
        \brief Get full name of this node from root. This function is slow because it go up by hierarchy and make full node name.
        \returns this node full name from root. Example [MaxScene->camera->instance0]
     */
    String GetFullName();
    
    /**
        \brief Set tag for this object.
        Tag can be used to identify object, or find it. You can mark objects with same properies by tag, and later find them using tag criteria. 
     */
    inline void SetTag(int32 _tag);
    
    /**
        \brief Return tag for this object
        \returns tag for this object
     */
    inline int32 GetTag(); 

	
	// virtual updates
	//virtual void	Update(float32 timeElapsed);
	virtual void	Draw();
	
	// properties
	void SetVisible(const bool & isVisible);
	inline const bool GetVisible();
	inline Entity * GetParent();
	DAVA_DEPRECATED(void SetUpdatable(bool isUpdatable));
	DAVA_DEPRECATED(inline bool GetUpdatable(void));
	inline bool IsLodPart(void);
    virtual bool IsLodMain(Entity *childToCheck = NULL);//if childToCheck is NULL checks the caller node
	
	// extract data from current node to use it in animations
	void ExtractCurrentNodeKeyForAnimation(SceneNodeAnimationKey & resultKey);
	
    const Matrix4 & GetLocalTransform(); 

    /**
     \brief This method means that you always modify matrix you get. 
        If you dont want to modify matrix call GetLocalTransform().
     */
	Matrix4 & ModifyLocalTransform(); 
    const Matrix4 & GetWorldTransform() const;
    const Matrix4 & GetDefaultLocalTransform(); 
    
    void SetLocalTransform(const Matrix4 & newMatrix);
    //inline void SetWorldTransform(const Matrix4 & newMatrix);
    inline void SetDefaultLocalTransform(const Matrix4 & newMatrix);
    //inline void InvalidateLocalTransform();
	Matrix4 AccamulateLocalTransform(Entity * fromParent);
    
    /*
        \brief Go down by hierarchy and bake all transforms.
        Function can be used to bake transforms to minimize amount of matrix multiplications.
     */
    virtual void BakeTransforms();


    /*
        \brief Go down by hierarchy and propogate bool custom property to all childs.
     */
	void PropagateBoolProperty(String name, bool value);
    
	enum
    {
        // NODE_STATIC = 0x1,  // this flag means that node is always static and we do not need to update it's worldTransform
        // NODE_DYNAMIC = 0x2, // node automatically become dynamic when we update it's local matrix
        NODE_WORLD_MATRIX_ACTUAL = 1, // if this flag set this means we do not need to rebuild worldMatrix
        NODE_VISIBLE = 1 << 1, // is node and subnodes should draw
        NODE_UPDATABLE = 1 << 2, // is node and subnodes should updates. This flag is updated by the engine and can be changed at any time. Flag is always rise up on node loading
        NODE_IS_LOD_PART = 1 << 3, // node is part of a LOD node.
        NODE_LOCAL_MATRIX_IDENTITY = 1 << 4, // local matrix of this node is identity. Used to avoid unnecessary computations.
        
        BOUNDING_VOLUME_AABB = 1 << 5,  // node has axial aligned bounding box.
        BOUNDING_VOLUME_OOB = 1 << 6,   // node has object oriented bounding box.
        BOUNDING_VOLUME_SPHERE = 1 << 7,    // node has bounding sphere.

        NODE_CLIPPED_PREV_FRAME = 1 << 8, // 
        NODE_CLIPPED_THIS_FRAME = 1 << 9, // 
        NODE_INVALID = 1 << 10,  // THIS NODE not passed some of verification stages and marked as invalid. Such nodes shouldn't be drawn.

		TRANSFORM_NEED_UPDATE = 1 << 11,
		TRANSFORM_DIRTY = 1 << 12,
		NODE_DELETED = 1 << 13,
        
        // I decided to put scene flags here to avoid 2 variables. But probably we can create additional variable later if it'll be required.
        SCENE_LIGHTS_MODIFIED = 1 << 14,
        
        ENTITY_INDEX_POSITION = 16,
        ENTITY_INDEX_MASK = 0xffff,
    };
	
    inline void AddFlag(int32 flagToAdd);
    inline void RemoveFlag(int32 flagToRemove);
    inline uint32 GetFlags() const;
    void AddFlagRecursive(int32 flagToAdd);
    void RemoveFlagRecursive(int32 flagToRemove);
    inline uint32 GetIndexInParent() { return (flags >> ENTITY_INDEX_POSITION) & ENTITY_INDEX_MASK; };
    inline void SetIndexInParent(uint32 index) { flags |= (index & ENTITY_INDEX_MASK) << ENTITY_INDEX_POSITION; };
    
	// animations 
// 	void ExecuteAnimation(SceneNodeAnimation * animation);	
// 	void DetachAnimation(SceneNodeAnimation * animation);
// 	virtual void StopAllAnimations(bool recursive = true);

	void RestoreOriginalTransforms();

	
    virtual Entity* Clone(Entity *dstNode = NULL);
	
    // Do not use variables 
    //std::deque<SceneNodeAnimation *> nodeAnimations;

    // Do we need enum, or we can use virtual functions? 
    enum
    {
        EVENT_CREATE_ENTITY = 1,
        EVENT_DELETE_ENTITY,
        EVENT_ADD_COMPONENT,
        EVENT_DELETE_COMPONENT,
        EVENT_NOTIFY_UPDATE,
    };
    
    
	/**
        \brief function to enable or disable debug drawing for particular node.
        By default it's not recursive. Some objects may support flags only partially.
        For example if node do not have bounding box flag DEBUG_DRAW_AABBOX will not produce any output
        These flags are mostly for debug purposes and we do not guarantee that logic of the debug rendering will remain unchanged between 
        framework versions.
     
        \param[in] debugFlags flags to be set
        \param[in] isRecursive do you want to set flags recursively
     
     */
	void SetDebugFlags(uint32 debugFlags, bool isRecursive = false);  
    /**
        \brief function returns debug flags of specific node
        \returns flags of this specific scene node
     */
    uint32 GetDebugFlags() const;
    	
    void SetSolid(bool isSolid);
    bool GetSolid();

	void SetLocked(bool isLocked);
	bool GetLocked();

    /**
        \brief function returns maximum bounding box of scene in world coordinates.
        \returns bounding box
     */
    virtual AABBox3 GetWTMaximumBoundingBoxSlow();
    
    /**
        \brief virtual function to save node to KeyedArchive
     */
    virtual void Save(KeyedArchive * archive, SerializationContext * serializationContext);
    
    /**
        \brief virtual function to load node to KeyedArchive
     */
	virtual void Load(KeyedArchive * archive, SerializationContext * serializationContext);
    
    /**
        \brief Function to get node description for debug printing
     */
    virtual String GetDebugDescription();
    
    /**
        \brief Function returns keyed archive of custom properties for this object. 
        Custom properties can be set for each node in editor, and used in the game later to implement game logic.
     */
    KeyedArchive* GetCustomProperties();
    
    /**
        \brief This function should be implemented in each node that have data nodes inside it.
     */
    virtual void GetDataNodes(Set<DataNode*> & dataNodes);
    /**
        \brief Function to get data nodes of requested type to specific container you provide.
     */
    template<template <typename> class Container, class T>
	void GetDataNodes(Container<T> & container);
	/**
	 \brief Optimize scene before export.
     */
	void OptimizeBeforeExport();

    /**
        \brief Function to get child nodes of requested type and move them to specific container you provide.
        For example if you want to get a list of MeshInstanceNodes you should do the following.
        \code   
        #include "Scene3D/Entity.h"
        #include "Scene3D/MeshInstanceNode.h"  // You should include MeshInstanceNode because Entity class do not know the type of node you want to convert to. 
        
        void YourClass::YourFunction()
        {
            List<MeshInstanceNode*> meshNodes;
            scene->GetChildNodes(meshNodes);
        }
        \endcode
     */
    template<template <typename> class Container, class T>
	void GetChildNodes(Container<T> & container);
    
    template<template <typename> class Container>
    void GetChildEntitiesWithComponent(Container<Entity*> & container, Component::eType type);

    /**
        \brief This function is called after scene is loaded from file.
        You can perform additional initialization here.
     */
    virtual void SceneDidLoaded();

    
    void SetFog_Kostil(float32 density, const Color &color);
    
	// Property names.
	static const char* SCENE_NODE_IS_SOLID_PROPERTY_NAME;
	static const char* SCENE_NODE_IS_LOCKED_PROPERTY_NAME;

	void FindComponentsByTypeRecursive(Component::eType type, List<DAVA::Entity*> & components);
    
    Vector<Entity*> children;
    
protected:
    
    inline void CleanupComponent(Component* component, uint32 componentCount);
    void RemoveAllComponents();
    void LoadComponentsV6(KeyedArchive *compsArch, SerializationContext * serializationContext);
    void LoadComponentsV7(KeyedArchive *compsArch, SerializationContext * serializationContext);
   
protected:

    String RecursiveBuildFullName(Entity * node, Entity * endNode);
	CustomPropertiesComponent* GetCustomPropertiesComponent();

//    virtual Entity* CopyDataTo(Entity *dstNode);
	void SetParent(Entity * node);

	Scene * scene;
	Entity * parent;
	

	String	name;
	int32	tag;

    uint32 flags;
    
private:
        
	Vector<Component *> components;
    uint32 componentFlags;
    uint32 componentUpdateMarks;
    
#if defined(COMPONENT_STORAGE_STDMAP)

    typedef Map<uint32, Vector<Component*>* > ComponentsMap;
    ComponentsMap componentsMap;

#else
    
    typedef HashMap<uint32, Vector<Component*>* > ComponentsMap;
    ComponentsMap componentsMap;
    
#endif

    Matrix4 defaultLocalTransform;
   	friend class Scene;
    
public:
	INTROSPECTION_EXTEND(Entity, BaseObject,
		MEMBER(name, "Name", I_SAVE | I_VIEW | I_EDIT)
        MEMBER(tag, "Tag", I_SAVE | I_VIEW | I_EDIT)
        MEMBER(flags, "Flags", I_SAVE | I_VIEW | I_EDIT)

        PROPERTY("visible", "Visible", GetVisible, SetVisible, I_VIEW | I_EDIT)

		//COLLECTION(components, "Components", INTROSPECTION_SERIALIZABLE | INTROSPECTION_EDITOR)
		//COLLECTION(children, "Children nodes", INTROSPECTION_SERIALIZABLE | INTROSPECTION_EDITOR)
    );
};
	
inline const bool Entity::GetVisible()
{
	return (flags & NODE_VISIBLE) != 0;
}
    
inline bool Entity::GetUpdatable(void)
{
	return (flags & NODE_UPDATABLE) != 0;
}
    
inline bool Entity::IsLodPart(void)
{
	return (flags & NODE_IS_LOD_PART) != 0;
}


inline void Entity::AddFlag(int32 flagToAdd)
{
    flags |= flagToAdd;
}
    
inline void Entity::RemoveFlag(int32 flagToRemove)
{
    flags &= ~flagToRemove;
}
    
inline uint32 Entity::GetFlags() const
{
    return flags;
}

inline Entity * Entity::GetParent()
{
	return parent;
}
    
inline const String & Entity::GetName() const
{
    return name;
}

inline int32 Entity::GetTag() 
{ 
    return tag; 
};;
    
inline const Matrix4 & Entity::GetDefaultLocalTransform()
{
    return defaultLocalTransform;
}
    
//
//inline void Entity::SetWorldTransform(const Matrix4 & newMatrix)
//{
//    worldTransform = newMatrix;
//}
//
    
//inline void Entity::InvalidateLocalTransform()
//{
//    flags &= ~(NODE_WORLD_MATRIX_ACTUAL | NODE_LOCAL_MATRIX_IDENTITY);
//}

    
inline void Entity::SetDefaultLocalTransform(const Matrix4 & newMatrix)
{
    defaultLocalTransform = newMatrix;
}
    
inline void Entity::SetTag(int32 _tag)
{
    tag = _tag;
}

    
template<template <typename> class Container, class T>
void Entity::GetDataNodes(Container<T> & container)
{
    Set<DataNode*> objects;
    GetDataNodes(objects);
    
    Set<DataNode*>::const_iterator end = objects.end();
    for (Set<DataNode*>::iterator t = objects.begin(); t != end; ++t)
    {
        DataNode* obj = *t;
        
        T res = dynamic_cast<T> (obj);
        if (res)
            container.push_back(res);
    }	
}
    
template<template <typename> class Container, class T>
void Entity::GetChildNodes(Container<T> & container)
{    
    Vector<Entity*>::const_iterator end = children.end();
    for (Vector<Entity*>::iterator t = children.begin(); t != end; ++t)
    {
        Entity* obj = *t;
        
        T res = dynamic_cast<T> (obj);
        if (res)
            container.push_back(res);
        
        obj->GetChildNodes(container);
    }	
}
    
template<template <typename> class Container>
void Entity::GetChildEntitiesWithComponent(Container<Entity*> & container, Component::eType type)
{
    Vector<Entity*>::const_iterator end = children.end();
    for (Vector<Entity*>::iterator t = children.begin(); t != end; ++t)
    {
        Entity* entity = *t;
        if (entity)
        {
            Component * component = entity->GetComponent(type);
            if (component)
                container.push_back(entity);
        }
        
        entity->GetChildEntitiesWithComponent(container, type);
    }	
}

uint32 Entity::GetAvailableComponentFlags()
{
    return componentFlags;
}
    
Entity * Entity::GetChild(int32 index) const
{
    return children[index];
}

int32 Entity::GetChildrenCount() const
{
    return (int32)children.size();
}


};

#endif // __DAVAENGINE_SCENENODE_H__





