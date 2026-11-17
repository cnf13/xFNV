#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"
#include "Havok/Source/Common/Serialize/Data/hkDataWorld.h"
#include "Reconstruction.hpp"

class hkClassNameRegistry;
class hkDataClassNative;
class hkVtableClassRegistry;

// The object's class from vtable, then that class's name looked up in classReg. Usercall
// (0xDBB100, ret 0): vtable in ecx (its vtable +0x10 called with edx), object in edx, classReg in edi (its vtable
// +0x10 called with the class's name).
const hkClass* findMostDerivedClass(const hkVtableClassRegistry* vtable, const void* object,
	const hkClassNameRegistry* classReg);

// The native world: live objects seen through their classes' reflection records.
class hkDataWorldNative : public hkDataWorld {
public:
	// The world over the builtin type registry's vtable and class name registries.
	hkDataWorldNative();
	~hkDataWorldNative() override;

	hkDataClassImpl* newClass(const hkDataClass::Cinfo& cinfo) override;
	// A zeroed object of klass's registered class, finished by the type registry and wrapped.
	hkDataObjectImpl* newObject(const hkDataClass& klass) const override;
	hkDataArrayImpl* newArray(hkDataObject& _obj, const hkDataClass::MemberInfo& minfo) const override;
	hkEnum<DataWorldType, int32_t> getType() const override;
	// An hkDataObjectNative of m_contents (thiscall, ret 4, hidden result, 0xDBB620).
	hkDataObject getContents() const override;
	// The registry's classes that are not local, as the world's class of each name.
	void findAllClasses(hkArray<hkDataClassImpl*>& classesOut) const override;
	// The world's class of name, made on first lookup.
	hkDataClassImpl* findClass(const char* name) const override;
	// The native object of handle's address and class (thiscall, ret 8, hidden result, 0xDBB5A0).
	hkDataObject findObject(const hkDataObject::Handle& handle) const override;

	void setVtableRegistry(const hkVtableClassRegistry* vt);
	// The native object of object as an instance of klass. Havok SDK signature (hkSerialize.lib,
	// tied by bytes,reloc).
	hkDataObjectImpl* wrapObject(void* object, const hkClass& klass) const;
	void setClassRegistry(const hkClassNameRegistry* r);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc,string).
	void setContents(void* object, const hkClass& klass);

	const hkClassNameRegistry* m_reg;
	const hkVtableClassRegistry* m_vtable;
	// The world's class of each name, made on first lookup.
	mutable hkStringMap<hkDataClassNative*> m_classes;
	// The root object and its class.
	hkVariant m_contents;
};

// A class's reflection record seen as a serialized class.
class hkDataClassNative : public hkDataClassImpl {
public:
	// No references yet; the record of klass in world.
	hkDataClassNative(const hkClass* klass, hkDataWorldNative* world) : m_class(klass), m_world(world) {}
	~hkDataClassNative() override;

	// m_world.
	hkDataWorld* getWorld() const override { return m_world; }
	const char* getName() const override;
	int32_t getVersion() const override;
	// The world's class of the parent's name.
	hkDataClassImpl* getParent() const override;
	// Not reconstructed yet (thiscall, ret 8, hidden result, 0xDBB970).
	hkBool isSuperClass(const hkDataClassImpl* k) const override;
	int32_t getNumDeclaredMembers() const override;
	// fillMemberInfo of the declared member i.
	void getDeclaredMemberInfo(int32_t i, MemberInfo& info) const override;
	int32_t getDeclaredMemberIndexByName(const char* name) const override;
	int32_t getNumMembers() const override;
	// Not reconstructed yet: fillMemberInfo of member i (thiscall, ret 8, 0xDBBFD0).
	void getMemberInfo(int32_t i, MemberInfo& info) const override;
	int32_t getMemberIndexByName(const char* name) const override;

	// info from the member record m.
	void fillMemberInfo(const hkClassMember& mem, MemberInfo& info) const;

	const hkClass* m_class;
	hkDataWorldNative* m_world;
};

// A live array member seen as a serialized array: m_arraySize elements m_elementStride bytes apart from m_address.
class hkDataArrayNative : public hkDataArrayImpl {
public:
	hkDataArrayNative(const hkDataWorldNative* world, const void* address, int32_t arraySize, int32_t elementStride,
		hkClassMember::Type stype, const hkClass* klass, int32_t cArrayCount);
	~hkDataArrayNative() override;

	// m_dataType.
	hkDataObject::Type getType() const override { return m_dataType; }
	// A native array is fixed.
	void clear() override {}
	void reserve(int32_t n) override {}
	void setSize(int32_t n) override {}
	// m_arraySize.
	int32_t getSize() const override { return m_arraySize; }
	const hkDataClassImpl* getClass() const override;
	hkDataArrayImpl* swizzleObjectMember(const char* name) const override;
	const float* asVec(int32_t index) const override;
	// The accessors by m_nativeType (thiscall, ret 4; 0xDBBC60, 0xDBBCF0, 0xDBBD90).
	const char* asString(int32_t index) const override;
	float asReal(int32_t index) const override;
	int32_t asInt(int32_t index) const override;
	int64_t asInt64(int32_t index) const override;
	hkDataObjectImpl* asObject(int32_t index) const override;
	hkDataArrayImpl* asArray(int32_t index) const override;

	const hkDataWorldNative* m_world;
	const void* m_address;
	int32_t m_arraySize;
	int32_t m_elementStride;
	hkClassMember::Type m_nativeType;
	const hkClass* m_class;
	hkDataObject::Type m_dataType;
	int32_t m_tupleCount;
};

// A live object seen as a serialized data object, through its class's reflection record.
class hkDataObjectNative : public hkDataObjectImpl {
public:
	~hkDataObjectNative() override;
	hkDataObject::Handle getHandle() const override;
	const hkDataClassImpl* getClass() const override;
	hkDataObject::Value accessByName(const char* name) override;
	// Whether the class has a member called name.
	int32_t hasMember(const char* name) const override;
	int32_t getMemberIterator() const override { return 0; }
	int32_t isValid(int32_t it) const override;
	int32_t getNextMember(int32_t it) const override;
	const char* getMemberName(int32_t it) const override;
	// Not reconstructed yet (thiscall, ret 8, hidden result, 0xDBBB40).
	const hkDataObject::Value getMemberValue(int32_t it) const override;
	// The accessors (thiscall; 0xDBC150, 0xDBC310, 0xDBC370, 0xDBBB60).
	hkDataArrayImpl* asArray(const char* name) override;
	const char* asString(const char* name) override;
	int64_t asInt(const char* name) override;
	hkDataObjectImpl* asObject(const char* name) override;
	const float* asVec(const char* name, int32_t nreal) override;
	float asReal(const char* name) override;
	// A native object is read-only: every assign is an empty stub. These six are the same function as
	// BSShaderProperty::GetTextureUse (0xD070A0), registered there.
	void assign(const char* handle, const hkDataObject::Value& value) override {}
	void assign(const char* handle, hkDataObjectImpl* value) override {}
	void assign(const char* handle, hkDataArrayImpl* value) override {}
	void assign(const char* handle, const char* value) override {}
	void assign(const char* handle, float r) override {}
	void assign(const char* handle, hkHalf r) override {}
	// These two are the same function as hkpCharacterProxyListener::processConstraintsCallback (0xC6C760),
	// registered there.
	void assign(const char* handle, const float* r, int32_t nreal) override {}
	void assign(const char* handle, int64_t valueIn) override {}

	// The variant of the object at apAddress, resolved through its class, the vtable class
	// registry and the class name registry: apAddress goes in ebx (usercall, 0xDBB140).
	// The exe returns the hkVariant in edx:eax, m_object in eax and m_class in edx, and its callers (0xDBC000,
	// 0xDBC4D0) take the two registers as the pair: declared as that 64-bit integer, which the usercall form carries
	// (the pointers go through uintptr_t into and out of it).
	// Inferred type: apClass: hkClass* from 0xdbc079 (hkDataArrayNative::m_class (this of hkDataArrayNative::asObject))
#if FNVD_64BIT
	// The 64-bit program returns the variant itself: its two pointers do not fit the 64-bit pair.
	static hkVariant DBB140(void* apAddress, const hkClass* apClass, const hkVtableClassRegistry* apVtable,
		const hkClassNameRegistry* apReg);
#else
	static uint64_t DBB140(void* apAddress, const hkClass* apClass, const hkVtableClassRegistry* apVtable,
		const hkClassNameRegistry* apReg);
#endif
	// The native object of the variant, in the world passed in ebx with the variant's address in edi (usercall,
	// 0xDBB260). A variant whose class the world does not know is reported and emptied first.
	// Inferred type: apWorld: hkDataWorldNative* from 0xdbb50b (this of hkDataWorldNative::wrapObject), its members
	// used at 0xdbb272
	static hkDataObjectImpl* DBB260(const hkDataWorldNative* apWorld, hkVariant* apVariant);

	hkVariant m_object;
	const hkDataWorldNative* m_world;
	// No references yet; a copy of the variant object in world.
	hkDataObjectNative(const hkVariant& object, const hkDataWorldNative* world) : m_object(object), m_world(world) {}
};

inline char* hkDataClassNative_s_hkDataObjectTupeAttributeID = const_cast<char*>("hk.DataObjectType");
inline constexpr fnvd::Vtable hkDataWorldNative_vftable{};
inline constexpr fnvd::Vtable hkDataClassNative_vftable{};
inline constexpr fnvd::Vtable hkDataArrayNative_vftable{};
inline constexpr fnvd::Vtable hkDataObjectNative_vftable{};
