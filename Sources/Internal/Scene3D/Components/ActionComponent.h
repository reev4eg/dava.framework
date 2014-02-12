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



#ifndef __DAVAENGINE_ACTION_COMPONENT_H__
#define __DAVAENGINE_ACTION_COMPONENT_H__

#include "Entity/Component.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
	class Entity;
	class Scene;
	class ActionUpdateSystem;
	class ActionComponent : public Component
	{
	public:
		
		struct Action
		{
			enum eType
			{
				TYPE_NONE = 0,
				TYPE_PARTICLE_EFFECT,
				TYPE_SOUND
			};

			enum eEvent
			{
				EVENT_SWITCH_CHANGED = 0,
				EVENT_ADDED_TO_SCENE,

				EVENTS_COUNT
			};
			
			eType type;
			eEvent eventType;
			int32 switchIndex;
			float32 delay;
			String entityName;
			//VI: properties needed to control particle effect
			int32 stopAfterNRepeats;
			bool stopWhenEmpty;

			
			Action() : type(TYPE_NONE), eventType(EVENT_SWITCH_CHANGED), delay(0.0f), switchIndex(-1),
						stopAfterNRepeats(-1), stopWhenEmpty(false)
			{				
			}
			
			const Action& operator=(const Action& action)
			{
				type = action.type;
				eventType = action.eventType;
				delay = action.delay;
				entityName = action.entityName;
				switchIndex = action.switchIndex;
				stopAfterNRepeats = action.stopAfterNRepeats;
				stopWhenEmpty = action.stopWhenEmpty;
				
				return *this;
			}
			
			INTROSPECTION(Action,
						  NULL);

		};
	protected:
		virtual ~ActionComponent();
	public:
		ActionComponent();
		
		void StartSwitch(int32 switchIndex = -1);
		void StartAdd();
		bool IsStarted();
		void StopAll();
		void StopSwitch(int32 switchIndex = -1);
		
		void Add(ActionComponent::Action action);
		void Remove(const ActionComponent::Action& action);
		void Remove(const ActionComponent::Action::eType type, const String& entityName, const int switchIndex);
		uint32 GetCount();
		ActionComponent::Action& Get(uint32 index);
		
		void Update(float32 timeElapsed);
		
		virtual Component * Clone(Entity * toEntity);
		virtual void Serialize(KeyedArchive *archive, SerializationContext *serializationContext);
		virtual void Deserialize(KeyedArchive *archive, SerializationContext *serializationContext);
		
		static ActionComponent::Action MakeAction(ActionComponent::Action::eType type, String targetName, float32 delay);
		static ActionComponent::Action MakeAction(ActionComponent::Action::eType type, String targetName, float32 delay, int32 switchIndex);
		
		IMPLEMENT_COMPONENT_TYPE(ACTION_COMPONENT);
		
	private:
		
		void EvaluateAction(const Action& action);
		
		void OnActionParticleEffect(const Action& action);
		void OnActionSound(const Action& action);
		
		Entity* GetTargetEntity(const String& name, Entity* parent);
		
	private:
		
		struct ActionContainer
		{
			Action action;
			float32 timer;
			bool active;
			bool markedForUpdate;
			
			ActionContainer() : timer(0.0f), active(false), markedForUpdate(false)
			{
			}
			
			ActionContainer(const Action& srcAction) : timer(0.0f), active(false)
			{
				action = srcAction;
			}
			
			INTROSPECTION(ActionContainer,
						  NULL);
		};
		
		Vector<ActionComponent::ActionContainer> actions;
		bool started;
		bool allActionsActive; //skip processing when all actions are active
				
	public:
		
		INTROSPECTION_EXTEND(ActionComponent, Component,
							 COLLECTION(actions, "Actions Array",  I_VIEW | I_EDIT)
							 );

	};
};

#endif /* defined(__DAVAENGINE_ACTION_COMPONENT_H__) */
