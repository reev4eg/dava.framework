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



#include "Commands2/CustomColorsCommands2.h"
#include "../Qt/Scene/System/LandscapeEditorDrawSystem/CustomColorsProxy.h"

#include "../Qt/Scene/SceneEditor2.h"
#include "../Qt/Scene/SceneSignals.h"

#include "../Qt/Main/QtUtils.h"

ActionEnableCustomColors::ActionEnableCustomColors(SceneEditor2* forSceneEditor)
:	CommandAction(CMDID_CUSTOM_COLORS_ENABLE)
,	sceneEditor(forSceneEditor)
{
}

void ActionEnableCustomColors::Redo()
{
	if (sceneEditor == NULL)
	{
		return;
	}
	
	bool enabled = sceneEditor->customColorsSystem->IsLandscapeEditingEnabled();
	if (enabled)
	{
		return;
	}
	
	sceneEditor->DisableTools(SceneEditor2::LANDSCAPE_TOOLS_ALL);

	bool success = !sceneEditor->IsToolsEnabled(SceneEditor2::LANDSCAPE_TOOLS_ALL);
	if (!success )
	{
		ShowErrorDialog(ResourceEditor::LANDSCAPE_EDITOR_SYSTEM_DISABLE_EDITORS);
	}
	
	LandscapeEditorDrawSystem::eErrorType enablingError = sceneEditor->customColorsSystem->EnableLandscapeEditing();
	if (enablingError != LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
	{
		ShowErrorDialog(LandscapeEditorDrawSystem::GetDescriptionByError(enablingError));
	}
    if (!sceneEditor->landscapeEditorDrawSystem->GetCustomColorsProxy()->IsTextureLoaded())
    {
        ShowErrorDialog(LandscapeEditorDrawSystem::GetDescriptionByError(LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_CUSTOMCOLORS_ABSENT));
        sceneEditor->landscapeEditorDrawSystem->GetCustomColorsProxy()->ResetLoadedState();
    }
    
    if(success &&
       LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS == enablingError)
    {
        sceneEditor->foliageSystem->SetFoliageVisible(false);
    }

	SceneSignals::Instance()->EmitCustomColorsToggled(sceneEditor);
}

ActionDisableCustomColors::ActionDisableCustomColors(SceneEditor2* forSceneEditor, bool textureSavingNeeded)
:	CommandAction(CMDID_CUSTOM_COLORS_DISABLE)
,	sceneEditor(forSceneEditor)
,	textureSavingNeeded(textureSavingNeeded)
{
}

void ActionDisableCustomColors::Redo()
{
	if (sceneEditor == NULL)
	{
		return;
	}
	
	bool disabled = !sceneEditor->customColorsSystem->IsLandscapeEditingEnabled();
	if (disabled)
	{
		return;
	}
	
	bool success = sceneEditor->customColorsSystem->DisableLandscapeEdititing(textureSavingNeeded);
	if (!success)
	{
		ShowErrorDialog(ResourceEditor::CUSTOM_COLORS_DISABLE_ERROR);
	}
    
    if(success)
    {
        sceneEditor->foliageSystem->SetFoliageVisible(true);
    }
    
	SceneSignals::Instance()->EmitCustomColorsToggled(sceneEditor);
}

ModifyCustomColorsCommand::ModifyCustomColorsCommand(Image* originalImage,
													 CustomColorsProxy* customColorsProxy,
													 const Rect& updatedRect)
:	Command2(CMDID_CUSTOM_COLORS_MODIFY, "Custom Colors Modification")
{
	this->updatedRect = updatedRect;
	this->customColorsProxy = SafeRetain(customColorsProxy);
	
	Image* currentImage = customColorsProxy->GetSprite()->GetTexture()->CreateImageFromMemory(RenderState::RENDERSTATE_2D_BLEND);
	
	undoImage = Image::CopyImageRegion(originalImage, updatedRect);
	redoImage = Image::CopyImageRegion(currentImage, updatedRect);
	
	SafeRelease(currentImage);
}

ModifyCustomColorsCommand::~ModifyCustomColorsCommand()
{
	SafeRelease(undoImage);
	SafeRelease(redoImage);
	SafeRelease(customColorsProxy);
}

void ModifyCustomColorsCommand::Undo()
{
	ApplyImage(undoImage);
	customColorsProxy->DecrementChanges();
}

void ModifyCustomColorsCommand::Redo()
{
	ApplyImage(redoImage);
	customColorsProxy->IncrementChanges();
}

void ModifyCustomColorsCommand::ApplyImage(DAVA::Image *image)
{
	Sprite* customColorsSprite = customColorsProxy->GetSprite();
	
	Texture* texture = Texture::CreateFromData(image->GetPixelFormat(), image->GetData(),
											   image->GetWidth(), image->GetHeight(), false);
	Sprite* sprite = Sprite::CreateFromTexture(texture, 0, 0, (float32)texture->GetWidth(), (float32)texture->GetHeight());
	
	RenderManager::Instance()->SetRenderTarget(customColorsSprite);
	
	RenderManager::Instance()->ClipPush();
	RenderManager::Instance()->SetClip(updatedRect);

    Sprite::DrawState drawState;
    drawState.SetPosition(updatedRect.x, updatedRect.y);
	sprite->Draw(&drawState);
	
	RenderManager::Instance()->ClipPop();
	RenderManager::Instance()->RestoreRenderTarget();
	
	customColorsProxy->UpdateRect(updatedRect);
	
	SafeRelease(sprite);
	SafeRelease(texture);
}

Entity* ModifyCustomColorsCommand::GetEntity() const
{
	return NULL;
}
