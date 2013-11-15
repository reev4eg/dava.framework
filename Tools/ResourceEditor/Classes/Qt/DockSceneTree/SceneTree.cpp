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



#include "DockSceneTree/SceneTree.h"
#include "Main/mainwindow.h"
#include "StringConstants.h"
#include <QBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QDropEvent>
#include <QMenu>

#include "SceneEditor/EditorSettings.h"
#include "SceneEditor/SceneValidator.h"
#include "Main/QTUtils.h"
#include "Project/ProjectManager.h"
#include "Scene/SceneEditor2.h"
#include "Scene/System/SelectionSystem.h"
#include "Tools/QtFileDialog/QtFileDialog.h"


// framework
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/ParticleEffectComponent.h"

// commands
#include "Commands2/ParticleEditorCommands.h"
#include "Commands2/SaveEntityAsAction.h"
#include "Commands2/ConvertToShadowCommand.h"

SceneTree::SceneTree(QWidget *parent /*= 0*/)
	: QTreeView(parent)
	, isInSync(false)
{
	CleanupParticleEditorSelectedItems();

	treeModel = new SceneTreeModel();
	filteringProxyModel = new SceneTreeFilteringModel(treeModel);

	setModel(filteringProxyModel);

	treeDelegate = new SceneTreeDelegate();
	setItemDelegate(treeDelegate);

	setDragDropMode(QAbstractItemView::DragDrop);
	setDragEnabled(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setContextMenuPolicy(Qt::CustomContextMenu);

	// scene signals
	QObject::connect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2 *)), this, SLOT(SceneActivated(SceneEditor2 *)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(Deactivated(SceneEditor2 *)), this, SLOT(SceneDeactivated(SceneEditor2 *)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(StructureChanged(SceneEditor2 *, DAVA::Entity *)), this, SLOT(SceneStructureChanged(SceneEditor2 *, DAVA::Entity *)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(SelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)), this, SLOT(SceneSelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)));

	// particles signals
	QObject::connect(SceneSignals::Instance(), SIGNAL(ParticleLayerValueChanged(SceneEditor2*, DAVA::ParticleLayer*)), this, SLOT(ParticleLayerValueChanged(SceneEditor2*, DAVA::ParticleLayer*)));

	// this widget signals
	QObject::connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(TreeSelectionChanged(const QItemSelection &, const QItemSelection &)));
	QObject::connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(TreeItemClicked(const QModelIndex &)));
	QObject::connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(TreeItemDoubleClicked(const QModelIndex &)));
	QObject::connect(this, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(TreeItemCollapsed(const QModelIndex &)));
	QObject::connect(this, SIGNAL(expanded(const QModelIndex &)), this, SLOT(TreeItemExpanded(const QModelIndex &)));
	QObject::connect(&refreshTimer, SIGNAL(timeout()), this, SLOT(OnRefreshTimeout()));

	QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));

	refreshTimer.start(1500);
}

SceneTree::~SceneTree()
{

}

void SceneTree::SetFilter(const QString &filter)
{
	filteringProxyModel->setFilterRegExp(QRegExp(filter, Qt::CaseInsensitive, QRegExp::FixedString));
    
    SyncSelectionToTree();
}

void SceneTree::GetDropParams(const QPoint &pos, QModelIndex &index, int &row, int &col)
{
	row = -1;
	col = -1;
	index = indexAt(pos);

	switch (dropIndicatorPosition()) 
	{
	case QAbstractItemView::AboveItem:
		row = index.row();
		col = index.column();
		index = index.parent();
		break;
	case QAbstractItemView::BelowItem:
		row = index.row() + 1;
		col = index.column();
		index = index.parent();
		break;
	case QAbstractItemView::OnItem:
	case QAbstractItemView::OnViewport:
		break;
	}
}

void SceneTree::dropEvent(QDropEvent * event)
{
	QTreeView::dropEvent(event);

	if(treeModel->DropAccepted())
	{
		int row, col; 
		QModelIndex parent;

		GetDropParams(event->pos(), parent, row, col);
		expand(parent);
	}

	// after processing don't allow this event to go higher
	// so no body will decide to remove/insert grag&dropped items into treeview
	// except our model. Model will do this when scene entity remove/move signals catched
	event->setDropAction(Qt::IgnoreAction);
	event->accept();
}

void SceneTree::dragMoveEvent(QDragMoveEvent *event)
{
	QTreeView::dragMoveEvent(event);

	{
		int row, col; 
		QModelIndex parent;

		GetDropParams(event->pos(), parent, row, col);
		if(treeModel->DropCanBeAccepted(event->mimeData(), event->dropAction(), row, col, filteringProxyModel->mapToSource(parent)))
		{
			event->setDropAction(Qt::MoveAction);
			event->accept();
		}
		else
		{
			event->setDropAction(Qt::IgnoreAction);
			event->accept();
		}
	}
}

void SceneTree::dragEnterEvent(QDragEnterEvent *event)
{
	QTreeView::dragEnterEvent(event);

	{
		int row, col; 
		QModelIndex parent;

		GetDropParams(event->pos(), parent, row, col);
		if(treeModel->DropCanBeAccepted(event->mimeData(), event->dropAction(), row, col, filteringProxyModel->mapToSource(parent)))
		{
			event->setDropAction(Qt::MoveAction);
			event->accept();
		}
		else
		{
			event->setDropAction(Qt::IgnoreAction);
			event->accept();
		}
	}
}

void SceneTree::SceneActivated(SceneEditor2 *scene)
{
	treeModel->SetScene(scene);
	SyncSelectionToTree();
}

void SceneTree::SceneDeactivated(SceneEditor2 *scene)
{
	if(treeModel->GetScene() == scene)
	{
		treeModel->SetScene(NULL);
	}
}

void SceneTree::SceneSelectionChanged(SceneEditor2 *scene, const EntityGroup *selected, const EntityGroup *deselected)
{
	if(scene == treeModel->GetScene())
	{
		SyncSelectionToTree();
	}
}

void SceneTree::SceneStructureChanged(SceneEditor2 *scene, DAVA::Entity *parent)
{
	if(scene == treeModel->GetScene())
	{
		treeModel->ResyncStructure(treeModel->invisibleRootItem(), treeModel->GetScene());
		SyncSelectionToTree();
	}
}

void SceneTree::TreeSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	SyncSelectionFromTree();

	// emit some signal about particles
	EmitParticleSignals(selected);
}

void SceneTree::TreeItemClicked(const QModelIndex & index)
{
	SceneEditor2* sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		// TODO:
		// ...
	}
}

void SceneTree::ParticleLayerValueChanged(SceneEditor2* scene, DAVA::ParticleLayer* layer)
{
	QModelIndexList indexList = selectionModel()->selection().indexes();
	if (indexList.empty())
	{
		return;
	}

	QModelIndex realIndex = filteringProxyModel->mapToSource(indexList[0]);
	SceneTreeItem *item = treeModel->GetItem(realIndex);
	if (item->ItemType() != SceneTreeItem::EIT_Layer)
	{
		return;
	}

	ParticleLayer* selectedLayer = SceneTreeItemParticleLayer::GetLayer(item);
	if (selectedLayer != layer)
	{
		return;
	}
	
	// Update the "isEnabled" flag, if it is changed.
	bool sceneTreeItemChecked = item->checkState() == Qt::Checked;
	if (layer->GetDisabled() == sceneTreeItemChecked)
	{
		blockSignals(true);
		item->setCheckState(sceneTreeItemChecked ? Qt::Unchecked : Qt::Checked);
		blockSignals(false);
	}
	
	//check if we need to resync tree for superemmiter	
	SceneTreeItemParticleLayer *itemLayer = (SceneTreeItemParticleLayer *) item;
	bool needEmmiter = selectedLayer->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES;	
	if (itemLayer->hasInnerEmmiter!=needEmmiter)
	{
		itemLayer->hasInnerEmmiter = needEmmiter;
		treeModel->ResyncStructure(treeModel->invisibleRootItem(), treeModel->GetScene());		
	}
}

void SceneTree::TreeItemDoubleClicked(const QModelIndex & index)
{
    LookAtSelection();
}

void SceneTree::ShowContextMenu(const QPoint &pos)
{
	CleanupParticleEditorSelectedItems();
	QModelIndex index = filteringProxyModel->mapToSource(indexAt(pos));
	SceneTreeItem *item = treeModel->GetItem(index);

	if(NULL != item)
	{
		switch (item->ItemType())
		{
		case SceneTreeItem::EIT_Entity:
			ShowContextMenuEntity(SceneTreeItemEntity::GetEntity(item), treeModel->GetCustomFlags(index), mapToGlobal(pos));
			break;

		case SceneTreeItem::EIT_Layer:
			ShowContextMenuLayer(SceneTreeItemParticleLayer::GetLayer(item), mapToGlobal(pos));
			break;

		case SceneTreeItem::EIT_InnerEmmiter:
			ShowContextMenuInnerEmitter(((SceneTreeItemParticleInnerEmmiter *)item)->emitter, ((SceneTreeItemParticleInnerEmmiter *)item)->parent, mapToGlobal(pos));
			break;

		case SceneTreeItem::EIT_Force:
		{
			// We have to know both Layer and Force.
			QStandardItem* parentItem = item->parent();
			if (!parentItem)
			{
				DVASSERT(false);
				return;
			}

			SceneTreeItem* layerItem = treeModel->GetItem(parentItem->index());
			DVASSERT(layerItem->ItemType() == SceneTreeItem::EIT_Layer);
			ShowContextMenuForce(SceneTreeItemParticleLayer::GetLayer(layerItem),
								 SceneTreeItemParticleForce::GetForce(item), mapToGlobal(pos));
			break;
		}

		default:
			break;
		}
	}
}

void SceneTree::ShowContextMenuEntity(DAVA::Entity *entity, int entityCustomFlags, const QPoint &pos)
{
	if(NULL != entity)
	{
		//Get selection size to show different menues
		const SceneEditor2 *scene = QtMainWindow::Instance()->GetCurrentScene();
		SceneSelectionSystem *selSystem = scene->selectionSystem;
		size_t selectionSize = selSystem->GetSelectionCount();

		QMenu contextMenu;
		if(entityCustomFlags & SceneTreeModel::CF_Disabled)
		{
			// disabled entities can only be removed
			contextMenu.addAction(QIcon(":/QtIcons/remove.png"), "Remove entity", this, SLOT(RemoveSelection()));
		}
		else
		{
			// look at
			contextMenu.addAction(QIcon(":/QtIcons/zoom.png"), "Look at", this, SLOT(LookAtSelection()));

			// look from
			if(NULL != GetCamera(entity))
			{
				contextMenu.addAction(QIcon(":/QtIcons/eye.png"), "Look from", this, SLOT(SetCurrentCamera()));
			}

			// add/remove
			contextMenu.addSeparator();
			contextMenu.addAction(QIcon(":/QtIcons/remove.png"), "Remove entity", this, SLOT(RemoveSelection()));

			// lock/unlock
			contextMenu.addSeparator();
			QAction *lockAction = contextMenu.addAction(QIcon(":/QtIcons/lock_add.png"), "Lock", this, SLOT(LockEntities()));
			QAction *unlockAction = contextMenu.addAction(QIcon(":/QtIcons/lock_delete.png"), "Unlock", this, SLOT(UnlockEntities()));
			if(entity->GetLocked())
			{
				lockAction->setDisabled(true);
			}
			else
			{
				unlockAction->setDisabled(true);
			}

			DAVA::ParticleEmitter* emitter = DAVA::GetEmitter(entity);
			// show save as/reload/edit for regular entity
			if(NULL == emitter)
			{
				// save model as
				contextMenu.addSeparator();
				contextMenu.addAction(QIcon(":/QtIcons/save_as.png"), "Save Entity As...", this, SLOT(SaveEntityAs()));

				DAVA::KeyedArchive *customProp = entity->GetCustomProperties();
				if(NULL != customProp)
				{
					DAVA::FilePath ownerRef = customProp->GetString(ResourceEditor::EDITOR_REFERENCE_TO_OWNER);
					if(!ownerRef.IsEmpty())
					{
						if(selectionSize == 1)
						{
							QAction *editModelAction = contextMenu.addAction("Edit Model", this, SLOT(EditModel()));
						}

						QAction *reloadModelAction = contextMenu.addAction("Reload Model...", this, SLOT(ReloadModel()));
					}
				}
				//DF-2004: Reload for every entity at scene
				QAction *reloadModelAsAction = contextMenu.addAction("Reload Model As...", this, SLOT(ReloadModelAs()));
			}
			// but particle emitter has it own menu actions
			else
			{
				contextMenu.addSeparator();
				QMenu *particleEffectMenu = contextMenu.addMenu("Particle Emitter");

				particleEffectMenu->addAction(QIcon(":/QtIcons/layer_particle.png"), "Add Layer", this, SLOT(AddLayer()));
				particleEffectMenu->addSeparator();
				particleEffectMenu->addAction(QIcon(":/QtIcons/openscene.png"), "Load Emitter from Yaml", this, SLOT(LoadEmitterFromYaml()));
				particleEffectMenu->addAction(QIcon(":/QtIcons/savescene.png"), "Save Emitter to Yaml", this, SLOT(SaveEmitterToYaml()));
				particleEffectMenu->addAction(QIcon(":/QtIcons/save_as.png"), "Save Emitter to Yaml As...", this, SLOT(SaveEmitterToYamlAs()));
			}

			// particle effect
			DAVA::ParticleEffectComponent* effect = DAVA::GetEffectComponent(entity);
			if(NULL != effect)
			{
				contextMenu.addSeparator();
				QMenu *particleEffectMenu = contextMenu.addMenu("Particle Effect");

				particleEffectMenu->addAction(QIcon(":/QtIcons/emitter_particle.png"), "Add Emitter", this, SLOT(AddEmitter()));
				particleEffectMenu->addSeparator();
				particleEffectMenu->addAction(QIcon(":/QtIcons/play.png"), "Start", this, SLOT(StartEmitter()));
				particleEffectMenu->addAction(QIcon(":/QtIcons/stop.png"), "Stop", this, SLOT(StopEmitter()));
				particleEffectMenu->addAction(QIcon(":/QtIcons/restart.png"), "Restart", this, SLOT(RestartEmitter()));
			}

			if(ConvertToShadowCommand::IsAvailableForConvertionToShadowVolume(entity))
			{
				contextMenu.addSeparator();
				contextMenu.addAction(QtMainWindow::Instance()->GetUI()->actionConvertToShadow);
			}


			//      Disabled for 0.5.5 version
			//		SceneEditor2* sceneEditor = treeModel->GetScene();
			//		if(NULL != sceneEditor)
			//		{
			//			if(sceneEditor->selectionSystem->GetSelectionCount() > 1)
			//			{
			//				contextMenu.addSeparator();
			//				contextMenu.addAction("Group to entity with merged LODs", QtMainWindow::Instance(), SLOT(OnUniteEntitiesWithLODs()));
			//			}
			//		}

			if(selectionSize == 1)
			{
				contextMenu.addSeparator();
				contextMenu.addAction(QIcon(":/QtIconsTextureDialog/filter.png"), "Set name as filter",this, SLOT(SetEntityNameAsFilter()));
			}
		}

		contextMenu.exec(pos);
	}
}

void SceneTree::ShowContextMenuLayer(DAVA::ParticleLayer *layer, const QPoint &pos)
{
	this->selectedLayer = layer;

	if (NULL == layer)
	{
		return;
	}

	QMenu contextMenu;
	contextMenu.addAction(QIcon(":/QtIcons/clone.png"), "Clone Layer", this, SLOT(CloneLayer()));
	contextMenu.addAction(QIcon(":/QtIcons/remove_layer.png"), "Remove Layer", this, SLOT(RemoveLayer()));
	contextMenu.addSeparator();
	contextMenu.addAction(QIcon(":/QtIcons/force.png"), "Add Force", this, SLOT(AddForce()));

	contextMenu.exec(pos);
}

void SceneTree::ShowContextMenuForce(DAVA::ParticleLayer* layer, DAVA::ParticleForce *force, const QPoint &pos)
{
	this->selectedLayer = layer;
	this->selectedForce = force;

	QMenu contextMenu;
	contextMenu.addAction(QIcon(":/QtIcons/remove_force.png"), "Remove Force", this, SLOT(RemoveForce()));
	contextMenu.exec(pos);
	
}

void SceneTree::ShowContextMenuInnerEmitter(DAVA::ParticleEmitter *emitter, DAVA::ParticleLayer *parentLayer, const QPoint &pos)
{
	this->selectedInnerEmmiter = emitter;
	this->selectedInnerEmmiterParentLayer = parentLayer;
	QMenu contextMenu;		
	contextMenu.addAction(QIcon(":/QtIcons/layer_particle.png"), "Add Layer", this, SLOT(AddLayer()));
	contextMenu.addSeparator();
	contextMenu.addAction(QIcon(":/QtIcons/openscene.png"), "Load Emitter from Yaml", this, SLOT(LoadInnerEmitterFromYaml()));
	contextMenu.addAction(QIcon(":/QtIcons/savescene.png"), "Save Emitter to Yaml", this, SLOT(SaveInnerEmitterToYaml()));
	contextMenu.addAction(QIcon(":/QtIcons/save_as.png"), "Save Emitter to Yaml As...", this, SLOT(SaveInnerEmitterToYamlAs()));
	contextMenu.exec(pos);
}


void SceneTree::LookAtSelection()
{
	SceneEditor2* sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		EntityGroup selection = sceneEditor->selectionSystem->GetSelection();
		if(selection.Size() > 0)
		{
			sceneEditor->cameraSystem->LookAt(selection.GetCommonBbox());
		}
	}
}

void SceneTree::RemoveSelection()
{
	SceneEditor2* sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		EntityGroup selection = sceneEditor->selectionSystem->GetSelection();
		sceneEditor->structureSystem->Remove(selection);
	}
}

void SceneTree::LockEntities()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		SceneSelectionSystem *ss = sceneEditor->selectionSystem;
		for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
		{
			ss->GetSelectionEntity(i)->SetLocked(true);
		}

		sceneEditor->MarkAsChanged();
	}
}

void SceneTree::UnlockEntities()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		SceneSelectionSystem *ss = sceneEditor->selectionSystem;
		for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
		{
			ss->GetSelectionEntity(i)->SetLocked(false);
		}
		sceneEditor->MarkAsChanged();
	}
}

void SceneTree::SetCurrentCamera()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		DAVA::Camera *camera = GetCamera(sceneEditor->selectionSystem->GetSelectionEntity(0));
		if(NULL != camera)
		{
			sceneEditor->SetCurrentCamera(camera);
		}
	}
}

void SceneTree::CollapseSwitch()
{
	QModelIndexList indexList = selectionModel()->selection().indexes();
	for (int i = 0; i < indexList.size(); ++i)
	{
		QModelIndex index = indexList.at(i);

		if(isExpanded(index))
		{
			collapse(index);
		}
		else
		{
			expand(index);
		}
	}
}

void SceneTree::EditModel()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		SceneSelectionSystem *ss = sceneEditor->selectionSystem;

		for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
		{
			DAVA::Entity *entity = ss->GetSelectionEntity(i);
			if(NULL != entity && NULL != entity->GetCustomProperties())
			{
				DAVA::KeyedArchive *archive = entity->GetCustomProperties();
				DAVA::FilePath entityRefPath = archive->GetString(ResourceEditor::EDITOR_REFERENCE_TO_OWNER);
				if(entityRefPath.Exists())
				{
					QtMainWindow::Instance()->OpenScene(entityRefPath.GetAbsolutePathname().c_str());
				}
				else
				{
					ShowErrorDialog(ResourceEditor::SCENE_TREE_WRONG_REF_TO_OWNER + entityRefPath.GetAbsolutePathname());
				}
			}
		}
	}
}

void SceneTree::ReloadModel()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		QDialog *dlg = new QDialog(this);

		QVBoxLayout *dlgLayout = new QVBoxLayout();
		dlgLayout->setMargin(10);

		dlg->setWindowTitle("Reload Model options");
		dlg->setLayout(dlgLayout);
	
		QCheckBox *lightmapsChBox = new QCheckBox("Reload lightmaps", dlg);
		dlgLayout->addWidget(lightmapsChBox);
		lightmapsChBox->setCheckState(Qt::Checked);

		QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dlg);
		dlgLayout->addWidget(buttons);

		QObject::connect(buttons, SIGNAL(accepted()), dlg, SLOT(accept()));
		QObject::connect(buttons, SIGNAL(rejected()), dlg, SLOT(reject()));

		if(QDialog::Accepted == dlg->exec())
		{
			EntityGroup selection = sceneEditor->selectionSystem->GetSelection();
			String wrongPathes;
			for(int i = 0; i < selection.Size(); ++i)
			{
				DAVA::Entity *entity = selection.GetEntity(i);
				DAVA::FilePath pathToReload(entity->GetCustomProperties()->GetString(ResourceEditor::EDITOR_REFERENCE_TO_OWNER));
				if(!pathToReload.Exists())
				{
					wrongPathes += Format("\r\n%s : %s",entity->GetName().c_str(),
										  pathToReload.GetAbsolutePathname().c_str());
				}
			}
			if(!wrongPathes.empty())
			{
				ShowErrorDialog(ResourceEditor::SCENE_TREE_WRONG_REF_TO_OWNER + wrongPathes);
			}
			sceneEditor->structureSystem->ReloadEntities(selection, lightmapsChBox->isChecked());
		}

		delete dlg;
	}
}

void SceneTree::ReloadModelAs()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		DAVA::Entity *entity = sceneEditor->selectionSystem->GetSelectionEntity(0);
		if(NULL != entity)
		{
			DAVA::String ownerPath = entity->GetCustomProperties()->GetString(ResourceEditor::EDITOR_REFERENCE_TO_OWNER);
			if(ownerPath.empty())
			{
				FilePath p = sceneEditor->GetScenePath().GetDirectory();
				if(p.Exists() && sceneEditor->IsLoaded())
				{
					ownerPath = p.GetAbsolutePathname();
				}
				else
				{
					ownerPath = EditorSettings::Instance()->GetDataSourcePath().GetAbsolutePathname();
				}
			}

			QString filePath = QtFileDialog::getOpenFileName(NULL, QString("Open scene file"), ownerPath.c_str(), QString("DAVA SceneV2 (*.sc2)"));
			if(!filePath.isEmpty())
			{
				sceneEditor->structureSystem->ReloadEntitiesAs(sceneEditor->selectionSystem->GetSelection(), filePath.toStdString());
			}
		}
	}
}

void SceneTree::SaveEntityAs()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		EntityGroup selection = sceneEditor->selectionSystem->GetSelection();
		if(selection.Size() > 0)
		{
			DAVA::FilePath scenePath = sceneEditor->GetScenePath().GetDirectory();
			if(!scenePath.Exists() || !sceneEditor->IsLoaded())
			{
				scenePath = DAVA::FilePath(ProjectManager::Instance()->CurProjectDataSourcePath().toStdString());
			}

			QString filePath = QtFileDialog::getSaveFileName(NULL, QString("Save scene file"), QString(scenePath.GetDirectory().GetAbsolutePathname().c_str()), QString("DAVA SceneV2 (*.sc2)"));
			if(!filePath.isEmpty())
			{
				sceneEditor->Exec(new SaveEntityAsAction(&selection, filePath.toStdString()));
			}
		}
	}
}

void SceneTree::TreeItemCollapsed(const QModelIndex &index)
{
	treeModel->SetSolid(filteringProxyModel->mapToSource(index), true);

	bool needSync = false;

	// if selected items were inside collapsed item, remove them from selection
	QModelIndexList indexList = selectionModel()->selection().indexes();
	for (int i = 0; i < indexList.size(); ++i)
	{
		QModelIndex childIndex = indexList[i];
		QModelIndex childParent = childIndex.parent();
		while(childParent.isValid())
		{
			if(childParent == index)
			{
				selectionModel()->select(childIndex, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
				needSync = true;
				break;
			}

			childParent = childParent.parent();
		}
	}

	if(needSync)
	{
		SyncSelectionFromTree();
	}
}

void SceneTree::TreeItemExpanded(const QModelIndex &index)
{
	treeModel->SetSolid(filteringProxyModel->mapToSource(index), false);
}

void SceneTree::SyncSelectionToTree()
{
	if(!isInSync)
	{
		isInSync = true;

		SceneEditor2* curScene = treeModel->GetScene();
		if(NULL != curScene)
		{
			QModelIndex lastValidIndex;

			selectionModel()->clear();

			SceneSelectionSystem *ss = curScene->selectionSystem;
			for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
			{
				QModelIndex sIndex = treeModel->GetIndex(ss->GetSelectionEntity(i));
				sIndex = filteringProxyModel->mapFromSource(sIndex);

				if(sIndex.isValid())
				{
					lastValidIndex = sIndex;
					selectionModel()->select(sIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
				}
			}

			if(lastValidIndex.isValid())
			{
				scrollTo(lastValidIndex, QAbstractItemView::PositionAtCenter);
			}
		}

		isInSync = false;
	}
}

void SceneTree::SyncSelectionFromTree()
{
	if(!isInSync)
	{
		isInSync = true;

		SceneEditor2* curScene = treeModel->GetScene();
		if(NULL != curScene)
		{
			QSet<DAVA::Entity*> treeSelectedEntities;

			// remove from selection system all entities that are not selected in tree
			EntityGroup selGroup = curScene->selectionSystem->GetSelection();
			for(size_t i = 0; i < selGroup.Size(); ++i)
			{
				if(!treeSelectedEntities.contains(selGroup.GetEntity(i)))
				{
					curScene->selectionSystem->RemSelection(selGroup.GetEntity(i));
				}
			}

			// select items in scene
			QModelIndexList indexList = selectionModel()->selection().indexes();
			for (int i = 0; i < indexList.size(); ++i)
			{
				DAVA::Entity *entity = SceneTreeItemEntity::GetEntity(treeModel->GetItem(filteringProxyModel->mapToSource(indexList[i])));

				if(NULL != entity)
				{
					treeSelectedEntities.insert(entity);
					curScene->selectionSystem->AddSelection(entity);
				}
			}

			// force selection system emit signals about new selection
			// this should be done until we are inSync mode, to prevent unnecessary updates
			// when signals from selection system will be emited on next frame
			curScene->selectionSystem->ForceEmitSignals();
		}

		isInSync = false;
	}
}

void SceneTree::EmitParticleSignals(const QItemSelection & selected)
{
	SceneEditor2* curScene = treeModel->GetScene();
	bool isParticleElements = false;

	// allow only single selected entities
	if(selected.size() == 1) 
	{
		QModelIndexList indexList = selectionModel()->selection().indexes();
		SceneTreeItem *item = treeModel->GetItem(filteringProxyModel->mapToSource(indexList[0]));

		if(NULL != item)
		{
			switch(item->ItemType())
			{
			case SceneTreeItem::EIT_Entity:
				{
					DAVA::Entity *entity = SceneTreeItemEntity::GetEntity(item);
					if(NULL != DAVA::GetEffectComponent(entity))
					{
						SceneSignals::Instance()->EmitEffectSelected(curScene, entity);
						isParticleElements = true;
					}
					else if(NULL != DAVA::GetEmitter(entity))
					{
						SceneSignals::Instance()->EmitEmitterSelected(curScene, entity);
						isParticleElements = true;
					}
				}
				break;
			case SceneTreeItem::EIT_InnerEmmiter:
				{
					SceneTreeItemParticleInnerEmmiter *itemEmitter = (SceneTreeItemParticleInnerEmmiter *) item;
					SceneSignals::Instance()->EmitInnerEmitterSelected(curScene, itemEmitter->emitter);
					isParticleElements = true;
				}
				break;
			case SceneTreeItem::EIT_Layer:
				{
					SceneTreeItemParticleLayer *itemLayer = (SceneTreeItemParticleLayer *) item;
					if(NULL != itemLayer->parent && NULL != itemLayer->layer)
					{
						SceneSignals::Instance()->EmitLayerSelected(curScene, itemLayer->layer, false);
						isParticleElements = true;
					}
				}
				break;
			case SceneTreeItem::EIT_Force:
				{
					SceneTreeItemParticleForce *itemForce = (SceneTreeItemParticleForce *) item;
					DAVA::ParticleLayer* layer = itemForce->parent;
					if(NULL != layer)
					{
						for(int i = 0; i < (int) layer->forces.size(); ++i)
						{
							if(layer->forces[i] == itemForce->force)
							{
								SceneSignals::Instance()->EmitForceSelected(curScene, layer, i);
								isParticleElements = true;

								break;
							}
						}
					}
				}
				break;
			}
		}
	}

	if(!isParticleElements)
	{
		SceneSignals::Instance()->EmitEmitterSelected(NULL, NULL);
	}
}

void SceneTree::AddEmitter()
{
	SceneEditor2* sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		QModelIndex realIndex = filteringProxyModel->mapToSource(currentIndex());
		DAVA::Entity *curEntity = SceneTreeItemEntity::GetEntity(treeModel->GetItem(realIndex));
		if(NULL != curEntity && DAVA::GetEffectComponent(curEntity))
		{
			CommandAddParticleEmitter* command = new CommandAddParticleEmitter(curEntity);
			sceneEditor->Exec(command);
			treeModel->ResyncStructure(treeModel->invisibleRootItem(), sceneEditor);
		}
	}
}

void SceneTree::StartEmitter()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		SceneSelectionSystem *ss = sceneEditor->selectionSystem;
		for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
		{
			DAVA::ParticleEffectComponent *effect = DAVA::GetEffectComponent(ss->GetSelectionEntity(i));
			if(NULL != effect)
			{
				// TODO, Yuri Coder, 2013/07/24. Think about CommandAction's batching.
				CommandStartStopParticleEffect* command = new CommandStartStopParticleEffect(ss->GetSelectionEntity(i), true);
				sceneEditor->Exec(command);
			}
		}
	}
}

void SceneTree::StopEmitter()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		SceneSelectionSystem *ss = sceneEditor->selectionSystem;
		for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
		{
			DAVA::ParticleEffectComponent *effect = DAVA::GetEffectComponent(ss->GetSelectionEntity(i));
			if(NULL != effect)
			{
				// TODO, Yuri Coder, 2013/07/24. Think about CommandAction's batching.
				CommandStartStopParticleEffect* command = new CommandStartStopParticleEffect(ss->GetSelectionEntity(i), false);
				sceneEditor->Exec(command);
			}
		}
	}
}

void SceneTree::RestartEmitter()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		SceneSelectionSystem *ss = sceneEditor->selectionSystem;
		for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
		{
			DAVA::ParticleEffectComponent *effect = DAVA::GetEffectComponent(ss->GetSelectionEntity(i));
			if(NULL != effect)
			{
				// TODO, Yuri Coder, 2013/07/24. Think about CommandAction's batching.
				CommandRestartParticleEffect* command = new CommandRestartParticleEffect(ss->GetSelectionEntity(i));
				sceneEditor->Exec(command);
			}
		}
	}
}

void SceneTree::AddLayer()
{
	SceneEditor2* sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		QModelIndex realIndex = filteringProxyModel->mapToSource(currentIndex());
		SceneTreeItem *curItem = treeModel->GetItem(realIndex);
		DAVA::ParticleEmitter* curEmitter= NULL;
		DAVA::Entity *curEntity = SceneTreeItemEntity::GetEntity(treeModel->GetItem(realIndex));
		if(NULL != curEntity)
		{
			curEmitter = DAVA::GetEmitter(curEntity);
		}
		else if (curItem->ItemType() == SceneTreeItem::EIT_InnerEmmiter) //special case for inner emmiter
		{
			curEmitter = ((SceneTreeItemParticleInnerEmmiter *)curItem)->emitter;
		}		
		if (curEmitter)
		{
			CommandAddParticleEmitterLayer* command = new CommandAddParticleEmitterLayer(curEmitter);
			sceneEditor->Exec(command);
			treeModel->ResyncStructure(treeModel->invisibleRootItem(), sceneEditor);
		}
	}
}

void SceneTree::LoadEmitterFromYaml()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(NULL != sceneEditor)
	{
		QString filePath = QFileDialog::getOpenFileName(NULL, QString("Open Particle Emitter Yaml file"),
														GetParticlesConfigPath(), QString("YAML File (*.yaml)"));
		if (filePath.isEmpty())
		{
			return;
		}

		bool validationOK = true;
		Set<String> validationErrors;
		
		SceneSelectionSystem *ss = sceneEditor->selectionSystem;
		for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
		{
			DAVA::ParticleEmitter* emitter = DAVA::GetEmitter(ss->GetSelectionEntity(i));
			if(NULL == emitter)
			{
				continue;
			}
			
			// TODO, Yuri Coder, 2013/07/24. Think about CommandAction's batching.
			CommandLoadParticleEmitterFromYaml* command =
				new CommandLoadParticleEmitterFromYaml(emitter, filePath.toStdString());
			sceneEditor->Exec(command);

			// Validate each Particle Emitter after load.
			if (!SceneValidator::Instance()->ValidateParticleEmitter(emitter, validationErrors))
			{
				validationOK = false;
			}
		}

		treeModel->ResyncStructure(treeModel->invisibleRootItem(), sceneEditor);
		
		if (!validationOK)
		{
			ShowErrorDialog(validationErrors);
		}
	}
}

void SceneTree::SaveEmitterToYaml()
{
	PerformSaveEmitter(false);
}

void SceneTree::SaveEmitterToYamlAs()
{
	PerformSaveEmitter(true);
}


void SceneTree::LoadInnerEmitterFromYaml()
{	
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(!sceneEditor) 
		return;
	if (!selectedInnerEmmiter) 
		return;
	if (!selectedInnerEmmiterParentLayer)
		return;
	
	QString filePath = QtFileDialog::getOpenFileName(NULL, QString("Open Particle Emitter Yaml file"),
		GetParticlesConfigPath(), QString("YAML File (*.yaml)"));
	if (filePath.isEmpty())
	{
		return;
	}		

	selectedInnerEmmiterParentLayer->innerEmitterPath = filePath.toStdString();
	Set<String> validationErrors;
	CommandLoadParticleEmitterFromYaml* command = new CommandLoadParticleEmitterFromYaml(selectedInnerEmmiter, filePath.toStdString());
	sceneEditor->Exec(command);			

	treeModel->ResyncStructure(treeModel->invisibleRootItem(), sceneEditor);

	if (!SceneValidator::Instance()->ValidateParticleEmitter(selectedInnerEmmiter, validationErrors))
	{
		ShowErrorDialog(validationErrors);
	}
	
}
void SceneTree::SaveInnerEmitterToYaml()
{
	PerformSaveInnerEmitter(false);
}
void SceneTree::SaveInnerEmitterToYamlAs()
{
	PerformSaveInnerEmitter(true);
}
void SceneTree::PerformSaveInnerEmitter(bool forceAskFileName)
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(!sceneEditor)	
		return;
	if (!selectedInnerEmmiter)
		return;	
	
	forceAskFileName|=selectedInnerEmmiter->GetConfigPath().IsEmpty();	

	FilePath yamlPath;
	if (forceAskFileName)
	{
		QString projectPath = QString(EditorSettings::Instance()->GetParticlesConfigsPath().GetAbsolutePathname().c_str());
		QString filePath = QtFileDialog::getSaveFileName(NULL, QString("Save Particle Emitter YAML file"),
			projectPath, QString("YAML File (*.yaml)"));

		if (filePath.isEmpty())
		{
			return;
		}

		yamlPath = FilePath(filePath.toStdString());
	}		

	FilePath curEmitterFilePath;
	if (forceAskFileName)
	{
		curEmitterFilePath = yamlPath;
	}
	else
	{
		curEmitterFilePath = selectedInnerEmmiter->GetConfigPath().IsEmpty() ? yamlPath : selectedInnerEmmiter->GetConfigPath();
	}

	selectedInnerEmmiterParentLayer->innerEmitterPath = curEmitterFilePath;
	CommandSaveParticleEmitterToYaml* command = new CommandSaveParticleEmitterToYaml(selectedInnerEmmiter, curEmitterFilePath);
	sceneEditor->Exec(command);	
}


void SceneTree::CloneLayer()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if (sceneEditor == NULL)
	{
		return;
	}

	if (!selectedLayer)
	{
		DVASSERT(false);
		return;
	}

	CommandCloneParticleEmitterLayer* command = new CommandCloneParticleEmitterLayer(selectedLayer);
	sceneEditor->Exec(command);

	treeModel->ResyncStructure(treeModel->invisibleRootItem(), sceneEditor);
}

void SceneTree::RemoveLayer()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if (sceneEditor == NULL)
	{
		return;
	}
	
	if (!selectedLayer)
	{
		DVASSERT(false);
		return;
	}
	
	CommandRemoveParticleEmitterLayer* command = new CommandRemoveParticleEmitterLayer(selectedLayer);
	sceneEditor->Exec(command);
	
	treeModel->ResyncStructure(treeModel->invisibleRootItem(), sceneEditor);
}

void SceneTree::AddForce()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if (sceneEditor == NULL)
	{
		return;
	}
	
	if (!selectedLayer)
	{
		DVASSERT(false);
		return;
	}
	
	CommandAddParticleEmitterForce* command = new CommandAddParticleEmitterForce(selectedLayer);
	sceneEditor->Exec(command);

	treeModel->ResyncStructure(treeModel->invisibleRootItem(), sceneEditor);
}

void SceneTree::RemoveForce()
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if (sceneEditor == NULL)
	{
		return;
	}

	if (!selectedLayer || !selectedForce)
	{
		DVASSERT(false);
		return;
	}

	CommandRemoveParticleEmitterForce* command = new CommandRemoveParticleEmitterForce(selectedLayer, selectedForce);
	sceneEditor->Exec(command);
	
	treeModel->ResyncStructure(treeModel->invisibleRootItem(), sceneEditor);
}

void SceneTree::PerformSaveEmitter(bool forceAskFileName)
{
	SceneEditor2 *sceneEditor = treeModel->GetScene();
	if(!sceneEditor)
	{
		return;
	}

	// Verify whether we have to ask about the file name. If at least one emitter
	// does not have emitter path - treat this as "force ask".
	EntityGroup selection = sceneEditor->selectionSystem->GetSelection();
	if (forceAskFileName == false)
	{
		for(size_t i = 0; i < selection.Size(); ++i)
		{
			DAVA::ParticleEmitter* emitter = DAVA::GetEmitter(selection.GetEntity(i));
			if (emitter && emitter->GetConfigPath().IsEmpty())
			{
				forceAskFileName = true;
				break;
			}
		}
	}

	FilePath yamlPath;
    if (forceAskFileName)
    {
        QString projectPath = QString(EditorSettings::Instance()->GetParticlesConfigsPath().GetAbsolutePathname().c_str());
        QString filePath = QtFileDialog::getSaveFileName(NULL, QString("Save Particle Emitter YAML file"),
                                                        projectPath, QString("YAML File (*.yaml)"));
		
        if (filePath.isEmpty())
        {
            return;
        }

        yamlPath = FilePath(filePath.toStdString());
	}
	
	// Re-save all the emitters using either YAML path just defined or emitter's
	// inner file path.
	for(size_t i = 0; i < selection.Size(); ++i)
	{
		DAVA::ParticleEmitter* emitter = DAVA::GetEmitter(selection.GetEntity(i));
		if (!emitter)
		{
			continue;
		}

		FilePath curEmitterFilePath;
		if (forceAskFileName)
		{
			curEmitterFilePath = yamlPath;
		}
		else
		{
			curEmitterFilePath = emitter->GetConfigPath().IsEmpty() ? yamlPath : emitter->GetConfigPath();
		}

		CommandSaveParticleEmitterToYaml* command = new CommandSaveParticleEmitterToYaml(emitter, curEmitterFilePath);
		sceneEditor->Exec(command);
	}
}

QString SceneTree::GetParticlesConfigPath()
{
	return QString(EditorSettings::Instance()->GetParticlesConfigsPath().GetAbsolutePathname().c_str());
}

void SceneTree::CleanupParticleEditorSelectedItems()
{
	this->selectedLayer = NULL;
	this->selectedForce = NULL;
	this->selectedInnerEmmiter = NULL;
	this->selectedInnerEmmiterParentLayer = NULL;
}

void SceneTree::OnRefreshTimeout()
{
	dataChanged(QModelIndex(), QModelIndex());
}

void SceneTree::SetEntityNameAsFilter()
{
	SceneEditor2 *scene = treeModel->GetScene();
	if(!scene) return;

	EntityGroup selection = scene->selectionSystem->GetSelection();
	if(selection.Size() != 1) return;

	Entity *entity = selection.GetEntity(0);
	QtMainWindow::Instance()->GetUI()->sceneTreeFilterEdit->setText(entity->GetName().c_str());
}
