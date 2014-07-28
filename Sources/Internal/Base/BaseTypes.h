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


#ifndef __DAVAENGINE_TYPES_H__
#define __DAVAENGINE_TYPES_H__

#include "DAVAConfig.h"
#include "Base/TemplateHelpers.h"

// Platform detection:

#if defined(__GNUC__) && ( defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__) )
//#if !defined(_WIN32) // fix for mac os platforms
#include <TargetConditionals.h>
#endif


#if defined(TARGET_OS_IPHONE)
#if TARGET_OS_IPHONE
	#if !defined(__DAVAENGINE_IPHONE__) // for old projects we check if users defined it
		#define __DAVAENGINE_IPHONE__
	#endif
#endif
#endif


#ifndef __DAVAENGINE_IPHONE__
#if defined(_WIN32)
#define __DAVAENGINE_WIN32__
//#elif defined(__APPLE__) || defined(MACOSX)
#elif defined(__GNUC__) && ( defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__) )
#define __DAVAENGINE_MACOS__

#if MAC_OS_X_VERSION_10_6 <= MAC_OS_X_VERSION_MAX_ALLOWED
#define __DAVAENGINE_MACOS_VERSION_10_6__
#endif //#if MAC_OS_X_VERSION_10_6 <= MAC_OS_X_VERSION_MAX_ALLOWED

#endif
#endif


// add some other platform detection here...
#if !defined (__DAVAENGINE_IPHONE__) && !defined(__DAVAENGINE_WIN32__) && !defined(__DAVAENGINE_MACOS__)
#if defined(__ANDROID__) || defined(ANDROID) 
	#define __DAVAENGINE_ANDROID__
#endif //#if defined(__ANDROID__) || defined(ANDROID) 
#endif //#if !defined (__DAVAENGINE_IPHONE__) && !defined(__DAVAENGINE_WIN32__) && !defined(__DAVAENGINE_MACOS__)


/////////
// Default headers per platform:


#if defined(__DAVAENGINE_WIN32__)
#define __DAVASOUND_AL__
#define WIN32_LEAN_AND_MEAN
//#include <windef.h>
#include <windows.h>
#include <windowsx.h>
#undef DrawState
#undef GetCommandLine
#undef GetClassName

#elif defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_MACOS__) // Mac & iPhone
#define __DAVASOUND_AL__

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>

#elif defined(__DAVAENGINE_ANDROID__)
//TODO: specific includes
//#define __DAVASOUND_AL__
#undef __DAVASOUND_AL__

#else
// some other platform...

#endif 


//#define _HAS_ITERATOR_DEBUGGING 0
//#define _SECURE_SCL 0

// MSVS: conversion from type1 to type2, possible loss of data

#if defined(__DAVAENGINE_WIN32__)
#pragma warning( push )
#pragma warning( disable : 4244)
#endif 

#include <string>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <set>
#include <stack>
#include <queue>

#if defined(__DAVAENGINE_WIN32__)
#pragma warning( pop )
#endif 

//#if TARGET_OS_IPHONE_SIMULATOR //) || defined(TARGET_IPHONE)
//#if defined(__APPLE__) || defined(MACOSX)
//#define __IPHONE__
//#endif 
//#define __DAVAENGINE_IPHONE__



namespace DAVA
{

typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;

typedef signed char		int8;
typedef signed short	int16;
typedef signed int		int32;

#if defined(_WIN32)
	typedef unsigned __int64 uint64;
	typedef signed __int64	int64;
#else
	typedef unsigned long long uint64;
	typedef signed long long int64;
#endif 
    
typedef Select<sizeof(void*) == 4, uint32, uint64>::Result pointer_size;
    
//#if (sizeof(void*) == 4)
//typedef uint32 pointer_size
//#elif (sizeof(void*) == 8)
//typedef uint64 pointer_size;
//#else
//#error(Pointer type size is invalid);
//#endif
	
#ifndef TRUE
#define TRUE	1
#endif
	
#ifndef FALSE
#define	FALSE	0
#endif

typedef char		char8;
typedef wchar_t		char16;

typedef float			float32;
typedef double			float64;

typedef std::string		String;
#if defined(__DAVAENGINE_ANDROID__)
	typedef std::basic_string<wchar_t>	WideString;
#else //#if defined(__DAVAENGINE_ANDROID__)
	typedef std::wstring	WideString;
#endif //#if defined(__DAVAENGINE_ANDROID__)

	

//template <typename _Ty, typename _Ax = std::allocator(_Ty)> 
//class List : public std::list<_Ty, _Ax>  {};


//#define List std::list
//#define Vector std::vector
template < typename E > class List : public std::list< E > {};
template < typename E > class Vector : public std::vector< E >
{
public:
    typedef E	   value_type;
    typedef size_t size_type;
    explicit Vector(size_type n, const value_type & value = value_type()) : std::vector< E >(n, value) {}
    Vector() : std::vector< E >() {}
};
template < class E > class Set : public std::set< E > {};
template < class E > class Deque : public std::deque< E > {};

template<	class _Kty,
			class _Ty,
			class _Pr = std::less<_Kty>,
			class _Alloc = std::allocator<std::pair<const _Kty, _Ty> > >
class Map : public std::map<_Kty, _Ty, _Pr, _Alloc> {};

template<	class _Kty,
			class _Ty,
			class _Pr = std::less<_Kty>,
			class _Alloc = std::allocator<std::pair<const _Kty, _Ty> > >
class MultiMap : public std::multimap<_Kty, _Ty, _Pr, _Alloc> {};

template < class T, class Container = std::deque<T> > class Stack : public std::stack< T, Container > {};

template < class T, class Container = std::vector<T>, class Compare = std::less<typename Container::value_type> > 
class PriorityQueue : public std::priority_queue< T, Container, Compare > {};

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

template <class T>
inline T Min(T a, T b)
{
	return (a < b) ? (a) : (b);
}

template <class T>
inline T Max(T a, T b)
{
	return (a > b) ? (a) : (b);
}
	
template <class T>
inline T Abs(T a)
{
	return (a >= 0) ? (a) : (-a);
}

template <class T>
inline T Clamp(T val, T a, T b)
{
	return Min(b, Max(val, a));
}
	

#if defined(__DAVAENGINE_WIN32__)
#define Snprinf	_snprintf	
#else //#if defined(__DAVAENGINE_WIN32__)
#define Snprinf	snprintf	
#endif //#if defined(__DAVAENGINE_WIN32__)

#define Memcmp memcmp
#define Memcpy memcpy
#define Memset memset
#define Memmove memmove
#define Alloc malloc
#define Free free
#define Realloc realloc

template <class TYPE>
void SafeDelete(TYPE * &d)
{
	if (d)
	{
		delete d;
		d = 0;
	}
}

template <class TYPE>
void SafeDeleteArray(TYPE * & d)
{
	if (d)
	{
		delete [] d;
		d = 0;
	}
}

#ifndef SAFE_DELETE // for compatibility with FCollada
#define SAFE_DELETE(x) if (x) { delete x; x = 0; };
#endif 
	
#ifndef SAFE_DELETE_ARRAY // for compatibility with FCollada
#define SAFE_DELETE_ARRAY(x) if (x) { delete [] x; x = 0; };
#endif
	
#ifndef OBJC_SAFE_RELEASE
#define OBJC_SAFE_RELEASE(x) [x release];x = nil;
#endif 
	
	/**
	 \enum Graphical object aligment.
	 */
enum eAlign 
{
	ALIGN_LEFT		= 0x01,	//!<Align graphical object by the left side.
	ALIGN_HCENTER	= 0x02,	//!<Align graphical object by the horizontal center.
	ALIGN_RIGHT		= 0x04,	//!<Align graphical object by the right side.
	ALIGN_TOP		= 0x08,	//!<Align graphical object by the top side.
	ALIGN_VCENTER	= 0x10,	//!<Align graphical object by the vertical center.
	ALIGN_BOTTOM	= 0x20,	//!<Align graphical object by the bottom side.
	ALIGN_HJUSTIFY	= 0x40	//!<Used only for the fonts. Stretch font string over all horizontal size of the area.
};

#ifndef COUNT_OF
#define COUNT_OF(x) (sizeof(x)/sizeof(*x))
#endif
    
#ifndef REMOVE_IN_RELEASE
    #if defined(__DAVAENGINE_DEBUG__)
        #define REMOVE_IN_RELEASE (x) x
    #else
        #define REMOVE_IN_RELEASE (x) 
    #endif
#endif

    
//#if defined(__DAVAENGINE_IPHONE__)
#ifdef __thumb__
#error "This file should be compiled in ARM mode only."
    // Note in Xcode, right click file, Get Info->Build, Other compiler flags = "-marm"
#endif
//#endif//#if !defined(__DAVAENGINE_ANDROID__)


#ifndef DAVAENGINE_HIDE_DEPRECATED
#ifdef __GNUC__
#define DAVA_DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DAVA_DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DAVA_DEPRECATED for this compiler")
#define DAVA_DEPRECATED(func) func
#endif
#else
#define DAVA_DEPRECATED(func) func
#endif //DAVAENGINE_HIDE_DEPRECATED
    
enum eErrorCode
{
    SUCCESS,
    ERROR_FILE_FORMAT_INCORRECT,
    ERROR_FILE_NOTFOUND,
    ERROR_READ_FAIL,
    ERROR_WRITE_FAIL
};

};

#endif

