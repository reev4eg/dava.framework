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


#include "TextEditScreen.h"
#include "GameCore.h"

TextEditScreen::TextEditScreen()
    : BaseScreen(L"Text Edit Screen")
    , login(NULL)
    , password(NULL)
    , loginText(L"")
    , passwordText(L"")
    , enteredText(NULL)
    , testCounter(0)
    , testTime(0.f)
{
}

TextEditScreen::~TextEditScreen()
{
    SafeRelease(enteredText);
    SafeRelease(login);
    SafeRelease(password);
}

void TextEditScreen::LoadResources()
{
    BaseScreen::LoadResources();
 
    ScopedPtr<UIButton> backButton(CreateButton(Rect(0, 0, 90, 30), L"Back"));
    backButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &TextEditScreen::OnBack));
    AddControl(backButton);
    
    DVASSERT(login == NULL);
    DVASSERT(password == NULL);
    
    const Rect screenRect = GetRect();
    Rect textRect(10.f, HEADER_HEIGHT + 10.f, screenRect.dx - 20.f, 30.f);
    
    
    login = new UITextField(textRect);
    login->SetFont(font);
    login->SetDebugDraw(true);
    login->SetText(loginText);
    login->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
    login->SetTextColor(Color::White);
    login->SetDelegate(this);
    AddControl(login);
    
    textRect.y += (10.f + textRect.dy);

    password = new UITextField(textRect);
    password->SetFont(font);
    password->SetDebugDraw(true);
    password->SetText(passwordText);
    password->SetTextAlign(ALIGN_RIGHT | ALIGN_VCENTER);
    password->SetTextColor(Color::White);
    password->SetDelegate(this);
    AddControl(password);
    
    textRect.y += (10.f + textRect.dy);
    textRect.dy = screenRect.dy - textRect.dy - 10.f;
    
    
    enteredText = new UIStaticText(textRect);
    enteredText->SetFont(font);
    enteredText->SetMultiline(true);
    enteredText->SetTextColor(Color(0.2f, 0.2f, 0.f, 1.f));
    enteredText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    AddControl(enteredText);
    
    testCounter = 0;
    testTime = 0.f;
}

void TextEditScreen::UnloadResources()
{
    loginText = login->GetText();
    passwordText = password->GetText();
    
    SafeRelease(enteredText);
    SafeRelease(login);
    SafeRelease(password);
    BaseScreen::UnloadResources();
}

void TextEditScreen::DidAppear()
{
}

void TextEditScreen::OnBack(BaseObject *caller, void *param, void *callerData)
{
    UIScreenManager::Instance()->SetScreen(BaseScreen::FIRST_SCREEN);
}

void TextEditScreen::TextFieldShouldReturn(UITextField * textField)
{
    enteredText->SetText(textField->GetText() + L"\n" + enteredText->GetText());
    textField->SetText(L"");
}

void TextEditScreen::Update(float32 timeElapsed)
{
    BaseScreen::Update(timeElapsed);
    
    ++testCounter;
    testTime += timeElapsed;

    switch (GameCore::Instance()->GetTest())
    {
        case GameCore::TEST_TEXTFIELD_ADDREMOVE:
        {
            RemoveControl(login);
            SafeRelease(login);
            
            RemoveControl(password);
            SafeRelease(password);
            
            
            const Rect screenRect = GetRect();
            Rect textRect(10.f, HEADER_HEIGHT + 10.f, screenRect.dx - 20.f, 30.f);
            
            login = new UITextField(textRect);
            login->SetFont(font);
            login->SetDebugDraw(true);
            login->SetText(loginText);
            login->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
            login->SetTextColor(Color::White);
            login->SetDelegate(this);
            AddControl(login);
            
            textRect.y += (10.f + textRect.dy);
            
            password = new UITextField(textRect);
            password->SetFont(font);
            password->SetDebugDraw(true);
            password->SetText(passwordText);
            password->SetTextAlign(ALIGN_RIGHT | ALIGN_VCENTER);
            password->SetTextColor(Color::White);
            password->SetDelegate(this);
            AddControl(password);
        }
        break;

        case GameCore::TEST_TEXTFIELD_SETTEXT:
        {
            break;
        }
            
        case GameCore::TEST_TEXTFIELD_TWOTEXTFIELDS:
            break;

        case GameCore::TEST_TEXTFIELD_CHANGEFOCUS:
        {
            if(UIControlSystem::Instance()->GetFocusedControl() == login)
            {
                password->SetFocused();
            }
            else
            {
                login->SetFocused();
            }
            break;
        }
            
        case GameCore::TEST_TEXTFIELD_CHANGEVISIBILITY:
        {
            if((testCounter % 10) == 0)
            {
                login->SetVisible(!login->GetVisible());
                password->SetVisible(!password->GetVisible());
            }
            break;
        }

        case GameCore::TEST_TEXTFIELD_SHOWHIDE:
        {
            if((testCounter % 2) == 0)
            {
                login->DidAppear();
                password->DidAppear();
            }
            else
            {
                login->WillDisappear();
                password->WillDisappear();
            }
            break;
        }

            
        default:
            break;
    }

    if(GameCore::Instance()->GetTest() != GameCore::TEST_TEXTFIELD_TWOTEXTFIELDS)
    {
        login->SetText(Format(L"Test counter is %d", testCounter));
        password->SetText(Format(L"Test time is %f", testTime));
    }
}

