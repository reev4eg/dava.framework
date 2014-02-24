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

#ifndef __DAVAENGINE_NMATERIAL_HELPER_H__
#define __DAVAENGINE_NMATERIAL_HELPER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/HashMap.h"
#include "Base/FastNameMap.h"
#include "Scene3D/DataNode.h"
#include "Render/RenderManager.h"
#include "Render/RenderState.h"
#include "Render/Material/NMaterialConsts.h"
#include "Render/Material/NMaterialTemplate.h"
#include "Render/Shader.h"
#include "Render/RenderState.h"
#include "Base/Introspection.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Render/Material/RenderTechnique.h"
#include "Render/Highlevel/RenderFastNames.h"

namespace DAVA
{

class NMaterial;
class NMaterialHelper
{
public:
		
    static void EnableStateFlags(const FastName& passName, NMaterial* target, uint32 stateFlags);
    static void DisableStateFlags(const FastName& passName, NMaterial* target, uint32 stateFlags);
    static void SetBlendMode(const FastName& passName, NMaterial* target, eBlendMode src, eBlendMode dst);
    static void SwitchTemplate(NMaterial* material, const FastName& templateName);
    static Texture* GetEffectiveTexture(const FastName& textureName, NMaterial* mat);
    static void SetFillMode(const FastName& passName, NMaterial* mat, eFillMode fillMode);
		
    static bool IsAlphatest(const FastName& passName, NMaterial* mat);
    static bool IsAlphablend(const FastName& passName, NMaterial* mat);
    static bool IsTwoSided(const FastName& passName, NMaterial* mat);
    static eFillMode GetFillMode(const FastName& passName, NMaterial* mat);
};

};

#endif