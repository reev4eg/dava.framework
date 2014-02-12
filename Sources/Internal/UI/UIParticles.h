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



#ifndef __DAVAENGINE_UI_PARTICLES__
#define __DAVAENGINE_UI_PARTICLES__

#include "Base/BaseTypes.h"
#include "UI/UIControl.h"
#include "Scene3D/Components/ParticleEffectComponent.h"
#include "Scene3D/Systems/ParticleEffectSystem.h"

namespace DAVA 
{

class UIParticles : public UIControl 
{
protected:
    virtual ~UIParticles();
public:
	UIParticles(const Rect &rect = Rect(), bool rectInAbsoluteCoordinates = FALSE);


    /*methods analogical to once in ParticleEffectComponent*/
    void Start();
    void Stop(bool isDeleteAllParticles = true);
    void Restart(bool isDeleteAllParticles = true); 
    bool IsStopped();	
    void Pause(bool isPaused = true);
    bool IsPaused();

    virtual void AddControl(UIControl *control);
    virtual void Update(float32 timeElapsed);
    virtual void Draw(const UIGeometricData &geometricData);

    // Load/save functionality.
    virtual void LoadFromYamlNode(const YamlNode * node, UIYamlLoader * loader);
    virtual YamlNode* SaveToYamlNode(UIYamlLoader * loader);

    void Load(const FilePath& path);
    void Reload();
    
    const FilePath& GetEffectPath() const;

    void SetAutostart(bool value);
    bool IsAutostart() const;

protected:
    // Start the playback in case Autostart flag is set.
    void HandleAutostart();

private:
    ParticleEffectComponent *effect;
    ParticleEffectSystem *system;
    Matrix4 matrix;
    //Camera *camera;
    float32 updateTime;

    FilePath effectPath;
    bool isAutostart;


    struct ParticleCameraWrap
    {
        Camera *camera;
        ParticleCameraWrap();
        ~ParticleCameraWrap();
    };
    static ParticleCameraWrap defaultCamera;
};
	
};

#endif