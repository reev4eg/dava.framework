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


#include "Render/GPUFamilyDescriptor.h"
#include "FileSystem/Logger.h"
#include "Debug/DVAssert.h"
#include "FileSystem/FilePath.h"
#include "Utils/StringFormat.h"
#include "Render/TextureDescriptor.h"
#include "Render/Texture.h"
#include "Render/PixelFormatDescriptor.h"

namespace DAVA
{
    
GPUFamilyDescriptor::GPUData GPUFamilyDescriptor::gpuData[GPU_FAMILY_COUNT];

void GPUFamilyDescriptor::SetupGPUParameters()
{
    SetupGPUFormats();
    SetupGPUPostfixes();
}

void GPUFamilyDescriptor::SetupGPUFormats()
{
    //pvr ios
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_RGBA8888] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_RGBA5551] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_RGBA4444] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_RGB888] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_RGB565] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_A8] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_PVR4] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_PVR2] = ".pvr";

    //es30
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_PVR2_2] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_PVR4_2] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_EAC_R11_UNSIGNED] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_EAC_R11_SIGNED] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_EAC_RG11_SIGNED] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_EAC_RG11_UNSIGNED] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_ETC2_RGB] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_ETC2_RGBA] = ".pvr";
    gpuData[GPU_POWERVR_IOS].availableFormats[FORMAT_ETC2_RGB_A1] = ".pvr";


    //pvr android
    gpuData[GPU_POWERVR_ANDROID].availableFormats[FORMAT_RGBA8888] = ".pvr";
    gpuData[GPU_POWERVR_ANDROID].availableFormats[FORMAT_RGBA5551] = ".pvr";
    gpuData[GPU_POWERVR_ANDROID].availableFormats[FORMAT_RGBA4444] = ".pvr";
    gpuData[GPU_POWERVR_ANDROID].availableFormats[FORMAT_RGB888] = ".pvr";
    gpuData[GPU_POWERVR_ANDROID].availableFormats[FORMAT_RGB565] = ".pvr";
    gpuData[GPU_POWERVR_ANDROID].availableFormats[FORMAT_A8] = ".pvr";
    gpuData[GPU_POWERVR_ANDROID].availableFormats[FORMAT_PVR4] = ".pvr";
    gpuData[GPU_POWERVR_ANDROID].availableFormats[FORMAT_PVR2] = ".pvr";
    gpuData[GPU_POWERVR_ANDROID].availableFormats[FORMAT_ETC1] = ".pvr";

    //tegra
    gpuData[GPU_TEGRA].availableFormats[FORMAT_RGBA8888] = ".pvr";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_RGBA5551] = ".pvr";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_RGBA4444] = ".pvr";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_RGB888] = ".pvr";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_RGB565] = ".pvr";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_A8] = ".pvr";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_DXT1] = ".dds";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_DXT1A] = ".dds";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_DXT3] = ".dds";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_DXT5] = ".dds";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_DXT5NM] = ".dds";
    gpuData[GPU_TEGRA].availableFormats[FORMAT_ETC1] = ".pvr";

    //mali
    gpuData[GPU_MALI].availableFormats[FORMAT_RGBA8888] = ".pvr";
    gpuData[GPU_MALI].availableFormats[FORMAT_RGBA5551] = ".pvr";
    gpuData[GPU_MALI].availableFormats[FORMAT_RGBA4444] = ".pvr";
    gpuData[GPU_MALI].availableFormats[FORMAT_RGB888] = ".pvr";
    gpuData[GPU_MALI].availableFormats[FORMAT_RGB565] = ".pvr";
    gpuData[GPU_MALI].availableFormats[FORMAT_A8] = ".pvr";
    gpuData[GPU_MALI].availableFormats[FORMAT_ETC1] = ".pvr";
    
    //adreno
    gpuData[GPU_ADRENO].availableFormats[FORMAT_RGBA8888] = ".pvr";
    gpuData[GPU_ADRENO].availableFormats[FORMAT_RGBA5551] = ".pvr";
    gpuData[GPU_ADRENO].availableFormats[FORMAT_RGBA4444] = ".pvr";
    gpuData[GPU_ADRENO].availableFormats[FORMAT_RGB888] = ".pvr";
    gpuData[GPU_ADRENO].availableFormats[FORMAT_RGB565] = ".pvr";
    gpuData[GPU_ADRENO].availableFormats[FORMAT_A8] = ".pvr";
    gpuData[GPU_ADRENO].availableFormats[FORMAT_ETC1] = ".pvr";
    gpuData[GPU_ADRENO].availableFormats[FORMAT_ATC_RGB] = ".dds";
	gpuData[GPU_ADRENO].availableFormats[FORMAT_ATC_RGBA_EXPLICIT_ALPHA] = ".dds";
	gpuData[GPU_ADRENO].availableFormats[FORMAT_ATC_RGBA_INTERPOLATED_ALPHA] = ".dds";
}

void GPUFamilyDescriptor::SetupGPUPostfixes()
{
    gpuData[GPU_POWERVR_IOS].name = "PowerVR_iOS";
    gpuData[GPU_POWERVR_ANDROID].name = "PowerVR_Android";
    gpuData[GPU_TEGRA].name = "tegra";
    gpuData[GPU_MALI].name = "mali";
    gpuData[GPU_ADRENO].name = "adreno";
}

    
const Map<PixelFormat, String> & GPUFamilyDescriptor::GetAvailableFormatsForGpu(eGPUFamily gpuFamily)
{
    DVASSERT(0 <= gpuFamily && gpuFamily < GPU_FAMILY_COUNT);
    
    return gpuData[gpuFamily].availableFormats;
}

eGPUFamily GPUFamilyDescriptor::GetGPUForPathname(const FilePath &pathname)
{
    const String filename = pathname.GetFilename();
    
    for(int32 i = 0; i < GPU_FAMILY_COUNT; ++i)
    {
        eGPUFamily gpu = (eGPUFamily)i;
        
        String strForFind(Format(".%s.", gpuData[gpu].name.c_str()));
        if(String::npos != filename.rfind(strForFind))
        {
            return gpu;
        }
    }

    
    return GPU_UNKNOWN;
}
 
FilePath GPUFamilyDescriptor::CreatePathnameForGPU(const TextureDescriptor *descriptor, const eGPUFamily gpuFamily)
{
    DVASSERT(descriptor);

	if(GPU_UNKNOWN == gpuFamily)
		return CreatePathnameForGPU(descriptor->pathname, GPU_UNKNOWN, FORMAT_INVALID);

	eGPUFamily requestedGPU = gpuFamily;
	
	PixelFormat requestedFormat = FORMAT_INVALID;
    if(descriptor->IsCompressedFile())
    {
        requestedGPU = (eGPUFamily)descriptor->exportedAsGpuFamily;
        requestedFormat = (PixelFormat)descriptor->format;
    }
	else
	{
		requestedFormat = (PixelFormat) descriptor->compression[gpuFamily].format;
	}
    
    return CreatePathnameForGPU(descriptor->pathname, requestedGPU, requestedFormat);
}

FilePath GPUFamilyDescriptor::CreatePathnameForGPU(const FilePath & pathname, const eGPUFamily gpuFamily, const PixelFormat pixelFormat)
{
    return FilePath::CreateWithNewExtension(pathname, GetFilenamePostfix(gpuFamily, pixelFormat));
}

    
    
const String & GPUFamilyDescriptor::GetGPUName(const eGPUFamily gpuFamily)
{
    DVASSERT(0 <= gpuFamily && gpuFamily < GPU_FAMILY_COUNT);

    return gpuData[gpuFamily].name;
}

eGPUFamily GPUFamilyDescriptor::GetGPUByName(const String & name)
{
    for(int32 i = 0; i < GPU_FAMILY_COUNT; ++i)
    {
        eGPUFamily gpu = (eGPUFamily)i;
        if(name == gpuData[gpu].name)
        {
            return gpu;
        }
    }
    
    return GPU_UNKNOWN;
}

bool GPUFamilyDescriptor::IsFormatSupported(const eGPUFamily gpu, const PixelFormat format)
{
	if(gpu <= GPU_UNKNOWN || gpu >= GPU_FAMILY_COUNT)
	{
		return false;
	}
	return gpuData[gpu].availableFormats.find(format) != gpuData[gpu].availableFormats.end();
}
    
const String & GPUFamilyDescriptor::GetCompressedFileExtension(const eGPUFamily gpuFamily, const PixelFormat pixelFormat)
{
    DVASSERT(0 <= gpuFamily && gpuFamily < GPU_FAMILY_COUNT);

    Map<PixelFormat, String>::const_iterator format = gpuData[gpuFamily].availableFormats.find(pixelFormat);
    DVASSERT(format != gpuData[gpuFamily].availableFormats.end());
    
    return format->second;
}

    
String GPUFamilyDescriptor::GetFilenamePostfix(const eGPUFamily gpuFamily, const PixelFormat pixelFormat)
{
    DVASSERT(gpuFamily < GPU_FAMILY_COUNT);

    if(gpuFamily == GPU_UNKNOWN || pixelFormat == FORMAT_INVALID)
        return ".png";
        
    
    Map<PixelFormat, String>::const_iterator format = gpuData[gpuFamily].availableFormats.find(pixelFormat);
	if(format == gpuData[gpuFamily].availableFormats.end())
	{
		Logger::Error("[GPUFamilyDescriptor::GetFilenamePostfix: can't find format %s for gpu %s]", PixelFormatDescriptor::GetPixelFormatString(pixelFormat), gpuData[gpuFamily].name.c_str());
		return ".png";
	}

	String postfix = "." + gpuData[gpuFamily].name + format->second;
    return postfix;
}
    
    
};
