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


#ifndef __DAVAENGINE_UNIQUESTATESET_H__
#define __DAVAENGINE_UNIQUESTATESET_H__

#include "Base/BaseTypes.h"
#include "Core/Core.h"

#define InvalidUniqueHandle (static_cast<DAVA::uint32>(-1))

namespace DAVA
{
typedef uint32 UniqueHandle;
//T is element type
//V is comparer type
template<typename T>
class UniqueStateSet
{
public:
	
	UniqueStateSet();
	~UniqueStateSet();
	
	UniqueHandle MakeUnique(const T& objRef);
	const T& GetUnique(UniqueHandle handle);
	bool IsUnique(const T& objRef);
	
	void RetainUnique(UniqueHandle handle);
	void ReleaseUnique(UniqueHandle handle);
	
private:
	
	Vector<T> values;
	Vector<uint32> refCounters;
	uint32 freeSlotCount;
};

template<typename T>
UniqueStateSet<T>::UniqueStateSet()
{
	freeSlotCount = 0;
}

template<typename T>
UniqueStateSet<T>::~UniqueStateSet()
{
	DVASSERT(values.size() == refCounters.size());
}

template<typename T>
UniqueHandle UniqueStateSet<T>::MakeUnique(const T& objRef)
{
	DVASSERT(!IsUnique(objRef));
	
	size_t freeSlot = InvalidUniqueHandle;
	UniqueHandle handle = InvalidUniqueHandle;
	
	size_t count = values.size();
	for(size_t i = 0; i < count; ++i)
	{
		if(0 == refCounters[i])
		{
			freeSlot = i;
		}
			
		if(objRef.Equals(values[i]))
		{
			handle = i;
			break;
		}
	}
	
	if(InvalidUniqueHandle == handle)
	{
		if(freeSlotCount > 0 &&
		   freeSlot != InvalidUniqueHandle)
		{
			freeSlotCount--;
			handle = freeSlot;
		}
		else
		{
			values.push_back(T());
			refCounters.push_back(0);
			handle = values.size() - 1;
		}
		
		values[handle] = objRef;
	}
	
	refCounters[handle] += 1;
	return handle;
}

template<typename T>
const T& UniqueStateSet<T>::GetUnique(UniqueHandle handle)
{
	return values[handle];
}

template<typename T>
bool UniqueStateSet<T>::IsUnique(const T& objRef)
{
	bool unique = ((values.size() != 0) &&
				   ((&objRef >= (&values[0])) &&
					(&objRef < ((&values[0]) + values.size()))));
	return unique;
}

template<typename T>
void UniqueStateSet<T>::ReleaseUnique(UniqueHandle handle)
{
	refCounters[handle] -= 1;
	
	if(0 == refCounters[handle])
	{
		values[handle].Clear();
		freeSlotCount++;
	}
}

template<typename T>
void UniqueStateSet<T>::RetainUnique(UniqueHandle handle)
{
	refCounters[handle] += 1;
}
};

#endif /* defined(__DAVAENGINE_UNIQUESTATESET_H__) */
