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


#include "TexturePacker/ResourcePacker2D.h"
#include "TexturePacker/DefinitionFile.h"
#include "TexturePacker/TexturePacker.h"
#include "TexturePacker/CommandLineParser.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FileList.h"
#include "Core/Core.h"
#include "Platform/SystemTimer.h"
#include "Utils/MD5.h"
#include "Utils/StringFormat.h"

#include <Magick++.h>
#include <magick/MagickCore.h>
#include <magick/property.h>

#include "Render/GPUFamilyDescriptor.h"
#include "FramePathHelper.h"

namespace DAVA
{

static const String FLAG_RECURSIVE = "--recursive";

ResourcePacker2D::ResourcePacker2D()
{
	isLightmapsPacking = false;
	clearProcessDirectory = false;
}

String ResourcePacker2D::GetProcessFolderName()
{
	return "$process/";
}

void ResourcePacker2D::InitFolders(const FilePath & inputPath,const FilePath & outputPath)
{
    DVASSERT(inputPath.IsDirectoryPathname() && outputPath.IsDirectoryPathname());
    
	inputGfxDirectory = inputPath;
	outputGfxDirectory = outputPath;
	excludeDirectory = inputPath + "../";
}
    
void ResourcePacker2D::PackResources(eGPUFamily forGPU)
{
	Logger::FrameworkDebug("\nInput: %s \nOutput: %s \nExclude: %s",
                  inputGfxDirectory.GetAbsolutePathname().c_str(),
                  outputGfxDirectory.GetAbsolutePathname().c_str(),
                  excludeDirectory.GetAbsolutePathname().c_str());

    Logger::FrameworkDebug("For GPU: %s", (GPU_UNKNOWN != forGPU) ? GPUFamilyDescriptor::GetGPUName(forGPU).c_str() : "Unknown");

    
	requestedGPUFamily = forGPU;
    
	isGfxModified = false;

    gfxDirName = inputGfxDirectory.GetLastDirectoryName();
	std::transform(gfxDirName.begin(), gfxDirName.end(), gfxDirName.begin(), ::tolower);


	FilePath processDirectoryPath = excludeDirectory + GetProcessFolderName();
	if (FileSystem::Instance()->CreateDirectory(processDirectoryPath, true) == FileSystem::DIRECTORY_CANT_CREATE)
	{
		//Logger::Error("Can't create directory: %s", processDirectoryPath.c_str());
	}


	if (IsMD5ChangedDir(processDirectoryPath, outputGfxDirectory, gfxDirName + ".md5", true))
	{
		if (Core::Instance()->IsConsoleMode())
			Logger::FrameworkDebug("[Gfx not available or changed - performing full repack]");
		isGfxModified = true;
	
		// Remove whole output directory
		bool result = FileSystem::Instance()->DeleteDirectory(outputGfxDirectory);
		if (result)
		{
			Logger::FrameworkDebug("Removed output directory: %s", outputGfxDirectory.GetAbsolutePathname().c_str());
		}
		if (!result && Core::Instance()->IsConsoleMode() && CommandLineParser::Instance()->GetVerbose())
		{
			AddError(Format("[ERROR: Can't delete directory %s]",
									outputGfxDirectory.GetAbsolutePathname().c_str()));
		}
	}

	RecursiveTreeWalk(inputGfxDirectory, outputGfxDirectory);

	// Put latest md5 after convertation
	IsMD5ChangedDir(processDirectoryPath, outputGfxDirectory, gfxDirName + ".md5", true);
}


bool ResourcePacker2D::IsMD5ChangedDir(const FilePath & processDirectoryPath, const FilePath & pathname, const String & name, bool isRecursive)
{
    DVASSERT(processDirectoryPath.IsDirectoryPathname());

	FilePath md5FileName = FilePath::CreateWithNewExtension(processDirectoryPath + name, ".md5");

    
	uint8 oldMD5Digest[16];
	uint8 newMD5Digest[16];
	bool isChanged = false;
	File * file = File::Create(md5FileName, File::OPEN | File::READ);
	if (!file)
	{
		isChanged = true;		
	}else
	{
		int32 bytes = file->Read(oldMD5Digest, 16);
		DVASSERT(bytes == 16 && "We should always read 16 bytes from md5 file");
	}
	SafeRelease(file);


	MD5::ForDirectory(pathname, newMD5Digest, isRecursive);

	file = File::Create(md5FileName, File::CREATE | File::WRITE);
    
	int32 bytes = file->Write(newMD5Digest, 16);
	DVASSERT(bytes == 16 && "16 bytes should be always written for md5 file");
	SafeRelease(file);

	// if already changed return without compare
	if (isChanged)
		return true;

	for (int32 k = 0; k < 16; ++k)
		if (oldMD5Digest[k] != newMD5Digest[k])
			isChanged = true;

	return isChanged;
}


bool ResourcePacker2D::IsMD5ChangedFile(const FilePath & processDirectoryPath, const FilePath & pathname, const String & psdName)
{
    DVASSERT(processDirectoryPath.IsDirectoryPathname());

	FilePath md5FileName = FilePath::CreateWithNewExtension(processDirectoryPath + psdName, ".md5");

	uint8 oldMD5Digest[16];
	uint8 newMD5Digest[16];
	bool isChanged = false;
	File * file = File::Create(md5FileName, File::OPEN | File::READ);
	if (!file)
	{
		isChanged = true;		
	}else
	{
		int32 bytes = file->Read(oldMD5Digest, 16);
		DVASSERT(bytes == 16 && "We should always read 16 bytes from md5 file");
	}
	SafeRelease(file);

		
	MD5::ForFile(pathname, newMD5Digest);
	
	file = File::Create(md5FileName, File::CREATE | File::WRITE);
	int32 bytes = file->Write(newMD5Digest, 16);
	DVASSERT(bytes == 16 && "16 bytes should be always written for md5 file");
	SafeRelease(file);

	// file->Write()

	for (int32 k = 0; k < 16; ++k)
		if (oldMD5Digest[k] != newMD5Digest[k])
			isChanged = true;
	
	return isChanged;
}

DefinitionFile * ResourcePacker2D::ProcessPSD(const FilePath & processDirectoryPath, const FilePath & psdPathname, const String & psdName)
{
    DVASSERT(processDirectoryPath.IsDirectoryPathname());
    
	uint32 maxTextureSize = (CommandLineParser::Instance()->IsFlagSet("--tsize4096")) ? TexturePacker::TSIZE_4096 : TexturePacker::DEFAULT_TEXTURE_SIZE;
	
	// TODO: Check CRC32
	Vector<Magick::Image> layers;
	
    FilePath psdNameWithoutExtension(processDirectoryPath + psdName);
    psdNameWithoutExtension.TruncateExtension();
	
	try 
	{
		Magick::readImages(&layers, psdPathname.GetAbsolutePathname());
		
		if (layers.size() == 0)
		{
			AddError(Format("Number of layers is too low: %s", psdPathname.GetAbsolutePathname().c_str()));
			
			return 0;
		}
		
		if (layers.size() == 1)
		{
			layers.push_back(layers[0]);
		}
		
		//Logger::FrameworkDebug("psd file: %s wext: %s", psdPathname.c_str(), psdNameWithoutExtension.c_str());
		
		int width = (int)layers[0].columns();
		int height = (int)layers[0].rows();
		
		for(int k = 1; k < (int)layers.size(); ++k)
		{
			FilePath outputFile = FramePathHelper::GetFramePathRelative(psdNameWithoutExtension, k - 1);

			Magick::Image & currentLayer = layers[k];
			currentLayer.crop(Magick::Geometry(width,height, 0, 0));
			currentLayer.magick("PNG");
			currentLayer.write(outputFile.GetAbsolutePathname());
		}

		DefinitionFile * defFile = new DefinitionFile;
		defFile->filename = FilePath::CreateWithNewExtension(psdNameWithoutExtension, ".txt");
		defFile->spriteWidth = width;
		defFile->spriteHeight = height;
		defFile->frameCount = (int)layers.size() -1;
		defFile->frameRects = new Rect2i[defFile->frameCount];
		
		for(int k = 1; k < (int)layers.size(); ++k)
		{
			Magick::Image & currentLayer = layers[k];
			Magick::Geometry bbox = currentLayer.page();
			int xOff = (int)bbox.xOff();
			if (bbox.xNegative())
				xOff = -xOff;
			int yOff = (int)bbox.yOff();
			if (bbox.yNegative())
				yOff = -yOff;
            
			// Get and save layer name
            defFile->frameNames.push_back(currentLayer.label());
            
			defFile->frameRects[k - 1] = Rect2i(xOff, yOff, (int32)bbox.width(), (int32)bbox.height());
			
			//printf("Percent: %d Aspect: %d Greater: %d Less: %d\n", (int)bbox.percent(), (int)bbox.aspect(), (int)bbox.greater(), (int)bbox.less());
			
			if ((defFile->frameRects[k - 1].dx > (int32)maxTextureSize) || (defFile->frameRects[k - 1].dy > (int32)maxTextureSize))
			{
				Logger::Warning("* WARNING * - frame of %s layer %d is bigger than maxTextureSize(%d) layer exportSize (%d x %d) FORCE REDUCE TO (%d x %d). Bewarned!!! Results not guaranteed!!!", psdName.c_str(), k - 1, maxTextureSize
					   , defFile->frameRects[k - 1].dx, defFile->frameRects[k - 1].dy, width, height);
                
				defFile->frameRects[k - 1].dx = width;
				defFile->frameRects[k - 1].dy = height;
			}
				
			if (CommandLineParser::Instance()->IsFlagSet("--add0pixel"))
			{
				
			}else if (CommandLineParser::Instance()->IsFlagSet("--add1pixel"))
			{
				defFile->frameRects[k - 1].dx++;
				defFile->frameRects[k - 1].dy++;
			}
			else if (CommandLineParser::Instance()->IsFlagSet("--add2pixel"))
			{
				defFile->frameRects[k - 1].dx+=2;
				defFile->frameRects[k - 1].dy+=2;
			}
			else if (CommandLineParser::Instance()->IsFlagSet("--add4pixel"))
			{
				defFile->frameRects[k - 1].dx+=4;
				defFile->frameRects[k - 1].dy+=4;
			}
			else if(CommandLineParser::Instance()->IsFlagSet("--add2sidepixel"))
			{
				defFile->frameRects[k - 1].dx+=2;
				defFile->frameRects[k - 1].dy+=2;
			}
			else
			{
				defFile->frameRects[k - 1].dx++;
				defFile->frameRects[k - 1].dy++;	
			}
		}
		
		return defFile;
	}
	catch( Magick::Exception &error_ )
    {
		AddError(Format("Caught exception: %s file: %s", error_.what(), psdPathname.GetAbsolutePathname().c_str()));

		return 0;
    }
	return 0;
}

Vector<String> ResourcePacker2D::ProcessFlags(const FilePath & flagsPathname)
{
	File * file = File::Create(flagsPathname, File::READ | File::OPEN);
	if (!file)
	{
		AddError(Format("Failed to open file: %s", flagsPathname.GetAbsolutePathname().c_str()));
		
        return Vector<String>();
	}

	Vector<char> flagsTmpVector;
	flagsTmpVector.reserve(file->GetSize() + 1);
	while(!file->IsEof())
	{
		char c = 0x00;
		int32 readSize = file->Read(&c, 1);
		if (readSize == 1)
		{
			// Terminate reading if end-of-line is detected.
			if (c == 0x0D || c == 0x0A)
			{
				break;
			}

			flagsTmpVector.push_back(c);
		}	
	}
	flagsTmpVector.push_back(0);

	currentFlags = flagsTmpVector.data();
	String flags = flagsTmpVector.data();
	
	const String & delims=" ";
	
	// Skip delims at beginning, find start of first token
	String::size_type lastPos = flags.find_first_not_of(delims, 0);
	// Find next delimiter @ end of token
	String::size_type pos     = flags.find_first_of(delims, lastPos);
	// output vector
	Vector<String> tokens;
	
	while (String::npos != pos || String::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(flags.substr(lastPos, pos - lastPos));
		// Skip delims.  Note the "not_of". this is beginning of token
		lastPos = flags.find_first_not_of(delims, pos);
		// Find next delimiter at end of token.
		pos     = flags.find_first_of(delims, lastPos);
	}
	
    for (int k = 0; k < (int) tokens.size(); ++k)
    {
        Logger::FrameworkDebug("Token: %s", tokens[k].c_str());
    }

	CommandLineParser::Instance()->SetArguments(tokens);
	
	SafeRelease(file);
	
	return tokens;
}


bool ResourcePacker2D::isRecursiveFlagSet(const Vector<String> & flags)
{
	for (uint32 k = 0; k < flags.size(); ++k)
	{
		if (flags[k] == FLAG_RECURSIVE)
		{
			return true;
		}
	}
	
	return false;
}

void ResourcePacker2D::RecursiveTreeWalk(const FilePath & inputPath, const FilePath & outputPath, const Vector<String> & flags)
{
	// Local list for flags command arguments
	Vector<String> currentCommandFlags = flags;

    DVASSERT(inputPath.IsDirectoryPathname() && outputPath.IsDirectoryPathname());
    
	uint64 packTime = SystemTimer::Instance()->AbsoluteMS();

	/* New $process folder structure */
	
	String dataSourceRelativePath = inputPath.GetRelativePathname(excludeDirectory);
	FilePath processDirectoryPath = excludeDirectory  + GetProcessFolderName() + dataSourceRelativePath;
	if (FileSystem::Instance()->CreateDirectory(processDirectoryPath, true) == FileSystem::DIRECTORY_CANT_CREATE)
	{
		//Logger::Error("Can't create directory: %s", processDirectoryPath.c_str());
	}

	if(clearProcessDirectory)
	{
		FileSystem::Instance()->DeleteDirectoryFiles(processDirectoryPath, false);
	}

	//String outputPath = outputPath;
	if (FileSystem::Instance()->CreateDirectory(outputPath) == FileSystem::DIRECTORY_CANT_CREATE)
	{
		//Logger::Error("Can't create directory: %s", outputPath.c_str());
	}
	
	CommandLineParser::Instance()->Clear();
	List<DefinitionFile *> definitionFileList;

	// Reset processed flag
	bool flagsProcessed = false;
	// Find flags and setup them
	FileList * fileList = new FileList(inputPath);
	for (int fi = 0; fi < fileList->GetCount(); ++fi)
	{
		if (!fileList->IsDirectory(fi))
		{
			if (fileList->GetFilename(fi) == "flags.txt")
			{
				currentCommandFlags = ProcessFlags(fileList->GetPathname(fi));
				flagsProcessed = true;
				break;
			}
		}
	}
	
	// If "flags.txt" do not exist - try to use previous flags command line
	if (!flagsProcessed)
	{
		currentFlags = "";
		if (currentCommandFlags.size() > 0)
		{
			CommandLineParser::Instance()->SetArguments(currentCommandFlags);
			for (uint32 k = 0; k < currentCommandFlags.size(); ++k)
			{
				currentFlags += currentCommandFlags[k];
				if (k != (currentCommandFlags.size() - 1))
				{
					 currentFlags += " ";
				}
			}
		}
	}
	
	bool modified = isGfxModified;
	// Process all psd / png files

	if (IsMD5ChangedDir(processDirectoryPath, inputPath, "dir.md5", false))
	{
		modified = true;
		//if (Core::Instance()->IsConsoleMode())
		//	printf("[Directory changed - rebuild: %s]\n", inputGfxDirectory.c_str());
	}

	bool needPackResourcesInThisDir = true;
	if (modified)
	{
		FileSystem::Instance()->DeleteDirectoryFiles(outputPath, false);
		
		for (int fi = 0; fi < fileList->GetCount(); ++fi)
		{
			if (!fileList->IsDirectory(fi))
			{
				FilePath fullname = fileList->GetPathname(fi);
				if (fullname.IsEqualToExtension(".psd"))
				{
                    //TODO: check if we need filename or pathname
					DefinitionFile * defFile = ProcessPSD(processDirectoryPath, fullname, fullname.GetFilename());
					if (!defFile)
					{
						// An error occured while converting this PSD file - cancel converting in this directory.
						needPackResourcesInThisDir = false;
						break;
					}

					definitionFileList.push_back(defFile);
				}
				else if(isLightmapsPacking && fullname.IsEqualToExtension(".png"))
				{
					DefinitionFile * defFile = new DefinitionFile();
					defFile->LoadPNG(fullname, processDirectoryPath);
					definitionFileList.push_back(defFile);
				}
				else if (fullname.IsEqualToExtension(".pngdef"))
				{
					DefinitionFile * defFile = new DefinitionFile();
					if (defFile->LoadPNGDef(fullname, processDirectoryPath))
					{
						definitionFileList.push_back(defFile);
					}
					else 
					{
						SafeDelete(defFile);
					}
				}
			}
		}

		// 
		if (needPackResourcesInThisDir && definitionFileList.size() > 0 && modified)
		{
			TexturePacker packer;
			if(isLightmapsPacking)
			{
				packer.UseOnlySquareTextures();
				packer.SetMaxTextureSize(2048);
			}
            else if(CommandLineParser::Instance()->IsFlagSet("--tsize4096"))
            {
                packer.SetMaxTextureSize(TexturePacker::TSIZE_4096);
            }

            if(definitionFileList.size() == 1)
            {
                DefinitionFile * def = definitionFileList.front();
                if(def->frameCount == 1)
                {
                    if (CommandLineParser::Instance()->IsFlagSet("--add2sidepixel") == false)
                    {
                        def->frameRects[0] = packer.ReduceRectToOriginalSize(def->frameRects[0]);
                        CommandLineParser::Instance()->AddArgument("--add0pixel");
                    }
                }
            }

			if (CommandLineParser::Instance()->IsFlagSet("--split"))
			{
				packer.PackToTexturesSeparate(excludeDirectory, outputPath, definitionFileList, requestedGPUFamily);
			}
			else
			{
				packer.PackToTextures(excludeDirectory, outputPath, definitionFileList, requestedGPUFamily);
			}
			
			Set<String> currentErrors = packer.GetErrors();
			if (!currentErrors.empty())
			{
				errors.insert(currentErrors.begin(), currentErrors.end());
			}
		}
	}	

	packTime = SystemTimer::Instance()->AbsoluteMS() - packTime;

	if (Core::Instance()->IsConsoleMode())
	{
        Logger::Info("[%d files packed with flags: %s]", (int)definitionFileList.size(), currentFlags.c_str());
	
		String result = "[unchanged]";
		if (modified)
			result = "[REPACKED]";

		Logger::Info("[%s - %.2lf secs] - %s", inputPath.GetAbsolutePathname().c_str(), (float64)packTime / 1000.0f, result.c_str());
	}

	
	for (List<DefinitionFile*>::iterator it = definitionFileList.begin(); it != definitionFileList.end(); ++it)
	{
		DefinitionFile * file = *it;
		SafeDelete(file);
	}
	definitionFileList.clear();
	
	for (int fi = 0; fi < fileList->GetCount(); ++fi)
	{
		if (fileList->IsDirectory(fi))
		{
			String filename = fileList->GetFilename(fi);
			if (!fileList->IsNavigationDirectory(fi) && (filename != "$process") && (filename != ".svn"))
			{
				if ((filename.size() > 0) && (filename[0] != '.'))
                {
                    FilePath input = inputPath + filename;
                    input.MakeDirectoryPathname();
                    
                    FilePath output = outputPath + filename;
                    output.MakeDirectoryPathname();
					
					if (isRecursiveFlagSet(currentCommandFlags))
					{
						RecursiveTreeWalk(input, output, currentCommandFlags);
					}
					else
					{
						RecursiveTreeWalk(input, output);
					}
                }
			}
		}
	}
	
	SafeRelease(fileList);
}

const Set<String>& ResourcePacker2D::GetErrors() const
{
	return errors;
}

void ResourcePacker2D::AddError(const String& errorMsg)
{
	Logger::Error(errorMsg.c_str());
	errors.insert(errorMsg);
}

};
