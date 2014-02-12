#include "TilemaskEditorPanel.h"
#include "../../Scene/SceneSignals.h"
#include "../../Scene/SceneEditor2.h"
#include "../../Tools/TileTexturePreviewWidget/TileTexturePreviewWidget.h"
#include "../../Tools/SliderWidget/SliderWidget.h"
#include "Constants.h"
#include "../LandscapeEditorShortcutManager.h"

#include <QLayout>
#include <QComboBox>
#include <QRadioButton>
#include <QLabel>

TilemaskEditorPanel::TilemaskEditorPanel(QWidget* parent)
:	LandscapeEditorBasePanel(parent)
,	sliderWidgetBrushSize(NULL)
,	sliderWidgetStrength(NULL)
,	comboBrushImage(NULL)
,	radioDraw(NULL)
,	radioCopyPaste(NULL)
,	frameStrength(NULL)
,	frameTileTexturesPreview(NULL)
{
	const DAVA::RenderStateData& default3dState = DAVA::RenderManager::Instance()->GetRenderStateData(DAVA::RenderState::RENDERSTATE_3D_BLEND);
	DAVA::RenderStateData noBlendStateData;
	memcpy(&noBlendStateData, &default3dState, sizeof(noBlendStateData));
	
	noBlendStateData.sourceFactor = DAVA::BLEND_ONE;
	noBlendStateData.destFactor = DAVA::BLEND_ZERO;
	
	noBlendDrawState = DAVA::RenderManager::Instance()->CreateRenderState(noBlendStateData);

	InitUI();
	ConnectToSignals();
}

TilemaskEditorPanel::~TilemaskEditorPanel()
{
	RenderManager::Instance()->ReleaseRenderState(noBlendDrawState);
}

bool TilemaskEditorPanel::GetEditorEnabled()
{
	return GetActiveScene()->tilemaskEditorSystem->IsLandscapeEditingEnabled();
}

void TilemaskEditorPanel::SetWidgetsState(bool enabled)
{
	sliderWidgetBrushSize->setEnabled(enabled);
	sliderWidgetStrength->setEnabled(enabled);
	comboBrushImage->setEnabled(enabled);
	tileTexturePreviewWidget->setEnabled(enabled);
	radioDraw->setEnabled(enabled);
	radioCopyPaste->setEnabled(enabled);
}

void TilemaskEditorPanel::BlockAllSignals(bool block)
{
	sliderWidgetBrushSize->blockSignals(block);
	sliderWidgetStrength->blockSignals(block);
	comboBrushImage->blockSignals(block);
	tileTexturePreviewWidget->blockSignals(block);
	radioDraw->blockSignals(block);
	radioCopyPaste->blockSignals(block);
}

void TilemaskEditorPanel::InitUI()
{
	QVBoxLayout* layout = new QVBoxLayout(this);

	sliderWidgetBrushSize = new SliderWidget(this);
	sliderWidgetStrength = new SliderWidget(this);
	comboBrushImage = new QComboBox(this);
	tileTexturePreviewWidget = new TileTexturePreviewWidget(this);
	radioDraw = new QRadioButton(this);
	radioCopyPaste = new QRadioButton(this);

	QLabel* labelBrushImageDesc = new QLabel(this);
	QLabel* labelTileTextureDesc = new QLabel(this);
	QFrame* frameBrushImage = new QFrame(this);
	QSpacerItem* spacer = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
	QHBoxLayout* layoutBrushImage = new QHBoxLayout();
	QVBoxLayout* layoutTileTexture = new QVBoxLayout();

	QHBoxLayout* layoutBrushSize = new QHBoxLayout();
	QLabel* labelBrushSize = new QLabel();
	labelBrushSize->setText(ResourceEditor::TILEMASK_EDITOR_BRUSH_SIZE_CAPTION.c_str());
	layoutBrushSize->addWidget(labelBrushSize);
	layoutBrushSize->addWidget(sliderWidgetBrushSize);

	QGridLayout* layoutDrawTypes = new QGridLayout();
	layoutDrawTypes->addWidget(radioDraw, 0, 0);
	layoutDrawTypes->addWidget(radioCopyPaste, 0, 1);

	QHBoxLayout* layoutStrength = new QHBoxLayout();
	QLabel* labelStrength = new QLabel();
	labelStrength->setText(ResourceEditor::TILEMASK_EDITOR_STRENGTH_CAPTION.c_str());
	layoutStrength->addWidget(labelStrength);
	layoutStrength->addWidget(sliderWidgetStrength);
	frameStrength = new QFrame(this);
	frameStrength->setLayout(layoutStrength);

	layoutBrushImage->addWidget(labelBrushImageDesc);
	layoutBrushImage->addWidget(comboBrushImage);
	layoutTileTexture->addWidget(labelTileTextureDesc);
	layoutTileTexture->QLayout::addWidget(tileTexturePreviewWidget);
	frameTileTexturesPreview = new QFrame(this);
	frameTileTexturesPreview->setLayout(layoutTileTexture);

	frameBrushImage->setLayout(layoutBrushImage);

	layout->addLayout(layoutBrushSize);
	layout->addWidget(frameBrushImage);
	layout->addLayout(layoutDrawTypes);
	layout->addWidget(frameStrength);
	layout->addWidget(frameTileTexturesPreview);
	layout->addSpacerItem(spacer);

	setLayout(layout);

	SetWidgetsState(false);
	BlockAllSignals(true);

	comboBrushImage->setMinimumHeight(44);
	labelBrushImageDesc->setText(ResourceEditor::TILEMASK_EDITOR_BRUSH_IMAGE_CAPTION.c_str());
	labelBrushImageDesc->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
	labelTileTextureDesc->setText(ResourceEditor::TILEMASK_EDITOR_TILE_TEXTURE_CAPTION.c_str());
	labelTileTextureDesc->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

	sliderWidgetBrushSize->Init(false, DEF_BRUSH_MAX_SIZE, DEF_BRUSH_MIN_SIZE, DEF_BRUSH_MIN_SIZE);
	sliderWidgetBrushSize->SetRangeBoundaries(ResourceEditor::BRUSH_MIN_BOUNDARY, ResourceEditor::BRUSH_MAX_BOUNDARY);
	sliderWidgetStrength->Init(false, DEF_STRENGTH_MAX_VALUE, DEF_STRENGTH_MIN_VALUE, DEF_STRENGTH_MIN_VALUE);
	sliderWidgetStrength->SetRangeBoundaries(DEF_STRENGTH_MIN_VALUE, STRENGTH_MAX_BOUNDARY);

	radioDraw->setText(ResourceEditor::TILEMASK_EDITOR_DRAW_CAPTION.c_str());
	radioCopyPaste->setText(ResourceEditor::TILEMASK_EDITOR_COPY_PASTE_CAPTION.c_str());

	tileTexturePreviewWidget->setMaximumHeight(160);

	layoutBrushImage->setContentsMargins(0, 0, 0, 0);
	layoutTileTexture->setContentsMargins(0, 0, 0, 0);
	layoutStrength->setContentsMargins(0, 0, 0, 0);

	InitBrushImages();
}

void TilemaskEditorPanel::ConnectToSignals()
{
	connect(SceneSignals::Instance(), SIGNAL(TilemaskEditorToggled(SceneEditor2*)),
			this, SLOT(EditorToggled(SceneEditor2*)));
	connect(SceneSignals::Instance(), SIGNAL(CommandExecuted(SceneEditor2*, const Command2*, bool)),
			this, SLOT(OnCommandExecuted(SceneEditor2*, const Command2*, bool)));

	connect(sliderWidgetBrushSize, SIGNAL(ValueChanged(int)), this, SLOT(SetBrushSize(int)));
	connect(sliderWidgetStrength, SIGNAL(ValueChanged(int)), this, SLOT(SetStrength(int)));
	connect(comboBrushImage, SIGNAL(currentIndexChanged(int)), this, SLOT(SetToolImage(int)));
	connect(tileTexturePreviewWidget, SIGNAL(SelectionChanged(int)), this, SLOT(SetDrawTexture(int)));
	connect(tileTexturePreviewWidget, SIGNAL(TileColorChanged(int32, Color)),
			this, SLOT(OnTileColorChanged(int32, Color)));

	connect(radioDraw, SIGNAL(clicked()), this, SLOT(SetNormalDrawing()));
	connect(radioCopyPaste, SIGNAL(clicked()), this, SLOT(SetCopyPaste()));
}

void TilemaskEditorPanel::StoreState()
{
	KeyedArchive* customProperties = GetActiveScene()->GetCustomProperties();
	if (customProperties)
	{
		customProperties->SetInt32(ResourceEditor::TILEMASK_EDITOR_BRUSH_SIZE_MIN,
								   (int32)sliderWidgetBrushSize->GetRangeMin());
		customProperties->SetInt32(ResourceEditor::TILEMASK_EDITOR_BRUSH_SIZE_MAX,
								   (int32)sliderWidgetBrushSize->GetRangeMax());
		customProperties->SetInt32(ResourceEditor::TILEMASK_EDITOR_STRENGTH_MIN,
								   (int32)sliderWidgetStrength->GetRangeMin());
		customProperties->SetInt32(ResourceEditor::TILEMASK_EDITOR_STRENGTH_MAX,
								   (int32)sliderWidgetStrength->GetRangeMax());
	}
}

void TilemaskEditorPanel::RestoreState()
{
	SceneEditor2* sceneEditor = GetActiveScene();

	bool enabled = sceneEditor->tilemaskEditorSystem->IsLandscapeEditingEnabled();
	int32 brushSize = BrushSizeSystemToUI(sceneEditor->tilemaskEditorSystem->GetBrushSize());
	int32 strength = StrengthSystemToUI(sceneEditor->tilemaskEditorSystem->GetStrength());
	uint32 tileTexture = sceneEditor->tilemaskEditorSystem->GetTileTextureIndex();
	int32 toolImage = sceneEditor->tilemaskEditorSystem->GetToolImage();

	int32 brushRangeMin = DEF_BRUSH_MIN_SIZE;
	int32 brushRangeMax = DEF_BRUSH_MAX_SIZE;
	int32 strRangeMin = DEF_STRENGTH_MIN_VALUE;
	int32 strRangeMax = DEF_STRENGTH_MAX_VALUE;

	KeyedArchive* customProperties = sceneEditor->GetCustomProperties();
	if (customProperties)
	{
		brushRangeMin = customProperties->GetInt32(ResourceEditor::TILEMASK_EDITOR_BRUSH_SIZE_MIN, DEF_BRUSH_MIN_SIZE);
		brushRangeMax = customProperties->GetInt32(ResourceEditor::TILEMASK_EDITOR_BRUSH_SIZE_MAX, DEF_BRUSH_MAX_SIZE);
		strRangeMin = customProperties->GetInt32(ResourceEditor::TILEMASK_EDITOR_STRENGTH_MIN, DEF_STRENGTH_MIN_VALUE);
		strRangeMax = customProperties->GetInt32(ResourceEditor::TILEMASK_EDITOR_STRENGTH_MAX, DEF_STRENGTH_MAX_VALUE);
	}

	SetWidgetsState(enabled);

	BlockAllSignals(true);
	sliderWidgetBrushSize->SetRangeMin(brushRangeMin);
	sliderWidgetBrushSize->SetRangeMax(brushRangeMax);
	sliderWidgetBrushSize->SetValue(brushSize);

	sliderWidgetStrength->SetRangeMin(strRangeMin);
	sliderWidgetStrength->SetRangeMax(strRangeMax);
	sliderWidgetStrength->SetValue(strength);

	UpdateTileTextures();
	tileTexturePreviewWidget->SetSelectedTexture(tileTexture);
	comboBrushImage->setCurrentIndex(toolImage);

	UpdateControls();

	BlockAllSignals(!enabled);
}

void TilemaskEditorPanel::OnEditorEnabled()
{
	UpdateTileTextures();

	SetToolImage(comboBrushImage->currentIndex());
}

void TilemaskEditorPanel::InitBrushImages()
{
	comboBrushImage->clear();

	QSize iconSize = comboBrushImage->iconSize();
	iconSize = iconSize.expandedTo(QSize(32, 32));
	comboBrushImage->setIconSize(iconSize);

	FilePath toolsPath(ResourceEditor::TILEMASK_EDITOR_TOOLS_PATH);
	FileList *fileList = new FileList(toolsPath);
	for(int32 iFile = 0; iFile < fileList->GetCount(); ++iFile)
	{
		String filename = fileList->GetFilename(iFile);
		if(fileList->GetPathname(iFile).IsEqualToExtension(".png"))
		{
			String fullname = fileList->GetPathname(iFile).GetAbsolutePathname();

			FilePath f = fileList->GetPathname(iFile);
			f.ReplaceExtension("");

			QString qFullname = QString::fromStdString(fullname);
			QIcon toolIcon(qFullname);
			comboBrushImage->addItem(toolIcon, f.GetFilename().c_str(), QVariant(qFullname));
		}
	}

	SafeRelease(fileList);
}

void TilemaskEditorPanel::SplitImageToChannels(Image* image, Image*& r, Image*& g, Image*& b, Image*& a)
{
	if (image->GetPixelFormat() != FORMAT_RGBA8888)
	{
		uint32 width = image->GetWidth();
		uint32 height = image->GetHeight();

		Texture* t = Texture::CreateFromData(image->GetPixelFormat(), image->GetData(),
											 width, height, false);
		Sprite* s = Sprite::CreateFromTexture(t, 0, 0, width, height);

		Sprite* sprite = Sprite::CreateAsRenderTarget(width, height, FORMAT_RGBA8888);
		RenderManager::Instance()->SetRenderTarget(sprite);
        
        Sprite::DrawState drawState;
		drawState.SetPosition(0.f, 0.f);
        drawState.SetRenderState(noBlendDrawState);
		s->Draw(&drawState);
		RenderManager::Instance()->RestoreRenderTarget();

		image = sprite->GetTexture()->CreateImageFromMemory(noBlendDrawState);
		image->ResizeCanvas(width, height);

		SafeRelease(sprite);
		SafeRelease(s);
		SafeRelease(t);
	}
	else
	{
		image->Retain();
	}

	const int32 CHANNELS_COUNT = 4;

	uint32 width = image->GetWidth();
	uint32 height = image->GetHeight();
	int32 size = width * height;

	Image* images[CHANNELS_COUNT];
	for (int32 i = 0; i < CHANNELS_COUNT; ++i)
	{
		images[i] = Image::Create(width, height, FORMAT_RGBA8888);
	}

	int32 pixelSize = Texture::GetPixelFormatSizeInBytes(FORMAT_RGBA8888);
	for(int32 i = 0; i < size; ++i)
	{
		int32 offset = i * pixelSize;
        
		images[0]->data[offset] = image->data[offset];
		images[0]->data[offset + 1] = image->data[offset];
		images[0]->data[offset + 2] = image->data[offset];
		images[0]->data[offset + 3] = 255;
        
		images[1]->data[offset] = image->data[offset + 1];
		images[1]->data[offset + 1] = image->data[offset + 1];
		images[1]->data[offset + 2] = image->data[offset + 1];
		images[1]->data[offset + 3] = 255;

		images[2]->data[offset] = image->data[offset + 2];
		images[2]->data[offset + 1] = image->data[offset + 2];
		images[2]->data[offset + 2] = image->data[offset + 2];
		images[2]->data[offset + 3] = 255;
        
		images[3]->data[offset] = image->data[offset + 3];
		images[3]->data[offset + 1] = image->data[offset + 3];
		images[3]->data[offset + 2] = image->data[offset + 3];
		images[3]->data[offset + 3] = 255;
    }

	r = images[0];
	g = images[1];
	b = images[2];
	a = images[3];

	image->Release();
}

void TilemaskEditorPanel::UpdateTileTextures()
{
	tileTexturePreviewWidget->Clear();

	SceneEditor2* sceneEditor = GetActiveScene();

	QSize iconSize = QSize(TileTexturePreviewWidget::TEXTURE_PREVIEW_WIDTH,
						   TileTexturePreviewWidget::TEXTURE_PREVIEW_HEIGHT);

	int32 count = (int32)sceneEditor->tilemaskEditorSystem->GetTileTextureCount();
	Image** images = new Image*[count];

    Image* image = sceneEditor->tilemaskEditorSystem->GetTileTexture(0)->CreateImageFromMemory(noBlendDrawState);
    
    image->ResizeCanvas(iconSize.width(), iconSize.height());
    
    SplitImageToChannels(image, images[0], images[1], images[2], images[3]);
    SafeRelease(image);
    
    tileTexturePreviewWidget->SetMode(TileTexturePreviewWidget::MODE_WITH_COLORS);
    
	for (int32 i = 0; i < count; ++i)
	{
		Color color = sceneEditor->tilemaskEditorSystem->GetTileColor(i);

		tileTexturePreviewWidget->AddTexture(images[i], color);

		SafeRelease(images[i]);
	}

	SafeDelete(images);
}

void TilemaskEditorPanel::SetBrushSize(int brushSize)
{
	GetActiveScene()->tilemaskEditorSystem->SetBrushSize(BrushSizeUIToSystem(brushSize));
}

void TilemaskEditorPanel::SetToolImage(int imageIndex)
{
	QString s = comboBrushImage->itemData(imageIndex).toString();
	
	if (!s.isEmpty())
	{
		FilePath fp(s.toStdString());
		GetActiveScene()->tilemaskEditorSystem->SetToolImage(fp, imageIndex);
	}
}

void TilemaskEditorPanel::SetStrength(int strength)
{
	GetActiveScene()->tilemaskEditorSystem->SetStrength(StrengthUIToSystem(strength));
}

void TilemaskEditorPanel::SetDrawTexture(int textureIndex)
{
	GetActiveScene()->tilemaskEditorSystem->SetTileTexture(textureIndex);
}

// these functions are designed to convert values from sliders in ui
// to the values suitable for tilemask editor system
int32 TilemaskEditorPanel::BrushSizeUIToSystem(int32 uiValue)
{
	int32 systemValue = uiValue * ResourceEditor::LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF;
	return systemValue;
}

int32 TilemaskEditorPanel::BrushSizeSystemToUI(int32 systemValue)
{
	int32 uiValue = systemValue / ResourceEditor::LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF;
	return uiValue;
}

float32 TilemaskEditorPanel::StrengthUIToSystem(int32 uiValue)
{
	// strength in tilemask editor is the real number in the range [0 .. 0.5]
	// (by default, upper bound could be different)
	float32 max = 2.0 * DEF_STRENGTH_MAX_VALUE;
	float32 systemValue = uiValue / max;
	return systemValue;
}

int32 TilemaskEditorPanel::StrengthSystemToUI(float32 systemValue)
{
	int32 uiValue = (int32)(systemValue * 2.f * DEF_STRENGTH_MAX_VALUE);
	return uiValue;
}
// end of convert functions ==========================

void TilemaskEditorPanel::ConnectToShortcuts()
{
	LandscapeEditorShortcutManager* shortcutManager = LandscapeEditorShortcutManager::Instance();

	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_SMALL), SIGNAL(activated()),
			this, SLOT(IncreaseBrushSize()));
	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_SMALL), SIGNAL(activated()),
			this, SLOT(DecreaseBrushSize()));
	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_LARGE), SIGNAL(activated()),
			this, SLOT(IncreaseBrushSizeLarge()));
	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_LARGE), SIGNAL(activated()),
			this, SLOT(DecreaseBrushSizeLarge()));

	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_INCREASE_SMALL), SIGNAL(activated()),
			this, SLOT(IncreaseStrength()));
	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_DECREASE_SMALL), SIGNAL(activated()),
			this, SLOT(DecreaseStrength()));
	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_INCREASE_LARGE), SIGNAL(activated()),
			this, SLOT(IncreaseStrengthLarge()));
	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_DECREASE_LARGE), SIGNAL(activated()),
			this, SLOT(DecreaseStrengthLarge()));

	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_TEXTURE_NEXT), SIGNAL(activated()),
			this, SLOT(NextTexture()));
	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_TEXTURE_PREV), SIGNAL(activated()),
			this, SLOT(PrevTexture()));

	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_IMAGE_NEXT), SIGNAL(activated()),
			this, SLOT(NextTool()));
	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_IMAGE_PREV), SIGNAL(activated()),
			this, SLOT(PrevTool()));

	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_NORMAL_DRAW_TILEMASK), SIGNAL(activated()),
			this, SLOT(SetNormalDrawing()));
	connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_COPY_PASTE_TILEMASK), SIGNAL(activated()),
			this, SLOT(SetCopyPaste()));
	
	shortcutManager->SetTileMaskEditorShortcutsEnabled(true);
	shortcutManager->SetBrushSizeShortcutsEnabled(true);
	shortcutManager->SetStrengthShortcutsEnabled(true);
	shortcutManager->SetTextureSwitchingShortcutsEnabled(true);
	shortcutManager->SetBrushImageSwitchingShortcutsEnabled(true);
}

void TilemaskEditorPanel::DisconnectFromShortcuts()
{
	LandscapeEditorShortcutManager* shortcutManager = LandscapeEditorShortcutManager::Instance();

	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_SMALL), SIGNAL(activated()),
			   this, SLOT(IncreaseBrushSize()));
	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_SMALL), SIGNAL(activated()),
			   this, SLOT(DecreaseBrushSize()));
	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_LARGE), SIGNAL(activated()),
			   this, SLOT(IncreaseBrushSizeLarge()));
	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_LARGE), SIGNAL(activated()),
			   this, SLOT(DecreaseBrushSizeLarge()));

	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_INCREASE_SMALL), SIGNAL(activated()),
			   this, SLOT(IncreaseStrength()));
	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_DECREASE_SMALL), SIGNAL(activated()),
			   this, SLOT(DecreaseStrength()));
	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_INCREASE_LARGE), SIGNAL(activated()),
			   this, SLOT(IncreaseStrengthLarge()));
	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_DECREASE_LARGE), SIGNAL(activated()),
			   this, SLOT(DecreaseStrengthLarge()));

	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_TEXTURE_NEXT), SIGNAL(activated()),
			   this, SLOT(NextTexture()));
	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_TEXTURE_PREV), SIGNAL(activated()),
			   this, SLOT(PrevTexture()));

	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_IMAGE_NEXT), SIGNAL(activated()),
			   this, SLOT(NextTool()));
	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_IMAGE_PREV), SIGNAL(activated()),
			   this, SLOT(PrevTool()));

	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_NORMAL_DRAW_TILEMASK), SIGNAL(activated()),
			   this, SLOT(SetNormalDrawing()));
	disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_COPY_PASTE_TILEMASK), SIGNAL(activated()),
			   this, SLOT(SetCopyPaste()));
	
	shortcutManager->SetTileMaskEditorShortcutsEnabled(false);
	shortcutManager->SetBrushSizeShortcutsEnabled(false);
	shortcutManager->SetStrengthShortcutsEnabled(false);
	shortcutManager->SetTextureSwitchingShortcutsEnabled(false);
	shortcutManager->SetBrushImageSwitchingShortcutsEnabled(false);
}

void TilemaskEditorPanel::IncreaseBrushSize()
{
	sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
									+ ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void TilemaskEditorPanel::DecreaseBrushSize()
{
	sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
									- ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void TilemaskEditorPanel::IncreaseBrushSizeLarge()
{
	sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
									+ ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void TilemaskEditorPanel::DecreaseBrushSizeLarge()
{
	sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
									- ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void TilemaskEditorPanel::IncreaseStrength()
{
	sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
								   + ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void TilemaskEditorPanel::DecreaseStrength()
{
	sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
								   - ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void TilemaskEditorPanel::IncreaseStrengthLarge()
{
	sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
								   + ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void TilemaskEditorPanel::DecreaseStrengthLarge()
{
	sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
								   - ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void TilemaskEditorPanel::PrevTexture()
{
	int32 curIndex = tileTexturePreviewWidget->GetSelectedTexture();
	if (curIndex)
	{
		tileTexturePreviewWidget->SetSelectedTexture(curIndex - 1);
	}
}

void TilemaskEditorPanel::NextTexture()
{
	SceneEditor2* sceneEditor = GetActiveScene();

	int32 curIndex = tileTexturePreviewWidget->GetSelectedTexture();
	if (curIndex < (int32)sceneEditor->tilemaskEditorSystem->GetTileTextureCount() - 1)
	{
		tileTexturePreviewWidget->SetSelectedTexture(curIndex + 1);
	}
}

void TilemaskEditorPanel::PrevTool()
{
	int32 curIndex = comboBrushImage->currentIndex();
	if (curIndex)
	{
		comboBrushImage->setCurrentIndex(curIndex - 1);
	}
}

void TilemaskEditorPanel::NextTool()
{
	int32 curIndex = comboBrushImage->currentIndex();
	if (curIndex < comboBrushImage->count() - 1)
	{
		comboBrushImage->setCurrentIndex(curIndex + 1);
	}
}

void TilemaskEditorPanel::OnTileColorChanged(int32 tileNumber, Color color)
{
	SceneEditor2* sceneEditor = GetActiveScene();
	sceneEditor->tilemaskEditorSystem->SetTileColor(tileNumber, color);
}

void TilemaskEditorPanel::OnCommandExecuted(SceneEditor2* scene, const Command2* command, bool redo)
{
	if (scene != GetActiveScene() || !GetEditorEnabled() || command->GetId() != CMDID_SET_TILE_COLOR)
	{
		return;
	}

	SceneEditor2* sceneEditor = GetActiveScene();
	int32 count = (int32)sceneEditor->tilemaskEditorSystem->GetTileTextureCount();

	for (int32 i = 0; i < count; ++i)
	{
		Color color = sceneEditor->tilemaskEditorSystem->GetTileColor(i);
		tileTexturePreviewWidget->UpdateColor(i, color);
	}
}

void TilemaskEditorPanel::SetNormalDrawing()
{
	GetActiveScene()->tilemaskEditorSystem->SetDrawingType(TilemaskEditorSystem::TILEMASK_DRAW_NORMAL);
	UpdateControls();
}

void TilemaskEditorPanel::SetCopyPaste()
{
	GetActiveScene()->tilemaskEditorSystem->SetDrawingType(TilemaskEditorSystem::TILEMASK_DRAW_COPY_PASTE);
	UpdateControls();
}

void TilemaskEditorPanel::UpdateControls()
{
	TilemaskEditorSystem::eTilemaskDrawType type = GetActiveScene()->tilemaskEditorSystem->GetDrawingType();

	bool blocked = radioDraw->signalsBlocked();
	radioDraw->blockSignals(true);
	radioCopyPaste->blockSignals(true);

	radioDraw->setChecked(type == TilemaskEditorSystem::TILEMASK_DRAW_NORMAL);
	radioCopyPaste->setChecked(type == TilemaskEditorSystem::TILEMASK_DRAW_COPY_PASTE);
	frameTileTexturesPreview->setVisible(type == TilemaskEditorSystem::TILEMASK_DRAW_NORMAL);
	frameStrength->setVisible(type == TilemaskEditorSystem::TILEMASK_DRAW_NORMAL);

	radioDraw->blockSignals(blocked);
	radioCopyPaste->blockSignals(blocked);
}
