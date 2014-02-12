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


#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLayer.h"
#include "Utils/StringFormat.h"
#include "FileSystem/FileSystem.h"


namespace DAVA 
{

#define PARTICLE_EMITTER_DEFAULT_LIFE_TIME 100.0f


ParticleEmitter::YamlCacheMap ParticleEmitter::emitterYamlCache;

PartilceEmitterLoadProxy::PartilceEmitterLoadProxy()
{
	AddFlag(RenderObject::ALWAYS_CLIPPING_VISIBLE);
	bbox = AABBox3(Vector3(0,0,0), Vector3(0,0,0));
}

void PartilceEmitterLoadProxy::Load(KeyedArchive *archive, SerializationContext *serializationContext)
{
	if(NULL != archive)	
		emitterFilename = archive->GetString("pe.configpath");
}

ParticleEmitter::ParticleEmitter()
{	
	Cleanup(false);
}

ParticleEmitter::~ParticleEmitter()
{
	CleanupLayers();	
}

void ParticleEmitter::Cleanup(bool needCleanupLayers)
{
	lifeTime = PARTICLE_EMITTER_DEFAULT_LIFE_TIME;
	emitterType = EMITTER_POINT;
	emissionVector.Set(NULL);
	emissionVector = RefPtr<PropertyLineValue<Vector3> >(new PropertyLineValue<Vector3>(Vector3(1.0f, 0.0f, 0.0f)));
	emissionAngle.Set(NULL);
	emissionAngle = RefPtr<PropertyLineValue<float32> >(new PropertyLineValue<float32>(0.0f));
	emissionRange.Set(NULL);
	emissionRange = RefPtr<PropertyLineValue<float32> >(new PropertyLineValue<float32>(0.0f));
	size = RefPtr<PropertyLineValue<Vector3> >(0);
	colorOverLife = 0;
	radius = 0;
	name = "Particle Emitter";
	
	if (needCleanupLayers)
	{
		CleanupLayers();
	}
}

void ParticleEmitter::CleanupLayers()
{
    for (int32 i=0, sz=layers.size(); i<sz; ++i)
		SafeRelease(layers[i]);
	layers.clear();
}

ParticleEmitter * ParticleEmitter::Clone()
{
	ParticleEmitter* clonedEmitter = new ParticleEmitter();
	clonedEmitter->configPath = this->configPath;
	clonedEmitter->position = this->position;
	
	clonedEmitter->name = name;
	clonedEmitter->lifeTime = lifeTime;

	if (this->emissionVector)
	{
		clonedEmitter->emissionVector = this->emissionVector->Clone();
		clonedEmitter->emissionVector->Release();
	}
	if (this->emissionAngle)
	{
		clonedEmitter->emissionAngle = this->emissionAngle->Clone();
		clonedEmitter->emissionAngle->Release();
	}
	if (this->emissionRange)
	{
		clonedEmitter->emissionRange = this->emissionRange->Clone();
		clonedEmitter->emissionRange->Release();
	}
	if (this->radius)
	{
		clonedEmitter->radius = this->radius->Clone();
		clonedEmitter->radius->Release();
	}
	if (this->colorOverLife)
	{
		clonedEmitter->colorOverLife = this->colorOverLife->Clone();
		clonedEmitter->colorOverLife->Release();
	}
	if (this->size)
	{
		clonedEmitter->size = this->size->Clone();
		clonedEmitter->size->Release();
	}

	clonedEmitter->emitterType = this->emitterType;	
	clonedEmitter->shortEffect = shortEffect;

	clonedEmitter->layers.resize(layers.size());
	for (int32 i=0, sz=layers.size(); i<sz; ++i)
	{		
		clonedEmitter->layers[i] = layers[i]->Clone();		
	}	

	clonedEmitter->emitterFileName = emitterFileName;
	RetainInCache(emitterFileName);

	return clonedEmitter;
}


void ParticleEmitter::AddLayer(ParticleLayer * layer)
{
	if (!layer)
	{
		return;
	}

	// Don't allow the same layer to be added twice.
	Vector<ParticleLayer*>::iterator layerIter = std::find(layers.begin(), layers.end(), layer);
	if (layerIter != layers.end())
	{
		DVASSERT(false);
		return;
	}

	layer->Retain();			
	layers.push_back(layer);		
}

ParticleLayer* ParticleEmitter::GetNextLayer(ParticleLayer* layer)
{
	if (!layer || layers.size() < 2)
	{
		return NULL;
	}

	int32 layersToCheck = layers.size() - 1;
	for (int32 i = 0; i < layersToCheck; i ++)
	{
		ParticleLayer* curLayer = layers[i];
		if (curLayer == layer)
		{
			return layers[i + 1];
		}
	}

	return NULL;
}

void ParticleEmitter::InsertLayer(ParticleLayer * layer, ParticleLayer * beforeLayer)
{
	AddLayer(layer);
	if (beforeLayer)
	{
		MoveLayer(layer, beforeLayer);
	}
}
	
void ParticleEmitter::RemoveLayer(ParticleLayer * layer)
{
	if (!layer)
	{
		return;
	}

	Vector<ParticleLayer*>::iterator layerIter = std::find(layers.begin(), layers.end(), layer);
	if (layerIter != this->layers.end())
	{		
		layers.erase(layerIter);     
		SafeRelease(layer);
	}
}
    
void ParticleEmitter::RemoveLayer(int32 index)
{
    DVASSERT(0 <= index && index < (int32)layers.size());
    RemoveLayer(layers[index]);
}

	
void ParticleEmitter::MoveLayer(ParticleLayer * layer, ParticleLayer * beforeLayer)
{
	Vector<ParticleLayer*>::iterator layerIter = std::find(layers.begin(), layers.end(), layer);
	Vector<ParticleLayer*>::iterator beforeLayerIter = std::find(layers.begin(), layers.end(),beforeLayer);

	if (layerIter == layers.end() || beforeLayerIter == layers.end() ||
		layerIter == beforeLayerIter)
	{
		return;
	}
		
	layers.erase(layerIter);

	// Look for the position again - an iterator might be changed.
	beforeLayerIter = std::find(layers.begin(), layers.end(), beforeLayer);
	layers.insert(beforeLayerIter, layer);
}


void ParticleEmitter::RetainInCache(const FilePath& name)
{
	YamlCacheMap::iterator it = emitterYamlCache.find(FILEPATH_MAP_KEY(name));
	if (it!=emitterYamlCache.end())
	{
		(*it).second.refCount++;
	}
}

void ParticleEmitter::ReleaseFromCache(const FilePath& name)
{
	YamlCacheMap::iterator it = emitterYamlCache.find(FILEPATH_MAP_KEY(name));
	if (it!=emitterYamlCache.end())
	{
		(*it).second.refCount--;
		if (!(*it).second.refCount)
		{
			SafeRelease((*it).second.parser);
			emitterYamlCache.erase(it);
		}
	}
}

YamlParser* ParticleEmitter::GetParser(const FilePath &filename)
{
	YamlParser *res = NULL;
	YamlCacheMap::iterator it = emitterYamlCache.find(FILEPATH_MAP_KEY(filename));
	if (it!=emitterYamlCache.end())
	{
		(*it).second.refCount++;
		res = (*it).second.parser;
	}
	else
	{
		res = YamlParser::Create(filename);
		EmitterYamlCacheEntry entry;
		entry.parser = res;
		entry.refCount = 1;
		emitterYamlCache[FILEPATH_MAP_KEY(filename)] = entry;
	}
	ReleaseFromCache(emitterFileName);
	emitterFileName = filename;
	return res;
}


void ParticleEmitter::LoadFromYaml(const FilePath & filename, bool preserveInheritPosition)
{
    Cleanup(true);
    
	YamlParser * parser = GetParser(filename);
	if(!parser)
	{
		Logger::Error("ParticleEmitter::LoadFromYaml failed (%s)", filename.GetAbsolutePathname().c_str());
		return;
	}

	configPath = filename;	

	YamlNode * rootNode = parser->GetRootNode();

	const YamlNode * emitterNode = rootNode->Get("emitter");
	if (emitterNode)
	{
	
		const YamlNode * lifeTimeNode = emitterNode->Get("life");
		if (lifeTimeNode)
		{
			lifeTime = lifeTimeNode->AsFloat();
		}else
		{
			lifeTime = PARTICLE_EMITTER_DEFAULT_LIFE_TIME;
		}
		

		const YamlNode * nameNode = emitterNode->Get("name");
		if (nameNode)		
			name = nameNode->AsString();
		if (emitterNode->Get("emissionAngle"))
			emissionAngle = PropertyLineYamlReader::CreatePropertyLine<float32>(emitterNode->Get("emissionAngle"));
        
		if (emitterNode->Get("emissionVector"))
			emissionVector = PropertyLineYamlReader::CreatePropertyLine<Vector3>(emitterNode->Get("emissionVector"));
        
		const YamlNode* emissionVectorInvertedNode = emitterNode->Get("emissionVectorInverted");
		if (!emissionVectorInvertedNode)
		{
			// Yuri Coder, 2013/04/12. This means that the emission vector in the YAML file is not inverted yet.
			// Because of [DF-1003] fix for such files we have to invert coordinates for this vector.
			InvertEmissionVectorCoordinates();
		}

		if (emitterNode->Get("emissionRange"))
			emissionRange = PropertyLineYamlReader::CreatePropertyLine<float32>(emitterNode->Get("emissionRange"));
        
		if (emitterNode->Get("colorOverLife"))
			colorOverLife = PropertyLineYamlReader::CreatePropertyLine<Color>(emitterNode->Get("colorOverLife"));
		if (emitterNode->Get("radius"))
			radius = PropertyLineYamlReader::CreatePropertyLine<float32>(emitterNode->Get("radius"));
								
		const YamlNode * shortEffectNode = emitterNode->Get("shortEffect");
		if (shortEffectNode)
			shortEffect = shortEffectNode->AsBool();
        
		const YamlNode * typeNode = emitterNode->Get("type");
		if (typeNode)
		{	
			if (typeNode->AsString() == "point")
				emitterType = EMITTER_POINT;
			else if (typeNode->AsString() == "line")
			{				
				emitterType = EMITTER_RECT;
			}
			else if (typeNode->AsString() == "rect")
				emitterType = EMITTER_RECT;
			else if (typeNode->AsString() == "oncircle")
				emitterType = EMITTER_ONCIRCLE_VOLUME;
			else if (typeNode->AsString() == "oncircle_edges")
				emitterType = EMITTER_ONCIRCLE_EDGES;
			else if (typeNode->AsString() == "shockwave")
				emitterType = EMITTER_SHOCKWAVE;
			else
				emitterType = EMITTER_POINT;
		}else
			emitterType = EMITTER_POINT;
		
        size = PropertyLineYamlReader::CreatePropertyLine<Vector3>(emitterNode->Get("size"));
        
        if(size == 0)
        {
            Vector3 _size(0, 0, 0);
            const YamlNode * widthNode = emitterNode->Get("width");
            if (widthNode)
                _size.x = widthNode->AsFloat();

            const YamlNode * heightNode = emitterNode->Get("height");
            if (heightNode)
                _size.y = heightNode->AsFloat();

            const YamlNode * depthNode = emitterNode->Get("depth");
            if (depthNode)
                _size.y = depthNode->AsFloat();
            
            size = new PropertyLineValue<Vector3>(_size);
        }        	


	}

	int cnt = rootNode->GetCount();
	for (int k = 0; k < cnt; ++k)
	{
		const YamlNode * node = rootNode->Get(k);
		const YamlNode * typeNode = node->Get("type");				
		if (typeNode && typeNode->AsString() == "layer")
		{
			LoadParticleLayerFromYaml(node, preserveInheritPosition);
		}
	}
	
	// Yuri Coder, 2013/01/15. The "name" node for Layer was just added and may not exist for
	// old yaml files. Generate the default name for nodes with empty names.
	UpdateEmptyLayerNames();		
}

void ParticleEmitter::SaveToYaml(const FilePath & filename)
{
    YamlParser* parser = YamlParser::Create();
    if (!parser)
    {
        Logger::Error("ParticleEmitter::SaveToYaml() - unable to create parser!");
        return;
    }

	configPath = filename;

    YamlNode* rootYamlNode = new YamlNode(YamlNode::TYPE_MAP);
    YamlNode* emitterYamlNode = new YamlNode(YamlNode::TYPE_MAP);
    rootYamlNode->AddNodeToMap("emitter", emitterYamlNode);
        
	emitterYamlNode->Set("name", name);
    emitterYamlNode->Set("type", GetEmitterTypeName());
	emitterYamlNode->Set("shortEffect", shortEffect);
    
    // Write the property lines.
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(emitterYamlNode, "emissionAngle", this->emissionAngle);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(emitterYamlNode, "emissionRange", this->emissionRange);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Vector3>(emitterYamlNode, "emissionVector", this->emissionVector);

	// Yuri Coder, 2013/04/12. After the coordinates inversion for the emission vector we need to introduce the
	// new "emissionVectorInverted" flag to mark we don't need to invert coordinates after re-loading the YAML.
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(emitterYamlNode, "emissionVectorInverted", true);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(emitterYamlNode, "radius", this->radius);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Color>(emitterYamlNode, "colorOverLife", this->colorOverLife);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Vector3>(emitterYamlNode, "size", this->size);    

    // Now write all the Layers. Note - layers are child of root node, not the emitter one.
    int32 layersCount = this->layers.size();
    for (int32 i = 0; i < layersCount; i ++)
    {
        this->layers[i]->SaveToYamlNode(configPath, rootYamlNode, i);
    }

    parser->SaveToYamlFile(filename, rootYamlNode, true);
    parser->Release();
}

void ParticleEmitter::GetModifableLines(List<ModifiablePropertyLineBase *> &modifiables)
{
	PropertyLineHelper::AddIfModifiable(emissionVector.Get(), modifiables);
	PropertyLineHelper::AddIfModifiable(emissionRange.Get(), modifiables);
	PropertyLineHelper::AddIfModifiable(radius.Get(), modifiables);
	PropertyLineHelper::AddIfModifiable(size.Get(), modifiables);
	PropertyLineHelper::AddIfModifiable(colorOverLife.Get(), modifiables);
	int32 layersCount = this->layers.size();
	for (int32 i = 0; i < layersCount; i ++)
	{
		layers[i]->GetModifableLines(modifiables);
	}
}
    


String ParticleEmitter::GetEmitterTypeName()
{
    switch (this->emitterType)
    {
        case EMITTER_POINT:
        {
            return "point";
        }

        case EMITTER_RECT:
        {
            return "rect";
        }

        case EMITTER_ONCIRCLE_VOLUME:
        {
            return "oncircle";
        }

		case EMITTER_ONCIRCLE_EDGES:
        {
            return "oncircle_edges";
        }

		case EMITTER_SHOCKWAVE:
        {
            return "shockwave";
        }

        default:
        {
            return "unknown";
        }
    }
}

void ParticleEmitter::UpdateEmptyLayerNames()
{
	int32 layersCount = layers.size();
	for (int i = 0; i < layersCount; i ++)
	{
		UpdateLayerNameIfEmpty(layers[i], i);
	}
}

void ParticleEmitter::UpdateLayerNameIfEmpty(ParticleLayer* layer, int32 index)
{
	if (layer && layer->layerName.empty())
	{
		layer->layerName = Format("Layer %i", index);
	}
}


void ParticleEmitter::LoadParticleLayerFromYaml(const YamlNode* yamlNode, bool preserveInheritPosition)
{
	ParticleLayer* layer = new ParticleLayer();	
	layer->LoadFromYaml(configPath, yamlNode, preserveInheritPosition);
	AddLayer(layer);	
	SafeRelease(layer);
}

void ParticleEmitter::InvertEmissionVectorCoordinates()
{
	if (!this->emissionVector)
	{
		return;
	}

	PropertyLine<Vector3> *pvk = emissionVector.Get();
	uint32 keysSize = pvk->keys.size();
	for (uint32 i = 0; i < keysSize; ++i)
	{
		pvk->keys[i].value *= -1;
	}
}
}; 
