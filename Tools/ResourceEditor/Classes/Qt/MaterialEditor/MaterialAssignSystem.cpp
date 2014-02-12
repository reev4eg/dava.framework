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


#include "MaterialAssignSystem.h"
#include "Scene/EntityGroup.h"
#include "Scene/SceneEditor2.h"


#include "Commands2/MaterialSwitchParentCommand.h"
#include "Tools/MimeData/MimeDataHelper2.h"

#include "Base/BaseTypes.h"
#include "Scene3D/Systems/MaterialSystem.h"

//Qt
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QCursor>

Q_DECLARE_METATYPE( DAVA::NMaterial * );

void MaterialAssignSystem::AssignMaterial(SceneEditor2 *scene, DAVA::NMaterial *instance, DAVA::NMaterial *newMaterialParent)
{
    scene->Exec(new MaterialSwitchParentCommand(instance, newMaterialParent));
}

void MaterialAssignSystem::AssignMaterialToGroup(SceneEditor2 *scene, const EntityGroup *group, DAVA::NMaterial *newMaterialParent)
{
    MaterialSystem *matSystem = scene->GetMaterialSystem();

    DAVA::Set<DAVA::NMaterial *> allMaterials;

    const size_t count = group->Size();
    for(size_t i = 0; i < count; ++i)
    {
        matSystem->BuildMaterialList(group->GetEntity(i), allMaterials);
    }

    AssignMaterial(scene, allMaterials, newMaterialParent);
}


void MaterialAssignSystem::AssignMaterialToEntity(SceneEditor2 *scene, DAVA::Entity *entity, DAVA::NMaterial *newMaterialParent)
{
    MaterialSystem *matSystem = scene->GetMaterialSystem();

    DAVA::Set<DAVA::NMaterial *> allMaterials;
    matSystem->BuildMaterialList(entity, allMaterials);

    AssignMaterial(scene, allMaterials, newMaterialParent);
}

void MaterialAssignSystem::AssignMaterial(SceneEditor2 *scene, const DAVA::Set<DAVA::NMaterial *> & allMaterials, DAVA::NMaterial *newMaterialParent)
{
    DAVA::Set<DAVA::NMaterial *> materials;
    DAVA::Set<DAVA::NMaterial *> instances;

    auto endIt = allMaterials.end();
    for(auto it = allMaterials.begin(); it != endIt; ++it)
    {
        if((*it)->GetMaterialType() == DAVA::NMaterial::MATERIALTYPE_INSTANCE)
        {
            instances.insert(*it);
        }
        else
        {
            materials.insert(*it);
        }
    }

    DAVA::NMaterial *selectedMaterial = SelectMaterial(materials);
    if(selectedMaterial)
    {
        scene->BeginBatch("Switch Material Parent");
        
        auto endIt = instances.end();
        for(auto it = instances.begin(); it != endIt; ++it)
        {
            if((*it)->GetParent() == selectedMaterial)
            {
                scene->Exec(new MaterialSwitchParentCommand(*it, newMaterialParent));
            }
        }
        
        scene->EndBatch();
    }
}


DAVA::NMaterial * MaterialAssignSystem::SelectMaterial(const DAVA::Set<DAVA::NMaterial *> & materials)
{
    if(materials.size() > 1)
    {
        QMenu selectMaterialMenu;
        
        auto endIt = materials.end();
        for(auto it = materials.begin(); it != endIt; ++it)
        {
            QVariant materialAsVariant = QVariant::fromValue<DAVA::NMaterial *>(*it);
            
            QString text = QString((*it)->GetName().c_str());
            QAction *action = selectMaterialMenu.addAction(text);
            action->setData(materialAsVariant);
        }
        
        QAction * selectedMaterialAction = selectMaterialMenu.exec(QCursor::pos());
        if(selectedMaterialAction)
        {
            QVariant materialAsVariant = selectedMaterialAction->data();
            return materialAsVariant.value<DAVA::NMaterial *>();
        }
    }
    else if(materials.size() == 1)
    {
        return *materials.begin();
    }
    
    return NULL;
}
