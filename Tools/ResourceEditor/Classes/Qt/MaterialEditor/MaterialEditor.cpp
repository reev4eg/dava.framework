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

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QAction>
#include <QVariant>

#include "MaterialEditor.h"
#include "ui_materialeditor.h"

#include "MaterialModel.h"
#include "MaterialFilterModel.h"

#include "Main/mainwindow.h"
#include "Main/QtUtils.h"
#include "QualitySwitcher/QualitySwitcher.h"
#include "Project/ProjectManager.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataIntrospection.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataInspMember.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataInspDynamic.h"
#include "Tools/QtPropertyEditor/QtPropertyDataValidator/TexturePathValidator.h"
#include "Qt/Settings/SettingsManager.h"

#include "Scene3D/Systems/QualitySettingsSystem.h"

#include "CommandLine/TextureDescriptor/TextureDescriptorUtils.h"
#include "CommandLine/TextureDescriptor/TextureDescriptorUtils.h"

MaterialEditor::MaterialEditor(QWidget *parent /* = 0 */)
: QDialog(parent)
, ui(new Ui::MaterialEditor)
, curMaterial(NULL)
{
	ui->setupUi(this);
	setWindowFlags(WINDOWFLAG_ON_TOP_OF_APPLICATION);

	ui->materialTree->setDragEnabled(true);
	ui->materialTree->setAcceptDrops(true);
	ui->materialTree->setDragDropMode(QAbstractItemView::DragDrop);

	ui->materialProperty->SetEditTracking(true);
    //ui->materialProperty->setSortingEnabled(true);
    //ui->materialProperty->header()->setSortIndicator(0, Qt::AscendingOrder);

	// global scene manager signals
	QObject::connect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2 *)), this, SLOT(sceneActivated(SceneEditor2 *)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(Deactivated(SceneEditor2 *)), this, SLOT(sceneDeactivated(SceneEditor2 *)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(CommandExecuted(SceneEditor2 *, const Command2*, bool)), this, SLOT(commandExecuted(SceneEditor2 *, const Command2 *, bool)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(SelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)), this, SLOT( autoExpand() ));
	
	// material tree
	QObject::connect(ui->materialTree->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(materialSelected(const QItemSelection &, const QItemSelection &)));
    QObject::connect(ui->materialTree, SIGNAL(Updated()), this, SLOT(autoExpand()));

	// material properties
	QObject::connect(ui->materialProperty, SIGNAL(PropertyEdited(const QModelIndex &)), this, SLOT(OnPropertyEdited(const QModelIndex &)));
	QObject::connect(ui->templateBox, SIGNAL(activated(int)), this, SLOT(OnTemplateChanged(int)));
    QObject::connect(ui->actionMaterialReload, SIGNAL(triggered(bool)), this, SLOT(OnMaterialReload(bool)));
    QObject::connect(ui->actionSwitchQuality, SIGNAL(triggered(bool)), this, SLOT(OnSwitchQuality(bool)));

	posSaver.Attach(this);
	posSaver.LoadState(ui->splitter);

	treeStateHelper = new PropertyEditorStateHelper(ui->materialProperty, (QtPropertyModel *) ui->materialProperty->model());

	DAVA::VariantType v1 = posSaver.LoadValue("splitPosProperties");
	DAVA::VariantType v2 = posSaver.LoadValue("splitPosPreview");
	if(v1.GetType() == DAVA::VariantType::TYPE_INT32) ui->materialProperty->header()->resizeSection(0, v1.AsInt32());
	if(v2.GetType() == DAVA::VariantType::TYPE_INT32) ui->materialProperty->header()->resizeSection(1, v2.AsInt32());

    expandMap[MaterialFilteringModel::SHOW_ALL] = false;
    expandMap[MaterialFilteringModel::SHOW_ONLY_INSTANCES] = true;
    expandMap[MaterialFilteringModel::SHOW_INSTANCES_AND_MATERIALS] = true;

    initActions();
}

MaterialEditor::~MaterialEditor()
{ 
	delete treeStateHelper;

	DAVA::VariantType v1(ui->materialProperty->header()->sectionSize(0));
	DAVA::VariantType v2(ui->materialProperty->header()->sectionSize(1));
	posSaver.SaveValue("splitPosProperties", v1);
	posSaver.SaveValue("splitPosPreview", v2);
    
	posSaver.SaveState(ui->splitter);
}

void MaterialEditor::initActions()
{
    ui->actionShowAll->setData( MaterialFilteringModel::SHOW_ALL );
    ui->actionInstances->setData( MaterialFilteringModel::SHOW_ONLY_INSTANCES );
    ui->actionMaterialsInstances->setData( MaterialFilteringModel::SHOW_INSTANCES_AND_MATERIALS );

    const int filterType =  MaterialFilteringModel::SHOW_ALL;
    foreach( QAction *action, ui->filterType->actions() )
    {
        connect( action, SIGNAL( triggered() ), SLOT( onFilterChanged() ) );
        if ( action->data().toInt() == filterType )
            action->activate( QAction::Trigger );
    }

    connect( ui->actionAutoExpand, SIGNAL( triggered( bool ) ), SLOT( onCurrentExpandModeChange( bool ) ) );
}

void MaterialEditor::autoExpand()
{
    QAction *action = ui->filterType->checkedAction();
    if ( action == NULL )
        return ;
    const int filterType = action->data().toInt();
    
    if ( expandMap[filterType] )
        ui->materialTree->expandAll();
}

void MaterialEditor::onFilterChanged()
{
    QAction *action = ui->filterType->checkedAction();
    if ( action == NULL )
        return ;
    const int filterType = action->data().toInt();
    ui->materialTree->setFilterType( filterType );

    ui->actionAutoExpand->setChecked( expandMap[filterType] );
    onCurrentExpandModeChange( expandMap[filterType] );
}

void MaterialEditor::SelectMaterial(DAVA::NMaterial *material)
{
	ui->materialTree->SelectMaterial(material);
}

void MaterialEditor::SelectEntities(DAVA::NMaterial *material)
{
	ui->materialTree->SelectEntities(material);
}

void MaterialEditor::SetCurMaterial(DAVA::NMaterial *material)
{
	curMaterial = material;

	treeStateHelper->SaveTreeViewState(false);

	FillMaterialProperties(material);
    FillMaterialTemplates(material);

	// Restore back the tree view state from the shared storage.
	if(!treeStateHelper->IsTreeStateStorageEmpty())
	{
		treeStateHelper->RestoreTreeViewState();
	}
	else
	{
		// Expand the root elements as default value.
		ui->materialProperty->expandToDepth(0);
	}
}

void MaterialEditor::sceneActivated(SceneEditor2 *scene)
{
	if(isVisible())
	{
		ui->materialTree->SetScene(scene);
        autoExpand();
	}
}

void MaterialEditor::sceneDeactivated(SceneEditor2 *scene)
{ 
	ui->materialTree->SetScene(NULL);
	SetCurMaterial(NULL);
}

void MaterialEditor::materialSelected(const QItemSelection & selected, const QItemSelection & deselected)
{
	if(1 == selected.size())
	{
        QModelIndex selectedIndex = selected.indexes().at(0);
		DAVA::NMaterial *material = ui->materialTree->GetMaterial(selectedIndex);
		SetCurMaterial(material);
	}
	else
	{
		SetCurMaterial(NULL);
	}
}

void MaterialEditor::commandExecuted(SceneEditor2 *scene, const Command2 *command, bool redo)
{
	if( command->GetId() == CMDID_INSP_DYNAMIC_MODIFY ||
		command->GetId() == CMDID_INSP_MEMBER_MODIFY || 
		command->GetId() == CMDID_META_OBJ_MODIFY)
	{
		SetCurMaterial(curMaterial);
	}
}

void MaterialEditor::onCurrentExpandModeChange( bool mode )
{
    QAction *action = ui->filterType->checkedAction();
    if ( action == NULL )
        return ;
    const int filterType = action->data().toInt();
    expandMap[filterType] = mode;

    if ( mode )
    {
        ui->materialTree->expandAll();
    }
    else
    {
        ui->materialTree->collapseAll();
    }
}

void MaterialEditor::showEvent(QShowEvent * event)
{
    FillMaterialTemplates(NULL);
	sceneActivated(QtMainWindow::Instance()->GetCurrentScene());
}

void MaterialEditor::FillMaterialProperties(DAVA::NMaterial *material)
{
    // Clear current properties
    // But don't remove properties immediately. Just extract them and remove later
    // this should be done, because we want to remove all properties when propertyEdited signal emited
    {
        QtPropertyData *propRoot = ui->materialProperty->GetRootProperty();
        while(propRoot->ChildCount() > 0)
        {
            QtPropertyData *child = propRoot->ChildGet(0);
            propRoot->ChildExtract(child);

            child->deleteLater();
        }
    }

    if(NULL != material)
    {
	    const DAVA::InspInfo *info = material->GetTypeInfo();
	    const DAVA::InspMember *materialProperties = info->Member("materialProperties");
	    const DAVA::InspMember *materialFlags = info->Member("materialSetFlags");
	    const DAVA::InspMember *materialIllumination = info->Member("illuminationParams");
        const DAVA::InspMember *materialTextures = info->Member("textures");

	    // fill material name
	    const DAVA::InspMember *nameMember = info->Member("materialName");
	    if(NULL != nameMember)
	    {
		    QtPropertyDataInspMember *name = new QtPropertyDataInspMember(material, nameMember);
		    ui->materialProperty->AppendProperty("Name", name);
	    }

        // fill material group, only for material type
        if(material->GetMaterialType() == DAVA::NMaterial::MATERIALTYPE_MATERIAL)
        {
            const DAVA::InspMember *groupMember = info->Member("materialGroup");
            if(NULL != groupMember)
            {
                QtPropertyDataInspMember *group = new QtPropertyDataInspMember(material, groupMember);
                ui->materialProperty->AppendProperty("Group", group);

                for(size_t i = 0; i < DAVA::QualitySettingsSystem::Instance()->GetMaQualityGroupCount(); ++i)
                {
                    DAVA::FastName groupName = DAVA::QualitySettingsSystem::Instance()->GetMaQualityGroupName(i);
                    group->AddAllowedValue(DAVA::VariantType(groupName), groupName.c_str());
                }
            }
        }

	    QtPropertyData *propertiesParent = new QtPropertyData();

	    // fill material flags
	    if(NULL != materialFlags)
	    {
		    const DAVA::InspMemberDynamic* dynamicInsp = materialFlags->Dynamic();

		    if(NULL != dynamicInsp)
		    {
			    DAVA::InspInfoDynamic *dynamicInfo = dynamicInsp->GetDynamicInfo();
			    DAVA::Vector<DAVA::FastName> membersList = dynamicInfo->MembersList(material); // this function can be slow
			
			    for(size_t i = 0; i < membersList.size(); ++i)
			    {
				    QtPropertyDataInspDynamic *dynamicMember = new QtPropertyDataInspDynamic(material, dynamicInfo, membersList[i]);
				    propertiesParent->ChildAdd(membersList[i].c_str(), dynamicMember);
			    }
		    }
	    }

	    // fill material properties
	    if(NULL != materialProperties)
	    {
		    const DAVA::InspMemberDynamic* dynamicInsp = materialProperties->Dynamic();

		    if(NULL != dynamicInsp)
		    {
			    DAVA::InspInfoDynamic *dynamicInfo = dynamicInsp->GetDynamicInfo();
			    DAVA::Vector<DAVA::FastName> membersList = dynamicInfo->MembersList(material); // this function can be slow

			    for(size_t i = 0; i < membersList.size(); ++i)
			    {
				    int memberFlags = dynamicInfo->MemberFlags(material, membersList[i]);
				    QtPropertyDataInspDynamic *dynamicMember = new QtPropertyDataInspDynamic(material, dynamicInfo, membersList[i]);

				    // self property
				    if(memberFlags & DAVA::I_EDIT)
				    {
					    QtPropertyToolButton* btn = dynamicMember->AddButton();
					    btn->setIcon(QIcon(":/QtIcons/cminus.png"));
					    btn->setIconSize(QSize(14, 14));
					    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(OnRemProperty()));

					    // isn't set in parent or shader
					    if(!(memberFlags & DAVA::I_VIEW) && !(memberFlags & DAVA::I_SAVE))
					    {
						    dynamicMember->SetBackground(QBrush(QColor(255, 0, 0, 10)));
					    }
				    }
				    // not self property (is set in parent or shader)
				    else
				    {
					    // disable property and it childs
					    dynamicMember->SetEnabled(false);
					    for(int m = 0; m < dynamicMember->ChildCount(); ++m)
					    {
						    dynamicMember->ChildGet(m)->SetEnabled(false);
					    }

					    QtPropertyToolButton* btn = dynamicMember->AddButton();
					    btn->setIcon(QIcon(":/QtIcons/cplus.png"));
					    btn->setIconSize(QSize(14, 14));
					    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(OnAddProperty()));

					    dynamicMember->SetBackground(QBrush(QColor(0, 0, 0, 10)));

					    // required by shader
					    //if(!(memberFlags & DAVA::I_VIEW) && (memberFlags & DAVA::I_SAVE))
					    //{	}
				    }

				    propertiesParent->ChildAdd(membersList[i].c_str(), dynamicMember);
			    }
		    }
	    }

	    ui->materialProperty->AppendProperty("Properties", propertiesParent);
	    ui->materialProperty->ApplyStyle(propertiesParent, QtPropertyEditor::HEADER_STYLE);

	    // fill illumination params
	    if(NULL != materialIllumination)
	    {
		    QtPropertyData *illumParams = QtPropertyDataIntrospection::CreateMemberData(material, materialIllumination);

		    if(illumParams->ChildCount() > 0)
		    {
			    ui->materialProperty->AppendProperty(materialIllumination->Name(), illumParams);
			    ui->materialProperty->ApplyStyle(illumParams, QtPropertyEditor::HEADER_STYLE);
		    }
		    else
		    {
			    delete illumParams;
		    }
	    }

        // fill own material textures
        if(NULL != materialTextures)
        {
            const DAVA::InspMemberDynamic* dynamicInsp = materialTextures->Dynamic();
            QtPropertyData *texturesParent = new QtPropertyData();

            if(NULL != dynamicInsp)
            {
                DAVA::InspInfoDynamic *dynamicInfo = dynamicInsp->GetDynamicInfo();
                DAVA::Vector<DAVA::FastName> membersList = dynamicInfo->MembersList(material); // this function can be slow

                QString dataSourcePath = ProjectManager::Instance()->CurProjectDataSourcePath();
                for(size_t i = 0; i < membersList.size(); ++i)
                {
                    int memberFlags = dynamicInfo->MemberFlags(material, membersList[i]);
                    QtPropertyDataInspDynamic *dynamicMember = new QtPropertyDataInspDynamic(material, dynamicInfo, membersList[i]);

                    dynamicMember->SetDefaultOpenDialogPath(dataSourcePath);
                    dynamicMember->SetOpenDialogFilter("All (*.tex *.png);;PNG (*.png);;TEX (*.tex)");
                    QStringList path;
                    path.append(dataSourcePath);
                    dynamicMember->SetValidator(new TexturePathValidator(path));
                    // self property
                    if(memberFlags & DAVA::I_EDIT)
                    {
                        QtPropertyToolButton* btn = dynamicMember->AddButton();
                        btn->setIcon(QIcon(":/QtIcons/cminus.png"));
                        btn->setIconSize(QSize(14, 14));
                        QObject::connect(btn, SIGNAL(clicked()), this, SLOT(OnRemTexture()));

                        // isn't set in parent or shader
                        if(!(memberFlags & DAVA::I_VIEW) && !(memberFlags & DAVA::I_SAVE))
                        {
                            dynamicMember->SetBackground(QBrush(QColor(255, 0, 0, 10)));
                        }
                    }
                    // not self property (is set in parent or shader)
                    else
                    {
                        // disable property and it childs
                        dynamicMember->SetEnabled(false);
                        for(int m = 0; m < dynamicMember->ChildCount(); ++m)
                        {
                            dynamicMember->ChildGet(m)->SetEnabled(false);
                        }

                        QtPropertyToolButton* btn = dynamicMember->AddButton();
                        btn->setIcon(QIcon(":/QtIcons/cplus.png"));
                        btn->setIconSize(QSize(14, 14));
                        QObject::connect(btn, SIGNAL(clicked()), this, SLOT(OnAddTexture()));

                        dynamicMember->SetBackground(QBrush(QColor(0, 0, 0, 10)));
                    }

                    texturesParent->ChildAdd(membersList[i].c_str(), dynamicMember);
                }

                ui->materialProperty->AppendProperty("Textures", texturesParent);
                ui->materialProperty->ApplyStyle(texturesParent, QtPropertyEditor::HEADER_STYLE);
            }
        }
    }
}

void MaterialEditor::FillMaterialTemplates(DAVA::NMaterial *material)
{
    if(0 == ui->templateBox->count())
    {
        ui->templateBox->addItem("");

        const QVector<ProjectManager::AvailableMaterialTemplate> *templates = ProjectManager::Instance()->GetAvailableMaterialTemplates();
        for(int i = 0; i < templates->size(); ++i)
        {
            ui->templateBox->addItem(templates->at(i).name, templates->at(i).path);
        }
    }

    if(NULL != material)
    {
        int indexToSet = -1;
        QString curMaterialTemplate = material->GetMaterialTemplate()->name.c_str();

        for(int i = 0; i < ui->templateBox->count(); ++i)
        {
            if(curMaterialTemplate == ui->templateBox->itemData(i).toString())
            {
                indexToSet = i;
                break;
            }
        }

        if(-1 != indexToSet)
        {
            ui->templateBox->setCurrentIndex(indexToSet);
            ui->templateBox->setItemText(0, "");
        }
        else
        {
            // add template path to the name
            ui->templateBox->setCurrentIndex(0);
            ui->templateBox->setItemText(0, "NON-ASSIGNABLE: " + curMaterialTemplate);
        }

        // enable template selection only for real materials, not instances
        // but don't allow to change template for runtime materials
        if(material->GetMaterialType() == DAVA::NMaterial::MATERIALTYPE_MATERIAL &&
            material->GetNodeGlags() != DAVA::DataNode::NodeRuntimeFlag)
        {
            ui->templateBox->setEnabled(true);
        }
        else
        {
            ui->templateBox->setEnabled(false);
        }
    }
    else
    {
        ui->templateBox->setCurrentIndex(0);
        ui->templateBox->setItemText(0, "");
        ui->templateBox->setEnabled(false);
    }
}

void MaterialEditor::OnAddProperty()
{
	QtPropertyToolButton *btn = dynamic_cast<QtPropertyToolButton *>(QObject::sender());
	
	if(NULL != btn && NULL != curMaterial)
	{
		QtPropertyDataInspDynamic *data = dynamic_cast<QtPropertyDataInspDynamic *>(btn->GetPropertyData());
		if(NULL != data)
		{
			data->SetVariantValue(data->GetAliasVariant());
			data->SetValue(data->GetValue(), QtPropertyData::VALUE_EDITED);
			data->EmitDataChanged(QtPropertyData::VALUE_EDITED);

			// reload material properties
			SetCurMaterial(curMaterial);
		}
	}
}

void MaterialEditor::OnRemProperty()
{
	QtPropertyToolButton *btn = dynamic_cast<QtPropertyToolButton *>(QObject::sender());

	if(NULL != btn && NULL != curMaterial)
	{
		QtPropertyDataInspDynamic *data = dynamic_cast<QtPropertyDataInspDynamic *>(btn->GetPropertyData());
		if(NULL != data)
		{
			data->SetValue(QVariant(), QtPropertyData::VALUE_EDITED);

			// reload material properties
			SetCurMaterial(curMaterial);
		}
	}
}

void MaterialEditor::OnAddTexture()
{
	QtPropertyToolButton *btn = dynamic_cast<QtPropertyToolButton *>(QObject::sender());

	if(NULL != btn && NULL != curMaterial)
	{
		QtPropertyDataInspDynamic *data = dynamic_cast<QtPropertyDataInspDynamic *>(btn->GetPropertyData());
		if(NULL != data)
		{
			data->SetVariantValue(data->GetAliasVariant());
			data->SetValue(data->GetValue(), QtPropertyData::VALUE_EDITED);
			data->EmitDataChanged(QtPropertyData::VALUE_EDITED);

			// reload material properties
			SetCurMaterial(curMaterial);
		}
	}
}

void MaterialEditor::OnRemTexture()
{
	QtPropertyToolButton *btn = dynamic_cast<QtPropertyToolButton *>(QObject::sender());

	if(NULL != btn && NULL != curMaterial)
	{
		QtPropertyDataInspDynamic *data = dynamic_cast<QtPropertyDataInspDynamic *>(btn->GetPropertyData());
		if(NULL != data)
		{
			data->SetValue(QVariant(), QtPropertyData::VALUE_EDITED);

			// reload material properties
			SetCurMaterial(curMaterial);
		}
	}
}

void MaterialEditor::OnTemplateChanged(int index)
{
	if(NULL != curMaterial && index > 0)
	{
        QString newTemplatePath = ui->templateBox->itemData(index).toString();
        if(!newTemplatePath.isEmpty())
        {
            const DAVA::InspMember *templateMember = curMaterial->GetTypeInfo()->Member("materialTemplate");

            if(NULL != templateMember)
            {
                QtMainWindow::Instance()->GetCurrentScene()->Exec(new InspMemberModifyCommand(templateMember, curMaterial, 
                    DAVA::VariantType(DAVA::FastName(newTemplatePath.toStdString().c_str()))));
            }
        }
	}

	SetCurMaterial(curMaterial);
}

void MaterialEditor::OnPropertyEdited(const QModelIndex &index)
{
	QtPropertyEditor *editor = dynamic_cast<QtPropertyEditor *>(QObject::sender());
	if(NULL != editor)
	{
		QtPropertyData *propData = editor->GetProperty(index);

		if(NULL != propData)
		{
			Command2 *command = (Command2 *) propData->CreateLastCommand();
			if(NULL != command)
			{
				SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
				if(NULL != curScene)
				{
					QtMainWindow::Instance()->GetCurrentScene()->Exec(command);
				}
			}
		}
	}

	ui->materialTree->Update();
}

void MaterialEditor::OnSwitchQuality(bool checked)
{
    QualitySwitcher::Show();
}

QVariant MaterialEditor::CheckForTextureDescriptor(const QVariant& value)
{
    if (value.type() == QVariant::String)
    {
        String s = value.toString().toStdString();
        FilePath fp = FilePath(s);
        if (!fp.IsEmpty() && fp.Exists())
        {
            if (fp.GetExtension() == ".png")
            {
                TextureDescriptorUtils::CreateDescriptorIfNeed(fp);
                FilePath texFile = TextureDescriptor::GetDescriptorPathname(fp);
                return QVariant(QString::fromStdString(texFile.GetAbsolutePathname()));
            }
        }
    }
    return QVariant();
}

void MaterialEditor::OnMaterialReload(bool checked)
{
    if (curMaterial != 0)
    {
        
    }
}
