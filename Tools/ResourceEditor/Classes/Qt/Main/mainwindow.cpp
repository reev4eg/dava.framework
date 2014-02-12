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

#include "mainwindow.h"
#include "QtUtils.h"
#include "Project/ProjectManager.h"
#include "DockConsole/Console.h"
#include "Scene/SceneHelper.h"
#include "SpritesPacker/SpritePackerHelper.h"

#include "TextureBrowser/TextureBrowser.h"
#include "TextureBrowser/TextureCache.h"
#include "MaterialEditor/MaterialEditor.h"
#include "QualitySwitcher/QualitySwitcher.h"

#include "Qt/Settings/SettingsManager.h"
#include "Deprecated/EditorConfig.h"

#include "../CubemapEditor/CubemapUtils.h"
#include "../CubemapEditor/CubemapTextureBrowser.h"
#include "../Tools/AddSkyboxDialog/AddSkyboxDialog.h"

#include "Tools/BaseAddEntityDialog/BaseAddEntityDialog.h"
#include "Tools/QtFileDialog/QtFileDialog.h"

#ifdef __DAVAENGINE_SPEEDTREE__
#include "Classes/Qt/SpeedTreeImport/SpeedTreeImportDialog.h"
#endif

#include "../Tools/AddSwitchEntityDialog/AddSwitchEntityDialog.h"

#include "Classes/Commands2/EntityAddCommand.h"
#include "StringConstants.h"
#include "../Settings/SettingsManager.h"

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
#include "Tools/SettingsDialog/GeneralSettingsDialog.h"
#include "Tools/SettingsDialog/SceneSettingsDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QColorDialog>
#include <QShortcut>
#include <QKeySequence>

#include "Scene3D/Components/ActionComponent.h"
#include "Scene3D/Systems/SkyboxSystem.h"

#include "Classes/Constants.h"

#include "TextureCompression/TextureConverter.h"
#include "RecentFilesManager.h"

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
{
	new Console();
	new ProjectManager();
	new RecentFilesManager();
	ui->setupUi(this);
    
    SetupTitle();

	qApp->installEventFilter(this);
	EditorConfig::Instance()->ParseConfig(SettingsManager::Instance()->GetValue(ResourceEditor::SETTINGS_PROJECT_PATH, SettingsManager::INTERNAL).AsString()
		+ "EditorConfig.yaml");

	SetupMainMenu();
	SetupToolBars();
	SetupStatusBar();
	SetupDocks();
	SetupActions();
	SetupShortCuts();

	// create tool windows
	new TextureBrowser(this);
	new MaterialEditor(this);
	waitDialog = new QtWaitDialog(this);
	beastWaitDialog = new QtWaitDialog(this);

	posSaver.Attach(this);
	posSaver.LoadState(this);

	QObject::connect(ProjectManager::Instance(), SIGNAL(ProjectOpened(const QString &)), this, SLOT(ProjectOpened(const QString &)));
	QObject::connect(ProjectManager::Instance(), SIGNAL(ProjectClosed()), this, SLOT(ProjectClosed()));

	QObject::connect(SceneSignals::Instance(), SIGNAL(CommandExecuted(SceneEditor2 *, const Command2*, bool)), this, SLOT(SceneCommandExecuted(SceneEditor2 *, const Command2*, bool)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2 *)), this, SLOT(SceneActivated(SceneEditor2 *)));
	QObject::connect(SceneSignals::Instance(), SIGNAL(Deactivated(SceneEditor2 *)), this, SLOT(SceneDeactivated(SceneEditor2 *)));

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
            DAVA::FilePath dataSourcePath = ProjectManager::Instance()->CurProjectDataSourcePath().toStdString();
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
	
	return (eGPUFamily)SettingsManager::Instance()->GetValue(ResourceEditor::SETTINGS_TEXTURE_VIEW_GPU, SettingsManager::INTERNAL).AsInt32();
}

void QtMainWindow::SetGPUFormat(DAVA::eGPUFamily gpu)
{
	// before reloading textures we should save tilemask texture for all opened scenes
	if(SaveTilemask())
	{
		SettingsManager::Instance()->SetValue(ResourceEditor::SETTINGS_TEXTURE_VIEW_GPU, VariantType(gpu), SettingsManager::INTERNAL);
		DAVA::Texture::SetDefaultGPU(gpu);

		DAVA::TexturesMap allScenesTextures;
		for(int tab = 0; tab < GetSceneWidget()->GetTabCount(); ++tab)
		{
			SceneEditor2 *scene = GetSceneWidget()->GetTabScene(tab);
			SceneHelper::EnumerateSceneTextures(scene, allScenesTextures);
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

	return QMainWindow::eventFilter(obj, event);
}

void QtMainWindow::SetupTitle()
{
	DAVA::KeyedArchive *options = DAVA::Core::Instance()->GetOptions();
	QString title = options->GetString("title").c_str();

	if(ProjectManager::Instance()->IsOpened())
	{
		title += " | Project - ";
		title += ProjectManager::Instance()->CurProjectPath();
	}

	this->setWindowTitle(title);
}

void QtMainWindow::SetupMainMenu()
{
	ui->menuView->addAction(ui->dockSceneInfo->toggleViewAction());
	ui->menuView->addAction(ui->dockLibrary->toggleViewAction());
	ui->menuView->addAction(ui->dockProperties->toggleViewAction());
	ui->menuView->addAction(ui->dockParticleEditor->toggleViewAction());
	ui->menuView->addAction(ui->dockParticleEditorTimeLine->toggleViewAction());
	ui->menuView->addAction(ui->dockSceneTree->toggleViewAction());
	ui->menuView->addAction(ui->dockConsole->toggleViewAction());
	ui->menuView->addAction(ui->dockLODEditor->toggleViewAction());
	ui->menuView->addAction(ui->dockLandscapeEditorControls->toggleViewAction());

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
		ui->sceneToolBar->addSeparator();
		ui->sceneToolBar->addWidget(hangingBtn);
		hangingBtn->setAutoRaise(false);
	}

	// outline by object type
	{
		objectTypesWidget = new QComboBox();
		objectTypesWidget->setMaximumWidth(ResourceEditor::DEFAULT_TOOLBAR_CONTROL_SIZE_WITH_TEXT);
		objectTypesWidget->setMinimumWidth(ResourceEditor::DEFAULT_TOOLBAR_CONTROL_SIZE_WITH_TEXT);

		const QList<QAction *> actions = ui->menuObjectTypes->actions();

		auto endIt = actions.end();
		for(auto it = actions.begin(); it != endIt; ++it)
		{
			if((*it)->isSeparator()) continue;

			objectTypesWidget->addItem((*it)->icon(), (*it)->text());
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
	ui->actionExportPNG->setData(GPU_UNKNOWN);
	
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
	ui->actionReloadPNG->setData(GPU_UNKNOWN);
	QObject::connect(ui->menuTexturesForGPU, SIGNAL(triggered(QAction *)), this, SLOT(OnReloadTexturesTriggered(QAction *)));
	QObject::connect(ui->actionReloadTextures, SIGNAL(triggered()), this, SLOT(OnReloadTextures()));
	QObject::connect(ui->actionReloadSprites, SIGNAL(triggered()), this, SLOT(OnReloadSprites()));

    QObject::connect(ui->actionAlbedo, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
    QObject::connect(ui->actionAmbient, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
    QObject::connect(ui->actionDiffuse, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
    QObject::connect(ui->actionSpecular, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
	
	QObject::connect(ui->actionShowEditorGizmo, SIGNAL(toggled(bool)), this, SLOT(OnEditorGizmoToggle(bool)));
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

	// tools
	QObject::connect(ui->actionMaterialEditor, SIGNAL(triggered()), this, SLOT(OnMaterialEditor()));
	QObject::connect(ui->actionTextureConverter, SIGNAL(triggered()), this, SLOT(OnTextureBrowser()));
	QObject::connect(ui->actionEnableCameraLight, SIGNAL(triggered()), this, SLOT(OnSceneLightMode()));
	QObject::connect(ui->actionCubemapEditor, SIGNAL(triggered()), this, SLOT(OnCubemapEditor()));

	QObject::connect(ui->actionShowNotPassableLandscape, SIGNAL(triggered()), this, SLOT(OnNotPassableTerrain()));
	QObject::connect(ui->actionCustomColorsEditor, SIGNAL(triggered()), this, SLOT(OnCustomColorsEditor()));
	QObject::connect(ui->actionHeightMapEditor, SIGNAL(triggered()), this, SLOT(OnHeightmapEditor()));
	QObject::connect(ui->actionTileMapEditor, SIGNAL(triggered()), this, SLOT(OnTilemaskEditor()));
	QObject::connect(ui->actionVisibilityCheckTool, SIGNAL(triggered()), this, SLOT(OnVisibilityTool()));
	QObject::connect(ui->actionRulerTool, SIGNAL(triggered()), this, SLOT(OnRulerTool()));

	QObject::connect(ui->actionLight, SIGNAL(triggered()), this, SLOT(OnLightDialog()));
	QObject::connect(ui->actionCamera, SIGNAL(triggered()), this, SLOT(OnCameraDialog()));
	QObject::connect(ui->actionAddEmptyEntity, SIGNAL(triggered()), this, SLOT(OnEmptyEntity()));
	QObject::connect(ui->actionUserNode, SIGNAL(triggered()), this, SLOT(OnUserNodeDialog()));
	QObject::connect(ui->actionSwitchNode, SIGNAL(triggered()), this, SLOT(OnSwitchEntityDialog()));
	QObject::connect(ui->actionParticleEffectNode, SIGNAL(triggered()), this, SLOT(OnParticleEffectDialog()));
    QObject::connect(ui->actionEditor_2D_Camera, SIGNAL(triggered()), this, SLOT(OnEditor2DCameraDialog()));
    QObject::connect(ui->actionEditor_Sprite, SIGNAL(triggered()), this, SLOT(OnEditorSpriteDialog()));
	QObject::connect(ui->actionAddNewEntity, SIGNAL(triggered()), this, SLOT(OnAddEntityFromSceneTree()));
	QObject::connect(ui->actionRemoveEntity, SIGNAL(triggered()), ui->sceneTree, SLOT(RemoveSelection()));
	QObject::connect(ui->actionExpandSceneTree, SIGNAL(triggered()), ui->sceneTree, SLOT(expandAll()));
	QObject::connect(ui->actionCollapseSceneTree, SIGNAL(triggered()), ui->sceneTree, SLOT(CollapseAll()));
    QObject::connect(ui->actionAddLandscape, SIGNAL(triggered()), this, SLOT(OnAddLandscape()));
    QObject::connect(ui->actionAddSkybox, SIGNAL(triggered()), this, SLOT(OnAddSkybox()));
			
	QObject::connect(ui->actionShowSettings, SIGNAL(triggered()), this, SLOT(OnShowGeneralSettings()));
	QObject::connect(ui->actionCurrentSceneSettings, SIGNAL(triggered()), this, SLOT(OnShowCurrentSceneSettings()));
	
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

	QObject::connect(ui->actionAddActionComponent, SIGNAL(triggered()), this, SLOT(OnAddActionComponent()));
	QObject::connect(ui->actionAddStaticOcclusionComponent, SIGNAL(triggered()), this, SLOT(OnAddStaticOcclusionComponent()));
	QObject::connect(ui->actionAddQualitySettingsComponent, SIGNAL(triggered()), this, SLOT(OnAddModelTypeComponent()));

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
	QObject::connect(ui->menuObjectTypes, SIGNAL(aboutToShow()), this, SLOT(OnObjectsTypeMenuWillShow()));

	QObject::connect(ui->actionHangingObjects, SIGNAL(triggered()), this, SLOT(OnHangingObjects()));
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

	int32 tools = scene->GetEnabledTools();
	UpdateConflictingActionsState(tools == 0);
}

void QtMainWindow::SceneDeactivated(SceneEditor2 *scene)
{
	// block some actions, when there is no scene
	EnableSceneActions(false);
}

void QtMainWindow::EnableProjectActions(bool enable)
{
	ui->actionNewScene->setEnabled(enable);
	ui->actionOpenScene->setEnabled(enable);
	ui->actionSaveScene->setEnabled(enable);
	ui->actionSaveToFolder->setEnabled(enable);
	ui->actionCubemapEditor->setEnabled(enable);
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
	ui->menuComponent->setEnabled(enable);
	ui->menuScene->setEnabled(enable);
    ui->menuLightView->setEnabled(enable);
    ui->menuTexturesForGPU->setEnabled(enable);
    
    ui->sceneToolBar->setEnabled(enable);
	ui->actionConvertModifiedTextures->setEnabled(enable);
}

void QtMainWindow::SceneCommandExecuted(SceneEditor2 *scene, const Command2* command, bool redo)
{
	if(scene == GetCurrentScene())
	{
		LoadUndoRedoState(scene);
	}
}

// ###################################################################################################
// Mainwindow Qt actions
// ###################################################################################################

void QtMainWindow::OnProjectOpen()
{
    QString newPath = ProjectManager::Instance()->ProjectOpenDialog();
    if(!newPath.isEmpty() && ui->sceneTabWidget->CloseAllTabs())
    {
        ProjectManager::Instance()->ProjectOpen(newPath);
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
	QString path = QtFileDialog::getOpenFileName(this, "Open scene file", ProjectManager::Instance()->CurProjectDataSourcePath(), "DAVA Scene V2 (*.sc2)");
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
	if(gpu >= DAVA::GPU_UNKNOWN && gpu < DAVA::GPU_FAMILY_COUNT)
	{
		SetGPUFormat(gpu);
	}
}

void QtMainWindow::OnReloadSprites()
{
    SpritePackerHelper::Instance()->UpdateParticleSprites((eGPUFamily)SettingsManager::Instance()->
		GetValue(ResourceEditor::SETTINGS_TEXTURE_VIEW_GPU, SettingsManager::INTERNAL).AsInt32());
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

void QtMainWindow::OnMaterialEditor()
{ 
	MaterialEditor::Instance()->show();
}

void QtMainWindow::OnTextureBrowser()
{
	SceneEditor2* sceneEditor = GetCurrentScene();
	EntityGroup selectedEntities;

	if(NULL != sceneEditor)
	{
		selectedEntities = sceneEditor->selectionSystem->GetSelection();
	}

	TextureBrowser::Instance()->show();
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
    float32 defaultLandscapeSize = SettingsManager::Instance()->GetValue("DefaultLandscapeSize", SettingsManager::DEFAULT).AsFloat();
    float32 defaultLandscapeHeight = SettingsManager::Instance()->GetValue("DefaultLandscapeHeight", SettingsManager::DEFAULT).AsFloat();
    
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
	sceneNode->SetName(ResourceEditor::PARTICLE_EFFECT_NODE_NAME);
	SceneEditor2* sceneEditor = GetCurrentScene();
	if(sceneEditor)
	{
		sceneEditor->Exec(new EntityAddCommand(sceneNode, sceneEditor));
		sceneEditor->selectionSystem->SetSelection(sceneNode);
	}
	SafeRelease(sceneNode);
}

void QtMainWindow::OnEditor2DCameraDialog()
{
    Entity* sceneNode = new Entity();
    Camera * camera = new Camera();
    
    float32 w = Core::Instance()->GetVirtualScreenXMax() - Core::Instance()->GetVirtualScreenXMin();
    float32 h = Core::Instance()->GetVirtualScreenYMax() - Core::Instance()->GetVirtualScreenYMin();
    float32 aspect = w / h;
    camera->SetupOrtho(w, aspect, 1, 1000);        
    camera->SetPosition(Vector3(0,0, -500));
    camera->SetTarget(Vector3(0, 0, 0));  
    camera->SetUp(Vector3(0, -1, 0));
    camera->RebuildCameraFromValues();        

    sceneNode->AddComponent(new CameraComponent(camera));
    sceneNode->SetName(ResourceEditor::EDITOR_2D_CAMERA);
    SceneEditor2* sceneEditor = GetCurrentScene();
    if(sceneEditor)
    {
        sceneEditor->Exec(new EntityAddCommand(sceneNode, sceneEditor));
        sceneEditor->selectionSystem->SetSelection(sceneNode);
    }
    SafeRelease(sceneNode);
    SafeRelease(camera);
}
void QtMainWindow::OnEditorSpriteDialog()
{
    FilePath projectPath = FilePath(ProjectManager::Instance()->CurProjectPath().toStdString());
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

void QtMainWindow::OnShowGeneralSettings()
{
	GeneralSettingsDialog t(this);
	t.exec();
}

void QtMainWindow::OnShowCurrentSceneSettings()
{
	SceneSettingsDialog currentSceneSettings(this);
	currentSceneSettings.exec();
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
    int curViewMode = SettingsManager::Instance()->GetValue("materialsLightViewMode", SettingsManager::INTERNAL).AsInt32();

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
			ImageLoader::Save(image, pathToSave);
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
		
		Vector<eGPUFamily> updatedGPUs = it->second;
		WaitSetMessage(descriptor->GetSourceTexturePathname().GetAbsolutePathname().c_str());
		foreach(eGPUFamily gpu, updatedGPUs)
		{
			DAVA::TextureConverter::ConvertTexture(*descriptor, gpu, true);
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

    int32 ret = ShowQuestion("Beast", "The operation will take a lot of time. After lightmaps are generated, scene will be saved. Do you want to proceed?", MB_FLAG_YES | MB_FLAG_NO, MB_FLAG_NO);
    if(ret == MB_FLAG_NO) return;

	if (!SaveTilemask(false))
	{
		return;
	}

	RunBeast();
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

void QtMainWindow::RunBeast()
{
#if defined (__DAVAENGINE_BEAST__)

	SceneEditor2* scene = GetCurrentScene();
	if(!scene) return;

	scene->Exec(new BeastAction(scene, beastWaitDialog));

	OnReloadTextures();

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
	KeyedArchive* customProps = landscape->GetCustomProperties();
	if(NULL == customProps)
	{
		return false;
	}
	
	String pathToSave = selectedPathname.GetRelativePathname(ProjectManager::Instance()->CurProjectPath().toStdString());
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

void QtMainWindow::OnAddActionComponent()
{
	SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;
	
	SceneSelectionSystem *ss = scene->selectionSystem;
	if(ss->GetSelectionCount() > 0)
	{
		scene->BeginBatch("Add Action Component");

		for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
		{
			scene->Exec(new AddComponentCommand(ss->GetSelectionEntity(i), Component::CreateByType(Component::ACTION_COMPONENT)));
		}

		scene->EndBatch();
	}
}

void QtMainWindow::OnAddStaticOcclusionComponent()
{
	SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;
	
	SceneSelectionSystem *ss = scene->selectionSystem;
	if(ss->GetSelectionCount() > 0)
	{
		scene->BeginBatch("Add Static Occlusion Component");
        
		for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
		{
			scene->Exec(new AddComponentCommand(ss->GetSelectionEntity(i), Component::CreateByType(Component::STATIC_OCCLUSION_COMPONENT)));
		}
        
		scene->EndBatch();
	}
}

void QtMainWindow::OnAddModelTypeComponent()
{
	SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;
	
	SceneSelectionSystem *ss = scene->selectionSystem;
	if(ss->GetSelectionCount() > 0)
	{
		scene->BeginBatch("Add Model Type Component");
        
		for(size_t i = 0; i < ss->GetSelectionCount(); ++i)
		{
			scene->Exec(new AddComponentCommand(ss->GetSelectionEntity(i), new QualitySettingsComponent()));
		}
        
		scene->EndBatch();
	}
}

void QtMainWindow::OnBuildStaticOcclusion()
{
    SceneEditor2* scene = GetCurrentScene();
    if(!scene) return;
    
    scene->staticOcclusionBuildSystem->BuildOcclusionInformation();
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


void QtMainWindow::OnObjectsTypeMenuWillShow()
{
	SceneEditor2* scene = GetCurrentScene();
	if(!scene) return;

	ResourceEditor::eSceneObjectType objectType = scene->debugDrawSystem->GetRequestedObjectType();

	ui->actionObjectTypesOff->setChecked(ResourceEditor::ESOT_NONE == objectType);
	ui->actionNoObject->setChecked(ResourceEditor::ESOT_NO_COLISION == objectType);
	ui->actionTree->setChecked(ResourceEditor::ESOT_TREE == objectType);
	ui->actionBush->setChecked(ResourceEditor::ESOT_BUSH == objectType);
	ui->actionFragileProj->setChecked(ResourceEditor::ESOT_FRAGILE_PROJ == objectType);
	ui->actionFragileProjInv->setChecked(ResourceEditor::ESOT_FRAGILE_PROJ_INV == objectType);
	ui->actionFalling->setChecked(ResourceEditor::ESOT_FALLING == objectType);
	ui->actionBuilding->setChecked(ResourceEditor::ESOT_BUILDING == objectType);
	ui->actionInvisibleWall->setChecked(ResourceEditor::ESOT_INVISIBLE_WALL == objectType);
	ui->actionSpeedTree->setChecked(ResourceEditor::ESOT_SPEED_TREE == objectType);
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
		FilePath projectPath(ProjectManager::Instance()->CurProjectPath().toStdString());
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
            else
            {
                QMessageBox::critical(this, "Open scene error.", "Unexpected opening error. See logs for more info.");
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
		int answer = QMessageBox::question(NULL, "Scene was changed", "Do you want to quit anyway?",
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

    if(newMode != SettingsManager::Instance()->GetValue("materialsLightViewMode", SettingsManager::INTERNAL).AsInt32())
    {
        SettingsManager::Instance()->SetValue("materialsLightViewMode", DAVA::VariantType(newMode), SettingsManager::INTERNAL);
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
	ui->actionAddNewComponent->setVisible(false);
	ui->actionRemoveComponent->setVisible(false);
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

bool QtMainWindow::LoadAppropriateTextureFormat()
{
	if (GetGPUFormat() != GPU_UNKNOWN)
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

	return (GetGPUFormat() == GPU_UNKNOWN);
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

