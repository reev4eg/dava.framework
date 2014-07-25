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


#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "DAVAEngine.h"
#include "FileSystem/YamlParser.h"
#include "FileSystem/VariantType.h"

#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QDateTime>
#include <QDebug>

using namespace DAVA;

const QString APP_NAME = "Resource_Explorer";
const QString APP_COMPANY = "DAVA";
const QString YAML_PATH = "YAML_PATH";
const QString RES_PATH = "RES_PATH";
const QString IGNORE_PATH = "IGNORE_PATH";
const QString SAVE_SETTINGS = "SAVE_SETTINGS";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{	
    ui->setupUi(this);    
	this->setWindowTitle("Resource Explorer");

	RestoreProjectSettings();
    
    ui->deleteResourcesButton->setVisible(false);

	connect(ui->yamlButton, SIGNAL(clicked()), this, SLOT(OnOpenYamlDirectoryDialog()));
	connect(ui->resourcesButton, SIGNAL(clicked()), this, SLOT(OnOpenResourceDirectoryDialog()));
	connect(ui->ignoreButton, SIGNAL(clicked()), this, SLOT(OnOpenIgnoreFileDialog()));
	connect(ui->checkUnusedResourcesButton, SIGNAL(clicked()), this, SLOT(OnGetUnusedResourcesList()));
}

MainWindow::~MainWindow()
{
	SaveProjectSettings();
    delete ui;
}

void MainWindow::OnOpenYamlDirectoryDialog()
{
	QString yamlDir = QFileDialog::getExistingDirectory(this, tr("Choose yaml files folder:"));
	if (!yamlDir.isEmpty())
	{
		ui->yamlLineEdit->setText(yamlDir);
	}
}

void MainWindow::OnOpenResourceDirectoryDialog()
{
	QString resDir = QFileDialog::getExistingDirectory(this, tr("Choose resources folder:"));
	if (!resDir.isEmpty())
	{
		ui->resourcesLineEdit->setText(resDir);
	}
}

void MainWindow::OnOpenIgnoreFileDialog()
{
	QString ignoreFile = QFileDialog::getOpenFileName(this, tr("Choose ignore list file:"));
	if (!ignoreFile.isEmpty())
	{
		ui->ignoreLineEdit->setText(ignoreFile);
	}	
}

void MainWindow::SaveProjectSettings()
{
	QSettings settings(APP_COMPANY, APP_NAME);
	settings.setValue(SAVE_SETTINGS, ui->saveSettingsCheckBox->isChecked());
	settings.setValue(IGNORE_PATH, ui->ignoreLineEdit->text());
	if (ui->saveSettingsCheckBox->isChecked())
	{
		settings.setValue(YAML_PATH, ui->yamlLineEdit->text());
		settings.setValue(RES_PATH, ui->resourcesLineEdit->text());
	}

}

void MainWindow::RestoreProjectSettings()
{
	QSettings settings(APP_COMPANY, APP_NAME);
	// Init project default settings
	if (!settings.value(SAVE_SETTINGS).isNull() && settings.value(SAVE_SETTINGS).isValid())
	{
		ui->saveSettingsCheckBox->setChecked(settings.value(SAVE_SETTINGS).toBool());
	}

	if (!settings.value(IGNORE_PATH).isNull() && settings.value(IGNORE_PATH).isValid())
	{
		ui->ignoreLineEdit->setText(settings.value(IGNORE_PATH).toString());
	}
	// Do not restore yaml and resources path if flag is dropped
	if (ui->saveSettingsCheckBox->isChecked())
	{
		if (!settings.value(YAML_PATH).isNull() && settings.value(YAML_PATH).isValid())
		{
			ui->yamlLineEdit->setText(settings.value(YAML_PATH).toString());
		}
		if (!settings.value(RES_PATH).isNull() && settings.value(RES_PATH).isValid())
		{
			ui->resourcesLineEdit->setText(settings.value(RES_PATH).toString());
		}
	}
}

void MainWindow::OnExitApplication()
{
	QCoreApplication::exit();
}

void MainWindow::OnGetUnusedResourcesList()
{
	resourcesList.clear();
	yamlList.clear();
	ignoreList.clear();

	BuildYamlResourcesList(ui->yamlLineEdit->text());
	BuildResourcesList(ui->resourcesLineEdit->text());
	BuildIgnoreResourcesList(ui->ignoreLineEdit->text());

	BuildUnusedResourcesList();
/*	qDebug() << " FOUND RESOURCES COUNT - " << resourcesList.count();
    qDebug() << " FOUND YAML COUNT - " << resourcesList.count();
	for (int i = 0; i < resourcesList.size(); ++i) 
	{
	 qDebug() << " RES - " << resourcesList.at(i);
	}
	for (int i = 0; i < yamlList.size(); ++i) 
	{
	 qDebug() << " YAML - " << yamlList.at(i);
	}*/
}

void MainWindow::BuildYamlResourcesList(const QString &filePath)
{
	QDirIterator dirIt(filePath, QDirIterator::Subdirectories);
	while (dirIt.hasNext()) 
	{
		dirIt.next();
		if (QFileInfo(dirIt.filePath()).isFile())
		{
			// Get yaml file and find resources
			if (QFileInfo(dirIt.filePath()).suffix() == "yaml")
			{
				LoadResourcesFromYamlFile(dirIt.filePath().toStdString());
			}
		}
	}
}

void MainWindow::LoadResourcesFromYamlFile(const FilePath & pathName)
{
	YamlParser	*parser	= DAVA::YamlParser::Create(pathName);
    if(NULL == parser)
        return;

    LoadFromYamlNode(parser->GetRootNode());
	SafeRelease(parser);
}

void MainWindow::LoadFromYamlNode(const YamlNode* rootNode)
{
	if (NULL == rootNode)
		return;

	int cnt = rootNode->GetCount();
	for (int k = 0; k < cnt; ++k)
	{
		const YamlNode * node = rootNode->Get(k);
		if (NULL == node)
			continue;

		const YamlNode * spriteNode = node->Get("sprite");
		// Get resource path from sprite node
		if (spriteNode) 
		{ 
			QString absoluteSpritePath;
			String spritePath = spriteNode->AsString();
			FilePath spriteFilePath = FilePath(spritePath);

			if (spriteFilePath.GetType() == FilePath::PATH_IN_RESOURCES)
			{
				absoluteSpritePath = QString::fromStdString(spritePath.substr(5));
			}
			else
			{				
				absoluteSpritePath = QString::fromStdString(spriteFilePath.GetAbsolutePathname());
			}
			
			if (!absoluteSpritePath.isEmpty())
				yamlList.push_back(absoluteSpritePath + ".psd");
		}

		LoadFromYamlNode(node);
	}
}

void MainWindow::BuildResourcesList(const QString &filePath)
{
	QDirIterator dirIt(filePath, QDirIterator::Subdirectories);
	while (dirIt.hasNext()) 
	{
		dirIt.next();
		if (QFileInfo(dirIt.filePath()).isFile())
		{
			// Get yaml file and find resources
			if (QFileInfo(dirIt.filePath()).suffix() == "psd")
			{
				resourcesList.push_back(dirIt.filePath());
			}
		}
	}
}

void MainWindow::BuildIgnoreResourcesList(const QString &filePath)
{
	QFile inputFile(filePath);
    if (inputFile.open(QIODevice::ReadOnly))
    {
       QTextStream in(&inputFile);
       while ( !in.atEnd() )
       {
          QString line = in.readLine();
		  ignoreList.push_back(line);
       }
       inputFile.close();
    }
}

void MainWindow::BuildUnusedResourcesList()
{
	QStringList unusedResList(resourcesList);
	ui->unusedRecourcesList->clear();
    
	for (int i = 0; i < yamlList.size(); ++i)
	{
		QString resourceStr = yamlList.at(i);
        QStringList searchResult = resourcesList.filter(resourceStr);
        // Remove used resources from list
        for (int k = 0; k < searchResult.size(); ++k)
        {
        	unusedResList.removeAll(searchResult.at(k));
        }
	}
    
    for (int i = 0; i < ignoreList.size(); ++i)
    {
		QString resourceStr = ignoreList.at(i);
        QStringList searchResult = resourcesList.filter(resourceStr);
        // Remove ignored resources from list
        for (int k = 0; k < searchResult.size(); ++k)
        {
        	unusedResList.removeAll(searchResult.at(k));
        }
    }

	if (unusedResList.size() > 0)
    {
		ui->unusedRecourcesList->addItems(unusedResList);
        ui->statusBar->showMessage(QString("Found %1 unused resources.").arg(unusedResList.size()), 10000);
    }
}

QString MainWindow::ConvertPathToUnixStyle(const QString& inputString)
{
	// Replace simple slash to unix style slash
	QString outputString = inputString;
	outputString.replace(QString("\\") ,QString("/"));
	return outputString;
}