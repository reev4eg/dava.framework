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


#ifndef __UIEditor__TextControlNodeMetadata__
#define __UIEditor__TextControlNodeMetadata__

#include "UIControlMetadata.h"
#include "UI/UIStaticText.h"

#include <QColor>

namespace DAVA {

// Metadata class for DAVA text-aware controls, responsible for the localization.
class UITextControlMetadata : public UIControlMetadata
{
    Q_OBJECT

    // Font Properties
    Q_PROPERTY(Font* Font READ GetFont WRITE SetFont);
    Q_PROPERTY(float FontSize READ GetFontSize WRITE SetFontSize);
    Q_PROPERTY(QColor FontColor READ GetFontColor WRITE SetFontColor);

    Q_PROPERTY(QString LocalizedTextKey READ GetLocalizedTextKey WRITE SetLocalizedTextKey);
	
	Q_PROPERTY(float ShadowOffsetX READ GetShadowOffsetX WRITE SetShadowOffsetX);
	Q_PROPERTY(float ShadowOffsetY READ GetShadowOffsetY WRITE SetShadowOffsetY);
	Q_PROPERTY(QColor ShadowColor READ GetShadowColor WRITE SetShadowColor);
	Q_PROPERTY(int TextAlign READ GetTextAlign WRITE SetTextAlign);
    Q_PROPERTY(int FittingType READ GetFittingType WRITE SetFittingType);

public:
    UITextControlMetadata(QObject* parent = 0);
    QString GetLocalizedTextValue();
    
protected:
    virtual QString GetLocalizedTextKey() const;
    virtual void SetLocalizedTextKey(const QString& value);
    
    // Getters/setters.
    virtual Font * GetFont() = 0;
    virtual void SetFont(Font * font) = 0;

	virtual int GetTextAlign() = 0;
    virtual void SetTextAlign(int align) = 0;
    
    virtual float GetFontSize() const = 0;
    virtual void SetFontSize(float fontSize) = 0;

    virtual QColor GetFontColor() const = 0;
    virtual void SetFontColor(const QColor& value) = 0;
	
	virtual float GetShadowOffsetX() const = 0;
	virtual void SetShadowOffsetX(float offset) = 0;
	
	virtual float GetShadowOffsetY() const = 0;
	virtual void SetShadowOffsetY(float offset) = 0;
	
	virtual QColor GetShadowColor() const = 0;
	virtual void SetShadowColor(const QColor& value) = 0;
	
	Vector2 GetOffsetX(const Vector2& currentOffset, float offsetX);
	Vector2 GetOffsetY(const Vector2& currentOffset, float offsetY);

    // These methods should not be overriden for the UITextField, that's why not pure virtual.
    virtual int GetFittingType() const;
    virtual void SetFittingType(int value);

    // Get the localized text for particular control state.
    QString GetLocalizedTextKeyForState(UIControl::eControlState controlState) const;
    
    // Update the static text extra data based on update style.
    void UpdateStaticTextExtraData(UIStaticText* staticText, UIControl::eControlState state,
                                   HierarchyTreeNodeExtraData& extraData,
                                   eExtraDataUpdateStyle updateStyle);
};

};

#endif /* defined(__UIEditor__TextControlNodeMetadata__)*/
