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



#include "SceneExporter.h"
#include "Deprecated/SceneValidator.h"

#include "TextureCompression/TextureConverter.h"

#include "Render/TextureDescriptor.h"
#include "Qt/Scene/SceneHelper.h"
#include "Render/GPUFamilyDescriptor.h"

#include "../StringConstants.h"

#include "../Qt/Main/QtUtils.h"

using namespace DAVA;

SceneExporter::SceneExporter()
{
    exportForGPU = GPU_UNKNOWN;
	quality = TextureConverter::ECQ_DEFAULT;
	optimizeOnExport = true;
}

SceneExporter::~SceneExporter()
{
}


void SceneExporter::SetInFolder(const FilePath &folderPathname)
{
    sceneUtils.SetInFolder(folderPathname);
}

void SceneExporter::SetOutFolder(const FilePath &folderPathname)
{
    sceneUtils.SetOutFolder(folderPathname);
}

void SceneExporter::SetGPUForExporting(const String &newGPU)
{
    SetGPUForExporting(GPUFamilyDescriptor::GetGPUByName(newGPU));
}

void SceneExporter::SetGPUForExporting(const eGPUFamily newGPU)
{
    exportForGPU = newGPU;
}


void SceneExporter::ExportFile(const String &fileName, Set<String> &errorLog)
{
    Logger::FrameworkDebug("[SceneExporter::ExportFile] %s", fileName.c_str());
    
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
		
		ExportScene(scene, filePath, errorLog);
    }
	else
	{
		errorLog.insert(Format("[SceneExporter::ExportFile] Can't open file %s", filePath.GetAbsolutePathname().c_str()));
	}

    SafeRelease(scene);
}

void SceneExporter::ExportScene(Scene *scene, const FilePath &fileName, Set<String> &errorLog)
{
    uint64 startTime = SystemTimer::Instance()->AbsoluteMS();
    
    //Create destination folder
    String relativeFilename = fileName.GetRelativePathname(sceneUtils.dataSourceFolder);
    sceneUtils.workingFolder = fileName.GetDirectory().GetRelativePathname(sceneUtils.dataSourceFolder);
    
    FileSystem::Instance()->CreateDirectory(sceneUtils.dataFolder + sceneUtils.workingFolder, true); 
    
    uint64 removeEditorNodesStart = SystemTimer::Instance()->AbsoluteMS();
    //Export scene data
    RemoveEditorNodes(scene);
    uint64 removeEditorNodesTime = SystemTimer::Instance()->AbsoluteMS() - removeEditorNodesStart;
    
    uint64 removeEditorCPStart = SystemTimer::Instance()->AbsoluteMS();
    if(optimizeOnExport)
    {
        RemoveEditorCustomProperties(scene);
    }
    uint64 removeEditorCPTime = SystemTimer::Instance()->AbsoluteMS() - removeEditorCPStart;

    uint64 exportDescriptorsStart = SystemTimer::Instance()->AbsoluteMS();
    bool sceneWasExportedCorrectly = ExportDescriptors(scene, errorLog);
    uint64 exportDescriptorsTime = SystemTimer::Instance()->AbsoluteMS() - exportDescriptorsStart;

    
    uint64 validationStart = SystemTimer::Instance()->AbsoluteMS();
    FilePath oldPath = SceneValidator::Instance()->SetPathForChecking(sceneUtils.dataSourceFolder);
    SceneValidator::Instance()->ValidateScene(scene, fileName, errorLog);
	//SceneValidator::Instance()->ValidateScales(scene, errorLog);
    uint64 validationTime = SystemTimer::Instance()->AbsoluteMS() - validationStart;

    
    uint64 landscapeStart = SystemTimer::Instance()->AbsoluteMS();
    sceneWasExportedCorrectly &= ExportLandscape(scene, errorLog);
    uint64 landscapeTime = SystemTimer::Instance()->AbsoluteMS() - landscapeStart;

    uint64 vegetationStart = SystemTimer::Instance()->AbsoluteMS();
    sceneWasExportedCorrectly &= ExportVegetation(scene, errorLog);
    uint64 vegetationTime = SystemTimer::Instance()->AbsoluteMS() - vegetationStart;

    //save scene to new place
    uint64 saveStart = SystemTimer::Instance()->AbsoluteMS();
    FilePath tempSceneName = FilePath::CreateWithNewExtension(sceneUtils.dataSourceFolder + relativeFilename, ".exported.sc2");
    scene->Save(tempSceneName, optimizeOnExport);
    uint64 saveTime = SystemTimer::Instance()->AbsoluteMS() - saveStart;

    
    uint64 moveStart = SystemTimer::Instance()->AbsoluteMS();
    bool moved = FileSystem::Instance()->MoveFile(tempSceneName, sceneUtils.dataFolder + relativeFilename, true);
	if(!moved)
	{
		errorLog.insert(Format("Can't move file %s", fileName.GetAbsolutePathname().c_str()));
        sceneWasExportedCorrectly = false;
	}
    uint64 moveTime = SystemTimer::Instance()->AbsoluteMS() - moveStart;

    SceneValidator::Instance()->SetPathForChecking(oldPath);
    
    if(!sceneWasExportedCorrectly)
    {   // *** to highlight this message from other error messages
        Logger::Error("***  Scene %s was exported with errors!", fileName.GetAbsolutePathname().c_str());
    }
    
    
    uint64 exportTime = SystemTimer::Instance()->AbsoluteMS() - startTime;
    Logger::Info("Export Status\n\tScene: %s\n\tExport time: %ldms\n\tRemove editor nodes time: %ldms\n\tRemove custom properties: %ldms\n\tExport descriptors: %ldms\n\tValidation time: %ldms\n\tLandscape time: %ldms\n\tVegetation time: %ldms\n\tSave time: %ldms\n\tMove time: %ldms\n\tErrors occured: %d",
                 fileName.GetStringValue().c_str(), exportTime, removeEditorNodesTime, removeEditorCPTime, exportDescriptorsTime, validationTime, landscapeTime, vegetationTime, saveTime, moveTime, !sceneWasExportedCorrectly
                 );
    
    return;
}

void SceneExporter::RemoveEditorNodes(DAVA::Entity *rootNode)
{
    //Remove scene nodes
    Vector<Entity *> scenenodes;
    rootNode->GetChildNodes(scenenodes);
        
    //remove nodes from hierarhy
    Vector<Entity *>::reverse_iterator endItDeletion = scenenodes.rend();
    for (Vector<Entity *>::reverse_iterator it = scenenodes.rbegin(); it != endItDeletion; ++it)
    {
        Entity * node = *it;
		String::size_type pos = node->GetName().find(ResourceEditor::EDITOR_BASE);
        if(String::npos != pos)
        {
            node->GetParent()->RemoveNode(node);
        }
    }
}

void SceneExporter::RemoveEditorCustomProperties(Entity *rootNode)
{
    Vector<Entity *> scenenodes;
    rootNode->GetChildNodes(scenenodes);
    
//    "editor.dynamiclight.enable";
//    "editor.donotremove";
//    
//    "editor.referenceToOwner";
//    "editor.isSolid";
//    "editor.isLocked";
//    "editor.designerName"
//    "editor.modificationData"
//    "editor.staticlight.enable";
//    "editor.staticlight.used"
//    "editor.staticlight.castshadows";
//    "editor.staticlight.receiveshadows";
//    "editor.staticlight.falloffcutoff"
//    "editor.staticlight.falloffexponent"
//    "editor.staticlight.shadowangle"
//    "editor.staticlight.shadowsamples"
//    "editor.staticlight.shadowradius"
//    "editor.intensity"
    
    Vector<Entity *>::const_iterator endIt = scenenodes.end();
    for (Vector<Entity *>::iterator it = scenenodes.begin(); it != endIt; ++it)
    {
        Entity * node = *it;

        KeyedArchive *props = GetCustomPropertiesArchieve(node);
        if(props)
        {
            const Map<String, VariantType*> propsMap = props->GetArchieveData();
            
            auto endIt = propsMap.end();
            for(auto it = propsMap.begin(); it != endIt; ++it)
            {
                String key = it->first;
                
                if(key.find(ResourceEditor::EDITOR_BASE) == 0)
                {
                    if((key != ResourceEditor::EDITOR_DO_NOT_REMOVE) && (key != ResourceEditor::EDITOR_DYNAMIC_LIGHT_ENABLE))
                    {
                        props->DeleteKey(key);
                    }
                }
            }
            
			if(props->Count() == 0)
            {
                node->RemoveComponent(DAVA::Component::CUSTOM_PROPERTIES_COMPONENT);
            }
        }
    }
}


bool SceneExporter::ExportDescriptors(DAVA::Scene *scene, Set<String> &errorLog)
{
    bool allDescriptorsWereExported = true;
    
    DAVA::TexturesMap textures;
    SceneHelper::EnumerateSceneTextures(scene, textures, SceneHelper::INCLUDE_NULL);

    auto endIt = textures.end();
    for(auto it = textures.begin(); it != endIt; ++it)
    {
        DAVA::FilePath pathname = it->first;
        if((pathname.GetType() == DAVA::FilePath::PATH_IN_MEMORY) || pathname.IsEmpty())
        {
            continue;
        }

        allDescriptorsWereExported &= ExportTextureDescriptor(it->first, errorLog);
    }

    textures.clear();
    
    return allDescriptorsWereExported;
}

bool SceneExporter::ExportTextureDescriptor(const FilePath &pathname, Set<String> &errorLog)
{
    TextureDescriptor *descriptor = TextureDescriptor::CreateFromFile(pathname);
    if(!descriptor)
    {
        errorLog.insert(Format("Can't create descriptor for pathname %s", pathname.GetAbsolutePathname().c_str()));
        return false;
    }
    
    descriptor->exportedAsGpuFamily = exportForGPU;
    descriptor->format = descriptor->GetPixelFormatForCompression(exportForGPU);

    if((descriptor->exportedAsGpuFamily != GPU_UNKNOWN) && (descriptor->format == FORMAT_INVALID))
    {
        errorLog.insert(Format("Not selected export format for pathname %s", pathname.GetAbsolutePathname().c_str()));
        
		delete descriptor;
        return false;
    }
    
    
    String workingPathname = descriptor->pathname.GetRelativePathname(sceneUtils.dataSourceFolder);
    sceneUtils.PrepareFolderForCopyFile(workingPathname, errorLog);

    bool isExported = ExportTexture(descriptor, errorLog);
    if(isExported)
    {
        descriptor->Export(sceneUtils.dataFolder + workingPathname);
    }
    
	delete descriptor;
    return isExported;
}

bool SceneExporter::ExportTexture(const TextureDescriptor * descriptor, Set<String> &errorLog)
{
    CompressTextureIfNeed(descriptor, errorLog);
    
    if(descriptor->exportedAsGpuFamily == GPU_UNKNOWN)
    {
		bool copyResult = true;
		
		if(descriptor->IsCubeMap())
		{
			Vector<FilePath> faceNames;
			Texture::GenerateCubeFaceNames(descriptor->pathname.GetAbsolutePathname().c_str(), faceNames);
			for(Vector<FilePath>::iterator it = faceNames.begin();
				it != faceNames.end();
				++it)
			{
				bool result = sceneUtils.CopyFile(*it, errorLog);
				copyResult = copyResult && result;
			}
		}
		else
		{
			FilePath sourceTexturePathname =  FilePath::CreateWithNewExtension(descriptor->pathname, ".png");
			copyResult = sceneUtils.CopyFile(sourceTexturePathname, errorLog);
		}
		
		return copyResult;
    }

    FilePath compressedTexureName = GPUFamilyDescriptor::CreatePathnameForGPU(descriptor, (eGPUFamily)descriptor->exportedAsGpuFamily);
    return sceneUtils.CopyFile(compressedTexureName, errorLog);
}



void SceneExporter::ExportFolder(const String &folderName, Set<String> &errorLog)
{
    FilePath folderPathname = sceneUtils.dataSourceFolder + folderName;
    folderPathname.MakeDirectoryPathname();
    
	FileList * fileList = new FileList(folderPathname);
    for (int32 i = 0; i < fileList->GetCount(); ++i)
	{
        FilePath pathname = fileList->GetPathname(i);
		if(fileList->IsDirectory(i))
		{
            if(!fileList->IsNavigationDirectory(i))
            {
                String workingPathname = pathname.GetRelativePathname(sceneUtils.dataSourceFolder);
                ExportFolder(workingPathname, errorLog);
            }
        }
        else 
        {
            if(pathname.IsEqualToExtension(".sc2"))
            {
                String::size_type exportedPos = pathname.GetAbsolutePathname().find(".exported.sc2");
                if(exportedPos != String::npos)
                {
                    //Skip temporary files, created during export
                    continue;
                }
                
                String workingPathname = pathname.GetRelativePathname(sceneUtils.dataSourceFolder);
                ExportFile(workingPathname, errorLog);
            }
        }
    }
    
    SafeRelease(fileList);
}

bool SceneExporter::ExportLandscape(Scene *scene, Set<String> &errorLog)
{
    DVASSERT(scene);

    Landscape *landscape = FindLandscape(scene);
    if (landscape)
    {
        return sceneUtils.CopyFile(landscape->GetHeightmapPathname(), errorLog);
    }
    
    return true;
}

bool SceneExporter::ExportVegetation(Scene *scene, Set<String> &errorLog)
{
    DVASSERT(scene);
    
    bool wasExported = true;
    
    VegetationRenderObject *vegetation = FindVegetation(scene);
    if (vegetation)
    {
        const FilePath& textureSheetPath = vegetation->GetTextureSheetPath();
        if(textureSheetPath.Exists())
        {
            wasExported |= sceneUtils.CopyFile(vegetation->GetTextureSheetPath(), errorLog);
        }
    }
    
    return wasExported;
}

void SceneExporter::CompressTextureIfNeed(const TextureDescriptor * descriptor, Set<String> &errorLog)
{
    if(descriptor->exportedAsGpuFamily == GPU_UNKNOWN)
        return;
    
    
    FilePath compressedTexureName = GPUFamilyDescriptor::CreatePathnameForGPU(descriptor, (eGPUFamily)descriptor->exportedAsGpuFamily);
    FilePath sourceTexturePathname =  FilePath::CreateWithNewExtension(descriptor->pathname, ".png");

    bool fileExcists = FileSystem::Instance()->IsFile(compressedTexureName);
    bool needToConvert = SceneValidator::IsTextureChanged(descriptor, (eGPUFamily)descriptor->exportedAsGpuFamily);
    
    if(needToConvert || !fileExcists)
    {
        //TODO: convert to pvr/dxt
        //TODO: do we need to convert to pvr if needToConvert is false, but *.pvr file isn't at filesystem
        
		eGPUFamily gpuFamily = (eGPUFamily)descriptor->exportedAsGpuFamily;

		TextureConverter::CleanupOldTextures(descriptor, gpuFamily, descriptor->format);
		TextureConverter::ConvertTexture(*descriptor, gpuFamily, true, quality);
        
        DAVA::TexturesMap texturesMap = Texture::GetTextureMap();
        
        DAVA::TexturesMap::iterator found = texturesMap.find(FILEPATH_MAP_KEY(descriptor->pathname));
        if(found != texturesMap.end())
        {
            DAVA::Texture *tex = found->second;
            tex->Reload();
        }
    }
}

void SceneExporter::EnableOptimizations( bool enable )
{
	optimizeOnExport = enable;
}

void SceneExporter::SetCompressionQuality( TextureConverter::eConvertQuality _quality )
{
	quality = _quality;
}

