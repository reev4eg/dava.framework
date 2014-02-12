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



#ifndef __QT_PROPERTY_ITEM_H__
#define __QT_PROPERTY_ITEM_H__

/*
#include <QStandardItem>
#include "QtPropertyData.h"

Q_DECLARE_METATYPE(QtPropertyData *);

class QtPropertyItem : public QObject, public QStandardItem
{
	Q_OBJECT;

public:
	enum PropertyItemDataRole
	{
		PropertyDataRole = Qt::UserRole,
	};

	QtPropertyItem();
	QtPropertyItem(QtPropertyData* data, QtPropertyItem *name);
	QtPropertyItem(const QVariant &value);
	~QtPropertyItem();

	QtPropertyData* GetPropertyData() const;
	QtPropertyItem* GetParentNameItem() const;

	int	type() const;
	QVariant data(int role) const;
	void setData(const QVariant & value, int role);

	bool Update();
	
protected:
	QtPropertyData* itemData;
	QtPropertyItem *parentName;

	bool itemDataDeleteByParent;

	void ChildAdd(const QString &key, QtPropertyData* data);
	void ChildRemove(QtPropertyData* data);

	void ApplyDataFlags();
	void ApplyNameStyle();

protected slots:
	void DataChildAdded(const QString &key, QtPropertyData *data);
	void DataChildRemoving(const QString &key, QtPropertyData *data);
	void DataValueChanged(QtPropertyData::ValueChangeReason reason);
	void DataFlagsChanged();
};

class QtPropertyItem : public QObject, public QStandardItem
{

};
*/

#endif // __QT_PROPERTY_ITEM_H__
