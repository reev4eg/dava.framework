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


#include "Platform/TemplateWin32/CorePlatformWin32.h"
#include "Platform/TemplateWin32/WindowsSpecifics.h"
#include "Platform/Thread.h"
#include "Platform/DeviceInfo.h"
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
		bool windowCreated = core->CreateWin32Window(handle);
		if(windowCreated)
		{
			core->InitArgs();
			core->Run();
			core->ReleaseSingletons();
			
#ifdef ENABLE_MEMORY_MANAGER
			if (DAVA::MemoryManager::Instance() != 0)
			{
				DAVA::MemoryManager::Instance()->FinalLog();
			}
#endif
		}

		CloseHandle(core->hMutex);
		return 0;
	
	}

	/*const Vector2 & Core::GetMouseLocation();
	{
		POINT pt;
		GetCursorPos(&pt);
		
	}*/
    
	int Core::RunCmdTool(int argc, char * argv[], AppHandle handle)
	{
		CoreWin32Platform * core = new CoreWin32Platform();

		//core->CreateWin32Window(handle);
		//core->Run();
		core->EnableConsoleMode();
		core->CreateSingletons();

		core->InitArgs();

		Logger::Instance()->EnableConsoleMode();
		
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

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	bool CoreWin32Platform::CreateWin32Window(HINSTANCE hInstance)
	{	
		this->hInstance = hInstance;

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
        SetLastError(0);
		hMutex = CreateMutex(NULL, FALSE, fileName);
		if(ERROR_ALREADY_EXISTS == GetLastError())
		{
			return false;
		}

		windowedMode = DisplayMode(800, 600, 16, 0);
		fullscreenMode = DisplayMode(800, 600, 16, 0);
		currentMode = windowedMode;
		isFullscreen = false;

		// create the window, only if we do not use the null device
		LPCWSTR className = L"DavaFrameworkWindowsDevice";

		// Register Class

		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX); 
		wcex.style			= CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= (WNDPROC)WndProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= hInstance;
		wcex.hIcon			= 0;
		wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
		wcex.lpszMenuName	= 0;
		wcex.lpszClassName	= className;
		wcex.hIconSm		= 0;

		RegisterClassEx(&wcex);

		// calculate client size

		RECT clientSize;
		clientSize.top = 0;
		clientSize.left = 0;
		clientSize.right = currentMode.width;
		clientSize.bottom = currentMode.height;

		ULONG style = WINDOWED_STYLE | WS_CLIPCHILDREN;

		// Create the rendering window
		if (isFullscreen)
		{
			style  = WS_VISIBLE | WS_POPUP;
		} // End if Fullscreen


		AdjustWindowRect(&clientSize, style, FALSE);

		int32 realWidth = clientSize.right - clientSize.left;
		int32 realHeight = clientSize.bottom - clientSize.top;

		int32 windowLeft = -10000;//(GetSystemMetrics(SM_CXSCREEN) - realWidth) / 2;
		int32 windowTop = -10000;//(GetSystemMetrics(SM_CYSCREEN) - realHeight) / 2;

		if (isFullscreen)
		{
			windowLeft = 0;
			windowTop = 0;
		}

		// create window
		hWindow = CreateWindow( className, L"", style, windowLeft, windowTop, 
			realWidth, realHeight,	NULL, NULL, hInstance, NULL);

		ShowWindow(hWindow, SW_SHOW);
		UpdateWindow(hWindow);

		// fix ugly ATI driver bugs. Thanks to ariaci (Taken from Irrlight).
		MoveWindow(hWindow, windowLeft, windowTop, realWidth, realHeight, TRUE);
	
#if defined(__DAVAENGINE_DIRECTX9__)
		RenderManager::Create(Core::RENDERER_DIRECTX9);
#elif defined(__DAVAENGINE_OPENGL__)
		RenderManager::Create(Core::RENDERER_OPENGL);
#endif
		RenderManager::Instance()->Create(hInstance, hWindow);
		// DF-2274 - Get actual screen resolution and save it inside DeviceInfo
		int nScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
		int nScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

		DeviceInfo::SetScreenInfo(nScreenWidth, nScreenHeight, 1);

		FrameworkDidLaunched();
		KeyedArchive * options = Core::GetOptions();

		//fullscreenMode = GetCurrentDisplayMode();
		fullscreenMode = GetCurrentDisplayMode();//FindBestMode(fullscreenMode);
		if (options)
		{
			windowedMode.width = options->GetInt32("width");
			windowedMode.height = options->GetInt32("height");
			windowedMode.bpp = options->GetInt32("bpp");
			
			// get values from config in case if they are available
			fullscreenMode.width = options->GetInt32("fullscreen.width", fullscreenMode.width);
			fullscreenMode.height = options->GetInt32("fullscreen.height", fullscreenMode.height);
			fullscreenMode.bpp = windowedMode.bpp;

			fullscreenMode = FindBestMode(fullscreenMode);

			isFullscreen = (0 != options->GetInt32("fullscreen"));	
			String title = options->GetString("title", "[set application title using core options property 'title']");
			WideString titleW = StringToWString(title);
			SetWindowText(hWindow, titleW.c_str());
		}

		Logger::FrameworkDebug("[PlatformWin32] best display fullscreen mode matched: %d x %d x %d refreshRate: %d", fullscreenMode.width, fullscreenMode.height, fullscreenMode.bpp, fullscreenMode.refreshRate);

		currentMode = windowedMode;
		if (isFullscreen)
		{
			currentMode = fullscreenMode;
		}

		clientSize.top = 0;
		clientSize.left = 0;
		clientSize.right = currentMode.width;
		clientSize.bottom = currentMode.height;

		AdjustWindowRect(&clientSize, style, FALSE);

		realWidth = clientSize.right - clientSize.left;
		realHeight = clientSize.bottom - clientSize.top;

		windowLeft = (GetSystemMetrics(SM_CXSCREEN) - realWidth) / 2;
		windowTop = (GetSystemMetrics(SM_CYSCREEN) - realHeight) / 2;
		MoveWindow(hWindow, windowLeft, windowTop, realWidth, realHeight, TRUE);
	
        RAWINPUTDEVICE Rid;

        Rid.usUsagePage = 0x01; 
        Rid.usUsage = 0x02; 
        Rid.dwFlags = 0;
        Rid.hwndTarget = 0;

        RegisterRawInputDevices(&Rid, 1, sizeof(Rid));

		RenderManager::Instance()->ChangeDisplayMode(currentMode, isFullscreen);
		RenderManager::Instance()->Init(currentMode.width, currentMode.height);
		UIControlSystem::Instance()->SetInputScreenAreaSize(currentMode.width, currentMode.height);
		Core::Instance()->SetPhysicalScreenSize(currentMode.width, currentMode.height);

		return true;
	}

	void CoreWin32Platform::Run()
	{
		Core::Instance()->SystemAppStarted();

		MSG msg;
		while(1)
		{
			// process messages
			willQuit = false;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);

				if(msg.message == WM_QUIT)
				{
					ApplicationCore * appCore = Core::Instance()->GetApplicationCore();
					if(appCore && appCore->OnQuit())
					{
						exit(0);
					}
					else
					{
						willQuit = true;
					}
				}
			}
			Sleep(1);
			RenderManager::Instance()->Lock();
			Core::SystemProcessFrame();
			RenderManager::Instance()->Unlock();

			if (willQuit)
			{	
				break;
			}
		}

		Core::Instance()->SystemAppFinished();
		FrameworkWillTerminate();
	}

/*	void CoreWin32Platform::InitOpenGL()
	{
		hDC = GetDC(hWindow);

		PIXELFORMATDESCRIPTOR pfd;
		ZeroMemory( &pfd, sizeof( pfd ) );
		pfd.nSize = sizeof( pfd );
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cDepthBits = 16;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int iFormat = ChoosePixelFormat(hDC, &pfd);
		SetPixelFormat(hDC, iFormat, &pfd);

		hRC = wglCreateContext(hDC);
		Thread::secondaryContext = wglCreateContext(hDC);
		Thread::currentDC = hDC;
		
		wglShareLists(Thread::secondaryContext, hRC);
		wglMakeCurrent(hDC, hRC);

		Thread * t = Thread::Create(Message());
		t->EnableCopyContext();
		t->Start();

		glewInit();
		const GLubyte * extensions = glGetString(GL_EXTENSIONS);
		Logger::FrameworkDebug("[CoreWin32Platform] gl extensions: %s", (const char*)extensions);
	}*/

	void CoreWin32Platform::ReleaseOpenGL()
	{
		wglMakeCurrent(0, 0);
		wglDeleteContext(hRC);
		ReleaseDC(hWindow, hDC);		
	}
	
	RECT CoreWin32Platform::GetWindowedRectForDisplayMode(DisplayMode & dm)
	{
		RECT clientSize;
		clientSize.top = 0;
		clientSize.left = 0;
		clientSize.right = dm.width;
		clientSize.bottom = dm.height;

		AdjustWindowRect(&clientSize, GetWindowLong(hWindow, GWL_STYLE), FALSE);

		return clientSize;
	}

	void CoreWin32Platform::ToggleFullscreen()
	{
		// Setup styles based on windowed / fullscreen mode
		isFullscreen = !isFullscreen;

		if ( isFullscreen )
		{
			currentMode = fullscreenMode;
			GetWindowRect(hWindow, &windowPositionBeforeFullscreen);

			SetMenu( hWindow, NULL );
			SetWindowLong( hWindow, GWL_STYLE, FULLSCREEN_STYLE );
			SetWindowPos( hWindow, NULL, 0, 0, currentMode.width, currentMode.height, SWP_NOZORDER );
		} 
		else
		{
			SetWindowLong( hWindow, GWL_STYLE, WINDOWED_STYLE );

			currentMode = windowedMode;
			RECT windowedRect = GetWindowedRectForDisplayMode(currentMode);
	
			SetWindowPos( hWindow, HWND_NOTOPMOST, windowPositionBeforeFullscreen.left, windowPositionBeforeFullscreen.top, windowedRect.right - windowedRect.left, windowedRect.bottom - windowedRect.top, SWP_NOACTIVATE | SWP_SHOWWINDOW );
		}
		
		Logger::FrameworkDebug("[RenderManagerDX9] toggle mode: %d x %d isFullscreen: %d", currentMode.width, currentMode.height, isFullscreen);

		RenderManager::Instance()->ChangeDisplayMode(currentMode, isFullscreen);
		RenderManager::Instance()->Init(currentMode.width, currentMode.height);
		UIControlSystem::Instance()->SetInputScreenAreaSize(currentMode.width, currentMode.height);
		Core::Instance()->SetPhysicalScreenSize(currentMode.width, currentMode.height);
	}

	Core::eScreenMode CoreWin32Platform::GetScreenMode()
	{
		if (isFullscreen)return Core::MODE_FULLSCREEN;
		else return Core::MODE_WINDOWED;
	}

	void CoreWin32Platform::SwitchScreenToMode(eScreenMode screenMode)
	{
		if (GetScreenMode() != screenMode) // check if we try to switch mode
		{
			if (screenMode == Core::MODE_FULLSCREEN)
			{
				ToggleFullscreen();
			}else if (screenMode == Core::MODE_WINDOWED)
			{
				ToggleFullscreen();
			}
		}else
		{
		}
	}

	void CoreWin32Platform::GetAvailableDisplayModes(List<DisplayMode> & availableDisplayModes)
	{
		availableDisplayModes.clear();

		DWORD iModeNum = 0;
		DEVMODE	dmi;
		ZeroMemory (&dmi, sizeof(dmi)) ;
		dmi.dmSize = sizeof(dmi) ;

		while(EnumDisplaySettings(NULL, iModeNum++, &dmi))
		{
			DisplayMode mode;
			mode.width = dmi.dmPelsWidth;
			mode.height = dmi.dmPelsHeight;
			mode.bpp = dmi.dmBitsPerPel;
			mode.refreshRate = dmi.dmDisplayFrequency;
			ZeroMemory (&dmi, sizeof(dmi)) ;
			availableDisplayModes.push_back(mode);

			Logger::FrameworkDebug(L"[RenderManagerDX9::GetAvailableDisplayModes] mode found: %d x %d x %d",
				mode.width,
				mode.height,
				mode.bpp);
		}
	}

	DisplayMode CoreWin32Platform::GetCurrentDisplayMode()
	{
		DWORD iModeNum = 0;
		DEVMODE	dmi;
		ZeroMemory (&dmi, sizeof(dmi)) ;
		dmi.dmSize = sizeof(dmi);

		DisplayMode mode;
		if(EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dmi))
		{
			mode.width = dmi.dmPelsWidth;
			mode.height = dmi.dmPelsHeight;
			mode.bpp = dmi.dmBitsPerPel;
			mode.refreshRate = dmi.dmDisplayFrequency;
			ZeroMemory (&dmi, sizeof(dmi)) ;
		}

		return mode;
	}


	void CoreWin32Platform::Quit()
	{
		PostQuitMessage(0);
	}

	void CoreWin32Platform::SetIcon(int32 iconId)
	{
		HINSTANCE hInst= GetModuleHandle(0);
		HICON smallIcon = static_cast<HICON>(LoadImage(hInst,
			MAKEINTRESOURCE(iconId),
			IMAGE_ICON,
			0,
			0,
			LR_DEFAULTSIZE));
		SendMessage(hWindow, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
		SendMessage(hWindow, WM_SETICON, ICON_BIG, (LPARAM)smallIcon);
	}

    bool isRightButtonPressed = false;
    bool isLeftButtonPressed = false;
    bool isMiddleButtonPressed = false;
	static Vector<DAVA::UIEvent> activeTouches;
	int32 MoveTouchsToVector(USHORT buttsFlags, WPARAM wParam, LPARAM lParam, Vector<UIEvent> *outTouches)
	{
		int button = 0;
        int phase = -1;

        if(LOWORD(wParam))
            phase = UIEvent::PHASE_MOVE;

        if(isLeftButtonPressed)
            button = 1;
        else if(isRightButtonPressed)
            button = 2;
        else if(isMiddleButtonPressed)
            button = 3;

		if(buttsFlags & RI_MOUSE_LEFT_BUTTON_DOWN || buttsFlags & RI_MOUSE_RIGHT_BUTTON_DOWN || buttsFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
		{
            phase = UIEvent::PHASE_BEGAN;
            if(buttsFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
            {
                isLeftButtonPressed = true;
                button = 1;
            }
            if(buttsFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
            {
                isRightButtonPressed = true;
                button = 2;
            }
            if(buttsFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
            {
                isMiddleButtonPressed = true;
                button = 3;
            }
		}
		else if(buttsFlags & RI_MOUSE_LEFT_BUTTON_UP || buttsFlags & RI_MOUSE_RIGHT_BUTTON_UP || buttsFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
		{
            phase = UIEvent::PHASE_ENDED;
            if(buttsFlags & RI_MOUSE_LEFT_BUTTON_UP)
            {
                isLeftButtonPressed = false;
                button = 1;
            }
            if(buttsFlags & RI_MOUSE_RIGHT_BUTTON_UP)
            {
                isRightButtonPressed = false;
                button = 2;
            }
            if(buttsFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
            {
                isMiddleButtonPressed = false;
                button = 3;
            }
		}
		else if(button && phase == UIEvent::PHASE_MOVE)
		{
			phase = UIEvent::PHASE_DRAG;
		}

        if(phase == -1)
            return phase;

		bool isFind = false;
		for(Vector<DAVA::UIEvent>::iterator it = activeTouches.begin(); it != activeTouches.end(); it++)
		{
			if(it->tid == button)
			{
				isFind = true;

				it->physPoint.x = (float32)GET_X_LPARAM(lParam);
				it->physPoint.y = (float32)GET_Y_LPARAM(lParam);
				it->phase = phase;

				break;
			}
		}

		if(!isFind)
		{
			UIEvent newTouch;
			newTouch.tid = button;
			newTouch.physPoint.x = (float32)GET_X_LPARAM(lParam);
			newTouch.physPoint.y = (float32)GET_Y_LPARAM(lParam);
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

	static bool mouseCursorShown = true;
	static USHORT mouseButtonsDownMask = 0;

	void HandleMouseButtonsPressed(USHORT buttsFlags)
	{
		if (buttsFlags & RI_MOUSE_BUTTON_1_DOWN)
		{
			mouseButtonsDownMask |= RI_MOUSE_BUTTON_1_DOWN;
		}
		if (buttsFlags & RI_MOUSE_BUTTON_2_DOWN)
		{
			mouseButtonsDownMask |= RI_MOUSE_BUTTON_2_DOWN;
		}
		if (buttsFlags & RI_MOUSE_BUTTON_3_DOWN)
		{
			mouseButtonsDownMask |= RI_MOUSE_BUTTON_3_DOWN;
		}
		if (buttsFlags & RI_MOUSE_BUTTON_4_DOWN)
		{
			mouseButtonsDownMask |= RI_MOUSE_BUTTON_4_DOWN;
		}
		if (buttsFlags & RI_MOUSE_BUTTON_5_DOWN)
		{
			mouseButtonsDownMask |= RI_MOUSE_BUTTON_5_DOWN;
		}
	}

	void HandleMouseButtonsReleased(USHORT buttsFlags)
	{
		if (mouseButtonsDownMask == 0)
		{
			return;
		}

		// Reset the mouse buttons mask, release capture if mask is empty (all buttons released).
		if (buttsFlags & RI_MOUSE_BUTTON_1_UP)
		{
			mouseButtonsDownMask &= ~RI_MOUSE_BUTTON_1_DOWN;
		}
		if (buttsFlags & RI_MOUSE_BUTTON_2_UP)
		{
			mouseButtonsDownMask &= ~RI_MOUSE_BUTTON_2_DOWN;
		}
		if (buttsFlags & RI_MOUSE_BUTTON_3_UP)
		{
			mouseButtonsDownMask &= ~RI_MOUSE_BUTTON_3_DOWN;
		}
		if (buttsFlags & RI_MOUSE_BUTTON_4_UP)
		{
			mouseButtonsDownMask &= ~RI_MOUSE_BUTTON_4_DOWN;
		}
		if (buttsFlags & RI_MOUSE_BUTTON_5_UP)
		{
			mouseButtonsDownMask &= ~RI_MOUSE_BUTTON_5_DOWN;
		}
	}

	void OnMouseEvent(USHORT buttsFlags, WPARAM wParam, LPARAM lParam, USHORT buttonData)
    {
        Vector<DAVA::UIEvent> touches;
        Vector<DAVA::UIEvent> emptyTouches;
        int32 touchPhase = -1;

        if (HIWORD(wParam) || mouseButtonsDownMask > 0) // isPoint inside window or some clicks already captured
        {
            HandleMouseButtonsPressed(buttsFlags);
        }

        if(buttsFlags & RI_MOUSE_WHEEL)
        {
            UIEvent newTouch;
            newTouch.tid = 0;
            newTouch.physPoint.x = 0;
            newTouch.physPoint.y = ((SHORT)buttonData) / (float32)(WHEEL_DELTA);
            newTouch.phase = touchPhase = UIEvent::PHASE_WHEEL;
            touches.push_back(newTouch);
        }
        else
		{
            if(HIWORD(wParam) || mouseButtonsDownMask > 0) // HIWORD(wParam) - isPoint inside window
			{
			    touchPhase = MoveTouchsToVector(buttsFlags, wParam, lParam, &touches);
			}
		}

        if(touchPhase != -1)
            UIControlSystem::Instance()->OnInput(touchPhase, emptyTouches, touches);

		if (RenderManager::Instance()->GetCursor() != 0 && mouseCursorShown)
		{
			ShowCursor(false);
			mouseCursorShown = false;
		}
		if (RenderManager::Instance()->GetCursor() == 0 && !mouseCursorShown)			
		{
			ShowCursor(false);
			mouseCursorShown = false;
		}

		HandleMouseButtonsReleased(buttsFlags);
	}

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WHEEL_DELTA                     
#define WHEEL_DELTA 120
#endif

		switch (message) 
		{
		case WM_ERASEBKGND:
				return 0;

		case WM_KEYUP:
			{
				InputSystem::Instance()->GetKeyboard()->OnSystemKeyUnpressed((int32)wParam);
			};
			break;

		case WM_KEYDOWN:
			{
				BYTE allKeys[256];
				GetKeyboardState(allKeys);
	
				if ((allKeys[VK_MENU] & 0x80)
					&& (allKeys[VK_TAB] & 0x80))
				{
					ShowWindow(hWnd, SW_MINIMIZE);
				}

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
				ev.tid = InputSystem::Instance()->GetKeyboard()->GetDavaKeyForSystemKey((int32)wParam);

				touches.push_back(ev);

				UIControlSystem::Instance()->OnInput(0, emptyTouches, touches);
				touches.pop_back();
				UIControlSystem::Instance()->OnInput(0, emptyTouches, touches);

				InputSystem::Instance()->GetKeyboard()->OnSystemKeyPressed((int32)wParam);
			};
			break;

		case WM_CHAR:
		{
			if(wParam > 27) //TODO: remove this elegant check
			{
				Vector<DAVA::UIEvent> touches;
				Vector<DAVA::UIEvent> emptyTouches;

				for(Vector<DAVA::UIEvent>::iterator it = activeTouches.begin(); it != activeTouches.end(); it++)
				{
					touches.push_back(*it);
				}

				DAVA::UIEvent ev;
				ev.keyChar = (char16)wParam;
				ev.phase = DAVA::UIEvent::PHASE_KEYCHAR;
				ev.tapCount = 1;
				ev.tid = 0;

				touches.push_back(ev);

				UIControlSystem::Instance()->OnInput(0, emptyTouches, touches);
				touches.pop_back();
				UIControlSystem::Instance()->OnInput(0, emptyTouches, touches);
			}
		}
		break;

        case WM_INPUT:
        {
            UINT dwSize;

            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, 
                sizeof(RAWINPUTHEADER));
            LPBYTE lpb = new BYTE[dwSize];
            if (lpb == NULL)
                return 0;

            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, 
                sizeof(RAWINPUTHEADER)) != dwSize )
                OutputDebugString (TEXT("GetRawInputData does not return correct size !\n")); 

            RAWINPUT* raw = (RAWINPUT*)lpb;

            if(raw->header.dwType == RIM_TYPEMOUSE && raw->data.mouse.usFlags == 0)
            {
                LONG x = raw->data.mouse.lLastX;
                LONG y = raw->data.mouse.lLastY;

                bool isMove = x || y;

                if(InputSystem::Instance()->IsCursorPining())
                {
                    RECT wndRect;
                    GetWindowRect(hWnd, &wndRect);
                    int centerX = (int)((wndRect.left + wndRect.right) >> 1);
                    int centerY = (int)((wndRect.bottom + wndRect.top) >> 1);
                    SetCursorPos(centerX, centerY);
                }
                else
                {
                    POINT p;
                    GetCursorPos(&p);
                    ScreenToClient(hWnd, &p);
                    x += p.x;
                    y += p.y;
                }

                RECT clientRect;
                GetClientRect(hWnd, &clientRect);

                bool isInside = (x > clientRect.left && x < clientRect.right && y > clientRect.top && y < clientRect.bottom) || InputSystem::Instance()->IsCursorPining();

                OnMouseEvent(raw->data.mouse.usButtonFlags, MAKEWPARAM(isMove, isInside), MAKELPARAM(x, y), raw->data.mouse.usButtonData); // only move, drag and wheel events
            }

            break;
        }
		case WM_MOUSEMOVE:
            //OnMouseEvent(message, wParam, lParam);
			break;

		case WM_NCMOUSEMOVE:
			if (!mouseCursorShown)
			{	
				ShowCursor(true);
				mouseCursorShown = true;
			}
			break;

		case WM_NCMOUSELEAVE:
			//ShowCursor(false);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_ACTIVATE:
			{
				ApplicationCore * core = Core::Instance()->GetApplicationCore();
                WORD loWord = LOWORD(wParam);
                WORD hiWord = HIWORD(wParam);
                if(!loWord || hiWord)
                {
                    Logger::FrameworkDebug("[PlatformWin32] deactivate application");
                    RenderResource::SaveAllResourcesToSystemMem();
					
                    if(core)
					{
						core->OnSuspend();
					}
					else 
					{
						Core::Instance()->SetIsActive(false);
					}
                }
                else
                {
                    Logger::FrameworkDebug("[PlatformWin32] activate application");
					if(core)
					{
						core->OnResume();
					}
					else 
					{
						Core::Instance()->SetIsActive(true);
					}
                }
			};
			break;
		case WM_SYSCOMMAND:
			// prevent screensaver or monitor powersave mode from starting
			if (wParam == SC_SCREENSAVE ||
				wParam == SC_MONITORPOWER)
				return 0;
			break;
		}

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}
#endif // #if defined(__DAVAENGINE_WIN32__)