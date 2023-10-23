#pragma once

#include "Macros/Member.hpp"

#include "Hash/Hash.hpp"

#include <memory>

#include <string>

#define CLASS_COMMON_MEMBERS(n) \
public: \
	typedef std::shred_ptr<n> Shared; \
public: \
	static std::string typeNameStatic() \
	{ \
		return #n; \
	} \
	virtual std::string typeName ()\
	{ \
		return #n; \
	} \
	static unsigned int hashStatic() \
	{ \
		std::string type_name = n::typeNameStatic(); \
		static const unsigned int hash = FNV1a32Hash((void*)type_name.data() \
													,type_name.length()); \
		return hash; \
	} \
	virtual unsigned int hash() \
	{ \
		return n::hashStatic(); \
	} \

#define ROOT_CLASS_BEGIN(n) \
	class n \
	{ \
		CLASS_COMMON_MEMBERS(n) \

#define CLASS_BEGIN(n,p) \
	class n:public p \
	{ \
		CLASS_COMMON_MEMBERS(n) \
		typedef p ParentClass; \

#define CLASS_BEGIN2(n,p1,p2) \
	class n:public p1, public p2 \
	{ \
		CLASS_COMMON_MEMBERS(n)
		typedef p1 ParentClass1; \
		typedef p2 ParentClass2; \

#define CLASS_BEGIN3(n,p1,p2,p3) \
	class n:public p1, public p2, public p3 \
	{ \
		CLASS_COMMON_MEMBERS(n) \
		typedef p1 ParentClass1; \
		typedef p2 ParentClass2; \
		typedef p3 ParentClass3; \

#define CLASS_END };
