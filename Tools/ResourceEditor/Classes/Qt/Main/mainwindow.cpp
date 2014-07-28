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



#include "DAVAEngine.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QColorDialog>
#include <QShortcut>
#include <QKeySequence>
#include <QMetaObject>
#include <QMetaType>

#include "mainwindow.h"
#include "QtUtils.h"
#include "Project/ProjectManager.h"
#include "DockConsole/Console.h"
#include "Scene/SceneHelper.h"
#include "SpritesPacker/SpritePackerHelper.h"

#include "TextureBrowser/TextureBrowser.h"
#include "SoundComponentEditor/FMODSoundBrowser.h"
#include "TextureBrowser/TextureCache.h"
#include "MaterialEditor/MaterialEditor.h"
#include "QualitySwitcher/QualitySwitcher.h"

#include "Qt/Settings/SettingsManager.h"
#include "Deprecated/EditorConfig.h"

#include "../CubemapEditor/CubemapUtils.h"
#include "../CubemapEditor/CubemapTextureBrowser.h"
#include "../Tools/AddSkyboxDialog/AddSkyboxDialog.h"
#include "../ImageSplitterDialog/ImageSplitterDialog.h"

#include "Tools/BaseAddEntityDialog/BaseAddEntityDialog.h"
#include "Tools/QtFileDialog/QtFileDialog.h"

#ifdef __DAVAENGINE_SPEEDTREE__
#include "Classes/Qt/SpeedTreeImport/SpeedTreeImportDialog.h"
#endif

#include "../Tools/AddSwitchEntityDialog/AddSwitchEntityDialog.h"

#include "Classes/Commands2/EntityAddCommand.h"
#include "StringConstants.h"
#include "Settings/SettingsManager.h"
#include "Settings/SettingsDialog.h"

#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/CommandLine/CommandLineManager.h"

#include "Render/Highlevel/ShadowVolumeRenderPass.h"

#include "Classes/Commands2/LandscapeEditorDrawSystemActions.h"

#include "Classes/CommandLine/SceneSaver/SceneSaver.h"
#include "Classes/Qt/Main/Request.h"
#include "Classes/Commands2/BeastAction.h"

#include "Classes/Commands2/CustomColorsCommands2.h"
#include "Classes/Commands2/HeightmapEditorCommands2.h"
#include "Classes/Commands2/LandscapeEditorDrawSystemActions.h"
#include "Classes/Commands2/RulerToolActions.h"
#include "Classes/Commands2/TilemaskEditorCommands.h"
#include "Classes/Commands2/VisibilityToolActions.h"
#include "Classes/Commands2/AddComponentCommand.h"
#include "Classes/Commands2/RemoveComponentCommand.h"
#include "Classes/Commands2/EntityRemoveCommand.h"
#include "Classes/Commands2/DynamicShadowCommands.h"

#include "Classes/Qt/Tools/QtLabelWithActions/QtLabelWithActions.h"

#include "Tools/HangingObjectsHeight/HangingObjectsHeight.h"
#include "Tools/ToolButtonWithWidget/ToolButtonWithWidget.h"

#include "Scene3D/Components/ActionComponent.h"
#include "Scene3D/Systems/SkyboxSystem.h"
#include "Scene3D/Systems/MaterialSystem.h"

#include "Classes/Constants.h"

#include "TextureCompression/TextureConverter.h"
#include "RecentFilesManager.h"
#include "Deprecated/SceneValidator.h"

#include "Tools/DeveloperTools/DeveloperTools.h"
#include "Render/Highlevel/Vegetation/VegetationRenderObject.h"

#include "Classes/Qt/BeastDialog/BeastDialog.h"
#include "DebugTools/VersionInfoWidget/VersionInfoWidget.h"
#include "Classes/Qt/RunActionEventWidget/RunActionEventWidget.h"


QtMainWindow::QtMainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, waitDialog(NULL)
	, beastWaitDialog(NULL)
	, objectTypesLabel(NULL)
	, addSwitchEntityDialog(NULL)
	, hangingObjectsWidget(NULL)
	, globalInvalidate(false)
    , modificationWidget(NULL)
    , developerTools(new DeveloperTools(this))
{
	new Console();
	new ProjectManager();
	new RecentFilesManager();
	ui->setupUi(this);
    
    SetupTitle();

	qApp->installEventFilter(this);

    SetupDocks();
	SetupMainMenu();
	SetupToolBars();
	SetupStatusBar();
	SetupActions();
	SetupShortCuts();

	// create tool windows
	new TextureBrowser(this);
	new MaterialEditor(this);
    new FMODSoundBrowser(this);

	waitDialog = new QtWaitDialog(this);

	beastWaitDialog = new QtWaitDialog(this);

	posSaver.Attach(this);
	posSaver.LoadState(this);

	QObject::connect(ProjectManager::Instance(), SIGNAL(ProjectOpened(const QString &)), this, SLOT(ProjectOpened(const QString &)));
	QObject::connect(ProjectManager::Instance(), SIGNAL(ProjectClosed()), this, SLOT(ProjectClosed()));

	QObject::connect(SceneSignals::Instance(), SIGNAL(CommandExecuted(SceneEditor2 *, const Command2*, bool)), this, SLOT(SceneCommandExecuted(SceneEditor2 *, const Command2*, bool)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2 *)), this, SLOT(SceneActivated(SceneEditor2 *)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(Deactivated(SceneEditor2 *)), this, SLOT(SceneDeactivated(SceneEditor2 *)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(SelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)), this, SLOT(SceneSelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)));


	QObject::connect(SceneSignals::Instance(), SIGNAL(EditorLightEnabled(bool)), this, SLOT(EditorLightEnabled(bool)));

    QObject::connect(this, SIGNAL(TexturesReloaded()), TextureCache::Instance(), SLOT(ClearCache()));
    
	LoadGPUFormat();
    LoadMaterialLightViewMode();

    EnableGlobalTimeout(globalInvalidate);

	EnableProjectActions(false);
	EnableSceneActions(false);

	DiableUIForFutureUsing();
}

QtMainWindow::~QtMainWindow()
{
	SafeDelete(addSwitchEntityDialog);
    SafeDelete(developerTools);
    
    TextureBrowser::Instance()->Release();
	MaterialEditor::Instance()->Release();

	posSaver.SaveState(this);

	delete ui;
	ui = NULL;

	ProjectManager::Instance()->Release();
	Console::Instance()->Release();
	RecentFilesManager::Instance()->Release();
}

Ui::MainWindow* QtMainWindow::GetUI()
{
	return ui;
}

SceneTabWidget* QtMainWindow::GetSceneWidget()
{
	return ui->sceneTabWidget;
}

SceneEditor2* QtMainWindow::GetCurrentScene()
{
	return ui->sceneTabWidget->GetCurrentScene();
}

bool QtMainWindow::SaveScene( SceneEditor2 *scene )
{
	bool sceneWasSaved = false;

	DAVA::FilePath scenePath = scene->GetScenePath();
	if(!scene->IsLoaded() || scenePath.IsEmpty())
	{
		sceneWasSaved = SaveSceneAs(scene);
	} 
	else
	{
		// SZ: df-2128
		// This check was removed until all editor actions will be done through commands
		// because it's not possible to save scene if some thing changes without command
		// 
		//if(scene->IsChanged())
		{
			if(DAVA::SceneFileV2::ERROR_NO_ERROR != scene->Save(scenePath))
			{
				QMessageBox::warning(this, "Save error", "An error occurred while saving the scene. See log for more info.", QMessageBox::Ok);
			}
            else
            {
                sceneWasSaved = true;
            }
		}
	}

	return sceneWasSaved;
}


bool QtMainWindow::SaveSceneAs(SceneEditor2 *scene)
{
	bool ret = false;

	if(NULL != scene)
	{
		DAVA::FilePath saveAsPath = scene->GetScenePath();
		if(!saveAsPath.Exists())
		{
            DAVA::FilePath dataSourcePath = ProjectManager::Instance()->CurProjectDataSourcePath();
			saveAsPath = dataSourcePath.MakeDirectoryPathname() + scene->GetScenePath().GetFilename();
		}

		QString selectedPath = QtFileDialog::getSaveFileName(this, "Save scene as", saveAsPath.GetAbsolutePathname().c_str(), "DAVA Scene V2 (*.sc2)");
		if(!selectedPath.isEmpty())
		{
			DAVA::FilePath scenePath = DAVA::FilePath(selectedPath.toStdString());
			if(!scenePath.IsEmpty())
			{
				scene->SetScenePath(scenePath);
				ret = scene->Save(scenePath);

				if(DAVA::SceneFileV2::ERROR_NO_ERROR != ret)
				{
					QMessageBox::warning(this, "Save error", "An error occurred while saving the scene. Please, see logs for more info.", QMessageBox::Ok);
				}
				else
				{
					ret = true;
					AddRecent(scenePath.GetAbsolutePathname().c_str());
				}
			}
		}
	}

	return ret;
}

DAVA::eGPUFamily QtMainWindow::GetGPUFormat()
{
    return GPUFamilyDescriptor::ConvertValueToGPU(SettingsManager::GetValue(Settings::Internal_TextureViewGPU).AsInt32());
}

void QtMainWindow::SetGPUFormat(DAVA::eGPUFamily gpu)
{
	// before reloading textures we should save tilemask texture for all opened scenes
	if(SaveTilemask())
	{
		SettingsManager::SetValue(Settings::Internal_TextureViewGPU, VariantType(gpu));
		DAVA::Texture::SetDefaultGPU(gpu);

		DAVA::TexturesMap allScenesTextures;
		for(int tab = 0; tab < GetSceneWidget()->GetTabCount(); ++tab)
		{
			SceneEditor2 *scene = GetSceneWidget()->GetTabScene(tab);
			SceneHelper::EnumerateSceneTextures(scene, allScenesTextures, SceneHelper::EXCLUDE_NULL);
		}

		if(allScenesTextures.size() > 0)
		{
			int progress = 0;
			WaitStart("Reloading textures...", "", 0, allScenesTextures.size());

			DAVA::TexturesMap::const_iterator it = allScenesTextures.begin();
			DAVA::TexturesMap::const_iterator end = allScenesTextures.end();

			for(; it != end; ++it)
			{
				it->second->ReloadAs(gpu);

#if defined(USE_FILEPATH_IN_MAP)
				WaitSetMessage(it->first.GetAbsolutePathname().c_str());
#else //#if defined(USE_FILEPATH_IN_MAP)
				WaitSetMessage(it->first.c_str());
#endif //#if defined(USE_FILEPATH_IN_MAP)
				WaitSetValue(progress++);
			}

            emit TexturesReloaded();
            
			WaitStop();
		}
	}
	LoadGPUFormat();
}

void QtMainWindow::WaitStart(const QString &title, const QString &message, int min /* = 0 */, int max /* = 100 */)
{
	waitDialog->SetRange(min, max);
	waitDialog->Show(title, message, false, false);
}

void QtMainWindow::WaitSetMessage(const QString &messsage)
{
	waitDialog->SetMessage(messsage);
}

void QtMainWindow::WaitSetValue(int value)
{
	waitDialog->SetValue(value);
}

void QtMainWindow::WaitStop()
{
	waitDialog->Reset();
}

bool QtMainWindow::eventFilter(QObject *obj, QEvent *event)
{
	QEvent::Type eventType = event->type();

	if(qApp == obj && ProjectManager::Instance()->IsOpened())
	{
		if(QEvent::ApplicationActivate == eventType)
		{
			if(QtLayer::Instance())
			{
				QtLayer::Instance()->OnResume();
				Core::Instance()->GetApplicationCore()->OnResume();
			}
		}
		else if(QEvent::ApplicationDeactivate == eventType)
		{
			if(QtLayer::Instance())
			{
				QtLayer::Instance()->OnSuspend();
				Core::Instance()->GetApplicationCore()->OnSuspend();
			}
		}
	}

	if(obj == this && QEvent::WindowUnblocked == eventType)
	{
		if(isActiveWindow())
		{
			ui->sceneTabWidget->setFocus(Qt::ActiveWindowFocusReason);
		}
	}
    
    if(obj == this && QEvent::KeyPress == eventType)
    {
        QKeyEvent *keyEvent = (QKeyEvent *)event;
        int32 keyValue = keyEvent->key();
        // check chars of russian alphabet(unicode table)
        if(keyValue >= 0x410 && keyValue <=0x44F)
        {
            // according to reference of QKeyEvent, it's impossible to get scanCode on mac os
            // so we use platform depending nativeVirtualKey()
            int32 systemKeyCode = keyEvent->nativeVirtualKey();
            int32 davaKey = DAVA::InputSystem::Instance()->GetKeyboard()->GetDavaKeyForSystemKey(systemKeyCode);
            // translate davaKey to ascii to find out real key pressed
            // offset between ascii and letters in davakey table - 29 positions
            int32 qtKey = davaKey + 29;
            QKeyEvent eventNew = QKeyEvent(QEvent::KeyPress, qtKey, keyEvent->modifiers());
            QApplication::sendEvent(obj, &eventNew);
        }
    }
    
	return QMainWindow::eventFilter(obj, event);
}

void QtMainWindow::SetupTitle()
{
	DAVA::KeyedArchive *options = DAVA::Core::Instance()->GetOptions();
	QString title = options->GetString("title").c_str();

	if(ProjectManager::Instance()->IsOpened())
	{
		title += " | Project - ";
		title += ProjectManager::Instance()->CurProjectPath().GetAbsolutePathname().c_str();
	}

	this->setWindowTitle(title);
}

void QtMainWindow::SetupMainMenu()
{
 	ui->menuDockWindows->addAction(ui->dockSceneInfo->toggleViewAction());
	ui->menuDockWindows->addAction(ui->dockLibrary->toggleViewAction());
	ui->menuDockWindows->addAction(ui->dockProperties->toggleViewAction());
	ui->menuDockWindows->addAction(ui->dockParticleEditor->toggleViewAction());
	ui->menuDockWindows->addAction(ui->dockParticleEditorTimeLine->toggleViewAction());
	ui->menuDockWindows->addAction(ui->dockSceneTree->toggleViewAction());
	ui->menuDockWindows->addAction(ui->dockConsole->toggleViewAction());
	ui->menuDockWindows->addAction(ui->dockLODEditor->toggleViewAction());
	ui->menuDockWindows->addAction(ui->dockLandscapeEditorControls->toggleViewAction());

    ui->menuDockWindows->addAction(dockActionEvent->toggleViewAction());

	InitRecent();
}


void QtMainWindow::SetupToolBars()
{
	QAction *actionMainToolBar = ui->mainToolBar->toggleViewAction();
	QAction *actionModifToolBar = ui->modificationToolBar->toggleViewAction();
	QAction *actionViewModeToolBar = ui->viewModeToolBar->toggleViewAction();
	QAction *actionLandscapeToolbar = ui->landscapeToolBar->toggleViewAction();

	ui->menuToolbars->addAction(actionMainToolBar);
	ui->menuToolbars->addAction(actionModifToolBar);
	ui->menuToolbars->addAction(actionViewModeToolBar);
	ui->menuToolbars->addAction(actionLandscapeToolbar);
	ui->menuToolbars->addAction(ui->sceneToolBar->toggleViewAction());
    ui->menuToolbars->addAction(ui->testingToolBar->toggleViewAction());

	// undo/redo
	QToolButton *undoBtn = (QToolButton *) ui->mainToolBar->widgetForAction(ui->actionUndo);
	QToolButton *redoBtn = (QToolButton *) ui->mainToolBar->widgetForAction(ui->actionRedo);
	undoBtn->setPopupMode(QToolButton::MenuButtonPopup);
	redoBtn->setPopupMode(QToolButton::MenuButtonPopup);

	// modification widget
	modificationWidget = new ModificationWidget(NULL);
	ui->modificationToolBar->insertWidget(ui->actionModifyReset, modificationWidget);

	// adding reload textures actions
	{
		QToolButton *reloadTexturesBtn = new QToolButton();
		reloadTexturesBtn->setMenu(ui->menuTexturesForGPU);
		reloadTexturesBtn->setPopupMode(QToolButton::MenuButtonPopup);
		reloadTexturesBtn->setDefaultAction(ui->actionReloadTextures);
		reloadTexturesBtn->setMaximumWidth(ResourceEditor::DEFAULT_TOOLBAR_CONTROL_SIZE_WITH_TEXT);
		reloadTexturesBtn->setMinimumWidth(ResourceEditor::DEFAULT_TOOLBAR_CONTROL_SIZE_WITH_TEXT);
		ui->mainToolBar->addSeparator();
		ui->mainToolBar->addWidget(reloadTexturesBtn);
		reloadTexturesBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		reloadTexturesBtn->setAutoRaise(false);
	}

    // adding menu for material light view mode
    {
        QToolButton *setLightViewMode = new QToolButton();
        setLightViewMode->setMenu(ui->menuLightView);
        setLightViewMode->setPopupMode(QToolButton::InstantPopup);
        setLightViewMode->setDefaultAction(ui->actionSetLightViewMode);
        ui->mainToolBar->addWidget(setLightViewMode);
        setLightViewMode->setToolButtonStyle(Qt::ToolButtonIconOnly);
        setLightViewMode->setAutoRaise(false);
    }

	//hanging objects	
	{
		HangingObjectsHeight *hangingObjectsWidget = new HangingObjectsHeight(this);
		QObject::connect(hangingObjectsWidget, SIGNAL(HeightChanged(double)), this, SLOT(OnHangingObjectsHeight(double)));

		ToolButtonWithWidget *hangingBtn = new ToolButtonWithWidget();
		hangingBtn->setDefaultAction(ui->actionHangingObjects);
		hangingBtn->SetWidget(hangingObjectsWidget);
		hangingBtn->setMaximumWidth(ResourceEditor::DEFAULT_TOOLBAR_CONTROL_SIZE_WITH_ICON);
		hangingBtn->setMinimumWidth(ResourceEditor::DEFAULT_TOOLBAR_CONTROL_SIZE_WITH_ICON);
        ui->testingToolBar->addWidget(hangingBtn);
		hangingBtn->setAutoRaise(false);
	}

	// outline by object type
	{
		objectTypesWidget = new QComboBox();
		//objectTypesWidget->setMaximumWidth(ResourceEditor::DEFAULT_TOOLBAR_CONTROL_SIZE_WITH_TEXT);
		//objectTypesWidget->setMinimumWidth(ResourceEditor::DEFAULT_TOOLBAR_CONTROL_SIZE_WITH_TEXT);

		const QList<QAction *> actions = ui->menuObjectTypes->actions();
        QActionGroup *group = new QActionGroup(ui->menuObjectTypes);

		auto endIt = actions.end();
		for(auto it = actions.begin(); it != endIt; ++it)
		{
			if((*it)->isSeparator()) continue;

			objectTypesWidget->addItem((*it)->icon(), (*it)->text());
            group->addAction(*it);
		}

		objectTypesWidget->setCurrentIndex(ResourceEditor::ESOT_NONE + 1);
		QObject::connect(objectTypesWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(OnObjectsTypeChanged(int)));

		ui->sceneToolBar->addSeparator();
		ui->sceneToolBar->addWidget(objectTypesWidget);
	}
}

void QtMainWindow::SetupStatusBar()
{
	QObject::connect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2 *)), ui->statusBar, SLOT(SceneActivated(SceneEditor2 *)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(SelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)), ui->statusBar, SLOT(SceneSelectionChanged(SceneEditor2 *, const EntityGroup *, const EntityGroup *)));
    QObject::connect(SceneSignals::Instance(), SIGNAL(CommandExecuted(SceneEditor2 *, const Command2*, bool)), ui->statusBar, SLOT(CommandExecuted(SceneEditor2 *, const Command2*, bool)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(StructureChanged(SceneEditor2 *, DAVA::Entity *)), ui->statusBar, SLOT(StructureChanged(SceneEditor2 *, DAVA::Entity *)));

	QObject::connect(this, SIGNAL(GlobalInvalidateTimeout()), ui->statusBar, SLOT(UpdateByTimer()));

	QToolButton *gizmoStatusBtn = new QToolButton();
	gizmoStatusBtn->setDefaultAction(ui->actionShowEditorGizmo);
	gizmoStatusBtn->setAutoRaise(true);
	gizmoStatusBtn->setMaximumSize(QSize(16, 16));
	ui->statusBar->insertPermanentWidget(0, gizmoStatusBtn);

    QToolButton *lighmapCanvasStatusBtn = new QToolButton();
    lighmapCanvasStatusBtn->setDefaultAction(ui->actionLightmapCanvas);
    lighmapCanvasStatusBtn->setAutoRaise(true);
    lighmapCanvasStatusBtn->setMaximumSize(QSize(16, 16));
    ui->statusBar->insertPermanentWidget(0, lighmapCanvasStatusBtn);

	QToolButton *onSceneSelectStatusBtn = new QToolButton();
	onSceneSelectStatusBtn->setDefaultAction(ui->actionOnSceneSelection);
	onSceneSelectStatusBtn->setAutoRaise(true);
	onSceneSelectStatusBtn->setMaximumSize(QSize(16, 16));
	ui->statusBar->insertPermanentWidget(0, onSceneSelectStatusBtn);

	QObject::connect(ui->sceneTabWidget->GetDavaWidget(), SIGNAL(Resized(int, int)), ui->statusBar, SLOT(OnSceneGeometryChaged(int, int)));
}


void QtMainWindow::SetupDocks()
{
	QObject::connect(ui->sceneTreeFilterClear, SIGNAL(pressed()), ui->sceneTreeFilterEdit, SLOT(clear()));
	QObject::connect(ui->sceneTreeFilterEdit, SIGNAL(textChanged(const QString &)), ui->sceneTree, SLOT(SetFilter(const QString &)));

    QObject::connect(ui->sceneTabWidget, SIGNAL(CloseTabRequest(int , Request *)), this, SLOT(OnCloseTabRequest(int, Request *)));
    
	QObject::connect(this, SIGNAL(GlobalInvalidateTimeout()), ui->sceneInfo , SLOT(UpdateInfoByTimer()));
	QObject::connect(this, SIGNAL(TexturesReloaded()), ui->sceneInfo , SLOT(TexturesReloaded()));
	QObject::connect(this, SIGNAL(SpritesReloaded()), ui->sceneInfo , SLOT(SpritesReloaded()));
    
    ui->libraryWidget->SetupSignals();
    // Run Action Event dock
    {
        dockActionEvent = new QDockWidget("Run Action Event", this);
        dockActionEvent->setWidget(new RunActionEventWidget());
        dockActionEvent->setObjectName(QString( "dock_%1" ).arg(dockActionEvent->widget()->objectName()));
        addDockWidget(Qt::RightDockWidgetArea, dockActionEvent);
    }
    
	ui->dockProperties->Init();
}

void QtMainWindow::SetupActions()
{
	// scene file actions
	QObject::connect(ui->actionOpenProject, SIGNAL(triggered()), this, SLOT(OnProjectOpen()));
    QObject::connect(ui->actionCloseProject, SIGNAL(triggered()), this, SLOT(OnProjectClose()));
	QObject::connect(ui->actionOpenScene, SIGNAL(triggered()), this, SLOT(OnSceneOpen()));
	QObject::connect(ui->actionNewScene, SIGNAL(triggered()), this, SLOT(OnSceneNew()));
	QObject::connect(ui->actionSaveScene, SIGNAL(triggered()), this, SLOT(OnSceneSave()));
	QObject::connect(ui->actionSaveSceneAs, SIGNAL(triggered()), this, SLOT(OnSceneSaveAs()));
	QObject::connect(ui->actionSaveToFolder, SIGNAL(triggered()), this, SLOT(OnSceneSaveToFolder()));

	QObject::connect(ui->menuFile, SIGNAL(triggered(QAction *)), this, SLOT(OnRecentTriggered(QAction *)));

	// export
	QObject::connect(ui->menuExport, SIGNAL(triggered(QAction *)), this, SLOT(ExportMenuTriggered(QAction *)));
    ui->actionExportPVRIOS->setData(GPU_POWERVR_IOS);
	ui->actionExportPVRAndroid->setData(GPU_POWERVR_ANDROID);
	ui->actionExportTegra->setData(GPU_TEGRA);
	ui->actionExportMali->setData(GPU_MALI);
	ui->actionExportAdreno->setData(GPU_ADRENO);
	ui->actionExportPNG->setData(GPU_PNG);
	
	// import
#ifdef __DAVAENGINE_SPEEDTREE__
    QObject::connect(ui->actionImportSpeedTreeXML, SIGNAL(triggered()), this, SLOT(OnImportSpeedTreeXML()));
#endif //__DAVAENGINE_SPEEDTREE__

	// reload
	ui->actionReloadPoverVRIOS->setData(GPU_POWERVR_IOS);
	ui->actionReloadPoverVRAndroid->setData(GPU_POWERVR_ANDROID);
	ui->actionReloadTegra->setData(GPU_TEGRA);
	ui->actionReloadMali->setData(GPU_MALI);
	ui->actionReloadAdreno->setData(GPU_ADRENO);
	ui->actionReloadPNG->setData(GPU_PNG);
	QObject::connect(ui->menuTexturesForGPU, SIGNAL(triggered(QAction *)), this, SLOT(OnReloadTexturesTriggered(QAction *)));
	QObject::connect(ui->actionReloadTextures, SIGNAL(triggered()), this, SLOT(OnReloadTextures()));
	QObject::connect(ui->actionReloadSprites, SIGNAL(triggered()), this, SLOT(OnReloadSprites()));

    QObject::connect(ui->actionAlbedo, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
    QObject::connect(ui->actionAmbient, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
    QObject::connect(ui->actionDiffuse, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
    QObject::connect(ui->actionSpecular, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
	
	QObject::connect(ui->actionShowEditorGizmo, SIGNAL(toggled(bool)), this, SLOT(OnEditorGizmoToggle(bool)));
    QObject::connect(ui->actionLightmapCanvas, SIGNAL(toggled(bool)), this, SLOT(OnViewLightmapCanvas(bool)));
	QObject::connect(ui->actionOnSceneSelection, SIGNAL(toggled(bool)), this, SLOT(OnAllowOnSceneSelectionToggle(bool)));

	// scene undo/redo
	QObject::connect(ui->actionUndo, SIGNAL(triggered()), this, SLOT(OnUndo()));
	QObject::connect(ui->actionRedo, SIGNAL(triggered()), this, SLOT(OnRedo()));

    // quality
    QObject::connect(ui->actionCustomQuality, SIGNAL(triggered()), this, SLOT(OnCustomQuality()));

	// scene modifications
	QObject::connect(ui->actionModifySelect, SIGNAL(triggered()), this, SLOT(OnSelectMode()));
	QObject::connect(ui->actionModifyMove, SIGNAL(triggered()), this, SLOT(OnMoveMode()));
	QObject::connect(ui->actionModifyRotate, SIGNAL(triggered()), this, SLOT(OnRotateMode()));
	QObject::connect(ui->actionModifyScale, SIGNAL(triggered()), this, SLOT(OnScaleMode()));
	QObject::connect(ui->actionPivotCenter, SIGNAL(triggered()), this, SLOT(OnPivotCenterMode()));
	QObject::connect(ui->actionPivotCommon, SIGNAL(triggered()), this, SLOT(OnPivotCommonMode()));
	QObject::connect(ui->actionManualModifMode, SIGNAL(triggered()), this, SLOT(OnManualModifMode()));
	QObject::connect(ui->actionModifyPlaceOnLandscape, SIGNAL(triggered()), this, SLOT(OnPlaceOnLandscape()));
	QObject::connect(ui->actionModifySnapToLandscape, SIGNAL(triggered()), this, SLOT(OnSnapToLandscape()));
	QObject::connect(ui->actionModifyReset, SIGNAL(triggered()), this, SLOT(OnResetTransform()));
	QObject::connect(ui->actionLockTransform, SIGNAL(triggered()), this, SLOT(OnLockTransform()));
	QObject::connect(ui->actionUnlockTransform, SIGNAL(triggered()), this, SLOT(OnUnlockTransform()));
	QObject::connect(ui->actionCenterPivotPoint, SIGNAL(triggered()), this, SLOT(OnCenterPivotPoint()));
	QObject::connect(ui->actionZeroPivotPoint, SIGNAL(triggered()), this, SLOT(OnZeroPivotPoint()));

	// tools
	QObject::connect(ui->actionMaterialEditor, SIGNAL(triggered()), this, SLOT(OnMaterialEditor()));
	QObject::connect(ui->actionTextureConverter, SIGNAL(triggered()), this, SLOT(OnTextureBrowser()));
	QObject::connect(ui->actionEnableCameraLight, SIGNAL(triggered()), this, SLOT(OnSceneLightMode()));
	QObject::connect(ui->actionCubemapEditor, SIGNAL(triggered()), this, SLOT(OnCubemapEditor()));
    QObject::connect(ui->actionImageSplitter, SIGNAL(triggered()), this, SLOT(OnImageSplitter()));

	QObject::connect(ui->actionShowNotPassableLandscape, SIGNAL(triggered()), this, SLOT(OnNotPassableTerrain()));
	QObject::connect(ui->actionCustomColorsEditor, SIGNAL(triggered()), this, SLOT(OnCustomColorsEditor()));
	QObject::connect(ui->actionHeightMapEditor, SIGNAL(triggered()), this, SLOT(OnHeightmapEditor()));
	QObject::connect(ui->actionTileMapEditor, SIGNAL(triggered()), this, SLOT(OnTilemaskEditor()));
	QObject::connect(ui->actionVisibilityCheckTool, SIGNAL(triggered()), this, SLOT(OnVisibilityTool()));
	QObject::connect(ui->actionRulerTool, SIGNAL(triggered()), this, SLOT(OnRulerTool()));
    QObject::connect(ui->actionGrasEditor, SIGNAL(triggered()), this, SLOT(OnGrasEditor()));

	QObject::connect(ui->actionLight, SIGNAL(triggered()), this, SLOT(OnLightDialog()));
	QObject::connect(ui->actionCamera, SIGNAL(triggered()), this, SLOT(OnCameraDialog()));
	QObject::connect(ui->actionAddEmptyEntity, SIGNAL(triggered()), this, SLOT(OnEmptyEntity()));
	QObject::connect(ui->actionUserNode, SIGNAL(triggered()), this, SLOT(OnUserNodeDialog()));
	QObject::connect(ui->actionSwitchNode, SIGNAL(triggered()), this, SLOT(OnSwitchEntityDialog()));
	QObject::connect(ui->actionParticleEffectNode, SIGNAL(triggered()), this, SLOT(OnParticleEffectDialog()));
    QObject::connect(ui->actionEditor_2D_Camera, SIGNAL(triggered()), this, SLOT(On2DCameraDialog()));
    QObject::connect(ui->actionEditor_Sprite, SIGNAL(triggered()), this, SLOT(On2DSpriteDialog()));
	QObject::connect(ui->actionAddNewEntity, SIGNAL(triggered()), this, SLOT(OnAddEntityFromSceneTree()));
	QObject::connect(ui->actionRemoveEntity, SIGNAL(triggered()), ui->sceneTree, SLOT(RemoveSelection()));
	QObject::connect(ui->actionExpandSceneTree, SIGNAL(triggered()), ui->sceneTree, SLOT(expandAll()));
	QObject::connect(ui->actionCollapseSceneTree, SIGNAL(triggered()), ui->sceneTree, SLOT(CollapseAll()));
    QObject::connect(ui->actionAddLandscape, SIGNAL(triggered()), this, SLOT(OnAddLandscape()));
    QObject::connect(ui->actionAddSkybox, SIGNAL(triggered()), this, SLOT(OnAddSkybox()));
	QObject::connect(ui->actionAddWind, SIGNAL(triggered()), this, SLOT(OnAddWindEntity()));
    QObject::connect(ui->actionAddVegetation, SIGNAL(triggered()), this, SLOT(OnAddVegetation()));
			
	QObject::connect(ui->actionShowSettings, SIGNAL(triggered()), this, SLOT(OnShowSettings()));
	
	QObject::connect(ui->actionSetShadowColor, SIGNAL(triggered()), this, SLOT(OnSetShadowColor()));
	QObject::connect(ui->actionDynamicBlendModeAlpha, SIGNAL(triggered()), this, SLOT(OnShadowBlendModeAlpha()));
	QObject::connect(ui->actionDynamicBlendModeMultiply, SIGNAL(triggered()), this, SLOT(OnShadowBlendModeMultiply()));
	QObject::connect(ui->menuDynamicShadowBlendMode, SIGNAL(aboutToShow()), this, SLOT(OnShadowBlendModeWillShow()));

    
	QObject::connect(ui->actionSaveHeightmapToPNG, SIGNAL(triggered()), this, SLOT(OnSaveHeightmapToPNG()));
	QObject::connect(ui->actionSaveTiledTexture, SIGNAL(triggered()), this, SLOT(OnSaveTiledTexture()));
	
	QObject::connect(ui->actionConvertModifiedTextures, SIGNAL(triggered()), this, SLOT(OnConvertModifiedTextures()));
    
#if defined(__DAVAENGINE_BEAST__)
	QObject::connect(ui->actionBeastAndSave, SIGNAL(triggered()), this, SLOT(OnBeastAndSave()));
#else
	//ui->menuScene->removeAction(ui->menuBeast->menuAction());
#endif //#if defined(__DAVAENGINE_BEAST__)
    
    QObject::connect(ui->actionBuildStaticOcclusion, SIGNAL(triggered()), this, SLOT(OnBuildStaticOcclusion()));
    QObject::connect(ui->actionRebuildCurrentOcclusionCell, SIGNAL(triggered()), this, SLOT(OnRebuildCurrentOcclusionCell()));
    QObject::connect(ui->actionInvalidateStaticOcclusion, SIGNAL(triggered()), this, SLOT(OnInavalidateStaticOcclusion()));
    
	//Help
    QObject::connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(OnOpenHelp()));

	//Landscape editors toggled
	QObject::connect(SceneSignals::Instance(), SIGNAL(VisibilityToolToggled(SceneEditor2*)), this, SLOT(OnLandscapeEditorToggled(SceneEditor2*)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(CustomColorsToggled(SceneEditor2*)), this, SLOT(OnLandscapeEditorToggled(SceneEditor2*)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(HeightmapEditorToggled(SceneEditor2*)), this, SLOT(OnLandscapeEditorToggled(SceneEditor2*)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(TilemaskEditorToggled(SceneEditor2*)), this, SLOT(OnLandscapeEditorToggled(SceneEditor2*)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(RulerToolToggled(SceneEditor2*)), this, SLOT(OnLandscapeEditorToggled(SceneEditor2*)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(NotPassableTerrainToggled(SceneEditor2*)), this, SLOT(OnLandscapeEditorToggled(SceneEditor2*)));

	QObject::connect(SceneSignals::Instance(), SIGNAL(SnapToLandscapeChanged(SceneEditor2*, bool)),
					 this, SLOT(OnSnapToLandscapeChanged(SceneEditor2*, bool)));

    // Debug functions
	QObject::connect(ui->actionGridCopy, SIGNAL(triggered()), developerTools, SLOT(OnDebugFunctionsGridCopy()));
	{
#ifdef USER_VERSIONING_DEBUG_FEATURES
        QAction *act = ui->menuDebug_Functions->addAction("Edit version tags");
        connect(act, SIGNAL(triggered()), SLOT(DebugVersionInfo()));
#endif
	}
    
 	//Collision Box Types
    objectTypesLabel = new QtLabelWithActions();
 	objectTypesLabel->setMenu(ui->menuObjectTypes);
 	objectTypesLabel->setDefaultAction(ui->actionNoObject);
	
    ui->sceneTabWidget->AddToolWidget(objectTypesLabel);

	ui->actionObjectTypesOff->setData(ResourceEditor::ESOT_NONE);
	ui->actionNoObject->setData(ResourceEditor::ESOT_NO_COLISION);
	ui->actionTree->setData(ResourceEditor::ESOT_TREE);
	ui->actionBush->setData(ResourceEditor::ESOT_BUSH);
	ui->actionFragileProj->setData(ResourceEditor::ESOT_FRAGILE_PROJ);
	ui->actionFragileProjInv->setData(ResourceEditor::ESOT_FRAGILE_PROJ_INV);
	ui->actionFalling->setData(ResourceEditor::ESOT_FALLING);
	ui->actionBuilding->setData(ResourceEditor::ESOT_BUILDING);
	ui->actionInvisibleWall->setData(ResourceEditor::ESOT_INVISIBLE_WALL);
    ui->actionSpeedTree->setData(ResourceEditor::ESOT_SPEED_TREE);
	
    QObject::connect(ui->menuObjectTypes, SIGNAL(triggered(QAction *)), this, SLOT(OnObjectsTypeChanged(QAction *)));
	QObject::connect(ui->actionHangingObjects, SIGNAL(triggered()), this, SLOT(OnHangingObjects()));
	QObject::connect(ui->actionReloadShader, SIGNAL(triggered()), this, SLOT(OnReloadShaders()));
    QObject::connect(ui->actionSwitchesWithDifferentLODs, SIGNAL(triggered(bool)), this, SLOT(OnSwitchWithDifferentLODs(bool)));
}

void QtMainWindow::SetupShortCuts()
{
	// select mode
	QObject::connect(ui->sceneTabWidget, SIGNAL(Escape()), this, SLOT(OnSelectMode()));
	
	// look at
	QObject::connect(new QShortcut(QKeySequence(Qt::Key_Z), this), SIGNAL(activated()), ui->sceneTree, SLOT(LookAtSelection()));
	
	// delete
	QObject::connect(new QShortcut(QKeySequence(Qt::Key_Delete), this), SIGNAL(activated()), ui->sceneTree, SLOT(RemoveSelection()));
	QObject::connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Backspace), this), SIGNAL(activated()), ui->sceneTree, SLOT(RemoveSelection()));

	// camera speed
	QObject::connect(new QShortcut(QKeySequence(Qt::Key_1), ui->sceneTabWidget), SIGNAL(activated()), this, SLOT(OnCameraSpeed0()));
	QObject::connect(new QShortcut(QKeySequence(Qt::Key_2), ui->sceneTabWidget), SIGNAL(activated()), this, SLOT(OnCameraSpeed1()));
	QObject::connect(new QShortcut(QKeySequence(Qt::Key_3), ui->sceneTabWidget), SIGNAL(activated()), this, SLOT(OnCameraSpeed2()));
	QObject::connect(new QShortcut(QKeySequence(Qt::Key_4), ui->sceneTabWidget), SIGNAL(activated()), this, SLOT(OnCameraSpeed3()));
	QObject::connect(new QShortcut(QKeySequence(Qt::Key_T), ui->sceneTabWidget), SIGNAL(activated()), this, SLOT(OnCameraLookFromTop()));

	// scene tree collapse/expand
	QObject::connect(new QShortcut(QKeySequence(Qt::Key_X), ui->sceneTree), SIGNAL(activated()), ui->sceneTree, SLOT(CollapseSwitch()));
	
	//tab closing
	QObject::connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), ui->sceneTabWidget), SIGNAL(activated()), ui->sceneTabWidget, SLOT(TabBarCloseCurrentRequest()));
#if defined (__DAVAENGINE_WIN32__)
	QObject::connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F4), ui->sceneTabWidget), SIGNAL(activated()), ui->sceneTabWidget, SLOT(TabBarCloseCurrentRequest()));
#endif
}

void QtMainWindow::InitRecent()
{
	Vector<String> filesList = RecentFilesManager::Instance()->GetRecentFiles();

	foreach(String path, filesList)
	{
		if (path.empty())
		{
			continue;
		}
		QAction *action = ui->menuFile->addAction(path.c_str());

		action->setData(QString(path.c_str()));
		recentScenes.push_back(action);
	}
}

void QtMainWindow::AddRecent(const QString &pathString)
{
    while(recentScenes.size())
    {
        ui->menuFile->removeAction(recentScenes[0]);
        recentScenes.removeAt(0);
    }
    
	RecentFilesManager::Instance()->SetFileToRecent(pathString.toStdString());
	InitRecent();
}

// ###################################################################################################
// Scene signals
// ###################################################################################################

void QtMainWindow::ProjectOpened(const QString &path)
{
	EditorConfig::Instance()->ParseConfig(ProjectManager::Instance()->CurProjectPath() + "EditorConfig.yaml");

	EnableProjectActions(true);
	SetupTitle();
}

void QtMainWindow::ProjectClosed()
{
	EnableProjectActions(false);
	SetupTitle();
}

void QtMainWindow::SceneActivated(SceneEditor2 *scene)
{
	EnableSceneActions(true);

	LoadViewState(scene);
	LoadUndoRedoState(scene);
	LoadModificationState(scene);
	LoadEditorLightState(scene);
	LoadShadowBlendModeState(scene);
	LoadLandscapeEditorState(scene);
	LoadObjectTypes(scene);
	LoadHangingObjects(scene);

    OnMaterialLightViewChanged(true);
    OnViewLightmapCanvas(true);

	int32 tools = scene->GetEnabledTools();
	UpdateConflictingActionsState(tools == 0);
    UpdateModificationActionsState();

    ui->actionSwitchesWithDifferentLODs->setChecked(scene->debugDrawSystem->SwithcesWithDifferentLODsModeEnabled());

    if(NULL != scene)
    {
        EntityGroup curSelection = scene->selectionSystem->GetSelection();
        SceneSelectionChanged(scene, &curSelection, NULL);
    }
}

void QtMainWindow::SceneDeactivated(SceneEditor2 *scene)
{
	// block some actions, when there is no scene
	EnableSceneActions(false);
}

void QtMainWindow::SceneSelectionChanged(SceneEditor2 *scene, const EntityGroup *selected, const EntityGroup *deselected)
{
    UpdateModificationActionsState();
}

void QtMainWindow::EnableProjectActions(bool enable)
{
	ui->actionNewScene->setEnabled(enable);
	ui->actionOpenScene->setEnabled(enable);
	ui->actionCubemapEditor->setEnabled(enable);
    ui->actionImageSplitter->setEnabled(enable);
	ui->dockLibrary->setEnabled(enable);
    ui->actionCloseProject->setEnabled(enable);
    
    auto endIt = recentScenes.end();
    for(auto it = recentScenes.begin(); it != endIt; ++it)
    {
        (*it)->setEnabled(enable);
    }
}

void QtMainWindow::EnableSceneActions(bool enable)
{
	ui->actionUndo->setEnabled(enable);
	ui->actionRedo->setEnabled(enable);

	ui->dockLODEditor->setEnabled(enable);
	ui->dockProperties->setEnabled(enable);
	ui->dockSceneTree->setEnabled(enable);
	ui->dockSceneInfo->setEnabled(enable);

	ui->actionSaveScene->setEnabled(enable);
	ui->actionSaveSceneAs->setEnabled(enable);
	ui->actionSaveToFolder->setEnabled(enable);

	ui->actionModifySelect->setEnabled(enable);
	ui->actionModifyMove->setEnabled(enable);
	ui->actionModifyReset->setEnabled(enable);
	ui->actionModifyRotate->setEnabled(enable);
	ui->actionModifyScale->setEnabled(enable);
	ui->actionModifyPlaceOnLandscape->setEnabled(enable);
	ui->actionModifySnapToLandscape->setEnabled(enable);
	ui->actionConvertToShadow->setEnabled(enable);
	ui->actionPivotCenter->setEnabled(enable);
	ui->actionPivotCommon->setEnabled(enable);
	ui->actionCenterPivotPoint->setEnabled(enable);
	ui->actionZeroPivotPoint->setEnabled(enable);
	ui->actionManualModifMode->setEnabled(enable);

    if(modificationWidget)
        modificationWidget->setEnabled(enable);

	ui->actionTextureConverter->setEnabled(enable);
	ui->actionMaterialEditor->setEnabled(enable);
	ui->actionHeightMapEditor->setEnabled(enable);
	ui->actionTileMapEditor->setEnabled(enable);
	ui->actionShowNotPassableLandscape->setEnabled(enable);
	ui->actionRulerTool->setEnabled(enable);
	ui->actionVisibilityCheckTool->setEnabled(enable);
	ui->actionCustomColorsEditor->setEnabled(enable);
    ui->actionGrasEditor->setEnabled(enable);

	ui->actionEnableCameraLight->setEnabled(enable);
	ui->actionReloadTextures->setEnabled(enable);
	ui->actionReloadSprites->setEnabled(enable);
    ui->actionSetLightViewMode->setEnabled(enable);

	ui->actionSaveHeightmapToPNG->setEnabled(enable);
	ui->actionSaveTiledTexture->setEnabled(enable);

	ui->actionBeastAndSave->setEnabled(enable);

	ui->actionDynamicBlendModeAlpha->setEnabled(enable);
	ui->actionDynamicBlendModeMultiply->setEnabled(enable);
	ui->actionSetShadowColor->setEnabled(enable);

	ui->actionHangingObjects->setEnabled(enable);

	ui->menuExport->setEnabled(enable);
	ui->menuEdit->setEnabled(enable);
	ui->menuCreateNode->setEnabled(enable);
	ui->menuScene->setEnabled(enable);
    ui->menuLightView->setEnabled(enable);
    ui->menuTexturesForGPU->setEnabled(enable);
    
    ui->sceneToolBar->setEnabled(enable);
	ui->actionConvertModifiedTextures->setEnabled(enable);
    
    ui->actionReloadShader->setEnabled(enable);
    ui->actionSwitchesWithDifferentLODs->setEnabled(enable);
}

void QtMainWindow::UpdateModificationActionsState()
{
    bool canModify = false;
    bool isMultiple = false;

    SceneEditor2 *scene = GetCurrentScene();
    if(NULL != scene)
    {
        EntityGroup selection = scene->selectionSystem->GetSelection();
        canModify = scene->modifSystem->ModifCanStart(selection);
        isMultiple = (selection.Size() > 1);
    }

    ui->actionModifyReset->setEnabled(canModify);
    ui->actionModifyPlaceOnLandscape->setEnabled(canModify);

    ui->actionCenterPivotPoint->setEnabled(canModify && !isMultiple);
    ui->actionZeroPivotPoint->setEnabled(canModify && !isMultiple);

    modificationWidget->setEnabled(canModify);
}

void QtMainWindow::SceneCommandExecuted(SceneEditor2 *scene, const Command2* command, bool redo)
{
	if(scene == GetCurrentScene())
	{
		LoadUndoRedoState(scene);
        UpdateModificationActionsState();
	}
}

// ###################################################################################################
// Mainwindow Qt actions
// ###################################################################################################

void QtMainWindow::OnProjectOpen()
{
    FilePath incomePath = ProjectManager::Instance()->ProjectOpenDialog();
    
    if(!incomePath.IsEmpty() &&
       ProjectManager::Instance()->CurProjectPath() != incomePath &&
       ui->sceneTabWidget->CloseAllTabs())
    {
        ProjectManager::Instance()->ProjectOpen(incomePath);
    }
}

void QtMainWindow::OnProjectClose()
{
    if(ui->sceneTabWidget->CloseAllTabs())
    {
        ProjectManager::Instance()->ProjectClose();
    }
}

void QtMainWindow::OnSceneNew()
{
	ui->sceneTabWidget->OpenTab();
}

void QtMainWindow::OnSceneOpen()
{
	QString path = QtFileDialog::getOpenFileName(this, "Open scene file", ProjectManager::Instance()->CurProjectDataSourcePath().GetAbsolutePathname().c_str(), "DAVA Scene V2 (*.sc2)");
	OpenScene(path);
}

void QtMainWindow::OnSceneSave()
{
	if (!IsSavingAllowed())
	{
		return;
	}

	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		SaveScene(scene);
	}
}

void QtMainWindow::OnSceneSaveAs()
{
	if (!IsSavingAllowed())
	{
		return;
	}

	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		SaveSceneAs(scene);
	}
}

void QtMainWindow::OnSceneSaveToFolder()
{
	if (!IsSavingAllowed())
	{
		return;
	}

	SceneEditor2* scene = GetCurrentScene();
	if(!scene) return;

	FilePath scenePathname = scene->GetScenePath();
	if(scenePathname.IsEmpty() || scenePathname.GetType() == FilePath::PATH_IN_MEMORY || !scene->IsLoaded())
	{
		ShowErrorDialog("Can't save not saved scene.");
		return;
	}

	QString path = QtFileDialog::getExistingDirectory(NULL, QString("Open Folder"), QString("/"));
	if(path.isEmpty())
		return;


	WaitStart("Save with Children", "Please wait...");


	FilePath folder = PathnameToDAVAStyle(path);
	folder.MakeDirectoryPathname();

	SceneSaver sceneSaver;
	sceneSaver.SetInFolder(scene->GetScenePath().GetDirectory());
	sceneSaver.SetOutFolder(folder);

	Set<String> errorsLog;

	SceneEditor2 *sceneForSaving = scene->CreateCopyForExport();
	sceneSaver.SaveScene(sceneForSaving, scene->GetScenePath(), errorsLog);
	sceneForSaving->Release();

	WaitStop();

	ShowErrorDialog(errorsLog);
}

void QtMainWindow::OnCloseTabRequest(int tabIndex, Request *closeRequest)
{
    SceneEditor2 *scene = ui->sceneTabWidget->GetTabScene(tabIndex);
    if(!scene)
    {
        closeRequest->Accept();
        return;
    }

	int32 toolsFlags = scene->GetEnabledTools();
	if (!scene->IsChanged())
	{
		if (toolsFlags)
		{
			scene->DisableTools(SceneEditor2::LANDSCAPE_TOOLS_ALL);
		}
		closeRequest->Accept();
        return;
	}

    if ( !IsSavingAllowed() )
    {
        closeRequest->Cancel();
        return;
    }

    int answer = QMessageBox::warning(NULL, "Scene was changed", "Do you want to save changes, made to scene?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);
    if(answer == QMessageBox::Cancel)
    {
        closeRequest->Cancel();
        return;
    }

	if(answer == QMessageBox::No)
	{
		if (toolsFlags)
		{
			scene->DisableTools(SceneEditor2::LANDSCAPE_TOOLS_ALL, false);
		}
		closeRequest->Accept();
		return;
	}
	
	if (toolsFlags)
	{
		FilePath colorSystemTexturePath = scene->customColorsSystem->GetCurrentSaveFileName();
		if( (toolsFlags & SceneEditor2::LANDSCAPE_TOOL_CUSTOM_COLOR) &&
		    (colorSystemTexturePath.IsEmpty() || !colorSystemTexturePath.Exists()) &&
		    !SelectCustomColorsTexturePath())
		{
			closeRequest->Cancel();
			return;
		}
		
		scene->DisableTools(SceneEditor2::LANDSCAPE_TOOLS_ALL, true);
	}

    if(!SaveScene(scene))
    {
        closeRequest->Cancel();
        return;
    }
	
    closeRequest->Accept();
}


void QtMainWindow::ExportMenuTriggered(QAction *exportAsAction)
{
	SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;

	if (!SaveTilemask(false))
	{
		return;
	}
    
	WaitStart("Export", "Please wait...");

    eGPUFamily gpuFamily = (eGPUFamily)exportAsAction->data().toInt();
    if (!scene->Export(gpuFamily))
    {
        QMessageBox::warning(this, "Export error", "An error occurred while exporting the scene. See log for more info.", QMessageBox::Ok);
    }

	WaitStop();
}

void QtMainWindow::OnImportSpeedTreeXML()
{
#ifdef __DAVAENGINE_SPEEDTREE__
    SpeedTreeImportDialog importDialog(this);
    importDialog.exec();
#endif //__DAVAENGINE_SPEEDTREE__
}

void QtMainWindow::OnRecentTriggered(QAction *recentAction)
{
	if(recentScenes.contains(recentAction))
	{
		QString path = recentAction->data().toString();
        OpenScene(path);
	}
}

void QtMainWindow::OnUndo()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		scene->Undo();
	}
}

void QtMainWindow::OnRedo()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		scene->Redo();
	}
}

void QtMainWindow::OnEditorGizmoToggle(bool show)
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		scene->SetHUDVisible(show);
	}
}

void QtMainWindow::OnViewLightmapCanvas(bool show)
{
    bool showCanvas = ui->actionLightmapCanvas->isChecked();
    if(showCanvas != SettingsManager::GetValue(Settings::Internal_MaterialsShowLightmapCanvas).AsBool())
    {
        SettingsManager::SetValue(Settings::Internal_MaterialsShowLightmapCanvas, DAVA::VariantType(showCanvas));
    }

    if(NULL != GetCurrentScene())
    {
        GetCurrentScene()->materialSystem->SetLightmapCanvasVisible(showCanvas);
    }
}

void QtMainWindow::OnAllowOnSceneSelectionToggle(bool allow)
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		scene->selectionSystem->SetSelectionAllowed(allow);
	}
}

void QtMainWindow::OnReloadTextures()
{
	SetGPUFormat(GetGPUFormat());
}

void QtMainWindow::OnReloadTexturesTriggered(QAction *reloadAction)
{
	DAVA::eGPUFamily gpu = (DAVA::eGPUFamily) reloadAction->data().toInt();
	if(gpu >= 0 && gpu < DAVA::GPU_FAMILY_COUNT)
	{
		SetGPUFormat(gpu);
	}
}

void QtMainWindow::OnReloadSprites()
{
    SpritePackerHelper::Instance()->UpdateParticleSprites(GetGPUFormat());
	emit SpritesReloaded();
}

void QtMainWindow::OnSelectMode()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		scene->modifSystem->SetModifMode(ST_MODIF_OFF);
		LoadModificationState(scene);
	}
}

void QtMainWindow::OnMoveMode()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		scene->modifSystem->SetModifMode(ST_MODIF_MOVE);
		LoadModificationState(scene);
	}
}

void QtMainWindow::OnRotateMode()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		scene->modifSystem->SetModifMode(ST_MODIF_ROTATE);
		LoadModificationState(scene);
	}
}

void QtMainWindow::OnScaleMode()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		scene->modifSystem->SetModifMode(ST_MODIF_SCALE);
		LoadModificationState(scene);
	}
}

void QtMainWindow::OnPivotCenterMode()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		scene->selectionSystem->SetPivotPoint(ST_PIVOT_ENTITY_CENTER);
		LoadModificationState(scene);
	}
}

void QtMainWindow::OnPivotCommonMode()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		scene->selectionSystem->SetPivotPoint(ST_PIVOT_COMMON_CENTER);
		LoadModificationState(scene);
	}
}

void QtMainWindow::OnManualModifMode()
{
	if(ui->actionManualModifMode->isChecked())
	{
		modificationWidget->SetPivotMode(ModificationWidget::PivotRelative);
	}
	else
	{
		modificationWidget->SetPivotMode(ModificationWidget::PivotAbsolute);
	}
}

void QtMainWindow::OnPlaceOnLandscape()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		Entity *landscapeEntity = FindLandscapeEntity(scene);
		if (landscapeEntity == NULL || GetLandscape(landscapeEntity) == NULL)
		{
			ShowErrorDialog(ResourceEditor::NO_LANDSCAPE_ERROR_MESSAGE);
			return;
		}

		EntityGroup selection = scene->selectionSystem->GetSelection();
		scene->modifSystem->PlaceOnLandscape(selection);
	}
}

void QtMainWindow::OnSnapToLandscape()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		Entity *landscapeEntity = FindLandscapeEntity(scene);
		if (landscapeEntity == NULL || GetLandscape(landscapeEntity) == NULL)
		{
			ShowErrorDialog(ResourceEditor::NO_LANDSCAPE_ERROR_MESSAGE);
			ui->actionModifySnapToLandscape->setChecked(false);
			return;
		}

		scene->modifSystem->SetLandscapeSnap(ui->actionModifySnapToLandscape->isChecked());
		LoadModificationState(scene);
	}
}

void QtMainWindow::OnResetTransform()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		EntityGroup selection = scene->selectionSystem->GetSelection();
		scene->modifSystem->ResetTransform(selection);
	}
}

void QtMainWindow::OnLockTransform()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		EntityGroup selection = scene->selectionSystem->GetSelection();
		scene->modifSystem->LockTransform(selection, true);
	}

    UpdateModificationActionsState();
}

void QtMainWindow::OnUnlockTransform()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		EntityGroup selection = scene->selectionSystem->GetSelection();
		scene->modifSystem->LockTransform(selection, false);
	}

    UpdateModificationActionsState();
}

void QtMainWindow::OnCenterPivotPoint()
{
    SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
	if(NULL != curScene)
	{
		EntityGroup selection = curScene->selectionSystem->GetSelection();
		curScene->modifSystem->MovePivotCenter(selection);
    }
}

void QtMainWindow::OnZeroPivotPoint()
{
    SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
	if(NULL != curScene)
	{
		EntityGroup selection = curScene->selectionSystem->GetSelection();
		curScene->modifSystem->MovePivotZero(selection);
    }
}

void QtMainWindow::OnMaterialEditor()
{ 
	MaterialEditor::Instance()->showNormal();
}

void QtMainWindow::OnTextureBrowser()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	EntityGroup selectedEntities;

	if(NULL != sceneEditor)
	{
		selectedEntities = sceneEditor->selectionSystem->GetSelection();
	}

	TextureBrowser::Instance()->showNormal();
	TextureBrowser::Instance()->sceneActivated(sceneEditor);
	TextureBrowser::Instance()->sceneSelectionChanged(sceneEditor, &selectedEntities, NULL); 
}

void QtMainWindow::OnSceneLightMode()
{
	SceneEditor2* scene = GetCurrentScene();
	if(NULL != scene)
	{
		if(ui->actionEnableCameraLight->isChecked())
		{
			scene->editorLightSystem->SetCameraLightEnabled(true);
		}
		else
		{
			scene->editorLightSystem->SetCameraLightEnabled(false);
		}

		LoadEditorLightState(scene);
	}
}

void QtMainWindow::OnCubemapEditor()
{
	SceneEditor2* scene = GetCurrentScene();
	
	CubeMapTextureBrowser dlg(scene, dynamic_cast<QWidget*>(parent()));
	dlg.exec();
}

void QtMainWindow::OnImageSplitter()
{
	ImageSplitterDialog dlg(this);
	dlg.exec();
}

void QtMainWindow::OnSwitchEntityDialog()
{
	if(NULL != addSwitchEntityDialog)
	{
		return;
	}
	addSwitchEntityDialog = new AddSwitchEntityDialog( this);
	addSwitchEntityDialog->show();
	connect(addSwitchEntityDialog, SIGNAL(finished(int)), this, SLOT(UnmodalDialogFinished(int)));
}


void QtMainWindow::UnmodalDialogFinished(int)
{
	QObject* sender = QObject::sender();
	disconnect(sender, SIGNAL(finished(int)), this, SLOT(UnmodalDialogFinished(int)));
	if(sender == addSwitchEntityDialog)
	{
		addSwitchEntityDialog = NULL;
	}
}

void QtMainWindow::OnAddLandscape()
{
    Entity* entityToProcess = new Entity();
    entityToProcess->SetName(ResourceEditor::LANDSCAPE_NODE_NAME);
    entityToProcess->SetLocked(true);
    Landscape* newLandscape = new Landscape();
    RenderComponent* component = new RenderComponent();
    component->SetRenderObject(newLandscape);
	newLandscape->Release();
    entityToProcess->AddComponent(component);

    AABBox3 bboxForLandscape;
    float32 defaultLandscapeSize = 600.0f;
    float32 defaultLandscapeHeight = 50.0f;
    
    bboxForLandscape.AddPoint(Vector3(-defaultLandscapeSize/2.f, -defaultLandscapeSize/2.f, 0.f));
    bboxForLandscape.AddPoint(Vector3(defaultLandscapeSize/2.f, defaultLandscapeSize/2.f, defaultLandscapeHeight));
    newLandscape->BuildLandscapeFromHeightmapImage("", bboxForLandscape);

    SceneEditor2* sceneEditor = GetCurrentScene();
    if(sceneEditor)
    {
        sceneEditor->Exec(new EntityAddCommand(entityToProcess, sceneEditor));
        sceneEditor->selectionSystem->SetSelection(entityToProcess);
    }
    SafeRelease(entityToProcess);
}

void QtMainWindow::OnAddSkybox()
{
    SceneEditor2* sceneEditor = GetCurrentScene();
    if(!sceneEditor)
    {
        return;
    }
    Entity* skyboxEntity = sceneEditor->skyboxSystem->AddSkybox();
    skyboxEntity->Retain();
    
    skyboxEntity->GetParent()->RemoveNode(skyboxEntity);
    sceneEditor->Exec(new EntityAddCommand(skyboxEntity, sceneEditor));
    skyboxEntity->Release();
}

void QtMainWindow::OnAddVegetation()
{
    SceneEditor2* sceneEditor = GetCurrentScene();
    if(sceneEditor)
    {
        DAVA::VegetationRenderObject* vro = new DAVA::VegetationRenderObject();
        RenderComponent* rc = new RenderComponent();
        rc->SetRenderObject(vro);
        SafeRelease(vro);

        Entity* vegetationNode = new Entity();
        vegetationNode->AddComponent(rc);
        vegetationNode->SetName(ResourceEditor::VEGETATION_NODE_NAME);
        vegetationNode->SetLocked(true);

        sceneEditor->Exec(new EntityAddCommand(vegetationNode, sceneEditor));
        sceneEditor->selectionSystem->SetSelection(vegetationNode);

        SafeRelease(vegetationNode);
    }
}

void QtMainWindow::OnLightDialog()
{
	Entity* sceneNode = new Entity();
	sceneNode->AddComponent(new LightComponent(ScopedPtr<Light>(new Light)));
	sceneNode->SetName(ResourceEditor::LIGHT_NODE_NAME);
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(sceneEditor)
	{
		sceneEditor->Exec(new EntityAddCommand(sceneNode, sceneEditor));
		sceneEditor->selectionSystem->SetSelection(sceneNode);
	}
	SafeRelease(sceneNode);
}

void QtMainWindow::OnCameraDialog()
{
	Entity* sceneNode = new Entity();
	Camera * camera = new Camera();

	camera->SetUp(DAVA::Vector3(0.0f, 0.0f, 1.0f));
	camera->SetPosition(DAVA::Vector3(0.0f, 0.0f, 0.0f));
	camera->SetTarget(DAVA::Vector3(1.0f, 0.0f, 0.0f));
	camera->SetupPerspective(70.0f, 320.0f / 480.0f, 1.0f, 5000.0f);
	camera->SetAspect(1.0f);

	sceneNode->AddComponent(new CameraComponent(camera));
	sceneNode->SetName(ResourceEditor::CAMERA_NODE_NAME);
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(sceneEditor)
	{
		sceneEditor->Exec(new EntityAddCommand(sceneNode, sceneEditor));
		sceneEditor->selectionSystem->SetSelection(sceneNode);
	}
	SafeRelease(sceneNode);
	SafeRelease(camera);
}

void QtMainWindow::OnUserNodeDialog()
{
	Entity* sceneNode = new Entity();
	sceneNode->AddComponent(new UserComponent());
	sceneNode->SetName(ResourceEditor::USER_NODE_NAME);
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(sceneEditor)
	{
		sceneEditor->Exec(new EntityAddCommand(sceneNode, sceneEditor));
		sceneEditor->selectionSystem->SetSelection(sceneNode);
	}
	SafeRelease(sceneNode);
}

void QtMainWindow::OnParticleEffectDialog()
{
	Entity* sceneNode = new Entity();
	sceneNode->AddComponent(new ParticleEffectComponent());
    sceneNode->AddComponent(new LodComponent());
	sceneNode->SetName(ResourceEditor::PARTICLE_EFFECT_NODE_NAME);
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(sceneEditor)
	{
		sceneEditor->Exec(new EntityAddCommand(sceneNode, sceneEditor));
		sceneEditor->selectionSystem->SetSelection(sceneNode);
	}
	SafeRelease(sceneNode);
}

void QtMainWindow::On2DCameraDialog()
{
    Entity* sceneNode = new Entity();
    Camera * camera = new Camera();
    
    float32 w = Core::Instance()->GetVirtualScreenXMax() - Core::Instance()->GetVirtualScreenXMin();
    float32 h = Core::Instance()->GetVirtualScreenYMax() - Core::Instance()->GetVirtualScreenYMin();
    float32 aspect = w / h;
    camera->SetupOrtho(w, aspect, 1, 1000);        
    camera->SetPosition(Vector3(0,0, -10000));
    camera->SetZFar(10000);
    camera->SetTarget(Vector3(0, 0, 0));  
    camera->SetUp(Vector3(0, -1, 0));
    camera->RebuildCameraFromValues();        

    sceneNode->AddComponent(new CameraComponent(camera));
    sceneNode->SetName("Camera 2D");
    SceneEditor2* sceneEditor = GetCurrentScene();
    if(sceneEditor)
    {
        sceneEditor->Exec(new EntityAddCommand(sceneNode, sceneEditor));
        sceneEditor->selectionSystem->SetSelection(sceneNode);
    }
    SafeRelease(sceneNode);
    SafeRelease(camera);
}
void QtMainWindow::On2DSpriteDialog()
{
    FilePath projectPath = ProjectManager::Instance()->CurProjectPath();
    projectPath += "Data/Gfx/";

    QString filePath = QtFileDialog::getOpenFileName(NULL, QString("Open sprite"), QString::fromStdString(projectPath.GetAbsolutePathname()), QString("Sprite File (*.txt)"));
    if (filePath.isEmpty())
        return;        
    filePath.remove(filePath.size() - 4, 4);
    Sprite* sprite = Sprite::Create(filePath.toStdString());
    if (!sprite)
        return;

    Entity *sceneNode = new Entity();
    sceneNode->SetName(ResourceEditor::EDITOR_SPRITE);
    SpriteObject *spriteObject = new SpriteObject(sprite, 0, Vector2(1,1), Vector2(0.5f*sprite->GetWidth(), 0.5f*sprite->GetHeight()));
    spriteObject->AddFlag(RenderObject::ALWAYS_CLIPPING_VISIBLE);
    sceneNode->AddComponent(new RenderComponent(spriteObject));    
    Matrix4 m = Matrix4(1,0,0,0,
                        0,1,0,0,
                        0,0,-1,0,                        
                        0,0,0,1);
    sceneNode->SetLocalTransform(m);
    SceneEditor2* sceneEditor = GetCurrentScene();
    if(sceneEditor)
    {
        sceneEditor->Exec(new EntityAddCommand(sceneNode, sceneEditor));
        sceneEditor->selectionSystem->SetSelection(sceneNode);
    }
    SafeRelease(sceneNode);
    SafeRelease(spriteObject);
    SafeRelease(sprite);
}

void QtMainWindow::OnAddEntityFromSceneTree()
{
	ui->menuAdd->exec(QCursor::pos());
}

void QtMainWindow::OnShowSettings()
{
	SettingsDialog t(this);
	t.exec();
}

void QtMainWindow::OnOpenHelp()
{
	FilePath docsPath = ResourceEditor::DOCUMENTATION_PATH + "index.html";
	QString docsFile = QString::fromStdString("file:///" + docsPath.GetAbsolutePathname());
	QDesktopServices::openUrl(QUrl(docsFile));
}

// ###################################################################################################
// Mainwindow load state functions
// ###################################################################################################

void QtMainWindow::LoadViewState(SceneEditor2 *scene)
{
	if(NULL != scene)
	{
		ui->actionShowEditorGizmo->setChecked(scene->IsHUDVisible());
		ui->actionOnSceneSelection->setChecked(scene->selectionSystem->IsSelectionAllowed());
     
        bool viewLMCanvas = SettingsManager::GetValue(Settings::Internal_MaterialsShowLightmapCanvas).AsBool();
        ui->actionLightmapCanvas->setChecked(viewLMCanvas);
	}
}

void QtMainWindow::LoadModificationState(SceneEditor2 *scene)
{
	if(NULL != scene)
	{
		ui->actionModifySelect->setChecked(false);
		ui->actionModifyMove->setChecked(false);
		ui->actionModifyRotate->setChecked(false);
		ui->actionModifyScale->setChecked(false);

		ST_ModifMode modifMode = scene->modifSystem->GetModifMode();
		modificationWidget->SetModifMode(modifMode);

		switch (modifMode)
		{
		case ST_MODIF_OFF:
			ui->actionModifySelect->setChecked(true);
			break;
		case ST_MODIF_MOVE:
			ui->actionModifyMove->setChecked(true);
			break;
		case ST_MODIF_ROTATE:
			ui->actionModifyRotate->setChecked(true);
			break;
		case ST_MODIF_SCALE:
			ui->actionModifyScale->setChecked(true);
			break;
		default:
			break;
		}


		// pivot point
		if(scene->selectionSystem->GetPivotPoint() == ST_PIVOT_ENTITY_CENTER)
		{
			ui->actionPivotCenter->setChecked(true);
			ui->actionPivotCommon->setChecked(false);
		}
		else
		{
			ui->actionPivotCenter->setChecked(false);
			ui->actionPivotCommon->setChecked(true);
		}

		// landscape snap
		ui->actionModifySnapToLandscape->setChecked(scene->modifSystem->GetLandscapeSnap());
	}
}

void QtMainWindow::LoadUndoRedoState(SceneEditor2 *scene)
{
	if(NULL != scene)
	{
		ui->actionUndo->setEnabled(scene->CanUndo());
		ui->actionRedo->setEnabled(scene->CanRedo());
	}
}

void QtMainWindow::LoadEditorLightState(SceneEditor2 *scene)
{
	if(NULL != scene)
	{
		ui->actionEnableCameraLight->setChecked(scene->editorLightSystem->GetCameraLightEnabled());
	}
}

void QtMainWindow::LoadShadowBlendModeState(SceneEditor2* scene)
{
	if(NULL != scene)
	{
		const ShadowPassBlendMode::eBlend blend = scene->GetShadowBlendMode();

		ui->actionDynamicBlendModeAlpha->setChecked(blend == ShadowPassBlendMode::MODE_BLEND_ALPHA);
		ui->actionDynamicBlendModeMultiply->setChecked(blend == ShadowPassBlendMode::MODE_BLEND_MULTIPLY);
	}
}


void QtMainWindow::LoadGPUFormat()
{
	int curGPU = GetGPUFormat();

	QList<QAction *> allActions = ui->menuTexturesForGPU->actions();
	for(int i = 0; i < allActions.size(); ++i)
	{
		QAction *actionN = allActions[i];

		if(!actionN->data().isNull() &&
			actionN->data().toInt() == curGPU)
		{
			actionN->setChecked(true);
			ui->actionReloadTextures->setText(actionN->text());
		}
		else
		{
			actionN->setChecked(false);
		}
	}
}

void QtMainWindow::LoadMaterialLightViewMode()
{
    int curViewMode = SettingsManager::GetValue(Settings::Internal_MaterialsLightViewMode).AsInt32();

    ui->actionAlbedo->setChecked((bool) (curViewMode & EditorMaterialSystem::LIGHTVIEW_ALBEDO));
    ui->actionAmbient->setChecked((bool) (curViewMode & EditorMaterialSystem::LIGHTVIEW_AMBIENT));
    ui->actionSpecular->setChecked((bool) (curViewMode & EditorMaterialSystem::LIGHTVIEW_SPECULAR));
    ui->actionDiffuse->setChecked((bool) (curViewMode & EditorMaterialSystem::LIGHTVIEW_DIFFUSE));
}

void QtMainWindow::LoadLandscapeEditorState(SceneEditor2* scene)
{
	OnLandscapeEditorToggled(scene);
}

void QtMainWindow::OnSetShadowColor()
{
	SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;
    if(NULL == FindLandscape(scene))
	{
		ShowErrorDialog(ResourceEditor::NO_LANDSCAPE_ERROR_MESSAGE);
		return;
	}
	
    QColor color = QColorDialog::getColor(ColorToQColor(scene->GetShadowColor()), 0, tr("Shadow Color"), QColorDialog::ShowAlphaChannel);

	scene->Exec(new ChangeDynamicShadowColorCommand(scene, QColorToColor(color)));
}

void QtMainWindow::OnShadowBlendModeWillShow()
{
	SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;

    LoadShadowBlendModeState(scene);
}

void QtMainWindow::OnShadowBlendModeAlpha()
{
	SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;

	if(NULL == FindLandscape(scene))
	{
		ShowErrorDialog(ResourceEditor::NO_LANDSCAPE_ERROR_MESSAGE);
		return;
	}
	
	scene->Exec(new ChangeDynamicShadowModeCommand(scene, ShadowPassBlendMode::MODE_BLEND_ALPHA));
}

void QtMainWindow::OnShadowBlendModeMultiply()
{
	SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;
	if(NULL == FindLandscape(scene))
	{
		ShowErrorDialog(ResourceEditor::NO_LANDSCAPE_ERROR_MESSAGE);
		return;
	}
	
	scene->Exec(new ChangeDynamicShadowModeCommand(scene, ShadowPassBlendMode::MODE_BLEND_MULTIPLY));
}

void QtMainWindow::OnSaveHeightmapToPNG()
{
	if (!IsSavingAllowed())
	{
		return;
	}

	SceneEditor2* scene = GetCurrentScene();
	
    Landscape *landscape = FindLandscape(scene);
	QString titleString = "Saving is not allowed";
	
	if (!landscape)
	{
		QMessageBox::warning(this, titleString, "There is no landscape in scene!");
		return;
	}
	if (!landscape->GetHeightmap()->Size())
	{
		QMessageBox::warning(this, titleString, "There is no heightmap in landscape!");
		return;
	}
	
    Heightmap * heightmap = landscape->GetHeightmap();
    FilePath heightmapPath = landscape->GetHeightmapPathname();
    FilePath requestedPngPath = FilePath::CreateWithNewExtension(heightmapPath, ".png");

    QString selectedPath = QtFileDialog::getSaveFileName(this, "Save heightmap as", requestedPngPath.GetAbsolutePathname().c_str(), "PGN Image (*.png)");
    if(selectedPath.isEmpty()) return;

    requestedPngPath = DAVA::FilePath(selectedPath.toStdString());
    heightmap->SaveToImage(requestedPngPath);
}

void QtMainWindow::OnSaveTiledTexture()
{
	if (!IsSavingAllowed())
	{
		return;
	}

	SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;

	LandscapeEditorDrawSystem::eErrorType varifLandscapeError = scene->landscapeEditorDrawSystem->VerifyLandscape();
	if (varifLandscapeError != LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
	{
		ShowErrorDialog(LandscapeEditorDrawSystem::GetDescriptionByError(varifLandscapeError));
		return;
	}

    Landscape *landscape = FindLandscape(scene);
    if(!landscape) return;

	Texture* landscapeTexture = landscape->CreateLandscapeTexture();
	if (landscapeTexture)
	{
		FilePath pathToSave;
		pathToSave = landscape->GetTextureName(Landscape::TEXTURE_COLOR);
		if (pathToSave.IsEmpty())
		{
			FilePath scenePath = scene->GetScenePath().GetDirectory();
			QString selectedPath = QtFileDialog::getSaveFileName(this, "Save landscape texture as",
														 scenePath.GetAbsolutePathname().c_str(),
														 "PGN Image (*.png)");
			if (selectedPath.isEmpty())
			{
				SafeRelease(landscapeTexture);
				return;
			}

			pathToSave = FilePath(selectedPath.toStdString());
		}
		else
		{
			pathToSave.ReplaceExtension(".thumbnail.png");
		}

		Image *image = landscapeTexture->CreateImageFromMemory(RenderState::RENDERSTATE_2D_OPAQUE);
		if(image)
		{
            ImageSystem::Instance()->Save(pathToSave, image);
			SafeRelease(image);
		}

		SafeRelease(landscapeTexture);
	}
}

void QtMainWindow::OnConvertModifiedTextures()
{
	SceneEditor2* scene = GetCurrentScene();
	if(!scene)
	{
		return;
	}
	
	WaitStart("Conversion of modified textures.","Checking for modified textures.");
	Map<Texture *, Vector<eGPUFamily> > textures;
	int filesToUpdate = SceneHelper::EnumerateModifiedTextures(scene, textures);
	
	if(filesToUpdate == 0)
	{
		WaitStop();
		return;
	}
	
	int convretedNumber = 0;
	waitDialog->SetRange(convretedNumber, filesToUpdate);
	WaitSetValue(convretedNumber);
	for(Map<Texture *, Vector<eGPUFamily> >::iterator it = textures.begin(); it != textures.end(); ++it)
	{
		DAVA::TextureDescriptor *descriptor = it->first->GetDescriptor();
		
		if(NULL == descriptor)
		{
			continue;
		}

        DAVA::VariantType quality = SettingsManager::Instance()->GetValue(Settings::General_CompressionQuality);
        
		Vector<eGPUFamily> updatedGPUs = it->second;
		WaitSetMessage(descriptor->GetSourceTexturePathname().GetAbsolutePathname().c_str());
		foreach(eGPUFamily gpu, updatedGPUs)
		{

			DAVA::TextureConverter::ConvertTexture(*descriptor, gpu, true, (TextureConverter::eConvertQuality)quality.AsInt32());
			WaitSetValue(++convretedNumber);
		}
	}
	WaitStop();
}

void QtMainWindow::OnGlobalInvalidateTimeout()
{
    emit GlobalInvalidateTimeout();
    if(globalInvalidate)
    {
        StartGlobalInvalidateTimer();
    }
}


void QtMainWindow::EnableGlobalTimeout(bool enable)
{
    if(globalInvalidate != enable)
    {
        globalInvalidate = enable;
        
        if(globalInvalidate)
        {
            StartGlobalInvalidateTimer();
        }
    }
}

void QtMainWindow::StartGlobalInvalidateTimer()
{
    QTimer::singleShot(GLOBAL_INVALIDATE_TIMER_DELTA, this, SLOT(OnGlobalInvalidateTimeout()));
}

void QtMainWindow::EditorLightEnabled( bool enabled )
{
	ui->actionEnableCameraLight->setChecked(enabled);
}

void QtMainWindow::OnBeastAndSave()
{
	SceneEditor2* scene = GetCurrentScene();
	if(!scene) return;

    if (!scene->IsLoaded() || scene->IsChanged())
    {
        if (!SaveScene(scene))
            return;
    }

    BeastDialog dlg(this);
    dlg.SetScene(scene);
    const bool run = dlg.Exec();
    if ( !run ) return;

	if (!SaveTilemask(false))
	{
		return;
	}

    RunBeast(dlg.GetPath(), dlg.GetMode());
	SaveScene(scene);

    scene->ClearAllCommands();
    LoadUndoRedoState(scene);
}

void QtMainWindow::OnCameraSpeed0()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(NULL != sceneEditor)
	{
		sceneEditor->cameraSystem->SetMoveSpeedArrayIndex(0);
	}
}

void QtMainWindow::OnCameraSpeed1()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(NULL != sceneEditor)
	{
		sceneEditor->cameraSystem->SetMoveSpeedArrayIndex(1);
	}
}

void QtMainWindow::OnCameraSpeed2()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(NULL != sceneEditor)
	{
		sceneEditor->cameraSystem->SetMoveSpeedArrayIndex(2);
	}
}

void QtMainWindow::OnCameraSpeed3()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(NULL != sceneEditor)
	{
		sceneEditor->cameraSystem->SetMoveSpeedArrayIndex(3);
	}
}

void QtMainWindow::OnCameraLookFromTop()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(NULL != sceneEditor)
	{
		sceneEditor->cameraSystem->MoveTo(DAVA::Vector3(0, 0, 200), DAVA::Vector3(1, 0, 0));
	}
}

void QtMainWindow::RunBeast(const QString& outputPath, BeastProxy::eBeastMode mode)
{
#if defined (__DAVAENGINE_BEAST__)

	SceneEditor2* scene = GetCurrentScene();
	if(!scene) return;

    const DAVA::FilePath path = outputPath.toStdString();
    scene->Exec(new BeastAction(scene, path, mode, beastWaitDialog));

    if(mode == BeastProxy::MODE_LIGHTMAPS)
    {
	    OnReloadTextures();
    }

#endif //#if defined (__DAVAENGINE_BEAST__)
}

void QtMainWindow::BeastWaitSetMessage(const QString &messsage)
{
	beastWaitDialog->SetMessage(messsage);

}

bool QtMainWindow::BeastWaitCanceled()
{
	return beastWaitDialog->WasCanceled();
}

void QtMainWindow::OnLandscapeEditorToggled(SceneEditor2* scene)
{
	if (scene != GetCurrentScene())
	{
		return;
	}

	ui->actionCustomColorsEditor->setChecked(false);
	ui->actionHeightMapEditor->setChecked(false);
	ui->actionRulerTool->setChecked(false);
	ui->actionTileMapEditor->setChecked(false);
	ui->actionVisibilityCheckTool->setChecked(false);
	ui->actionShowNotPassableLandscape->setChecked(false);
    ui->actionGrasEditor->setChecked(false);
	
	int32 tools = scene->GetEnabledTools();

	UpdateConflictingActionsState(tools == 0);

	if (tools & SceneEditor2::LANDSCAPE_TOOL_CUSTOM_COLOR)
	{
		ui->actionCustomColorsEditor->setChecked(true);
	}
	if (tools & SceneEditor2::LANDSCAPE_TOOL_HEIGHTMAP_EDITOR)
	{
		ui->actionHeightMapEditor->setChecked(true);
	}
	if (tools & SceneEditor2::LANDSCAPE_TOOL_RULER)
	{
		ui->actionRulerTool->setChecked(true);
	}
	if (tools & SceneEditor2::LANDSCAPE_TOOL_TILEMAP_EDITOR)
	{
		ui->actionTileMapEditor->setChecked(true);
	}
	if (tools & SceneEditor2::LANDSCAPE_TOOL_VISIBILITY)
	{
		ui->actionVisibilityCheckTool->setChecked(true);
	}
	if (tools & SceneEditor2::LANDSCAPE_TOOL_NOT_PASSABLE_TERRAIN)
	{
		ui->actionShowNotPassableLandscape->setChecked(true);
	}
    if(tools & SceneEditor2::LANDSCAPE_TOOL_GRASS_EDITOR)
    {
        ui->actionGrasEditor->setChecked(true);
    }
}

void QtMainWindow::OnCustomColorsEditor()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if (!sceneEditor)
	{
		return;
	}
	
	if(!sceneEditor->customColorsSystem->IsLandscapeEditingEnabled())
	{
		if (LoadAppropriateTextureFormat())
		{
			sceneEditor->Exec(new ActionEnableCustomColors(sceneEditor));
		}
		else
		{
			OnLandscapeEditorToggled(sceneEditor);
		}
		return;
	}

    if (sceneEditor->customColorsSystem->ChangesPresent())
    {
        FilePath currentTexturePath = sceneEditor->customColorsSystem->GetCurrentSaveFileName();
	
        if ((currentTexturePath.IsEmpty() || !currentTexturePath.Exists()) &&
            !SelectCustomColorsTexturePath())
        {
            ui->actionCustomColorsEditor->setChecked(true);
            return;
        }
	}
	
	sceneEditor->DisableTools(SceneEditor2::LANDSCAPE_TOOL_CUSTOM_COLOR, true);
	ui->actionCustomColorsEditor->setChecked(false);
}

bool QtMainWindow::SelectCustomColorsTexturePath()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(!sceneEditor)
	{
		return false;
	}
	FilePath scenePath = sceneEditor->GetScenePath().GetDirectory();
	
	QString filePath = QtFileDialog::getSaveFileName(NULL,
													 QString(ResourceEditor::CUSTOM_COLORS_SAVE_CAPTION.c_str()),
													 QString(scenePath.GetAbsolutePathname().c_str()),
													 QString(ResourceEditor::CUSTOM_COLORS_FILE_FILTER.c_str()));
	FilePath selectedPathname = PathnameToDAVAStyle(filePath);
	Entity* landscape = FindLandscapeEntity(sceneEditor);
	if (selectedPathname.IsEmpty() || NULL == landscape)
	{
		return false;
	}

	KeyedArchive* customProps = GetOrCreateCustomProperties(landscape)->GetArchive();
	if(NULL == customProps)
	{
		return false;
	}
	
	String pathToSave = selectedPathname.GetRelativePathname(ProjectManager::Instance()->CurProjectPath().GetAbsolutePathname());
	customProps->SetString(ResourceEditor::CUSTOM_COLOR_TEXTURE_PROP,pathToSave);
	
    return true;
}

void QtMainWindow::OnHeightmapEditor()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if (!sceneEditor)
	{
		return;
	}
	
	if (sceneEditor->heightmapEditorSystem->IsLandscapeEditingEnabled())
	{
		sceneEditor->Exec(new ActionDisableHeightmapEditor(sceneEditor));
	}
	else
	{
		if (LoadAppropriateTextureFormat())
		{
			sceneEditor->Exec(new ActionEnableHeightmapEditor(sceneEditor));
		}
		else
		{
			OnLandscapeEditorToggled(sceneEditor);
		}
	}
}

void QtMainWindow::OnRulerTool()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if (!sceneEditor)
	{
		return;
	}

	if (sceneEditor->rulerToolSystem->IsLandscapeEditingEnabled())
	{
		sceneEditor->Exec(new ActionDisableRulerTool(sceneEditor));
	}
	else
	{
		if (LoadAppropriateTextureFormat())
		{
			sceneEditor->Exec(new ActionEnableRulerTool(sceneEditor));
		}
		else
		{
			OnLandscapeEditorToggled(sceneEditor);
		}
	}
}

void QtMainWindow::OnTilemaskEditor()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if (!sceneEditor)
	{
		return;
	}
	
	if (sceneEditor->tilemaskEditorSystem->IsLandscapeEditingEnabled())
	{
		sceneEditor->Exec(new ActionDisableTilemaskEditor(sceneEditor));
	}
	else
	{
		if (LoadAppropriateTextureFormat())
		{
			sceneEditor->Exec(new ActionEnableTilemaskEditor(sceneEditor));
		}
		else
		{
			OnLandscapeEditorToggled(sceneEditor);
		}
	}
}

void QtMainWindow::OnVisibilityTool()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if (!sceneEditor)
	{
		return;
	}
	
	if (sceneEditor->visibilityToolSystem->IsLandscapeEditingEnabled())
	{
		sceneEditor->Exec(new ActionDisableVisibilityTool(sceneEditor));
	}
	else
	{
		if (LoadAppropriateTextureFormat())
		{
			sceneEditor->Exec(new ActionEnableVisibilityTool(sceneEditor));
		}
		else
		{
			OnLandscapeEditorToggled(sceneEditor);
		}
	}
}

void QtMainWindow::OnNotPassableTerrain()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	if (!sceneEditor)
	{
		return;
	}
	
	if (sceneEditor->landscapeEditorDrawSystem->IsNotPassableTerrainEnabled())
	{
		sceneEditor->Exec(new ActionDisableNotPassable(sceneEditor));
	}
	else
	{
		if (LoadAppropriateTextureFormat())
		{
			sceneEditor->Exec(new ActionEnableNotPassable(sceneEditor));
		}
		else
		{
			OnLandscapeEditorToggled(sceneEditor);
		}
	}
}

void QtMainWindow::OnGrasEditor()
{
    /*SceneEditor2* sceneEditor = GetCurrentScene();
    if(!sceneEditor)
    {
        return;
    }

    bool toggled = false;
    if(sceneEditor->grassEditorSystem->IsEnabledGrassEdit())
    {
        toggled = sceneEditor->grassEditorSystem->EnableGrassEdit(false);
    }
    else
    {
        sceneEditor->DisableTools(SceneEditor2::LANDSCAPE_TOOLS_ALL);
        toggled = sceneEditor->grassEditorSystem->EnableGrassEdit(true);
    }

    if(toggled)
    {
        SceneSignals::Instance()->EmitGrassEditorToggled(sceneEditor);
        OnLandscapeEditorToggled(sceneEditor);
    }*/
}

void QtMainWindow::OnBuildStaticOcclusion()
{
    SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;
    
    QtWaitDialog *waitOcclusionDlg = new QtWaitDialog(this);
    waitOcclusionDlg->Show("Static occlusion", "Please wait while building static occlusion.", true, true);

    scene->staticOcclusionBuildSystem->Build();
    while(scene->staticOcclusionBuildSystem->IsInBuild())
    {
        if(waitOcclusionDlg->WasCanceled())
        {
            scene->staticOcclusionBuildSystem->Cancel();
        }
        else
        {
            waitOcclusionDlg->SetValue(scene->staticOcclusionBuildSystem->GetBuildStatus());
        }
    }

    delete waitOcclusionDlg;
}

void QtMainWindow::OnInavalidateStaticOcclusion()
{
    SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;
    scene->staticOcclusionSystem->InvalidateOcclusion();
}

void QtMainWindow::OnRebuildCurrentOcclusionCell()
{
    SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;

    scene->staticOcclusionBuildSystem->RebuildCurrentCell();
}

bool QtMainWindow::IsSavingAllowed()
{
	SceneEditor2* scene = GetCurrentScene();
	
	if (!scene || scene->GetEnabledTools() != 0)
	{
		QMessageBox::warning(this, "Saving is not allowed", "Disable landscape editing before save!");
		return false;
	}
	
	return true;
}

void QtMainWindow::OnObjectsTypeChanged( QAction *action )
{
	SceneEditor2* scene = GetCurrentScene();
	if(!scene) return;

	ResourceEditor::eSceneObjectType objectType = (ResourceEditor::eSceneObjectType) action->data().toInt();
	if(objectType < ResourceEditor::ESOT_COUNT && objectType >= ResourceEditor::ESOT_NONE)
	{
		scene->debugDrawSystem->SetRequestedObjectType(objectType);
	}
    
    bool wasBlocked = objectTypesWidget->blockSignals(true);
    objectTypesWidget->setCurrentIndex(objectType + 1);
    objectTypesWidget->blockSignals(wasBlocked);
}

void QtMainWindow::OnObjectsTypeChanged(int type)
{
	SceneEditor2* scene = GetCurrentScene();
	if(!scene) return;

	ResourceEditor::eSceneObjectType objectType = (ResourceEditor::eSceneObjectType) (type - 1);
	if(objectType < ResourceEditor::ESOT_COUNT && objectType >= ResourceEditor::ESOT_NONE)
	{
		scene->debugDrawSystem->SetRequestedObjectType(objectType);
	}
}

void QtMainWindow::LoadObjectTypes( SceneEditor2 *scene )
{
	if(!scene) return;
	ResourceEditor::eSceneObjectType objectType = scene->debugDrawSystem->GetRequestedObjectType();

	QList<QAction *> actions = ui->menuObjectTypes->actions();

	auto endIt = actions.end();
	for(auto it = actions.begin(); it != endIt; ++it)
	{
		ResourceEditor::eSceneObjectType objectTypeAction = (ResourceEditor::eSceneObjectType) (*it)->data().toInt();
		if(objectTypeAction == objectType)
		{
			objectTypesLabel->setDefaultAction(*it);
			break;
		}
	}

    objectTypesWidget->setCurrentIndex(objectType + 1);
}

bool QtMainWindow::OpenScene( const QString & path )
{
	bool ret = false;

	if(!path.isEmpty())
	{
		FilePath projectPath(ProjectManager::Instance()->CurProjectPath());
		FilePath argumentPath(path.toStdString());

		if(!FilePath::ContainPath(argumentPath, projectPath))
		{
			QMessageBox::warning(this, "Open scene error.", QString().sprintf("Can't open scene file outside project path.\n\nScene:\n%s\n\nProject:\n%s", 
				projectPath.GetAbsolutePathname().c_str(),
				argumentPath.GetAbsolutePathname().c_str()));
		}
		else
		{
            int needCloseIndex = -1;
			SceneEditor2 *scene = ui->sceneTabWidget->GetCurrentScene();
			if(scene && (ui->sceneTabWidget->GetTabCount() == 1))
			{
				FilePath path = scene->GetScenePath();
				if(path.GetFilename() == "newscene1.sc2" && !scene->CanUndo())
				{
					needCloseIndex = 0;
				}
			}

			DAVA::FilePath scenePath = DAVA::FilePath(path.toStdString());

			WaitStart("Opening scene...", scenePath.GetAbsolutePathname().c_str());

			int index = ui->sceneTabWidget->OpenTab(scenePath);

            WaitStop();

            if(index != -1)
			{
				ui->sceneTabWidget->SetCurrentTab(index);
				AddRecent(path);

                // close empty default scene
                if(-1 != needCloseIndex)
                {
                    ui->sceneTabWidget->CloseTab(needCloseIndex);
                }

				ret = true;
			}
		}
	}

    return ret;
}

void QtMainWindow::OnSnapToLandscapeChanged(SceneEditor2* scene, bool isSpanToLandscape)
{
	if (GetCurrentScene() != scene)
	{
		return;
	}

	ui->actionModifySnapToLandscape->setChecked(isSpanToLandscape);
}

void QtMainWindow::closeEvent( QCloseEvent * e )
{
	bool changed = IsAnySceneChanged();
	if(changed)
	{
		int answer = QMessageBox::question(this, "Scene was changed", "Do you want to quit anyway?",
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

		if(answer == QMessageBox::No)
		{
			e->ignore();
			return;
		}
	}

	e->accept();
	QMainWindow::closeEvent(e);
}

bool QtMainWindow::IsAnySceneChanged()
{
	int count = ui->sceneTabWidget->GetTabCount();
	for(int i = 0; i < count; ++i)
	{
		SceneEditor2 *scene = ui->sceneTabWidget->GetTabScene(i);
		if(scene->IsChanged())
		{
			return true;
		}
	}

	return false;
}

void QtMainWindow::OnHangingObjects()
{
	SceneEditor2* scene = GetCurrentScene();
	if(!scene) return;

	scene->debugDrawSystem->EnableHangingObjectsMode(ui->actionHangingObjects->isChecked());
}

void QtMainWindow::LoadHangingObjects( SceneEditor2 * scene )
{
	ui->actionHangingObjects->setChecked(scene->debugDrawSystem->HangingObjectsModeEnabled());
	if(hangingObjectsWidget)
	{
		hangingObjectsWidget->SetHeight(DebugDrawSystem::HANGING_OBJECTS_HEIGHT);
	}
}

void QtMainWindow::OnHangingObjectsHeight( double value)
{
	DebugDrawSystem::HANGING_OBJECTS_HEIGHT = (DAVA::float32) value;
}

void QtMainWindow::OnMaterialLightViewChanged(bool)
{
    int newMode = EditorMaterialSystem::LIGHTVIEW_NOTHING;

    if(ui->actionAlbedo->isChecked()) newMode |= EditorMaterialSystem::LIGHTVIEW_ALBEDO;
    if(ui->actionDiffuse->isChecked()) newMode |= EditorMaterialSystem::LIGHTVIEW_DIFFUSE;
    if(ui->actionAmbient->isChecked()) newMode |= EditorMaterialSystem::LIGHTVIEW_AMBIENT;
    if(ui->actionSpecular->isChecked()) newMode |= EditorMaterialSystem::LIGHTVIEW_SPECULAR;

    if(newMode != SettingsManager::GetValue(Settings::Internal_MaterialsLightViewMode).AsInt32())
    {
        SettingsManager::SetValue(Settings::Internal_MaterialsLightViewMode, DAVA::VariantType(newMode));
    }

    if(NULL != GetCurrentScene())
    {
        GetCurrentScene()->materialSystem->SetLightViewMode(newMode);
    }
}

void QtMainWindow::OnCustomQuality()
{
    QualitySwitcher::Show();
}

void QtMainWindow::UpdateConflictingActionsState(bool enable)
{
	ui->menuTexturesForGPU->setEnabled(enable);
	ui->actionReloadTextures->setEnabled(enable);
	ui->menuExport->setEnabled(enable);
    ui->actionSaveToFolder->setEnabled(enable);
}

void QtMainWindow::DiableUIForFutureUsing()
{
	//TODO: temporary disabled
	//-->
	//ui->actionAddNewComponent->setVisible(false);
	//ui->actionRemoveComponent->setVisible(false);
	//<--
}

void QtMainWindow::OnEmptyEntity()
{
	SceneEditor2* scene = GetCurrentScene();
	if(!scene) return;

	Entity* newEntity = new Entity();
	newEntity->SetName(ResourceEditor::ENTITY_NAME);

	scene->Exec(new EntityAddCommand(newEntity, scene));
	scene->selectionSystem->SetSelection(newEntity);

	newEntity->Release();
}

void QtMainWindow::OnAddWindEntity()
{
	SceneEditor2* scene = GetCurrentScene();
	if(!scene) return;

	Entity * windEntity = new Entity();
	windEntity->SetName(ResourceEditor::WIND_NODE_NAME);

	Matrix4 ltMx = Matrix4::MakeTranslation(Vector3(0.f, 0.f, 20.f));
	GetTransformComponent(windEntity)->SetLocalTransform(&ltMx);

	WindComponent * wind = new WindComponent();
	windEntity->AddComponent(wind);

	scene->Exec(new EntityAddCommand(windEntity, scene));
	scene->selectionSystem->SetSelection(windEntity);

	windEntity->Release();
}

bool QtMainWindow::LoadAppropriateTextureFormat()
{
	if (GetGPUFormat() != GPU_PNG)
	{
		int answer = ShowQuestion("Inappropriate texture format",
								  "Landscape editing is only allowed in PNG texture format.\nDo you want to reload textures in PNG format?",
								  MB_FLAG_YES | MB_FLAG_NO, MB_FLAG_NO);
		if (answer == MB_FLAG_NO)
		{
			return false;
		}

		OnReloadTexturesTriggered(ui->actionReloadPNG);
	}

	return (GetGPUFormat() == GPU_PNG);
}

bool QtMainWindow::IsTilemaskModificationCommand(const Command2* cmd)
{
	if (cmd->GetId() == CMDID_TILEMASK_MODIFY)
	{
		return true;
	}

	if (cmd->GetId() == CMDID_BATCH)
	{
		CommandBatch* batch = (CommandBatch*)cmd;
		for (int32 i = 0; i < batch->Size(); ++i)
		{
			if (IsTilemaskModificationCommand(batch->GetCommand(i)))
			{
				return true;
			}
		}
	}

	return false;
}

bool QtMainWindow::SaveTilemask(bool forAllTabs /* = true */)
{
	SceneTabWidget *sceneWidget = GetSceneWidget();
	
	int lastSceneTab = sceneWidget->GetCurrentTab();
	int answer = QMessageBox::Cancel;
	bool needQuestion = true;

	// tabs range where tilemask should be saved
	int32 firstTab = forAllTabs ? 0 : sceneWidget->GetCurrentTab();
	int32 lastTab = forAllTabs ? sceneWidget->GetTabCount() : sceneWidget->GetCurrentTab() + 1;

	for(int i = firstTab; i < lastTab; ++i)
	{
		SceneEditor2 *tabEditor = sceneWidget->GetTabScene(i);
		if(NULL != tabEditor)
		{
			const CommandStack *cmdStack = tabEditor->GetCommandStack();
			for(size_t j = cmdStack->GetCleanIndex(); j < cmdStack->GetNextIndex(); j++)
			{
				const Command2 *cmd = cmdStack->GetCommand(j);
				if(IsTilemaskModificationCommand(cmd))
				{
					// ask user about saving tilemask changes
					sceneWidget->SetCurrentTab(i);

					if(needQuestion)
					{
						QString message = tabEditor->GetScenePath().GetFilename().c_str();
						message += " has unsaved tilemask changes.\nDo you want to save?";

						// if more than one scene to precess
						if((lastTab - firstTab) > 1)
						{
							answer = QMessageBox::warning(this, "", message, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel | QMessageBox::YesToAll | QMessageBox::NoToAll, QMessageBox::Cancel);
						}
						else
						{
							answer = QMessageBox::warning(this, "", message, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);
						}
					}

					switch(answer)
					{
					case QMessageBox::YesAll:
						needQuestion = false;
					case QMessageBox::Yes:
						{
							// turn off editor
							tabEditor->DisableTools(SceneEditor2::LANDSCAPE_TOOLS_ALL);

							// save
							tabEditor->landscapeEditorDrawSystem->SaveTileMaskTexture();
						}
						break;

					case QMessageBox::NoAll:
						needQuestion = false;
					case QMessageBox::No:
						{
							// turn off editor
							tabEditor->DisableTools(SceneEditor2::LANDSCAPE_TOOLS_ALL);
						}
						break;

					case QMessageBox::Cancel:
					default:
						{
							// cancel save process
							return false;
						}
						break;
					}

					// finish for cycle going through commands
					break;
				}
			}

			//reset tilemask
			tabEditor->landscapeEditorDrawSystem->ResetTileMaskTexture();

			// clear all tilemask commands in commandStack because they will be
			// invalid after tilemask reloading
			tabEditor->ClearCommands(CMDID_TILEMASK_MODIFY);
		}
	}

	sceneWidget->SetCurrentTab(lastSceneTab);

	return true;
}


void QtMainWindow::OnReloadShaders()
{
    ShaderCache::Instance()->Reload();
    
    DAVA::uint32 count = ui->sceneTabWidget->GetTabCount();
    for(DAVA::uint32 i = 0; i < count; ++i)
    {
        SceneEditor2 * scene = ui->sceneTabWidget->GetTabScene(i);
        if(!scene) continue;
        
        DAVA::Set<DAVA::NMaterial *> materialList;
        DAVA::MaterialSystem *matSystem = scene->GetMaterialSystem();
        matSystem->BuildMaterialList(scene, materialList, NMaterial::MATERIALTYPE_NONE, true);
        
        const Map<uint32, NMaterial *> & particleInstances = scene->particleEffectSystem->GetMaterialInstances();
        Map<uint32, NMaterial *>::const_iterator endParticleIt = particleInstances.end();
        Map<uint32, NMaterial *>::const_iterator particleIt = particleInstances.begin();
        for( ; particleIt != endParticleIt; ++particleIt)
        {
            materialList.insert(particleIt->second);
            if(particleIt->second->GetParent())
                materialList.insert(particleIt->second->GetParent());
        }

        DAVA::Set<DAVA::NMaterial *>::iterator it = materialList.begin();
        DAVA::Set<DAVA::NMaterial *>::iterator endIt = materialList.end();
        while (it != endIt)
        {
            DAVA::NMaterial * material = *it;
            DVASSERT(material);
            
            if(material)
                material->BuildActiveUniformsCacheParamsCache();
            
            ++it;
        }
        
        if(scene->GetGlobalMaterial())
            scene->GetGlobalMaterial()->BuildActiveUniformsCacheParamsCache();
    }
}

void QtMainWindow::OnSwitchWithDifferentLODs(bool checked)
{
    SceneEditor2 *scene = GetCurrentScene();
    if(!scene) return;

    scene->debugDrawSystem->EnableSwithcesWithDifferentLODsMode(checked);

    if(checked)
    {
        Set<FastName> entitiNames;
        SceneValidator::FindSwitchesWithDifferentLODs(scene, entitiNames);

        DAVA::Set<FastName>::iterator it = entitiNames.begin();
        DAVA::Set<FastName>::iterator endIt = entitiNames.end();
        while (it != endIt)
        {
            Logger::Info("Entity %s has different lods count.", it->c_str());
            ++it;
        }
    }
}

void QtMainWindow::DebugVersionInfo()
{
    if (!versionInfoWidget)
    {
        versionInfoWidget = new VersionInfoWidget(this);
        versionInfoWidget->setWindowFlags(Qt::Window);
        versionInfoWidget->setAttribute(Qt::WA_DeleteOnClose);
    }

    versionInfoWidget->show();
}
