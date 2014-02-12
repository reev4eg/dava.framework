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



#ifndef __DAVAENGINE_INTROSPECTION_BASE_H__
#define __DAVAENGINE_INTROSPECTION_BASE_H__

#include "Base/BaseTypes.h"
#include "FileSystem/VariantType.h"
#include "Base/GlobalEnum.h"

namespace DAVA
{
	class InspInfo;
	class InspMemberDynamic;
	class InspColl;
	class KeyedArchive;
	struct MetaInfo;

	// абстрактный базовый класс для интроспекции
	class InspBase
	{
	protected:
		virtual ~InspBase(){}
	public:
		InspBase(){}
		// Возвращает интроспекцию класса
		virtual const InspInfo* GetTypeInfo() const = 0;
	};

	struct InspDesc
	{
		const char *text;
		const EnumMap *enumMap;

		InspDesc(const char *_text) 
			: text(_text)
			, enumMap(NULL)
		{ }

		explicit InspDesc(const char *_text, const EnumMap *_enumMap)
			: text(_text)
			, enumMap(_enumMap)
		{}
	};

	// Базовое представление члена интроспекции
	class InspMember
	{
		friend class InspInfo;

	public:
		InspMember(const char *_name, const InspDesc &_desc, const long int _offset, const MetaInfo *_type, int _flags = 0);

		// Имя члена интроспекции, соответствует имени члена класса
		const char* Name() const;

		// Описание члена интроспекции, произвольно указанное пользователем при объявлении интроспекции
		const InspDesc& Desc() const;

		// Возвдащает мета-тип члена интроспекции
		const MetaInfo* Type() const;

		// Возвращает указатель непосдедственно на данные члена интроспекции
		// Следует учитывать, что если членом интроспекции является указатель, 
		// то данная функция вернет указатель на указатель
		// Проверить является ли член интроспекции указателем можно по его мета-информации:
		//   MetaInfo* meta = member->Type();
		//   meta->IsPointer();
		//
		// для int  a    - функция вернет &a, тип int *
		// для int* a    - функция вернет &a, тип int **
		// для classC  c - функция вернет &c, тип c *
		// для classC* c - функция вернет &c, тип с **
		// 
		// см. так же функцию Object()
		// 
		virtual void* Pointer(void *object) const;

		// Вернет указатель на объек данного члена интроспекции. 
		// Функция сама проверит, является ли данный член интроспекции указателем и
		// если он таковым является то вернет значение данного указателя.
		// Привер по типам члена интроспекции:
		// 
		// для int  a    - функция вернет &a, тип int *
		// для int* a    - функция вернет a,  тип int *
		// для classC  c - функция вернет &c, тип classC *
		// для classC* c - функция вернет c,  тип classC *
		// 
		virtual void* Data(void *object) const;

		// Возвращает вариант данных члена интроспекции. Имлементация варианта должна поддерживать
		// создание из данных, определяемыт мета-типом данного члена интроспекции.
		virtual VariantType Value(void *object) const;

		// Устанавливает данные члена интроспекции из указанного варианта. 
		virtual void SetValue(void *object, const VariantType &val) const;

		// Возвращает данные члена интроспекции в виде коллекции
		virtual const InspColl* Collection() const;

		virtual const InspMemberDynamic* Dynamic() const;

		int Flags() const;

	protected:
		void ApplyParentInsp(const InspInfo *parentInsp) const;

		const char* name;
		InspDesc desc;
		const long int offset;
		const MetaInfo* type;
		const int flags;
		mutable const InspInfo *parentInsp;
	};

	// Базовое представление члена интроспекции, являющегося коллекцией
	class InspColl : public InspMember
	{
	public:
		typedef void* Iterator;

		InspColl(const char *_name, const InspDesc &_desc, const int _offset, const MetaInfo *_type, int _flags = 0)
			: InspMember(_name, _desc, _offset, _type, _flags)
		{ }

		virtual MetaInfo* CollectionType() const = 0;
		virtual MetaInfo* ItemType() const = 0;
		virtual int Size(void *object) const = 0;
		virtual Iterator Begin(void *object) const = 0;
		virtual Iterator Next(Iterator i) const = 0;
		virtual void Finish(Iterator i) const = 0;
		virtual void ItemValueGet(Iterator i, void *itemDst) const = 0;
		virtual void ItemValueSet(Iterator i, void *itemSrc) = 0;
		virtual void* ItemPointer(Iterator i) const = 0;
		virtual void* ItemData(Iterator i) const = 0;
		virtual MetaInfo* ItemKeyType() const = 0;
		virtual const void* ItemKeyPointer(Iterator i) const = 0;
		virtual const void* ItemKeyData(Iterator i) const = 0;
	};

	// Вспомогательный класс для определения содержит ли указанный шаблонный тип интроспекцию
	// Наличие интроспекции определяется по наличию функции GetTypeInfo у данного типа
	template<typename T> 
	class HasInsp
	{
		class yes {	char m; };
		class no { yes m[2]; };

		// Базовый класс для проверки, содержит пустую искомую функцию
		struct TestBase
		{ 
			const InspInfo* GetTypeInfo();
		};

		// Для проверки типа Т мы создаем склаа наследованный от TestBase и Т
		// Таким образом если Т содержит функцию GetTypeInfo, то Test::GetTypeInfo будет указывать
		// на Т::GetTypeInfo, иначе на TestBase::GetTypeInfo
		struct Test : public T, public TestBase 
		{};

		// Вспомогательный класс для вывода типов
		template <typename C, C c> struct Helper
		{};

		// Функция для проверки является содержит заданный тип U имплементацию функции TestBase::GetTypeInfo
		// Компилятор сможет вывести типы для класса Helper только в том случае если &U::GetTypeInfo соответствует 
		// указателю на функцию GetTypeInfo, являющуюся членом класса TestBase
		template <typename U> 
		static no Check(U*, Helper<const InspInfo* (TestBase::*)(), &U::GetTypeInfo>* = 0); 

		// В случае когда вывод типов невозможен для первой функции, будет вызвана эта. Это произойдет только тогда, 
		// когда Т содержит свою функцию GetTypeInfo, а следовательно содержит интроспекцию
		static yes Check(...);

	public: 
		// Статическая переменна, значение которой будет равно true в случае,
		// когда тип Т содержит интроспекцию
		static const bool result = (sizeof(yes) == sizeof(Check((Test*)(0))));

		// Статическая функция для автоматического вывода типа Т по
		// переданной ссылке. 
		static bool resultByObject(const T &t)
		{
			return HasInsp<T>::result;
		}
	};

	// Параметризированные имплементации HasIntrospection для базовых типов 
	// (так как наследование класса Test от базового типа невозможно)
	template<> class HasInsp<void> { public: static const bool result = false; };
	template<> class HasInsp<bool> { public: static const bool result = false; };
	template<> class HasInsp<char8> { public: static const bool result = false; };
	template<> class HasInsp<char16> { public: static const bool result = false; };
	template<> class HasInsp<int8> { public: static const bool result = false; };
	template<> class HasInsp<uint8> { public: static const bool result = false; };
	template<> class HasInsp<int16> { public: static const bool result = false; };
	template<> class HasInsp<uint16> { public: static const bool result = false; };
	template<> class HasInsp<int32> { public: static const bool result = false; };
	template<> class HasInsp<uint32> { public: static const bool result = false; };
	template<> class HasInsp<int64> { public: static const bool result = false; };
	template<> class HasInsp<uint64> { public: static const bool result = false; };
	template<> class HasInsp<float32> { public: static const bool result = false; };
	template<> class HasInsp<float64> { public: static const bool result = false; };
	template<> class HasInsp<KeyedArchive *> { public: static const bool result = false; };
	
	// Глобальная шаблонная функция(#1) для получения интроспекции заданного типа
	// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет true
	template<typename T> 
	typename EnableIf<HasInsp<T>::result, const InspInfo*>::type GetIntrospection() 
	{
		return T::TypeInfo();
	}

	// Глобальная шаблонная функция(#2) для получения интроспекции заданного типа
	// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет false
	template<typename T>
	typename EnableIf<!HasInsp<T>::result, const InspInfo*>::type GetIntrospection() 
	{
		return NULL;
	}

	// Глобальная шаблонная функция(#3) для получения интроспекции типа заданного объекта. 
	// Тип объекта будет выведен компилятором автоматически.
	// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет true
	// Пример:
	//
	// class A {};
	// class B : public class A { ... INTROSPECTION ... }
	// B *b;
	// GetIntrospection(b)			// вернет интроспекцию класса B
	// GetIntrospection((A *) b)	// вернет NULL, т.к. будет вызвана функция #4, см. ниже
	//
	template<typename T> 
	typename EnableIf<HasInsp<T>::result, const InspInfo*>::type GetIntrospection(const T *t) 
	{
		const InspInfo* ret = NULL;

		if(NULL != t)
		{
			ret = t->GetTypeInfo();
		}

		return ret;
	}

	// Глобальная шаблонная функция(#4) для получения интроспекции типа заданного объекта. 
	// Тип объекта будет выведен компилятором автоматически.
	// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет false
	template<typename T>
	typename EnableIf<!HasInsp<T>::result, const InspInfo*>::type GetIntrospection(const T *t) 
	{
		return NULL;
	}
	
	template<typename T>
	const InspInfo* GetIntrospectionByObject(void *object)
	{
		const T* t = (const T *) object;
		return GetIntrospection(t);
	}

	/*
	template<typename T> 
	struct HasIntrospection
	{
		template<int N>
		struct CheckHelper
		{
			char x[N];
		};

		template<typename Q>
		static inline char Check(Q *t, CheckHelper<sizeof(&Q::GetTypeInfo)> *u)
		{
			return sizeof(*u);
		}

		static inline int Check(...)
		{
			return 4;
		}

		static const bool result = (1 == sizeof(Check((T *) 0)));
	};


	template<typename T, bool hasIntrospection> 
	struct GetIntrospectionBase;

	template<typename T> 
	struct GetIntrospectionBase<T, false>
	{
		static inline const IntrospectionInfo* GetInfo() { return NULL; }
	};

	template<typename T> 
	struct GetIntrospectionBase<T, true>
	{
		static inline const IntrospectionInfo* GetInfo() { return &T::GetTypeInfo(); }
	};

	// Глобальная шаблонная функция(#2) для получения интроспекции заданного типа
	// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет false
	template<typename T>
	const IntrospectionInfo* GetIntrospection() 
	{
		return GetIntrospectionBase<T, HasIntrospection<T>::result>::GetInfo();
	}

	template<typename T>
	const IntrospectionInfo* GetIntrospection(const T *t) 
	{
		return GetIntrospectionBase<T, HasIntrospection<T>::result>::GetInfo();
	}

	*/
};

#endif // __DAVAENGINE_INTROSPECTION_BASE_H__
