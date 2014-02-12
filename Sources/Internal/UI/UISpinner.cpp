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



#include "UISpinner.h"

namespace DAVA 
{

//use these names for children buttons to define UISpinner in .yaml
static const String UISPINNER_BUTTON_NEXT_NAME = "buttonNext";
static const String UISPINNER_BUTTON_PREVIOUS_NAME = "buttonPrevious";
static const String UISPINNER_CONTENT_NAME = "content";
static const float32 ANIMATION_TIME = 0.1f;
static const int32 MOVE_ANIMATION_TRACK = 10;
static const float32 X_UNDEFINED = 10000;
static const float32 SLIDE_GESTURE_SPEED = 20.f;
static const float32 SLIDE_GESTURE_TIME = 0.1f;
    
void SpinnerAdapter::AddObserver(SelectionObserver* anObserver)
{
    observers.insert(anObserver);
}

void SpinnerAdapter::RemoveObserver(SelectionObserver* anObserver)
{
    observers.erase(anObserver);
}

void SpinnerAdapter::NotifyObservers(bool isSelectedFirst, bool isSelectedLast, bool isSelectedChanged)
{
    Set<SelectionObserver*>::const_iterator end = observers.end();
    for (Set<SelectionObserver*>::iterator it = observers.begin(); it != end; ++it)
    {
        (*it)->OnSelectedChanged(isSelectedFirst, isSelectedLast, isSelectedChanged);
    }
}

bool SpinnerAdapter::Next()
{
    bool completedOk = SelectNext();
    if (completedOk)
        NotifyObservers(false /*as we selected next it can't be first*/, IsSelectedLast(), true);
    return completedOk;
}

bool SpinnerAdapter::Previous()
{
    bool completedOk = SelectPrevious();
    if (completedOk)
        NotifyObservers(IsSelectedFirst(), false /*as we selected previous it can't be last*/, true);
    return completedOk;
}

void SpinnerAdapter::DisplaySelectedData(UISpinner * spinner)
{
    FillScrollableContent(spinner->GetContent(), CURRENT);
}
    
UISpinner::UISpinner(const Rect &rect, bool rectInAbsoluteCoordinates/* = FALSE*/) 
    : UIControl(rect, rectInAbsoluteCoordinates)
    , buttonNext(new UIButton())
    , buttonPrevious(new UIButton())
    , content(new UIControl())
    , nextContent(new UIControl())
    , contentViewport(new UIControl())
    , adapter(NULL)
    , totalGestureTime(0)
    , totalGestureDx(0)
    , currentTouchX(X_UNDEFINED)
    , previousTouchX(X_UNDEFINED)
    , dragAnchorX(X_UNDEFINED)
{
    buttonNext->SetName(UISPINNER_BUTTON_NEXT_NAME);
    buttonPrevious->SetName(UISPINNER_BUTTON_PREVIOUS_NAME);
    AddControl(buttonNext);
    AddControl(buttonPrevious);
    content->SetName(UISPINNER_CONTENT_NAME);
    AddControl(content);
    contentViewport->AddControl(nextContent);
    contentViewport->SetInputEnabled(false);
    contentViewport->SetClipContents(true);
    InitButtons();
}

UISpinner::~UISpinner()
{
    ReleaseButtons();
    if (adapter)
        adapter->RemoveObserver(this);
    SafeRelease(adapter);
    SafeRelease(contentViewport);
    SafeRelease(nextContent);
}
    
void UISpinner::Update(DAVA::float32 timeElapsed)
{
    if (currentTouchX < X_UNDEFINED)
    {
        Move move;
        move.dx = currentTouchX - previousTouchX;
        move.time = timeElapsed;
        moves.push_back(move);
        totalGestureDx += move.dx;
        totalGestureTime += move.time;
        if (totalGestureTime > SLIDE_GESTURE_TIME)
        {
            List<Move>::iterator it = moves.begin();
            totalGestureTime -= it->time;
            totalGestureDx -= it->dx;
            moves.erase(it);
        }
        previousTouchX = currentTouchX;
    }
}
    
void UISpinner::ContentChanged()
{
    content->SetInputEnabled(false);        
    contentViewport->SetRect(content->GetRect());
    contentViewport->pivotPoint = content->pivotPoint;
    nextContent->CopyDataFrom(content);
    nextContent->relativePosition = Vector2();
    nextContent->pivotPoint.x = content->size.dx;
}
    
void UISpinner::Input(UIEvent *currentInput)
{
    if (NULL == adapter)
    {
        return;
    }

    if (content->IsAnimating(MOVE_ANIMATION_TRACK))
    {
        return;
    }
    
    Vector2 touchPos = currentInput->point;
    if (currentInput->phase == UIEvent::PHASE_BEGAN)
    {
        if (content->IsPointInside(touchPos))
        {
            DVASSERT(NULL == contentViewport->GetParent());
            
            content->relativePosition = Vector2();
            content->pivotPoint = Vector2();
            
            contentViewport->AddControl(content);
            AddControl(contentViewport);
            dragAnchorX = touchPos.x - content->relativePosition.x;
            currentTouchX = touchPos.x;
            previousTouchX = currentTouchX;
        }
        else
        {
            dragAnchorX = X_UNDEFINED;
        }    
    }
    else if (currentInput->phase == UIEvent::PHASE_DRAG)
    {
        if (dragAnchorX < X_UNDEFINED)
        {
            currentTouchX = touchPos.x;
            float32 contentNewX = touchPos.x - dragAnchorX;
            float32 contentNewLeftEdge = contentNewX - content->pivotPoint.x;
            if (!(contentNewLeftEdge < 0 && adapter->IsSelectedLast()) && !(contentNewLeftEdge > 0 && adapter->IsSelectedFirst()))
            {
                if (contentNewX != 0)
                {
                    if (content->relativePosition.x * contentNewX <= 0) //next content just appears or visible side changes
                    {
                        //adjust nextContent->pivotPoint to make more convenient setting of nextContent->relativePosition below
                        nextContent->pivotPoint.x = contentNewX > 0 ? content->size.dx : - content->size.dx;
                        adapter->FillScrollableContent(nextContent, contentNewX > 0 ? SpinnerAdapter::PREVIOUS : SpinnerAdapter::NEXT);
                    }
                }
                content->relativePosition.x = contentNewX;
                nextContent->relativePosition.x = contentNewX; //for this to work we adjust pivotPoint above
                
                if (abs(content->relativePosition.x) > content->size.dx / 2)
                {
                    OnSelectWithSlide(content->relativePosition.x > 0);
                    dragAnchorX = touchPos.x - content->relativePosition.x;
                }
            }
        }
    }
    else if (currentInput->phase == UIEvent::PHASE_ENDED || currentInput->phase == UIEvent::PHASE_CANCELLED)
    {
        if (dragAnchorX < X_UNDEFINED)
        {
            if (totalGestureTime > 0)
            {
                float32 averageSpeed = totalGestureDx / totalGestureTime;
                bool selectPrevious = averageSpeed > 0;
                if (selectPrevious == content->relativePosition.x > 0) //switch only if selected item is already shifted in slide direction
                {
                    bool isSelectedLast = selectPrevious ? adapter->IsSelectedFirst() : adapter->IsSelectedLast();
                    if (abs(averageSpeed) > SLIDE_GESTURE_SPEED && !isSelectedLast)
                    {
                        OnSelectWithSlide(selectPrevious);
                    }
                }
            }
            
            Animation* animation = content->PositionAnimation(Vector2(0, content->relativePosition.y), ANIMATION_TIME, Interpolation::EASY_IN, MOVE_ANIMATION_TRACK);
            animation->AddEvent(Animation::EVENT_ANIMATION_END, Message(this, &UISpinner::OnScrollAnimationEnd));
            nextContent->PositionAnimation(Vector2(0, content->relativePosition.y), ANIMATION_TIME, Interpolation::EASY_IN, MOVE_ANIMATION_TRACK);

            currentTouchX = X_UNDEFINED;
            previousTouchX = X_UNDEFINED;
            dragAnchorX = X_UNDEFINED;
            moves.clear();
            totalGestureTime = 0;
            totalGestureDx = 0;
        }
    }

    currentInput->SetInputHandledType(UIEvent::INPUT_HANDLED_HARD); // Drag is handled - see please DF-2508.
}
    
void UISpinner::OnSelectWithSlide(bool isPrevious)
{
    UIControl * temp = content;
    content = nextContent;
    nextContent = temp;
    
    //save display position but change pivot points
    nextContent->pivotPoint.x -= content->pivotPoint.x;
    nextContent->relativePosition.x -= content->pivotPoint.x;
    content->relativePosition.x -= content->pivotPoint.x;
    content->pivotPoint.x = 0;
    
    if (isPrevious)
        adapter->Previous();
    else
        adapter->Next();
}
    
void UISpinner::OnScrollAnimationEnd(BaseObject * caller, void * param, void *callerData)
{
    DVASSERT(NULL != contentViewport->GetParent());
    content->pivotPoint = contentViewport->pivotPoint;
    content->relativePosition = contentViewport->relativePosition;
    RemoveControl(contentViewport);
    AddControl(content);
}
    
void UISpinner::InitButtons()
{
    ContentChanged();
    buttonNext->SetDisabled(!adapter || adapter->IsSelectedLast());
    buttonPrevious->SetDisabled(!adapter || adapter->IsSelectedFirst());
    buttonNext->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &UISpinner::OnNextPressed));
    buttonPrevious->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &UISpinner::OnPreviousPressed));
}

void UISpinner::ReleaseButtons()
{
    RemoveControl(buttonNext);
    RemoveControl(buttonPrevious);
    buttonNext->RemoveEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &UISpinner::OnNextPressed));
    buttonPrevious->RemoveEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &UISpinner::OnPreviousPressed));
    SafeRelease(buttonNext);
    SafeRelease(buttonPrevious);
    RemoveControl(content);
    SafeRelease(content);
}

void UISpinner::LoadFromYamlNode(const YamlNode * node, UIYamlLoader * loader)
{
    //release default buttons - they have to be loaded from yaml
    ReleaseButtons();
    UIControl::LoadFromYamlNode(node, loader);
}

void UISpinner::CopyDataFrom(UIControl *srcControl)
{
    ReleaseButtons();
    UIControl::CopyDataFrom(srcControl);

	// Yuri Coder, 2013/03/28. CopyDataFrom works with real children, so need to copy inner buttons explicitely.
	UISpinner* t = static_cast<UISpinner*>(srcControl);

	this->buttonPrevious = static_cast<UIButton*>(t->buttonPrevious->Clone());
	this->buttonNext = static_cast<UIButton*>(t->buttonNext->Clone());
	this->content = t->content->Clone();

	AddControl(buttonPrevious);
	AddControl(buttonNext);
	AddControl(content);

    InitButtons();
}

UIControl* UISpinner::Clone()
{
	UISpinner *t = new UISpinner(GetRect());
	t->CopyDataFrom(this);
	return t;
}

void UISpinner::AddControl(UIControl *control)
{
	// Synchronize the pointers to the buttons each time new control is added.
	UIControl::AddControl(control);

	if (control->GetName() == UISPINNER_BUTTON_NEXT_NAME)
	{
		buttonNext = (UIButton*)control;
	}
	else if (control->GetName() == UISPINNER_BUTTON_PREVIOUS_NAME)
	{
		buttonPrevious = (UIButton*)control;
	}
	else if (control->GetName() == UISPINNER_CONTENT_NAME)
	{
		content = control;		
	}
}

void UISpinner::FindRequiredControls()
{
    UIControl * nextButtonControl = FindByName(UISPINNER_BUTTON_NEXT_NAME);
    UIControl * previousButtonControl = FindByName(UISPINNER_BUTTON_PREVIOUS_NAME);
    DVASSERT(nextButtonControl);
    DVASSERT(previousButtonControl);
    buttonNext = SafeRetain(DynamicTypeCheck<UIButton*>(nextButtonControl));
    buttonPrevious = SafeRetain(DynamicTypeCheck<UIButton*>(previousButtonControl));
    DVASSERT(buttonNext);
    DVASSERT(buttonPrevious);
    content = SafeRetain(FindByName(UISPINNER_CONTENT_NAME));
    DVASSERT(content);
}

void UISpinner::LoadFromYamlNodeCompleted()
{
    FindRequiredControls();
    InitButtons();
}

YamlNode * UISpinner::SaveToYamlNode(UIYamlLoader * loader)
{
    buttonPrevious->SetName(UISPINNER_BUTTON_PREVIOUS_NAME);
	buttonNext->SetName(UISPINNER_BUTTON_NEXT_NAME);
	content->SetName(UISPINNER_CONTENT_NAME);

	YamlNode *node = UIControl::SaveToYamlNode(loader);
	SetPreferredNodeType(node, "UISpinner");

	return node;
}
	
List<UIControl* > UISpinner::GetSubcontrols()
{
	List<UIControl* > subControls;

	// Lookup for the contols by their names.
	AddControlToList(subControls, UISPINNER_BUTTON_PREVIOUS_NAME);
	AddControlToList(subControls, UISPINNER_BUTTON_NEXT_NAME);
	AddControlToList(subControls, UISPINNER_CONTENT_NAME);

	return subControls;
}

void UISpinner::SetAdapter(SpinnerAdapter * anAdapter)
{
    if (adapter)
    {
        adapter->RemoveObserver(this);
        SafeRelease(adapter);
    }

    adapter = SafeRetain(anAdapter);
    if (adapter)
    {
        buttonNext->SetDisabled(adapter->IsSelectedLast());
        buttonPrevious->SetDisabled(adapter->IsSelectedFirst());
        adapter->DisplaySelectedData(this);
        adapter->AddObserver(this);
    }
    else
    {
        buttonNext->SetDisabled(true);
        buttonPrevious->SetDisabled(true);
    }
}

void UISpinner::OnNextPressed(DAVA::BaseObject * caller, void * param, void *callerData)
{
    if (content->IsAnimating(MOVE_ANIMATION_TRACK))
    {
        return;
    }
    //buttonNext is disabled if we have no adapter or selected adapter element is last, so we don't need checks here
    adapter->Next();
}    

void UISpinner::OnPreviousPressed(DAVA::BaseObject * caller, void * param, void *callerData)
{
    if (content->IsAnimating(MOVE_ANIMATION_TRACK))
    {
        return;
    }
    //buttonPrevious is disabled if we have no adapter or selected adapter element is first, so we don't need checks here
    adapter->Previous();
}

void UISpinner::OnSelectedChanged(bool isSelectedFirst, bool isSelectedLast, bool isSelectedChanged)
{
    buttonNext->SetDisabled(isSelectedLast);
    buttonPrevious->SetDisabled(isSelectedFirst);
    if (isSelectedChanged)
    {
        adapter->DisplaySelectedData(this);
        PerformEvent(UIControl::EVENT_VALUE_CHANGED);
    }
}

}