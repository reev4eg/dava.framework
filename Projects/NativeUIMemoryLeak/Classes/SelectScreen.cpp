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


#include "SelectScreen.h"
#include "GameCore.h"


SelectScreen::SelectScreen()
: BaseScreen(L"Select Text Screen")
{
    dummyMemory = new uint8[100 * 1024 * 1024];
}

SelectScreen::~SelectScreen()
{
    SafeDeleteArray(dummyMemory);
}

void SelectScreen::LoadResources()
{
    BaseScreen::LoadResources();
    
    const Rect screenRect = GetRect();
    Rect buttonRect(10.f, HEADER_HEIGHT + 10.f, screenRect.dx - 20.f, 30.f);
    
    ScopedPtr<UIButton> webViewButton(CreateButton(buttonRect, L"Web Vew Screen"));
    webViewButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &SelectScreen::OnWebView));
    AddControl(webViewButton);

    buttonRect.y += (10.f + buttonRect.dy);
    
    ScopedPtr<UIButton> textEditButton(CreateButton(buttonRect, L"Text Edit Screen"));
    textEditButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &SelectScreen::OnTextEdit));
    AddControl(textEditButton);
    
    buttonRect.y += (10.f + buttonRect.dy);
    
    ScopedPtr<UIButton> noNativeButton(CreateButton(buttonRect, L"No Native Controls Screen"));
    noNativeButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &SelectScreen::OnNoNative));
    AddControl(noNativeButton);
    
    Logger::Info("%d", sizeof(dummyMemory)/sizeof(uint8));
}

void SelectScreen::OnWebView(BaseObject *caller, void *param, void *callerData)
{
    UIScreenManager::Instance()->SetScreen(1);
}

void SelectScreen::OnTextEdit(BaseObject *caller, void *param, void *callerData)
{
    UIScreenManager::Instance()->SetScreen(2);
}

void SelectScreen::OnNoNative(BaseObject *caller, void *param, void *callerData)
{
    UIScreenManager::Instance()->SetScreen(3);
}


