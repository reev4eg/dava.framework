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



#ifndef __MATERIALS_DROP_SYSTEM_H__
#define __MATERIALS_DROP_SYSTEM_H__

#include "Scene3D/Entity.h"
#include "Render/Material/NMaterial.h"
#include "Render/Highlevel/RenderBatch.h"

class EntityGroup;
class SceneEditor2;
class MaterialAssignSystem
{
public:

    static void AssignMaterial(SceneEditor2 *scene, DAVA::NMaterial *instance, DAVA::NMaterial *newMaterialParent);

    static void AssignMaterialToGroup(SceneEditor2 *scene, const EntityGroup *group, DAVA::NMaterial *newMaterialParent);
    static void AssignMaterialToEntity(SceneEditor2 *scene, DAVA::Entity *entity, DAVA::NMaterial *newMaterialParent);
    
protected:

    static void AssignMaterial(SceneEditor2 *scene, const DAVA::Set<DAVA::NMaterial *> & allMaterials, DAVA::NMaterial *newMaterialParent);
    
    static DAVA::NMaterial * SelectMaterial(const DAVA::Set<DAVA::NMaterial *> & materials);
};

#endif // __MATERIALS_DROP_SYSTEM_H__
