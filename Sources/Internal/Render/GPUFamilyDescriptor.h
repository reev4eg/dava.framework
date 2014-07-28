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


#ifndef __GPU_FAMILY_H__
#define __GPU_FAMILY_H__

#include "Base/BaseTypes.h"
#include "Render/RenderBase.h"

namespace DAVA
{

class FilePath;
class TextureDescriptor;
class GPUFamilyDescriptor
{
    
public:
    
    struct GPUData
    {
        void SetName(const String & newName);
        
        String name;
        String prefix;
        
        Map<PixelFormat, String> availableFormats;
    };
    
public:

    static void SetupGPUParameters();
    static const Map<PixelFormat, String> & GetAvailableFormatsForGpu(eGPUFamily gpuFamily);

    static eGPUFamily GetGPUForPathname(const FilePath &pathname);
    static FilePath CreatePathnameForGPU(const TextureDescriptor *descriptor, const eGPUFamily gpuFamily);
    static FilePath CreatePathnameForGPU(const FilePath & pathname, const eGPUFamily gpuFamily, const PixelFormat pixelFormat);
    
    
    static String GetFilenamePostfix(const eGPUFamily gpuFamily, const PixelFormat pixelFormat);

    static const String & GetGPUName(const eGPUFamily gpuFamily);
    static const String & GetCompressedFileExtension(const eGPUFamily gpuFamily, const PixelFormat pixelFormat);
    
    static eGPUFamily GetGPUByName(const String & name);
	
	static bool IsFormatSupported(const eGPUFamily gpu, const PixelFormat format);
    
    static eGPUFamily ConvertValueToGPU(const int32 value);
    static bool IsGPUForDevice(const eGPUFamily gpu);
    
protected:

    static void SetupGPUFormats();
    static void SetupGPUPostfixes();

protected:
    
    static GPUData gpuData[GPU_FAMILY_COUNT];
};
    
};

#endif // __GPU_FAMILY_H__
