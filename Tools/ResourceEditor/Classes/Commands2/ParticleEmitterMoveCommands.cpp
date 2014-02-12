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



#include "Commands2/ParticleEmitterMoveCommands.h"

ParticleEmitterMoveCommand::ParticleEmitterMoveCommand(DAVA::ParticleEffectComponent *_oldEffect, DAVA::ParticleEmitter* _emitter, DAVA::ParticleEffectComponent *_newEffect, int _newIndex)
	: Command2(CMDID_PARTICLE_EMITTER_MOVE, "Move particle emitter")
	, emitter(_emitter)
	, oldEffect(_oldEffect)
	, oldIndex(-1)
	, newEffect(_newEffect)
	, newIndex(_newIndex)
{
	SafeRetain(emitter);
	if(NULL != emitter && NULL != oldEffect)
	{
		oldIndex = oldEffect->GetEmitterId(emitter);
	}
}

ParticleEmitterMoveCommand::~ParticleEmitterMoveCommand()
{
	SafeRelease(emitter);
}

void ParticleEmitterMoveCommand::Undo()
{
	if(NULL != emitter)
	{
		if(NULL != newEffect)
		{
			newEffect->RemoveEmitter(emitter);
		}

		if(NULL != oldEffect)
		{
			if(-1 != oldIndex)
			{
				oldEffect->InsertEmitterAt(emitter, oldIndex);
			}
			else
			{
				oldEffect->AddEmitter(emitter);
			}
		}
	}
}

void ParticleEmitterMoveCommand::Redo()
{
	if(NULL != emitter && NULL != newEffect)
	{
		if(NULL != oldEffect)
		{
			oldEffect->RemoveEmitter(emitter);
		}

		if(-1 != newIndex)
		{
			newEffect->InsertEmitterAt(emitter, newIndex);
		}
		else
		{
			newEffect->AddEmitter(emitter);
		}
	}
}
