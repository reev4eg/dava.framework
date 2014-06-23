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



#include "Render/Image/Image.h"
#include "Render/Image/LibJpegHelper.h"
#include "Render/Image/ImageConvert.h"

#include "Render/Texture.h"
#include "Render/RenderManager.h"

#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"

#include <stdlib.h>
#include <stdio.h>

#include "libjpeg/jpeglib.h"
#include <setjmp.h>

#define QUALITY 100 //0..100

namespace DAVA
{
//
struct jpegErrorManager
{
    /* "public" fields */
    struct jpeg_error_mgr pub;
    /* for return to caller */
    jmp_buf setjmp_buffer;
};
    
char jpegLastErrorMsg[JMSG_LENGTH_MAX];
    
void jpegErrorExit (j_common_ptr cinfo)
{
    /* cinfo->err actually points to a jpegErrorManager struct */
    jpegErrorManager* myerr = (jpegErrorManager*) cinfo->err;
    /* note : *(cinfo->err) is now equivalent to myerr->pub */
    
    /* output_message is a method to print an error message */
    /*(* (cinfo->err->output_message) ) (cinfo);*/
    
    /* Create the message */
    ( *(cinfo->err->format_message) ) (cinfo, jpegLastErrorMsg);
    
    /* Jump to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}
    
LibJpegWrapper::LibJpegWrapper()
{
    supportedExtensions.push_back(".jpeg");
    supportedExtensions.push_back(".jpg");
}
    
bool LibJpegWrapper::IsImage(File *infile) const
{
    return GetDataSize(infile) != 0;
}
    
uint32 LibJpegWrapper::GetDataSize(File *infile) const
{
    jpeg_decompress_struct cinfo;
    jpegErrorManager jerr;

    infile->Seek(0, File::SEEK_FROM_START);
	uint32 fileSize = infile->GetSize();
	uint8* fileBuffer= new uint8[fileSize];
	infile->Read(fileBuffer, fileSize);
    cinfo.err = jpeg_std_error( &jerr.pub );
    
    jerr.pub.error_exit = jpegErrorExit;
    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        SafeDeleteArray(fileBuffer);
        infile->Seek(0,  File::SEEK_FROM_START);
        Logger::Error("[LibJpegWrapper::GetDataSize] File %s has wrong jpeg header", infile->GetFilename().GetAbsolutePathname().c_str());
        return 0;
    }
    jpeg_create_decompress( &cinfo );
    jpeg_mem_src( &cinfo, fileBuffer,fileSize);
    jpeg_read_header( &cinfo, true );
    infile->Seek(0,  File::SEEK_FROM_START);
    uint32 dataSize = cinfo.src->bytes_in_buffer;
    jpeg_destroy_decompress( &cinfo );
    SafeDeleteArray(fileBuffer);
    return dataSize;
}
  
eErrorCode LibJpegWrapper::ReadFile(File *infile, Vector<Image *> &imageSet, int32 baseMipMap) const
{
    jpeg_decompress_struct cinfo;
    jpegErrorManager jerr;

    infile->Seek(0, File::SEEK_FROM_START);
	uint32 fileSize = infile->GetSize();
	uint8* fileBuffer= new uint8[fileSize];
	infile->Read(fileBuffer, fileSize);
	infile->Seek(0, File::SEEK_FROM_START);
    
    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = jpegErrorExit;
    
    Image* image = new Image();
    
    //set error handling block, which will be called in case of fail of jpeg_start_decompress,jpeg_read_scanlines...
    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        SafeDeleteArray(fileBuffer);
        SafeRelease(image);
        Logger::Error("[LibJpegWrapper::ReadFile] File %s has wrong jpeg header", infile->GetFilename() .GetAbsolutePathname().c_str());
        return ERROR_FILE_FORMAT_INCORRECT;
    }
    
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, fileBuffer, fileSize);
    jpeg_read_header( &cinfo, TRUE );
    jpeg_start_decompress(&cinfo);
    
    image->width = cinfo.image_width;
    image->height = cinfo.image_height;
    //as image->data will be rewrited, need to erase present buffer
    SafeDeleteArray(image->data);
    image->data = new uint8 [cinfo.output_width * cinfo.output_height * cinfo.num_components];
    
    JSAMPROW output_data;
    unsigned int scanline_len = cinfo.output_width * cinfo.output_components;
    
    unsigned int scanline_count = 0;
    while (cinfo.output_scanline < cinfo.output_height)
    {
        output_data = (image->data + (scanline_count * scanline_len));
        jpeg_read_scanlines(&cinfo, &output_data, 1);
        scanline_count++;
    }
    
    image->format = FORMAT_RGB888;
    if(cinfo.jpeg_color_space == JCS_GRAYSCALE)
    {
        image->format = FORMAT_A8;
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
	SafeDeleteArray(fileBuffer);
    imageSet.push_back(image);
    return SUCCESS;
}

eErrorCode LibJpegWrapper::WriteFileAsCubeMap(const FilePath & fileName, const Vector<Image *> &imageSet, PixelFormat compressionFormat) const
{
    Logger::Error("[LibJpegWrapper::WriteFileAsCubeMap] For jpeg cubeMaps are not supported");
    return ERROR_WRITE_FAIL;
}
    
eErrorCode LibJpegWrapper::WriteFile(const FilePath & fileName, const Vector<Image *> &imageSet, PixelFormat compressionFormat) const
{
    DVASSERT(imageSet.size());
    const Image* original = imageSet[0];
    int32 width = original->width;
    int32 height = original->height;
    uint8* imageData = original->data;
    PixelFormat format = original->format;
    Image* convertedImage = NULL;
    if(!(FORMAT_RGB888 == original->format || FORMAT_A8 == original->format))
    {
        if(FORMAT_RGBA8888 == original->format)
        {
            convertedImage = Image::Create(width, height, FORMAT_RGB888);
            ConvertDirect<uint32, RGB888, ConvertRGBA8888toRGB888> convert;
            convert(imageData, width, height, sizeof(uint32)*width, convertedImage->data, width, height, sizeof(RGB888)*width);
        }
        else if(FORMAT_A16 == original->format)
        {
            convertedImage = Image::Create(width, height, FORMAT_A8);
            ConvertDirect<uint16, uint8, ConvertA16toA8> convert;
            convert(imageData, width, height, sizeof(uint16)*width, convertedImage->data, width, height, sizeof(uint8)*width);
        }
        DVASSERT(convertedImage);
        imageData = convertedImage->data;
        format = convertedImage->format;
    }
    
    jpeg_compress_struct cinfo;
    jpegErrorManager jerr;
    
    JSAMPROW row_pointer[1];
	FILE *outfile = fopen(fileName.GetAbsolutePathname().c_str(), "wb");
    
    if ( !outfile )
    {
		Logger::Error("[LibJpegWrapper::WriteJpegFile] File %s could not be opened for writing", fileName.GetAbsolutePathname().c_str());
        SafeRelease(convertedImage);
        return ERROR_FILE_NOTFOUND;
    }
    cinfo.err = jpeg_std_error( &jerr.pub );
    
    jerr.pub.error_exit = jpegErrorExit;
    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_compress( &cinfo );
        fclose(outfile);
		Logger::Error("[LibJpegWrapper::WriteJpegFile] Error during compression of jpeg into file %s.", fileName.GetAbsolutePathname().c_str());
        SafeRelease(convertedImage);
        return ERROR_WRITE_FAIL;
    }
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);
    
    // Setting the parameters of the output file here
    cinfo.image_width = width;
    cinfo.image_height = height;
    
    cinfo.in_color_space = JCS_RGB;
    int colorComponents = 3;
    if(format == FORMAT_A8)
    {
        cinfo.in_color_space = JCS_GRAYSCALE;
        colorComponents = 1;
    }
    
    cinfo.input_components = colorComponents;
    
    jpeg_set_defaults( &cinfo );
    cinfo.num_components = colorComponents;
    //cinfo.data_precision = 4;
    cinfo.dct_method = JDCT_FLOAT;
    
    //The quality value ranges from 0..100. If "force_baseline" is TRUE, the computed quantization table entries are limited to 1..255 for JPEG baseline compatibility.
    jpeg_set_quality(&cinfo, QUALITY, TRUE);

    jpeg_start_compress( &cinfo, TRUE );
    while( cinfo.next_scanline < cinfo.image_height )
    {
        row_pointer[0] = &imageData[ cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
    }
    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );
    fclose( outfile );
    SafeRelease(convertedImage);
    return SUCCESS;
}
   
    
};