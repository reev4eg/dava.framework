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



#include "SceneSaver.h"
#include "Deprecated/SceneValidator.h"

#include "Qt/Scene/SceneHelper.h"

#include "Classes/StringConstants.h"
#include "Classes/Qt/Main/QtUtils.h"

#include "Scene3D/Components/CustomPropertiesComponent.h"

using namespace DAVA;

SceneSaver::SceneSaver()
    : copyConverted(false)
{
}

SceneSaver::~SceneSaver()
{
    ReleaseTextures();
}


void SceneSaver::SetInFolder(const FilePath &folderPathname)
{
    sceneUtils.SetInFolder(folderPathname);
}

void SceneSaver::SetOutFolder(const FilePath &folderPathname)
{
    sceneUtils.SetOutFolder(folderPathname);
}

void SceneSaver::EnableCopyConverted(bool enabled)
{
    copyConverted = enabled;
}

void SceneSaver::SaveFile(const String &fileName, Set<String> &errorLog)
{
    Logger::FrameworkDebug("[SceneSaver::SaveFile] %s", fileName.c_str());
    
    FilePath filePath = sceneUtils.dataSourceFolder + fileName;

    //Load scene with *.sc2
    Scene *scene = new Scene();
    Entity *rootNode = scene->GetRootNode(filePath);
    if(rootNode)
    {
        int32 count = rootNode->GetChildrenCount();
		Vector<Entity*> tempV;
		tempV.reserve((count));
        for(int32 i = 0; i < count; ++i)
        {
			tempV.push_back(rootNode->GetChild(i));
        }
		for(int32 i = 0; i < count; ++i)
		{
			scene->AddNode(tempV[i]);
		}
		
		SaveScene(scene, filePath, errorLog);
    }
	else
	{
		errorLog.insert(Format("[SceneSaver::SaveFile] Can't open file %s", fileName.c_str()));
	}

    SafeRelease(scene);
}

void SceneSaver::ResaveFile(const String &fileName, Set<String> &errorLog)
{
	Logger::FrameworkDebug("[SceneSaver::ResaveFile] %s", fileName.c_str());

	FilePath sc2Filename = sceneUtils.dataSourceFolder + fileName;

	//Load scene with *.sc2
	Scene *scene = new Scene();
	Entity *rootNode = scene->GetRootNode(sc2Filename);
	if(rootNode)
	{
		int32 count = rootNode->GetChildrenCount();

		Vector<Entity*> tempV;
		tempV.reserve((count));
		for(int32 i = 0; i < count; ++i)
		{
			tempV.push_back(rootNode->GetChild(i));
		}
		for(int32 i = 0; i < count; ++i)
		{
			scene->AddNode(tempV[i]);
		}

		//scene->Update(0.f);
        scene->Save(sc2Filename);
	}
	else
	{
		errorLog.insert(Format("[SceneSaver::ResaveFile] Can't open file %s", fileName.c_str()));
	}

	SafeRelease(scene);
}

void SceneSaver::SaveScene(Scene *scene, const FilePath &fileName, Set<String> &errorLog)
{
    uint64 startTime = SystemTimer::Instance()->AbsoluteMS();
    
    DVASSERT(0 == texturesForSave.size())
    
    String relativeFilename = fileName.GetRelativePathname(sceneUtils.dataSourceFolder);
    sceneUtils.workingFolder = fileName.GetDirectory().GetRelativePathname(sceneUtils.dataSourceFolder);
    
    FileSystem::Instance()->CreateDirectory(sceneUtils.dataFolder + sceneUtils.workingFolder, true);

    //scene->Update(0.1f);

    FilePath oldPath = SceneValidator::Instance()->SetPathForChecking(sceneUtils.dataSourceFolder);
    SceneValidator::Instance()->ValidateScene(scene, fileName, errorLog);

    texturesForSave.clear();
    SceneHelper::EnumerateSceneTextures(scene, texturesForSave, SceneHelper::INCLUDE_NULL);

    CopyTextures(scene);
	ReleaseTextures();

	Landscape *landscape = FindLandscape(scene);
    if (landscape)
    {
        sceneUtils.AddFile(landscape->GetHeightmapPathname());
    }
    
    VegetationRenderObject* vegetation = FindVegetation(scene);
    if(vegetation)
    {
        const FilePath& textureSheetPath = vegetation->GetTextureSheetPath();
        if(!textureSheetPath.IsEmpty())
        {
            sceneUtils.AddFile(vegetation->GetTextureSheetPath());
        }
        
        const FilePath vegetationCustomGeometry = vegetation->GetCustomGeometryPath();
        if(!vegetationCustomGeometry.IsEmpty())
        {
            sceneUtils.AddFile(vegetationCustomGeometry);
        }
    }

	CopyReferencedObject(scene);
	CopyEffects(scene);
	CopyCustomColorTexture(scene, fileName.GetDirectory(), errorLog);

    //save scene to new place
    FilePath tempSceneName = sceneUtils.dataSourceFolder + relativeFilename;
    tempSceneName.ReplaceExtension(".saved.sc2");
    
    sceneUtils.CopyFiles(errorLog);
    scene->Save(tempSceneName, true);

    bool moved = FileSystem::Instance()->MoveFile(tempSceneName, sceneUtils.dataFolder + relativeFilename, true);
	if(!moved)
	{
		errorLog.insert(Format("Can't move file %s", fileName.GetAbsolutePathname().c_str()));
	}
    
    SceneValidator::Instance()->SetPathForChecking(oldPath);
    
    uint64 saveTime = SystemTimer::Instance()->AbsoluteMS() - startTime;
    Logger::Info("Save of %s to folder was done for %ldms", fileName.GetStringValue().c_str(), saveTime);
}


void SceneSaver::CopyTextures(DAVA::Scene *scene)
{
    TexturesMap::const_iterator endIt = texturesForSave.end();
    TexturesMap::iterator it = texturesForSave.begin();
    for( ; it != endIt; ++it)
    {
        CopyTexture(it->first);
    }
}

void SceneSaver::ReleaseTextures()
{
    texturesForSave.clear();
}

void SceneSaver::CopyTexture(const FilePath &texturePathname)
{
    FilePath descriptorPathname = TextureDescriptor::GetDescriptorPathname(texturePathname);
	
	TextureDescriptor* desc = TextureDescriptor::CreateFromFile(descriptorPathname);
	if(!desc)
	{
		//errorLog.insert(Format("Can't open file %s", descriptorPathname.GetAbsolutePathname().c_str()));
        Logger::Error("Can't open file %s", descriptorPathname.GetAbsolutePathname().c_str());
		return;
	}

    //copy descriptor
    sceneUtils.AddFile(descriptorPathname);

    //copy source textures
	if(desc->IsCubeMap())
	{
		Vector<FilePath> faceNames;

		Texture::GenerateCubeFaceNames(descriptorPathname.GetAbsolutePathname().c_str(), faceNames);
		for(Vector<FilePath>::iterator it = faceNames.begin();
			it != faceNames.end();
			++it)
		{
			sceneUtils.AddFile(*it);
		}
	}
	else
	{
		FilePath pngPathname = GPUFamilyDescriptor::CreatePathnameForGPU(texturePathname, GPU_UNKNOWN, FORMAT_RGBA8888);
		sceneUtils.AddFile(pngPathname);
	}
	

	//copy converted textures (*.pvr and *.dds)
    if(copyConverted)
    {
        for(int32 i = 0; i < GPU_FAMILY_COUNT; ++i)
        {
            eGPUFamily gpu = (eGPUFamily)i;
            
            PixelFormat format = desc->GetPixelFormatForCompression(gpu);
            if(format == FORMAT_INVALID)
            {
                continue;
            }
            
            FilePath imagePathname = GPUFamilyDescriptor::CreatePathnameForGPU(desc, gpu);
            sceneUtils.AddFile(imagePathname);
        }
    }
    
	delete desc;
}

void SceneSaver::CopyReferencedObject( Entity *node)
{
	KeyedArchive *customProperties = GetCustomPropertiesArchieve(node);
	if(customProperties && customProperties->IsKeyExists(ResourceEditor::EDITOR_REFERENCE_TO_OWNER))
	{
		String path = customProperties->GetString(ResourceEditor::EDITOR_REFERENCE_TO_OWNER);
		sceneUtils.AddFile(path);
	}
	for (int i = 0; i < node->GetChildrenCount(); i++)
	{
		CopyReferencedObject(node->GetChild(i));
	}
}

void SceneSaver::CopyEffects(Entity *node)
{
	ParticleEffectComponent *effect = GetEffectComponent(node);
	if(effect)
	{
		for (int32 i=0, sz=effect->GetEmittersCount(); i<sz; ++i)
			CopyEmitter(effect->GetEmitter(i));
	}

	for (int i = 0; i < node->GetChildrenCount(); ++i)
	{
		CopyEffects(node->GetChild(i));
	}
}

void SceneSaver::CopyEmitter( ParticleEmitter *emitter)
{
    if(emitter->configPath.IsEmpty() == false)
    {
        sceneUtils.AddFile(emitter->configPath);
    }
    else
    {
        Logger::Warning("[SceneSaver::CopyEmitter] empty config path for emitter %s", emitter->name.c_str());
    }

	const Vector<ParticleLayer*> &layers = emitter->layers;

	uint32 count = (uint32)layers.size();
	for(uint32 i = 0; i < count; ++i)
	{
		if(layers[i]->type == ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
		{
			CopyEmitter(layers[i]->innerEmitter);
		}
		else
		{
			Sprite *sprite = layers[i]->sprite;
			if(!sprite) continue;

			FilePath psdPath = ReplaceInString(sprite->GetRelativePathname().GetAbsolutePathname(), "/Data/", "/DataSource/");
			psdPath.ReplaceExtension(".psd");
			sceneUtils.AddFile(psdPath);
		}
	}
}

void SceneSaver::CopyCustomColorTexture(Scene *scene, const FilePath & sceneFolder, Set<String> &errorLog)
{
	Entity *land = FindLandscapeEntity(scene);
	if(!land) return;

	KeyedArchive* customProps = GetCustomPropertiesArchieve(land);
	if(!customProps) return;

	String pathname = customProps->GetString(ResourceEditor::CUSTOM_COLOR_TEXTURE_PROP);
	if(pathname.empty()) return;

    FilePath projectPath = CreateProjectPathFromPath(sceneFolder);
    if(projectPath.IsEmpty())
    {
        errorLog.insert(Format("Can't copy custom colors texture (%s)", pathname.c_str()));
        return;
    }

    FilePath texPathname = projectPath + pathname;
    sceneUtils.AddFile(texPathname);
    
    FilePath newTexPathname = sceneUtils.GetNewFilePath(texPathname);
    FilePath newProjectPathname = CreateProjectPathFromPath(sceneUtils.dataFolder);
    if(newProjectPathname.IsEmpty())
    {
        errorLog.insert(Format("Can't save custom colors texture (%s)", pathname.c_str()));
        return;
    }
    
    //save new path to custom colors texture
    customProps->SetString(ResourceEditor::CUSTOM_COLOR_TEXTURE_PROP, newTexPathname.GetRelativePathname(newProjectPathname));
}

FilePath SceneSaver::CreateProjectPathFromPath(const FilePath & pathname)
{
	String fullPath = pathname.GetAbsolutePathname();
	String::size_type pos = fullPath.find("/Data");
	if(pos != String::npos)
	{
        return fullPath.substr(0, pos+1);
	}
    
    return FilePath();
}
