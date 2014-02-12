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


#ifndef __DAVAENGINE_RENDER_EFFECT_H__
#define __DAVAENGINE_RENDER_EFFECT_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Render/RenderBase.h"

namespace DAVA
{
class Texture;
	
/**
    \brief Class that defines high-level rendering technique that renders data using current hardware
 
    Each render effect can be implemented for the set of the platforms and for the specific set of the hardware.
 
 
 
    Required features: 
    - ability to load render effects from disk in source, compiled form.
    - define different lod levels for different rendereffect tags [glsl, hlsl, high, medium, low]
 
    Example of .yaml rendereffect

    # rendereffect.yaml
    vertexshader:
        glsl: rendereffect_vertex.glsl
        hlsl: rendereffect_vertex.hlsl
 
    pixelshader:
        glsl: rendereffect_pixel.glsl
        hlsl: rendereffect_pixel.hlsl
  */
    
//class RenderBuffer
//{
//public:
//    
//    virtual void Set
//    
//    
//};
    
#define CONDITIONAL_CREATE(rendererType, classToCreate)\
if (renderer == rendererType)\
    return new classToCreate()
    
#define DECLARE_EFFECT(name)\
    const char * GetName() { return #name; };

class RenderEffect : public BaseObject 
{
protected:
    virtual ~RenderEffect();

public:
    RenderEffect();
    
    virtual const char * GetName();
    virtual void DrawArrays(ePrimitiveType mode, int32 first, int32 count);	
    virtual void DrawElements(ePrimitiveType type, int32 count, eIndexFormat indexFormat, void * indices); 
    
public:
    static Map<String, RenderEffect*> effectsMap;
};
};

#endif
