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



#ifndef __ROTATE_HOOD_H__
#define __ROTATE_HOOD_H__

#include "Scene/System/HoodSystem/HoodObject.h"

#

#define ROTATE_HOOD_CIRCLE_PARTS_COUNT 5

struct RotateHood : public HoodObject
{
	RotateHood();
	~RotateHood();

	virtual void Draw(ST_Axis selectedAxis, ST_Axis mouseOverAxis, TextDrawSystem *textDrawSystem);

	HoodCollObject* axisX;
	HoodCollObject* axisY;
	HoodCollObject* axisZ;

	HoodCollObject* axisXc[ROTATE_HOOD_CIRCLE_PARTS_COUNT];
	HoodCollObject* axisYc[ROTATE_HOOD_CIRCLE_PARTS_COUNT];
	HoodCollObject* axisZc[ROTATE_HOOD_CIRCLE_PARTS_COUNT];

	DAVA::float32 modifRotate;

private:
	DAVA::float32 radius;
};

#endif // __ROTATE_HOOD_H__
