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


#include "Platform/Qt/Win32/CorePlatformWin32.h"
#include "Platform/Qt/Win32/WindowsSpecifics.h"
#include "Platform/DeviceInfo.h"
#include "Platform/Thread.h"
#include "Utils/Utils.h"

#if defined(__DAVAENGINE_WIN32__)

#include <shellapi.h>

extern void FrameworkDidLaunched();
extern void FrameworkWillTerminate();

namespace DAVA 
{
int Core::Run(int argc, char * argv[], AppHandle handle)
{
	CoreWin32Platform * core = new CoreWin32Platform();
	core->CreateSingletons();
	core->InitArgs();

	return 0;
}

int Core::RunCmdTool(int argc, char * argv[], AppHandle handle)
{
	CoreWin32Platform * core = new CoreWin32Platform();

	core->EnableConsoleMode();
	core->CreateSingletons();

	core->InitArgs();

	Logger::Instance()->EnableConsoleMode();
	
	// DF-2274 - Get actual screen resolution and save it inside DeviceInfo
	int nScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	DeviceInfo::SetScreenInfo(nScreenWidth, nScreenHeight, 1);

	FrameworkDidLaunched();
	FrameworkWillTerminate();
	core->ReleaseSingletons();
#ifdef ENABLE_MEMORY_MANAGER
	if (DAVA::MemoryManager::Instance() != 0)
	{
		DAVA::MemoryManager::Instance()->FinalLog();
	}
#endif
	return 0;

}

void CoreWin32Platform::InitArgs()
{
	LPWSTR *szArglist;
	int nArgs;
	int i;
	szArglist = ::CommandLineToArgvW(::GetCommandLineW(), &nArgs);
	if( NULL == szArglist )
	{
		Logger::Error("CommandLineToArgvW failed\n");
		return;
	}
	else 
	{
		Vector<String> & cl = GetCommandLine();
		for( i=0; i<nArgs; i++)
		{
			WideString w = szArglist[i];
			String nonWide = WStringToString(w);
			cl.push_back(nonWide);
			Logger::FrameworkDebug("%d: %s\n", i, nonWide.c_str());
		}
	}
	// Free memory allocated for CommandLineToArgvW arguments.
	LocalFree(szArglist);
}


bool CoreWin32Platform::SetupWindow(HINSTANCE hInstance, HWND hWindow)
{
    needToSkipMouseUp = false;

	//single instance check
	TCHAR fileName[MAX_PATH];
	GetModuleFileName(NULL, fileName, MAX_PATH);
	fileName[MAX_PATH-1] = 0; //string can be not null-terminated on winXP
	for(int32 i = 0; i < MAX_PATH; ++i)
	{
		if(fileName[i] == L'\\') //symbol \ is not allowed in CreateMutex mutex name
		{
			fileName[i] = ' ';
		}
	}

	return true;
}



void CoreWin32Platform::Quit()
{
	PostQuitMessage(0);
}


static Vector<DAVA::UIEvent> activeTouches;
int32 MoveTouchsToVector(UINT message, WPARAM wParam, LPARAM lParam, Vector<UIEvent> *outTouches)
{
		
	int button = 0;
	if(message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_MOUSEMOVE || message == WM_LBUTTONDBLCLK)
	{
		button = 1;
	}
	else if(message == WM_RBUTTONDOWN || message == WM_RBUTTONUP || message == WM_RBUTTONDBLCLK)
	{
		button = 2;
	}
	else if(message == WM_MBUTTONDOWN || message == WM_MBUTTONUP)
	{
		button = 3;
	}

	int phase = UIEvent::PHASE_MOVE;
	if(message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN || message == WM_RBUTTONDBLCLK || message == WM_LBUTTONDBLCLK /*|| message == WM_XBUTTONDOWN*/)
	{
		phase = UIEvent::PHASE_BEGAN;
		//		NSLog(@"Event phase PHASE_BEGAN");
	}
	else if(message == WM_LBUTTONUP || message == WM_RBUTTONUP || message == WM_MBUTTONUP /*|| message == WM_XBUTTONUP*/)
	{
		phase = UIEvent::PHASE_ENDED;
		//		NSLog(@"Event phase PHASE_ENDED");
	}
	else if(LOWORD(wParam)&MK_LBUTTON || LOWORD(wParam)&MK_RBUTTON || LOWORD(wParam)&MK_MBUTTON /*|| LOWORD(wParam)&MK_XBUTTON1 || LOWORD(wParam)&MK_XBUTTON2*/)
	{
		phase = UIEvent::PHASE_DRAG;
	}

	if(phase == UIEvent::PHASE_DRAG)
	{
		for(Vector<DAVA::UIEvent>::iterator it = activeTouches.begin(); it != activeTouches.end(); it++)
		{
			Vector2 p((float32)GET_X_LPARAM(lParam), (float32)GET_Y_LPARAM(lParam));

			it->physPoint.x = p.x;
			it->physPoint.y = p.y;
			it->phase = phase;
		}
	}

	bool isFind = false;
	for(Vector<DAVA::UIEvent>::iterator it = activeTouches.begin(); it != activeTouches.end(); it++)
	{
		if(it->tid == button)
		{
			isFind = true;

			Vector2 p((float32)GET_X_LPARAM(lParam), (float32)GET_Y_LPARAM(lParam));

			it->physPoint.x = p.x;
			it->physPoint.y = p.y;
			it->phase = phase;
			//				it->timestamp = curEvent.timestamp;
			//				it->tapCount = curEvent.clickCount;
			it->phase = phase;

			break;
		}
	}

	if(!isFind)
	{
		UIEvent newTouch;
		newTouch.tid = button;
		Vector2 p((float32)GET_X_LPARAM(lParam), (float32)GET_Y_LPARAM(lParam));
		newTouch.physPoint.x = p.x;
		newTouch.physPoint.y = p.y;
		//			newTouch.timestamp = curEvent.timestamp;
		//			newTouch.tapCount = curEvent.clickCount;
		newTouch.phase = phase;
		activeTouches.push_back(newTouch);
	}

	for(Vector<DAVA::UIEvent>::iterator it = activeTouches.begin(); it != activeTouches.end(); it++)
	{
		outTouches->push_back(*it);
	}

	if(phase == UIEvent::PHASE_ENDED || phase == UIEvent::PHASE_MOVE)
	{
		for(Vector<DAVA::UIEvent>::iterator it = activeTouches.begin(); it != activeTouches.end(); it++)
		{
			if(it->tid == button)
			{
				activeTouches.erase(it);
				break;
			}
		}
	}
	return phase;
}


bool CoreWin32Platform::WinEvent(MSG *message, long *result)
{
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WHEEL_DELTA                     
#define WHEEL_DELTA 120
#endif

//		Logger::FrameworkDebug("Event: %d(%0x)", message->message, message->message);

	switch (message->message) 
	{
	case WM_KEYUP:
		{
			InputSystem::Instance()->GetKeyboard()->OnSystemKeyUnpressed((int32)message->wParam);

			// translate this to WM_CHAR message
			TranslateMessage(message);
		}
		break;

	case WM_KEYDOWN:
		{
			BYTE allKeys[256];
			GetKeyboardState(allKeys);

			Vector<DAVA::UIEvent> touches;
			Vector<DAVA::UIEvent> emptyTouches;

			for(Vector<DAVA::UIEvent>::iterator it = activeTouches.begin(); it != activeTouches.end(); it++)
			{
				touches.push_back(*it);
			}

			DAVA::UIEvent ev;
			ev.keyChar = 0;
			ev.phase = DAVA::UIEvent::PHASE_KEYCHAR;
			ev.tapCount = 1;
			ev.tid = InputSystem::Instance()->GetKeyboard()->GetDavaKeyForSystemKey((int32)message->wParam);

			touches.push_back(ev);

			UIControlSystem::Instance()->OnInput(0, emptyTouches, touches);
			touches.pop_back();
			UIControlSystem::Instance()->OnInput(0, emptyTouches, touches);

			InputSystem::Instance()->GetKeyboard()->OnSystemKeyPressed((int32)message->wParam);

			// translate this to WM_CHAR message
			TranslateMessage(message);
		}
		break;

	case WM_CHAR:
		{
			if(message->wParam > 27) //TODO: remove this elegant check
			{
				Vector<DAVA::UIEvent> touches;
				Vector<DAVA::UIEvent> emptyTouches;

				for(Vector<DAVA::UIEvent>::iterator it = activeTouches.begin(); it != activeTouches.end(); it++)
				{
					touches.push_back(*it);
				}

				DAVA::UIEvent ev;
				ev.keyChar = (char16)message->wParam;
				ev.phase = DAVA::UIEvent::PHASE_KEYCHAR;
				ev.tapCount = 1;
				ev.tid = 0;

				touches.push_back(ev);

				UIControlSystem::Instance()->OnInput(0, emptyTouches, touches);
				touches.pop_back();
				UIControlSystem::Instance()->OnInput(0, emptyTouches, touches);
			}

			*result = 0;
			return true;
		}
		break;

	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
		{
			Vector<DAVA::UIEvent> touches;
			Vector<DAVA::UIEvent> emptyTouches;

			int32 touchPhase = MoveTouchsToVector(message->message, message->wParam, message->lParam, &touches);

			UIControlSystem::Instance()->OnInput(touchPhase, emptyTouches, touches);

			touches.clear();
		}
		break;
	}

	return false;
}
    
void CoreWin32Platform::SetFocused(bool focused)
{
	if(isFocused != focused)
	{
		isFocused = focused;
	}
}

}
#endif // #if defined(__DAVAENGINE_WIN32__)