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



#ifndef __DAVAENGINE_INTROSPECTION_H__
#define __DAVAENGINE_INTROSPECTION_H__

#include "Debug/DVAssert.h"
#include "FileSystem/VariantType.h"

#include "Base/Meta.h"
#include "Base/IntrospectionBase.h"
#include "Base/IntrospectionProperty.h"
#include "Base/IntrospectionCollection.h"
#include "Base/IntrospectionFlags.h"
#include "Base/IntrospectionDynamic.h"
#include "Base/BaseTypes.h"


namespace DAVA
{
	// Класс интроспекции. 
	// Добавляет интроспекцию к любому пользовательскому классу. Интроспекция статическая и существует в единичном экземпляре,
	// не зависимо от количества объектов класса.
	//
	// Использование:
	//
	// 	class A
	// 	{
	// 	public:
	// 		int		i;
	// 		String	s;
	// 		Matrix4	m;
	//		Vector<int> v;
	//
	//		String GetName() { return s; }
	//      void SetName(const String &_s) { s = _s; }
	// 
	// 		INTROSPECTION(A,
	// 			MEMBER(i, "simple int var", 0)
	// 			MEMBER(s, "string", 0)
	//			PROPERTY(s, "property with setter and getter", GetName, SetName, 0)
	//			COLLECTION(v, "vector collection")
	// 			);
	// 	};
	//
	//  class B : public A
	//	{
	//	public:
	//		int b;
	//	
	//		INTROSPECTION_EXTEND(B, A, 
	//			MEMBER(b)
	//			);
	//	}
	//
	class InspInfo
	{
	public:
		InspInfo(const char *_name, const InspMember **_members, const int _members_count)
			: name(_name)
			, meta(NULL)
			, base_info(NULL)
			, members(_members)
			, members_count(_members_count)
		{
			MembersInit();
		}

		InspInfo(const InspInfo *_base, const char *_name, const InspMember **_members, const int _members_count)
			: name(_name)
			, meta(NULL)
			, base_info(_base)
			, members(_members)
			, members_count(_members_count)
		{
			MembersInit();
		}

		~InspInfo()
		{
			MembersRelease();
		}

		const char* Name() const
		{
			return name;
		}

		const MetaInfo* Type() const
		{
			return meta;
		}

		int MembersCount() const
		{
			return members_count;
		}

		// Возвращает указатель на член интроспекции по заданному индексу, или NULL если такой не найден.
		const InspMember* Member(int index) const
		{
			const InspMember *member = NULL;

			if(index < members_count)
				if(NULL != members[index])
					member = members[index];

			return member;
		}

		// Возвращает указатель на член интроспекции по заданному имени, или NULL если такой не найден.
		const InspMember* Member(const char* name) const
		{
			const InspMember *member = NULL;

			for(int i = 0; i < members_count; ++i)
			{
				if(NULL != members[i])
				{
					if(members[i]->name == name)
					{
						member = members[i];
						break;
					}
				}
			}

			return member;
		}

		// Возвращает указатель на базовую интроспекцию, или NULL если такой не существует.
		const InspInfo* BaseInfo() const
		{
			return base_info;
		}
        
		// Единожды установить в текущей интроспекции для типа Т указатель 
		// на мета-информацию типа T
        template<typename T>
        void OneTimeMetaSafeSet()
        {
            if(!metaOneTimeSet)
            {
                metaOneTimeSet = true;
                meta = MetaInfo::Instance<T>();
            }
        }
        
	protected:
		const char* name;
		const MetaInfo* meta;

		const InspInfo *base_info;
		const InspMember **members;
		int members_count;
        
        bool metaOneTimeSet;

		// Инициализация членов интроспекции
		// Все члены интроспекция должны быть валидны(созданы), в противном случае
		// данная интроспекция будет пустой
		void MembersInit()
		{
			// Проверяем или все члены интроспекции валидны
			for(int i = 0; i < members_count; ++i)
			{
				// Если хоть один не создан, то освобождаем все остальные.
				if(NULL == members[i])
				{
					MembersRelease();
					break;
				}

				// обратная связь члена интроспекции непостредственно к интроспекции
				members[i]->ApplyParentInsp(this);
			}
		}

		// Освобождает члены интроспекции и устанавливает их количество в 0
		void MembersRelease()
		{
			for(int i = 0; i < members_count; ++i)
			{
				if(NULL != members[i])
				{
					delete members[i];
					members[i] = NULL;
				}
			}
			members_count = 0;
		}
	};

	class InspInfoDynamic
	{
		friend class InspMemberDynamic;

	public:
		InspInfoDynamic() : memberDynamic(NULL) {};
		virtual ~InspInfoDynamic() {};

		virtual Vector<FastName> MembersList(void *object) const = 0;
		virtual InspDesc MemberDesc(void *object, const FastName &member) const = 0;
		virtual int MemberFlags(void *object, const FastName &member) const = 0;
		virtual VariantType MemberAliasGet(void *object, const FastName &member) const { return VariantType(); };
		virtual VariantType MemberValueGet(void *object, const FastName &member) const = 0;
		virtual void MemberValueSet(void *object, const FastName &member, const VariantType &value) = 0;

		const InspMemberDynamic* GetMember() const { return memberDynamic; };

	protected:
		const InspMemberDynamic* memberDynamic;
	};
};

// Определение интоспекции внутри класса. См. пример в описании класса IntrospectionInfo
#define INTROSPECTION(_type, _members) \
	static const DAVA::InspInfo* TypeInfo() \
	{ \
		typedef _type ObjectT; \
		static const DAVA::InspMember* data[] = { _members }; \
		static DAVA::InspInfo info = DAVA::InspInfo(#_type, data, sizeof(data)/sizeof(data[0])); \
		info.OneTimeMetaSafeSet<_type>(); \
        return &info; \
	} \
	virtual const DAVA::InspInfo* GetTypeInfo() const \
	{ \
		return _type::TypeInfo(); \
	}

// Наследование интоспекции. См. пример в описании класса IntrospectionInfo
#define  INTROSPECTION_EXTEND(_type, _base_type, _members) \
	static const DAVA::InspInfo* TypeInfo() \
	{ \
		typedef _type ObjectT; \
		static const DAVA::InspMember* data[] = { _members }; \
		static DAVA::InspInfo info = DAVA::InspInfo(_base_type::TypeInfo(), #_type, data, sizeof(data)/sizeof(data[0])); \
		info.OneTimeMetaSafeSet<_type>(); \
		return &info; \
	} \
	virtual const DAVA::InspInfo* GetTypeInfo() const \
	{ \
		return _type::TypeInfo(); \
	}

// Определение обычного члена интроспекции. Доступ к нему осуществляется непосредственно.
#define MEMBER(_name, _desc, _flags) \
	new DAVA::InspMember(#_name, _desc, (int) ((long int) &((ObjectT *) 0)->_name), DAVA::MetaInfo::Instance(&ObjectT::_name), _flags),

// Определение члена интроспекции, как свойства. Доступ к нему осуществляется через функци Get/Set. 
#define PROPERTY(_name, _desc, _getter, _setter, _flags) \
	DAVA::CreateIspProp(_name, _desc, &ObjectT::_getter, &ObjectT::_setter, _flags),

// Определение члена интроспекции, как коллекции. Доступ - см. IntrospectionCollection
#define COLLECTION(_name, _desc, _flags) \
	DAVA::CreateInspColl(&((ObjectT *) 0)->_name, #_name, _desc, (int) ((long int) &((ObjectT *) 0)->_name), DAVA::MetaInfo::Instance(&ObjectT::_name), _flags),

// Определение члена интроспекции с динамической структурой. Структуру определяет _dynamic, импементирующая интерфейс InspDynamicInfo
#define DYNAMIC(_name, _desc, _dynamic, _flags) \
	new DAVA::InspMemberDynamic(#_name, _desc, (int)((long int)&((ObjectT *)0)->_name), DAVA::MetaInfo::Instance(&ObjectT::_name), _flags, _dynamic),

#endif // __DAVAENGINE_INTROSPECTION_H__
