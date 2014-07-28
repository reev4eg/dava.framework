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

#ifndef __DAVAENGINE_MUSIC_IOS_H__
#define __DAVAENGINE_MUSIC_IOS_H__

#ifdef __DAVAENGINE_IPHONE__

#include "Sound/SoundEvent.h"

namespace DAVA
{
	
class MusicIOSSoundEvent : public SoundEvent
{
public:
    static MusicIOSSoundEvent * CreateMusicEvent(const FilePath & path);
    
    virtual bool Trigger();
    virtual bool IsActive() const;
    virtual void Stop();
    virtual void SetPaused(bool paused);
    
    virtual void SetVolume(float32 volume);
    
    virtual void SetLoopCount(int32 looping); // -1 = infinity
    virtual int32 GetLoopCount() const;
    
    virtual void SetPosition(const Vector3 & position) {};
    virtual void SetDirection(const Vector3 & direction) {};
    virtual void UpdateInstancesPosition() {};
    virtual void SetVelocity(const Vector3 & velocity) {};
    
    virtual void SetParameterValue(const FastName & paramName, float32 value) {};
    virtual float32 GetParameterValue(const FastName & paramName) { return 0.f; };
    virtual bool IsParameterExists(const FastName & paramName) { return false; };

    virtual void GetEventParametersInfo(Vector<SoundEventParameterInfo> & paramsInfo) const { return; };

    virtual String GetEventName() const { return filePath.GetFrameworkPath(); };
    
    void PerformEndCallback();
    
protected:
    MusicIOSSoundEvent(const FilePath & path);
    virtual bool Init();
    virtual ~MusicIOSSoundEvent();
    
    void * avSound;
    FilePath filePath;
};

};

#endif //#ifdef __DAVAENGINE_IPHONE__

#endif //__DAVAENGINE_MUSIC_IOS_H__