/*==================================================================================
    Copyright (c) 2008, DAVA Consulting, LLC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA Consulting, LLC nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA CONSULTING, LLC AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL DAVA CONSULTING, LLC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    Revision History:
        * Created by Vitaliy Borodovsky 
=====================================================================================*/
#include "Render/RenderManager.h"
#include "Render/Texture.h"
#include "Render/2D/Sprite.h"
#include "Utils/Utils.h"
#include "Core/Core.h"
#include "Render/Shader.h"
#include "Render/RenderDataObject.h"
#include "Render/Effects/ColorOnlyEffect.h"
#include "Render/Effects/TextureMulColorEffect.h"


namespace DAVA
{
    
RenderEffect * RenderManager::FLAT_COLOR = 0;
RenderEffect * RenderManager::TEXTURE_MUL_FLAT_COLOR = 0;

    
RenderManager::RenderManager(Core::eRenderer _renderer)
{
	Logger::Debug("[RenderManager] created");
    renderer = _renderer;
    
	oldColor = Color::Clear();
    newColor = Color::Clear();

	oldSFactor = BLEND_NONE;
	oldDFactor = BLEND_NONE;
	newSFactor = BLEND_NONE;
	newDFactor = BLEND_NONE;

	currentTexture = NULL;
	newTextureEnabled = 0;
	oldTextureEnabled = 0;
	oldVertexArrayEnabled = 0;
	oldTextureCoordArrayEnabled = 0;
	oldColorArrayEnabled = 0;
	oldBlendingEnabled = 0;
	renderOrientation = 0;
	currentRenderTarget = NULL;
	
	currentRenderEffect = NULL;

	frameBufferWidth = 0;
	frameBufferHeight = 0;
	retScreenWidth = 0;
	retScreenHeight = 0;

	fps = 60;

	lockCount = 0;
	debugEnabled = false;
	fboViewRenderbuffer = 0;
	fboViewFramebuffer = 0;
	
	userDrawOffset = Vector2(0, 0);
	userDrawScale = Vector2(1, 1);

	viewMappingDrawOffset = Vector2(0, 0);
	viewMappingDrawScale = Vector2(1, 1);

	realDrawOffset = Vector2(0, 0);
	realDrawScale = Vector2(1, 1);

	currentDrawOffset = Vector2(0, 0);
	currentDrawScale = Vector2(1, 1);
	
	isInsideDraw = false;


#if defined(__DAVAENGINE_DIRECTX9__)
	depthStencilSurface = 0;
	backBufferSurface = 0;
#endif
	cursor = 0;
    currentRenderData = 0;
    pointerArraysCurrentState = 0;
    pointerArraysRendererState = 0;
    
    statsFrameCountToShowDebug = 0;
    frameToShowDebugStats = -1;
}
	
RenderManager::~RenderManager()
{
    SafeRelease(FLAT_COLOR);
    SafeRelease(TEXTURE_MUL_FLAT_COLOR);
	SafeRelease(cursor);
	Logger::Debug("[RenderManager] released");
}

void RenderManager::SetDebug(bool isDebugEnabled)
{
	debugEnabled = isDebugEnabled;
}
	
bool RenderManager::IsInsideDraw()
{
	return isInsideDraw;
}

void RenderManager::Init(int32 _frameBufferWidth, int32 _frameBufferHeight)
{
    FLAT_COLOR = ColorOnlyEffect::Create(renderer);
    TEXTURE_MUL_FLAT_COLOR = TextureMulColorEffect::Create(renderer);
    
    
	frameBufferWidth = _frameBufferWidth;
	frameBufferHeight = _frameBufferHeight;
    const char * extensions = (const char*)glGetString(GL_EXTENSIONS);
	Logger::Debug("[RenderManager::Init] orientation: %d x %d extensions: %s", frameBufferWidth, frameBufferHeight, extensions);
}

void RenderManager::Reset()
{
	oldColor.r = oldColor.g = oldColor.b = oldColor.a = -1.0f;
	ResetColor();
//#if defined(__DAVAENGINE_OPENGL__)
	oldSFactor = oldDFactor = BLEND_NONE;
	newSFactor = newDFactor = BLEND_NONE;
//#endif
	newTextureEnabled = oldTextureEnabled = -1;
	oldVertexArrayEnabled = -1;
	oldTextureCoordArrayEnabled = -1;
#if defined(__DAVAENGINE_OPENGL__)
	oldBlendingEnabled = -1;
#endif 	
	currentRenderTarget = NULL;
	currentRenderEffect = NULL;
	currentClip.x = 0;
	currentClip.y = 0;
	currentClip.dx = -1;
	currentClip.dy = -1;
	
	currentTexture = NULL;

	userDrawOffset = Vector2(0, 0);
	userDrawScale = Vector2(1, 1);
	
	realDrawOffset = Vector2(0, 0);
	realDrawScale = Vector2(1, 1);
	
	currentDrawOffset = Vector2(0, 0);
	currentDrawScale = Vector2(1, 1);
	
//	glLoadIdentity();
}

int32 RenderManager::GetRenderOrientation()
{
	return renderOrientation;
}
	
int32 RenderManager::GetScreenWidth()
{
	return retScreenWidth;	
}
int32 RenderManager::GetScreenHeight()
{
	return retScreenHeight;
}

void RenderManager::SetColor(float r, float g, float b, float a)
{
	newColor.r = r;
	newColor.g = g;
	newColor.b = b;
	newColor.a = a;
}
	
void RenderManager::SetColor(const Color & _color)
{
	newColor = _color;
}
	
float RenderManager::GetColorR()
{
	return newColor.r;
}
	
float RenderManager::GetColorG()
{
	return newColor.g;
}
	
float RenderManager::GetColorB()
{
	return newColor.b;
}
	
float RenderManager::GetColorA()
{
	return newColor.a;
}
    
const Color & RenderManager::GetColor() const
{
    return newColor;
}

void RenderManager::ResetColor()
{
	newColor.r = newColor.g = newColor.b = newColor.a = 1.0f;
}
	
	
void RenderManager::SetTexture(Texture *texture)
{
	if(texture != currentTexture)
	{
		currentTexture = texture;
		if(currentTexture)
		{
			if(debugEnabled)
			{
				Logger::Debug("Bind texture: id %d", currentTexture->id);
			}
            
#if defined(__DAVAENGINE_OPENGL__)
            RENDER_VERIFY(glBindTexture(GL_TEXTURE_2D, currentTexture->id));
#elif defined(__DAVAENGINE_DIRECTX9__)
            RENDER_VERIFY(GetD3DDevice()->SetTexture(0, currentTexture->id));
#endif 
		}
	}
}
	
Texture *RenderManager::GetTexture()
{
	return currentTexture;	
}

void RenderManager::SetRenderData(RenderDataObject * object)
{
    currentRenderData = object;
}
    
void RenderManager::AttachRenderData(Shader * shader)
{
    if (!shader)
    {
        // TODO: should be moved to RenderManagerGL
#if defined(__DAVAENGINE_MACOS__)
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, currentRenderData->vboBuffer);
#else
        glBindBuffer(GL_ARRAY_BUFFER, currentRenderData->vboBuffer);
#endif
        pointerArraysCurrentState = 0;
        int32 size = (int32)currentRenderData->streamArray.size();
        for (int32 k = 0; k < size; ++k)
        {
            RenderDataStream * stream = currentRenderData->streamArray[k];
            switch(stream->formatMark)
            {
                case EVF_VERTEX:
                    SetVertexPointer(stream->size, stream->type, stream->stride, stream->pointer);
                    pointerArraysCurrentState |= EVF_VERTEX;
                    break;
                case EVF_TEXCOORD0:
                    SetTexCoordPointer(stream->size, stream->type, stream->stride, stream->pointer);
                    pointerArraysCurrentState |= EVF_TEXCOORD0;
                    break;
                default:
                    break;
            };
        };
        
        uint32 difference = pointerArraysCurrentState ^ pointerArraysRendererState;

        if (difference & EVF_VERTEX)
        {
            EnableVertexArray(pointerArraysCurrentState & EVF_VERTEX);
        }
        if (difference & EVF_TEXCOORD0)
        {
            EnableTextureCoordArray(pointerArraysCurrentState & EVF_TEXCOORD0);
        }
        pointerArraysRendererState = pointerArraysCurrentState;
    }
//    for (uint32 formatFlag = EVF_LOWER_BIT; formatFlag <= EVF_HIGHER_BIT; formatFlag >>= 1)
//    {
//        if (formatFlag & EVF_VERTEX)
//        {
//            
//        }
//        if (formatFlag & EVF_TEXCOORD0)
//        {
//            
//        }
//    }
//    pointerArraysRendererState = pointerArraysCurrentState;
}

void RenderManager::EnableTexturing(bool isEnabled)
{
	newTextureEnabled = isEnabled;
}
		
void RenderManager::SetClip(const Rect &rect)
{
	SetHWClip(rect);
}
	
void RenderManager::RemoveClip()
{
	SetHWClip(Rect(0,0,-1,-1));
}

void RenderManager::ClipRect(const Rect &rect)
{
	Rect r = currentClip;
	if(r.dx < 0)
	{
		r.dx = retScreenWidth;
	}
	if(r.dy < 0)
	{
		r.dy = retScreenHeight;
	}
	
	r = r.Intersection(rect);
	SetHWClip(rect);
}

void RenderManager::ClipPush()
{
	clipStack.push(currentClip);
}

void RenderManager::ClipPop()
{
	if(clipStack.empty())
	{
		Rect r(0, 0, -1, -1);
		SetClip(r);
	}
	else
	{
		Rect r = clipStack.top();
		SetClip(r);
	}
	clipStack.pop();
}
	
void RenderManager::InitFBO(GLuint _viewRenderbuffer, GLuint _viewFramebuffer)
{
	fboViewRenderbuffer = _viewRenderbuffer;
	fboViewFramebuffer = _viewFramebuffer;
}

void RenderManager::SetRenderTarget(Sprite *renderTarget)
{
//	Logger::Info("Set Render target");
	RenderTarget rt;
	rt.spr = currentRenderTarget;
	rt.orientation = renderOrientation;
	renderTargetStack.push(rt);
		
	ClipPush();
	PushDrawMatrix();
	PushMappingMatrix();
	IdentityDrawMatrix();
	SetHWRenderTarget(renderTarget);
}
	
void RenderManager::RestoreRenderTarget()
{
//	Logger::Info("Restore Render target");
	RenderTarget rt = renderTargetStack.top();
	renderTargetStack.pop();
	SetHWRenderTarget(rt.spr);

	PopDrawMatrix();
	PopMappingMatrix();
	ClipPop();
}

bool RenderManager::IsRenderTarget()
{
	return currentRenderTarget != NULL;
}


void RenderManager::SetNewRenderEffect(RenderEffect *renderEffect)
{
	SafeRelease(currentRenderEffect);
	currentRenderEffect = SafeRetain(renderEffect);
}

void RenderManager::SetRenderEffect(RenderEffect *renderEffect)
{
	renderEffectStack.push(SafeRetain(currentRenderEffect));
	SetNewRenderEffect(renderEffect);
}

void RenderManager::RestoreRenderEffect()
{
	RenderEffect *renderEffect = renderEffectStack.top();
	renderEffectStack.pop();
	SetNewRenderEffect(renderEffect);
	SafeRelease(renderEffect);
}

void RenderManager::Lock()
{
	glMutex.Lock();
}
void RenderManager::Unlock()
{
	glMutex.Unlock();
}
	
void RenderManager::LockNonMain()
{
	if(!Thread::IsMainThread())
	{
		if(!lockCount)
		{
			Lock();
		}
		lockCount++;
	}
}
	
int32 RenderManager::GetNonMainLockCount()
{
	return lockCount;
}

void RenderManager::UnlockNonMain()
{
	if(!Thread::IsMainThread())
	{
		lockCount--;
		if(!lockCount)
		{
			Unlock();
		}
	}
}

void RenderManager::SetFPS(int32 newFps)
{
	fps = newFps;	
}
int32 RenderManager::GetFPS()
{
	return fps;
}
	
	
void RenderManager::SetDrawTranslate(const Vector2 &offset)
{
	userDrawOffset.x += offset.x * userDrawScale.x;
	userDrawOffset.y += offset.y * userDrawScale.y;
}
	
void RenderManager::SetDrawScale(const Vector2 &scale)
{
	userDrawScale.x *= scale.x;
	userDrawScale.y *= scale.y;
}
	
void RenderManager::IdentityDrawMatrix()
{
	userDrawScale.x = 1.0f;
	userDrawScale.y = 1.0f;

	userDrawOffset.x = 0.0f;
	userDrawOffset.y = 0.0f;
}

void RenderManager::IdentityTotalMatrix()
{
	userDrawOffset = Vector2(0, 0);
	userDrawScale = Vector2(1, 1);
	
	viewMappingDrawOffset = Vector2(0, 0);
	viewMappingDrawScale = Vector2(1, 1);
	
	realDrawOffset = Vector2(0, 0);
	realDrawScale = Vector2(1, 1);
	
	currentDrawOffset = Vector2(0, 0);
	currentDrawScale = Vector2(1, 1);
}
	
	
	
void RenderManager::SetPhysicalViewScale()
{
//	Logger::Info("Set physical view scale");
	viewMappingDrawScale.x = 1.0f;
	viewMappingDrawScale.y = 1.0f;
}

void RenderManager::SetPhysicalViewOffset()
{
	viewMappingDrawOffset = Core::Instance()->GetPhysicalDrawOffset();
}

void RenderManager::SetVirtualViewScale()
{
	viewMappingDrawScale.x = Core::GetVirtualToPhysicalFactor();
	viewMappingDrawScale.y = Core::GetVirtualToPhysicalFactor();
}

void RenderManager::SetVirtualViewOffset()
{
	viewMappingDrawOffset.x -= Core::Instance()->GetVirtualScreenXMin() * viewMappingDrawScale.x;
	viewMappingDrawOffset.y -= Core::Instance()->GetVirtualScreenYMin() * viewMappingDrawScale.y;
//	viewMappingDrawOffset = Core::Instance()->GetPhysicalDrawOffset();
}
	
void RenderManager::PushDrawMatrix()
{
	DrawMatrix dm;
	dm.userDrawOffset = userDrawOffset;
	dm.userDrawScale = userDrawScale;
	matrixStack.push(dm);
}

void RenderManager::PopDrawMatrix()
{
	IdentityDrawMatrix();
	DrawMatrix dm = matrixStack.top();
	matrixStack.pop();
	userDrawOffset = dm.userDrawOffset;
	userDrawScale = dm.userDrawScale;
}
	
void RenderManager::PushMappingMatrix()
{
	DrawMatrix dm;
	dm.userDrawOffset = viewMappingDrawOffset;
	dm.userDrawScale = viewMappingDrawScale;
	mappingMatrixStack.push(dm);
}

void RenderManager::PopMappingMatrix()
{
	IdentityDrawMatrix();
	DrawMatrix dm = mappingMatrixStack.top();
	mappingMatrixStack.pop();
	viewMappingDrawOffset = dm.userDrawOffset;
	viewMappingDrawScale = dm.userDrawScale;
}

void RenderManager::SetCursor(Cursor * _cursor)
{
#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WIN32__)
	SafeRelease(cursor);
	cursor = SafeRetain(_cursor);
	if (cursor)cursor->HardwareSet();
#endif
}

Cursor * RenderManager::GetCursor()
{
	return cursor;
}
	
const RenderManager::Caps & RenderManager::GetCaps()
{
	return caps;
}
    
const RenderManager::Stats & RenderManager::GetStats()
{
    return stats;
}
    
void RenderManager::RectFromRenderOrientationToViewport(Rect & rect)
{
    switch(renderOrientation)
    {
        case Core::SCREEN_ORIENTATION_PORTRAIT:
            break;
        case Core::SCREEN_ORIENTATION_LANDSCAPE_LEFT:
            {
                float32 newX = (float32)frameBufferWidth - (rect.y + rect.dy);
                float32 newY = (float32)frameBufferHeight - rect.x;
                float32 newDX = rect.dy;
                float32 newDY = rect.dx;
                rect.x = newX;
                rect.y = newY;
                rect.dx = newDX;
                rect.dy = newDY;
            }
            break;
        case Core::SCREEN_ORIENTATION_LANDSCAPE_RIGHT:
            {
                float32 newX = (float32)frameBufferWidth - (rect.y + rect.dy);
                float32 newY = (float32)frameBufferHeight - rect.x;
                float32 newDX = rect.dy;
                float32 newDY = rect.dx;
                rect.x = newX;
                rect.y = newY;
                rect.dx = newDX;
                rect.dy = newDY;
            }            
            break;
            
    };
}

const Matrix4 & RenderManager::GetMatrix(eMatrixType type)
{
    return matrices[type];
}

const Matrix4 & RenderManager::GetUniformMatrix(eUniformMatrixType type)
{
    if (uniformMatrixFlags[type] == 0)
    {
        if (type == UNIFORM_MATRIX_MODELVIEWPROJECTION)
        {
            uniformMatrices[type] =  matrices[MATRIX_MODELVIEW] * matrices[MATRIX_PROJECTION];
        }
        uniformMatrixFlags[type] = 1; // matrix is ready
    }
    return uniformMatrices[type];
}
    
void RenderManager::ClearUniformMatrices()
{
    for (int32 k = 0; k < UNIFORM_MATRIX_COUNT; ++k)
        uniformMatrixFlags[k] = 0;
}
    
void RenderManager::Stats::Clear()
{
    drawArraysCalls = 0;
    drawElementsCalls = 0;
    for (int32 k = 0; k < PRIMITIVETYPE_COUNT; ++k)
        primitiveCount[k] = 0;
}

void RenderManager::EnableOutputDebugStatsEveryNFrame(int32 _frameToShowDebugStats)
{
    frameToShowDebugStats = _frameToShowDebugStats;
}

void RenderManager::ProcessStats()
{
    if (frameToShowDebugStats == -1)return;
    
    statsFrameCountToShowDebug++;
    if (statsFrameCountToShowDebug >= frameToShowDebugStats)
    {
        statsFrameCountToShowDebug = 0;
        Logger::Debug("== Frame stats: DrawArraysCount: %d DrawElementCount: %d ==", stats.drawArraysCalls, stats.drawElementsCalls);
        for (int32 k = 0; k < PRIMITIVETYPE_COUNT; ++k)
            Logger::Debug("== Primitive Stats: %d ==", stats.primitiveCount[k]);
    }
}

	
};