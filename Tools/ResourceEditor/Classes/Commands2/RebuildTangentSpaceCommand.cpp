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



#include "RebuildTangentSpaceCommand.h"
#include "Render/3D/MeshUtils.h"

RebuildTangentSpaceCommand::RebuildTangentSpaceCommand(DAVA::RenderBatch *_renderBatch, bool _computeBinormal)
	: Command2(CMDID_REBUILD_TANGENT_SPACE, "Rebuild Tangent Space")
    , renderBatch(_renderBatch)    
    , computeBinormal(_computeBinormal)
{
    DVASSERT(renderBatch);        
    DAVA::PolygonGroup *srcGroup = renderBatch->GetPolygonGroup();
    DVASSERT(srcGroup);
    originalGroup = new DAVA::PolygonGroup();
    DAVA::MeshUtils::CopyGroupData(srcGroup, originalGroup);
    if (computeBinormal)
    {
        DAVA::NMaterial *material = renderBatch->GetMaterial();
        if (material)
            materialBinormalFlagState = material->GetFlagValue(DAVA::NMaterial::FLAG_PRECOMPUTED_BINORMAL);
    }
}

RebuildTangentSpaceCommand::~RebuildTangentSpaceCommand()
{
    SafeRelease(originalGroup);
}

void RebuildTangentSpaceCommand::Redo()
{
    DAVA::MeshUtils::RebuildMeshTangentSpace(renderBatch->GetPolygonGroup(), computeBinormal);
    if (computeBinormal)
    {
        DAVA::NMaterial *material = renderBatch->GetMaterial();
        if (material)
            material->SetFlag(DAVA::NMaterial::FLAG_PRECOMPUTED_BINORMAL, DAVA::NMaterial::FlagOn);
    }
}

void RebuildTangentSpaceCommand::Undo()
{
    DAVA::MeshUtils::CopyGroupData(originalGroup, renderBatch->GetPolygonGroup());    
    if (computeBinormal)
    {
        DAVA::NMaterial *material = renderBatch->GetMaterial();
        if (material)
        {
            if (materialBinormalFlagState&DAVA::NMaterial::FlagInherited)
                material->ResetFlag(DAVA::NMaterial::FLAG_PRECOMPUTED_BINORMAL);
            else 
                material->SetFlag(DAVA::NMaterial::FLAG_PRECOMPUTED_BINORMAL, (DAVA::NMaterial::eFlagValue)(materialBinormalFlagState&DAVA::NMaterial::FlagOn));
        }
        
    }    
}