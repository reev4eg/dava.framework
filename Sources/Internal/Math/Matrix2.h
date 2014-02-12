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


#ifndef __DAVAENGINE_MATH2DMATRIX2_H__
#define __DAVAENGINE_MATH2DMATRIX2_H__

#include <math.h>
#include "Base/BaseTypes.h"
#include "Math/MathDefines.h"
#include "Math/MathConstants.h"

namespace DAVA
{

/**	
	\ingroup math
	\brief Class to work with 2 x 2 matrices.
	It basic purpose to handle rotations in 2D space, but it can be used for any 
	2D transformations that can be performed by such matrix.
 */
struct Matrix2
{
	union
	{
		float32 data[4];
        float32 _data[2][2];
		struct {
			float32 _00, _01;
            float32 _10, _11;
		};
	};
	
	inline Matrix2();
	inline Matrix2(float32 m00, float32 m01, float32 m10, float32 m11);
	inline Matrix2(const Matrix2 & m);

	
	inline float32 Det() const;
	
	// Helpers
	inline void SetIdentity();
	void	BuildRotation(float32 angle);

	inline Matrix2& operator *= (const Matrix2 & arg);
	inline Matrix2 operator *	(const Matrix2 & arg) const;

	inline Matrix2& operator -= (const Matrix2 & arg);
	inline Matrix2 operator -	(const Matrix2 & arg) const;

	inline Matrix2& operator += (const Matrix2 & arg);
	inline Matrix2 operator +	(const Matrix2 & arg) const;
    
    //! Comparison operators
	inline bool operator == (const Matrix2 & _m) const;
	inline bool operator != (const Matrix2 & _m) const;
};



inline Matrix2::Matrix2()
{
    _00 = 1.0f; _01 = 0.0f;
    _10 = 0.0f; _11 = 1.0f;
}


inline Matrix2::Matrix2(float32 m00, float32 m01, float32 m10, float32 m11)
{
	data[0] = m00;
	data[1] = m01;
	data[2] = m10;
	data[3] = m11;
};

inline Matrix2::Matrix2(const Matrix2 & m)
{
	*this = m;
}

inline float32 Matrix2::Det() const
{
	return data[0] * data[3] - data[1] * data[2];
}

inline void Matrix2::SetIdentity()
{
    _00 = 1.0f; _01 = 0.0f;
    _10 = 0.0f; _11 = 1.0f;
}

inline void Matrix2::BuildRotation(float32 angle)
{
	float32	cosA = cosf(angle);
	float32	sinA = sinf(angle);

	data[0] = cosA; data[1] = sinA;
	data[2] = -sinA; data[3] = cosA;
}

inline Matrix2 Matrix2::operator *(const Matrix2 & m) const
{
	return Matrix2( _00 * m._00 + _01 * m._10, 
					_00 * m._01 + _01 * m._11, 
					
					_10 * m._00 + _11 * m._10, 
					_10 * m._01 + _11 * m._11);
}

inline Matrix2& Matrix2::operator *= (const Matrix2 & m)
{
	return (*this = *this * m);
}

inline Matrix2 Matrix2::operator +(const Matrix2 & m) const
{
	return Matrix2( _00 + m._00, _01 + m._01, 
						  _10 + m._10, _11 + m._11);
}

inline Matrix2& Matrix2::operator += (const Matrix2 & m)
{
	return (*this = *this + m);
}

inline Matrix2 Matrix2::operator -(const Matrix2 & m) const
{
	return Matrix2( _00 - m._00, _01 - m._01, 
						  _10 - m._10, _11 - m._11);
}

inline Matrix2& Matrix2::operator -= (const Matrix2 & m)
{
	return (*this = *this - m);
}

    //! Comparison operators
inline bool Matrix2::operator == (const Matrix2 & _m) const
{
    for (uint8 k = 0; k < COUNT_OF(data); ++k)
        if (!FLOAT_EQUAL(data[k], _m.data[k]))
            return false;
    return true;
}
    
inline bool Matrix2::operator != (const Matrix2 & _m) const
{
    return ! Matrix2::operator==(_m);
}


};	// end of namespace DAVA



#endif // __DAVAENGINE_MATH2DMATRIX2_H__

