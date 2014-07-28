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



#include "DAVAEngine.h"
#include "GameCore.h"
#include "TexturePacker/CommandLineParser.h"

#include "Render/ImageConvert.h"

using namespace DAVA;
 
void PrintUsage()
{
    printf("Usage:\n");

    printf("\t-usage or --help to display this help\n");
	printf("\t-file - pvr or dds file to unpack as png");
	printf("\t-folder - folder with pvr or dds files to unpack as png");
}


bool CheckPosition(int32 commandPosition)
{
    if(CommandLineParser::CheckPosition(commandPosition))
    {
        printf("Wrong arguments\n");
        PrintUsage();

        return false;
    }
    
    return true;
}

void UnpackFile(const FilePath & sourceImagePath)
{
    Vector<Image *> images;
    ImageLoader::CreateFromFileByExtension(sourceImagePath, images, 0);
    
    if(images.size() != 0)
    {
        Image *image = images[0];
        if((FORMAT_RGBA8888 == image->format) || (FORMAT_A8 == image->format) || (FORMAT_A16 == image->format))
        {
            ImageLoader::Save(image, FilePath::CreateWithNewExtension(sourceImagePath,".png"));
        }
        else
        {
			Image *savedImage = Image::Create(image->width, image->height, FORMAT_RGBA8888);

			ImageConvert::ConvertImageDirect(image->format, savedImage->format, image->data, image->width, image->height, image->width * PixelFormatDescriptor::GetPixelFormatSizeInBytes(image->format), 
					savedImage->data, savedImage->width, savedImage->height, savedImage->width * PixelFormatDescriptor::GetPixelFormatSizeInBytes(savedImage->format));

			ImageLoader::Save(savedImage, FilePath::CreateWithNewExtension(sourceImagePath,".png"));
			savedImage->Release();
        }
        
        for_each(images.begin(), images.end(), SafeRelease<Image>);
    }
    else
    {
        Logger::Error("Cannot load file: ", sourceImagePath.GetAbsolutePathname().c_str());
    }
}

void UnpackFolder(const FilePath & folderPath)
{
    FileList * fileList = new FileList(folderPath);
	for (int fi = 0; fi < fileList->GetCount(); ++fi)
	{
        const FilePath & pathname = fileList->GetPathname(fi);
		if (fileList->IsDirectory(fi) && !fileList->IsNavigationDirectory(fi))
		{
            UnpackFolder(pathname);
        }
        else
        {
            if(pathname.IsEqualToExtension(".pvr") || pathname.IsEqualToExtension(".dds"))
            {
                UnpackFile(pathname);
            }
        }
	}
    
    fileList->Release();
}


void ProcessImageUnpacker()
{
    RenderManager::Create(Core::RENDERER_OPENGL);
    PixelFormatDescriptor::InitializePixelFormatDescriptors();
    
    FilePath sourceFolderPath = CommandLineParser::GetCommandParam(String("-folder"));
    FilePath sourceFilePath = CommandLineParser::GetCommandParam(String("-file"));
    
    if(sourceFolderPath.IsEmpty() == false)
    {
        sourceFolderPath.MakeDirectoryPathname();
        UnpackFolder(sourceFolderPath);
    }
    else if (sourceFilePath.IsEmpty() == false)
    {
        UnpackFile(sourceFilePath);
    }
    else
    {
        PrintUsage();
    }
    
    RenderManager::Instance()->Release();
}

void FrameworkDidLaunched()
{
    Logger::Instance()->SetLogLevel(Logger::LEVEL_INFO);
	if (Core::Instance()->IsConsoleMode())
	{
        if(     CommandLineParser::GetCommandsCount() < 2
           ||   (CommandLineParser::CommandIsFound(String("-usage")))
           ||   (CommandLineParser::CommandIsFound(String("-help")))
           )
        {
            PrintUsage();
			return;
        }
	}

    ProcessImageUnpacker();
}


void FrameworkWillTerminate()
{
}
