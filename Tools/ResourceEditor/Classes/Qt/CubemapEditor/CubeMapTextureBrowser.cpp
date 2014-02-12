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


#include "CubemapEditor/CubeMapTextureBrowser.h"
#include "CubemapEditor/CubemapEditorDialog.h"
#include "CubemapEditor/CubemapUtils.h"
#include "Qt/Main/QtUtils.h"
#include "ui_CubeMapTextureBrowser.h"
#include "../../StringConstants.h"
#include "Scene3D/Systems/SkyboxSystem.h"
#include "Tools/QtFileDialog/QtFileDialog.h"
#include "Project/ProjectManager.h"

#include <QFileDialog>
#include <QScrollBar>

#include "Qt/Settings/SettingsManager.h"
#include <qdir>

const int FACE_IMAGE_SIZE = 64;

CubeMapTextureBrowser::CubeMapTextureBrowser(SceneEditor2* currentScene, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CubeMapTextureBrowser),
	cubeListItemDelegate(QSize(FACE_IMAGE_SIZE, FACE_IMAGE_SIZE))
{
	scene = currentScene;
	
    ui->setupUi(this);
	ui->loadingWidget->setVisible(false);
	ui->listTextures->setItemDelegate(&cubeListItemDelegate);
	
	ConnectSignals();
	
	FilePath projectPath = CubemapUtils::GetDialogSavedPath(ResourceEditor::SETTINGS_CUBEMAP_LAST_PROJECT_DIR,
															FilePath(ProjectManager::Instance()->CurProjectDataSourcePath().toStdString()).GetAbsolutePathname(),
															FilePath(ProjectManager::Instance()->CurProjectDataSourcePath().toStdString()).GetAbsolutePathname());
		
	ui->textRootPath->setText(projectPath.GetAbsolutePathname().c_str());
	ReloadTextures(projectPath.GetAbsolutePathname());
	
	UpdateCheckedState();
}

CubeMapTextureBrowser::~CubeMapTextureBrowser()
{
    delete ui;
}

void CubeMapTextureBrowser::ConnectSignals()
{    
	QObject::connect(ui->buttonSelectRootPath, SIGNAL(pressed()), this, SLOT(OnChooseDirectoryClicked()));
	QObject::connect(ui->buttonCreateCube, SIGNAL(pressed()), this, SLOT(OnCreateCubemapClicked()));
	QObject::connect(ui->buttonReload, SIGNAL(pressed()), this, SLOT(OnReloadClicked()));
	QObject::connect(&cubeListItemDelegate, SIGNAL(OnEditCubemap(const QModelIndex&)), this, SLOT(OnEditCubemap(const QModelIndex&)));
	QObject::connect(&cubeListItemDelegate, SIGNAL(OnItemCheckStateChanged(const QModelIndex&)), this, SLOT(OnItemCheckStateChanged(const QModelIndex&)));
	QObject::connect(ui->buttonRemove, SIGNAL(pressed()), this, SLOT(OnDeleteSelectedItemsClicked()));
}

void CubeMapTextureBrowser::ReloadTextures(const DAVA::String& rootPath)
{	
	cubeListItemDelegate.ClearCache();
	ui->listTextures->clear();
	ui->listTextures->setVisible(false);
	ui->loadingWidget->setVisible(true);
	
	
	this->paintEvent(NULL);
	ui->loadingWidget->update();
	QApplication::processEvents();
	QApplication::flush();
	
	this->setUpdatesEnabled(false);

	QDir dir(rootPath.c_str());
	QStringList filesList = dir.entryList(QStringList("*.tex"));
	size_t cubemapTextures = 0;
	
	if(filesList.size() > 0)
	{
		DAVA::Vector<CubeListItemDelegate::ListItemInfo> cubemapList;
		FilePath fp = rootPath;
		for(int i = 0; i < filesList.size(); ++i)
		{
			QString str = filesList.at(i);
			fp.ReplaceFilename(str.toStdString());
			
			DAVA::TextureDescriptor* texDesc = DAVA::TextureDescriptor::CreateFromFile(fp);
			if(texDesc && texDesc->IsCubeMap())
			{
				CubeListItemDelegate::ListItemInfo itemInfo;
				itemInfo.path = fp;
				itemInfo.valid = ValidateTextureAndFillThumbnails(fp, itemInfo.icons, itemInfo.actualSize);
				
				if(itemInfo.valid)
				{
					cubemapList.push_back(itemInfo);
				}
				else
				{
					//non-valid items should be always at the beginning of the list
					cubemapList.insert(cubemapList.begin(), itemInfo);
				}
			}

			SafeDelete(texDesc);
		}
		
		cubeListItemDelegate.UpdateCache(cubemapList);
		
		for(size_t i = 0; i < cubemapList.size(); ++i)
		{
			CubeListItemDelegate::ListItemInfo& itemInfo = cubemapList[i];
			
			QListWidgetItem* listItem = new QListWidgetItem();
			listItem->setData(Qt::CheckStateRole, false);
			listItem->setData(CUBELIST_DELEGATE_ITEMFULLPATH, itemInfo.path.GetAbsolutePathname().c_str());
			listItem->setData(CUBELIST_DELEGATE_ITEMFILENAME, itemInfo.path.GetFilename().c_str());
			ui->listTextures->addItem(listItem);
		}
		
		cubemapTextures = cubemapList.size();
		ui->listTextures->setCurrentItem(ui->listTextures->item(0));
	}
	
	this->setUpdatesEnabled(true);
	
	ui->listTextures->setVisible(cubemapTextures > 0);
	ui->loadingWidget->setVisible(false);
	
	UpdateCheckedState();
}

void CubeMapTextureBrowser::ReloadTexturesFromUI(QString& path)
{
	if(path.at(path.size() - 1) != QChar('/') &&
	   path.at(path.size() - 1) != QChar('\\'))
	{
		path += "/";
	}
	
	ui->textRootPath->setText(path);
	
	FilePath projectPath = path.toStdString();
	SettingsManager::Instance()->SetValue(ResourceEditor::SETTINGS_CUBEMAP_LAST_PROJECT_DIR, VariantType(projectPath.GetAbsolutePathname()), SettingsManager::INTERNAL);
	
	ReloadTextures(path.toStdString());
}

void CubeMapTextureBrowser::RestoreListSelection(int currentRow)
{
	if(ui->listTextures->count() >= currentRow &&
	   currentRow >= 0)
	{
		QListWidgetItem* item = ui->listTextures->item(currentRow);
		ui->listTextures->scrollToItem(item);
		ui->listTextures->setCurrentItem(item);
	}
}

///////////////////////////////////////////////////////////////

void CubeMapTextureBrowser::OnChooseDirectoryClicked()
{
	QString newDir = QtFileDialog::getExistingDirectory(this, tr("Open Directory"),
													   ui->textRootPath->text());
	if(!newDir.isNull())
	{
		ReloadTexturesFromUI(newDir);
	}
}

void CubeMapTextureBrowser::OnReloadClicked()
{
	QString path = ui->textRootPath->text();
	
	if(scene)
	{
		scene->skyboxSystem->Reload();
	}
	
	ReloadTexturesFromUI(path);
}

void CubeMapTextureBrowser::OnCreateCubemapClicked()
{
	QString fileName = QtFileDialog::getSaveFileName(this,
													tr("Create Cubemap Texture"),
													ui->textRootPath->text(),
													tr("Tex File (*.tex)"));
	
	if(!fileName.isNull())
	{
		CubemapEditorDialog dlg(this);
		FilePath fp = fileName.toStdString();
		
		DAVA::FilePath rootPath = fp.GetDirectory();
		dlg.InitForCreating(fp, rootPath);
		dlg.exec();
		
		QString path = rootPath.GetAbsolutePathname().c_str();
		ui->textRootPath->setText(path);
		int currentRow = ui->listTextures->currentRow();
		ReloadTexturesFromUI(path);
		
		if(ui->listTextures->count() > 0 &&
		   currentRow < ui->listTextures->count())
		{
			RestoreListSelection(currentRow);
		}
	}	
}

void CubeMapTextureBrowser::OnEditCubemap(const QModelIndex &index)
{
	CubemapEditorDialog dlg(this);
	
	QListWidgetItem* item = ui->listTextures->item(index.row());
	
	DAVA::FilePath rootPath = ui->textRootPath->text().toStdString();
	FilePath fp = item->data(CUBELIST_DELEGATE_ITEMFULLPATH).toString().toStdString();
	dlg.InitForEditing(fp, rootPath);
	dlg.exec();

	int currentRow = ui->listTextures->currentRow();
	QString path = ui->textRootPath->text();
	ReloadTexturesFromUI(path);
	
	RestoreListSelection(currentRow);
}

void CubeMapTextureBrowser::OnItemCheckStateChanged(const QModelIndex &index)
{
	UpdateCheckedState();
}

void CubeMapTextureBrowser::UpdateCheckedState()
{
	int checkedItemCount = GetCheckedItemsCount();
	QString text = (checkedItemCount > 0) ? QString("%1 item(s) selected").arg(QString().setNum(checkedItemCount)) : QString("");
	ui->selectedItemsStatus->setText(text);

	ui->buttonRemove->setEnabled(checkedItemCount > 0);
}

int CubeMapTextureBrowser::GetCheckedItemsCount()
{
	int itemCount = ui->listTextures->count();
	int checkedItemCount = 0;
	for(int i = 0; i < itemCount; ++i)
	{
		QListWidgetItem* item = ui->listTextures->item(i);
		bool checkedState = item->data(Qt::CheckStateRole).toBool();
		
		if(checkedState)
		{
			checkedItemCount++;
		}
	}
	
	return checkedItemCount;
}

void CubeMapTextureBrowser::OnDeleteSelectedItemsClicked()
{
	int checkedItemCount = GetCheckedItemsCount();
	int answer = MB_FLAG_NO;
	if(checkedItemCount > 0)
	{
		QString text = QString("%1 item(s) will be deleted. Continue?").arg(QString().setNum(checkedItemCount));
		answer = ShowQuestion("Confirmation",
							  text.toStdString(),
							  MB_FLAG_YES | MB_FLAG_NO,
							  MB_FLAG_NO);
	}
	
	if(MB_FLAG_YES == answer)
	{
		DAVA::Vector<DAVA::String> failedToRemove;
		int itemCount = ui->listTextures->count();
		for(int i = 0; i < itemCount; ++i)
		{
			QListWidgetItem* item = ui->listTextures->item(i);
			bool checkedState = item->data(Qt::CheckStateRole).toBool();
			
			if(checkedState)
			{
				FilePath fp = item->data(CUBELIST_DELEGATE_ITEMFULLPATH).toString().toStdString();
				if(fp.Exists())
				{
					DAVA::Vector<DAVA::String> faceNames;
					CubemapUtils::GenerateFaceNames(fp.GetAbsolutePathname(), faceNames);
					for(size_t faceIndex = 0; faceIndex < faceNames.size(); ++faceIndex)
					{
						FilePath hackTex = faceNames[faceIndex];
						hackTex.ReplaceExtension(".tex");
						
						QFile::remove(hackTex.GetAbsolutePathname().c_str());
						bool removeResult = QFile::remove(faceNames[faceIndex].c_str());
						if(!removeResult)
						{
							failedToRemove.push_back(faceNames[faceIndex]);
						}
					}
					
					bool removeResult = QFile::remove(fp.GetAbsolutePathname().c_str());
					if(!removeResult)
					{
						failedToRemove.push_back(fp.GetAbsolutePathname().c_str());
					}
				}
			}
		}
		
		if(failedToRemove.size() > 0)
		{
			DAVA::String fileList;
			int count = failedToRemove.size();
			for(int i = 0; i < count; ++i)
			{
				fileList += failedToRemove[i];
				fileList += "\n";
			}
			
			DAVA::String message = "Failed to remove the following files. Please delete them manually.\n";
			message += fileList;
			
			ShowErrorDialog(message);
		}

		QString path = ui->textRootPath->text();
		ReloadTexturesFromUI(path);
		UpdateCheckedState();
	}
}

bool CubeMapTextureBrowser::ValidateTextureAndFillThumbnails(DAVA::FilePath& fp,
									  DAVA::Vector<QImage*>& icons,
									  DAVA::Vector<QSize>& actualSize)
{
	bool result = true;
	
	int width = 0;
	int height = 0;
	DAVA::Vector<DAVA::String> faceNames;
	CubemapUtils::GenerateFaceNames(fp.GetAbsolutePathname(), faceNames);
	for(size_t i = 0; i < faceNames.size(); ++i)
	{
		QImage faceImage;
		if(!faceImage.load(faceNames[i].c_str())) //file must be present
		{
			result = false;
		}
	
		if(faceImage.width() != faceImage.height() || //file must be square and be power of 2
		   !IsPowerOf2(faceImage.width()))
		{
			result = false;
		}
				
		if(0 == i)
		{
			width = faceImage.width();
			height = faceImage.height();
		}
		else if(faceImage.width() != width || //all files should be the same size
				faceImage.height() != height)
		{
			result = false;
		}
		
		//scale image and put scaled version to an array
		QImage scaledFaceTemp = faceImage.scaled(FACE_IMAGE_SIZE, FACE_IMAGE_SIZE);
		QImage* scaledFace = new QImage(scaledFaceTemp);
		
		icons.push_back(scaledFace);
		actualSize.push_back(QSize(faceImage.width(), faceImage.height()));
	}
	
	return result;
}