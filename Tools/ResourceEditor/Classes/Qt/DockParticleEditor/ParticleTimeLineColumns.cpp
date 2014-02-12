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



#include "ParticleTimeLineColumns.h"

ParticlesExtraInfoColumn::ParticlesExtraInfoColumn(const ParticleTimeLineWidget* timeLineWidget,
												   QWidget *parent) :
QWidget(parent)
{
	this->timeLineWidget = timeLineWidget;
}

void ParticlesExtraInfoColumn::paintEvent(QPaintEvent *)
{
	if (!this->timeLineWidget)
	{
		return;
	}
	
	QPainter painter(this);
	painter.setPen(Qt::black);
	
	QRect ourRect = rect();
	ourRect.adjust(0, 0, -1, -1);
	painter.drawRect(ourRect);
	
	// Draw the header.
	painter.setFont(timeLineWidget->nameFont);
	painter.setPen(Qt::black);
	QRect textRect(0, 0, rect().width(), TOP_INDENT);
	painter.drawRect(textRect);
	painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, GetExtraInfoHeader());
	
	// Draw the per-layer particles count.
	OnBeforeGetExtraInfoLoop();
	
	QFontMetrics fontMetrics(timeLineWidget->nameFont);
	painter.setFont(timeLineWidget->nameFont);
	
	int32 i = 0;
	for (ParticleTimeLineWidget::LINE_MAP::const_iterator iter = timeLineWidget->lines.begin();
		 iter != timeLineWidget->lines.end(); ++iter, ++i)
	{
		const ParticleTimeLineWidget::LINE& line = iter->second;
		
		painter.setPen(QPen(line.color, LINE_WIDTH));
		int startY = i * LINE_STEP + LINE_STEP / 2;
		QRect textRect (EXTRA_INFO_LEFT_PADDING, TOP_INDENT + startY,
						rect().width() - EXTRA_INFO_LEFT_PADDING, LINE_STEP);
		painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
						 GetExtraInfoForLayerLine(line));
	}
	
	OnAfterGetExtraInfoLoop();
	
	// Draw the "Total" box.
	QPoint totalPoint(EXTRA_INFO_LEFT_PADDING, rect().bottom() - 3);
	QFont totalFont = timeLineWidget->nameFont;
	totalFont.setBold(true);
	
	painter.setPen(QPen(Qt::black, LINE_WIDTH));
	painter.drawText(totalPoint, GetExtraInfoFooter());
}

QString ParticlesExtraInfoColumn::FormatFloat(float32 value)
{
	QString strValue;
	if (fabs(value) < 10)
	{
		strValue = "%.4f";
	}
	else if (fabs(value) < 100)
	{
		strValue = "%.2f";
	}
	else
	{
		strValue = "%.0f";
	}
	
	strValue.sprintf(strValue.toAscii(), value);
	return strValue;
}

////////////////////////////////////////////////////////////////////////////////////

ParticlesExtraInfoCumulativeColumn::ParticlesExtraInfoCumulativeColumn(const ParticleTimeLineWidget* timeLineWidget,
																	   QWidget *parent) :
	ParticlesExtraInfoColumn(timeLineWidget, parent)
{
	CleanupCumulativeData();
}

void ParticlesExtraInfoCumulativeColumn::OnLayersListChanged()
{
	CleanupCumulativeData();
}

void ParticlesExtraInfoCumulativeColumn::UpdateCumulativeData(ParticleLayer* layer, float32 value)
{
	if (!layer)
	{
		return;
	}
	
	if (cumulativeData.find(layer) == cumulativeData.end())
	{
		cumulativeData[layer] = value;
	}
	else
	{
		cumulativeData[layer] += value;
	}
}

void ParticlesExtraInfoCumulativeColumn::UpdateCumulativeDataIfMaximum(ParticleLayer* layer, float32 value)
{
	if (!layer)
	{
		return;
	}

	if ((cumulativeData.find(layer) == cumulativeData.end()) ||
		(value > cumulativeData[layer]))
	{
		cumulativeData[layer] =  value;
	}
}

void ParticlesExtraInfoCumulativeColumn::CleanupCumulativeData()
{
	this->totalParticlesCount = 0;
	this->totalUpdatesCount = 0;
	this->totalParticlesArea = 0.0f;

	cumulativeData.clear();
}

////////////////////////////////////////////////////////////////////////////////////
ParticlesCountColumn::ParticlesCountColumn(const ParticleTimeLineWidget* timeLineWidget,
										   QWidget *parent) :
ParticlesExtraInfoColumn(timeLineWidget, parent)
{
	this->totalParticlesCount = 0;
}

void ParticlesCountColumn::OnBeforeGetExtraInfoLoop()
{
	this->totalParticlesCount = 0;
}

QString ParticlesCountColumn::GetExtraInfoForLayerLine(const ParticleTimeLineWidget::LINE& line)
{
	if (!line.layer)
	{
		return QString();
	}
	
	//int32 particlesNumber = line.layer->GetActiveParticlesCount();
	int32 particlesNumber = 0; //TODO: later think how to restore functionality
	this->totalParticlesCount += particlesNumber;
	
	return QString::number(particlesNumber);
}

QString ParticlesCountColumn::GetExtraInfoHeader()
{
	return "Count";
}

QString ParticlesCountColumn::GetExtraInfoFooter()
{
	return QString::number(this->totalParticlesCount);
}

////////////////////////////////////////////////////////////////////////////////////
ParticlesAverageCountColumn::ParticlesAverageCountColumn(const ParticleTimeLineWidget* timeLineWidget,
												 QWidget *parent) :
ParticlesExtraInfoCumulativeColumn(timeLineWidget, parent)
{
}

void ParticlesAverageCountColumn::Reset()
{
	CleanupCumulativeData();
}

QString ParticlesAverageCountColumn::GetExtraInfoForLayerLine(const ParticleTimeLineWidget::LINE& line)
{
	if (!line.layer)
	{
		return QString();
	}

	// Calculate the cumulative info.
	this->totalUpdatesCount ++;

	//int32 particlesNumber = line.layer->GetActiveParticlesCount();
	int32 particlesNumber = 0; //TODO: later think how to restore functionality
	UpdateCumulativeData(line.layer, particlesNumber);
	this->totalParticlesCount += particlesNumber;

	return FormatFloat(cumulativeData[line.layer] / (float)totalUpdatesCount);
}

QString ParticlesAverageCountColumn::GetExtraInfoHeader()
{
	return "Avg Cnt";
}

QString ParticlesAverageCountColumn::GetExtraInfoFooter()
{
	if (this->totalUpdatesCount == 0)
	{
		return FormatFloat(0);
	}
	else
	{
		return FormatFloat((float)totalParticlesCount / (float)totalUpdatesCount);
	}
}

////////////////////////////////////////////////////////////////////////////////////

ParticlesMaxCountColumn::ParticlesMaxCountColumn(const ParticleTimeLineWidget* timeLineWidget,
														 QWidget *parent) :
ParticlesExtraInfoCumulativeColumn(timeLineWidget, parent)
{
	Reset();
}

void ParticlesMaxCountColumn::OnLayersListChanged()
{
	ParticlesExtraInfoCumulativeColumn::OnLayersListChanged();
	Reset();
}

void ParticlesMaxCountColumn::Reset()
{
	CleanupCumulativeData();
	this->maxParticlesCount = 0;
	this->totalParticlesCountOnThisLoop = 0;
}

void ParticlesMaxCountColumn::OnBeforeGetExtraInfoLoop()
{
	this->totalParticlesCountOnThisLoop = 0;
}

QString ParticlesMaxCountColumn::GetExtraInfoForLayerLine(const ParticleTimeLineWidget::LINE& line)
{
	if (!line.layer)
	{
		return QString();
	}

	// Calculate the cumulative info.
	//int32 particlesCount = line.layer->GetActiveParticlesCount();
	int32 particlesNumber = 0; //TODO: later think how to restore functionality
	UpdateCumulativeDataIfMaximum(line.layer, particlesNumber);
	totalParticlesCountOnThisLoop += particlesNumber;
	
	return QString::number((int)cumulativeData[line.layer]);
}

void ParticlesMaxCountColumn::OnAfterGetExtraInfoLoop()
{
	if (maxParticlesCount < totalParticlesCountOnThisLoop)
	{
		maxParticlesCount = totalParticlesCountOnThisLoop;
	}
}

QString ParticlesMaxCountColumn::GetExtraInfoHeader()
{
	return "Max Cnt";
}

QString ParticlesMaxCountColumn::GetExtraInfoFooter()
{
	return QString::number((int)maxParticlesCount);
}

////////////////////////////////////////////////////////////////////////////////////

ParticlesAreaColumn::ParticlesAreaColumn(const ParticleTimeLineWidget* timeLineWidget,
										 QWidget *parent) :
ParticlesExtraInfoColumn(timeLineWidget, parent)
{
	this->totalParticlesArea = 0.0f;
}

void ParticlesAreaColumn::OnBeforeGetExtraInfoLoop()
{
	this->totalParticlesArea = 0;
}

QString ParticlesAreaColumn::GetExtraInfoForLayerLine(const ParticleTimeLineWidget::LINE& line)
{
	if (!line.layer)
	{
		return QString();
	}
	
	//float32 area = line.layer->GetActiveParticlesArea();
	float32 area = 0; //TODO: later think how to restore functionality
	this->totalParticlesArea += area;
	
	return FormatFloat(area);
}

QString ParticlesAreaColumn::GetExtraInfoHeader()
{
	return "Area";
}

QString ParticlesAreaColumn::GetExtraInfoFooter()
{
	return FormatFloat(this->totalParticlesArea);
}

////////////////////////////////////////////////////////////////////////////////////

ParticlesAverageAreaColumn::ParticlesAverageAreaColumn(const ParticleTimeLineWidget* timeLineWidget,
														 QWidget *parent) :
ParticlesExtraInfoCumulativeColumn(timeLineWidget, parent)
{
}

void ParticlesAverageAreaColumn::Reset()
{
	CleanupCumulativeData();
}

QString ParticlesAverageAreaColumn::GetExtraInfoForLayerLine(const ParticleTimeLineWidget::LINE& line)
{
	if (!line.layer)
	{
		return QString();
	}
	
	// Calculate the cumulative info.
	this->totalUpdatesCount ++;

	//float32 area = line.layer->GetActiveParticlesArea();
	float32 area = 0; //TODO: later think how to restore functionality
	UpdateCumulativeData(line.layer, area);
	this->totalParticlesArea += area;

	return FormatFloat(cumulativeData[line.layer] / (float)totalUpdatesCount);
}

QString ParticlesAverageAreaColumn::GetExtraInfoHeader()
{
	return "Avg Area";
}

QString ParticlesAverageAreaColumn::GetExtraInfoFooter()
{
	if (this->totalUpdatesCount == 0)
	{
		return FormatFloat(0);
	}
	else
	{
		return FormatFloat(totalParticlesArea / (float)totalUpdatesCount);
	}
}

////////////////////////////////////////////////////////////////////////////////////

ParticlesMaxAreaColumn::ParticlesMaxAreaColumn(const ParticleTimeLineWidget* timeLineWidget,
											   QWidget *parent) :
ParticlesExtraInfoCumulativeColumn(timeLineWidget, parent)
{
	Reset();
}

void ParticlesMaxAreaColumn::OnLayersListChanged()
{
	ParticlesExtraInfoCumulativeColumn::OnLayersListChanged();
	Reset();
}

void ParticlesMaxAreaColumn::Reset()
{
	CleanupCumulativeData();
	maxParticlesArea = 0;
	totalParticlesAreaOnThisLoop = 0;
}

void ParticlesMaxAreaColumn::OnBeforeGetExtraInfoLoop()
{
	totalParticlesAreaOnThisLoop = 0;
}

QString ParticlesMaxAreaColumn::GetExtraInfoForLayerLine(const ParticleTimeLineWidget::LINE& line)
{
	if (!line.layer)
	{
		return QString();
	}
	
	// Calculate the cumulative info.
	//float32 particlesArea = line.layer->GetActiveParticlesArea();
	float32 area = 0; //TODO: later think how to restore functionality
	UpdateCumulativeDataIfMaximum(line.layer, area);
	totalParticlesAreaOnThisLoop += area;
	
	return FormatFloat((float)cumulativeData[line.layer]);
}

void ParticlesMaxAreaColumn::OnAfterGetExtraInfoLoop()
{
	if (maxParticlesArea < totalParticlesAreaOnThisLoop)
	{
		maxParticlesArea = totalParticlesAreaOnThisLoop;
	}
}

QString ParticlesMaxAreaColumn::GetExtraInfoHeader()
{
	return "Max Area";
}

QString ParticlesMaxAreaColumn::GetExtraInfoFooter()
{
	return FormatFloat(maxParticlesArea);
}
