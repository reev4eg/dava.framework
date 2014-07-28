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


#include "Render/RenderBase.h"
#include "Render/Texture.h"
#include "Utils/Utils.h"
#include "FileSystem/Logger.h"
#include "Debug/DVAssert.h"
#include "Utils/Utils.h"
#include "Render/RenderManager.h"
#include "Utils/StringFormat.h"
#include "Platform/SystemTimer.h"
#include "FileSystem/File.h"
#include "Render/D3D9Helpers.h"
#include "FileSystem/FileSystem.h"
#include "Render/OGLHelpers.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"

#if defined(__DAVAENGINE_IPHONE__) 
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#elif defined(__DAVAENGINE_MACOS__)
#include <ApplicationServices/ApplicationServices.h>
#endif //PLATFORMS

#include "Render/Image/ImageSystem.h"
#include "Render/Image/ImageConvert.h"
#include "Render/OGLHelpers.h"

#include "Render/TextureDescriptor.h"
#include "Render/GPUFamilyDescriptor.h"
#include "Job/JobManager.h"
#include "Job/JobWaiter.h"
#include "Math/MathHelpers.h"


#ifdef __DAVAENGINE_ANDROID__
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif //GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif //GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#endif //__DAVAENGINE_ANDROID__

namespace DAVA 
{
    
#if defined __DAVAENGINE_OPENGL__
static GLuint CUBE_FACE_GL_NAMES[] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};
	
#define SELECT_GL_TEXTURE_TYPE(__engineTextureType__) ((Texture::TEXTURE_CUBE == __engineTextureType__) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D)
#endif //#if defined __DAVAENGINE_OPENGL__
	
static int32 CUBE_FACE_MAPPING[] =
{
	Texture::CUBE_FACE_POSITIVE_X,
	Texture::CUBE_FACE_NEGATIVE_X,
	Texture::CUBE_FACE_POSITIVE_Y,
	Texture::CUBE_FACE_NEGATIVE_Y,
	Texture::CUBE_FACE_POSITIVE_Z,
	Texture::CUBE_FACE_NEGATIVE_Z
};

static DAVA::String FACE_NAME_SUFFIX[] =
{
    DAVA::String("_px"),
    DAVA::String("_nx"),
    DAVA::String("_py"),
    DAVA::String("_ny"),
    DAVA::String("_pz"),
    DAVA::String("_nz")
};
	
class TextureMemoryUsageInfo
{
public:
	TextureMemoryUsageInfo()
	{
		pvrTexturesMemoryUsed = 0;
		texturesMemoryUsed = 0;
		fboMemoryUsed = 0;
	}
	
	void AllocPVRTexture(int size)
	{
		pvrTexturesMemoryUsed += size;
	}
	
	void ReleasePVRTexture(int size)
	{
		pvrTexturesMemoryUsed -= size;
	}
	
	void AllocTexture(int size)
	{
		texturesMemoryUsed += size;
	}
	
	void ReleaseTexture(int size)
	{
		texturesMemoryUsed -= size;
	}
	
	void AllocFBOTexture(int size)
	{
		fboMemoryUsed += size;
	}
	
	void ReleaseFBOTexture(int size)
	{
		fboMemoryUsed -= size;
	}
	
	// STATISTICS
	int pvrTexturesMemoryUsed;
	int texturesMemoryUsed;
	int	fboMemoryUsed;
};

eGPUFamily Texture::defaultGPU = GPU_UNKNOWN;
    
static TextureMemoryUsageInfo texMemoryUsageInfo;
	
TexturesMap Texture::textureMap;

Mutex Texture::textureMapMutex;

static int32 textureFboCounter = 0;

bool Texture::pixelizationFlag = false;

// Main constructors
Texture * Texture::Get(const FilePath & pathName)
{
    textureMapMutex.Lock();

	Texture * texture = NULL;
	TexturesMap::iterator it = textureMap.find(FILEPATH_MAP_KEY(pathName));
	if (it != textureMap.end())
	{
		texture = it->second;
		texture->Retain();

        textureMapMutex.Unlock();

		return texture;
	}

    textureMapMutex.Unlock();

	return 0;
}

void Texture::AddToMap(Texture *tex)
{
    if(!tex->texDescriptor->pathname.IsEmpty())
    {
        textureMapMutex.Lock();
		textureMap[FILEPATH_MAP_KEY(tex->texDescriptor->pathname)] = tex;
        textureMapMutex.Unlock();
    }
}


Texture::Texture()
:	id(0)
,	width(0)
,	height(0)
,	depthFormat(DEPTH_NONE)
,	isRenderTarget(false)
,   loadedAsFile(GPU_UNKNOWN)
,	textureType(Texture::TEXTURE_2D)
,	isPink(false)
,	state(STATE_INVALID)
,	invalidater(NULL)
{
#ifdef __DAVAENGINE_DIRECTX9__
	saveTexture = 0;
	renderTargetModified = false;
    renderTargetAutosave = true;
#endif //#ifdef __DAVAENGINE_DIRECTX9__


#ifdef __DAVAENGINE_OPENGL__
	fboID = -1;
	rboID = -1;
#if defined(__DAVAENGINE_ANDROID__)
    stencilRboID = -1;
#endif

#endif

	texDescriptor = new TextureDescriptor();
}

Texture::~Texture()
{
    ReleaseTextureData();
	SafeDelete(texDescriptor);
}
    
void Texture::ReleaseTextureData()
{
	state = STATE_INVALID;

	ReleaseTextureDataContainer * container = new ReleaseTextureDataContainer();
	container->textureType = textureType;
	container->id = id;
	container->fboID = fboID;
	container->rboID = rboID;
#if defined(__DAVAENGINE_ANDROID__)
    container->stencilRboID = stencilRboID;
#endif
	ScopedPtr<Job> job = JobManager::Instance()->CreateJob(JobManager::THREAD_MAIN, Message(this, &Texture::ReleaseTextureDataInternal, container), Job::NO_FLAGS);
    
    id = 0;
	fboID = -1;
	rboID = -1;
#if defined(__DAVAENGINE_ANDROID__)
    stencilRboID = -1;
#endif
    isRenderTarget = false;
}

void Texture::ReleaseTextureDataInternal(BaseObject * caller, void * param, void *callerData)
{
	ReleaseTextureDataContainer * container = (ReleaseTextureDataContainer*) param;
	DVASSERT(container);

#if defined(__DAVAENGINE_OPENGL__)
	//if(RenderManager::Instance()->GetTexture() == this)
	//{//to avoid drawing deleted textures
	//	RenderManager::Instance()->SetTexture(0);
	//}

	//VI: reset texture for the current texture type in order to avoid
	//issue when cubemap texture was deleted while being binded to the state
	if(RenderManager::Instance()->lastBindedTexture[container->textureType] == container->id)
	{
		RenderManager::Instance()->HWglForceBindTexture(0, container->textureType);
	}
    
	if(container->fboID != (uint32)-1)
	{
		RENDER_VERIFY(glDeleteFramebuffers(1, &container->fboID));
	}
    
#if defined(__DAVAENGINE_ANDROID__)
    if (container->stencilRboID != (uint32)-1)
    {
        RENDER_VERIFY(glDeleteRenderbuffers(1, &container->stencilRboID));
    }
#endif

	if (container->rboID != (uint32)-1)
	{
		RENDER_VERIFY(glDeleteRenderbuffers(1, &container->rboID));
	}

	if(container->id)
	{
		RENDER_VERIFY(glDeleteTextures(1, &container->id));
	}
	
#elif defined(__DAVAENGINE_DIRECTX9__)
	D3DSafeRelease(id);
	D3DSafeRelease(saveTexture);
#endif //#if defined(__DAVAENGINE_OPENGL__)

	SafeDelete(container);
}


Texture * Texture::CreateTextFromData(PixelFormat format, uint8 * data, uint32 width, uint32 height, bool generateMipMaps, const char * addInfo)
{
	Texture * tx = CreateFromData(format, data, width, height, generateMipMaps);
    
	if (!addInfo)
    {
        tx->texDescriptor->pathname = Format("Text texture %d", textureFboCounter);
    }
	else
    {
        tx->texDescriptor->pathname = Format("Text texture %d info:%s", textureFboCounter, addInfo);
    }
    AddToMap(tx);
    
	textureFboCounter++;
	return tx;
}
	
void Texture::TexImage(int32 level, uint32 width, uint32 height, const void * _data, uint32 dataSize, uint32 cubeFaceId)
{
#if defined(__DAVAENGINE_OPENGL__)

	int32 saveId = RenderManager::Instance()->HWglGetLastTextureID(textureType);
	
	RenderManager::Instance()->HWglBindTexture(id, textureType);

    RENDER_VERIFY(glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ));

	const PixelFormatDescriptor & formatDescriptor = PixelFormatDescriptor::GetPixelFormatDescriptor(texDescriptor->format);
    if(FORMAT_INVALID != formatDescriptor.formatID)
    {
		GLuint textureMode = GL_TEXTURE_2D;
		
		if(cubeFaceId != Texture::CUBE_FACE_INVALID)
		{
			textureMode = CUBE_FACE_GL_NAMES[cubeFaceId];
		}
		
		if (formatDescriptor.isCompressedFormat)
        {
			RENDER_VERIFY(glCompressedTexImage2D(textureMode, level, formatDescriptor.internalformat, width, height, 0, dataSize, _data));
        }
        else
        {
            RENDER_VERIFY(glTexImage2D(textureMode, level, formatDescriptor.internalformat, width, height, 0, formatDescriptor.format, formatDescriptor.type, _data));
        }
    }
	
	if(0 != saveId)
	{
		RenderManager::Instance()->HWglBindTexture(saveId, textureType);
	}

#elif defined(__DAVAENGINE_DIRECTX9__)
	if (!id)
		return;

	D3DLOCKED_RECT rect;
	HRESULT hr = id->LockRect(level, &rect, 0, 0);
	if (FAILED(hr))
	{
		Logger::Error("[TextureDX9] Could not lock DirectX9 Texture.");
		return;
	}

	// \todo instead of hardcoding transformations, use ImageConvert.
	int32 pixelSizeInBits = PixelFormatDescriptor::GetPixelFormatSize(format);
	if (format ==  FORMAT_RGBA8888)
	{
		//int32 pitchInBytes = 

		uint8 * destBits = (uint8*)rect.pBits;
		uint8 * sourceBits = (uint8*)_data;
		for (uint32 h = 0; h < height * width; ++h)
		{
            destBits[0] = sourceBits[2];
            destBits[1] = sourceBits[1];
            destBits[2] = sourceBits[0];
            destBits[3] = sourceBits[3];

			destBits += 4;
			sourceBits += 4;
		}
	}else 
	{
		// pixel conversion from R4G4B4A4 (OpenGL format) => A4R4G4B4 (DirectX format)
		uint16 * destBits = (uint16*)rect.pBits;
		uint16 * sourceBits = (uint16*)_data;
		for (uint32 h = 0; h < height * width; ++h)
		{
			uint32 rgba = sourceBits[0];

			destBits[0] = ((rgba & 0xF) << 12) | (rgba >> 4);

			destBits ++;
			sourceBits ++;
		}	
	}

	id->UnlockRect(level);
#endif 
}
    
Texture * Texture::CreateFromData(PixelFormat _format, const uint8 *_data, uint32 _width, uint32 _height, bool generateMipMaps)
{
	Image *image = Image::CreateFromData(_width, _height, _format, _data);
	if(!image) return NULL;

	Texture * texture = new Texture();
	texture->texDescriptor->Initialize(WRAP_CLAMP_TO_EDGE, generateMipMaps);
    
    Vector<Image *> *images = new Vector<Image *>();
    images->push_back(image);
	
    texture->SetParamsFromImages(images);
	texture->FlushDataToRenderer(images);

	return texture;
}
    
Texture * Texture::CreateFromData(Image *image, bool generateMipMaps)
{
	Texture * texture = new Texture();
	texture->texDescriptor->Initialize(WRAP_CLAMP_TO_EDGE, generateMipMaps);
    
    Vector<Image *> *images = new Vector<Image *>();
    image->Retain();
    images->push_back(image);
	
    texture->SetParamsFromImages(images);
	texture->FlushDataToRenderer(images);
    
	return texture;
}

	
void Texture::SetWrapMode(TextureWrap wrapS, TextureWrap wrapT)
{
#if defined(__DAVAENGINE_OPENGL__)
	int32 saveId = RenderManager::Instance()->HWglGetLastTextureID(textureType);
	
	RenderManager::Instance()->HWglBindTexture(id, textureType);
	
	RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_WRAP_S, TEXTURE_WRAP_MAP[wrapS]));
	RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_WRAP_T, TEXTURE_WRAP_MAP[wrapT]));

	if (saveId != 0)
	{
		RenderManager::Instance()->HWglBindTexture(saveId, textureType);
	}
#elif defined(__DAVAENGINE_DIRECTX9____)
	
#endif //#if defined(__DAVAENGINE_OPENGL__) 
}

void Texture::SetMinMagFilter(TextureFilter minFilter, TextureFilter magFilter)
{
#if defined(__DAVAENGINE_OPENGL__)
	int32 saveId = RenderManager::Instance()->HWglGetLastTextureID(textureType);

	RenderManager::Instance()->HWglBindTexture(id, textureType);

	RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_MIN_FILTER, TEXTURE_FILTER_MAP[minFilter]));
	RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_MAG_FILTER, TEXTURE_FILTER_MAP[magFilter]));

	if (saveId != 0)
	{
		RenderManager::Instance()->HWglBindTexture(saveId, textureType);
	}
#elif defined(__DAVAENGINE_DIRECTX9____)
	
#endif //#if defined(__DAVAENGINE_OPENGL__) 
}
	
void Texture::GenerateMipmaps()
{
	ScopedPtr<Job> job = JobManager::Instance()->CreateJob(JobManager::THREAD_MAIN, Message(this, &Texture::GenerateMipmapsInternal));
	JobInstanceWaiter waiter(job);
	waiter.Wait();
}

void Texture::GenerateMipmapsInternal(BaseObject * caller, void * param, void *callerData)
{
	const PixelFormatDescriptor & formatDescriptor = PixelFormatDescriptor::GetPixelFormatDescriptor(texDescriptor->format);
	if(formatDescriptor.isCompressedFormat)
    {
		return;
	}
    
    
#if defined(__DAVAENGINE_OPENGL__)
    
	int32 saveId = RenderManager::Instance()->HWglGetLastTextureID(textureType);
	
	RenderManager::Instance()->HWglBindTexture(id, textureType);
		
    Image * image0 = CreateImageFromMemory(RenderState::RENDERSTATE_2D_BLEND);
    Vector<Image *> images = image0->CreateMipMapsImages(texDescriptor->dataSettings.GetIsNormalMap());
    SafeRelease(image0);

    for(uint32 i = 1; i < (uint32)images.size(); ++i)
        TexImage((images[i]->mipmapLevel != (uint32)-1) ? images[i]->mipmapLevel : i, images[i]->width, images[i]->height, images[i]->data, images[i]->dataSize, images[i]->cubeFaceID);

    ReleaseImages(&images);

    RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	if (saveId != 0)
	{
		RenderManager::Instance()->HWglBindTexture(saveId, textureType);
	}
	
	
#elif defined(__DAVAENGINE_DIRECTX9__)

#endif // #if defined(__DAVAENGINE_OPENGL__)
}



void Texture::GeneratePixelesation()
{
	ScopedPtr<Job> job = JobManager::Instance()->CreateJob(JobManager::THREAD_MAIN, Message(this, &Texture::GeneratePixelesationInternal));
	JobInstanceWaiter waiter(job);
	waiter.Wait();
}

void Texture::GeneratePixelesationInternal(BaseObject * caller, void * param, void *callerData)
{

#if defined(__DAVAENGINE_OPENGL__)
    
	int saveId = RenderManager::Instance()->HWglGetLastTextureID(textureType);
	
	RenderManager::Instance()->HWglBindTexture(id, textureType);
		
	RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    
	if (saveId != 0)
	{
		RenderManager::Instance()->HWglBindTexture(saveId, textureType);
	}
	
	
#elif defined(__DAVAENGINE_DIRECTX9__)
    
#endif // #if defined(__DAVAENGINE_OPENGL__)
}
    

Texture * Texture::CreateFromImage(TextureDescriptor *descriptor, eGPUFamily gpu)
{
	Texture * texture = new Texture();
	texture->texDescriptor->Initialize(descriptor);

    Vector<Image *> * images = new Vector<Image *> ();
    
	bool loaded = texture->LoadImages(gpu, images);
    if(!loaded)
	{
		Logger::Error("[Texture::CreateFromImage] Cannot load texture from image");

        SafeDelete(images);
		SafeRelease(texture);
		return NULL;
	}

	texture->SetParamsFromImages(images);
	texture->FlushDataToRenderer(images);

	return texture;
}

bool Texture::LoadImages(eGPUFamily gpu, Vector<Image *> * images)
{
	if(!IsLoadAvailable(gpu))
		return false;
	
    int32 baseMipMap = GetBaseMipMap();

	if(texDescriptor->IsCubeMap() && (GPU_UNKNOWN == gpu))
	{
		Vector<FilePath> faceNames;
		GenerateCubeFaceNames(texDescriptor->GetSourceTexturePathname(), faceNames);

		for(size_t i = 0; i < faceNames.size(); ++i)
		{
            Vector<Image *> imageFace;
            ImageSystem::Instance()->Load(faceNames[i], imageFace,baseMipMap);
            if(imageFace.size() == 0)
			{
				Logger::Error("[Texture::LoadImages] Cannot open file %s", faceNames[i].GetAbsolutePathname().c_str());

				ReleaseImages(images);
				return false;
			}

			DVASSERT(imageFace.size() == 1);

			imageFace[0]->cubeFaceID = CUBE_FACE_MAPPING[i];
			imageFace[0]->mipmapLevel = 0;

            if(texDescriptor->GetGenerateMipMaps())
            {
                Vector<Image *> mipmapsImages = imageFace[0]->CreateMipMapsImages();
                images->insert(images->end(), mipmapsImages.begin(), mipmapsImages.end());
                SafeRelease(imageFace[0]);
            }
            else
            {
			    images->push_back(imageFace[0]);
            }
		}
	}
	else
	{
		FilePath imagePathname = GPUFamilyDescriptor::CreatePathnameForGPU(texDescriptor, gpu);
        ImageSystem::Instance()->Load(imagePathname, *images,baseMipMap);
        
        if(images->size() == 1 && gpu == GPU_UNKNOWN && texDescriptor->GetGenerateMipMaps())
        {
            Image * img = *images->begin();
            *images = img->CreateMipMapsImages(texDescriptor->dataSettings.GetIsNormalMap());
            SafeRelease(img);
        }
    }

	if(0 == images->size())
		return false;

	bool isSizeCorrect = CheckImageSize(*images);
	if(!isSizeCorrect)
	{
		ReleaseImages(images);
		return false;
	}

	isPink = false;
	state = STATE_DATA_LOADED;

	return true;
}


void Texture::ReleaseImages(Vector<Image *> *images)
{
	for_each(images->begin(), images->end(), SafeRelease<Image>);
	images->clear();
}

void Texture::SetParamsFromImages(const Vector<Image *> * images)
{
	DVASSERT(images->size() != 0);

    Image *img = *images->begin();
	width = img->width;
	height = img->height;
	texDescriptor->format = img->format;

	textureType = (img->cubeFaceID != Texture::CUBE_FACE_INVALID) ? Texture::TEXTURE_CUBE : Texture::TEXTURE_2D;
    
    state = STATE_DATA_LOADED;
}

void Texture::FlushDataToRenderer(Vector<Image *> * images)
{
	JobManager::Instance()->CreateJob(JobManager::THREAD_MAIN, Message(this, &Texture::FlushDataToRendererInternal, images));
}

void Texture::FlushDataToRendererInternal(BaseObject * caller, void * param, void *callerData)
{
    Vector<Image *> * images = static_cast< Vector<Image *> * >(param);
    
	DVASSERT(images->size() != 0);
	DVASSERT(Thread::IsMainThread());

#if defined(__DAVAENGINE_OPENGL__)
	GenerateID();
#elif defined(__DAVAENGINE_DIRECTX9__)
	id = CreateTextureNative(Vector2((float32)_width, (float32)_height), texture->format, false, 0);
#endif //#if defined(__DAVAENGINE_OPENGL__)

	for(uint32 i = 0; i < (uint32)images->size(); ++i)
	{
        Image *img = (*images)[i];
		TexImage((img->mipmapLevel != (uint32)-1) ? img->mipmapLevel : i, img->width, img->height, img->data, img->dataSize, img->cubeFaceID);
	}

#if defined(__DAVAENGINE_OPENGL__)

	int32 saveId = RenderManager::Instance()->HWglGetLastTextureID(textureType);

	RenderManager::Instance()->HWglBindTexture(id, textureType);

	RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_WRAP_S, TEXTURE_WRAP_MAP[texDescriptor->drawSettings.wrapModeS]));
	RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_WRAP_T, TEXTURE_WRAP_MAP[texDescriptor->drawSettings.wrapModeT]));

    if (pixelizationFlag)
    {
        RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_MIN_FILTER, TEXTURE_FILTER_MAP[FILTER_NEAREST]));
        RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_MAG_FILTER, TEXTURE_FILTER_MAP[FILTER_NEAREST]));
    }
    else
    {
        RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_MIN_FILTER, TEXTURE_FILTER_MAP[texDescriptor->drawSettings.minFilter]));
        RENDER_VERIFY(glTexParameteri(SELECT_GL_TEXTURE_TYPE(textureType), GL_TEXTURE_MAG_FILTER, TEXTURE_FILTER_MAP[texDescriptor->drawSettings.magFilter]));
    }

	RenderManager::Instance()->HWglBindTexture(saveId, textureType);
#elif defined(__DAVAENGINE_DIRECTX9__)

#endif //#if defined(__DAVAENGINE_OPENGL__)

	state = STATE_VALID;

	ReleaseImages(images);
    SafeDelete(images);
}

bool Texture::CheckImageSize(const Vector<DAVA::Image *> &imageSet)
{
    for (int32 i = 0; i < (int32)imageSet.size(); ++i)
    {
        if(!IsPowerOf2(imageSet[i]->GetWidth()) || !IsPowerOf2(imageSet[i]->GetHeight()))
        {
            return false;
        }
    }
    
    return true;
}

Texture * Texture::CreateFromFile(const FilePath & pathName, const FastName &group, TextureType typeHint)
{
	Texture * texture = PureCreate(pathName, group);
 	if(!texture)
	{
		texture = CreatePink(typeHint);
        texture->texDescriptor->pathname = (!pathName.IsEmpty()) ? TextureDescriptor::GetDescriptorPathname(pathName) : FilePath();
        
        AddToMap(texture);
	}

	return texture;
}

Texture * Texture::PureCreate(const FilePath & pathName, const FastName &group)
{
	if(pathName.IsEmpty() || pathName.GetType() == FilePath::PATH_IN_MEMORY)
		return NULL;

    if(!RenderManager::Instance()->GetOptions()->IsOptionEnabled(RenderOptions::TEXTURE_LOAD_ENABLED))
        return NULL;

    FilePath descriptorPathname = TextureDescriptor::GetDescriptorPathname(pathName);
    Texture * texture = Texture::Get(descriptorPathname);
	if (texture) return texture;
    
    TextureDescriptor *descriptor = TextureDescriptor::CreateFromFile(descriptorPathname);
    if(!descriptor) return NULL;
    
    descriptor->SetQualityGroup(group);

	eGPUFamily gpuForLoading = GetGPUForLoading(defaultGPU, descriptor);
	texture = CreateFromImage(descriptor, gpuForLoading);
	if(texture)
	{
		texture->loadedAsFile = gpuForLoading;
		AddToMap(texture);
	}

	delete descriptor;
	return texture;
}
    

void Texture::Reload()
{
    ReloadAs(loadedAsFile);
}
    
void Texture::ReloadAs(eGPUFamily gpuFamily)
{
    DVASSERT(isRenderTarget == false);
    
    ReleaseTextureData();

	bool descriptorReloaded = texDescriptor->Reload();
    
	eGPUFamily gpuForLoading = GetGPUForLoading(gpuFamily, texDescriptor);
    Vector<Image *> *images = new Vector<Image *> ();
    
    bool loaded = false;
    if(descriptorReloaded && RenderManager::Instance()->GetOptions()->IsOptionEnabled(RenderOptions::TEXTURE_LOAD_ENABLED))
    {
	    loaded = LoadImages(gpuForLoading, images);
    }

	if(loaded)
	{
		loadedAsFile = gpuForLoading;

		SetParamsFromImages(images);
		FlushDataToRenderer(images);
	}
	else
    {
        SafeDelete(images);
        
        Logger::Error("[Texture::ReloadAs] Cannot reload from file %s", texDescriptor->pathname.GetAbsolutePathname().c_str());
        MakePink();
    }
}

    
bool Texture::IsLoadAvailable(const eGPUFamily gpuFamily) const
{
    if(texDescriptor->IsCompressedFile())
    {
        return true;
    }
    
    DVASSERT(gpuFamily < GPU_FAMILY_COUNT);
    
    if(gpuFamily != GPU_UNKNOWN && texDescriptor->compression[gpuFamily].format == FORMAT_INVALID)
    {
        return false;
    }
    
    return true;
}

    
int32 Texture::Release()
{
	if(GetRetainCount() == 1)
	{
        textureMapMutex.Lock();
		textureMap.erase(FILEPATH_MAP_KEY(texDescriptor->pathname));
        textureMapMutex.Unlock();
	}
	return BaseObject::Release();
}
	
Texture * Texture::CreateFBO(uint32 w, uint32 h, PixelFormat format, DepthFormat _depthFormat)
{
	int32 dx = Max((int32)w, 8);
    EnsurePowerOf2(dx);
    
	int32 dy = Max((int32)h, 8);
    EnsurePowerOf2(dy);
    

#if defined(__DAVAENGINE_OPENGL__)

	Texture *tx = Texture::CreateFromData(format, NULL, dx, dy, false);
	DVASSERT(tx);

	tx->depthFormat = _depthFormat;

	tx->HWglCreateFBOBuffers();	

#elif defined(__DAVAENGINE_DIRECTX9__)

	// TODO: Create FBO
	Texture * tx = new Texture();

	tx->width = dx;
	tx->height = dy;
	tx->format = format;

	RenderManager::Instance()->LockNonMain();
	tx->id = CreateTextureNative(Vector2((float32)tx->width, (float32)tx->height), tx->format, true, 0);
	RenderManager::Instance()->UnlockNonMain();

	tx->state = STATE_VALID;
#endif 


    tx->isRenderTarget = true;
    tx->texDescriptor->pathname = Format("FBO texture %d", textureFboCounter);
	AddToMap(tx);
	
	textureFboCounter++;
	
	return tx;
}

#if defined(__DAVAENGINE_OPENGL__)
void Texture::HWglCreateFBOBuffers()
{
	JobManager::Instance()->CreateJob(JobManager::THREAD_MAIN, Message(this, &Texture::HWglCreateFBOBuffersInternal));
}

void Texture::HWglCreateFBOBuffersInternal(BaseObject * caller, void * param, void *callerData)
{
	GLint saveFBO = RenderManager::Instance()->HWglGetLastFBO();
	GLint saveTexture = RenderManager::Instance()->HWglGetLastTextureID(textureType);

	RenderManager::Instance()->HWglBindTexture(id, textureType);

	RENDER_VERIFY(glGenFramebuffers(1, &fboID));
	RenderManager::Instance()->HWglBindFBO(fboID);
    
	if(DEPTH_RENDERBUFFER == depthFormat)
	{
		RENDER_VERIFY(glGenRenderbuffers(1, &rboID));
		RENDER_VERIFY(glBindRenderbuffer(GL_RENDERBUFFER, rboID));
        
#if defined(__DAVAENGINE_ANDROID__)
        if (RenderManager::Instance()->GetCaps().isGlDepth24Stencil8Supported)
#endif
        {
            RENDER_VERIFY(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height));
        }
#if defined(__DAVAENGINE_ANDROID__)
        else
        {
            if (RenderManager::Instance()->GetCaps().isGlDepthNvNonLinearSupported)
            {
                RENDER_VERIFY(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16_NONLINEAR_NV, width, height));
            }
            else
            {
                RENDER_VERIFY(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height));
            }

            if (!RenderManager::Instance()->GetCaps().isGlDepth24Stencil8Supported)
            {
                glGenRenderbuffers(1, &stencilRboID);
                glBindRenderbuffer(GL_RENDERBUFFER, stencilRboID);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
            }
        }
#endif
	}

	RENDER_VERIFY(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0));

	if(DEPTH_RENDERBUFFER == depthFormat)
	{
		RENDER_VERIFY(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboID));
#if defined(__DAVAENGINE_ANDROID__)
        if (RenderManager::Instance()->GetCaps().isGlDepth24Stencil8Supported)
#endif
        {
            RENDER_VERIFY(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboID));
        }
#if defined(__DAVAENGINE_ANDROID__)
        else
        {
            RENDER_VERIFY(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilRboID));
        }
#endif
	}

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
	{
		Logger::Error("[Texture::HWglCreateFBOBuffers] glCheckFramebufferStatus: %d", status);
	}

	RenderManager::Instance()->HWglBindFBO(saveFBO);

	if(saveTexture)
	{
		RenderManager::Instance()->HWglBindTexture(saveTexture, textureType);
	}

	state = STATE_VALID;
}

#endif //#if defined(__DAVAENGINE_OPENGL__)


	
void Texture::DumpTextures()
{
	uint32 allocSize = 0;
	int32 cnt = 0;
	Logger::FrameworkDebug("============================================================");
	Logger::FrameworkDebug("--------------- Currently allocated textures ---------------");

    textureMapMutex.Lock();
	for(TexturesMap::iterator it = textureMap.begin(); it != textureMap.end(); ++it)
	{
		Texture *t = it->second;
		Logger::FrameworkDebug("%s with id %d (%dx%d) retainCount: %d debug: %s format: %s", t->texDescriptor->pathname.GetAbsolutePathname().c_str(), t->id, t->width, t->height, 
								t->GetRetainCount(), t->debugInfo.c_str(), PixelFormatDescriptor::GetPixelFormatString(t->texDescriptor->format));
		cnt++;
        
        DVASSERT((0 <= t->texDescriptor->format) && (t->texDescriptor->format < FORMAT_COUNT));
        if(FORMAT_INVALID != t->texDescriptor->format)
        {
            allocSize += t->width * t->height * PixelFormatDescriptor::GetPixelFormatSizeInBits(t->texDescriptor->format);
        }
	}
    textureMapMutex.Unlock();

	Logger::FrameworkDebug("      Total allocated textures %d    memory size %d", cnt, allocSize/8);
	Logger::FrameworkDebug("============================================================");
}
	
void Texture::SetDebugInfo(const String & _debugInfo)
{
#if defined(__DAVAENGINE_DEBUG__)
	debugInfo = FastName(_debugInfo.c_str());
#endif
}
	
#if defined(__DAVAENGINE_ANDROID__)
	
void Texture::Lost()
{
	RenderResource::Lost();
    
    ReleaseTextureData();
}

void Texture::Invalidate()
{
	RenderResource::Invalidate();
	
	DVASSERT(id == 0 && "Texture always invalidated");
	if (id)
	{
		return;
	}
	
    const FilePath& relativePathname = texDescriptor->GetSourceTexturePathname();
	if (relativePathname.GetType() == FilePath::PATH_IN_FILESYSTEM ||
		relativePathname.GetType() == FilePath::PATH_IN_RESOURCES ||
		relativePathname.GetType() == FilePath::PATH_IN_DOCUMENTS)
	{
		Reload();
	}
	else if (relativePathname.GetType() == FilePath::PATH_IN_MEMORY)
	{
		if (invalidater)
			invalidater->InvalidateTexture(this);
	}
	else if (isPink)
	{
		MakePink();
	}
}
#endif //#if defined(__DAVAENGINE_ANDROID__)

Image * Texture::ReadDataToImage()
{
	const PixelFormatDescriptor & formatDescriptor = PixelFormatDescriptor::GetPixelFormatDescriptor(texDescriptor->format);

    Image *image = Image::Create(width, height, formatDescriptor.formatID);
    uint8 *imageData = image->GetData();
    
#if defined(__DAVAENGINE_OPENGL__)
    
    int32 saveFBO = RenderManager::Instance()->HWglGetLastFBO();
    int32 saveId = RenderManager::Instance()->HWglGetLastTextureID(textureType);

	RenderManager::Instance()->HWglBindTexture(id, textureType);
    
    if(FORMAT_INVALID != formatDescriptor.formatID)
    {
		RENDER_VERIFY(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
        RENDER_VERIFY(glReadPixels(0, 0, width, height, formatDescriptor.format, formatDescriptor.type, (GLvoid *)imageData));
    }

    RenderManager::Instance()->HWglBindFBO(saveFBO);
    RenderManager::Instance()->HWglBindTexture(saveId, textureType);
    
#endif //#if defined(__DAVAENGINE_OPENGL__)
    
    return image; 
}


Image * Texture::CreateImageFromMemory(UniqueHandle renderState)
{
    Image *image = NULL;
    if(isRenderTarget)
    {
        Sprite *renderTarget = Sprite::CreateFromTexture(this, 0, 0, (float32)width, (float32)height);
        RenderManager::Instance()->SetRenderTarget(renderTarget);
        
        image = ReadDataToImage();
            
        RenderManager::Instance()->RestoreRenderTarget();
        
        SafeRelease(renderTarget);
    }
    else
    {
        Sprite *renderTarget = Sprite::CreateAsRenderTarget((float32)width, (float32)height, texDescriptor->format);
        RenderManager::Instance()->SetRenderTarget(renderTarget);

        RenderManager::Instance()->ClearWithColor(0.f, 0.f, 0.f, 0.f);

		Sprite *drawTexture = Sprite::CreateFromTexture(this, 0, 0, (float32)width, (float32)height);

        Sprite::DrawState drawState;
        drawState.SetPosition(0, 0);
        drawState.SetRenderState(renderState);
        drawTexture->Draw(&drawState);

        RenderManager::Instance()->RestoreRenderTarget();
        
        image = renderTarget->GetTexture()->CreateImageFromMemory(renderState);

        SafeRelease(renderTarget);
        SafeRelease(drawTexture);
    }
        
    return image;
}
	
const TexturesMap & Texture::GetTextureMap()
{
    return textureMap;
}

uint32 Texture::GetDataSize() const
{
    DVASSERT((0 <= texDescriptor->format) && (texDescriptor->format < FORMAT_COUNT));
    
    uint32 allocSize = width * height * PixelFormatDescriptor::GetPixelFormatSizeInBits(texDescriptor->format) / 8;
    return allocSize;
}

Texture * Texture::CreatePink(TextureType requestedType, bool checkers)
{
	//we need instances for pink textures for ResourceEditor. We use it for reloading for different GPUs
	//pink textures at game is invalid situation
	Texture *tex = new Texture();
	if(Texture::TEXTURE_CUBE == requestedType)
	{
		tex->texDescriptor->Initialize(WRAP_CLAMP_TO_EDGE, true);
		tex->texDescriptor->dataSettings.faceDescription = 0x000000FF;
	}
	else
	{
		tex->texDescriptor->Initialize(WRAP_CLAMP_TO_EDGE, false);
	}

	tex->MakePink(checkers);

	return tex;
}

void Texture::MakePink(bool checkers)
{
    Vector<Image *> *images = new Vector<Image *> ();
	if(texDescriptor->IsCubeMap())
	{
		for(uint32 i = 0; i < Texture::CUBE_FACE_MAX_COUNT; ++i)
		{
            Image *img = Image::CreatePinkPlaceholder(checkers);
			img->cubeFaceID = i;
			img->mipmapLevel = 0;

			images->push_back(img);
		}
	}
	else
	{
		images->push_back(Image::CreatePinkPlaceholder(checkers));
	}

	SetParamsFromImages(images);
    FlushDataToRenderer(images);

    isPink = true;

	GeneratePixelesation();
}
    
bool Texture::IsPinkPlaceholder()
{
	return isPink;
}

void Texture::GenerateID()
{
#if defined(__DAVAENGINE_OPENGL__)
	RENDER_VERIFY(glGenTextures(1, &id));
	DVASSERT(id);
#endif //#if defined(__DAVAENGINE_OPENGL__)

}
    
void Texture::SetDefaultGPU(eGPUFamily gpuFamily)
{
    defaultGPU = gpuFamily;
}

eGPUFamily Texture::GetDefaultGPU()
{
    return defaultGPU;
}

    
eGPUFamily Texture::GetGPUForLoading(const eGPUFamily requestedGPU, const TextureDescriptor *descriptor)
{
    if(descriptor->IsCompressedFile())
        return (eGPUFamily)descriptor->exportedAsGpuFamily;
    
    return requestedGPU;
}

void Texture::SetInvalidater(TextureInvalidater* invalidater)
{
	this->invalidater = invalidater;
}

void Texture::GenerateCubeFaceNames(const FilePath & baseName, Vector<FilePath>& faceNames)
{
	static Vector<String> defaultSuffixes;
	if(defaultSuffixes.empty())
	{
		for(uint32 i = 0; i < Texture::CUBE_FACE_MAX_COUNT; ++i)
		{
			defaultSuffixes.push_back(FACE_NAME_SUFFIX[i]);
		}
	}
	
	GenerateCubeFaceNames(baseName, defaultSuffixes, faceNames);
}

void Texture::GenerateCubeFaceNames(const FilePath & filePath, const Vector<String>& faceNameSuffixes, Vector<FilePath>& faceNames)
{
	faceNames.clear();
	
	String fileNameWithoutExtension = filePath.GetBasename();
	String extension = filePath.GetExtension();
		
	for(size_t i = 0; i < faceNameSuffixes.size(); ++i)
	{
		DAVA::FilePath faceFilePath = filePath;
		faceFilePath.ReplaceFilename(fileNameWithoutExtension +
									 faceNameSuffixes[i] +
									 GPUFamilyDescriptor::GetFilenamePostfix(GPU_UNKNOWN, FORMAT_INVALID));
			
		faceNames.push_back(faceFilePath);
	}
}

const FilePath & Texture::GetPathname() const
{
    return texDescriptor->pathname;
}

PixelFormat Texture::GetFormat() const
{
	return texDescriptor->format;
}

void Texture::SetPixelization(bool value)
{
    if (value == pixelizationFlag)
    {
        return;
    }

    pixelizationFlag = value;
    const TexturesMap& texturesMap = GetTextureMap();

    textureMapMutex.Lock();
    for (Map<FilePath, Texture *>::const_iterator iter = texturesMap.begin(); iter != texturesMap.end(); iter ++)
    {
        Texture* texture = iter->second;
        TextureFilter minFilter = pixelizationFlag ? FILTER_NEAREST : (TextureFilter)texture->GetDescriptor()->drawSettings.minFilter;
        TextureFilter magFilter = pixelizationFlag ? FILTER_NEAREST : (TextureFilter)texture->GetDescriptor()->drawSettings.magFilter;
        texture->SetMinMagFilter(minFilter, magFilter);
    }
    textureMapMutex.Unlock();
}


int32 Texture::GetBaseMipMap() const
{
    if(texDescriptor->GetQualityGroup().IsValid())
    {
        const TextureQuality *curTxQuality = QualitySettingsSystem::Instance()->GetTxQuality(QualitySettingsSystem::Instance()->GetCurTextureQuality());
        if(NULL != curTxQuality)
        {
            return curTxQuality->albedoBaseMipMapLevel;
        }
    }

    return 0;
}

};
