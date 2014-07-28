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



#include "UI/UIControlSystem.h"
#include "UI/UIScreen.h"
#include "FileSystem/Logger.h"
#include "Render/RenderManager.h"
#include "Debug/DVAssert.h"
#include "Platform/SystemTimer.h"
#include "Debug/Replay.h"

namespace DAVA 
{
UIControlSystem::~UIControlSystem()
{
	SafeRelease(currentScreen); 
	SafeRelease(popupContainer);
}
	
UIControlSystem::UIControlSystem()
{
	screenLockCount = 0;
	frameSkip = 0;
	transitionType = 0;
	
	nextScreenTransition = 0;
	currentScreen = 0;
	nextScreen = 0;
	prevScreen = NULL;
	removeCurrentScreen = false;
    hovered = NULL;
    focusedControl = NULL;
	//mainControl = 0;

	popupContainer = new UIControl(Rect(0, 0, 1, 1));
	popupContainer->SetInputEnabled(false);
	
	exclusiveInputLocker = NULL;
	
	lockInputCounter = 0;
	
	baseGeometricData.position = Vector2(0, 0);
	baseGeometricData.size = Vector2(0, 0);
	baseGeometricData.pivotPoint = Vector2(0, 0);
	baseGeometricData.scale = Vector2(1.0f, 1.0f);
	baseGeometricData.angle = 0;

    ui3DViewCount = 0;
}
	
void UIControlSystem::SetScreen(UIScreen *_nextScreen, UIScreenTransition * _transition)
{
	if (_nextScreen == currentScreen)
	{
		if (nextScreen != 0)
		{
			SafeRelease(nextScreenTransition);
			SafeRelease(nextScreen);
		}
		return;
	}

	if (nextScreen)
	{
		Logger::Warning("2 screen switches during one frame.");
	}
    
	// 2 switches on one frame can cause memory leak
	SafeRelease(nextScreenTransition);
	SafeRelease(nextScreen);
    
	nextScreenTransition = SafeRetain(_transition);
	
	if (_nextScreen == 0)
	{
		removeCurrentScreen = true;
	}
	
	nextScreen = SafeRetain(_nextScreen);
}
	
	
void UIControlSystem::ReplaceScreen(UIScreen *newMainControl)
{
	prevScreen = currentScreen;
	currentScreen = newMainControl;
    NotifyListenersDidSwitch(currentScreen);
}

	
UIScreen *UIControlSystem::GetScreen()
{
	return currentScreen;	
}
	
void UIControlSystem::AddPopup(UIPopup *newPopup)
{
	for (Vector<UIPopup*>::iterator it = popupsToRemove.begin(); it != popupsToRemove.end(); it++)
	{
        if (*it == newPopup) 
        {
            popupsToRemove.erase(it);
            return;
        }
	}
	newPopup->LoadGroup();
	popupContainer->AddControl(newPopup);
}
	
void UIControlSystem::RemovePopup(UIPopup *popup)
{
	popupsToRemove.push_back(popup);
}
	
void UIControlSystem::RemoveAllPopups()
{
	const List<UIControl*> &totalChilds = popupContainer->GetChildren();
	for (List<UIControl*>::const_iterator it = totalChilds.begin(); it != totalChilds.end(); it++)
	{
		popupsToRemove.push_back((UIPopup *)*it);
	}
}
	
UIControl *UIControlSystem::GetPopupContainer()
{
	return popupContainer;
}

	
void UIControlSystem::Reset()
{
	SetScreen(0);
}
	
void UIControlSystem::ProcessScreenLogic()
{
	/*
	 if next screen or we need to removecurrent screen
	 */
	if (screenLockCount == 0 && (nextScreen || removeCurrentScreen))
	{
        UIScreen* nextScreenProcessed = 0;
        UIScreenTransition* transitionProcessed = 0;
        
        nextScreenProcessed = nextScreen;
        transitionProcessed = nextScreenTransition;
        nextScreen = 0; // functions called by this method can request another screen switch (for example, LoadResources)
        nextScreenTransition = 0;
        
		LockInput();
		
		CancelAllInputs();
		
        NotifyListenersWillSwitch(nextScreenProcessed);

		// If we have transition set
		if (transitionProcessed)
		{
			LockSwitch();

			// check if we have not loading transition
			if (!transitionProcessed->IsLoadingTransition())
			{
				// start transition and set currentScreen 
				transitionProcessed->StartTransition(currentScreen, nextScreenProcessed);
				currentScreen = transitionProcessed;
			}else
			{
				// if we got loading transition
				UILoadingTransition * loadingTransition = dynamic_cast<UILoadingTransition*> (transitionProcessed);
                DVASSERT(loadingTransition);

				// Firstly start transition
				loadingTransition->StartTransition(currentScreen, nextScreenProcessed);
				
				// Manage transfer to loading transition through InTransition of LoadingTransition
                if (loadingTransition->GetInTransition())
                {
                    loadingTransition->GetInTransition()->StartTransition(currentScreen, loadingTransition);
                    currentScreen = SafeRetain(loadingTransition->GetInTransition());
                }
                else 
                {
                    if(currentScreen)
                    {
                        currentScreen->SystemWillDisappear();
                        if ((nextScreenProcessed == 0) || (currentScreen->GetGroupId() != nextScreenProcessed->GetGroupId()))
                        {
                            currentScreen->UnloadGroup();
                        }
                        currentScreen->SystemDidDisappear();
                    }
                        // if we have next screen we load new resources, if it equal to zero we just remove screen
                    loadingTransition->LoadGroup();
                    loadingTransition->SystemWillAppear();
                    currentScreen = loadingTransition;
                    loadingTransition->SystemDidAppear();
                }
			}
		}
        else	// if there is no transition do change immediatelly
		{	
			// if we have current screen we call events, unload resources for it group
			if(currentScreen)
			{
				currentScreen->SystemWillDisappear();
				if ((nextScreenProcessed == 0) || (currentScreen->GetGroupId() != nextScreenProcessed->GetGroupId()))
				{
					currentScreen->UnloadGroup();
				}
				currentScreen->SystemDidDisappear();
			}
			// if we have next screen we load new resources, if it equal to zero we just remove screen
			if (nextScreenProcessed)
			{
				nextScreenProcessed->LoadGroup();
				nextScreenProcessed->SystemWillAppear();
			}
			currentScreen = nextScreenProcessed;
            NotifyListenersDidSwitch(currentScreen);
            if (nextScreenProcessed)
            {
				nextScreenProcessed->SystemDidAppear();
            }
			
			UnlockInput();
		}
		frameSkip = FRAME_SKIP;
		removeCurrentScreen = false;
	}
	
	/*
	 if we have popups to remove, we removes them here
	 */
	for (Vector<UIPopup*>::iterator it = popupsToRemove.begin(); it != popupsToRemove.end(); it++)
	{
		UIPopup *p = *it;
		if (p) 
		{
			p->Retain();
			popupContainer->RemoveControl(p);
			p->UnloadGroup();
            p->Release();
		}
	}
	popupsToRemove.clear();
}

void UIControlSystem::Update()
{
    updateCounter = 0;
	ProcessScreenLogic();
	
	float32 timeElapsed = SystemTimer::FrameDelta();

	if (RenderManager::Instance()->GetOptions()->IsOptionEnabled(RenderOptions::UPDATE_UI_CONTROL_SYSTEM))
	{
		if(currentScreen)
		{
			currentScreen->SystemUpdate(timeElapsed);
		}

		popupContainer->SystemUpdate(timeElapsed);
	}
	
	SafeRelease(prevScreen);
    //Logger::Info("UIControlSystem::updates: %d", updateCounter);
}
	
void UIControlSystem::Draw()
{
    drawCounter = 0;
    if (!ui3DViewCount)
    {
        UniqueHandle prevState = RenderManager::Instance()->currentState.stateHandle;
        RenderManager::Instance()->SetRenderState(RenderState::RENDERSTATE_3D_BLEND);
        RenderManager::Instance()->FlushState();            
        RenderManager::Instance()->Clear(Color(0,0,0,0), 1.0f, 0);        
        RenderManager::Instance()->SetRenderState(prevState);
    }
//	if(currentScreen && (!currentPopup || currentPopup->isTransparent))
	if (currentScreen)
	{
		currentScreen->SystemDraw(baseGeometricData);
//		currentScreen->SystemDraw(Rect(0, 0, RenderManager::Instance()->GetScreenWidth(), RenderManager::Instance()->GetScreenHeight()));
	}

	popupContainer->SystemDraw(baseGeometricData);
	
	if(frameSkip > 0)
	{
		frameSkip--;
	}
    //Logger::Info("UIControlSystem::draws: %d", drawCounter);
}
	
void UIControlSystem::SwitchInputToControl(int32 eventID, UIControl *targetControl)
{
	for (Vector<UIEvent>::iterator it = totalInputs.begin(); it != totalInputs.end(); it++) 
	{
		if((*it).tid == eventID)
		{
			CancelInput(&(*it));
			
			if(targetControl->IsPointInside((*it).point))
			{
				(*it).controlState = UIEvent::CONTROL_STATE_INSIDE;
				targetControl->touchesInside++;
			}
			else 
			{
				(*it).controlState = UIEvent::CONTROL_STATE_OUTSIDE;
			}
			(*it).touchLocker = targetControl;
			targetControl->currentInputID = eventID;
			if(targetControl->GetExclusiveInput())
			{
				SetExclusiveInputLocker(targetControl, eventID);
			}
			else 
			{
				SetExclusiveInputLocker(NULL, -1);
			}

			targetControl->totalTouches++;
		}
	}
}

	
void UIControlSystem::OnInput(int32 touchType, const Vector<UIEvent> &activeInputs, const Vector<UIEvent> &allInputs, bool fromReplay/* = false*/)
{
    inputCounter = 0;
	if(Replay::IsPlayback() && !fromReplay) return;
	if (lockInputCounter > 0)return;

	if(frameSkip <= 0)
	{
		if(Replay::IsRecord())
		{
			int32 count = (int32)activeInputs.size();
			Replay::Instance()->RecordEventsCount(count);
			for(Vector<UIEvent>::const_iterator it = activeInputs.begin(); it != activeInputs.end(); ++it) 
			{
				UIEvent ev = *it;
				RecalculatePointToVirtual(ev.physPoint, ev.point);
				Replay::Instance()->RecordEvent(&ev);
			}

			count = (int32)allInputs.size();
			Replay::Instance()->RecordEventsCount(count);
			for(Vector<UIEvent>::const_iterator it = allInputs.begin(); it != allInputs.end(); ++it) 
			{
				UIEvent ev = *it;
				RecalculatePointToVirtual(ev.physPoint, ev.point);
				Replay::Instance()->RecordEvent(&ev);
			}
		}

		//check all touches for active state
//		Logger::FrameworkDebug("IN   Active touches %d", activeInputs.size());
//		Logger::FrameworkDebug("IN   Total touches %d", allInputs.size());
		for (Vector<UIEvent>::iterator it = totalInputs.begin(); it != totalInputs.end(); it++) 
		{
			(*it).activeState = UIEvent::ACTIVITY_STATE_INACTIVE;
			
			for (Vector<UIEvent>::const_iterator wit = activeInputs.begin(); wit != activeInputs.end(); wit++) 
			{
				if((*it).tid == (*wit).tid)
				{
					if((*it).phase == (*wit).phase && (*it).physPoint == (*wit).physPoint)
					{
						(*it).activeState = UIEvent::ACTIVITY_STATE_ACTIVE;
					}
					else 
					{
						(*it).activeState = UIEvent::ACTIVITY_STATE_CHANGED;
					}
					
					(*it).phase = (*wit).phase;
					(*it).timestamp = (*wit).timestamp;
					(*it).physPoint = (*wit).physPoint;
					RecalculatePointToVirtual((*it).physPoint, (*it).point);
					(*it).tapCount = (*wit).tapCount;
					(*it).inputHandledType = (*wit).inputHandledType;
					break;
				}
			}
			if((*it).activeState == UIEvent::ACTIVITY_STATE_INACTIVE)
			{
				for (Vector<UIEvent>::const_iterator wit = allInputs.begin(); wit != allInputs.end(); wit++) 
				{
					if((*it).tid == (*wit).tid)
					{
						if((*it).phase == (*wit).phase && (*it).point == (*wit).point)
						{
							(*it).activeState = UIEvent::ACTIVITY_STATE_ACTIVE;
						}
						else 
						{
							(*it).activeState = UIEvent::ACTIVITY_STATE_CHANGED;
						}
						
						(*it).phase = (*wit).phase;
						(*it).timestamp = (*wit).timestamp;
						(*it).physPoint = (*wit).physPoint;
						(*it).point = (*wit).point;
						RecalculatePointToVirtual((*it).physPoint, (*it).point);
						(*it).tapCount = (*wit).tapCount;
						(*it).inputHandledType = (*wit).inputHandledType;
						break;
					}
				}
			}
		}
		
		//add new touches
		for (Vector<UIEvent>::const_iterator wit = activeInputs.begin(); wit != activeInputs.end(); wit++) 
		{
			bool isFind = FALSE;
			for (Vector<UIEvent>::iterator it = totalInputs.begin(); it != totalInputs.end(); it++) 
			{
				if((*it).tid == (*wit).tid)
				{
					isFind = TRUE;
                    break;
				}
			}
			if(!isFind)
			{
				totalInputs.push_back((*wit));
                
                Vector<UIEvent>::reference curr(totalInputs.back());
				curr.activeState = UIEvent::ACTIVITY_STATE_CHANGED;
                //curr.phase = UIEvent::PHASE_BEGAN;
				RecalculatePointToVirtual(curr.physPoint, curr.point);
			}
		}
		for (Vector<UIEvent>::const_iterator wit = allInputs.begin(); wit != allInputs.end(); wit++) 
		{
			bool isFind = FALSE;
			for (Vector<UIEvent>::iterator it = totalInputs.begin(); it != totalInputs.end(); it++) 
			{
				if((*it).tid == (*wit).tid)
				{
					isFind = TRUE;
                    break;
				}
			}
			if(!isFind)
			{
				totalInputs.push_back((*wit));
                
                Vector<UIEvent>::reference curr(totalInputs.back());
				curr.activeState = UIEvent::ACTIVITY_STATE_CHANGED;
				RecalculatePointToVirtual(curr.physPoint, curr.point);
			}
		}
		
		//removes inactive touches and cancelled touches
		for (Vector<UIEvent>::iterator it = totalInputs.begin(); it != totalInputs.end();)
		{
			if((*it).activeState == UIEvent::ACTIVITY_STATE_INACTIVE || (*it).phase == UIEvent::PHASE_CANCELLED)
			{
                if ((*it).phase != UIEvent::PHASE_ENDED)
                {
                    CancelInput(&(*it));
                }
				totalInputs.erase(it);
				it = totalInputs.begin();
				if(it == totalInputs.end())
				{
					break;
				}
                continue;
			}
            it++;
		}
		
//		Logger::FrameworkDebug("Total touches %d", totalInputs.size());
//		for (Vector<UIEvent>::iterator it = totalInputs.begin(); it != totalInputs.end(); it++)
//		{
//			Logger::FrameworkDebug("		ID %d", (*it).tid);
//			Logger::FrameworkDebug("		phase %d", (*it).phase);
//		}


		if(currentScreen)
		{
			for(Vector<UIEvent>::iterator it = totalInputs.begin(); it != totalInputs.end(); it++) 
			{
				if((*it).activeState == UIEvent::ACTIVITY_STATE_CHANGED)
				{
					if(!popupContainer->SystemInput(&(*it)))
					{
						currentScreen->SystemInput(&(*it));
					}
				}
				if(totalInputs.empty())
				{
					break;
				}
			}
		}
	}
    //Logger::Info("UIControlSystem::inputs: %d", inputCounter);
}

void UIControlSystem::OnInput(UIEvent * event)
{
	if(currentScreen)
	{
		if(!popupContainer->SystemInput(event))
		{
			currentScreen->SystemInput(event);
		}
	}
}
void UIControlSystem::CancelInput(UIEvent *touch)
{
	if(touch->touchLocker)
	{
		touch->touchLocker->SystemInputCancelled(touch);
	}
	if (touch->touchLocker != currentScreen)
	{
		currentScreen->SystemInputCancelled(touch);
	}
}
void UIControlSystem::CancelAllInputs()
{
	for (Vector<UIEvent>::iterator it = totalInputs.begin(); it != totalInputs.end(); it++) 
	{
		CancelInput(&(*it));
	}
	totalInputs.clear();
}

void UIControlSystem::CancelInputs(UIControl *control, bool hierarchical)
{
	for (Vector<UIEvent>::iterator it = totalInputs.begin(); it != totalInputs.end(); it++) 
	{
        if (!hierarchical)
        {
            if (it->touchLocker == control)
            {
                CancelInput(&(*it));
                break;
            }
            continue;
        }
        UIControl * parentLockerControl = it->touchLocker;
        while(parentLockerControl)
        {
            if(control == parentLockerControl)
            {
                CancelInput(&(*it));
                break;
            }
            parentLockerControl = parentLockerControl->GetParent();
        }
	}
}

//void UIControlSystem::SetTransitionType(int newTransitionType)
//{
//	transitionType = newTransitionType;
//}
	
int32 UIControlSystem::LockInput()
{
	lockInputCounter++;
	if (lockInputCounter > 0)
	{
		CancelAllInputs();
	}
	return lockInputCounter;
}

int32 UIControlSystem::UnlockInput()
{
	DVASSERT(lockInputCounter != 0);

	lockInputCounter--;
	if (lockInputCounter == 0)
	{
		// VB: Done that because hottych asked to do that.
		CancelAllInputs();
	}
	return lockInputCounter;
}
	
int32 UIControlSystem::GetLockInputCounter() const
{
	return lockInputCounter;
}

const Vector<UIEvent> & UIControlSystem::GetAllInputs()
{
	return totalInputs;
}
	
void UIControlSystem::SetExclusiveInputLocker(UIControl *locker, int32 lockEventId)
{
	SafeRelease(exclusiveInputLocker);
    if (locker != NULL)
    {
        for (Vector<UIEvent>::iterator it = totalInputs.begin(); it != totalInputs.end(); it++)
        {
            if (it->tid != lockEventId && it->touchLocker != locker)
            {//cancel all inputs excepts current input and inputs what allready handles by this locker.
                CancelInput(&(*it));
            }
        }
    }

	exclusiveInputLocker = SafeRetain(locker);
}
	
UIControl *UIControlSystem::GetExclusiveInputLocker()
{
	return exclusiveInputLocker;
}
    
void UIControlSystem::ScreenSizeChanged()
{
    popupContainer->SystemScreenSizeDidChanged(Rect(Core::Instance()->GetVirtualScreenXMin()
                                                 , Core::Instance()->GetVirtualScreenYMin()
                                                 , Core::Instance()->GetVirtualScreenXMax() - Core::Instance()->GetVirtualScreenXMin()
                                                 , Core::Instance()->GetVirtualScreenYMax() - Core::Instance()->GetVirtualScreenYMin()));
}

void UIControlSystem::SetHoveredControl(UIControl *newHovered)
{
    if (hovered != newHovered) 
    {
        if (hovered) 
        {
            hovered->SystemDidRemoveHovered();
            hovered->Release();
        }
        hovered = SafeRetain(newHovered);
        if (hovered) 
        {
            hovered->SystemDidSetHovered();
        }
    }
}
    
UIControl *UIControlSystem::GetHoveredControl(UIControl *newHovered)
{
    return hovered;
}
    
void UIControlSystem::SetFocusedControl(UIControl *newFocused, bool forceSet)
{
    if (focusedControl)
    {
        if (forceSet || focusedControl->IsLostFocusAllowed(newFocused)) 
        {
            focusedControl->SystemOnFocusLost(newFocused);
            SafeRelease(focusedControl);
            focusedControl = SafeRetain(newFocused);
            if (focusedControl) 
            {
                focusedControl->SystemOnFocused();
            }
        }
    }
    else 
    {
        focusedControl = SafeRetain(newFocused);
        if (focusedControl) 
        {
            focusedControl->SystemOnFocused();
        }
    }

}
    
UIControl *UIControlSystem::GetFocusedControl()
{
    return focusedControl;
}
    

	
const UIGeometricData &UIControlSystem::GetBaseGeometricData()
{
	return baseGeometricData;	
}
	
void UIControlSystem::SetInputScreenAreaSize(int32 width, int32 height)
{
    inputWidth = width;
    inputHeight = height;
}

void UIControlSystem::CalculateScaleMultipliers()
{
	
	float32 w, h;
	w = (float32)Core::Instance()->GetVirtualScreenWidth() / (float32)inputWidth;
	h = (float32)Core::Instance()->GetVirtualScreenHeight() / (float32)inputHeight;
	inputOffset.x = inputOffset.y = 0;
	if(w > h)
	{
		scaleFactor = w;
		inputOffset.y = 0.5f * ((float32)Core::Instance()->GetVirtualScreenHeight() - (float32)inputHeight * scaleFactor);
	}
	else
	{
		scaleFactor = h;
		inputOffset.x = 0.5f * ((float32)Core::Instance()->GetVirtualScreenWidth() - (float32)inputWidth * scaleFactor);
	}
}

void UIControlSystem::RecalculatePointToPhysical(const Vector2 &virtualPoint, Vector2 &physicalPoint)
{
    Vector2 calcPoint(virtualPoint);
    
    calcPoint -= inputOffset;
    calcPoint /= scaleFactor;
    
    physicalPoint = calcPoint;
}

void UIControlSystem::RecalculatePointToVirtual(const Vector2 &physicalPoint, Vector2 &virtualPoint)
{
	if(Replay::IsPlayback())
	{
		return;
	}

	
    virtualPoint = physicalPoint;
	
	virtualPoint *= scaleFactor;
	virtualPoint += inputOffset;
}

void UIControlSystem::ReplayEvents()
{
	while(Replay::Instance()->IsEvent())
	{
		int32 activeCount = Replay::Instance()->PlayEventsCount();
		Vector<UIEvent> activeInputs;
		while(activeCount--)
		{
			UIEvent ev = Replay::Instance()->PlayEvent();
			activeInputs.push_back(ev);
		}

		int32 allCount = Replay::Instance()->PlayEventsCount();
		Vector<UIEvent> allInputs;
		while(allCount--)
		{
			UIEvent ev = Replay::Instance()->PlayEvent();
			allInputs.push_back(ev);
		}

		if(activeCount || allCount)
		{
			OnInput(0, activeInputs, allInputs, true);
		}
	}
}

int32 UIControlSystem::LockSwitch()
{
	screenLockCount++;
	return screenLockCount;
}

int32 UIControlSystem::UnlockSwitch()
{
	screenLockCount--;
	DVASSERT(screenLockCount >= 0);
	return screenLockCount;
}

void UIControlSystem::AddScreenSwitchListener(ScreenSwitchListener * listener)
{
	screenSwitchListeners.push_back(listener);
}

void UIControlSystem::RemoveScreenSwitchListener(ScreenSwitchListener * listener)
{
	Vector<ScreenSwitchListener *>::iterator it = std::find(screenSwitchListeners.begin(), screenSwitchListeners.end(), listener);
	if(it != screenSwitchListeners.end())
		screenSwitchListeners.erase(it);
}

void UIControlSystem::NotifyListenersWillSwitch( UIScreen* screen )
{
    Vector<ScreenSwitchListener*> screenSwitchListenersCopy = screenSwitchListeners;
    uint32 listenersCount = screenSwitchListenersCopy.size();
    for(uint32 i = 0; i < listenersCount; ++i)
        screenSwitchListenersCopy[i]->OnScreenWillSwitch( screen );
}

void UIControlSystem::NotifyListenersDidSwitch( UIScreen* screen )
{
    Vector<ScreenSwitchListener*> screenSwitchListenersCopy = screenSwitchListeners;
    uint32 listenersCount = screenSwitchListenersCopy.size();
    for(uint32 i = 0; i < listenersCount; ++i)
        screenSwitchListenersCopy[i]->OnScreenDidSwitch( screen );
}


void UIControlSystem::UI3DViewAdded()
{
    ui3DViewCount++;
}
void UIControlSystem::UI3DViewRemoved()
{
    DVASSERT(ui3DViewCount);
    ui3DViewCount--;
}

};
