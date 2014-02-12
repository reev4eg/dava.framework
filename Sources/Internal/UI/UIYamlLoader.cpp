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


#include "UI/UIYamlLoader.h"
#include "Base/ObjectFactory.h"
#include "Platform/SystemTimer.h"
#include "UI/UIControl.h"
#include "FileSystem/YamlParser.h"
#include "FileSystem/FileSystem.h"
#include "Render/2D/GraphicsFont.h"
#include "Render/2D/FontManager.h"
#include "Render/2D/TextBlock.h"

namespace DAVA 
{
UIYamlLoader::UIYamlLoader() :
	BaseObject()
{
	// Default mode is to ASSERT if custom control isn't found.
	assertIfCustomControlNotFound = true;

	currentPath = FilePath();
}

int32 UIYamlLoader::GetDrawTypeFromNode(const YamlNode * drawTypeNode)
{
    int32 ret = UIControlBackground::DRAW_ALIGNED;
    if(!drawTypeNode)
        return ret;
    
    const String & type = drawTypeNode->AsString();
    
    if("DRAW_ALIGNED" == type) ret = UIControlBackground::DRAW_ALIGNED;
    if("DRAW_SCALE_TO_RECT" == type) ret = UIControlBackground::DRAW_SCALE_TO_RECT;
    if("DRAW_SCALE_PROPORTIONAL" == type) ret = UIControlBackground::DRAW_SCALE_PROPORTIONAL;
    if("DRAW_SCALE_PROPORTIONAL_ONE" == type) ret = UIControlBackground::DRAW_SCALE_PROPORTIONAL_ONE;
    if("DRAW_FILL" == type) ret = UIControlBackground::DRAW_FILL;
    if("DRAW_STRETCH_HORIZONTAL" == type) ret = UIControlBackground::DRAW_STRETCH_HORIZONTAL;
    if("DRAW_STRETCH_VERTICAL" == type) ret = UIControlBackground::DRAW_STRETCH_VERTICAL;
    if("DRAW_STRETCH_BOTH" == type) ret = UIControlBackground::DRAW_STRETCH_BOTH;
	if("DRAW_TILED" == type) ret = UIControlBackground::DRAW_TILED;
    
    return ret;
}

String UIYamlLoader::GetDrawTypeNodeValue(int32 drawType)
{
    String ret;
    switch (drawType) {
        case UIControlBackground::DRAW_ALIGNED:
            ret = "DRAW_ALIGNED";
            break;
        case UIControlBackground::DRAW_SCALE_TO_RECT:
            ret = "DRAW_SCALE_TO_RECT";
            break;
        case UIControlBackground::DRAW_SCALE_PROPORTIONAL:
            ret = "DRAW_SCALE_PROPORTIONAL";
            break;
        case UIControlBackground::DRAW_SCALE_PROPORTIONAL_ONE:
            ret = "DRAW_SCALE_PROPORTIONAL_ONE";
            break;
        case UIControlBackground::DRAW_FILL:
            ret = "DRAW_FILL";
            break;
        case UIControlBackground::DRAW_STRETCH_HORIZONTAL:
            ret = "DRAW_STRETCH_HORIZONTAL";
            break;
        case UIControlBackground::DRAW_STRETCH_VERTICAL:
            ret = "DRAW_STRETCH_VERTICAL";
            break;
        case UIControlBackground::DRAW_STRETCH_BOTH:
            ret = "DRAW_STRETCH_BOTH";
			break;
		case UIControlBackground::DRAW_TILED:
            ret = "DRAW_TILED";
			break;
        default:
            ret = "DRAW_ALIGNED";
            break;
    }
    return ret;
}
    
int32 UIYamlLoader::GetColorInheritTypeFromNode(const YamlNode * colorInheritNode)
{
	int32 ret = UIControlBackground::COLOR_IGNORE_PARENT;
	if(!colorInheritNode)
		return ret;

	const String & type = colorInheritNode->AsString();

	if("COLOR_MULTIPLY_ON_PARENT" == type) ret = UIControlBackground::COLOR_MULTIPLY_ON_PARENT;
	if("COLOR_ADD_TO_PARENT" == type) ret = UIControlBackground::COLOR_ADD_TO_PARENT;
	if("COLOR_REPLACE_TO_PARENT" == type) ret = UIControlBackground::COLOR_REPLACE_TO_PARENT;
	if("COLOR_IGNORE_PARENT" == type) ret = UIControlBackground::COLOR_IGNORE_PARENT;
	if("COLOR_MULTIPLY_ALPHA_ONLY" == type) ret = UIControlBackground::COLOR_MULTIPLY_ALPHA_ONLY;
	if("COLOR_REPLACE_ALPHA_ONLY" == type) ret = UIControlBackground::COLOR_REPLACE_ALPHA_ONLY;

	return ret;
}
	
String UIYamlLoader::GetColorInheritTypeNodeValue(int32 colorInheritType)
{
    String ret;    
    switch (colorInheritType) {
        case UIControlBackground::COLOR_MULTIPLY_ON_PARENT:
            ret = "COLOR_MULTIPLY_ON_PARENT";
            break;
        case UIControlBackground::COLOR_ADD_TO_PARENT:
            ret = "COLOR_ADD_TO_PARENT";
            break;
        case UIControlBackground::COLOR_REPLACE_TO_PARENT:
            ret = "COLOR_REPLACE_TO_PARENT";
            break;
        case UIControlBackground::COLOR_IGNORE_PARENT:
            ret = "COLOR_IGNORE_PARENT";
            break;
        case UIControlBackground::COLOR_MULTIPLY_ALPHA_ONLY:
            ret = "COLOR_MULTIPLY_ALPHA_ONLY";
            break;
        case UIControlBackground::COLOR_REPLACE_ALPHA_ONLY:
            ret = "COLOR_REPLACE_ALPHA_ONLY";
            break;
        default:
            ret = "COLOR_IGNORE_PARENT";
            break;
    }
    return ret;
}
	
int32 UIYamlLoader::GetAlignFromYamlNode(const YamlNode * alignNode)
{
	if (!alignNode)return ALIGN_HCENTER | ALIGN_VCENTER;
	
	const Vector<YamlNode*> & vec = alignNode->AsVector();
	if (vec.size() != 2)return ALIGN_HCENTER | ALIGN_VCENTER;
	
	const String & horzAlign = vec[0]->AsString();
	const String & vertAlign = vec[1]->AsString();
	
	int32 align = 0;
	if (horzAlign == "LEFT")align |= ALIGN_LEFT;
	else if (horzAlign == "HCENTER")align |= ALIGN_HCENTER;
	else if (horzAlign == "RIGHT")align |= ALIGN_RIGHT;
	
	if (vertAlign == "TOP")align |= ALIGN_TOP;
	else if (vertAlign == "VCENTER")align |= ALIGN_VCENTER;
	else if (vertAlign == "BOTTOM")align |= ALIGN_BOTTOM;
	
	return align;
}

int32 UIYamlLoader::GetFittingOptionFromYamlNode( const YamlNode * fittingNode ) const
{
    int32 fitting = TextBlock::FITTING_DISABLED;

    const Vector<YamlNode*> & vec = fittingNode->AsVector();

    for( uint32 index = 0; index < vec.size(); ++index )
    {
        const String &value = vec[index]->AsString();
        if( value == "DISABLED" )
        {
            fitting = TextBlock::FITTING_DISABLED;
            break;
        }
        else if( value == "ENLARGE" )
        {
            fitting |= TextBlock::FITTING_ENLARGE;
        }
        else if( value == "REDUCE" )
        {
            fitting |= TextBlock::FITTING_REDUCE;
        }
        else if( value == "POINTS" )
        {
            fitting |= TextBlock::FITTING_POINTS;
        }
    }

    return fitting;
}

//Vector<String> UIYamlLoader::GetAlignNodeValue(int32 align)
YamlNode * UIYamlLoader::GetAlignNodeValue(int32 align)
{
	YamlNode *alignNode = new YamlNode(YamlNode::TYPE_ARRAY);
	String horzAlign = "HCENTER";
	String vertAlign = "VCENTER";
	
	if (align & ALIGN_LEFT)
	{
		horzAlign = "LEFT";
	}
	else if (align & ALIGN_HCENTER)
	{
		horzAlign = "HCENTER";
	}
	else if (align & ALIGN_RIGHT)
	{
		horzAlign = "RIGHT";
	}
	
	if (align & ALIGN_TOP)
	{
		vertAlign = "TOP";
	}
	else if (align & ALIGN_VCENTER)
	{
		vertAlign = "VCENTER";
	}
	else if (align & ALIGN_BOTTOM)
	{
		vertAlign = "BOTTOM";
	}
	
	alignNode->AddValueToArray(horzAlign);
	alignNode->AddValueToArray(vertAlign);
	
	return alignNode;
}

YamlNode * UIYamlLoader::GetFittingOptionNodeValue( int32 fitting ) const
{
    YamlNode *fittingNode = new YamlNode(YamlNode::TYPE_ARRAY);

    if( fitting == TextBlock::FITTING_DISABLED )
    {
        fittingNode->AddValueToArray("DISABLED");
    }
    else
    {
        if( fitting & TextBlock::FITTING_ENLARGE )
        {
            fittingNode->AddValueToArray("ENLARGE");
        }
        if( fitting & TextBlock::FITTING_REDUCE )
        {
            fittingNode->AddValueToArray("REDUCE");
        }
        if( fitting & TextBlock::FITTING_POINTS )
        {
            fittingNode->AddValueToArray("POINTS");
        }
    }
    return fittingNode;
}

bool UIYamlLoader::GetBoolFromYamlNode(const YamlNode * node, bool defaultValue)
{
	if (!node)return defaultValue;
	
	const String & value = node->AsString();
	if (value == "yes")return true;
	else if (value == "no")return false;
	else if (value == "true")return true;
	
	return false;
}

int32 HexCharToInt(char c)
{
	if ((c >= '0') && (c <= '9'))return c - '0';
	else if ((c >= 'a') && (c <= 'f'))return c - 'a' + 10;
	else if ((c >= 'A') && (c <= 'F'))return c - 'A' + 10;
	return 0;
}

Color UIYamlLoader::GetColorFromYamlNode(const YamlNode * node)
{
	if (node->GetType() == YamlNode::TYPE_ARRAY)
	{
		if (node->GetCount() == 4)
			return node->AsColor();
		else return Color::White;
	}else
	{
		const String & color = node->AsString();
		
		int r = HexCharToInt(color[0]) * 16 + HexCharToInt(color[1]);
		int g = HexCharToInt(color[2]) * 16 + HexCharToInt(color[3]);
		int b = HexCharToInt(color[4]) * 16 + HexCharToInt(color[5]);
		int a = HexCharToInt(color[6]) * 16 + HexCharToInt(color[7]);
		
		return Color((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
	}
}
	

Font * UIYamlLoader::GetFontByName(const String & fontName)
{
	Map<String, Font*>::iterator it = fontMap.find(fontName);
	if (it != fontMap.end())
	{
		Font * font = it->second;
		return font;
	}
	return 0;
}

void UIYamlLoader::Load(UIControl * rootControl, const FilePath & yamlPathname, bool assertIfCustomControlNotFound)
{
	UIYamlLoader * loader = new UIYamlLoader();
	loader->SetAssertIfCustomControlNotFound(assertIfCustomControlNotFound);

	loader->ProcessLoad(rootControl, yamlPathname);

	loader->Release();
}

bool UIYamlLoader::Save(UIControl * rootControl, const FilePath & yamlPathname, bool skipRootNode)
{
	UIYamlLoader * loader = new UIYamlLoader();

	bool savedOK = loader->ProcessSave(rootControl, yamlPathname, skipRootNode);

	loader->Release();
	return savedOK;
}

void UIYamlLoader::ProcessLoad(UIControl * rootControl, const FilePath & yamlPathname)
{
	uint64 t1 = SystemTimer::Instance()->AbsoluteMS();
	YamlParser * parser = YamlParser::Create(yamlPathname);
	if (!parser)
	{
		Logger::Error("Failed to open yaml file: %s", yamlPathname.GetAbsolutePathname().c_str());
		return;
	}
	currentPath = yamlPathname.GetDirectory();

	YamlNode * rootNode = parser->GetRootNode();
    if (!rootNode)
    {
        // Empty YAML file.
        Logger::Warning("yaml file: %s is empty", yamlPathname.GetAbsolutePathname().c_str());
        return;
    }
	
	for (MultiMap<String, YamlNode*>::const_iterator t = rootNode->AsMap().begin(); t != rootNode->AsMap().end(); ++t)
	{
		YamlNode * node = t->second;
		const YamlNode * typeNode = node->Get("type");
		if (!typeNode)continue;
		
		const String & type = typeNode->AsString();
		if (type == "FTFont")
		{
			// parse font
			const YamlNode * fontNameNode = node->Get("name");
			if (!fontNameNode)continue;
			
			float32 fontSize = 10.0f;
			const YamlNode * fontSizeNode = node->Get("size");
			if (fontSizeNode)fontSize = fontSizeNode->AsFloat();
			
			FTFont * font = FTFont::Create(fontNameNode->AsString());
            if (!font)
            {
                continue;
            }

			font->SetSize(fontSize);
			
            const YamlNode * fontVerticalSpacingNode = node->Get("verticalSpacing");
            if(fontVerticalSpacingNode)
            {
                font->SetVerticalSpacing(fontVerticalSpacingNode->AsInt());
            }
            
			fontMap[t->first] = font;
			FontManager::Instance()->SetFontName(font, t->first);
		}
		else if(type == "GraphicsFont")
		{
			// parse font
			const YamlNode * fontNameNode = node->Get("sprite");
			if (!fontNameNode)continue;

			const YamlNode * definitionNode = node->Get("definition");
			if (!definitionNode)continue;

			GraphicsFont * font = GraphicsFont::Create(definitionNode->AsString(), fontNameNode->AsString());
            if (!font)
            {
                continue;
            }

			const YamlNode * fontSizeNode = node->Get("size");
			if (fontSizeNode)
			{
				font->SetSize(fontSizeNode->AsFloat());
			}
            
            const YamlNode * fontVerticalSpacingNode = node->Get("verticalSpacing");
            if(fontVerticalSpacingNode)
            {
                font->SetVerticalSpacing(fontVerticalSpacingNode->AsInt());
            }
            
            const YamlNode * fontHorizontalSpacingNode = node->Get("horizontalSpacing");
            if(fontHorizontalSpacingNode)
            {
                font->SetHorizontalSpacing(fontHorizontalSpacingNode->AsInt());
            }

			fontMap[t->first] = font;
			FontManager::Instance()->SetFontName(font, t->first);
		}
	}
	
	LoadFromNode(rootControl, rootNode, false);
	SafeRelease(parser);

	// After the scene is fully loaded, apply the align settings
	// to position child controls correctly.
	rootControl->ApplyAlignSettingsForChildren();
	
	for (Map<String, Font *>::iterator t = fontMap.begin(); t != fontMap.end(); ++t)
	{
		Font * font = t->second;
		SafeRelease(font);
	}
	fontMap.clear();
	uint64 t2 = SystemTimer::Instance()->AbsoluteMS();
	Logger::FrameworkDebug("Load of %s time: %lld", yamlPathname.GetAbsolutePathname().c_str(), t2 - t1);
}
	
bool UIYamlLoader::ProcessSave(UIControl * rootControl, const FilePath & yamlPathname, bool skipRootNode)
{
    uint64 t1 = SystemTimer::Instance()->AbsoluteMS();
    YamlParser * parser = YamlParser::Create();

    if (!parser)
    {
        Logger::Error("ProcessSave: error while creating YAML parser!");
        return false;
    }
    
    DVASSERT(rootControl);
    YamlNode* resultNode = SaveToNode(rootControl, NULL);

	uint32 fileAttr = File::CREATE | File::WRITE;
	//save used fonts
	const FontManager::TRACKED_FONTS& usedFonts = FontManager::Instance()->GetTrackedFont();
	ScopedPtr<YamlNode> fontsNode( new YamlNode(YamlNode::TYPE_MAP) );
	for (FontManager::TRACKED_FONTS::const_iterator iter = usedFonts.begin();
		 iter != usedFonts.end();
		 ++iter)
	{
		Font* font = (*iter);
		if (!font)
			continue;
		
		// The font should be stored once only.
        String fontName = FontManager::Instance()->GetFontName(font);
		if (fontsNode->AsMap().find(fontName) == fontsNode->AsMap().end())
		{
            fontsNode->AddNodeToMap( fontName, font->SaveToYamlNode() );
		}
	}

	//resultNode
	parser->SaveToYamlFile(yamlPathname, fontsNode, true, File::CREATE | File::WRITE);
	fileAttr = File::APPEND | File::WRITE;
	
    // Save the resulting YAML file to the path passed.
    bool savedOK = parser->SaveToYamlFile(yamlPathname, resultNode, skipRootNode, fileAttr);
    SafeRelease(parser);
    
    SafeRelease(resultNode);
    
    uint64 t2 = SystemTimer::Instance()->AbsoluteMS();
	Logger::FrameworkDebug("Save of %s time: %lld", yamlPathname.GetAbsolutePathname().c_str(), t2 - t1);

    return savedOK;
}
	
void UIYamlLoader::LoadFromNode(UIControl * parentControl, const YamlNode * rootNode, bool needParentCallback)
{
	//for (Map<String, YamlNode*>::iterator t = rootNode->AsMap().begin(); t != rootNode->AsMap().end(); ++t)
	int cnt = rootNode->GetCount();
	for (int k = 0; k < cnt; ++k)
	{
		const YamlNode * node = rootNode->Get(k);
		const YamlNode * typeNode = node->Get("type");
		if (!typeNode)continue;
		const String & type = typeNode->AsString();
		if (type == "FTFont")continue;
		if (type == "GraphicsFont")continue;

		// Base Type might be absent.
		const YamlNode* baseTypeNode = node->Get("baseType");
		const String baseType = baseTypeNode ? baseTypeNode->AsString() : String();

		// The control can be loaded either from its Type or from Base Type (depending on
		// whether the control is custom or not.
		UIControl* control = CreateControl(type, baseType);
		if (!control)
		{
			Logger::Warning("ObjectFactory haven't found object with type:%s, base type %s", type.c_str(), baseType.c_str());
			continue;
		}else
		{
			//Logger::FrameworkDebug("Create control with type:%s", type.c_str());
		}
		control->LoadFromYamlNode(node, this);
		parentControl->AddControl(control);
		LoadFromNode(control, node, true);
		control->SetName(rootNode->GetItemKeyName(k));
		SafeRelease(control);
	}
    
    if(needParentCallback)
	{
        parentControl->LoadFromYamlNodeCompleted();   
    }
}

UIControl* UIYamlLoader::CreateControl(const String& type, const String& baseType)
{
	// Firstly try Type (Custom Control).
	UIControl * control = dynamic_cast<UIControl*> (ObjectFactory::Instance()->New<UIControl>(type));
	if (control)
	{
		// Everything is OK. Just update the custom control type for the control, if any.
		bool hasCustomType = (!type.empty() && !baseType.empty() && (type != baseType));
		if (hasCustomType)
		{
			control->SetCustomControlType(type);
		}

		return control;
	}

	// The control can't be loaded by its type - probably it is Custom Control and we are
	// running under UIEditor or other app which doesn't support custom controls. Verify this.
	if (this->assertIfCustomControlNotFound)
	{
		Logger::Error("Unable to load UI Control %s and 'ASSERT if Custom Control Not Found' flag is set to TRUE", type.c_str());
		DVASSERT(false);
	}

	// Retry with base type, if any.
	if (!baseType.empty())
	{
		control = dynamic_cast<UIControl*> (ObjectFactory::Instance()->New<UIControl>(baseType));
		if (control)
		{
			// Even if the control of the base type was created, we have to store its custom type.
			control->SetCustomControlType(type);
		}
	}

	// A NULL might be here too.
	return control;
}

YamlNode* UIYamlLoader::SaveToNode(UIControl * parentControl, YamlNode * parentNode,
                                   int saveIndex)
{
    // Save ourselves and all children.
    YamlNode* childNode = parentControl->SaveToYamlNode(this);
    if (parentNode && parentNode->GetType() == YamlNode::TYPE_MAP)
    {
        parentNode->AddNodeToMap(parentControl->GetName(), childNode);
    }

    SaveChildren(parentControl, childNode, saveIndex);

    return childNode;
}

void UIYamlLoader::SaveChildren(UIControl* parentControl, YamlNode * parentNode, int saveIndex)
{
    // "Relative Depth" is needed to save the order of the nodes - it is important!
    parentNode->Set(YamlNode::SAVE_INDEX_NAME, saveIndex);
    
    int currentSaveIndex = 0;
    
	const List<UIControl*>& children = parentControl->GetRealChildren();
	for (List<UIControl*>::const_iterator childIter = children.begin(); childIter != children.end(); childIter ++)
    {
        UIControl* childControl = (*childIter);
        SaveToNode(childControl, parentNode, currentSaveIndex);
        currentSaveIndex ++;
    }
}

void UIYamlLoader::SetAssertIfCustomControlNotFound(bool value)
{
	this->assertIfCustomControlNotFound = value;
}

const FilePath & UIYamlLoader::GetCurrentPath() const
{
	return currentPath;
}
	
}