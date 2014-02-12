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
#include "Render/Highlevel/RenderHierarchy.h"
#include "Render/Highlevel/RenderBatchArray.h"
#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/Frustum.h"

namespace DAVA
{

	void LinearRenderHierarchy::AddRenderObject(RenderObject * object)
	{		
		renderObjectArray.push_back(object);
	}
	
	void LinearRenderHierarchy::RemoveRenderObject(RenderObject *renderObject)
	{				
		uint32 size = renderObjectArray.size();
		for (uint32 k = 0; k < size; ++k)
		{
			if (renderObjectArray[k] == renderObject)
			{
				renderObjectArray[k] = renderObjectArray[size - 1];
				renderObjectArray.pop_back();
				return;
			}
		}
		DVASSERT(0 && "Failed to find object");
	}
	
	void LinearRenderHierarchy::ObjectUpdated(RenderObject * renderObject)
	{		
	}
    
	void LinearRenderHierarchy::Clip(Camera * camera, VisibilityArray * _visibilityArray)
	{				
		visibilityArray = _visibilityArray;
		Frustum * frustum = camera->GetFrustum();
		uint32 size = renderObjectArray.size();
		for (uint32 pos = 0; pos < size; ++pos)
		{
			RenderObject * node = renderObjectArray[pos];						
			if ((node->GetFlags() & RenderObject::CLIPPING_VISIBILITY_CRITERIA) != RenderObject::CLIPPING_VISIBILITY_CRITERIA)
				continue;					
			//still need to add flags for particles to dicede if to use DefferedUpdate
			if ((RenderObject::ALWAYS_CLIPPING_VISIBLE & node->GetFlags()) || frustum->IsInside(node->GetWorldBoundingBox()))
			{
				node->AddFlag(RenderObject::VISIBLE_AFTER_CLIPPING_THIS_FRAME);
				visibilityArray->Add(node);
			}
			else
			{
				node->RemoveFlag(RenderObject::VISIBLE_AFTER_CLIPPING_THIS_FRAME);
			}			
			
		}
		       
	}
    
    void LinearRenderHierarchy::GetAllObjectsInBBox(const AABBox3 & bbox, VisibilityArray * visibilityArray)
    {
        uint32 size = renderObjectArray.size();
		for (uint32 pos = 0; pos < size; ++pos)
		{
			RenderObject * ro = renderObjectArray[pos];
            if (bbox.IntersectsWithBox(ro->GetWorldBoundingBox()))
            {
                visibilityArray->Add(ro);
            }
        }
    }

};