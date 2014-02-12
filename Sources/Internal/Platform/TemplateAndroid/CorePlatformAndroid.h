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




#ifndef __DAVAENGINE_CORE_PLATFORM_ANDROID_H__
#define __DAVAENGINE_CORE_PLATFORM_ANDROID_H__

#include "DAVAEngine.h"
#if defined(__DAVAENGINE_ANDROID__)

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

namespace DAVA
{

class AndroidSystemDelegate
{
public:
	AndroidSystemDelegate(JavaVM *vm);

	virtual void ShowKeyboard() = 0;
	virtual void HideKeyboard() = 0;
	virtual bool DownloadHttpFile(const String & url, const String & documentsPathname) = 0;
    
    virtual GLint RenderBuffer() = 0;
	virtual GLint FrameBuffer() = 0;

	JNIEnv* GetEnvironment() const {return environment;};
	JavaVM* GetVM() const {return vm;};
protected:
	JNIEnv* environment;
	JavaVM* vm;
};



class Thread;
class CorePlatformAndroid: public Core
{
public:

	CorePlatformAndroid();

	virtual void CreateAndroidWindow(const char8 *docPath, const char8 *assets, const char8 *logTag, AndroidSystemDelegate * sysDelegate);

	virtual void Quit();

	void RenderRecreated(int32 w, int32 h);
	void RepaintView();

	// called from Activity and manage a visible lifetime
	void StartVisible();
	void StopVisible();

	void StartForeground();
	void StopForeground(bool isLock);

	void OnCreateActivity();
	void OnDestroyActivity();

	void KeyUp(int32 keyCode);
	void KeyDown(int32 keyCode);

	void OnInput(int32 action, int32 id, float32 x, float32 y, float64 time, int32 source, int32 tapCount);

	bool DownloadHttpFile(const String & url, const String & documentsPathname);

	AAssetManager * GetAssetManager();
	void SetAssetManager(AAssetManager * mngr);

	const String& GetExternalStoragePathname() const {return externalStorage;};
	
	AndroidSystemDelegate* GetAndroidSystemDelegate() const;

private:

	void QuitAction();
	void ProcessWithoutDrawing();

	void UpdateScreenMode();

    void ResizeView(int32 w, int32 h);

    

private:
	int32 width;
	int32 height;

	bool wasCreated;
	bool renderIsActive;

	bool foreground;

	UIEvent CreateInputEvent(int32 action, int32 id, float32 x, float32 y, float64 time, int32 source, int32 tapCount);

	Vector<DAVA::UIEvent> totalTouches;

	AndroidSystemDelegate *androidDelegate;
	AAssetManager * assetMngr;

	String externalStorage;
};
};
#endif // #if defined(__DAVAENGINE_ANDROID__)
#endif // __DAVAENGINE_CORE_PLATFORM_ANDROID_H__
