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
    dummyMemorySize = 100 * 1024 * 1024;
    dummyMemory = new uint8[dummyMemorySize];
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
    
    Map<WideString, Message> testSet;
    testSet[L"WebView Loading Test"] = Message(this, &SelectScreen::OnWebViewLoading);
    testSet[L"WebView AddRemve Test"] = Message(this, &SelectScreen::OnWebViewAddRemove);
    testSet[L"Text Edit Two Test"] = Message(this, &SelectScreen::OnTextEditTwo);
    testSet[L"Text Edit AddRemove Test"] = Message(this, &SelectScreen::OnTextEditAddRemove);
    testSet[L"Text Edit SetText Test"] = Message(this, &SelectScreen::OnTextEditSetText);
    testSet[L"Text Edit ChangeFocus Test"] = Message(this, &SelectScreen::OnTextEditChangeFocus);
    
    for(auto it = testSet.begin(), endIt = testSet.end(); it != endIt; ++it)
    {
        ScopedPtr<UIButton> button(CreateButton(buttonRect, it->first));
        button->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, it->second);
        AddControl(button);
        
        buttonRect.y += (10.f + buttonRect.dy);
    }
    
    Memset(dummyMemory, GameCore::Instance()->GetTest(), dummyMemorySize);
}

void SelectScreen::OnWebViewLoading(BaseObject *caller, void *param, void *callerData)
{
    GameCore::Instance()->SetTest(GameCore::TEST_WEBVIEW_LOADING);
    UIScreenManager::Instance()->SetScreen(1);
}

void SelectScreen::OnWebViewAddRemove(BaseObject *caller, void *param, void *callerData)
{
    GameCore::Instance()->SetTest(GameCore::TEST_WEBVIEW_ADDREMOVE);
    UIScreenManager::Instance()->SetScreen(1);
}


void SelectScreen::OnTextEditTwo(BaseObject *caller, void *param, void *callerData)
{
    GameCore::Instance()->SetTest(GameCore::TEST_TEXTFIELD_TWOTEXTFIELDS);
    UIScreenManager::Instance()->SetScreen(2);
}

void SelectScreen::OnTextEditAddRemove(BaseObject *caller, void *param, void *callerData)
{
    GameCore::Instance()->SetTest(GameCore::TEST_TEXTFIELD_ADDREMOVE);
    UIScreenManager::Instance()->SetScreen(2);
}

void SelectScreen::OnTextEditSetText(BaseObject *caller, void *param, void *callerData)
{
    GameCore::Instance()->SetTest(GameCore::TEST_TEXTFIELD_SETTEXT);
    UIScreenManager::Instance()->SetScreen(2);
}

void SelectScreen::OnTextEditChangeFocus(BaseObject *caller, void *param, void *callerData)
{
    GameCore::Instance()->SetTest(GameCore::TEST_TEXTFIELD_CHANGEFOCUS);
    UIScreenManager::Instance()->SetScreen(2);
}


void SelectScreen::OnNoNative(BaseObject *caller, void *param, void *callerData)
{
    UIScreenManager::Instance()->SetScreen(3);
}


