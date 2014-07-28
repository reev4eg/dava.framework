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


#include "TexturePacker/TexturePacker.h"
#include "TexturePacker/CommandLineParser.h"
#include "TexturePacker/ImagePacker.h"
#include "TexturePacker/PngImage.h"
#include "TexturePacker/DefinitionFile.h"
#include "Render/TextureDescriptor.h"
#include "FileSystem/FileSystem.h"
#include "TextureCompression/TextureConverter.h"
#include "Render/GPUFamilyDescriptor.h"
#include "FramePathHelper.h"
#include "Utils/StringFormat.h"
#include "Render/PixelFormatDescriptor.h"


#ifdef WIN32
#define snprintf _snprintf
#endif

namespace DAVA
{

static Set<PixelFormat> InitPixelFormatsWithCompression()
{
    Set<PixelFormat> set;
    set.insert(FORMAT_PVR4);
    set.insert(FORMAT_PVR2);
    set.insert(FORMAT_DXT1);
    set.insert(FORMAT_DXT1A);
    set.insert(FORMAT_DXT3);
    set.insert(FORMAT_DXT5);
    set.insert(FORMAT_DXT5NM);
    set.insert(FORMAT_ETC1);
    set.insert(FORMAT_ATC_RGB);
    set.insert(FORMAT_ATC_RGBA_EXPLICIT_ALPHA);
    set.insert(FORMAT_ATC_RGBA_INTERPOLATED_ALPHA);
	set.insert(FORMAT_PVR2_2);
	set.insert(FORMAT_PVR4_2);
	set.insert(FORMAT_EAC_R11_UNSIGNED);
	set.insert(FORMAT_EAC_R11_SIGNED);
	set.insert(FORMAT_EAC_RG11_UNSIGNED);
	set.insert(FORMAT_EAC_RG11_SIGNED);
	set.insert(FORMAT_ETC2_RGB);
	set.insert(FORMAT_ETC2_RGBA);
	set.insert(FORMAT_ETC2_RGB_A1);

    return set;
}

const Set<PixelFormat> TexturePacker::PIXEL_FORMATS_WITH_COMPRESSION = InitPixelFormatsWithCompression();

TexturePacker::TexturePacker()
{
	quality = TextureConverter::ECQ_VERY_HIGH;
	if (CommandLineParser::Instance()->IsFlagSet("--quality"))
	{
		String qualityName = CommandLineParser::Instance()->GetParamForFlag("--quality");
		int32 q = atoi(qualityName.c_str());
		if((q >= TextureConverter::ECQ_FASTEST) && (q <= TextureConverter::ECQ_VERY_HIGH))
		{
			quality = (TextureConverter::eConvertQuality)q;
		}
	}
    
	maxTextureSize = DEFAULT_TEXTURE_SIZE;
	onlySquareTextures = false;
	errors.clear();
}

bool TexturePacker::TryToPack(const Rect2i & textureRect, List<DefinitionFile*> & /*defsList*/)
{
	ImagePacker * packer = new ImagePacker(textureRect);
	
	// Packing of sorted by size images
	for (int i = 0; i < (int)sortVector.size(); ++i)
	{
		DefinitionFile * defFile = sortVector[i].defFile;
		int frame = sortVector[i].frameIndex;

		if (!packer->AddImage(defFile->GetFrameSize(frame), &defFile->frameRects[frame]))
		{
			SafeDelete(packer);
			return false;
		}

        Logger::FrameworkDebug("p: %s %d",defFile->filename.GetAbsolutePathname().c_str(), frame);
	}
    Logger::FrameworkDebug("* %d x %d - success", textureRect.dx, textureRect.dy);
	
	if (lastPackedPacker)
	{
		SafeDelete(lastPackedPacker);
	}
	
	lastPackedPacker = packer;
	return true;
}

int TexturePacker::TryToPackFromSortVector(ImagePacker * packer,Vector<SizeSortItem> & tempSortVector)
{
	int packedCount = 0;
	// Packing of sorted by size images
	for (int i = 0; i < (int)tempSortVector.size(); ++i)
	{
		DefinitionFile * defFile = tempSortVector[i].defFile;
		int frame = tempSortVector[i].frameIndex;
		if (packer->AddImage(defFile->GetFrameSize(frame), &defFile->frameRects[frame]))
		{
			packedCount++;
			tempSortVector.erase(tempSortVector.begin() + i);
			i--;
		}
	}
	return packedCount;
}

float TexturePacker::TryToPackFromSortVectorWeight(ImagePacker * packer,Vector<SizeSortItem> & tempSortVector)
{
	float weight = 0.0f;
	
	// Packing of sorted by size images
	for (int i = 0; i < (int)tempSortVector.size(); ++i)
	{
		DefinitionFile * defFile = tempSortVector[i].defFile;
		int frame = tempSortVector[i].frameIndex;
		if (packer->AddImage(defFile->GetFrameSize(frame), &defFile->frameRects[frame]))
		{
			weight += (defFile->GetFrameWidth(frame) * defFile->GetFrameHeight(frame));// * weightCoeff;
			tempSortVector.erase(tempSortVector.begin() + i);
			i--;
		}
	}
	return weight;
}


bool sortFn(const SizeSortItem & a, const SizeSortItem & b)
{
	return a.imageSize > b.imageSize;	
}

void TexturePacker::PackToTexturesSeparate(const FilePath & excludeFolder, const FilePath & outputPath, List<DefinitionFile*> & defsList, eGPUFamily forGPU)
{
	lastPackedPacker = 0;
	int textureIndex = 0;
	for (List<DefinitionFile*>::iterator dfi = defsList.begin(); dfi != defsList.end(); ++dfi)
	{
		sortVector.clear();
		
		DefinitionFile * defFile = *dfi;
		for (int frame = 0; frame < defFile->frameCount; ++frame)
		{
			SizeSortItem sortItem;
			sortItem.imageSize = defFile->GetFrameWidth(frame) * defFile->GetFrameHeight(frame);
			sortItem.defFile = defFile;
			sortItem.frameIndex = frame;
			sortVector.push_back(sortItem);
		}
		std::sort(sortVector.begin(), sortVector.end(), sortFn);

		
		// try to pack for each resolution
		uint32 bestResolution = (maxTextureSize) * (maxTextureSize);
		uint32 bestXResolution, bestYResolution;
		
        Logger::FrameworkDebug("* Packing tries started: ");
		
		for (uint32 yResolution = 8; yResolution <= maxTextureSize; yResolution *= 2)
			for (uint32 xResolution = 8; xResolution <= maxTextureSize; xResolution *= 2)
			{
				Rect2i textureRect = Rect2i(0, 0, xResolution, yResolution);
				
				if (xResolution * yResolution < bestResolution)
					if (TryToPack(textureRect, defsList))
					{
						bestResolution = xResolution * yResolution;
						bestXResolution = xResolution;
						bestYResolution = yResolution;
					}
			}

        Logger::FrameworkDebug("");
        
		if (bestResolution != (maxTextureSize) * (maxTextureSize))
		{
			char textureNameWithIndex[50];
			sprintf(textureNameWithIndex, "texture%d", textureIndex++);
			FilePath textureName = outputPath + textureNameWithIndex;
            Logger::FrameworkDebug("* Writing final texture (%d x %d): %s", bestXResolution, bestYResolution , textureName.GetAbsolutePathname().c_str());
			
			PngImageExt finalImage;
			finalImage.Create(bestXResolution, bestYResolution);
			
			String fileName = defFile->filename.GetFilename();
			
			// Writing 
			for (int frame = 0; frame < defFile->frameCount; ++frame)
			{
				Rect2i *destRect = lastPackedPacker->SearchRectForPtr(&defFile->frameRects[frame]);
				if (!destRect)
				{
					AddError(Format("*** ERROR: Can't find rect for frame - %d. Definition - %s.",
									frame,
									fileName.c_str()));
				}
				
				FilePath withoutExt(defFile->filename);
                withoutExt.TruncateExtension();

				PngImageExt image;
				image.Read(FramePathHelper::GetFramePathRelative(withoutExt, frame));
				DrawToFinalImage(finalImage, image, *destRect, defFile->frameRects[frame]);
			}
			
			if (!WriteDefinition(excludeFolder, outputPath, textureNameWithIndex, defFile))
			{
				AddError(Format("* ERROR: Failed to write definition - %s.", fileName.c_str()));
			}

            textureName.ReplaceExtension(".png");
            ExportImage(&finalImage, FilePath(textureName), forGPU);
		}
	}
}

void TexturePacker::PackToTextures(const FilePath & excludeFolder, const FilePath & outputPath, List<DefinitionFile*> & defsList, eGPUFamily forGPU)
{
	lastPackedPacker = 0;
	for (List<DefinitionFile*>::iterator dfi = defsList.begin(); dfi != defsList.end(); ++dfi)
	{
		DefinitionFile * defFile = *dfi;
		for (int frame = 0; frame < defFile->frameCount; ++frame)
		{
			SizeSortItem sortItem;
			sortItem.imageSize = defFile->GetFrameWidth(frame) * defFile->GetFrameHeight(frame);
			sortItem.defFile = defFile;
			sortItem.frameIndex = frame;
			sortVector.push_back(sortItem);
		}
	}

	std::sort(sortVector.begin(), sortVector.end(), sortFn);

	// try to pack for each resolution
	uint32 bestResolution = (maxTextureSize) * (maxTextureSize);
	uint32 bestXResolution = 0, bestYResolution = 0;
	
    Logger::FrameworkDebug("* Packing tries started: ");
	
    bool needOnlySquareTexture = onlySquareTextures || NeedSquareTextureForCompression(forGPU);
	for (uint32 yResolution = 8; yResolution <= maxTextureSize; yResolution *= 2)
		 for (uint32 xResolution = 8; xResolution <= maxTextureSize; xResolution *= 2)
		 {
			 if (needOnlySquareTexture && (xResolution != yResolution))continue;
			 
			 Rect2i textureRect = Rect2i(0, 0, xResolution, yResolution);
			 
			 if (xResolution * yResolution < bestResolution)
				 if (TryToPack(textureRect, defsList))
				 {
					 bestResolution = xResolution * yResolution;
					 bestXResolution = xResolution;
					 bestYResolution = yResolution;
				 }
		 }
    Logger::FrameworkDebug("\n");

	if (bestResolution != (maxTextureSize) * (maxTextureSize))
	{
		FilePath textureName = outputPath + "texture";
        Logger::FrameworkDebug("* Writing final texture (%d x %d): %s", bestXResolution, bestYResolution , textureName.GetAbsolutePathname().c_str());
	
		PngImageExt finalImage;
		finalImage.Create(bestXResolution, bestYResolution);
		
		// Writing 
		for (List<DefinitionFile*>::iterator dfi = defsList.begin(); dfi != defsList.end(); ++dfi)
		{
			DefinitionFile * defFile = *dfi;
			String fileName = defFile->filename.GetFilename();
			
			for (int frame = 0; frame < defFile->frameCount; ++frame)
			{
				Rect2i *destRect = lastPackedPacker->SearchRectForPtr(&defFile->frameRects[frame]);
				if (!destRect)
				{
					AddError(Format("*** ERROR: Can't find rect for frame - %d. Definition - %s. ",
									frame,
									fileName.c_str()));
				}
				
				
                FilePath withoutExt(defFile->filename);
                withoutExt.TruncateExtension();

				PngImageExt image;
				image.Read(FramePathHelper::GetFramePathRelative(withoutExt, frame));
				DrawToFinalImage(finalImage, image, *destRect, defFile->frameRects[frame]);
			}
			
			if (!WriteDefinition(excludeFolder, outputPath, "texture", defFile))
			{
				AddError(Format("* ERROR: Failed to write definition - %s.", fileName.c_str()));
			}
		}

        textureName.ReplaceExtension(".png");
        ExportImage(&finalImage, textureName, forGPU);
	}else
	{
		// 
		PackToMultipleTextures(excludeFolder, outputPath, defsList, forGPU);
	}
}

void TexturePacker::PackToMultipleTextures(const FilePath & excludeFolder, const FilePath & outputPath, List<DefinitionFile*> & defList, eGPUFamily forGPU)
{
	if (defList.size() == 1)
	{
		AddError(Format("* ERROR: Failed to pack to multiple textures for path - %s.", outputPath.GetAbsolutePathname().c_str()));
	}

	for (int i = 0; i < (int)sortVector.size(); ++i)
	{
		DefinitionFile * defFile = sortVector[i].defFile;
		int frame = sortVector[i].frameIndex;
        
        Logger::FrameworkDebug("[MultiPack] prepack: %s frame: %d w:%d h:%d", defFile->filename.GetAbsolutePathname().c_str(), frame, defFile->frameRects[frame].dx, defFile->frameRects[frame].dy);
	}
	
	Vector<ImagePacker*> & packers = usedPackers;
	
	Vector<SizeSortItem> sortVectorWork = sortVector;
	
	while(sortVectorWork.size() > 0)
	{
		// try to pack for each resolution
		float maxValue = 0.0f;
		//int bestResolution = 1025 * 1025;
		
        Logger::FrameworkDebug("* Packing tries started: ");
		
		ImagePacker * bestPackerForThisStep = 0;
		Vector<SizeSortItem> newWorkVector;
		
        bool needOnlySquareTexture = onlySquareTextures || NeedSquareTextureForCompression(forGPU);
		for (uint32 yResolution = 8; yResolution <= maxTextureSize; yResolution *= 2)
			for (uint32 xResolution = 8; xResolution <= maxTextureSize; xResolution *= 2)
			{
				if (needOnlySquareTexture && (xResolution != yResolution))continue;
				
				Rect2i textureRect = Rect2i(0, 0, xResolution, yResolution);
				ImagePacker * packer = new ImagePacker(textureRect);
				
				Vector<SizeSortItem> tempSortVector = sortVectorWork;
				float n = TryToPackFromSortVectorWeight(packer, tempSortVector);
				
				if (n > maxValue) 
				{
					maxValue = n;
					SafeDelete(bestPackerForThisStep);
					bestPackerForThisStep = packer;
					newWorkVector = tempSortVector;
				}
				else
				{
					SafeDelete(packer);
				}
			}
		
		sortVectorWork = newWorkVector;

		if(bestPackerForThisStep)
			packers.push_back(bestPackerForThisStep);
	}
	
    Logger::FrameworkDebug("* Writing %d final textures", (int)packers.size());

	Vector<PngImageExt*> finalImages;
	finalImages.reserve(packers.size());
    
	for (int imageIndex = 0; imageIndex < (int)packers.size(); ++imageIndex)
	{
		PngImageExt * image = new PngImageExt();
		ImagePacker * packer = packers[imageIndex];
		image->Create(packer->GetRect().dx, packer->GetRect().dy);
		finalImages.push_back(image);
	}
	
	for (List<DefinitionFile*>::iterator defi = defList.begin(); defi != defList.end(); ++defi)
	{
		DefinitionFile * defFile = *defi;
		
		for (int frame = 0; frame < defFile->frameCount; ++frame)
		{
			Rect2i * destRect;
			ImagePacker * foundPacker = 0;
			int packerIndex = 0;
			FilePath imagePath;
			
			for (packerIndex = 0; packerIndex < (int)packers.size(); ++packerIndex)
			{
				destRect = packers[packerIndex]->SearchRectForPtr(&defFile->frameRects[frame]);
			
				if (destRect)
				{
					foundPacker = packers[packerIndex];
                    FilePath withoutExt(defFile->filename);
                    withoutExt.TruncateExtension();

					imagePath = FramePathHelper::GetFramePathRelative(withoutExt, frame);
					break;
				}
			}
			
			if (foundPacker)
			{
                Logger::FrameworkDebug("[MultiPack] pack to texture: %d", packerIndex);
                
				PngImageExt image;
				image.Read(imagePath);
				DrawToFinalImage(*finalImages[packerIndex], image, *destRect, defFile->frameRects[frame]);
			}
		}
	}
	
	for (int image = 0; image < (int)packers.size(); ++image)
	{
		char temp[256];
		sprintf(temp, "texture%d.png", image);
		FilePath textureName = outputPath + temp;
        ExportImage(finalImages[image], textureName, forGPU);
	}

	for (List<DefinitionFile*>::iterator defi = defList.begin(); defi != defList.end(); ++defi)
	{
		DefinitionFile * defFile = *defi;
		String fileName = defFile->filename.GetFilename();
		FilePath textureName = outputPath + "texture";
		
		if (!WriteMultipleDefinition(excludeFolder, outputPath, "texture", defFile))
		{
			AddError(Format("* ERROR: Failed to write definition - %s.", fileName.c_str()));
		}
	}	
}


Rect2i TexturePacker::ReduceRectToOriginalSize(const Rect2i & _input)
{
	Rect2i r = _input;
	if (CommandLineParser::Instance()->IsFlagSet("--add0pixel"))
	{
	}
	else if (CommandLineParser::Instance()->IsFlagSet("--add1pixel"))
	{
		r.dx--;
		r.dy--;
	}
	else if (CommandLineParser::Instance()->IsFlagSet("--add2pixel"))
	{
		r.dx-=2;
		r.dy-=2;
	}
	else if (CommandLineParser::Instance()->IsFlagSet("--add4pixel"))
	{
		r.dx-=4;
		r.dy-=4;
	}
	else if (CommandLineParser::Instance()->IsFlagSet("--add2sidepixel"))
	{
		r.x+=1;
		r.y+=1;
		r.dx-=2;
		r.dy-=2;
	}
	else		// add 1 pixel by default
	{
		r.dx--;
		r.dy--;
	}
	return r;
}

bool TexturePacker::WriteDefinition(const FilePath & /*excludeFolder*/, const FilePath & outputPath, const String & _textureName, DefinitionFile * defFile)
{
	String fileName = defFile->filename.GetFilename();
    Logger::FrameworkDebug("* Write definition: %s", fileName.c_str());
	
	FilePath defFilePath = outputPath + fileName;
	FILE * fp = fopen(defFilePath.GetAbsolutePathname().c_str(), "wt");
	if (!fp)return false;
	
	fprintf(fp, "%d\n", 1);
	
	String textureExtension = TextureDescriptor::GetDescriptorExtension();
	fprintf(fp, "%s%s\n", _textureName.c_str(), textureExtension.c_str());
	
	fprintf(fp, "%d %d\n", defFile->spriteWidth, defFile->spriteHeight);
	fprintf(fp, "%d\n", defFile->frameCount); 
	for (int frame = 0; frame < defFile->frameCount; ++frame)
	{
		Rect2i *destRect = lastPackedPacker->SearchRectForPtr(&defFile->frameRects[frame]);
		Rect2i origRect = defFile->frameRects[frame];
		Rect2i writeRect = ReduceRectToOriginalSize(*destRect);
		WriteDefinitionString(fp, writeRect, origRect, 0);

		if(!CheckFrameSize(Size2i(defFile->spriteWidth, defFile->spriteHeight), writeRect.GetSize()))
        {
            Logger::Warning("In sprite %s.psd frame %d has size bigger than sprite size. Frame will be cropped.", defFile->filename.GetBasename().c_str(), frame);
        }
	}
    
    for (int frameNameLine = 0; frameNameLine < (int)defFile->frameNames.size(); ++frameNameLine)
	{
		String & frameName = defFile->frameNames[frameNameLine];
		fprintf(fp, "%s\n", frameName.c_str());
	}
	
	for (int pathInfoLine = 0; pathInfoLine < (int)defFile->pathsInfo.size(); ++pathInfoLine)
	{
		String & line = defFile->pathsInfo[pathInfoLine];
		fprintf(fp, "%s", line.c_str());
	}
	
	fclose(fp);
	return true;
}

bool TexturePacker::WriteMultipleDefinition(const FilePath & /*excludeFolder*/, const FilePath & outputPath, const String & _textureName, DefinitionFile * defFile)
{
	String fileName = defFile->filename.GetFilename();
    Logger::FrameworkDebug("* Write definition: %s", fileName.c_str());
	
	FilePath defFilePath = outputPath + fileName;
	FILE * fp = fopen(defFilePath.GetAbsolutePathname().c_str(), "wt");
	if (!fp)return false;
	
	String textureExtension = TextureDescriptor::GetDescriptorExtension();
	
	Vector<int> packerIndexArray;
	packerIndexArray.resize(defFile->frameCount);
	
	Map<int, int> packerIndexToFileIndex;
	
	// find used texture indexes for this sprite
	for (int frame = 0; frame < defFile->frameCount; ++frame)
	{
		Rect2i * destRect = 0;
		int packerIndex = 0;
		for (packerIndex = 0; packerIndex < (int)usedPackers.size(); ++packerIndex)
		{
			destRect = usedPackers[packerIndex]->SearchRectForPtr(&defFile->frameRects[frame]);
			if (destRect)break;
		}
		// save packer index for frame
		packerIndexArray[frame] = packerIndex;
		// add value to map to show that this packerIndex was used
		packerIndexToFileIndex[packerIndex] = -1;
	}
		
	// write real used packers count
	fprintf(fp, "%d\n", (int)packerIndexToFileIndex.size());
	
	int realIndex = 0;
	// write user texture indexes
	for (int i = 0; i < (int)usedPackers.size(); ++i)
	{
		Map<int, int>::iterator isUsed = packerIndexToFileIndex.find(i);
		if (isUsed != packerIndexToFileIndex.end())
		{
			// here we write filename for i-th texture and write to map real index in file for this texture
			fprintf(fp, "%s%d%s\n", _textureName.c_str(), i, textureExtension.c_str());
			packerIndexToFileIndex[i] = realIndex++;
		}
	}
	
	fprintf(fp, "%d %d\n", defFile->spriteWidth, defFile->spriteHeight);
	fprintf(fp, "%d\n", defFile->frameCount); 
	for (int frame = 0; frame < defFile->frameCount; ++frame)
	{
		Rect2i * destRect = 0;
		for (int packerIndex = 0; packerIndex < (int)usedPackers.size(); ++packerIndex)
		{
			destRect = usedPackers[packerIndex]->SearchRectForPtr(&defFile->frameRects[frame]);
			if (destRect)break;
		}
		int packerIndex = packerIndexToFileIndex[packerIndexArray[frame]]; // here get real index in file for our used texture
		if (destRect)
		{
			Rect2i origRect = defFile->frameRects[frame];
			Rect2i writeRect = ReduceRectToOriginalSize(*destRect);
			WriteDefinitionString(fp, writeRect, origRect, packerIndex);

            if(!CheckFrameSize(Size2i(defFile->spriteWidth, defFile->spriteHeight), writeRect.GetSize()))
            {
                Logger::Warning("In sprite %s.psd frame %d has size bigger than sprite size. Frame will be cropped.", defFile->filename.GetBasename().c_str(), frame);
            }
		}else
		{
			AddError(Format("*** FATAL ERROR: Can't find rect in all of packers for frame - %d. Definition file - %s.",
								frame,
								fileName.c_str()));

			fclose(fp);
			FileSystem::Instance()->DeleteFile(outputPath + fileName);
			return false;
		}
	}
    
    for (int frameNameLine = 0; frameNameLine < (int)defFile->frameNames.size(); ++frameNameLine)
	{
		String & frameName = defFile->frameNames[frameNameLine];
		fprintf(fp, "%s\n", frameName.c_str());
	}
	
	for (int pathInfoLine = 0; pathInfoLine < (int)defFile->pathsInfo.size(); ++pathInfoLine)
	{
		String & line = defFile->pathsInfo[pathInfoLine];
		fprintf(fp, "%s", line.c_str());
	}
	
	fclose(fp);
	return true;
}

void TexturePacker::UseOnlySquareTextures()
{
	onlySquareTextures = true;
}

void TexturePacker::SetMaxTextureSize(uint32 _maxTextureSize)
{
	maxTextureSize = _maxTextureSize;
}

void TexturePacker::ExportImage(PngImageExt *image, const FilePath &exportedPathname, eGPUFamily forGPU)
{
    TextureDescriptor *descriptor = CreateDescriptor(forGPU);
    descriptor->pathname = TextureDescriptor::GetDescriptorPathname(exportedPathname);
    descriptor->Export(descriptor->pathname);

    image->DitherAlpha();
    image->Write(exportedPathname);

    eGPUFamily gpuFamily = (eGPUFamily)descriptor->exportedAsGpuFamily;
    if(gpuFamily != GPU_UNKNOWN)
    {
		TextureConverter::ConvertTexture(*descriptor, gpuFamily, false, quality);
        
        FileSystem::Instance()->DeleteFile(exportedPathname);
    }

	delete descriptor;
}


TextureDescriptor * TexturePacker::CreateDescriptor(eGPUFamily forGPU)
{
    TextureDescriptor *descriptor = new TextureDescriptor();

    descriptor->drawSettings.wrapModeS = descriptor->drawSettings.wrapModeT = GetDescriptorWrapMode();
    descriptor->SetGenerateMipmaps(CommandLineParser::Instance()->IsFlagSet(String("--generateMipMaps")));
	
	TexturePacker::FilterItem ftItem = GetDescriptorFilter(descriptor->GetGenerateMipMaps());
	descriptor->drawSettings.minFilter = ftItem.minFilter;
	descriptor->drawSettings.magFilter = ftItem.magFilter;
	
    if(forGPU == GPU_UNKNOWN)   // not need compression
        return descriptor;
    
    descriptor->exportedAsGpuFamily = forGPU;

    const String gpuNameFlag = "--" + GPUFamilyDescriptor::GetGPUName(forGPU);
    if(CommandLineParser::Instance()->IsFlagSet(gpuNameFlag))
    {
		String formatName = CommandLineParser::Instance()->GetParamForFlag(gpuNameFlag);
		PixelFormat format = PixelFormatDescriptor::GetPixelFormatByName(FastName(formatName.c_str()));

		// Additional check whether this format type is accepted for this GPU.
		if (IsFormatSupportedForGPU(format, forGPU))
		{
			descriptor->format = format;

			descriptor->compression[forGPU].format = format;

		}
		else
		{
			AddError(Format("Compression format '%s' is not supported for GPU '%s'",
									formatName.c_str(),
									GPUFamilyDescriptor::GetGPUName(forGPU).c_str()));
			
			descriptor->exportedAsGpuFamily = GPU_UNKNOWN;
		}
    }
    else
    {
        Logger::Warning("params for GPU %s were not set.\n", gpuNameFlag.c_str());
        
        descriptor->exportedAsGpuFamily = GPU_UNKNOWN;
    }
    
    return descriptor;
}

Texture::TextureWrap TexturePacker::GetDescriptorWrapMode()
{
	if (CommandLineParser::Instance()->IsFlagSet("--wrapClampToEdge"))
	{
		return Texture::WRAP_CLAMP_TO_EDGE;
	}
	else if (CommandLineParser::Instance()->IsFlagSet("--wrapRepeat"))
	{
		return Texture::WRAP_REPEAT;
	}
	
	// Default Wrap mode
	return Texture::WRAP_CLAMP_TO_EDGE;
}

TexturePacker::FilterItem TexturePacker::GetDescriptorFilter(bool generateMipMaps)
{
	// Default filter
	TexturePacker::FilterItem filterItem(generateMipMaps ? Texture::FILTER_LINEAR_MIPMAP_LINEAR :
															Texture::FILTER_LINEAR,
															Texture::FILTER_LINEAR);
	
	if (CommandLineParser::Instance()->IsFlagSet("--magFilterNearest"))
	{
		filterItem.magFilter = Texture::FILTER_NEAREST;
	}
	if (CommandLineParser::Instance()->IsFlagSet("--magFilterLinear"))
	{
		filterItem.magFilter = Texture::FILTER_LINEAR;
	}
	if (CommandLineParser::Instance()->IsFlagSet("--minFilterNearest"))
	{
		filterItem.minFilter = Texture::FILTER_NEAREST;
	}
	else if (CommandLineParser::Instance()->IsFlagSet("--minFilterLinear"))
	{
		filterItem.minFilter = Texture::FILTER_LINEAR;
	}
	else if (CommandLineParser::Instance()->IsFlagSet("--minFilterNearestMipmapNearest"))
	{
		filterItem.minFilter = Texture::FILTER_NEAREST_MIPMAP_NEAREST;
	}
	else if (CommandLineParser::Instance()->IsFlagSet("--minFilterLinearMipmapNearest"))
	{
		filterItem.minFilter = Texture::FILTER_LINEAR_MIPMAP_NEAREST;
	}
	else if (CommandLineParser::Instance()->IsFlagSet("--minFilterNearestMipmapLinear"))
	{
		filterItem.minFilter = Texture::FILTER_NEAREST_MIPMAP_LINEAR;
	}
	else if (CommandLineParser::Instance()->IsFlagSet("--minFilterLinearMipmapLinear"))
	{
		filterItem.minFilter = Texture::FILTER_LINEAR_MIPMAP_LINEAR;
	}

	return filterItem;
}
    
bool TexturePacker::NeedSquareTextureForCompression(eGPUFamily forGPU)
{
    if(forGPU != GPU_UNKNOWN)
    {
        const String gpuNameFlag = "--" + GPUFamilyDescriptor::GetGPUName(forGPU);
        if(CommandLineParser::Instance()->IsFlagSet(gpuNameFlag))
        {
            String formatName = CommandLineParser::Instance()->GetParamForFlag(gpuNameFlag);
            PixelFormat format = PixelFormatDescriptor::GetPixelFormatByName(FastName(formatName));
            bool result = PIXEL_FORMATS_WITH_COMPRESSION.count(format) > 0;
            return result;
        }
    }

    return false;
}

bool TexturePacker::IsFormatSupportedForGPU(PixelFormat format, eGPUFamily forGPU)
{
	if (format == FORMAT_INVALID)
	{
		return false;
	}

	Map<PixelFormat, String> supportedFormats = GPUFamilyDescriptor::GetAvailableFormatsForGpu(forGPU);
	Map<PixelFormat, String>::iterator curFormatIter = supportedFormats.find(format);

	return (curFormatIter != supportedFormats.end());
}
    
bool TexturePacker::CheckFrameSize(const Size2i &spriteSize, const Size2i &frameSize)
{
    bool isSizeCorrect = ((frameSize.dx <= spriteSize.dx) && (frameSize.dy <= spriteSize.dy));
    
    return isSizeCorrect;
}

void TexturePacker::DrawToFinalImage( PngImageExt & finalImage, PngImageExt & drawedImage, const Rect2i & drawRect, const Rect2i &frameRect )
{
	if(CommandLineParser::Instance()->IsFlagSet("--disableCropAlpha"))
	{
		finalImage.DrawImage(drawRect.x + frameRect.x, drawRect.y + frameRect.y, &drawedImage);
	}
	else
	{
		finalImage.DrawImage(drawRect.x, drawRect.y, &drawedImage);
	}

	if (CommandLineParser::Instance()->IsFlagSet("--debug"))
	{
		finalImage.DrawRect(drawRect, 0xFF0000FF);
	}
}

void TexturePacker::WriteDefinitionString(FILE *fp, const Rect2i & writeRect, const Rect2i &originRect, int textureIndex)
{
	if(CommandLineParser::Instance()->IsFlagSet("--disableCropAlpha"))
	{
		fprintf(fp, "%d %d %d %d %d %d %d\n", writeRect.x, writeRect.y, writeRect.dx, writeRect.dy, 0, 0, textureIndex);
	}
	else
	{
		fprintf(fp, "%d %d %d %d %d %d %d\n", writeRect.x, writeRect.y, writeRect.dx, writeRect.dy, originRect.x, originRect.y, textureIndex);
	}
}

const Set<String>& TexturePacker::GetErrors() const
{
	return errors;
}
    
void TexturePacker::SetConvertQuality(TextureConverter::eConvertQuality _quality)
{
    quality = _quality;
}


void TexturePacker::AddError(const String& errorMsg)
{
	Logger::Error(errorMsg.c_str());
	errors.insert(errorMsg);
}

};
