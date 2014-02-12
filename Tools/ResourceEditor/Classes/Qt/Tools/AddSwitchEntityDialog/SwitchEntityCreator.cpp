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



#include "SwitchEntityCreator.h"
#include "StringConstants.h"

DAVA::Entity *SwitchEntityCreator::CreateSwitchEntity(const DAVA::Vector<DAVA::Entity *> & fromEntities)
{
	DAVA::Entity* switchEntity = new DAVA::Entity();
	switchEntity->AddComponent(new DAVA::SwitchComponent());
	switchEntity->SetName(ResourceEditor::SWITCH_NODE_NAME);

	DAVA::KeyedArchive *customProperties = switchEntity->GetCustomProperties();
	customProperties->SetBool(DAVA::Entity::SCENE_NODE_IS_SOLID_PROPERTY_NAME, false);


	DAVA::uint32 count = (DAVA::uint32)fromEntities.size();
	DVASSERT(count <= MAX_SWITCH_COUNT);

	clonedEntities.reserve(count);
	bool singleMode = true;
	for(DAVA::uint32 i = 0; i < count; ++i)
	{
		DVASSERT(0 == CountSwitchComponentsRecursive(fromEntities[i]));

		clonedEntities.push_back(fromEntities[i]->Clone());

		FindRenderObjectsRecursive(clonedEntities[i], renderPairs[i]);
		if(renderPairs[i].size() > 1)
		{
			singleMode = false;
		}

		DAVA::KeyedArchive *childProps = clonedEntities[i]->GetCustomProperties();
		if(childProps->IsKeyExists("CollisionType"))
		{	
			if(0 == i)
			{
				customProperties->SetInt32("CollisionType", childProps->GetInt32("CollisionType", 0));
			}
			else
			{
				customProperties->SetInt32("CollisionTypeCrashed", childProps->GetInt32("CollisionType", 0));
			}
		}
	}

	if(singleMode)
	{
		CreateSingleObjectData(switchEntity);
	}
	else
	{
		CreateMultipleObjectsData();
	}

	count = (DAVA::uint32)clonedEntities.size();
	for(DAVA::uint32 i = 0; i < count; ++i)
	{
		SafeRelease(clonedEntities[i]);
	}
	clonedEntities.clear();

	count = (DAVA::uint32)realChildren.size();
	for(DAVA::uint32 i = 0; i < count; ++i)
	{
		switchEntity->AddNode(realChildren[i]);
		realChildren[i]->Release();
	}
	realChildren.clear();

	return switchEntity;
}


void SwitchEntityCreator::FindRenderObjectsRecursive( DAVA::Entity * fromEntity, DAVA::Vector<RENDER_PAIR> & entityAndObjectPairs )
{
	DAVA::RenderObject * ro = GetRenderObject(fromEntity);
	if(ro && ro->GetType() == DAVA::RenderObject::TYPE_MESH)
	{
		entityAndObjectPairs.push_back(std::make_pair(fromEntity, ro));
	}

	DAVA::int32 size = fromEntity->GetChildrenCount();
	for(DAVA::int32 i = 0; i < size; ++i)
	{
		DAVA::Entity * child = fromEntity->GetChild(i);
		FindRenderObjectsRecursive(child, entityAndObjectPairs);
	}
}

DAVA::uint32 SwitchEntityCreator::CountSwitchComponentsRecursive( DAVA::Entity * fromEntity )
{
	DAVA::uint32 count = 0;
	if(GetSwitchComponent(fromEntity))
	{
		++count;
	}

	DAVA::int32 size = fromEntity->GetChildrenCount();
	for(DAVA::int32 i = 0; i < size; ++i)
	{
		count += CountSwitchComponentsRecursive(fromEntity->GetChild(i));
	}

	return count;
}



void SwitchEntityCreator::CreateSingleObjectData(DAVA::Entity *switchEntity)
{
	DAVA::RenderObject *singleRenderObject = new DAVA::Mesh();
	singleRenderObject->SetAABBox(DAVA::AABBox3(DAVA::Vector3(0, 0, 0), DAVA::Vector3(0, 0, 0)));    
	switchEntity->AddComponent(new DAVA::RenderComponent(singleRenderObject));


	DAVA::Set<DAVA::PolygonGroup*> bakedPolygonGroups;
	DAVA::uint32 count = (DAVA::uint32)clonedEntities.size();
	for(DAVA::uint32 i = 0; i < count; ++i)
	{
		DAVA::Entity * sourceEntity = clonedEntities[i];

		DAVA::RenderObject *sourceRenderObject = renderPairs[i][0].second;
		if(sourceRenderObject)
		{
			DAVA::TransformComponent * sourceTransform = GetTransformComponent(sourceEntity);
			if (sourceTransform->GetLocalTransform() != DAVA::Matrix4::IDENTITY)
			{
				DAVA::PolygonGroup * pg = sourceRenderObject->GetRenderBatchCount() > 0 ? sourceRenderObject->GetRenderBatch(0)->GetPolygonGroup() : 0;
				if(pg && bakedPolygonGroups.end() == bakedPolygonGroups.find(pg))
				{
					sourceRenderObject->BakeTransform(sourceTransform->GetLocalTransform());
					bakedPolygonGroups.insert(pg);
				}
			}

			DAVA::uint32 sourceSize = sourceRenderObject->GetRenderBatchCount();
			while(sourceSize)
			{
				DAVA::int32 lodIndex, switchIndex;
				DAVA::RenderBatch * sourceRenderBatch = sourceRenderObject->GetRenderBatch(0, lodIndex, switchIndex);
				sourceRenderBatch->Retain();
				sourceRenderObject->RemoveRenderBatch(sourceRenderBatch);
				singleRenderObject->AddRenderBatch(sourceRenderBatch, lodIndex, i);
				sourceRenderBatch->Release();
				sourceSize--;
			}
		}

		DAVA::LodComponent * lc = GetLodComponent(sourceEntity);
		if((0 != lc) && (0 == GetLodComponent(switchEntity)))
		{
			switchEntity->AddComponent(lc->Clone(switchEntity));
		}

		renderPairs[i][0].first->RemoveComponent(DAVA::Component::RENDER_COMPONENT);
		renderPairs[i][0].first->RemoveComponent(DAVA::Component::LOD_COMPONENT);

		DAVA::uint32 childrenCount = sourceEntity->GetChildrenCount();
		while(childrenCount)
		{
			DAVA::Entity *child = sourceEntity->GetChild(0);
			child->Retain();

			sourceEntity->RemoveNode(child);
			realChildren.push_back(child);

			--childrenCount;
		}
	}
}

void SwitchEntityCreator::CreateMultipleObjectsData()
{
	DAVA::uint32 count = (DAVA::uint32)clonedEntities.size();
	for(DAVA::uint32 i = 0; i < count; ++i)
	{
		DAVA::uint32 pairsCount = (DAVA::uint32)renderPairs[i].size();
		for(DAVA::uint32 p = 0; p < pairsCount; ++p)
		{
			DAVA::RenderObject *sourceRenderObject = renderPairs[i][p].second;
			DAVA::uint32 batchCount = sourceRenderObject->GetRenderBatchCount();
			for(DAVA::int32 b = (DAVA::int32)(batchCount - 1); b >= 0; --b)
			{
				DAVA::int32 lodIndex = -1, switchIndex = -1;
				DAVA::RenderBatch *batch = sourceRenderObject->GetRenderBatch(b, lodIndex, switchIndex);
				batch->Retain();

				sourceRenderObject->RemoveRenderBatch(b);
				sourceRenderObject->AddRenderBatch(batch, lodIndex, i);

				batch->Release();
			}
		}

		realChildren.push_back(clonedEntities[i]);
		clonedEntities[i] = NULL;
	}
}

