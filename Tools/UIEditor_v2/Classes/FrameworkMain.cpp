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



#include "DAVAEngine.h"
#include "GameCore.h"
#include "version.h"

using namespace DAVA;

void FrameworkDidLaunched()
{
    KeyedArchive * appOptions = new KeyedArchive();

    appOptions->SetInt32("fullscreen", 0);
    appOptions->SetInt32("bpp", 32);
    appOptions->SetBool("trackFont", true);

    appOptions->SetString("title", DAVA::Format("DAVA Framework - UIEditor | %s-%s", DAVAENGINE_VERSION, UI_EDITOR_VERSION));
#ifdef __DAVAENGINE_MACOS__
    //NSScreen *mainScreen = [NSScreen mainScreen];
#endif
    Size2i screenSize(100, 100);// = GetNativeScreenSize();
    appOptions->SetInt32("width",  screenSize.dx);
	appOptions->SetInt32("height", screenSize.dy);
    Core::Instance()->SetVirtualScreenSize(screenSize.dx, screenSize.dy);
    Core::Instance()->RegisterAvailableResourceSize(screenSize.dx, screenSize.dy, "Gfx");
    float32 screenScale = 1.0f;//GetNativeScreenScale();
    if (screenScale != 1.0f)
    {
        Core::Instance()->RegisterAvailableResourceSize(screenSize.dx*screenScale, screenSize.dy*screenScale, "Gfx2");
    }
    Core::Instance()->SetOptions(appOptions);
    Core::Instance()->EnableReloadResourceOnResize(false);

    GameCore * core = new GameCore();
    Core::SetApplicationCore(core);
}

void FrameworkWillTerminate()
{
    ApplicationCore* core = Core::GetApplicationCore();
    SafeRelease(core);
}