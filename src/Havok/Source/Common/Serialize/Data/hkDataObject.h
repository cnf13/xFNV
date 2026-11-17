#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkDataArray;
class hkDataClass;
class hkDataClassImpl;
class hkDataWorld;

// The serialized data object interface. Only the member src uses is declared.
class hkDataObject {
public:
	// The type of a member or array element. Values checked against the arrays' getType bodies and the
	// BasicArrayImplementation template arguments (PDB): VecArrayImplementation returns 4, 6 and 7, the tuples
	// set 0x20 on their element type, and ArrayArrayImplementation's template argument is 16. The array and tuple
	// element types are the PDB's, in its order after TYPE_ARRAY and TYPE_TUPLE.
	enum Type : int32_t {
		TYPE_VOID = 0,
		TYPE_BYTE = 1,
		TYPE_INT = 2,
		TYPE_REAL = 3,
		TYPE_VEC_4 = 4,
		TYPE_VEC_8 = 5,
		TYPE_VEC_12 = 6,
		TYPE_VEC_16 = 7,
		TYPE_OBJECT = 8,
		TYPE_STRUCT = 9,
		TYPE_CSTRING = 10,
		TYPE_NUM_BASIC_TYPES = 11,
		TYPE_MASK_BASIC_TYPES = 0xF,
		TYPE_ARRAY = 0x10,
		TYPE_ARRAY_BYTE = 0x11,
		TYPE_ARRAY_INT = 0x12,
		TYPE_ARRAY_REAL = 0x13,
		TYPE_ARRAY_VEC_4 = 0x14,
		TYPE_ARRAY_VEC_8 = 0x15,
		TYPE_ARRAY_VEC_12 = 0x16,
		TYPE_ARRAY_VEC_16 = 0x17,
		TYPE_ARRAY_OBJECT = 0x18,
		TYPE_ARRAY_STRUCT = 0x19,
		TYPE_ARRAY_CSTRING = 0x1A,
		TYPE_TUPLE = 0x20,
		TYPE_TUPLE_BYTE = 0x21,
		TYPE_TUPLE_INT = 0x22,
		TYPE_TUPLE_REAL = 0x23,
		TYPE_TUPLE_VEC_4 = 0x24,
		TYPE_TUPLE_VEC_8 = 0x25,
		TYPE_TUPLE_VEC_12 = 0x26,
		TYPE_TUPLE_VEC_16 = 0x27,
		TYPE_TUPLE_OBJECT = 0x28,
		TYPE_TUPLE_STRUCT = 0x29,
		TYPE_TUPLE_CSTRING = 0x2A,
	};

	// A position in an object's member list: an index.
	typedef int32_t Iterator;

	// The identity of an object: its implementation and a second word (the native objects' class, 0 for the
	// dictionary ones). Member functions return it through a hidden pointer (MS ABI).
	struct Handle {
		void* p0;
		void* p1;
	};

	// A reference to one member of an object.
	class Value {
	public:
		// m_impl->asInt(m_name), truncated to int.
		int32_t asInt() const;
		// m_impl->asVec(m_name, nreal).
		const float* asVec(int32_t n) const;
		// m_impl->asString(m_name).
		const char* asString() const;
		// m_impl->asReal(m_name) (thiscall, ST0, 0xDBAA00).
		float asReal() const;
		// The SDK's operator=(int): m_impl's int64 assign (slot 16), the value sign-extended (thiscall, ret 4,
		// 0xDBAA30). Not named in the PDB or IDA; name chosen here.
		void setInt(int32_t a1);
		// The SDK's operator=(const hkDataObject&): m_impl's slot of assign(handle, hkDataObjectImpl*) with o's
		// implementation. Not named in the PDB or IDA; name chosen here.
		void setObject(const hkDataObject& o);
		void setInt64(int32_t a1, int32_t a2);
		// Inferred type: a1: float from 0xdc5af7 (result of hkDataObject::Value::asReal)
		void setReal(float a1);
		// Stores a half: m_impl's slot 18 (the half assign, which reads the argument's low word)
		// with m_name and h's dword as pushed.
		void operator=(hkHalf h);
		void operator=(bool b);
		// Stores the member l refers to in this member: m_impl's slot 23 with m_name and &l.
		void operator=(const Value& l);
		// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
		void setVec(const float* a1, int32_t a2);
		// The SDK's operator=(const char*): m_impl's string assign (slot 20) (thiscall, ret 4, 0xDBAB30). Not named
		// in the PDB or IDA; name chosen here.
		void setString(const char* a1);
		void setArray(const hkDataArray& a);
		// The member's type, from m_impl's class.
		hkDataObject::Type getType() const;
		// The PDB's hkDataObject asObject() const: the result through the hidden result pointer apResult
		// (thiscall, ret 4, 0xDBA9D0).
		hkDataObject* asObject(hkDataObject* apResult) const;
		// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
		// Havok SDK: result through the hidden result pointer apResult.
		hkDataArray* asArray(hkDataArray* apResult) const;

		class hkDataObjectImpl* m_impl;
		// The member's name: the dictionary objects' interned name, the native objects' name as passed in.
		const char* m_name;
	};

	// A null object.
	hkDataObject();
	// Takes an external reference on impl.
	hkDataObject(class hkDataObjectImpl* impl);
	// Takes an external reference on a1's implementation.
	hkDataObject(const hkDataObject& a1);
	// Releases m_impl's external reference, deleting it with the last reference.
	~hkDataObject();

	// Takes a reference on o's implementation, releases this one's (destroying it when
	// that was the last), then holds o's.
	void operator=(const hkDataObject& o);
	// Havok SDK signature (hkSerialize.lib, tied by reloc).
	int32_t getMemberIterator() const;
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	int32_t isValid(int32_t it) const;
	// Havok SDK signature (hkSerialize.lib, tied by reloc).
	int32_t getNextMember(int32_t it) const;
	// Havok SDK signature (hkSerialize.lib, tied by reloc).
	int32_t hasMember(const char* name) const;
	// The object's handle, or a null handle when m_impl is null.
	Handle* getHandle(Handle* apResult) const;
	// m_impl->getClass() as an hkDataClass.
	hkDataClass* getClass(hkDataClass* apResult) const;
	// Not reconstructed yet. The PDB's Value operator[](name): m_impl->accessByName(name) (thiscall, ret 8,
	// 0xDBA870). src names it access because the exe returns the Value through the hidden result pointer apResult.
	Value* access(Value* apResult, const char* name);
	// Not reconstructed yet. The PDB's const Value operator[](name) const, the same code as the non-const one
	// (thiscall, ret 8, 0xDBA890); const callers (Writer::writeObject 0xDC00B0) call this copy.
	Value* access(Value* apResult, const char* name) const;
	// m_impl->getMemberName(i).
	const char* getMemberName(int32_t it) const;
	// m_impl->getMemberValue(i).
	Value* getMemberValue(Value* apResult, int32_t i) const;
	// Whether the object has no implementation; Havok's hkBool32.
	int32_t isNull() const;
	// Whether both objects have the same handle.
	int32_t operator==(const hkDataObject& o);
	// Not operator==.
	bool operator!=(const hkDataObject& o);

	// The object's implementation. hkDataObject has constructors in the PDB, so member functions return it through
	// a hidden pointer (MS ABI, as every class return of a member function).
	class hkDataObjectImpl* m_impl;
};

// The serialized class interface. Only the member description is declared.
class hkDataClass {
public:
	// One member's description, as getDeclaredMemberInfo and getMemberInfo fill it.
	struct MemberInfo {
		const char* m_name;
		// The class that declares the member.
		const hkDataClassImpl* m_owner;
		hkDataObject::Type m_type;
		// The class of an object or struct member, else null.
		const hkDataClassImpl* m_class;
		int32_t m_tupleCount;
	};

	// The description a world builds a new class from: the name, the version, the parent's name and the members.
	struct Cinfo {
		// One member of the new class: its name, type, tuple count and class name.
		struct Member {
			const char* name;
			hkDataObject::Type type;
			int32_t tupleCount;
			const char* className;
		};

		const char* name;
		int32_t version;
		const char* parent;
		hkArray<Member> members;
	};

	// The hybrid's implicit default constructor: a call fills the local in (getParent's result).
	hkDataClass() = default;
	hkDataClass(hkDataClassImpl* impl);
	// The class only refers to its implementation: nothing to release.
	~hkDataClass() = default;

	// m_impl->getName() (thiscall, plain ret, 0xDBA680).
	const char* getName() const;
	// The class's description, into arCinfo.
	void getCinfo(Cinfo& cinfo) const;
	// The parent class, into arResult.
	hkDataClass& getParent(hkDataClass& arResult) const;
	// Whether the class is null.
	int32_t isNull() const;
	hkBool* isSuperClass(hkBool* apResult, const hkDataClass& k);
	int32_t getDeclaredMemberIndexByName(const char* name);
	void getDeclaredMemberInfo(int32_t i, hkDataClass::MemberInfo& info);
	void getMemberInfo(int32_t i, hkDataClass::MemberInfo& info);
	int32_t getNumMembers() const;
	int32_t getMemberIndexByName(const char* name);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void getAllMemberInfo(hkArrayBase<hkDataClass::MemberInfo>& minfos) const;
	const hkDataWorld* getWorld();
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	int32_t getVersion() const;
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	int32_t getNumDeclaredMembers() const;
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void getAllDeclaredMemberInfo(hkArrayBase<hkDataClass::MemberInfo>& minfos) const;

	hkDataClassImpl* m_impl;
};
