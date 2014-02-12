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



#include "Scene/System/HoodSystem.h"
#include "Scene/System/ModifSystem.h"
#include "Scene/System/CollisionSystem.h"
#include "Scene/System/CameraSystem.h"
#include "Scene/System/SelectionSystem.h"
#include "Scene/SceneEditor2.h"

HoodSystem::HoodSystem(DAVA::Scene * scene, SceneCameraSystem *camSys)
	: DAVA::SceneSystem(scene)
	, cameraSystem(camSys)
	, curMode(ST_MODIF_OFF)
	, moseOverAxis(ST_AXIS_NONE)
    , curScale(1.0f)
	, curHood(NULL)
	, moveHood()
	, lockedScale(false)
	, lockedModif(false)
	, lockedAxis(false)
	, isVisible(true)
{
	btVector3 worldMin(-1000,-1000,-1000);
	btVector3 worldMax(1000,1000,1000);

	collConfiguration = new btDefaultCollisionConfiguration();
	collDispatcher = new btCollisionDispatcher(collConfiguration);
	collBroadphase = new btAxisSweep3(worldMin,worldMax);
	collDebugDraw = new SceneCollisionDebugDrawer();
	collDebugDraw->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
	collWorld = new btCollisionWorld(collDispatcher, collBroadphase, collConfiguration);
	collWorld->setDebugDrawer(collDebugDraw);

	SetModifAxis(ST_AXIS_X);
	SetModifMode(ST_MODIF_MOVE);

	moveHood.colorX = DAVA::Color(1, 0, 0, 1);
	moveHood.colorY = DAVA::Color(0, 1, 0, 1);
	moveHood.colorZ = DAVA::Color(0, 0, 1, 1);
	moveHood.colorS = DAVA::Color(1, 1, 0, 1);

	rotateHood.colorX = DAVA::Color(1, 0, 0, 1);
	rotateHood.colorY = DAVA::Color(0, 1, 0, 1);
	rotateHood.colorZ = DAVA::Color(0, 0, 1, 1);
	rotateHood.colorS = DAVA::Color(1, 1, 0, 1);

	scaleHood.colorX = DAVA::Color(1, 0, 0, 1);
	scaleHood.colorY = DAVA::Color(0, 1, 0, 1);
	scaleHood.colorZ = DAVA::Color(0, 0, 1, 1);
	scaleHood.colorS = DAVA::Color(1, 1, 0, 1);

	normalHood.colorX = DAVA::Color(0.7f, 0.3f, 0.3f, 1);
	normalHood.colorY = DAVA::Color(0.3f, 0.7f, 0.3f, 1);
	normalHood.colorZ = DAVA::Color(0.3f, 0.3f, 0.7f, 1);
	normalHood.colorS = DAVA::Color(0, 0, 0, 1);
}

HoodSystem::~HoodSystem()
{
	delete collWorld;
	delete collDebugDraw;
	delete collBroadphase;
	delete collDispatcher;
	delete collConfiguration;
}

DAVA::Vector3 HoodSystem::GetPosition() const
{
	return (curPos + modifOffset);
}

void HoodSystem::SetPosition(const DAVA::Vector3 &pos)
{
	if(!IsLocked() && !lockedScale)
	{
		if(curPos != pos || !modifOffset.IsZero())
		{
			curPos = pos;
			ResetModifValues();

			if(NULL != curHood)
			{
				curHood->UpdatePos(curPos);
				normalHood.UpdatePos(curPos);
			}
		}
	}
}

void HoodSystem::SetModifOffset(const DAVA::Vector3 &offset)
{
	if(!IsLocked())
	{
		moveHood.modifOffset = offset;

		if(modifOffset != offset)
		{
			modifOffset = offset;

			if(NULL != curHood)
			{
				curHood->UpdatePos(curPos + modifOffset);
				normalHood.UpdatePos(curPos + modifOffset);
			}
		}
	}
}

void HoodSystem::SetModifRotate(const DAVA::float32 &angle)
{
	if(!IsLocked())
	{
		rotateHood.modifRotate = angle;
	}
}

void HoodSystem::SetModifScale(const DAVA::float32 &scale)
{
	if(!IsLocked())
	{
		scaleHood.modifScale = scale;
	}
}

void HoodSystem::SetScale(DAVA::float32 scale)
{
	if(!IsLocked())
	{
		if(curScale != scale && 0 != scale)
		{
			curScale = scale;

			if(NULL != curHood)
			{
				curHood->UpdateScale(curScale);
				normalHood.UpdateScale(curScale);

				collWorld->updateAabbs();
			}
		}
	}
}

DAVA::float32 HoodSystem::GetScale() const
{
	return curScale;
}

void HoodSystem::SetModifMode(ST_ModifMode mode)
{
	if(!IsLocked())
	{
		if(curMode != mode)
		{
			if(NULL != curHood)
			{
				RemCollObjects(&curHood->collObjects);
			}

			curMode = mode;
			switch (mode)
			{
			case ST_MODIF_MOVE:
				curHood = &moveHood;
				break;
			case ST_MODIF_SCALE:
				curHood = &scaleHood;
				break;
			case ST_MODIF_ROTATE:
				curHood = &rotateHood;
				break;
			default:
				curHood = &normalHood;
				break;
			}

			if(NULL != curHood)
			{
				AddCollObjects(&curHood->collObjects);

				curHood->UpdatePos(curPos + modifOffset);
				curHood->UpdateScale(curScale);
			}

			collWorld->updateAabbs();
		}
	}
}

ST_ModifMode HoodSystem::GetModifMode() const
{
	if(lockedModif)
	{
		return ST_MODIF_OFF;
	}

	return curMode;
}

void HoodSystem::SetVisible(bool visible)
{
	if(!IsLocked())
	{
		isVisible = visible;
	}
}

bool HoodSystem::IsVisible() const
{
	return isVisible;
}

void HoodSystem::AddCollObjects(const DAVA::Vector<HoodCollObject*>* objects)
{
	if(NULL != objects)
	{
		for (size_t i = 0; i < objects->size(); ++i)
		{
			collWorld->addCollisionObject(objects->operator[](i)->btObject);
		}
	}
}

void HoodSystem::RemCollObjects(const DAVA::Vector<HoodCollObject*>* objects)
{
	if(NULL != objects)
	{
		for (size_t i = 0; i < objects->size(); ++i)
		{
			collWorld->removeCollisionObject(objects->operator[](i)->btObject);
		}
	}
}

void HoodSystem::ResetModifValues()
{
	modifOffset = DAVA::Vector3(0, 0, 0);

	rotateHood.modifRotate = 0;
	scaleHood.modifScale = 0;
}

void HoodSystem::Update(float timeElapsed)
{
	if(!IsLocked() && !lockedScale)
	{
		// scale hood depending on current camera position
		DAVA::Vector3 camPosition = cameraSystem->GetCameraPosition();
		SetScale((GetPosition() - camPosition).Length() / 20.f);
	}
}

void HoodSystem::ProcessUIEvent(DAVA::UIEvent *event)
{
	if(!event->point.IsZero())
	{
		// before checking result mark that there is no hood axis under mouse
		if(!lockedScale && !lockedAxis)
		{
			moseOverAxis = ST_AXIS_NONE;

			// if is visible and not locked check mouse over status
			if(!lockedModif && NULL != curHood)
			{
				// get intersected items in the line from camera to current mouse position
				DAVA::Vector3 camPosition = cameraSystem->GetCameraPosition();
				DAVA::Vector3 camToPointDirection = cameraSystem->GetPointDirection(event->point);
				DAVA::Vector3 traceTo = camPosition + camToPointDirection * 1000.0f;

				btVector3 btFrom(camPosition.x, camPosition.y, camPosition.z);
				btVector3 btTo(traceTo.x, traceTo.y, traceTo.z);

				btCollisionWorld::AllHitsRayResultCallback btCallback(btFrom, btTo);
				collWorld->rayTest(btFrom, btTo, btCallback);

				if(btCallback.hasHit())
				{
					const DAVA::Vector<HoodCollObject*>* curHoodObjects = &curHood->collObjects;
					for(size_t i = 0; i < curHoodObjects->size(); ++i)
					{
						HoodCollObject *hObj = curHoodObjects->operator[](i);

						if(hObj->btObject == btCallback.m_collisionObjects[0])
						{
							// mark that mouse is over one of hood axis
							moseOverAxis = hObj->axis;
							break;
						}
					}
				}
			}
		}
	}
}

void HoodSystem::Draw()
{
	if(NULL != curHood && IsVisible())
	{
		TextDrawSystem *textDrawSys = ((SceneEditor2 *) GetScene())->textDrawSystem;

		// modification isn't locked and whole system isn't locked
		if(!IsLocked() && !lockedModif)
		{
			ST_Axis showAsSelected = curAxis;

			if(curMode != ST_MODIF_OFF)
			{
				if(ST_AXIS_NONE != moseOverAxis)
				{
					showAsSelected = moseOverAxis;
				}
			}

			curHood->Draw(showAsSelected, moseOverAxis, textDrawSys);

			// zero pos point
			DAVA::RenderManager::Instance()->SetColor(DAVA::Color(1.0f, 1.0f, 1.0f, 1.0f));
			DAVA::RenderHelper::Instance()->DrawPoint(GetPosition(), 1.0f, DAVA::RenderState::RENDERSTATE_2D_BLEND);
			
			// debug draw axis collision word
			//collWorld->debugDrawWorld();
		}
		else
		{
			normalHood.Draw(curAxis, ST_AXIS_NONE, textDrawSys);
		}
	}
}

void HoodSystem::ProcessCommand(const Command2 *command, bool redo)
{

}

void HoodSystem::SetModifAxis(ST_Axis axis)
{
	if(ST_AXIS_NONE != axis)
	{
		curAxis = axis;
	}
}

ST_Axis HoodSystem::GetModifAxis() const
{
	return curAxis;
}

ST_Axis HoodSystem::GetPassingAxis() const
{
	return moseOverAxis;
}

void HoodSystem::LockScale(bool lock)
{
	lockedScale = lock;
}

void HoodSystem::LockModif(bool lock)
{
	lockedModif = lock;
}

void HoodSystem::LockAxis(bool lock)
{
	lockedAxis = lock;
}