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


#include "TextureConverter.h"
#include "DXTConverter.h"
#include "PVRConverter.h"
#include "Render/Texture.h"
#include "Render/TextureDescriptor.h"
#include "Render/GPUFamilyDescriptor.h"
#include "FileSystem/FileSystem.h"

namespace DAVA
{
	FilePath TextureConverter::ConvertTexture(const TextureDescriptor &descriptor, eGPUFamily gpuFamily, bool updateAfterConversion)
	{
		FilePath outputPath;
		const String& outExtension = GPUFamilyDescriptor::GetCompressedFileExtension(gpuFamily, (DAVA::PixelFormat)descriptor.compression[gpuFamily].format);
		if(outExtension == ".pvr")
		{
			Logger::FrameworkDebug("Starting PVR (%s) conversion (%s)...",
							   GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(descriptor.compression[gpuFamily].format), descriptor.pathname.GetAbsolutePathname().c_str());
			
			outputPath = PVRConverter::Instance()->ConvertPngToPvr(descriptor, gpuFamily);
		}
		else if(outExtension == ".dds")
		{
			DAVA::Logger::FrameworkDebug("Starting DXT(%s) conversion (%s)...",
							   GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(descriptor.compression[gpuFamily].format), descriptor.pathname.GetAbsolutePathname().c_str());
			
			
			if(descriptor.IsCubeMap())
			{
				outputPath = DXTConverter::ConvertCubemapPngToDxt(descriptor, gpuFamily);
			}
			else
			{
				outputPath = DXTConverter::ConvertPngToDxt(descriptor, gpuFamily);
			}
		}
		else
		{
			DVASSERT(false);
		}

		if(updateAfterConversion)
		{
			bool wasUpdated = descriptor.UpdateCrcForFormat(gpuFamily);
			if(wasUpdated)
			{
				// Potential problem may occur in case of multithread convertion of
				// one texture: Save() will dump to drive unvalid compression info
				// and final variant of descriptor must be dumped again after finishing
				// of all threads.
				descriptor.Save();
			}
		}
		
		return outputPath;
	}
	
	bool TextureConverter::CleanupOldTextures(const DAVA::TextureDescriptor *descriptor,
											  const DAVA::eGPUFamily forGPU,
											  const DAVA::PixelFormat format)
	{
		bool result = true;
		const String & extension = GPUFamilyDescriptor::GetCompressedFileExtension(forGPU, format);
		if(extension == ".pvr")
		{
			DeleteOldPVRTextureIfPowerVr_IOS(descriptor, forGPU);
		}
		else if(extension == ".dds")
		{
			DeleteOldDXTTextureIfTegra(descriptor, forGPU);
		}
		else
		{
			DVASSERT(false);
			result = false;
		}
		
		return result;
	}
	
	void TextureConverter::DeleteOldPVRTextureIfPowerVr_IOS(const DAVA::TextureDescriptor *descriptor, const DAVA::eGPUFamily gpu)
	{
		if(!descriptor || gpu != GPU_POWERVR_IOS) return;
		
		FilePath oldPvrPath = FilePath::CreateWithNewExtension(descriptor->pathname, ".pvr");
		FileSystem::Instance()->DeleteFile(oldPvrPath);
	}
	
	void TextureConverter::DeleteOldDXTTextureIfTegra(const DAVA::TextureDescriptor *descriptor, const DAVA::eGPUFamily gpu)
	{
		if(!descriptor || gpu != GPU_TEGRA) return;
		
		FilePath oldDdsPath = FilePath::CreateWithNewExtension(descriptor->pathname, ".dds");
		FileSystem::Instance()->DeleteFile(oldDdsPath);
	}
	
	FilePath TextureConverter::GetOutputPath(const TextureDescriptor &descriptor, eGPUFamily gpuFamily)
	{
		return GPUFamilyDescriptor::CreatePathnameForGPU(&descriptor, gpuFamily);
	}
};