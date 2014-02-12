#include "TileTexturePreviewWidget.h"
#include "TextureBrowser/TextureConvertor.h"
#include "../../Main/QtUtils.h"
#include "StringConstants.h"

#include "TileTexturePreviewWidgetItemDelegate.h"

#include <QHeaderView>
#include <QLabel>
#include <QColorDialog>
#include <QEvent>

TileTexturePreviewWidget::TileTexturePreviewWidget(QWidget* parent)
:	QTreeWidget(parent)
,	selectedTexture(0)
,	mode(MODES_COUNT)
,	validator(NULL)
{
	colors.reserve(DEF_TILE_TEXTURES_COUNT);
	images.reserve(DEF_TILE_TEXTURES_COUNT);
	labels.reserve(DEF_TILE_TEXTURES_COUNT);

	SetMode(MODE_WITH_COLORS);
	ConnectToSignals();

	validator = new QRegExpValidator();
	validator->setRegExp(QRegExp(TileTexturePreviewWidgetItemDelegate::TILE_COLOR_VALIDATE_REGEXP, Qt::CaseInsensitive));
}

TileTexturePreviewWidget::~TileTexturePreviewWidget()
{
	Clear();

	SafeDelete(validator);
}

void TileTexturePreviewWidget::Clear()
{
	clear();

	for (int32 i = 0; i < (int32)images.size(); ++i)
	{
		SafeRelease(images[i]);
	}

	colors.clear();
	images.clear();
	labels.clear();
}

void TileTexturePreviewWidget::AddTexture(Image* previewTexture, const Color& color /*  = Color::White */)
{
	bool blocked = signalsBlocked();
	blockSignals(true);

	images.push_back(SafeRetain(previewTexture));

	QTreeWidgetItem* item = new QTreeWidgetItem();
	item->setCheckState(0, Qt::Unchecked);
	addTopLevelItem(item);

	if (mode == MODE_WITHOUT_COLORS)
	{
		item->setFlags((item->flags() | Qt::ItemIsUserCheckable) & ~(Qt::ItemIsSelectable | Qt::ItemIsEditable));
	}
	else
	{
		item->setFlags((item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable) & ~(Qt::ItemIsSelectable));

		QLabel* label = new QLabel();
		label->setMinimumWidth(26);
		label->setFrameShape(QFrame::Box);
		label->setAutoFillBackground(true);
		setItemWidget(item, COLOR_PREVIEW_COLUMN, label);
		label->setMinimumHeight(TEXTURE_PREVIEW_HEIGHT);
		labels.push_back(label);
		label->installEventFilter(this);
		label->setToolTip(ResourceEditor::TILE_TEXTURE_PREVIEW_CHANGE_COLOR_TOOLTIP.c_str());
		label->setCursor(Qt::PointingHandCursor);

		colors.push_back(color);

		UpdateColor(images.size() - 1);

		this->setItemDelegate(new TileTexturePreviewWidgetItemDelegate());
	}

	UpdateImage(images.size() - 1);
	UpdateSelection();

	blockSignals(blocked);
}

void TileTexturePreviewWidget::InitWithColors()
{
	bool blocked = signalsBlocked();
	blockSignals(true);

	Clear();

	setHeaderHidden(true);
	setColumnCount(2);
	setRootIsDecorated(false);
	header()->setStretchLastSection(false);
	header()->setResizeMode(0, QHeaderView::Stretch);
	header()->setResizeMode(1, QHeaderView::ResizeToContents);
	setIconSize(QSize(TEXTURE_PREVIEW_WIDTH_SMALL, TEXTURE_PREVIEW_HEIGHT));

	blockSignals(blocked);
}

void TileTexturePreviewWidget::InitWithoutColors()
{
	bool blocked = signalsBlocked();
	blockSignals(true);

	Clear();

	setHeaderHidden(true);
	setColumnCount(1);
	setRootIsDecorated(false);
	setIconSize(QSize(TEXTURE_PREVIEW_WIDTH, TEXTURE_PREVIEW_HEIGHT));

	blockSignals(blocked);
}

void TileTexturePreviewWidget::ConnectToSignals()
{
	connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
			this, SLOT(OnCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(OnItemChanged(QTreeWidgetItem*, int)));
}

int32 TileTexturePreviewWidget::GetSelectedTexture()
{
	return selectedTexture;
}

void TileTexturePreviewWidget::SetSelectedTexture(int32 number)
{
	if (number < 0 || number >= (int32)images.size())
	{
		return;
	}

	selectedTexture = number;
	UpdateSelection();

	emit SelectionChanged(selectedTexture);
}

void TileTexturePreviewWidget::UpdateImage(int32 number)
{
	DVASSERT(number >= 0 && number < (int32)images.size());

	QTreeWidgetItem* item = topLevelItem(number);

	Image* image;
	if (mode == MODE_WITH_COLORS)
	{
		image = MultiplyImageWithColor(images[number], colors[number]);
	}
	else
	{
		image = SafeRetain(images[number]);
	}

	QImage qimg = TextureConvertor::FromDavaImage(image);
	SafeRelease(image);

	QSize size = QSize(TEXTURE_PREVIEW_WIDTH, TEXTURE_PREVIEW_HEIGHT);
	if (mode == MODE_WITH_COLORS)
	{
		size.setWidth(TEXTURE_PREVIEW_WIDTH_SMALL);
	}

	QImage previewImage = qimg.copy(0, 0, size.width(), size.height());
	QIcon icon = QIcon(QPixmap::fromImage(previewImage));

	item->setIcon(0, icon);
}

void TileTexturePreviewWidget::UpdateColor(int32 number)
{
	DVASSERT(number >= 0 && number < (int32)images.size());

	QTreeWidgetItem* item = topLevelItem(number);
	QColor color = ColorToQColor(colors[number]);

	QPalette palette = labels[number]->palette();
	palette.setColor(labels[number]->backgroundRole(), color);
	labels[number]->setPalette(palette);

	QString str;
	str.sprintf("#%02x%02x%02x", color.red(), color.green(), color.blue());
	item->setText(0, str);

	UpdateImage(number);
}

void TileTexturePreviewWidget::UpdateSelection()
{
	for (int32 i = 0; i < (int32)images.size(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		item->setCheckState(0, (i == selectedTexture ? Qt::Checked : Qt::Unchecked));
	}
}

void TileTexturePreviewWidget::OnCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
	QModelIndex index = currentIndex();
	selectedTexture = index.row();
	UpdateSelection();

	emit SelectionChanged(selectedTexture);
}

void TileTexturePreviewWidget::OnItemChanged(QTreeWidgetItem* item, int column)
{
	int32 index = indexOfTopLevelItem(item);

	if (mode == MODE_WITH_COLORS)
	{
		int32 len = 0;
		QString str = item->text(0);
		QValidator::State state = validator->validate(str, len);

		if (state == QValidator::Acceptable)
		{
			QString colorString = item->text(0);
			if (!colorString.startsWith("#"))
			{
				colorString = "#" + colorString;
			}

			QColor color = QColor(colorString);
			if (color.isValid())
			{
				Color c = QColorToColor(color);
				if (c != colors[index])
				{
					SetColor(index, c);
				}
			}
		}
	}

	if (item->checkState(0) == Qt::Checked)
	{
		SetSelectedTexture(index);
	}
	else
	{
		UpdateSelection();
	}
}

bool TileTexturePreviewWidget::eventFilter(QObject *obj, QEvent *ev)
{
	for (int32 i = 0; i < (int32)labels.size(); ++i)
	{
		if (obj == labels[i])
		{
			if (ev->type() == QEvent::MouseButtonRelease)
			{
				QColor curColor = ColorToQColor(colors[i]);
				QColor color = QColorDialog::getColor(curColor, this, tr("Tile color"), 0);

				if (color.isValid() && color != curColor)
				{
					SetColor(i, QColorToColor(color));
				}

				return true;
			}
			else if (ev->type() == QEvent::MouseButtonPress)
			{
				return true;
			}
		}
	}

	return QObject::eventFilter(obj, ev);
}

void TileTexturePreviewWidget::SetColor(int32 number, const Color& color)
{
	colors[number] = color;
	emit TileColorChanged(number, colors[number]);
	UpdateColor(number);
}

void TileTexturePreviewWidget::SetMode(TileTexturePreviewWidget::eWidgetModes mode)
{
	if (mode == this->mode)
	{
		return;
	}

	if (mode == MODE_WITH_COLORS)
	{
		InitWithColors();
	}
	else if (mode == MODE_WITHOUT_COLORS)
	{
		InitWithoutColors();
	}
	this->mode = mode;
}

Image* TileTexturePreviewWidget::MultiplyImageWithColor(DAVA::Image *image, const DAVA::Color &color)
{
	uint32 width = image->GetWidth();
	uint32 height = image->GetHeight();

	Texture* srcTexture = Texture::CreateFromData(image->GetPixelFormat(), image->GetData(),
												  width, height, false);
	Sprite* srcSprite = Sprite::CreateFromTexture(srcTexture, 0, 0, width, height);

	Sprite* dstSprite = Sprite::CreateAsRenderTarget(width, height, FORMAT_RGBA8888);

	RenderManager::Instance()->SetRenderTarget(dstSprite);
	RenderManager::Instance()->ClearWithColor(0.f, 0.f, 0.f, 1.f);
	RenderManager::Instance()->SetColor(color);

    Sprite::DrawState drawState;
	drawState.SetPosition(0.f, 0.f);
    drawState.SetRenderState(RenderState::RENDERSTATE_3D_BLEND);
	srcSprite->Draw(&drawState);

	RenderManager::Instance()->ResetColor();
	RenderManager::Instance()->RestoreRenderTarget();

	Image* res = dstSprite->GetTexture()->CreateImageFromMemory(RenderState::RENDERSTATE_3D_BLEND);

	SafeRelease(dstSprite);
	SafeRelease(srcSprite);
	SafeRelease(srcTexture);

	return res;
}

void TileTexturePreviewWidget::UpdateColor(int32 index, const Color& color)
{
	if (index < 0 || index >= (int32)colors.size())
	{
		return;
	}

	if (colors[index] != color)
	{
		SetColor(index, color);
	}
}
