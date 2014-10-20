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

#include "UIMovieTest.h"
#include "FrameworkMain.h"

#include "IdeUnitTestsSupport.h"

static const float MOVIE_TEST_AUTO_CLOSE_TIME = 10.0f;

UIMovieTest::UIMovieTest() :
TestTemplate<UIMovieTest>("UIMovieTest")
{
	testFinished = false;
	onScreenTime = 0.f;
	movieView = NULL;

	playButton = NULL;
	stopButton = NULL;
	pauseButton = NULL;
	resumeButton = NULL;
	hideShowButton = NULL;
	
	playerStateText = NULL;

	RegisterFunction(this, &UIMovieTest::TestFunction, DAVA::Format("UIMovieTest"), NULL);
}

void UIMovieTest::LoadResources()
{
	movieView = new DAVA::UIMovieView(DAVA::Rect(10, 10, 940, 600));
	movieView->OpenMovie("~res://TestData/MovieTest/bunny.m4v", DAVA::OpenMovieParams());
	AddControl(movieView);
	
	// Create the "player" buttons.
//	const int buf_size = 260;
//	TCHAR buf[buf_size] = {0};
//	GetCurrentDirectory(buf_size, buf);
	DAVA::Font *font = DAVA::FTFont::Create("~res:/Fonts/korinna.ttf"); // "~res:/Fonts/korinna.ttf"
    DVASSERT(font);
	font->SetSize(14);

	playButton = new DAVA::UIButton(DAVA::Rect(10, 620, 60, 20));
	playButton->SetStateFont(0xFF, font);
	playButton->SetDebugDraw(true);
	playButton->SetStateText(0xFF, L"Play");
	playButton->AddEvent(DAVA::UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &UIMovieTest::ButtonPressed));
	AddControl(playButton);

	stopButton = new DAVA::UIButton(DAVA::Rect(80, 620, 60, 20));
	stopButton->SetStateFont(0xFF, font);
	stopButton->SetDebugDraw(true);
	stopButton->SetStateText(0xFF, L"Stop");
	stopButton->AddEvent(DAVA::UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &UIMovieTest::ButtonPressed));
	AddControl(stopButton);

	pauseButton = new DAVA::UIButton(DAVA::Rect(150, 620, 60, 20));
	pauseButton->SetStateFont(0xFF, font);
	pauseButton->SetDebugDraw(true);
	pauseButton->SetStateText(0xFF, L"Pause");
	pauseButton->AddEvent(DAVA::UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &UIMovieTest::ButtonPressed));
	AddControl(pauseButton);

	resumeButton = new DAVA::UIButton(DAVA::Rect(220, 620, 60, 20));
	resumeButton->SetStateFont(0xFF, font);
	resumeButton->SetDebugDraw(true);
	resumeButton->SetStateText(0xFF, L"Resume");
	resumeButton->AddEvent(DAVA::UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &UIMovieTest::ButtonPressed));
	AddControl(resumeButton);

	hideShowButton = new DAVA::UIButton(DAVA::Rect(290, 620, 60, 20));
	hideShowButton->SetStateFont(0xFF, font);
	hideShowButton->SetDebugDraw(true);
	hideShowButton->SetStateText(0xFF, L"Hide");
	hideShowButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &UIMovieTest::ButtonPressed));
	AddControl(hideShowButton);

	playerStateText = new DAVA::UIStaticText(DAVA::Rect(400, 620, 100, 20));
	playerStateText->SetFont(font);
	AddControl(playerStateText);
	
	finishTestBtn = new DAVA::UIButton(DAVA::Rect(850, 620, 100, 20));
	finishTestBtn->SetStateFont(0xFF, font);
	finishTestBtn->SetStateText(0xFF, L"Finish test");
	finishTestBtn->SetDebugDraw(true);
	finishTestBtn->AddEvent(DAVA::UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &UIMovieTest::ButtonPressed));
	AddControl(finishTestBtn);
	SafeRelease(font);
}

void UIMovieTest::UnloadResources()
{
	RemoveAllControls();

	SafeRelease(movieView);

	SafeRelease(playButton);
	SafeRelease(stopButton);
	SafeRelease(pauseButton);
	SafeRelease(resumeButton);
	SafeRelease(hideShowButton);
	
	SafeRelease(playerStateText);

	SafeRelease(finishTestBtn);
}

void UIMovieTest::DidAppear()
{
    onScreenTime = 0.f;
	movieView->Play();
}

void UIMovieTest::Update(DAVA::float32 timeElapsed)
{
    onScreenTime += timeElapsed;
    if(onScreenTime > MOVIE_TEST_AUTO_CLOSE_TIME)
    {
        testFinished = true;
		//IDE_REQUIRE(true);
    }
 
	UpdatePlayerStateText();
    TestTemplate<UIMovieTest>::Update(timeElapsed);
}

void UIMovieTest::UpdatePlayerStateText()
{
	if (!movieView || !playerStateText)
	{
		return;
	}
	
	bool isPlaying = movieView->IsPlaying();
	playerStateText->SetText(isPlaying ? L"Playing" : L"Paused");
}

void UIMovieTest::TestFunction(PerfFuncData * data)
{
	return;
}

bool UIMovieTest::RunTest(DAVA::int32 testNum)
{
	TestTemplate<UIMovieTest>::RunTest(testNum);
	return testFinished;
}

void UIMovieTest::ButtonPressed(DAVA::BaseObject *obj, void *data, void *callerData)
{
	if (obj == playButton)
	{
		movieView->Play();
	}
	else if (obj == stopButton)
	{
		movieView->Stop();
	}
	else if (obj == pauseButton)
	{
		movieView->Pause();
	}
	else if (obj == resumeButton)
	{
		movieView->Resume();
	}
	else if (obj == hideShowButton)
	{
		bool isMoviePlayerVisible = movieView->GetVisible();
		isMoviePlayerVisible = !isMoviePlayerVisible;

		hideShowButton->SetStateText(0xFF, isMoviePlayerVisible ? L"Hide" : L"Show");
		movieView->SetVisible(isMoviePlayerVisible);
	}
	else if (obj == finishTestBtn)
	{
		testFinished = true;
		// TEST code
//		IDE_REQUIRE(false);
	}
}

IDE_TEST_CASE_START(UIMovieTestCase, "[ui]")
{
	g_nameOfUiTestToStart = "UIMovieTest";
	// TODO test
//	HINSTANCE hInstance = GetModuleHandle(NULL);
//	SetCurrentDirectoryA("./../");
	
    //SetCurrentDirectoryA("c:\\Users\\l_chayka\\job\\dava.framework\\Projects\\UnitTests");
    {
        DAVA::Core::Run(0, 0, 0);
    }
    int i = 1;
    ++i;
}
IDE_TEST_CASE_END

IDE_TEST_CASE_START(SecondTestCase, "[tag]")
{
	int i = 1;
	IDE_REQUIRE(1 == i);
}
IDE_TEST_CASE_END