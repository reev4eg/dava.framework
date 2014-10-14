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


#include "NoNativeScreen.h"
#include "GameCore.h"

NoNativeScreen::NoNativeScreen()
    : BaseScreen(L"No Native Controls Screen")
{
}

void NoNativeScreen::LoadResources()
{
    BaseScreen::LoadResources();
 
    ScopedPtr<UIButton> backButton(CreateButton(Rect(0, 0, 90, 30), L"Back"));
    backButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &NoNativeScreen::OnBack));
    AddControl(backButton);
    
    ScopedPtr<UIButton> sampleButton(CreateButton(Rect(10, 100, 200, 30), L"Sample Button"));
    AddControl(sampleButton);
    
    ScopedPtr<UIStaticText> sampleText(new UIStaticText(Rect(0, 150, GetRect().dx, HEADER_HEIGHT)));
    sampleText->SetText(L"Sample static text");
    sampleText->SetFont(font);
    sampleText->SetTextColor(Color::White);
    AddControl(sampleText);


}

void NoNativeScreen::OnBack(BaseObject *caller, void *param, void *callerData)
{
    UIScreenManager::Instance()->SetScreen(BaseScreen::FIRST_SCREEN);
}

