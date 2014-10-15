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


#include "WebViewScreen.h"
#include "GameCore.h"

WebViewScreen::WebViewScreen()
    : BaseScreen(L"Web View Screen")
    , webView(NULL)
    , textCount(0)
{
    urls.push_back("http://www.apple.com");
    urls.push_back("http://www.eurosport.ru");
    urls.push_back("http://bash.im");
//    urls.push_back("about:blank");
}

WebViewScreen::~WebViewScreen()
{
    SafeRelease(webView);
}

void WebViewScreen::LoadResources()
{
    BaseScreen::LoadResources();
 
    ScopedPtr<UIButton> backButton(CreateButton(Rect(0, 0, 90, 30), L"Back"));
    backButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &WebViewScreen::OnBack));
    AddControl(backButton);
    
    const Rect & screenRect = GetRect();
    DVASSERT(webView == NULL);
    webView = new UIWebView(Rect(10.f, HEADER_HEIGHT, screenRect.dx - 20.f, screenRect.dy - HEADER_HEIGHT - 10.f));
    AddControl(webView);
}

void WebViewScreen::UnloadResources()
{
    SafeRelease(webView);
    BaseScreen::UnloadResources();
}


void WebViewScreen::DidAppear()
{
    webView->OpenURL(urls[textCount % urls.size()]);
    ++textCount;
}

void WebViewScreen::OnBack(BaseObject *caller, void *param, void *callerData)
{
    UIScreenManager::Instance()->SetScreen(BaseScreen::FIRST_SCREEN);
}

void WebViewScreen::Update(float32 timeElapsed)
{
    BaseScreen::Update(timeElapsed);
}


