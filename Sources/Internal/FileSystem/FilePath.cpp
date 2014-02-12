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


#include "FileSystem/FilePath.h"
#include "FileSystem/FileSystem.h"
#include "Utils/Utils.h"
#include "Utils/StringFormat.h"

#if defined(__DAVAENGINE_ANDROID__)
    #ifdef USE_LOCAL_RESOURCES
        #define USE_LOCAL_RESOURCES_PATH "/mnt/sdcard/DavaProject/"
    #endif //USE_LOCAL_RESOURCES
#endif //__DAVAENGINE_ANDROID__



namespace DAVA
{

List<FilePath> FilePath::resourceFolders;

void FilePath::SetBundleName(const FilePath & newBundlePath)
{
	FilePath virtualBundlePath = newBundlePath;

	if(!virtualBundlePath.IsEmpty())
	{
		virtualBundlePath.MakeDirectoryPathname();
	}

    virtualBundlePath.pathType = PATH_IN_RESOURCES;

    if(resourceFolders.size())
        resourceFolders.pop_front();
    
    resourceFolders.push_front(virtualBundlePath);
}

const FilePath & FilePath::GetBundleName()
{
    DVASSERT(resourceFolders.size());
	return resourceFolders.front();
}
    
void FilePath::AddResourcesFolder(const FilePath & folder)
{
	DVASSERT(!folder.IsEmpty());

    for(List<FilePath>::iterator it = resourceFolders.begin(); it != resourceFolders.end(); ++it)
    {
        if(folder == *it)
        {
            DVASSERT(false);
        }
    }
    
    FilePath resPath = folder;
    resPath.pathType = PATH_IN_RESOURCES;
    resourceFolders.push_back(resPath);
}

void FilePath::AddTopResourcesFolder(const FilePath & folder)
{
	DVASSERT(!folder.IsEmpty());

	for(List<FilePath>::iterator it = resourceFolders.begin(); it != resourceFolders.end(); ++it)
	{
		if(folder == *it)
		{
			DVASSERT(false);
		}
	}

	FilePath resPath = folder;
	resPath.pathType = PATH_IN_RESOURCES;
	resourceFolders.push_front(resPath);
}
    
void FilePath::RemoveResourcesFolder(const FilePath & folder)
{
    for(List<FilePath>::iterator it = resourceFolders.begin(); it != resourceFolders.end(); ++it)
    {
        if(folder == *it)
        {
            resourceFolders.erase(it);
            return;
        }
    }
    
    DVASSERT(false);
}
    
const List<FilePath> FilePath::GetResourcesFolders()
{
    return resourceFolders;
}

    
#if defined(__DAVAENGINE_WIN32__)
void FilePath::InitializeBundleName()
{
	FilePath execDirectory = FileSystem::Instance()->GetCurrentExecutableDirectory();
	FilePath workingDirectory = FileSystem::Instance()->GetCurrentWorkingDirectory();
	SetBundleName(execDirectory);
	if(workingDirectory != execDirectory)
	{
		AddResourcesFolder(workingDirectory);
	}
}
#endif //#if defined(__DAVAENGINE_WIN32__)


#if defined(__DAVAENGINE_ANDROID__)
void FilePath::InitializeBundleName()
{
#ifdef USE_LOCAL_RESOURCES
    SetBundleName(FilePath(USE_LOCAL_RESOURCES_PATH));
#else
    SetBundleName(FilePath());
#endif
}

#endif //#if defined(__DAVAENGINE_ANDROID__)

FilePath FilePath::FilepathInDocuments(const char * relativePathname)
{
	FilePath path(FileSystem::Instance()->GetCurrentDocumentsDirectory() + relativePathname);
	path.pathType = PATH_IN_DOCUMENTS;
    return path;
}

FilePath FilePath::FilepathInDocuments(const String & relativePathname)
{
    return FilepathInDocuments(relativePathname.c_str());
}

bool FilePath::ContainPath(const FilePath& basePath, const FilePath& partPath)
{
	return basePath.GetAbsolutePathname().find(partPath.GetAbsolutePathname()) != std::string::npos;
}

bool ContainPath(const FilePath& basePath, const String & partPath)
{
	return basePath.GetAbsolutePathname().find(partPath) != std::string::npos;
}

bool ContainPath(const FilePath& basePath, const char * partPath)
{
	return ContainPath(basePath, String(partPath));
}


bool operator < (const FilePath& left, const FilePath& right)
{
	return left.Compare(right) < 0;
}



FilePath::FilePath()
{
    pathType = PATH_EMPTY;
    absolutePathname = String();
}

FilePath::FilePath(const FilePath &path)
{
    pathType = path.pathType;
    absolutePathname = path.absolutePathname;
}
    
FilePath::FilePath(const char * sourcePath)
{
    Initialize(String(sourcePath));
}

FilePath::FilePath(const String &pathname)
{
    Initialize(pathname);
}

    
FilePath::FilePath(const char * directory, const String &filename)
{
    FilePath directoryPath(directory);
    DVASSERT(!directoryPath.IsEmpty());
    
    directoryPath.MakeDirectoryPathname();
    
    pathType = directoryPath.pathType;
    absolutePathname = AddPath(directoryPath, filename);
}

FilePath::FilePath(const String &directory, const String &filename)
{
    FilePath directoryPath(directory);
    DVASSERT(!directoryPath.IsEmpty());
    directoryPath.MakeDirectoryPathname();
    
    pathType = directoryPath.pathType;
    absolutePathname = AddPath(directoryPath, filename);
}

FilePath::FilePath(const FilePath &directory, const String &filename)
{
	DVASSERT(directory.IsDirectoryPathname());

    pathType = directory.pathType;
	absolutePathname = AddPath(directory, filename);
}


void FilePath::Initialize(const String &_pathname)
{
	String pathname = NormalizePathname(_pathname);
    pathType = GetPathType(pathname);

	if (pathType == PATH_EMPTY)
	{
		absolutePathname = String();
	}
    else if(pathType == PATH_IN_RESOURCES || pathType == PATH_IN_MEMORY)
    {
        absolutePathname = pathname;
    }
    else if(pathType == PATH_IN_DOCUMENTS)
    {
        absolutePathname = GetSystemPathname(pathname, pathType);
    }
    else if(IsAbsolutePathname(pathname))
    {
        absolutePathname = pathname;
    }
    else
    {
        Logger::FrameworkDebug("[FilePath::Initialize] FilePath was initialized from relative path name (%s)", _pathname.c_str());
        
#if defined(__DAVAENGINE_ANDROID__)
        absolutePathname = pathname;
#else //#if defined(__DAVAENGINE_ANDROID__)
        FilePath path = FileSystem::Instance()->GetCurrentWorkingDirectory() + pathname;
        absolutePathname = path.GetAbsolutePathname();
#endif //#if defined(__DAVAENGINE_ANDROID__)
    }
}

    
    
FilePath::~FilePath()
{
    
}
    
const String FilePath::GetAbsolutePathname() const
{
    if(pathType == PATH_IN_RESOURCES)
    {
        return ResolveResourcesPath();
    }
    
    return absolutePathname;
}

String FilePath::ResolveResourcesPath() const
{
    String::size_type find = absolutePathname.find("~res:");
    if(find != String::npos)
    {
        String relativePathname = "Data" + absolutePathname.substr(5);
        FilePath path;

        if(resourceFolders.size() == 1) // optimization to avoid call path.Exists()
        {
            path = (*resourceFolders.begin()).absolutePathname + relativePathname;
        }
        else
        {
            List<FilePath>::reverse_iterator endIt = resourceFolders.rend();
            for(List<FilePath>::reverse_iterator it = resourceFolders.rbegin(); it != endIt; ++it)
            {
                path = (*it).absolutePathname + relativePathname;
                if(path.Exists())
                {
                    break;
                }
				else
				{
					path = "";
				}
            }
        }
        
        return path.absolutePathname;
    }
    
    return absolutePathname;
}


FilePath& FilePath::operator=(const FilePath &path)
{
    this->absolutePathname = path.absolutePathname;
    this->pathType = path.pathType;
    
    return *this;
}
    
FilePath FilePath::operator+(const String &path) const
{
    FilePath pathname(AddPath(*this, path));

    pathname.pathType = this->pathType;
	if (this->pathType == PATH_EMPTY)
	{
		pathname.pathType = GetPathType(pathname.absolutePathname);
	}

    return pathname;
}

FilePath& FilePath::operator+=(const String & path)
{
    if(pathType == PATH_EMPTY)
    {
        Initialize(path);
    }
    else
    {
        absolutePathname = AddPath(*this, path);
    }

    return (*this);
}
    
bool FilePath::operator==(const FilePath &path) const
{
    return absolutePathname == path.absolutePathname;
}

bool FilePath::operator!=(const FilePath &path) const
{
    return absolutePathname != path.absolutePathname;
}

    
bool FilePath::IsDirectoryPathname() const
{
    if(IsEmpty())
    {
        return false;
    }

    const int32 lastPosition = absolutePathname.length() - 1;
    return (absolutePathname.at(lastPosition) == '/');
}


String FilePath::GetFilename() const
{
    return GetFilename(absolutePathname);
}
    
String FilePath::GetFilename(const String &pathname)
{
    String::size_type dotpos = pathname.rfind(String("/"));
    if (dotpos == String::npos)
        return pathname;
    
    return pathname.substr(dotpos+1);
}



String FilePath::GetBasename() const
{
    const String filename = GetFilename();
    
    const String::size_type dotpos = filename.rfind(String("."));
	if (dotpos == String::npos)
		return filename;
    
	return filename.substr(0, dotpos);
}

String FilePath::GetExtension() const
{
    const String filename = GetFilename();
    
    const String::size_type dotpos = filename.rfind(String("."));
	if (dotpos == String::npos)
        return String();
    
    return filename.substr(dotpos);
}

    
FilePath FilePath::GetDirectory() const
{
    FilePath directory;
    
    const String::size_type slashpos = absolutePathname.rfind(String("/"));
    if (slashpos != String::npos)
    {
        directory = absolutePathname.substr(0, slashpos + 1);
    }
    
    directory.pathType = pathType;
    return directory;
}

    
String FilePath::GetRelativePathname() const
{
    return GetRelativePathname(FileSystem::Instance()->GetCurrentWorkingDirectory());
}
    
String FilePath::GetRelativePathname(const FilePath &forDirectory) const
{
    if(forDirectory.IsEmpty())
        return GetAbsolutePathname();
    
    DVASSERT(forDirectory.IsDirectoryPathname());
    
    return AbsoluteToRelative(forDirectory, *this);
}

String FilePath::GetRelativePathname(const String &forDirectory) const
{
    if(forDirectory.empty())
        return String();
    
	return GetRelativePathname(FilePath(forDirectory));
}
    
String FilePath::GetRelativePathname(const char * forDirectory) const
{
    if(forDirectory == NULL)
        return String();
    
	return GetRelativePathname(FilePath(forDirectory));
}

    
    
void FilePath::ReplaceFilename(const String &filename)
{
    DVASSERT(!IsEmpty());
    
    absolutePathname = (GetDirectory() + filename).absolutePathname;
}
    
void FilePath::ReplaceBasename(const String &basename)
{
    if(!IsEmpty())
    {
        const String extension = GetExtension();
        absolutePathname = (GetDirectory() + (basename + extension)).absolutePathname;
    }
}
    
void FilePath::ReplaceExtension(const String &extension)
{
    if(!IsEmpty())
    {
        const String basename = GetBasename();
        absolutePathname = (GetDirectory() + (basename + extension)).absolutePathname;
    }
}
    
void FilePath::ReplaceDirectory(const String &directory)
{
    DVASSERT(!IsEmpty());
    
    const String filename = GetFilename();
    Initialize((MakeDirectory(directory) + filename));
}
    
void FilePath::ReplaceDirectory(const FilePath &directory)
{
    DVASSERT(!IsEmpty());
    
    DVASSERT(directory.IsDirectoryPathname());
    const String filename = GetFilename();

    absolutePathname = (directory + filename).absolutePathname;
    pathType = directory.pathType;
}
    
FilePath & FilePath::MakeDirectoryPathname()
{
    DVASSERT(!IsEmpty());
    
    absolutePathname = MakeDirectory(absolutePathname);
    
    return *this;
}
    
void FilePath::TruncateExtension()
{
    DVASSERT(!IsEmpty());
    
    ReplaceExtension(String(""));
}
    
String FilePath::GetLastDirectoryName() const
{
    DVASSERT(!IsEmpty() && IsDirectoryPathname());
    
    String path = absolutePathname;
    path = path.substr(0, path.length() - 1);
    
    return FilePath(path).GetFilename();
}
    
bool FilePath::IsEqualToExtension( const String & extension ) const
{
	String selfExtension = GetExtension();
	return (CompareCaseInsensitive(extension, selfExtension) == 0);
}

    
FilePath FilePath::CreateWithNewExtension(const FilePath &pathname, const String &extension)
{
    FilePath path(pathname);
    path.ReplaceExtension(extension);
    return path;
}

    
String FilePath::GetSystemPathname(const String &pathname, const ePathType pType)
{
    if(pType == PATH_IN_FILESYSTEM || pType == PATH_IN_MEMORY)
        return pathname;
    
    String retPath = pathname;
	if(pType == PATH_IN_RESOURCES)
	{
		retPath = FilePath(retPath).GetAbsolutePathname();
	}
	else if(pType == PATH_IN_DOCUMENTS)
	{
        retPath = retPath.erase(0, 5);
        retPath = FilepathInDocuments(retPath).GetAbsolutePathname();
	}
    
    return NormalizePathname(retPath);
}
    

String FilePath::GetFrameworkPath() const
{
    if(PATH_IN_RESOURCES == pathType)
        return absolutePathname;
    
    String pathInRes = GetFrameworkPathForPrefix("~res:/", PATH_IN_RESOURCES);
    if(!pathInRes.empty())
    {
        return pathInRes;
    }
    
    String pathInDoc = GetFrameworkPathForPrefix("~doc:/", PATH_IN_DOCUMENTS);
    if(!pathInDoc.empty())
    {
        return pathInDoc;
    }
    
	DVASSERT(false);

	return String();
}


String FilePath::GetFrameworkPathForPrefix( const String &typePrefix, const ePathType pType) const
{
    DVASSERT(!typePrefix.empty());
    
	String prefixPathname = GetSystemPathname(typePrefix, pType);

	String::size_type pos = absolutePathname.find(prefixPathname);
	if(pos == 0)
	{
		String pathname = absolutePathname;
		pathname = pathname.replace(pos, prefixPathname.length(), typePrefix);
		return pathname;
	}

	return String();
}


String FilePath::NormalizePathname(const String &pathname)
{
	if(pathname.empty())
		return String();
	
	String path = pathname;
    std::replace(path.begin(), path.end(),'\\','/');
    
    Vector<String> tokens;
    Split(path, "/", tokens);
    
    //TODO: correctly process situation ../../folders/filename
    for (int32 i = 0; i < (int32)tokens.size(); ++i)
    {
        if (String(".") == tokens[i])
        {
            for (int32 k = i + 1; k < (int32)tokens.size(); ++k)
            {
                tokens[k - 1] = tokens[k];
            }
            --i;
            tokens.pop_back();
        }
        else if ((1 <= i) && (String("..") == tokens[i] && String("..") != tokens[i-1]))
        {
            for (int32 k = i + 1; k < (int32)tokens.size(); ++k)
            {
                tokens[k - 2] = tokens[k];
            }
            i-=2;
            tokens.pop_back();
            tokens.pop_back();
        }
    }
    
    String result = "";
    if('/' == path[0])
		result = "/";
    
    for (int32 k = 0; k < (int32)tokens.size(); ++k)
    {
        result += tokens[k];
        if (k + 1 != (int32)tokens.size())
            result += String("/");
    }
    
	//process last /
	if(('/' == path[path.length() - 1]) && (path.length() != 1))
		result += String("/");
    
    return result;
}

String FilePath::MakeDirectory(const String &pathname)
{
    if(pathname.empty())
    {
        return String();
    }
    
    const int32 lastPosition = pathname.length() - 1;
    if(pathname.at(lastPosition) != '/')
    {
        return pathname + String("/");
    }
    
    return pathname;
}
    
String FilePath::AbsoluteToRelative(const FilePath &directoryPathname, const FilePath &absolutePathname)
{
    if(absolutePathname.IsEmpty())
        return String();

    DVASSERT(directoryPathname.IsDirectoryPathname());

    Vector<String> folders;
	Vector<String> fileFolders;

	if(directoryPathname.GetType() == PATH_IN_RESOURCES && absolutePathname.GetType() == PATH_IN_RESOURCES)
	{
		Split(directoryPathname.absolutePathname, "/", folders);
		Split(absolutePathname.GetDirectory().absolutePathname, "/", fileFolders);
	}
	else
	{
		Split(directoryPathname.GetAbsolutePathname(), "/", folders);
		Split(absolutePathname.GetDirectory().GetAbsolutePathname(), "/", fileFolders);
	}
    
    Vector<String>::size_type equalCount = 0;
    for(; equalCount < folders.size() && equalCount < fileFolders.size(); ++equalCount)
    {
        if(folders[equalCount] != fileFolders[equalCount])
        {
            break;
        }
    }
    
    String retPath = "";
    for(Vector<String>::size_type i = equalCount; i < folders.size(); ++i)
    {
        retPath += "../";
    }
    
    for(Vector<String>::size_type i = equalCount; i < fileFolders.size(); ++i)
    {
        retPath += fileFolders[i] + "/";
    }
    
    return (retPath + absolutePathname.GetFilename());
}
    
    
bool FilePath::IsAbsolutePathname(const String &pathname)
{
    if(pathname.empty())
        return false;
    
    //Unix style
    if(pathname[0] == '/')
        return true;
    
    //Win or DAVA style (c:/, ~res:/, ~doc:/)
    String::size_type winFound = pathname.find(":");
    if(winFound != String::npos)
    {
        return true;
    }
    
    return false;
}
    
String FilePath::AddPath(const FilePath &folder, const String & addition)
{
    if(folder.IsEmpty()) return NormalizePathname(addition);
    
    String absPathname = folder.absolutePathname + addition;
    if(folder.pathType == PATH_IN_RESOURCES && absPathname.find("~res:") == 0)
    {
        const String frameworkPath = GetSystemPathname("~res:/", PATH_IN_RESOURCES) + "Data";
        absPathname = NormalizePathname(frameworkPath + absPathname.substr(5));
        
        if(absPathname.find(frameworkPath) == 0)
        {
            absPathname.replace(0, frameworkPath.length(), "~res:");
        }

        return absPathname;
    }
    
    return NormalizePathname(absPathname);
}

FilePath::ePathType FilePath::GetPathType(const String &pathname)
{
	if (pathname.empty())
	{
		return PATH_EMPTY;
	}

    String::size_type find = pathname.find("~res:");
    if(find == 0)
    {
        return PATH_IN_RESOURCES;
    }

    find = pathname.find("~doc:");
    if(find == 0)
    {
        return PATH_IN_DOCUMENTS;
    }
    
    if(    (pathname.find("FBO ") == 0)
       ||  (pathname.find("memoryfile_") == 0)
       ||  (pathname.find("Text ") == 0))
    {
        return PATH_IN_MEMORY;
    }
    
    return PATH_IN_FILESYSTEM;
}

    
bool FilePath::Exists() const
{
    if(pathType == PATH_IN_MEMORY || pathType == PATH_EMPTY)
    {
        return false;
    }
    
    if(IsDirectoryPathname())
    {
        return FileSystem::Instance()->IsDirectory(*this);
    }

    return FileSystem::Instance()->IsFile(*this);
}

int32 FilePath::Compare( const FilePath &right ) const
{
	if(absolutePathname < right.absolutePathname) return -1;
	if(absolutePathname > right.absolutePathname) return 1;

	return 0;
}

    
}
