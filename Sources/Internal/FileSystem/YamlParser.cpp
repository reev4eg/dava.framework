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


#include "FileSystem/YamlParser.h"
#include "FileSystem/Logger.h"
#include "Utils/Utils.h"
#include <sstream>
#include "yaml/yaml.h"
#include "Utils/UTF8Utils.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/VariantType.h"
#include "FileSystem/KeyedArchive.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlEmitter.h"

namespace DAVA 
{

// Inner clsee used for sorting.
class YamlNodeKeyValuePair
{
public:
    YamlNodeKeyValuePair(const String& nodeName, YamlNode* value)
    {
        this->yamlNodeName = nodeName;
        this->yamlNodeValue = value;
    }

    String GetNodeName() const  {return this->yamlNodeName;};
    YamlNode* GetNodeValue() const {return this->yamlNodeValue;};

    bool operator < (const YamlNodeKeyValuePair& right) const
    {
        const YamlNode* leftIndexNode = this->yamlNodeValue->Get(YamlNode::SAVE_INDEX_NAME);
        const YamlNode* rightIndexNode = right.yamlNodeValue->Get(YamlNode::SAVE_INDEX_NAME);

        if (!leftIndexNode || !rightIndexNode)
        {
            return false;
        }

        return leftIndexNode->AsInt() < rightIndexNode->AsInt();
    }

private:
    String yamlNodeName;
    YamlNode* yamlNodeValue;
};

YamlParser * YamlParser::Create()
{
    return new YamlParser();
}
    

bool YamlParser::Parse(const String& data)
{
    YamlDataHolder dataHolder;
    dataHolder.fileSize = data.size();
    dataHolder.data = (uint8 *)data.c_str();
    dataHolder.dataOffset = 0;    

    return Parse(&dataHolder);
}

bool YamlParser::Parse(const FilePath & pathName)
{
    File * yamlFile = File::Create(pathName, File::OPEN | File::READ);
    if (!yamlFile)
    {
        Logger::Error("[YamlParser::Parse] Can't create file: %s", pathName.GetAbsolutePathname().c_str());
        return false;
    }

    YamlDataHolder dataHolder;
    dataHolder.fileSize = yamlFile->GetSize();
    dataHolder.data = new uint8[dataHolder.fileSize];
    dataHolder.dataOffset = 0;
    yamlFile->Read(dataHolder.data, dataHolder.fileSize);
    SafeRelease(yamlFile);

    bool result = Parse(&dataHolder);
    SafeDeleteArray(dataHolder.data);
    return result;
}

bool YamlParser::Parse(YamlDataHolder * dataHolder)
{
	yaml_parser_t parser;
	yaml_event_t event;
	
	int done = 0;
	
	/* Create the Parser object. */
	yaml_parser_initialize(&parser);
	
	yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
	
	/* Set a string input. */
	//yaml_parser_set_input_string(&parser, (const unsigned char*)pathName.c_str(), pathName.length());
		
	yaml_parser_set_input(&parser, read_handler, dataHolder);

	YamlNode * mapKey = 0;
//	YamlNode * mapValue = 0;

	/* Read the event sequence. */
	while (!done) 
	{
		
		/* Get the next event. */
		if (!yaml_parser_parse(&parser, &event))
		{
			Logger::Error("[YamlParser::Parse] error: type: %d %s line: %d pos: %d", parser.error, parser.problem, parser.problem_mark.line, parser.problem_mark.column);
			break;
		}
		
		/*if (event.encoding != YAML_UTF8_ENCODING)
		{
			Logger::FrameworkDebug("wrong encoding");
		}*/
		
		switch(event.type)
		{
		case YAML_ALIAS_EVENT:
			Logger::FrameworkDebug("alias: %s", event.data.alias.anchor);
			break;
		
		case YAML_SCALAR_EVENT:
			{
				YamlNode * node = new YamlNode(YamlNode::TYPE_STRING);

				/*CFStringRef s = CFStringCreateWithBytes(NULL, event.data.scalar.value, event.data.scalar.length, kCFStringEncodingUTF8, false);
				int32 length = CFStringGetLength(s); 
				node->stringValue.resize(length); 
				for (int i = 0; i < length; i++) 
				{
					UniChar uchar = CFStringGetCharacterAtIndex(s, i);
					node->stringValue[i] = (wchar_t)uchar;
				}
				CFRelease(s);
				node->nwStringValue = String((const char*)event.data.scalar.value); */
				node->nwStringValue = String((const char*)event.data.scalar.value);
				UTF8Utils::EncodeToWideString((uint8*)event.data.scalar.value, (int32)event.data.scalar.length, node->stringValue);
				
				
				if (objectStack.size() == 0)
				{
					rootObject = node;
				}else
				{
					YamlNode * topContainer = objectStack.top();
					if (topContainer->type == YamlNode::TYPE_MAP)
					{
						if (mapKey == 0)mapKey = node;
						else
						{
//							if (topContainer->Get(mapKey->nwStringValue))
//							{
//								Logger::Error("[YamlParser::Parse] error in %s: attempt to create duplicate map node: %s", pathName.c_str(), mapKey->nwStringValue.c_str());
//							}
							
							node->mapIndex = topContainer->mapCount ++;
							topContainer->objectMap.insert(std::pair<String, YamlNode*>(mapKey->nwStringValue, node));
							topContainer->objectArray.push_back(SafeRetain(node)); // duplicate in array
							SafeRelease(mapKey);
						}
					}else if (topContainer->type == YamlNode::TYPE_ARRAY)
					{
						topContainer->objectArray.push_back(node);
					}
				}
				
//				NSLog()
//				wprintf(L"scalar: %s %S\n", event.data.scalar.value, node->stringValue.c_str());
//				Logger::FrameworkDebug("scalar: %s %d", event.data.scalar.value, length);
//				CFIndex length = CFStringGetLength(s);
//				UniChar *buffer = malloc(length * sizeof(UniChar));
//				CFStringGetCharacters(str, CFRangeMake(0, length), buffer);
//				node->stringValue = (char buffer;
//				free(buffer);
				
				//node->stringValue = event.data.scalar.value;
			}
			break;
		
		case YAML_DOCUMENT_START_EVENT:
			//Logger::FrameworkDebug("document start:");
			break;
		
		case YAML_DOCUMENT_END_EVENT:
			//Logger::FrameworkDebug("document end:");
			break;

		case YAML_SEQUENCE_START_EVENT:
			{
//				printf("[");
				YamlNode * node = new YamlNode(YamlNode::TYPE_ARRAY);
				if (objectStack.size() == 0)
					rootObject = node;
				else
				{
					YamlNode * topContainer = objectStack.top();
					if (topContainer->type == YamlNode::TYPE_MAP)
					{
						if (mapKey == 0)
						{
							printf("Something wrong");
						}
						else
						{
//							String s = String(mapKey->stringValue.begin(), mapKey->stringValue.end());
//							printf("put to map: %s\n", s.c_str());
							
//							if (topContainer->Get(mapKey->nwStringValue))
//							{
//								Logger::Error("[YamlParser::Parse] error in %s: attempt to create duplicate map node: %s", pathName.c_str(), mapKey->nwStringValue.c_str());
//							}
							
							node->mapIndex = topContainer->mapCount ++;
							topContainer->objectMap.insert(std::pair<String, YamlNode*>(mapKey->nwStringValue, node));
							topContainer->objectArray.push_back(SafeRetain(node));
							SafeRelease(mapKey);
						}
					}else if (topContainer->type == YamlNode::TYPE_ARRAY)
					{
						topContainer->objectArray.push_back(node);
					}
				}
				objectStack.push(node);
			}break;
				
		case YAML_SEQUENCE_END_EVENT:
			{
//				printf("]");
				objectStack.pop();
			}break;
		
		case YAML_MAPPING_START_EVENT:
			{
//				printf("{");
				YamlNode * node = new YamlNode(YamlNode::TYPE_MAP);
				if (objectStack.size() == 0)
					rootObject = node;
				else
				{
					YamlNode * topContainer = objectStack.top();
					
					if (topContainer->type == YamlNode::TYPE_MAP)
					{
						if (mapKey == 0)
						{
//							printf("Something wrong");
						}
						else
						{
							//String s = String(mapKey->stringValue.begin(), mapKey->stringValue.end());
//							printf("put to map: %s\n", s.c_str());

//							if (topContainer->Get(mapKey->nwStringValue))
//							{
//								Logger::Error("[YamlParser::Parse] error in %s: attempt to create duplicate map node: %s", pathName.c_str(), mapKey->nwStringValue.c_str());
//							}
							
							node->mapIndex = topContainer->mapCount ++;
							topContainer->objectMap.insert(std::pair<String, YamlNode*>(mapKey->nwStringValue, node));
							node->stringValue = mapKey->stringValue;
							node->nwStringValue = mapKey->nwStringValue;
							topContainer->objectArray.push_back(SafeRetain(node));
							SafeRelease(mapKey);
						}
					}else if (topContainer->type == YamlNode::TYPE_ARRAY)
					{
						topContainer->objectArray.push_back(node);
					}
				}
				objectStack.push(node);
			}
			break;
				
		case YAML_MAPPING_END_EVENT:
			{
//				printf("}");
				objectStack.pop();
			}
			break;
        default:
            break;
		};

		/* Are we finished? */
		done = (event.type == YAML_STREAM_END_EVENT);
		
		/* The application is responsible for destroying the event object. */
		yaml_event_delete(&event);
		
	}
	
	//rootObject->Print(0);
	
	//printf("%s (%d events)\n", (error ? "FAILURE" : "SUCCESS"), count);
	
	/* Destroy the Parser object. */
	yaml_parser_delete(&parser);
//	fclose(input);
    
    DVASSERT(objectStack.size() == 0);
	
	return true;
}

bool YamlParser::SaveStringsList(const FilePath & fileName, YamlNode * rootNode, uint32 attr)
{
	File * yamlFileToSave = File::Create(fileName, attr);
	if (!yamlFileToSave)
	{
		Logger::Error("[YamlParser::Save] Can't create file: %s for output", fileName.GetAbsolutePathname().c_str());
		return false;
	}
	
	// Strings List is a bit different - it contains one and only Map node with the list of the
	// strings themselves.
	DVASSERT(rootNode->GetType() == YamlNode::TYPE_MAP);
	const MultiMap<String, YamlNode*> & childrenList = rootNode->AsMap();

	bool saveSucceeded = true;
	for (MultiMap<String, YamlNode*>::const_iterator iter = childrenList.begin();
		 iter != childrenList.end(); iter ++)
	{
		saveSucceeded &= WriteStringListNodeToYamlFie(yamlFileToSave, iter->first, iter->second);
	}
	
	// Cleanup the memory.
    SafeRelease(yamlFileToSave);

	return saveSucceeded;
}

bool YamlParser::WriteStringListNodeToYamlFie(File* fileToSave, const String& nodeName, const YamlNode* currentNode) const
{
	const char16* NAME_VALUE_DELIMITER = L"\": ";
	WideString resultString = L"\"";

	// String nodes must be enquoted and contains no "\n" chars.
	resultString += ReplaceLineEndings(StringToWString(nodeName));
	resultString += NAME_VALUE_DELIMITER;

	resultString += ReplaceLineEndings(currentNode->AsWString());
	resultString += L"\n";

	return WriteStringToYamlFile(fileToSave, resultString);
}

bool YamlParser::WriteStringToYamlFile(File* fileToSave, const String& stringToWrite) const
{
    uint32 prevFileSize = fileToSave->GetSize();
    uint32 bytesWritten = fileToSave->Write(stringToWrite.c_str(), stringToWrite.size());
    
    return (fileToSave->GetSize() == prevFileSize + bytesWritten);
}

bool YamlParser::WriteStringToYamlFile(File* fileToSave, const WideString& stringToWrite) const
{
	// Yaml contains UTF8 strings only.
	String utf8String = UTF8Utils::EncodeToUTF8(stringToWrite);
	return WriteStringToYamlFile(fileToSave, utf8String);
}

WideString YamlParser::ReplaceLineEndings(const WideString& rawString) const
{
	WideString resultString = rawString;
	size_t pos = WideString::npos;
	while ((pos = resultString.find(L"\n")) != WideString::npos)
	{
		resultString.replace(pos, WideString(L"\n").length(), L"\\n");
	}

	return resultString;
}

YamlParser::YamlParser()
{
	rootObject = 0;
}

YamlParser::~YamlParser()
{
	SafeRelease(rootObject);
}
	
YamlNode * YamlParser::GetRootNode() const
{
	return rootObject;
}

}