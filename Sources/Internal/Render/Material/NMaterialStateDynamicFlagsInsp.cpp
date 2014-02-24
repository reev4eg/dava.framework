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

#include "Render/Material/NMaterial.h"

namespace DAVA
{

Vector<FastName> NMaterialStateDynamicFlagsInsp::MembersList(void *object) const
{
	static Vector<FastName> ret;
	
	if(0 == ret.size())
	{
		ret.reserve(3);
		ret.push_back(NMaterial::FLAG_VERTEXFOG);
		ret.push_back(NMaterial::FLAG_FLATCOLOR);
		ret.push_back(NMaterial::FLAG_TEXTURESHIFT);
		ret.push_back(NMaterial::FLAG_TEXTURE0_ANIMATION_SHIFT);
	}
	return ret;
}

InspDesc NMaterialStateDynamicFlagsInsp::MemberDesc(void *object, const FastName &member) const
{
	return InspDesc(member.c_str());
}

VariantType NMaterialStateDynamicFlagsInsp::MemberValueGet(void *object, const FastName &member) const
{
	VariantType ret;
	NMaterial *state = (NMaterial*) object;
	DVASSERT(state);
	
	ret.SetBool(state->IsFlagEffective(member));
	
	return ret;
}

void NMaterialStateDynamicFlagsInsp::MemberValueSet(void *object, const FastName &member, const VariantType &value)
{
	NMaterial *state = (NMaterial*) object;
	DVASSERT(state);
	
	NMaterial::eFlagValue newValue = NMaterial::FlagOff;
	if(value.AsBool())
	{
		newValue = NMaterial::FlagOn;
	}
	
	state->SetFlag(member, newValue);
}

int NMaterialStateDynamicFlagsInsp::MemberFlags(void *object, const FastName &member) const
{
	return I_VIEW | I_EDIT;
}

};