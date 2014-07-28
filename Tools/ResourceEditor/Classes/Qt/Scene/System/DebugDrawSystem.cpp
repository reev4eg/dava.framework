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



#include "Scene/System/DebugDrawSystem.h"
#include "Scene/SceneEditor2.h"
#include "Scene/System/LandscapeEditorDrawSystem/LandscapeProxy.h"
#include "Deprecated/EditorConfig.h"
#include "Scene3D/Components/ComponentHelpers.h"

#include "Deprecated/SceneValidator.h"

using namespace DAVA;

float32 DebugDrawSystem::HANGING_OBJECTS_HEIGHT = 0.001f;

DebugDrawSystem::DebugDrawSystem(DAVA::Scene * scene)
	: DAVA::SceneSystem(scene)
	, objectType(ResourceEditor::ESOT_NONE)
    , objectTypeColor(Color::White)
	, hangingObjectsModeEnabled(false)
    , switchesWithDifferentLodsEnabled(false)
{
	SceneEditor2 *sc = (SceneEditor2 *)GetScene();

	collSystem = sc->collisionSystem;
	selSystem = sc->selectionSystem;

	DVASSERT(NULL != collSystem);
	DVASSERT(NULL != selSystem);
	
	debugDrawState = DAVA::RenderManager::Instance()->Subclass3DRenderState(DAVA::RenderStateData::STATE_BLEND |
																		  DAVA::RenderStateData::STATE_COLORMASK_ALL |
																		  DAVA::RenderStateData::STATE_DEPTH_TEST);
}


DebugDrawSystem::~DebugDrawSystem()
{ }

void DebugDrawSystem::SetRequestedObjectType(ResourceEditor::eSceneObjectType _objectType)
{
	objectType = _objectType;

	if(ResourceEditor::ESOT_NONE != objectType)
	{
        const Vector<Color> & colors = EditorConfig::Instance()->GetColorPropertyValues("CollisionTypeColor");
        if((uint32)objectType < (uint32)colors.size())
        {
            objectTypeColor = colors[objectType];
        }
        else
        {
            objectTypeColor = Color(1.f, 0, 0, 1.f);
        }
	}
}


ResourceEditor::eSceneObjectType DebugDrawSystem::GetRequestedObjectType() const
{
	return objectType;
}

void DebugDrawSystem::Draw()
{
	DAVA::RenderManager::Instance()->SetRenderState(debugDrawState);
	DAVA::RenderManager::Instance()->FlushState();
	
	Draw(GetScene());
}

void DebugDrawSystem::Draw(DAVA::Entity *entity)
{
	if(NULL != entity)
	{
        bool isSelected = selSystem->GetSelection().HasEntity(entity);

		DrawObjectBoxesByType(entity);
		DrawUserNode(entity);
		DrawLightNode(entity);
		DrawHangingObjects(entity);        
		DrawSwitchesWithDifferentLods(entity);
		DrawWindNode(entity);
        DrawSwitchesWithDifferentLods(entity);
        DrawSoundNode(entity);

        if(isSelected)
        {
            DrawSelectedSoundNode(entity);
            DrawStaticOcclusionComponent(entity);
        }

		for(int32 i = 0; i < entity->GetChildrenCount(); ++i)
		{
			Draw(entity->GetChild(i));
		}
	}
}

void DebugDrawSystem::DrawObjectBoxesByType(DAVA::Entity *entity)
{
	bool drawBox = false;

	KeyedArchive * customProperties = GetCustomPropertiesArchieve(entity);
    if ( customProperties )
    {
        if ( customProperties->IsKeyExists( "CollisionType" ) )
        {
            drawBox = customProperties->GetInt32( "CollisionType", 0 ) == objectType;
        }
        else if ( objectType == ResourceEditor::ESOT_UNDEFINED_COLLISION && entity->GetParent() == GetScene() )
        {
            const bool skip =
                GetLight( entity ) == NULL &&
                GetCamera( entity ) == NULL &&
                GetLandscape( entity ) == NULL &&
                GetSkybox( entity ) == NULL;

            drawBox = skip;
        }
    }

    if ( drawBox )
	{
		DrawEntityBox(entity, objectTypeColor);
	}
}

void DebugDrawSystem::DrawUserNode(DAVA::Entity *entity)
{
	if(NULL != entity->GetComponent(DAVA::Component::USER_COMPONENT))
	{
        RenderManager::SetDynamicParam(PARAM_WORLD, &entity->GetWorldTransform(), (pointer_size)&entity->GetWorldTransform());
        
		AABBox3 worldBox = selSystem->GetSelectionAABox(entity);
		DAVA::float32 delta = worldBox.GetSize().Length() / 4;

		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0.5f, 0.5f, 1.0f, 0.3f));
		DAVA::RenderHelper::Instance()->FillBox(worldBox, debugDrawState);
		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0.2f, 0.2f, 0.8f, 1.0f));
		DAVA::RenderHelper::Instance()->DrawBox(worldBox, 1.0f, debugDrawState);

		// axises
		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0.7f, 0, 0, 1.0f));
		DAVA::RenderHelper::Instance()->DrawLine(DAVA::Vector3(0, 0, 0), DAVA::Vector3(delta, 0, 0), 1.0f, debugDrawState);
		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0, 0.7f, 0, 1.0f));
		DAVA::RenderHelper::Instance()->DrawLine(DAVA::Vector3(0, 0, 0), DAVA::Vector3(0, delta, 0), 1.0f, debugDrawState);
		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0, 0, 0.7f, 1.0f));
		DAVA::RenderHelper::Instance()->DrawLine(DAVA::Vector3(0, 0, 0), DAVA::Vector3(0, 0, delta), 1.0f, debugDrawState);
    }
}


void DebugDrawSystem::DrawStaticOcclusionComponent(DAVA::Entity *entity)
{
    StaticOcclusionComponent * staticOcclusionComponent = 0;
	if((staticOcclusionComponent = (StaticOcclusionComponent*)entity->GetComponent(DAVA::Component::STATIC_OCCLUSION_COMPONENT)) != 0)
	{
        Camera * camera = GetScene()->GetCurrentCamera();
       
		RenderManager::SetDynamicParam(PARAM_WORLD, &entity->GetWorldTransform(), (pointer_size)&entity->GetWorldTransform());
        
		AABBox3 localBox = staticOcclusionComponent->GetBoundingBox();
		DAVA::float32 delta = localBox.GetSize().Length() / 4;
        
		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0.1f, 0.5f, 0.1f, 0.3f));
		DAVA::RenderHelper::Instance()->FillBox(localBox, debugDrawState);
		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0.9f, 0.2f, 0.8f, 1.0f));
		DAVA::RenderHelper::Instance()->DrawBox(localBox, 1.0f, debugDrawState);
        
        Vector3 boxSize = localBox.GetSize();
        boxSize.x /= staticOcclusionComponent->GetSubdivisionsX();
        boxSize.y /= staticOcclusionComponent->GetSubdivisionsY();
        boxSize.z /= staticOcclusionComponent->GetSubdivisionsZ();
        
        // Draw block grid
        
        for (uint32 xs = 0; xs < staticOcclusionComponent->GetSubdivisionsX(); ++xs)
            for (uint32 ys = 0; ys < staticOcclusionComponent->GetSubdivisionsY(); ++ys)
                for (uint32 zs = 0; zs < staticOcclusionComponent->GetSubdivisionsZ(); ++zs)
                {
                    AABBox3 box(localBox.min + boxSize * Vector3(xs, ys, zs),
                                localBox.min + boxSize * Vector3(xs + 1, ys + 1, zs + 1) );
                    DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0.0f, 0.3f, 0.1f, 0.1f));
                    DAVA::RenderHelper::Instance()->DrawBox(box, 1.0f, debugDrawState);
                }
        
        
        const Vector3 & position = camera->GetPosition();
        
        AABBox3 worldBox;
        localBox.GetTransformedBox(entity->GetWorldTransform(), worldBox);
        
        if (worldBox.IsInside(position))
        {
            float32 xpf = (position.x - worldBox.min.x) / (worldBox.max.x - worldBox.min.x);
            float32 ypf = (position.y - worldBox.min.y) / (worldBox.max.y - worldBox.min.y);
            float32 zpf = (position.z - worldBox.min.z) / (worldBox.max.z - worldBox.min.z);
            
            uint32 xpi = (uint32)(xpf * (float32)staticOcclusionComponent->GetSubdivisionsX());
            uint32 ypi = (uint32)(ypf * (float32)staticOcclusionComponent->GetSubdivisionsY());
            uint32 zpi = (uint32)(zpf * (float32)staticOcclusionComponent->GetSubdivisionsZ());

            AABBox3 box(localBox.min + boxSize * Vector3(xpi, ypi, zpi),
                        localBox.min + boxSize * Vector3(xpi + 1, ypi + 1, zpi + 1));
            
            DAVA::RenderManager::Instance()->SetColor(DAVA::Color(1.0f, 0.3f, 0.1f, 0.1f));
            DAVA::RenderHelper::Instance()->FillBox(box, debugDrawState);
        }
        
		// axises
		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0.7f, 0, 0, 1.0f));
		DAVA::RenderHelper::Instance()->DrawLine(DAVA::Vector3(0, 0, 0), DAVA::Vector3(delta, 0, 0), 1.0f, debugDrawState);
		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0, 0.7f, 0, 1.0f));
		DAVA::RenderHelper::Instance()->DrawLine(DAVA::Vector3(0, 0, 0), DAVA::Vector3(0, delta, 0), 1.0f, debugDrawState);
		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0, 0, 0.7f, 1.0f));
		DAVA::RenderHelper::Instance()->DrawLine(DAVA::Vector3(0, 0, 0), DAVA::Vector3(0, 0, delta), 1.0f, debugDrawState);
	}
}

void DebugDrawSystem::DrawLightNode(DAVA::Entity *entity)
{
	DAVA::Light *light = GetLight(entity);
	if(NULL != light)
	{
        RenderManager::SetDynamicParam(PARAM_WORLD, &Matrix4::IDENTITY, (pointer_size) &Matrix4::IDENTITY);

        AABBox3 worldBox = selSystem->GetSelectionAABox(entity, entity->GetWorldTransform());

		if(light->GetType() == Light::TYPE_DIRECTIONAL)
		{
			DAVA::Vector3 center = worldBox.GetCenter();
			DAVA::Vector3 direction = -light->GetDirection();

			direction.Normalize();
			direction = direction * worldBox.GetSize().x;

			center -= (direction / 2);

			DAVA::RenderManager::Instance()->SetColor(DAVA::Color(1.0f, 1.0f, 0, 1.0f));
			DAVA::RenderHelper::Instance()->DrawArrow(center + direction, center, direction.Length() / 2, 1.0f, debugDrawState);
		}
		else if(light->GetType() == Light::TYPE_POINT)
		{
			DAVA::RenderManager::Instance()->SetColor(DAVA::Color(1.0f, 1.0f, 0, 0.3f));
			DAVA::RenderHelper::Instance()->FillDodecahedron(worldBox.GetCenter(), worldBox.GetSize().x / 2, debugDrawState);
			DAVA::RenderManager::Instance()->SetColor(DAVA::Color(1.0f, 1.0f, 0, 1.0f));
			DAVA::RenderHelper::Instance()->DrawDodecahedron(worldBox.GetCenter(), worldBox.GetSize().x / 2, 1.0f, debugDrawState);
		}
		else
		{
			DAVA::RenderManager::Instance()->SetColor(DAVA::Color(1.0f, 1.0f, 0, 0.3f));
			DAVA::RenderHelper::Instance()->FillBox(worldBox, debugDrawState);
			DAVA::RenderManager::Instance()->SetColor(DAVA::Color(1.0f, 1.0f, 0, 1.0f));
			DAVA::RenderHelper::Instance()->DrawBox(worldBox, 1.0f, debugDrawState);
		}
	}
}

void DebugDrawSystem::DrawSoundNode(DAVA::Entity *entity)
{
    SettingsManager * settings = SettingsManager::Instance();

    if(!settings->GetValue(Settings::Scene_Sound_SoundObjectDraw).AsBool())
        return;

    DAVA::SoundComponent * sc = GetSoundComponent(entity);
    if(sc)
    {
        RenderManager::SetDynamicParam(PARAM_WORLD, &Matrix4::IDENTITY, (pointer_size) &Matrix4::IDENTITY);
        AABBox3 worldBox = selSystem->GetSelectionAABox(entity, entity->GetWorldTransform());

        DAVA::RenderManager::Instance()->SetColor(settings->GetValue(Settings::Scene_Sound_SoundObjectBoxColor).AsColor());
        DAVA::RenderHelper::Instance()->FillBox(worldBox, debugDrawState);
    }
}

void DebugDrawSystem::DrawSelectedSoundNode(DAVA::Entity *entity)
{
    SettingsManager * settings = SettingsManager::Instance();

    if(!settings->GetValue(Settings::Scene_Sound_SoundObjectDraw).AsBool())
        return;

    DAVA::SoundComponent * sc = GetSoundComponent(entity);
    if(sc)
    {
        SceneEditor2 * sceneEditor = ((SceneEditor2 *)GetScene());

        RenderManager::SetDynamicParam(PARAM_WORLD, &entity->GetWorldTransform(), (pointer_size)&entity->GetWorldTransform());
        Vector3 position = entity->GetWorldTransform().GetTranslationVector();

        uint32 fontHeight = 0;
        GraphicsFont * debugTextFont = sceneEditor->textDrawSystem->GetFont();
        if(debugTextFont)
            fontHeight = debugTextFont->GetFontHeight();

        bool hasDirectionSound = false;
        uint32 eventsCount = sc->GetEventsCount();
        for(uint32 i = 0; i < eventsCount; ++i)
        {
            SoundEvent * sEvent = sc->GetSoundEvent(i);
            float32 distance = sEvent->GetMaxDistance();
            hasDirectionSound |= sEvent->IsDirectional();

            DAVA::RenderManager::Instance()->SetColor(settings->GetValue(Settings::Scene_Sound_SoundObjectSphereColor).AsColor());
            DAVA::RenderHelper::Instance()->FillSphere(Vector3(), distance, debugDrawState);

            sceneEditor->textDrawSystem->DrawText(sceneEditor->textDrawSystem->ToPos2d(position) - Vector2(0.f, fontHeight - 2.f) * i, sEvent->GetEventName(), Color::White, TextDrawSystem::Center);
        }

        if(hasDirectionSound)
        {
            DAVA::RenderManager::Instance()->SetColor(DAVA::Color(0.0f, 1.0f, 0.3f, 1.0f));
            DAVA::RenderHelper::Instance()->DrawArrow(Vector3(), sc->GetLocalDirection(), 10.f, 1.f, debugDrawState);
        }
    }
}

void DebugDrawSystem::DrawWindNode(DAVA::Entity *entity)
{
	WindComponent * wind = GetWindComponent(entity);
	if(wind)
	{
		RenderManager::SetDynamicParam(PARAM_WORLD, &Matrix4::IDENTITY, (pointer_size) &Matrix4::IDENTITY);
		const Matrix4 & worldMx = entity->GetWorldTransform();
		Vector3 worldPosition = worldMx.GetTranslationVector();

		DAVA::RenderManager::Instance()->SetColor(DAVA::Color(1.0f, 0.5f, 0.2f, 1.0f));
		DAVA::RenderHelper::Instance()->DrawArrow(worldPosition, worldPosition + wind->GetDirection() * 3.f, 10.f, 1.f, debugDrawState);
	}
}

void DebugDrawSystem::DrawEntityBox( DAVA::Entity *entity, const DAVA::Color &color )
{
    RenderManager::SetDynamicParam(PARAM_WORLD, &Matrix4::IDENTITY, (pointer_size) &Matrix4::IDENTITY);

    AABBox3 worldBox = selSystem->GetSelectionAABox(entity, entity->GetWorldTransform());

	DAVA::RenderManager::Instance()->SetColor(color);
	DAVA::RenderHelper::Instance()->DrawBox(worldBox, 1.0f, debugDrawState);
}


void DebugDrawSystem::DrawHangingObjects( DAVA::Entity *entity )
{
	if(!hangingObjectsModeEnabled)
	{
        return;
    }
    //skyBox should not be marked as hanging object
	if (entity->GetParent() != GetScene() || NULL != GetSkybox(entity))
    {
		return;
    }
    
	if(IsObjectHanging(entity))
	{
		DrawEntityBox(entity, Color(1.f, 0.f, 0.f, 1.f));
	}
}


bool DebugDrawSystem::IsObjectHanging(Entity * entity)
{
	RenderObject *ro = GetRenderObject(entity);
	if(!ro || (ro->GetType() != RenderObject::TYPE_MESH && ro->GetType() != RenderObject::TYPE_RENDEROBJECT && ro->GetType() != RenderObject::TYPE_SPEED_TREE))
		return false;

	const AABBox3 & worldBox = ro->GetWorldBoundingBox();
	if(worldBox.IsEmpty() && worldBox.min.x == worldBox.max.x && worldBox.min.y == worldBox.max.y && worldBox.min.z == worldBox.max.z)
		return false;

	const Matrix4 & wt = entity->GetWorldTransform();
	Vector3 position, scale, orientation;
	wt.Decomposition(position, scale, orientation);

	Vector<Vector3> lowestVertexes;
	GetLowestVertexes(ro, lowestVertexes, scale);

	const uint32 count = lowestVertexes.size();
	if(count == 0) return false; // we can be in state when selected lod had been set less than lodLayerNumber

	bool isAllVertextesUnderLandscape = true;
	for(uint32 i = 0; i < count && isAllVertextesUnderLandscape; ++i)
	{
		Vector3 pos = lowestVertexes[i];
		pos = pos * wt;

		const Vector3 landscapePoint = GetLandscapePointAtCoordinates(Vector2(pos.x, pos.y));

		bool isVertexUnderLandscape = ((pos.z - landscapePoint.z) < DAVA::EPSILON);
		isAllVertextesUnderLandscape &= isVertexUnderLandscape;
	}

	return !isAllVertextesUnderLandscape;
}

void DebugDrawSystem::GetLowestVertexes(const DAVA::RenderObject *ro, DAVA::Vector<DAVA::Vector3> &vertexes, const DAVA::Vector3 & scale)
{
	const float32 minZ = GetMinimalZ(ro);
	const float32 vertexDelta = HANGING_OBJECTS_HEIGHT / scale.z;

	uint32 count = ro->GetActiveRenderBatchCount();
	for(uint32 i = 0; i < count; ++i)
	{
		RenderBatch *batch = ro->GetActiveRenderBatch(i);
		DVASSERT(batch);

		PolygonGroup *pg = batch->GetPolygonGroup();
		if(pg)
		{
			uint32 vertexCount = pg->GetVertexCount();
			for(uint32 v = 0; v < vertexCount; ++v)
			{
				Vector3 pos;
				pg->GetCoord(v, pos);

				if((pos.z - minZ) <= vertexDelta)   //accuracy of finding of lowest vertexes
				{
					vertexes.push_back(pos);
				}
			}
		}
	}
}

float32 DebugDrawSystem::GetMinimalZ(const DAVA::RenderObject *ro)
{
	float32 minZ = AABBOX_INFINITY;
	
	uint32 count = ro->GetActiveRenderBatchCount();
	for(uint32 i = 0; i < count; ++i)
	{
		RenderBatch *batch = ro->GetActiveRenderBatch(i);
		DVASSERT(batch);

		PolygonGroup *pg = batch->GetPolygonGroup();
		if(pg)
		{
			uint32 vertexCount = pg->GetVertexCount();
			for(uint32 v = 0; v < vertexCount; ++v)
			{
				Vector3 pos;
				pg->GetCoord(v, pos);

				if(pos.z < minZ)
				{
					minZ = pos.z;
				}
			}
		}
	}

	return minZ;
}




Vector3 DebugDrawSystem::GetLandscapePointAtCoordinates(const Vector2 & centerXY)
{
	LandscapeEditorDrawSystem *landSystem = ((SceneEditor2 *)GetScene())->landscapeEditorDrawSystem;
	LandscapeProxy* landscape = landSystem->GetLandscapeProxy();

	if(landscape)
	{
		return landscape->PlacePoint(Vector3(centerXY));
	}

	return Vector3();
}

void DebugDrawSystem::DrawSwitchesWithDifferentLods( DAVA::Entity *entity )
{
    if(!switchesWithDifferentLodsEnabled) return;

    if(SceneValidator::IsEntityHasDifferentLODsCount(entity))
    {
        RenderManager::SetDynamicParam(PARAM_WORLD, &Matrix4::IDENTITY, (pointer_size) &Matrix4::IDENTITY);

        AABBox3 worldBox = selSystem->GetSelectionAABox(entity, entity->GetWorldTransform());

        DAVA::RenderManager::Instance()->SetColor(Color(1.0f, 0.f, 0.f, 1.f));
        DAVA::RenderHelper::Instance()->DrawBox(worldBox, 1.0f, debugDrawState);
    }
}
