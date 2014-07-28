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



#include "QtUtils.h"
#include "Deprecated/SceneValidator.h"
#include "Tools/QtFileDialog/QtFileDialog.h"

#include <QMessageBox>
#include <QToolButton>
#include <QFileInfo>

#include "mainwindow.h"

#include "TexturePacker/CommandLineParser.h"
#include "Classes/CommandLine/TextureDescriptor/TextureDescriptorUtils.h"

#include "DAVAEngine.h"
#include <QProcess>
using namespace DAVA;


DAVA::FilePath PathnameToDAVAStyle(const QString &convertedPathname)
{
    return FilePath((const String &)QSTRING_TO_DAVASTRING(convertedPathname));
}


DAVA::FilePath GetOpenFileName(const DAVA::String &title, const DAVA::FilePath &pathname, const DAVA::String &filter)
{
    QString filePath = QtFileDialog::getOpenFileName(NULL, QString(title.c_str()), QString(pathname.GetAbsolutePathname().c_str()),
                                                    QString(filter.c_str()));
    
	// TODO: mainwindow
    //QtMainWindowHandler::Instance()->RestoreDefaultFocus();

    FilePath openedPathname = PathnameToDAVAStyle(filePath);
    if(!openedPathname.IsEmpty() && !SceneValidator::Instance()->IsPathCorrectForProject(openedPathname))
    {
        //Need to Show Error
		ShowErrorDialog(String(Format("File(%s) was selected from incorect project.", openedPathname.GetAbsolutePathname().c_str())));
        openedPathname = FilePath();
    }
    
    if(openedPathname.IsEqualToExtension(".png"))
    {
        //VK: create descriptor only for *.png without paired *.tex
        TextureDescriptorUtils::CreateDescriptorIfNeed(openedPathname);
    }
    
    return openedPathname;
}


DAVA::String SizeInBytesToString(DAVA::float32 size)
{
    DAVA::String retString = "";
    
    if(1000000 < size)
    {
        retString = Format("%0.2f MB", size / (1024 * 1024) );
    }
    else if(1000 < size)
    {
        retString = Format("%0.2f KB", size / 1024);
    }
    else
    {
        retString = Format("%d B", (int32)size);
    }
    
    return  retString;
}

DAVA::WideString SizeInBytesToWideString(DAVA::float32 size)
{
    return StringToWString(SizeInBytesToString(size));
}


DAVA::Image * CreateTopLevelImage(const DAVA::FilePath &imagePathname)
{
    Image *image = NULL;
    Vector<Image *> imageSet;
    ImageSystem::Instance()->Load(imagePathname, imageSet);
    if(0 != imageSet.size())
    {
        image = SafeRetain(imageSet[0]);
		for_each(imageSet.begin(), imageSet.end(), SafeRelease<Image>);
    }
    
    return image;
}

void ShowErrorDialog(const DAVA::Set<DAVA::String> &errors)
{
    if(errors.empty())
        return;
    
    String errorMessage = String("");
    Set<String>::const_iterator endIt = errors.end();
    for(Set<String>::const_iterator it = errors.begin(); it != endIt; ++it)
    {
		Logger::Error((*it).c_str());

        errorMessage += *it + String("\n");
    }
    
    ShowErrorDialog(errorMessage);
}

void ShowErrorDialog(const DAVA::String &errorMessage)
{
	bool forceClose =    CommandLineParser::CommandIsFound(String("-force"))
					||  CommandLineParser::CommandIsFound(String("-forceclose"));
	if(!forceClose)
	{
		QMessageBox::critical(QtMainWindow::Instance(), "Error", errorMessage.c_str());
	}
}

bool IsKeyModificatorPressed(int32 key)
{
	return InputSystem::Instance()->GetKeyboard()->IsKeyPressed(key);
}

bool IsKeyModificatorsPressed()
{
	return (IsKeyModificatorPressed(DVKEY_SHIFT) || IsKeyModificatorPressed(DVKEY_CTRL) || IsKeyModificatorPressed(DVKEY_ALT));
}

QColor ColorToQColor(const DAVA::Color& color)
{
    DAVA::float32 maxC = 1.0;

    if(maxC < color.r) maxC = color.r;
    if(maxC < color.g) maxC = color.g;
    if(maxC < color.b) maxC = color.b;

	return QColor::fromRgbF(color.r / maxC, color.g / maxC, color.b / maxC, DAVA::Clamp(color.a, 0.0f, 1.0f));
}

DAVA::Color QColorToColor(const QColor &qcolor)
{
	return Color(qcolor.redF(), qcolor.greenF(), qcolor.blueF(), qcolor.alphaF());
}

int ShowQuestion(const DAVA::String &header, const DAVA::String &question, int buttons, int defaultButton)
{
    int answer = QMessageBox::question(NULL, QString::fromStdString(header), QString::fromStdString(question),
                                       (QMessageBox::StandardButton)buttons, (QMessageBox::StandardButton)defaultButton);
    return answer;
}

void ShowActionWithText(QToolBar *toolbar, QAction *action, bool showText)
{
	if(NULL != toolbar && NULL != action)
	{
		QToolButton *toolBnt = dynamic_cast<QToolButton *>(toolbar->widgetForAction(action));
		if(NULL != toolBnt)
		{
			if(showText)
			{
				toolBnt->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			}
			else
			{
				toolBnt->setToolButtonStyle(Qt::ToolButtonIconOnly);
			}
		}
	}
}

DAVA::String ReplaceInString(const DAVA::String & sourceString, const DAVA::String & what, const DAVA::String & on)
{
	String::size_type pos = sourceString.find(what);
	if(pos != String::npos)
	{
		String newString = sourceString;
		newString = newString.replace(pos, what.length(), on);
		return newString;
	}

	return sourceString;
}

void ShowFileInExplorer(const QString& path)
{
    const QFileInfo fileInfo(path);

#if defined (Q_WS_MAC)
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \"" + fileInfo.absoluteFilePath() + "\"";
    args << "-e";
    args << "end tell";
    QProcess::startDetached( "osascript", args );
#elif defined (Q_WS_WIN)
    QStringList args;
    args << "/select," << QDir::toNativeSeparators( fileInfo.absoluteFilePath() );
    QProcess::startDetached( "explorer", args );
#endif//

}
