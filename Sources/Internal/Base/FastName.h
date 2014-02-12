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



#ifndef __DAVAENGINE_FAST_NAME__
#define __DAVAENGINE_FAST_NAME__

#include "Base/HashMap.h"
#include "Base/StaticSingleton.h"

namespace DAVA
{

struct FastNameDB : public StaticSingleton<FastNameDB>
{
	FastNameDB()
		: namesHash(HashMap<const char *, int>(8192 * 2, -1))
	{};

	~FastNameDB()
	{
		for(size_t i = 0; i < namesTable.size(); ++i)
		{
			if(NULL != namesTable[i])
			{
				free((void *) namesTable[i]);
				namesTable[i] = NULL;
			}
		}
	}

	Vector<const char *> namesTable;
	Vector<int> namesRefCounts;
	Vector<int> namesEmptyIndexes;
	HashMap<const char *, int> namesHash;
};

class FastName
{
public:
	FastName();
	explicit FastName(const char * name);
	FastName(const FastName & _name);
    explicit FastName(const String & name);
	~FastName();

	void Reset();
	
	const char* c_str() const
	{
		DVASSERT(index >= -1 && index < (int)FastNameDB::Instance()->namesTable.size());
		if(index >= 0)
		{
			return FastNameDB::Instance()->namesTable[index];
		}
		
		return NULL;
	}
	
	inline FastName& operator=(const FastName &_name)
	{
		RemRef(index);
		
		index = _name.index;
		
#ifdef DAVA_DEBUG
		debug_str_ptr = _name.debug_str_ptr;
#endif
		
		AddRef(index);
		return *this;
	}

	
	inline bool operator==(const FastName &_name) const
	{
		return index == _name.index;
	}
	
	inline bool operator!=(const FastName &_name) const
	{
		return index != _name.index;
	}
	
	inline const char* operator*() const
	{
		return c_str();
	}
	
	inline int Index() const
	{
		return index;
	}
	
	inline bool IsValid() const
	{
		return (index >= 0);
	}

    
private:
    void Init(const char * name);
	int index;

#ifdef DAVA_DEBUG
	const char* debug_str_ptr;
#endif

	void AddRef(int i) const;
	void RemRef(int i) const;
};

};

#endif // __DAVAENGINE_FAST_NAME__
