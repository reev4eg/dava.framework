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



#include "ShadowRect.h"
#include "Render/RenderDataObject.h"
#include "Base/BaseMath.h"
#include "Render/RenderBase.h"
#include "Render/RenderManager.h"
#include "Scene3D/Scene.h"
#include "FileSystem/FilePath.h"
#include "Render/ShaderCache.h"

namespace DAVA
{
	
	ShadowRect * ShadowRect::instance = 0;
	FastName ShadowRect::SHADOW_RECT_SHADER("~res:/Materials/Shaders/ShadowVolume/shadowrect");
	
	ShadowRect * ShadowRect::Create()
	{
		if(instance)
		{
			instance->Retain();
		}
		else
		{
			instance = new ShadowRect();
		}
		
		return instance;
	}
	
	ShadowRect::ShadowRect()
	{
		rdo = new RenderDataObject();
		
		Vector3 vert3[4] = {Vector3(-100.f, 100.f, -50), Vector3(100.f, 100.f, -50), Vector3(-100.f, -100.f, -50), Vector3(100.f, -100.f, -50)};
		
		for(int32 i = 0; i < 4; ++i)
		{
			vertices[i*3] = vert3[i].x;
			vertices[i*3+1] = vert3[i].y;
			vertices[i*3+2] = vert3[i].z;
		}
		
		rdo->SetStream(EVF_VERTEX, TYPE_FLOAT, 3, 0, vertices);
		
		
		shadowColor = Color(0, 0, 0, 0.5f);
		
		shader = SafeRetain(ShaderCache::Instance()->Get(SHADOW_RECT_SHADER, FastNameSet()));
		
		uniformShadowColor = shader->FindUniformIndexByName(FastName("shadowColor"));
		DVASSERT(uniformShadowColor != -1);
	}
	
	ShadowRect::~ShadowRect()
	{
		uniformShadowColor = -1;
		SafeRelease(shader);
		SafeRelease(rdo);
		
		instance = 0;
	}
	
	void ShadowRect::Draw()
	{
		RenderManager::Instance()->SetShader(shader);
		RenderManager::Instance()->SetRenderData(rdo);
		RenderManager::Instance()->FlushState();
		
		shader->SetUniformColor4ByIndex(uniformShadowColor, shadowColor);
		
		RenderManager::Instance()->AttachRenderData();
		RenderManager::Instance()->HWDrawArrays(PRIMITIVETYPE_TRIANGLESTRIP, 0, 4);
	}
    
	void ShadowRect::SetColor(const Color &color)
	{
		shadowColor = color;
	}
	
	const Color & ShadowRect::GetColor() const
	{
		return shadowColor;
	}
	
	
};
