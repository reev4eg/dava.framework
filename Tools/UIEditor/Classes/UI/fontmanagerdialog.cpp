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


#include "fontmanagerdialog.h"
#include "ui_fontmanagerdialog.h"
#include "ResourcesManageHelper.h"
#include "EditorFontManager.h"
#include "EditorSettings.h"
#include "TexturePacker/ResourcePacker2D.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QModelIndexList>
#include <QStandardItemModel>
#include <QStringList>
#include <QTableWidgetItem>

static const QString FONT_TABLE_NAME_COLUMN = "Font Name";
static const QString FONT_TABLE_TYPE_COLUMN = "Font Type";
static const QString FONT_TYPE_BASIC = "Basic";
static const QString FONT_TYPE_GRAPHIC = "Graphics";
static const QString LOAD_FONT_ERROR_MESSAGE = "Can't load font %1! Try again or select another one.";
static const QString LOAD_FONT_ERROR_INFO_TEXT = "An error occured while loading font...";

FontManagerDialog::FontManagerDialog(bool okButtonEnable,  const QString& graphicsFontPath, QDialog *parent) :
    QDialog(parent),
    ui(new Ui::FontManagerDialog)
{    
    ui->setupUi(this);
    // Setup ok button - it should be visible only if user want to change font of control.
    ui->okButton->setVisible(okButtonEnable);
    dialogResultFont = NULL;
	// Pack graphics fonts sprites each time sprite dialog is opened
	ResourcePacker2D *resPacker = new ResourcePacker2D();

	DAVA::String inDir = ResourcesManageHelper::GetFontSpritesDatasourceDirectory().toStdString();
	DAVA::String outDir = ResourcesManageHelper::GetFontSpritesDirectory().toStdString();
	// Do not pack resources if input/output folders are empty
	if (!inDir.empty() && !outDir.empty())
	{
		resPacker->InitFolders(inDir, outDir);
		resPacker->PackResources(GPU_UNKNOWN);
	}
    // Initialize dialog
    ConnectToSignals();
    InitializeTableView();
    UpdateTableViewContents();
 	UpdateDialogInformation();

	SafeDelete(resPacker);
	
	// Setup default path for graphics font sprites
	if (!graphicsFontPath.isEmpty())
	{
		currentFontPath = ResourcesManageHelper::GetFontAbsolutePath(graphicsFontPath);
	}
}

FontManagerDialog::~FontManagerDialog()
{
    SafeDelete(tableModel);
    SafeDelete(ui);
    SafeRelease(dialogResultFont);
}

void FontManagerDialog::ConnectToSignals()
{
    //Connect signal and slots
    connect(ui->okButton, SIGNAL(clicked()), this, SLOT(OkButtonClicked()));
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->setDefaultButton, SIGNAL(clicked()), this, SLOT(SetDefaultButtonClicked()));
}

void FontManagerDialog::UpdateDialogInformation()
{
	QString resDir = ResourcesManageHelper::GetResourceRootDirectory();
	// If resource folders are not available - hide infromation text
	if (resDir.isNull() || resDir.isEmpty())
	{
		ui->graphicsFontPath->setHidden(true);
		ui->trueTypeFontPath->setHidden(true);
		ui->trueTypeFontPathLabel->setHidden(true);
		ui->graphicsFontPathLabel->setHidden(true);	
	}
	else
	{
		// Show font folders
		ui->graphicsFontPath->setText(resDir + "/Fondef");
    	ui->trueTypeFontPath->setText(resDir + "/Fonts");
	}
}

void FontManagerDialog::InitializeTableView()
{
    //Setup table appearence
    ui->fontsTableView->verticalHeader()->hide();
    ui->fontsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fontsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->fontsTableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    //Disable editing of table
    ui->fontsTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);    
    //Create and set table view model
    tableModel = new QStandardItemModel(this); //0 Rows and 2 Columns
    //Setup column name
    
    tableModel->setHorizontalHeaderItem(0, new QStandardItem(QString(FONT_TABLE_NAME_COLUMN)));
    tableModel->setHorizontalHeaderItem(1, new QStandardItem(QString(FONT_TABLE_TYPE_COLUMN)));

    ui->fontsTableView->setModel(tableModel);
}

void FontManagerDialog::UpdateTableViewContents()
{
    //Remove all rows from table
    if (tableModel->rowCount() > 0)
    {
        tableModel->removeRows(0, tableModel->rowCount());
    }

	QString defFontName = EditorFontManager::Instance()->GetDefaultFontName();
    // Get all available fonts in resource folder
    QStringList fontsList = ResourcesManageHelper::GetFontsList();

    for (int i = 0; i < fontsList.size(); ++i)
    {
        QList<QStandardItem *> itemsList;
        // Fill table view with font name and font type values
        QString fontName = fontsList.at(i);
        if( !fontName.isNull() && !fontName.isEmpty())
        {            
			QStandardItem* item = CreateFontItem(fontName, fontName, defFontName);
			// Add font name
            itemsList.append(item);
			item = NULL;

            // Check font type - "*.def" should be a Graphics Font
            if (fontName.contains(".def"))
            {
                item = CreateFontItem(FONT_TYPE_GRAPHIC, fontName, defFontName);
            }
            else
            {
				item = CreateFontItem(FONT_TYPE_BASIC, fontName, defFontName);
            }						
			itemsList.append(item);
			item = NULL;
        }

        tableModel->appendRow(itemsList);
        itemsList.clear();
    }
}

void FontManagerDialog::OkButtonClicked()
{
    Font *returnFont = NULL;
    QItemSelectionModel *selectionModel = ui->fontsTableView->selectionModel();
    
    if (selectionModel->hasSelection())
    {
		returnFont = GetSelectedFont(selectionModel);
           
        if (returnFont)
        {
            //Set dialog resulting font - it corresponds to selected font by user
            dialogResultFont = returnFont->Clone();
			SafeRelease(returnFont);
            //Set dialog result as QDialog::Accepted and close it
            accept();
        }
    }
}

void FontManagerDialog::SetDefaultButtonClicked()
{
	Font *returnFont = NULL;
    QItemSelectionModel *selectionModel = ui->fontsTableView->selectionModel();
	
    if (selectionModel->hasSelection())
    {
		returnFont = GetSelectedFont(selectionModel);
        
        if (returnFont)
        {
			EditorFontManager::Instance()->SetDefaultFont(returnFont);
			// Update table view to show new default font
			UpdateTableViewContents();
			SafeRelease(returnFont);
        }
	}
}

QStandardItem* FontManagerDialog::CreateFontItem(QString itemText, QString fontName, QString defaultFontName)
{
	QStandardItem *item = new QStandardItem(itemText);
	if (!defaultFontName.isNull() && !defaultFontName.isEmpty() && defaultFontName.contains(fontName))
	{
		QFont itemFont = item->font();
		itemFont.setBold(true);
	    item->setFont(itemFont);
	}
	return item;
}

Font* FontManagerDialog::GetSelectedFont(QItemSelectionModel *selectionModel)
{
	Font *returnFont = NULL;
	// Get selected item
	QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
    //Get selected font name and type
	QString fontName = selectedIndexes.value(0).data().toString();
    QString fontType = selectedIndexes.value(1).data().toString();
	// Created font according to its type
	if (fontType == FONT_TYPE_GRAPHIC)
    {
		// Get sprites directory to open
		QString currentFontSpriteDir = ResourcesManageHelper::GetDefaultFontSpritesPath(currentFontPath);
    	QString fontSpritePath = QFileDialog::getOpenFileName(this, tr( "Select font sprite" ),
																	currentFontSpriteDir,
																	tr( "Sprites (*.txt)" ));
             
        if (!fontSpritePath.isNull() && !fontSpritePath.isEmpty())
        {
			// Convert file path into Unix-style path
			fontSpritePath = ResourcesManageHelper::ConvertPathToUnixStyle(fontSpritePath);

			if (ResourcesManageHelper::ValidateResourcePath(fontSpritePath))
			{
				// Get font definition relative path by it's name
				QString fontDefinition = ResourcesManageHelper::GetFontRelativePath(fontName, true);
				// Get sprite file relative path
				QString fontSprite = ResourcesManageHelper::GetResourceRelativePath(fontSpritePath);
				// Create Graphics font to validate it - but first truncate "*.txt" extension of sprite
				returnFont = GraphicsFont::Create(fontDefinition.toStdString(), fontSprite.toStdString());
			}
			else
			{
				ResourcesManageHelper::ShowErrorMessage(fontName);
				return returnFont;
			}
		}
	}
    else if (fontType == FONT_TYPE_BASIC)
    {
    	//Actions for simple font
        //Get font absolute path
        QString fontPath = ResourcesManageHelper::GetFontRelativePath(fontName);
        //Try to create font to validate it
        returnFont = FTFont::Create(fontPath.toStdString());
	}
	
	if (!returnFont)
	{
    	//If font was not created - show error message
        //No dialog result will be set in this case
         QString message;
         QMessageBox messageBox;
         messageBox.setText(message.append(QString(LOAD_FONT_ERROR_MESSAGE).arg(fontName)));
         messageBox.setInformativeText(LOAD_FONT_ERROR_INFO_TEXT);
         messageBox.setStandardButtons(QMessageBox::Ok);
         messageBox.exec();
	}
	
	return returnFont;
}

Font * FontManagerDialog::ResultFont()
{
    return (dialogResultFont ? dialogResultFont->Clone() : NULL);
}
