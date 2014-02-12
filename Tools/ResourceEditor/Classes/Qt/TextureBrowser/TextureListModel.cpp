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



#include "TextureListModel.h"
#include <QPainter>
#include <QFileInfo>

TextureListModel::TextureListModel(QObject *parent /* = 0 */) 
	: QAbstractListModel(parent)
	, curSortMode(TextureListModel::SortByName)
	, curFilterBySelectedNode(false)
    , activeScene(NULL)
{}

TextureListModel::~TextureListModel()
{
	clear();
}

int TextureListModel::rowCount(const QModelIndex & /* parent */) const
{
	return textureDescriptorsFiltredSorted.size();
}

QVariant TextureListModel::data(const QModelIndex &index, int role) const
{
	if(index.isValid())
	{
		const DAVA::TextureDescriptor *curTextureDescriptor = textureDescriptorsFiltredSorted[index.row()];

		switch(role)
		{
		case Qt::DisplayRole:
			return QVariant(QFileInfo(curTextureDescriptor->GetSourceTexturePathname().GetAbsolutePathname().c_str()).fileName());
			break;

		default:
			break;
		}
	}

	return QVariant();
}

DAVA::Texture* TextureListModel::getTexture(const QModelIndex &index) const
{
	DAVA::Texture *ret = NULL;
	DAVA::TextureDescriptor *desc = getDescriptor(index);

	if(index.isValid() && texturesAll.contains(desc))
	{
		ret = texturesAll[desc];
	}

	return ret;
}

DAVA::Texture* TextureListModel::getTexture(const DAVA::TextureDescriptor* descriptor) const
{
	DAVA::Texture *ret = NULL;

	if(texturesAll.contains(descriptor))
	{
		ret = texturesAll[descriptor];
	}

	return ret;
}

DAVA::TextureDescriptor* TextureListModel::getDescriptor(const QModelIndex &index) const
{
	DAVA::TextureDescriptor *ret = NULL;

	if(index.isValid() && textureDescriptorsFiltredSorted.size() > index.row())
	{
		ret = textureDescriptorsFiltredSorted[index.row()];
	}

	return ret;
}

void TextureListModel::setTexture(const DAVA::TextureDescriptor* descriptor, DAVA::Texture *texture)
{
	if(texturesAll.contains(descriptor))
	{
		texturesAll[descriptor] = texture;
	}
}

bool TextureListModel::isHighlited(const QModelIndex &index) const
{
	bool ret = false;
	DAVA::TextureDescriptor *descriptor = getDescriptor(index);

	if(NULL != descriptor)
	{
		ret = textureDescriptorsHighlight.contains(descriptor);
	}

	return ret;
}

void TextureListModel::dataReady(const DAVA::TextureDescriptor *desc)
{
	int i = textureDescriptorsFiltredSorted.indexOf((DAVA::TextureDescriptor * const) desc);
	emit dataChanged(this->index(i), this->index(i));
}

void TextureListModel::setFilter(QString filter)
{
	beginResetModel();
	curFilter = filter;
	applyFilterAndSort();
	endResetModel();
}

void TextureListModel::setFilterBySelectedNode(bool enabled)
{
	beginResetModel();
	curFilterBySelectedNode = enabled;
	applyFilterAndSort();
	endResetModel();
}

void TextureListModel::setSortMode(TextureListModel::TextureListSortMode sortMode)
{
	beginResetModel();
	curSortMode = sortMode;
	applyFilterAndSort();
	endResetModel();
}

void TextureListModel::setScene(DAVA::Scene *scene)
{
	beginResetModel();

	clear();

    activeScene = scene;
    
	DAVA::TexturesMap texturesInNode;
	SceneHelper::EnumerateSceneTextures(scene, texturesInNode);

	for(DAVA::TexturesMap::iterator t = texturesInNode.begin(); t != texturesInNode.end(); ++t)
	{
		const DAVA::FilePath descPath = t->first;

		// if there is no the same descriptor and this file exists
		if(DAVA::FileSystem::Instance()->IsFile(descPath))
		{
			DAVA::TextureDescriptor * descriptor = DAVA::TextureDescriptor::CreateFromFile(descPath);

			if(NULL != descriptor)
			{
				textureDescriptorsAll.push_back(descriptor);
				texturesAll[descriptor] = t->second;
			}
		}
	}

	applyFilterAndSort();

	endResetModel();
}

void TextureListModel::setHighlight(const EntityGroup *nodes)
{
	beginResetModel();

	textureDescriptorsHighlight.clear();

	if(NULL != nodes)
	{
        DAVA::TexturesMap texturesInGroup;
		for(int i = 0; i < (int)nodes->Size(); ++i)
		{
			SceneHelper::EnumerateEntityTextures(activeScene, nodes->GetEntity(i), texturesInGroup);
		}
        
        for(DAVA::TexturesMap::iterator t = texturesInGroup.begin(); t != texturesInGroup.end(); ++t)
        {
            const DAVA::FilePath descPath = t->first;
            for(int i = 0; i < textureDescriptorsAll.size(); ++i)
            {
                if(textureDescriptorsAll[i]->pathname == descPath)
                {
                    textureDescriptorsHighlight.push_back(textureDescriptorsAll[i]);
                }
            }
        }
	}

	if(curFilterBySelectedNode)
	{
		applyFilterAndSort();
	}

	endResetModel();
}

void TextureListModel::clear()
{
    activeScene = NULL;
    
	texturesAll.clear();
	textureDescriptorsHighlight.clear();
	textureDescriptorsFiltredSorted.clear();

	for(int i = 0; i < textureDescriptorsAll.size(); ++i)
	{
		DAVA::SafeDelete(textureDescriptorsAll[i]);
	}

	textureDescriptorsAll.clear();
}

void TextureListModel::applyFilterAndSort()
{
	textureDescriptorsFiltredSorted.clear();

	for(int i = 0; i < (int) textureDescriptorsAll.size(); ++i)
	{
		if( (curFilter.isEmpty() || DAVA::String::npos != textureDescriptorsAll[i]->pathname.GetAbsolutePathname().find(curFilter.toStdString())) &&	// text filter
			(!curFilterBySelectedNode || textureDescriptorsHighlight.contains(textureDescriptorsAll[i])))						// cur selected node filter
		{
			textureDescriptorsFiltredSorted.push_back(textureDescriptorsAll[i]);
		}
	}

	switch(curSortMode)
	{
	case SortByName:
		{
			SortFnByName fn;
			std::sort(textureDescriptorsFiltredSorted.begin(), textureDescriptorsFiltredSorted.end(), fn);
		}
		break;
	case SortByFileSize:
		{
			SortFnByFileSize fn;
			std::sort(textureDescriptorsFiltredSorted.begin(), textureDescriptorsFiltredSorted.end(), fn);
		}
		break;
	case  SortByImageSize:
		{
			SortFnByImageSize fn(this);
			std::sort(textureDescriptorsFiltredSorted.begin(), textureDescriptorsFiltredSorted.end(), fn);
		}
		break;
	case  SortByDataSize:
		{
			SortFnByDataSize fn(this);
			std::sort(textureDescriptorsFiltredSorted.begin(), textureDescriptorsFiltredSorted.end(), fn);
		}
		break;
	default:
		break;
	}
}

bool SortFnByName::operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2)
{
	return QFileInfo(t1->pathname.GetAbsolutePathname().c_str()).completeBaseName() < QFileInfo(t2->pathname.GetAbsolutePathname().c_str()).completeBaseName();
}

bool SortFnByFileSize::operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2)
{
	return QFileInfo(t1->GetSourceTexturePathname().GetAbsolutePathname().c_str()).size() < QFileInfo(t2->GetSourceTexturePathname().GetAbsolutePathname().c_str()).size();
}

bool SortFnByImageSize::operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2)
{
	DAVA::Texture *tx1 = model->getTexture(t1);
	DAVA::Texture *tx2 = model->getTexture(t2);

	return (tx1->width * tx1->height) < (tx2->width * tx2->height);
}

bool SortFnByDataSize::operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2)
{
	DAVA::Texture *tx1 = model->getTexture(t1);
	DAVA::Texture *tx2 = model->getTexture(t2);

	return (tx1->width * tx1->height * DAVA::Texture::GetPixelFormatSizeInBytes(tx1->GetFormat())) < (tx2->width * tx2->height * DAVA::Texture::GetPixelFormatSizeInBytes(tx2->GetFormat()));
}
