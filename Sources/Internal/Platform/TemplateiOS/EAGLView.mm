/*==================================================================================
    Copyright (c) 2008, DAVA, INC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA, INC nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA, INC AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DAVA, INC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/
#include "Base/BaseTypes.h"
#include "Core/Core.h"
#include "UI/UIControlSystem.h"
#include "UI/UIEvent.h"

#if defined(__DAVAENGINE_IPHONE__)


#import "Platform/TemplateiOS/EAGLView.h"

#import "Platform/TemplateiOS/ES1Renderer.h"
#import "Platform/TemplateiOS/ES2Renderer.h"
#import "Platform/TemplateiOS/ES3Renderer.h"

#include "DAVAEngine.h"

#include "Utils/Utils.h"

@implementation EAGLView

@synthesize animating;
@dynamic animationFrameInterval;

// You must implement this method
+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
//- (id) initWithCoder: (NSCoder *)aDecoder
- (id)initWithFrame:(CGRect)aRect
{    
    if ((self = [super initWithFrame:aRect]))
	{
        // Get the layer
		if (DAVA::Core::IsAutodetectContentScaleFactor()) 
		{
			if ([UIScreen instancesRespondToSelector: @selector(scale) ]
				&& [UIView instancesRespondToSelector: @selector(contentScaleFactor) ]) 
			{
				float scf = (int)[[UIScreen mainScreen] scale];
				[self setContentScaleFactor: scf];
			}
		}
//		if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
//		{
////			[self setFrame:CGRectMake(0, 768, 1024, 768)];
//			[self setFrame:CGRectMake(0, 0, 768, 1024)];
//
////			DAVA::UIControlSystem::Instance()->SetInputScreenAreaSize(768, 1024);
//			
//		}
//		else
//		{
//			// The device is an iPhone or iPod touch.
////			DAVA::UIControlSystem::Instance()->SetInputScreenAreaSize(320, 480);
//		}

		// Subscribe to "keyboard change frame" notifications to block GL while keyboard change is performed (see please DF-2012 for details).
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillChangeFrame:) name:UIKeyboardWillChangeFrameNotification object:nil];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardDidChangeFrame:) name:UIKeyboardDidChangeFrameNotification object:nil];

        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
            
        DAVA::KeyedArchive * options = DAVA::Core::Instance()->GetOptions();
        DAVA::Core::eRenderer rendererRequested = (DAVA::Core::eRenderer)options->GetInt32("renderer", DAVA::Core::RENDERER_OPENGL_ES_1_0);

        switch ((DAVA::Core::eScreenOrientation)options->GetInt32("orientation", DAVA::Core::SCREEN_ORIENTATION_PORTRAIT)) 
        {
            case DAVA::Core::SCREEN_ORIENTATION_PORTRAIT:
            {
                [[UIApplication sharedApplication] setStatusBarOrientation: UIInterfaceOrientationPortrait animated: false];
            }
                break;
            case DAVA::Core::SCREEN_ORIENTATION_LANDSCAPE_LEFT:
            {
                [[UIApplication sharedApplication] setStatusBarOrientation: UIInterfaceOrientationLandscapeLeft animated: false];
            }
                break;
            case DAVA::Core::SCREEN_ORIENTATION_PORTRAIT_UPSIDE_DOWN:
            {
                [[UIApplication sharedApplication] setStatusBarOrientation: UIInterfaceOrientationPortraitUpsideDown animated: false];
            }
                break;
            case DAVA::Core::SCREEN_ORIENTATION_LANDSCAPE_RIGHT:
            {
                [[UIApplication sharedApplication] setStatusBarOrientation: UIInterfaceOrientationLandscapeRight animated: false];
            }
                break;
                
            default:
                break;
        }
        
        DAVA::Core::eRenderer rendererCreated = DAVA::Core::RENDERER_OPENGL_ES_1_0;
        
        if (rendererRequested == DAVA::Core::RENDERER_OPENGL_ES_3_0)
        {
            renderer = [[ES3Renderer alloc] init];
            if(renderer != nil)
            {
                rendererCreated = DAVA::Core::RENDERER_OPENGL_ES_3_0;
                DAVA::RenderManager::Create(DAVA::Core::RENDERER_OPENGL_ES_3_0);
                DAVA::RenderManager::Instance()->InitFBO([renderer getColorRenderbuffer], [renderer getDefaultFramebuffer]);
            }
            else
            {
                rendererRequested =DAVA::Core::RENDERER_OPENGL_ES_2_0;
            }
        }
        
        if (rendererRequested == DAVA::Core::RENDERER_OPENGL_ES_2_0)
        {
            ES2Renderer* es2Renderer =  [[ES2Renderer alloc] init];
            renderer = es2Renderer;
            BOOL isGL30Created = [es2Renderer getIsGL30];
            rendererCreated = (NO == isGL30Created) ? DAVA::Core::RENDERER_OPENGL_ES_2_0 : DAVA::Core::RENDERER_OPENGL_ES_3_0;
            DAVA::RenderManager::Create(rendererCreated);
            DAVA::RenderManager::Instance()->InitFBO([renderer getColorRenderbuffer], [renderer getDefaultFramebuffer]);
        }
        
		if (!renderer)
		{
            renderer = [[ES1Renderer alloc] init];
			rendererCreated = DAVA::Core::RENDERER_OPENGL_ES_1_0;
			DAVA::RenderManager::Create(DAVA::Core::RENDERER_OPENGL_ES_1_0);
            DAVA::RenderManager::Instance()->InitFBO([renderer getColorRenderbuffer], [renderer getDefaultFramebuffer]);

			if (!renderer)
			{
				[self release];
				return nil;
			}
		}
        
		DAVA::RenderManager::Instance()->SetRenderContextId(DAVA::EglGetCurrentContext());
        DAVA::RenderManager::Instance()->Init(DAVA::Core::Instance()->GetPhysicalScreenWidth(), DAVA::Core::Instance()->GetPhysicalScreenHeight());
        DAVA::RenderManager::Instance()->DetectRenderingCapabilities();
        // Disable multitouch for the whole project
		self.multipleTouchEnabled = NO;
		animating = FALSE;
		displayLinkSupported = FALSE;
		animationFrameInterval = 1;
		currFPS = 60;
		displayLink = nil;
		animationTimer = nil;
		blockDrawView = false;
		
        // A system version of 3.1 or greater is required to use CADisplayLink. The NSTimer
        // class is used as fallback when it isn't available.
        NSString *reqSysVer = @"3.1";
        NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
        if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
            displayLinkSupported = TRUE;
        
        DAVA::Logger::Debug("OpenGL ES View Created successfully. displayLink: %d", (int)displayLinkSupported);
    }
	
    return self;
}



- (void) drawView:(id)sender
{
	if (blockDrawView)
	{
		// Yuri Coder, 2013/02/06. In case we are displaying ASSERT dialog we need to block rendering because RenderManager might be already locked here.
		return;
	}

	DAVA::RenderManager::Instance()->Lock();
    
    DAVA::uint64 renderManagerContextId = DAVA::RenderManager::Instance()->GetRenderContextId();
    DAVA::uint64 currentContextId = DAVA::EglGetCurrentContext();
    if (renderManagerContextId!=currentContextId)
    {
        EAGLContext * context =  (EAGLContext *)renderManagerContextId;
        [EAGLContext setCurrentContext:context];
    }
    
    if(DAVA::Core::Instance()->IsActive())
    {
        [renderer startRendering];
	}
        
	DAVA::Core::Instance()->SystemProcessFrame();
	
    if(DAVA::Core::Instance()->IsActive())
    {
        [renderer endRendering];
    }
    
	DAVA::RenderManager::Instance()->Unlock();
	
	if(currFPS != DAVA::RenderManager::Instance()->GetFPS())
	{
		currFPS = DAVA::RenderManager::Instance()->GetFPS();
		float interval = 60.0f / currFPS;
		if(interval < 1.0f)
		{
			interval = 1.0f;
		}
		[self setAnimationFrameInterval:(int)interval];
	}
}


- (void) layoutSubviews
{
	[renderer resizeFromLayer:(CAEAGLLayer*)self.layer];
    
    // Yuri Coder, 2013/11/28. The line below is commented out because of DF-2799.
    // [self drawView:nil];
}

- (NSInteger) animationFrameInterval
{
	return animationFrameInterval;
}

- (void) setAnimationFrameInterval:(NSInteger)frameInterval
{
	// Frame interval defines how many display frames must pass between each time the
	// display link fires. The display link will only fire 30 times a second when the
	// frame internal is two on a display that refreshes 60 times a second. The default
	// frame interval setting of one will fire 60 times a second when the display refreshes
	// at 60 times a second. A frame interval setting of less than one results in undefined
	// behavior.
	if (frameInterval >= 1)
	{
		animationFrameInterval = frameInterval;
		
		if (animating)
		{
			[self stopAnimation];
			[self startAnimation];
		}
	}
}

- (void) startAnimation
{
	if (!animating)
	{
		if (displayLinkSupported)
		{
			// CADisplayLink is API new to iPhone SDK 3.1. Compiling against earlier versions will result in a warning, but can be dismissed
			// if the system version runtime check for CADisplayLink exists in -initWithCoder:. The runtime check ensures this code will
			// not be called in system versions earlier than 3.1.

			displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(drawView:)];
			[displayLink setFrameInterval:animationFrameInterval];
			[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		}
		else
			animationTimer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)((1.0 / 60.0) * animationFrameInterval) target:self selector:@selector(drawView:) userInfo:nil repeats:TRUE];
		
		animating = TRUE;
	}
}

- (void)stopAnimation
{
	if (animating)
	{
		if (displayLinkSupported)
		{
			[displayLink invalidate];
			displayLink = nil;
		}
		else
		{
			[animationTimer invalidate];
			animationTimer = nil;
		}
		
		animating = FALSE;
	}
}

void MoveTouchsToVector(void *inTouches, DAVA::Vector<DAVA::UIEvent> *outTouches)
{
	NSArray *ar = (NSArray *)inTouches;
	for(UITouch *curTouch in ar)
	{
		DAVA::UIEvent newTouch;
		newTouch.tid = (DAVA::int32)curTouch;
//		newTouch.buttonId = DAVA::UIEvent::BUTTON_1;
		CGPoint p = [curTouch locationInView: curTouch.view ];
		newTouch.physPoint.x = p.x;
		newTouch.physPoint.y = p.y;
//		if(DAVA::RenderManager::Instance()->GetScreenOrientation() == DAVA::RenderManager::Instance()->ORIENTATION_LANDSCAPE_LEFT)
//		{
//			newTouch.point.x = (480 - p.y);
//			newTouch.point.y = (p.x);
//		}
//		else if(DAVA::RenderManager::Instance()->GetScreenOrientation() == DAVA::RenderManager::Instance()->ORIENTATION_LANDSCAPE_RIGHT)
//		{
//			newTouch.point.x = (p.y);
//			newTouch.point.y = (320 - p.x);
//		}
//		else
//		{
//			newTouch.point.x = p.x;
//			newTouch.point.y = p.y;
//		}
		newTouch.timestamp = curTouch.timestamp;
		newTouch.tapCount = curTouch.tapCount;
		
		switch(curTouch.phase)
		{
			case UITouchPhaseBegan:
				newTouch.phase = DAVA::UIEvent::PHASE_BEGAN;
				break;
			case UITouchPhaseEnded:
				newTouch.phase = DAVA::UIEvent::PHASE_ENDED;
				break;
			case UITouchPhaseMoved:
			case UITouchPhaseStationary:
				newTouch.phase = DAVA::UIEvent::PHASE_DRAG;
				break;
			case UITouchPhaseCancelled:
				newTouch.phase = DAVA::UIEvent::PHASE_CANCELLED;
				break;
				
		}
		outTouches->push_back(newTouch);
	}
}


-(void)process:(int) touchType touch:(NSArray*)active withEvent: (NSArray*)total
{
	MoveTouchsToVector(active, &activeTouches);
	MoveTouchsToVector(total, &totalTouches);
	DAVA::UIControlSystem::Instance()->OnInput(touchType, activeTouches, totalTouches);
	activeTouches.clear();
	totalTouches.clear();
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	//	LogDebug("TOUCH BEGAN");
	[self process:DAVA::UIEvent::PHASE_BEGAN touch:[touches allObjects] withEvent:[[event allTouches] allObjects]];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	//	LogDebug("TOUCH MOVED");
	[self process:DAVA::UIEvent::PHASE_DRAG touch:[touches allObjects] withEvent:[[event allTouches] allObjects]];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	//	LogDebug("TOUCH ENDED");
	[self process:DAVA::UIEvent::PHASE_ENDED touch:[touches allObjects] withEvent:[[event allTouches] allObjects]];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self process:DAVA::UIEvent::PHASE_CANCELLED touch:[touches allObjects] withEvent:[[event allTouches] allObjects]];
}

- (void) dealloc
{
    [renderer release];
	
    [super dealloc];
}

- (void) setCurrentContext
{
	[renderer setCurrentContext];
}

- (void) blockDrawing
{
	blockDrawView = true;
}

- (void)keyboardWillChangeFrame:(NSNotification *)notification
{
	blockDrawView = true;
}

- (void)keyboardDidChangeFrame:(NSNotification *)notification
{
	blockDrawView = false;
}

@end

#endif // #if defined(__DAVAENGINE_IPHONE__)

