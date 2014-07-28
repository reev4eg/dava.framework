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


#include "ActionComponentEditor.h"
#include "ui_ActionComponentEditor.h"
#include "../Qt/Main/QtUtils.h"

#include <QTableWidgetItem>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QCompleter>
#include <QLineEdit>
#include <QRegExpValidator>


namespace
{
    
    enum eColumns
    {
        COLUMN_EVENT_TYPE,
        COLUMN_EVENT_NAME,
        COLUMN_ACTION_TYPE,
        COLUMN_ENTITY_NAME,
        COLUMN_DELAY,
        COLUMN_DELAY_VARIATION,
        COLUMN_SWITCH_INDEX,
        COLUMN_STOPAFTERNREPEATS_INDEX,
        COLUMN_STOPWHENEMPTY_INDEX,
    };

    enum eYesNo
    {
        COMBO_YES_INDEX,
        COMBO_NO_INDEX,
    };

    QString ACTION_TYPE_NAME[] = 
    {
        "None",
        "Particle start",
        "Particle stop",
        "Sound",
    };
    const size_t ACTION_NAME_COUNT = sizeof( ACTION_TYPE_NAME ) / sizeof( *ACTION_TYPE_NAME );

    QString EVENT_TYPE_NAME[] =
    {
        "Switch",
        "Added",
        "User",
    };
    const size_t EVENT_NAME_COUNT = sizeof( EVENT_TYPE_NAME ) / sizeof( *EVENT_TYPE_NAME );
    
}


ActionComponentEditor::ActionComponentEditor(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ActionComponentEditor)
    , isModified(false)
{
    ui->setupUi(this);
	
	targetComponent = NULL;
	connect(ui->buttonAddItem, SIGNAL(clicked()), SLOT(OnAddAction()));
    connect(ui->buttonRemoveItem, SIGNAL(clicked()), SLOT(OnRemoveAction()));
	connect(ui->tableActions, SIGNAL(itemSelectionChanged()), SLOT(OnSelectedItemChanged()));
	
	ui->tableActions->resizeColumnsToContents();
	ui->buttonRemoveItem->setEnabled(false);
	
	editDelegate.setParent(ui->tableActions);
	editDelegate.SetComponentEditor(this);
	ui->tableActions->setItemDelegate(&editDelegate);

    actionTypes[DAVA::ActionComponent::Action::TYPE_PARTICLE_EFFECT_START] = "Particle start";
    actionTypes[DAVA::ActionComponent::Action::TYPE_PARTICLE_EFFECT_STOP] = "Particle stop";
    actionTypes[DAVA::ActionComponent::Action::TYPE_SOUND] = "Sound";

    eventTypes[DAVA::ActionComponent::Action::EVENT_SWITCH_CHANGED] = "Switch";
    eventTypes[DAVA::ActionComponent::Action::EVENT_ADDED_TO_SCENE] = "Added";
    eventTypes[DAVA::ActionComponent::Action::EVENT_CUSTOM] = "User";
}

ActionComponentEditor::~ActionComponentEditor()
{
    delete ui;
}

void ActionComponentEditor::SetComponent(DAVA::ActionComponent* component)
{
	editDelegate.SetComponent(component);
	targetComponent = component;
	UpdateTableFromComponent(targetComponent);
	
	ui->buttonAddItem->setEnabled(!IsActionPresent(GetDefaultAction()));
}

void ActionComponentEditor::UpdateTableFromComponent(DAVA::ActionComponent* component)
{
	ui->tableActions->clearContents();
	
	int actionCount = component->GetCount();
	ui->tableActions->setRowCount(actionCount);
	
	if(actionCount > 0)
	{
		for(int i = 0; i < actionCount; ++i)
		{
			DAVA::ActionComponent::Action& action = component->Get(i);
			
            QTableWidgetItem* eventTypeTableItem = new QTableWidgetItem( eventTypes[action.eventType], COLUMN_ACTION_TYPE );
            QTableWidgetItem* eventNameTableItem = new QTableWidgetItem( action.userEventId.c_str(), COLUMN_EVENT_NAME );
            QTableWidgetItem* actionTypeTableItem = new QTableWidgetItem( actionTypes[action.type], COLUMN_ACTION_TYPE );
            QTableWidgetItem* entityNameTableItem = new QTableWidgetItem( action.entityName.c_str(), COLUMN_ENTITY_NAME );
            QTableWidgetItem* delayTableItem = new QTableWidgetItem( QString( "%1" ).arg( action.delay, 16, 'f', 2 ), COLUMN_DELAY );
            QTableWidgetItem* delayVariationTableItem = new QTableWidgetItem( QString( "%1" ).arg( action.delayVariation, 16, 'f', 2 ), COLUMN_DELAY_VARIATION );
            QTableWidgetItem* switchIndexTableItem = new QTableWidgetItem( QString( "%1" ).arg( action.switchIndex ), COLUMN_SWITCH_INDEX );
            QTableWidgetItem* stopAfterNRepeatsTableItem = new QTableWidgetItem( QString( "%1" ).arg( action.stopAfterNRepeats ), COLUMN_STOPAFTERNREPEATS_INDEX );
            QTableWidgetItem* stopWhenEmptyTableItem = new QTableWidgetItem( ( action.stopWhenEmpty ) ? "Yes" : "No", COLUMN_STOPWHENEMPTY_INDEX );
			
			ui->tableActions->setItem(i, COLUMN_EVENT_TYPE, eventTypeTableItem);
            ui->tableActions->setItem( i, COLUMN_EVENT_NAME, eventNameTableItem );
			ui->tableActions->setItem(i, COLUMN_ACTION_TYPE, actionTypeTableItem);
			ui->tableActions->setItem(i, COLUMN_ENTITY_NAME, entityNameTableItem);
			ui->tableActions->setItem(i, COLUMN_DELAY, delayTableItem);
            ui->tableActions->setItem( i, COLUMN_DELAY_VARIATION, delayVariationTableItem );
			ui->tableActions->setItem(i, COLUMN_SWITCH_INDEX, switchIndexTableItem);
			ui->tableActions->setItem(i, COLUMN_STOPAFTERNREPEATS_INDEX, stopAfterNRepeatsTableItem);
			ui->tableActions->setItem(i, COLUMN_STOPWHENEMPTY_INDEX, stopWhenEmptyTableItem);
		}
		
		ui->tableActions->resizeColumnsToContents();
		ui->tableActions->resizeRowsToContents();
	}
}

void ActionComponentEditor::OnAddAction()
{
	//add action with default values
	DAVA::ActionComponent::Action action = GetDefaultAction();
	
	bool duplicateAction = IsActionPresent(action);	
	if(duplicateAction)
	{
		ShowErrorDialog("Duplicate actions not allowed!");
	}
	else
	{
		targetComponent->Add(action);
		UpdateTableFromComponent(targetComponent);
		
		ui->buttonAddItem->setEnabled(false);
        isModified = true;
	}
}

void ActionComponentEditor::OnRemoveAction()
{
	int currentRow = ui->tableActions->currentRow();
	if(currentRow >= 0 &&
	   currentRow < (int)targetComponent->GetCount())
	{
		targetComponent->Remove(targetComponent->Get(currentRow));
		UpdateTableFromComponent(targetComponent);
		
		bool itemsPresent = (targetComponent->GetCount() > 0);
		ui->buttonRemoveItem->setEnabled(itemsPresent);
		if(itemsPresent)
		{
			ui->tableActions->setCurrentCell(0, 0);
		}
		
		ui->buttonAddItem->setEnabled(!IsActionPresent(GetDefaultAction()));
        isModified = true;
	}
}

void ActionComponentEditor::OnSelectedItemChanged()
{
	int currentRow = ui->tableActions->currentRow();
	ui->buttonRemoveItem->setEnabled(currentRow >= 0);
}

QWidget* ActionItemEditDelegate::createFloatEditor(QWidget *parent) const
{
    QLineEdit *sb = new QLineEdit(parent);
    sb->setValidator(new QRegExpValidator(QRegExp("\\s*-?\\d*[,\\.]?\\d*\\s*")));

    return sb;
}

DAVA::ActionComponent::Action ActionComponentEditor::GetDefaultAction()
{
	DAVA::ActionComponent::Action action;
	action.eventType = DAVA::ActionComponent::Action::EVENT_SWITCH_CHANGED;
	action.type = DAVA::ActionComponent::Action::TYPE_PARTICLE_EFFECT_START;	
	action.entityName = DAVA::ActionComponent::ACTION_COMPONENT_SELF_ENTITY_NAME;
	action.delay = 0.0f;
	action.switchIndex = -1;

	return action;
}

bool ActionComponentEditor::IsActionPresent(const DAVA::ActionComponent::Action action)
{
	bool actionPresent = false;
	for(DAVA::uint32 i = 0; i < targetComponent->GetCount(); ++i)
	{
		const DAVA::ActionComponent::Action& innerAction = targetComponent->Get(i);
		if((innerAction.type == action.type) &&  //different type
			(innerAction.eventType == action.eventType) && //different event
			((action.eventType!=DAVA::ActionComponent::Action::EVENT_SWITCH_CHANGED) || (innerAction.switchIndex == action.switchIndex)) && //different switch for EVENT_SWITCH_CHNGED only
			(innerAction.entityName == action.entityName)) //different entity
		{
			actionPresent = true;
			break;
		}
	}

	return actionPresent;
}

void ActionComponentEditor::Update()
{
	ui->tableActions->resizeColumnsToContents();
	ui->tableActions->resizeRowsToContents();

	ui->buttonAddItem->setEnabled(!IsActionPresent(GetDefaultAction()));
    isModified = true;
}

bool ActionComponentEditor::IsModified() const
{
    return isModified;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

ActionItemEditDelegate::ActionItemEditDelegate(QObject *parent)
    : QStyledItemDelegate( parent )
    , targetComponent(NULL)
    , componentEditor(NULL)
{
    actionTypes["Particle start"] = DAVA::ActionComponent::Action::TYPE_PARTICLE_EFFECT_START;
    actionTypes["Particle stop"] = DAVA::ActionComponent::Action::TYPE_PARTICLE_EFFECT_STOP;
    actionTypes["Sound"] = DAVA::ActionComponent::Action::TYPE_SOUND;

    eventTypes["Switch"] = DAVA::ActionComponent::Action::EVENT_SWITCH_CHANGED;
    eventTypes["Added"] = DAVA::ActionComponent::Action::EVENT_ADDED_TO_SCENE;
    eventTypes["User"] = DAVA::ActionComponent::Action::EVENT_CUSTOM;
}

QWidget* ActionItemEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
					  const QModelIndex &index) const
{	
	QWidget* editor = NULL;

	switch(index.column())
	{
		case COLUMN_EVENT_TYPE:
		{
            QComboBox* combo = new QComboBox( parent );
            combo->setFrame( false );
            for ( auto it = eventTypes.begin(); it != eventTypes.end(); it++ )
            {
                combo->addItem( it.key(), it.value() );
            }

			editor = combo;

			break;
		}

        case COLUMN_EVENT_NAME:
        {
            editor = new QLineEdit( parent );
            break;
        }

		case COLUMN_ACTION_TYPE:
		{
			QComboBox* combo = new QComboBox(parent);
			combo->setFrame(false);

            for ( auto it = actionTypes.begin(); it != actionTypes.end(); it++ )
            {
                combo->addItem( it.key(), it.value() );
            }

			editor = combo;
			
			break;
		}
			
		case COLUMN_ENTITY_NAME:
		{
			DAVA::Entity* parentEntity = targetComponent->GetEntity();
			DAVA::Vector<DAVA::Entity*> allChildren;
			parentEntity->GetChildNodes(allChildren);

			DAVA::Vector<DAVA::String> childrenNames;
            childrenNames.reserve(allChildren.size() + 1);
            
			childrenNames.push_back(DAVA::ActionComponent::ACTION_COMPONENT_SELF_ENTITY_NAME.c_str());

			for(int i = 0; i < (int)allChildren.size(); ++i)
			{
				childrenNames.push_back(allChildren[i]->GetName().c_str());
			}
			
			std::sort(childrenNames.begin(), childrenNames.end());
			childrenNames.erase(std::unique(childrenNames.begin(), childrenNames.end()), childrenNames.end());
			
			QComboBox* combo = new QComboBox(parent);
			combo->setFrame(false);
			for(int i = 0; i < (int)childrenNames.size(); ++i)
			{
				combo->addItem(childrenNames[i].c_str());
			}
			
			editor = combo;
			
			break;
		}
			
		case COLUMN_DELAY:
        case COLUMN_DELAY_VARIATION:
		{
			editor = createFloatEditor(parent);
			break;
		}
			
		case COLUMN_SWITCH_INDEX:
		{
			QSpinBox* spinBox = new QSpinBox(parent);
			spinBox->setMinimum(-1);
			spinBox->setMaximum(128);
			spinBox->setSingleStep(1);
			
			editor = spinBox;

			break;
		}
			
		case COLUMN_STOPAFTERNREPEATS_INDEX:
		{
			QSpinBox* spinBox = new QSpinBox(parent);
			spinBox->setMinimum(-1);
			spinBox->setMaximum(100000);
			spinBox->setSingleStep(1);
			
			editor = spinBox;
			
			break;
		}

		case COLUMN_STOPWHENEMPTY_INDEX:
		{
			QComboBox* combo = new QComboBox(parent);
			combo->setFrame(false);
            combo->addItem( "Yes", COMBO_YES_INDEX );
            combo->addItem( "No", COMBO_NO_INDEX );
			
			editor = combo;
			break;
		}
	}

	return editor;
}

void ActionItemEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	const DAVA::ActionComponent::Action& currentAction = targetComponent->Get(index.row());

	switch(index.column())
	{
		case COLUMN_EVENT_TYPE:
		{
            QComboBox* combo = static_cast<QComboBox*>( editor );
            const int n = combo->count();
            for ( int i = 0; i < n; i++ )
            {
                if ( combo->itemData( i ).toInt() == (int)currentAction.eventType )
                {
                    combo->setCurrentIndex( i );
                    break;
                }
            }

			break;
		}

        case COLUMN_EVENT_NAME:
        {
            QLineEdit* edit = static_cast<QLineEdit*>( editor );
            edit->setText( currentAction.userEventId.c_str() );
            break;
        }

        case COLUMN_ACTION_TYPE:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
            const int n = combo->count();
            for ( int i = 0; i < n; i++ )
            {
                if ( combo->itemData( i ).toInt() == (int)currentAction.type )
                {
                    combo->setCurrentIndex( i );
                    break;
                }
            }
			
			break;
		}
			
		case COLUMN_ENTITY_NAME:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
			for(int i = 0; i < combo->count(); ++i)
			{
				if(combo->itemText(i) == currentAction.entityName.c_str())
				{
					combo->setCurrentIndex(i);
					break;
				}
			}
			
			break;
		}
			
		case COLUMN_DELAY:
		{
            QLineEdit *edit = static_cast<QLineEdit*>(editor);
			edit->setText(QString::number(currentAction.delay));
			break;
		}

        case COLUMN_DELAY_VARIATION:
        {
            QLineEdit *edit = static_cast<QLineEdit*>(editor);
			edit->setText(QString::number(currentAction.delayVariation));
            break;
        }

		case COLUMN_SWITCH_INDEX:
		{
			QSpinBox* spinBox = static_cast<QSpinBox*>(editor);
			spinBox->setValue(currentAction.switchIndex);
			
			break;
		}
			
		case COLUMN_STOPAFTERNREPEATS_INDEX:
		{
			QSpinBox* spinBox = static_cast<QSpinBox*>(editor);
			spinBox->setValue(currentAction.stopAfterNRepeats);
			
			break;
		}
			
		case COLUMN_STOPWHENEMPTY_INDEX:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
			combo->setCurrentIndex(currentAction.stopWhenEmpty ? COMBO_YES_INDEX : COMBO_NO_INDEX);
			break;
		}

	}

}

void ActionItemEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
				  const QModelIndex &index) const
{
	DAVA::ActionComponent::Action& currentAction = targetComponent->Get(index.row());
	
	switch(index.column())
	{
		case COLUMN_EVENT_TYPE:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
            currentAction.eventType = ( DAVA::ActionComponent::Action::eEvent )combo->itemData( combo->currentIndex() ).toUInt();
			model->setData(index, combo->currentText(), Qt::EditRole);
			break;
		}

        case COLUMN_EVENT_NAME:
        {
            QLineEdit* edit = static_cast<QLineEdit*>( editor );
            currentAction.userEventId = DAVA::FastName( edit->text().toStdString().c_str() );
            model->setData( index, edit->text(), Qt::EditRole );
            break;
        }

		case COLUMN_ACTION_TYPE:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
            DAVA::ActionComponent::Action::eType t = (DAVA::ActionComponent::Action::eType)combo->itemData( combo->currentIndex() ).toUInt();
            currentAction.type = t;
			model->setData(index, combo->currentText(), Qt::EditRole);
			
			break;
		}
			
		case COLUMN_ENTITY_NAME:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
			currentAction.entityName = DAVA::FastName(combo->currentText().toStdString());
			model->setData(index, combo->currentText(), Qt::EditRole);
			
			break;
		}
			
		case COLUMN_DELAY:
		{
            QLineEdit *edit = static_cast<QLineEdit*>(editor);
            currentAction.delay = edit->text().toDouble();
			model->setData(index, QString("%1").arg(currentAction.delay, 16, 'f', 2), Qt::EditRole);
			break;
		}
			
        case COLUMN_DELAY_VARIATION:
        {
            QLineEdit *edit = static_cast<QLineEdit*>(editor);
            currentAction.delayVariation = edit->text().toDouble();
			model->setData(index, QString("%1").arg(currentAction.delayVariation, 16, 'f', 2), Qt::EditRole);
            break;
        }

        case COLUMN_SWITCH_INDEX:
		{
			QSpinBox* spinBox = static_cast<QSpinBox*>(editor);
			currentAction.switchIndex = spinBox->value();
			model->setData(index, QString("%1").arg(currentAction.switchIndex), Qt::EditRole);
			
			break;
		}
			
		case COLUMN_STOPAFTERNREPEATS_INDEX:
		{
			QSpinBox* spinBox = static_cast<QSpinBox*>(editor);
			currentAction.stopAfterNRepeats = spinBox->value();
			model->setData(index, QString("%1").arg(currentAction.stopAfterNRepeats), Qt::EditRole);
			
			break;
		}
			
		case COLUMN_STOPWHENEMPTY_INDEX:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
			currentAction.stopWhenEmpty = (combo->currentIndex() == COMBO_YES_INDEX);
			model->setData(index, (currentAction.stopWhenEmpty) ? "Yes" : "No", Qt::EditRole);
			
			break;
		}

	}
	
	componentEditor->Update();
}

void ActionItemEditDelegate::updateEditorGeometry(QWidget *editor,
						  const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	editor->setGeometry(option.rect);
}

void ActionItemEditDelegate::SetComponent(DAVA::ActionComponent* component)
{
	targetComponent = component;
}

void ActionItemEditDelegate::SetComponentEditor(ActionComponentEditor* editor)
{
	componentEditor = editor;
}

