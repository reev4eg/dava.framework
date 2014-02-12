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

#include "MaterialTree.h"
#include "MaterialFilterModel.h"
#include "Main/mainwindow.h"
#include "Scene/SceneSignals.h"
#include "MaterialEditor/MaterialAssignSystem.h"

#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QHeaderView>


MaterialTree::MaterialTree(QWidget *parent /* = 0 */)
: QTreeView(parent)
{ 
	treeModel = new MaterialFilteringModel(new MaterialModel());
	setModel(treeModel);
	setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionBehavior(QAbstractItemView::SelectRows);

	QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));

	QObject::connect(SceneSignals::Instance(), SIGNAL(CommandExecuted(SceneEditor2*, const Command2*, bool)), this, SLOT(OnCommandExecuted(SceneEditor2*, const Command2*, bool)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(StructureChanged(SceneEditor2 *, DAVA::Entity *)), this, SLOT(OnStructureChanged(SceneEditor2 *, DAVA::Entity *)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(SelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)), this, SLOT(OnSelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)));

    header()->setSortIndicator( 0, Qt::AscendingOrder );
    header()->setStretchLastSection(false);
    header()->setResizeMode(0, QHeaderView::Stretch);
    header()->resizeSection(1, 25);
    header()->resizeSection(2, 25);
}

MaterialTree::~MaterialTree()
{}

void MaterialTree::SetScene(SceneEditor2 *sceneEditor)
{
	treeModel->SetScene(sceneEditor);

	if(NULL != sceneEditor)
	{
		EntityGroup curSelection = sceneEditor->selectionSystem->GetSelection();
        OnSelectionChanged( sceneEditor, &curSelection, NULL );
		//treeModel->SetSelection(&curSelection);
	}
	else
	{
		treeModel->SetSelection(NULL);
	}

    sortByColumn(0);
}

void MaterialTree::AssignMaterialToSelection( DAVA::NMaterial *material )
{
    SceneEditor2 *curScene = treeModel->GetScene();
    Q_ASSERT( curScene );
    if ( !curScene )
        return ;
    EntityGroup selection = curScene->selectionSystem->GetSelection();
    MaterialAssignSystem::AssignMaterialToGroup(curScene, &selection, material);
}

DAVA::NMaterial* MaterialTree::GetMaterial(const QModelIndex &index) const
{
	return treeModel->GetMaterial(index);
}

void MaterialTree::SelectMaterial(DAVA::NMaterial *material)
{
	selectionModel()->clear();

	QModelIndex index = treeModel->GetIndex(material);
	if(index.isValid())
	{
		selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
		scrollTo(index, QAbstractItemView::PositionAtCenter);
	}
}

void MaterialTree::SelectEntities(DAVA::NMaterial *material)
{
	SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
	if(NULL != curScene && NULL != material)
	{
		curScene->selectionSystem->Clear();

		if(material->GetMaterialType() == NMaterial::MATERIALTYPE_INSTANCE)
		{
			DAVA::Entity *entity = curScene->materialSystem->GetEntity(material);
			curScene->selectionSystem->AddSelection(curScene->selectionSystem->GetSelectableEntity(entity));
		}
		else
		{
			DAVA::Set<DAVA::NMaterial *> instances;
			curScene->materialSystem->BuildInstancesList(material, instances);

			auto it = instances.begin();
			auto end = instances.end();

			for(; it != end; ++it)
			{
				DAVA::Entity *entity = curScene->materialSystem->GetEntity(*it);
				curScene->selectionSystem->AddSelection(curScene->selectionSystem->GetSelectableEntity(entity));
			}
		}

        QtMainWindow::Instance()->GetUI()->sceneTree->LookAtSelection();
	}
}

void MaterialTree::Update()
{
	treeModel->Sync();
    treeModel->invalidate();
    emit Updated();
}

int MaterialTree::getFilterType() const
{
    return treeModel->getFilterType();
}

void MaterialTree::setFilterType(int filterType)
{
    treeModel->setFilterType( filterType );
}

void MaterialTree::ShowContextMenu(const QPoint &pos)
{ 
	QMenu contextMenu(this);

	contextMenu.addAction(QIcon(":/QtIcons/zoom.png"), "Select entities", this, SLOT(OnSelectEntities()));

/*
	// add/remove
	contextMenu.addSeparator();
	contextMenu.addAction(QIcon(":/QtIcons/remove.png"), "Remove entity", this, SLOT(RemoveSelection()));

	// lock/unlock
	contextMenu.addSeparator();

	// save model as
	contextMenu.addSeparator();
	contextMenu.addAction(QIcon(":/QtIcons/save_as.png"), "Save Entity As...", this, SLOT(SaveEntityAs()));
*/

    // "Assign to Selection" item
    {
        const QModelIndexList& selection = selectionModel()->selectedIndexes();
        int nMaterials = 0;
        int nInstances = 0;

        foreach( const QModelIndex& index, selection )
        {
            DAVA::NMaterial *material = treeModel->GetMaterial(index);
            if ( material )
            {
                switch ( material->GetMaterialType() )
                {
                case DAVA::NMaterial::MATERIALTYPE_MATERIAL:
                    nMaterials++;
                    break;
                case DAVA::NMaterial::MATERIALTYPE_INSTANCE:
                    nInstances++;
                    break;
                default:
                    break;
                }
            }
            if ( nMaterials > 0 && nInstances > 0 )
                break;
        }

        const bool isVisible = nMaterials > 0;
        const bool isEnabled = ( nMaterials == 1 ) && ( nInstances == 0 );
        
        if ( isVisible )
        {
            const QModelIndex first = selection[0];
            DAVA::NMaterial *material = treeModel->GetMaterial(first);
            QVariant materialAsVariant = QVariant::fromValue<DAVA::NMaterial *>(material);
            QAction * actionAssign = contextMenu.addAction("Assign to Selection", this, SLOT(OnAssignToSelection()));

            if ( isEnabled )
                actionAssign->setData(materialAsVariant);
            actionAssign->setEnabled( isEnabled );
        }
    }

	contextMenu.exec(mapToGlobal(pos));
}

void MaterialTree::OnAssignToSelection()
{
    QAction *act = qobject_cast<QAction *>( sender() );
    if ( !act )
        return ;
    QVariant indexAsVariant = act->data();
    if ( !indexAsVariant.isValid() )
        return ;
    DAVA::NMaterial *material = indexAsVariant.value<DAVA::NMaterial *>();
    if ( !material )
        return ;
    AssignMaterialToSelection( material );
}

void MaterialTree::dragEnterEvent(QDragEnterEvent * event)
{
	QTreeView::dragEnterEvent(event);
	dragTryAccepted(event);
}

void MaterialTree::dragMoveEvent(QDragMoveEvent * event)
{
	QTreeView::dragMoveEvent(event);
	dragTryAccepted(event);
}

void MaterialTree::dropEvent(QDropEvent * event)
{
	QTreeView::dropEvent(event);

	event->setDropAction(Qt::IgnoreAction);
	event->accept();
}

void MaterialTree::dragTryAccepted(QDragMoveEvent *event)
{
	int row, col;
	QModelIndex parent;

	GetDropParams(event->pos(), parent, row, col);
	if(treeModel->dropCanBeAccepted(event->mimeData(), event->dropAction(), row, col, parent))
	{
		event->setDropAction(Qt::MoveAction);
		event->accept();
        treeModel->invalidate();
	}
	else
	{
		event->setDropAction(Qt::IgnoreAction);
		event->accept();
	}
}

void MaterialTree::GetDropParams(const QPoint &pos, QModelIndex &index, int &row, int &col)
{
	row = -1;
	col = -1;
	index = indexAt(pos);

	switch(dropIndicatorPosition())
	{
	case QAbstractItemView::OnItem:
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
	case QAbstractItemView::OnViewport:
        index = QModelIndex();
		break;
	}
}

void MaterialTree::OnCommandExecuted(SceneEditor2 *scene, const Command2 *command, bool redo)
{
	if(QtMainWindow::Instance()->GetCurrentScene() == scene)
	{
		int commandID = command->GetId();
		if(		(commandID == CMDID_DELETE_RENDER_BATCH) 
			||	(commandID == CMDID_CONVERT_TO_SHADOW) 
			||	(commandID == CMDID_MATERIAL_SWITCH_PARENT))
		{
			Update();
		}
	}
}

void MaterialTree::OnStructureChanged(SceneEditor2 *scene, DAVA::Entity *parent)
{
	treeModel->Sync();
}

void MaterialTree::OnSelectionChanged(SceneEditor2 *scene, const EntityGroup *selected, const EntityGroup *deselected)
{
	if(QtMainWindow::Instance()->GetCurrentScene() == scene)
	{
		treeModel->SetSelection(selected);
		treeModel->invalidate();
	}
}

void MaterialTree::OnSelectEntities()
{
	DAVA::NMaterial *currentMaterial = treeModel->GetMaterial(currentIndex());
	SelectEntities(currentMaterial);
}
