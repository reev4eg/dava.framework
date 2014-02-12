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



#include <QtGui>

#include "QtPropertyItemDelegate.h"
#include "QtPropertyModel.h"
#include "QtPropertyData.h"
#include "QtPropertyWidgets/QtColorLineEdit.h"

QtPropertyItemDelegate::QtPropertyItemDelegate(QtPropertyModel *_model, QWidget *parent /* = 0 */)
	: QStyledItemDelegate(parent)
	, model(_model)
	, lastHoverData(NULL)
{ }

QtPropertyItemDelegate::~QtPropertyItemDelegate()
{ }

void QtPropertyItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionViewItemV4 opt = option;
	initStyleOption(&opt, index);

	// data
	if(index.column() == 1)
	{
		opt.textElideMode = Qt::ElideLeft;
		drawOptionalButtons(painter, opt, index, NORMAL);
	}

	QStyledItemDelegate::paint(painter, opt, index);

	if(index.column() == 1)
	{
		drawOptionalButtons(painter, opt, index, OVERLAYED);
	}
}

QSize QtPropertyItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize s = QStyledItemDelegate::sizeHint(option, index);
    return QSize(s.width(), s.height() + 5);
}

QWidget* QtPropertyItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QWidget* editWidget = NULL;

	if(model == index.model())
	{
		QtPropertyData* data = model->itemFromIndex(index);

		if(NULL != data)
		{
			editWidget = data->CreateEditor(parent, option);
		}

        // if widget wasn't created and it isn't checkable
        // let base class create editor
		if(NULL == editWidget && !data->IsCheckable())
		{
			editWidget = QStyledItemDelegate::createEditor(parent, option, index);
		}
	}

    return editWidget;
}

void QtPropertyItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	bool doneByInternalEditor = false;

	if(model == index.model())
	{
		QtPropertyData* data = model->itemFromIndex(index);
		if(NULL != data)
		{
            doneByInternalEditor = data->SetEditorData(editor);
		}
	}

	if(!doneByInternalEditor)
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

bool QtPropertyItemDelegate::editorEvent(QEvent * event, QAbstractItemModel * _model, const QStyleOptionViewItem & option, const QModelIndex & index)
{
	if(event->type() == QEvent::MouseMove)
	{
		QtPropertyData* data = model->itemFromIndex(index);
		if(data != lastHoverData)
		{
			showButtons(data);
		}
	}

	return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void QtPropertyItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *_model, const QModelIndex &index) const
{
	bool doneByInternalEditor = false;

	if(model == _model)
	{
		QtPropertyData* data = model->itemFromIndex(index);
		if(NULL != data)
		{
			doneByInternalEditor = data->EditorDone(editor);
		}
	}

	if(!doneByInternalEditor)
	{
	    QStyledItemDelegate::setModelData(editor, _model, index);
	}
}

void QtPropertyItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QStyledItemDelegate::updateEditorGeometry(editor, option, index);

	// tune widget border and geometry
	if(NULL != editor)
	{
		editor->setObjectName("customPropertyEditor");
		editor->setStyleSheet("#customPropertyEditor{ border: 1px solid gray; }");
		QRect r = option.rect;

		if(!index.model()->data(index, Qt::DecorationRole).isNull())
		{
			r.adjust(17, 0, 0, 0);
		}

		editor->setGeometry(r);
	}
}

bool QtPropertyItemDelegate::helpEvent(QHelpEvent * event, QAbstractItemView * view, const QStyleOptionViewItem & option, const QModelIndex & index)
{
	bool showToolTip = true;

	if(NULL != event && NULL != view && event->type() == QEvent::ToolTip)
	{
		QRect rect = view->visualRect(index);
		QSize size = sizeHint(option, index);
		if(rect.width() >= size.width()) 
		{
			showToolTip = false;
		}
	}

	if(showToolTip)
	{
		return QStyledItemDelegate::helpEvent(event, view, option, index);
	}
	else
	{
		QToolTip::hideText();
	}

	return false;
}

void QtPropertyItemDelegate::drawOptionalButtons(QPainter *painter, QStyleOptionViewItem &opt, const QModelIndex &index, OptionalButtonsType type) const
{
	QtPropertyData* data = model->itemFromIndex(index);
	if(index.column() == 1 && NULL != data && data->GetButtonsCount() > 0)
	{
		int owSpacing = 1;
		int owXPos = opt.rect.right() - owSpacing;
		int owYPos;

		// draw not overlaid widgets
		for(int i = data->GetButtonsCount() - 1; i >= 0; --i)
		{
			QtPropertyToolButton *btn = data->GetButton(i);
			if((type == NORMAL && !btn->overlayed) || (type == OVERLAYED && btn->overlayed))
			{
				// update widget height
				if(btn->height() != opt.rect.height())
				{
					QRect geom = btn->geometry();
					geom.setHeight(opt.rect.height());
					btn->setGeometry(geom);
				}

				owXPos -= btn->width();
				owYPos = opt.rect.y() + (opt.rect.height() - btn->height()) / 2;

				if(btn->isVisible())
				{
					btn->move(owXPos, owYPos);
				}
				else
				{
					QPixmap pix = QPixmap::grabWidget(btn);
					painter->drawPixmap(owXPos, owYPos, pix);
				}

				owXPos -= owSpacing;
			}
		}

		if(type == NORMAL)
		{
			opt.rect.setRight(owXPos);
		}
	}
}

void QtPropertyItemDelegate::showButtons(QtPropertyData *data)
{
	showOptionalButtons(lastHoverData, false);
	showOptionalButtons(data, true);

	lastHoverData = data;
}

void QtPropertyItemDelegate::showOptionalButtons(QtPropertyData *data, bool show)
{
	if(NULL != data)
	{
		for(int i = 0; i < data->GetButtonsCount(); ++i)
		{
			if(show)
			{
				data->GetButton(i)->show();
			}
			else
			{
				data->GetButton(i)->hide();
			}
		}
	}
}

void QtPropertyItemDelegate::invalidateButtons()
{
	if(NULL != lastHoverData)
	{
		showOptionalButtons(lastHoverData, false);
		lastHoverData = NULL;
	}
}
