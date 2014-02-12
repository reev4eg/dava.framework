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

#ifndef __DAVAENGINE_PARTICLE_RENDER_OBJECT_H_
#define __DAVAENGINE_PARTICLE_RENDER_OBJECT_H_

#include "ParticleGroup.h"

namespace DAVA
{

struct ParticleRenderGroup
{
	RenderBatch *renderBatch;	
	
	Vector<float> vertices;
	Vector<float> texcoords;
	Vector<uint32> colors;

	Vector<float> texcoords2;
	Vector<float> times;	

	uint16 currParticlesCount;
	bool enableFrameBlend;

	void ClearArrays();
	void ResizeArrays(uint32 particlesCount);
	void UpdateRenderBatch(uint32 vertexSize, uint32 vertexStride);
};

class ParticleRenderObject : public RenderObject
{
	ParticleEffectData *effectData;
	Vector<ParticleRenderGroup*> renderGroupCache;

	void AppendParticleGroup(const ParticleGroup &group, ParticleRenderGroup *renderGroup, const Vector3& cameraDirection);	
	void PrepareRenderData(Camera * camera);
    Matrix4* effectMatrix;
	Vector<uint16> indices;
    uint32 sortingOffset;

    uint32 vertexSize, vertexStride;
public:
	ParticleRenderObject(ParticleEffectData *effect);
	~ParticleRenderObject();
	

	virtual void PrepareToRender(Camera *camera);
	
    void SetEffectMatrix(Matrix4 *matrix);
    Matrix4 *GetEffectMatrix();

    void SetSortingOffset(uint32 offset);

    void Set2DMode(bool is2d);

	virtual void RecalcBoundingBox(){}
	virtual void RecalculateWorldBoundingBox(){
		worldBBox = bbox;}
	
};

}

#endif