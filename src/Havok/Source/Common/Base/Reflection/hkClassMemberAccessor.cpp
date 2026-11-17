#include "Havok/Source/Common/Base/Reflection/hkClassMemberAccessor.h"

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

hkClassAccessor::hkClassAccessor(void* object, const hkClass& klass) {
    m_variant.m_object = object;
    m_variant.m_class = &klass;
}

hkClassMemberAccessor::hkClassMemberAccessor(const hkVariant& v, const char* memberName) {
    connect(v.m_object, *v.m_class, memberName);
}

hkClassMemberAccessor::hkClassMemberAccessor(void* object, const hkClassMember* member) {
    connect(object, member);
}

hkClassMemberAccessor::hkClassMemberAccessor(void* object, const hkClass& klass, const char* memberName) {
    connect(object, klass, memberName);
}

// The address is cleared before the lookup.
void hkClassMemberAccessor::connect(void* object, const hkClass& klass, const char* memberName) {
    m_address = nullptr;
    m_member = klass.getMemberByName(memberName);
    if (m_member) {
        m_address = static_cast<char*>(object) + m_member->m_offset;
    }
}

const hkClassMember& hkClassMemberAccessor::getClassMember() const {
    return *m_member;
}

void* hkClassMemberAccessor::getAddress() const {
    return m_address;
}

void*& hkClassMemberAccessor::asPointer(int32_t index) const {
    return static_cast<void**>(m_address)[index];
}

hkVariant& hkClassMemberAccessor::asVariant(int32_t index) const {
    return static_cast<hkVariant*>(m_address)[index];
}

float& hkClassMemberAccessor::asReal(int32_t index) const {
    return static_cast<float*>(m_address)[index];
}

hkHalf& hkClassMemberAccessor::asHalf(int32_t index) const {
    return static_cast<hkHalf*>(m_address)[index];
}

char*& hkClassMemberAccessor::asCstring(int32_t index) const {
    return static_cast<char**>(m_address)[index];
}

hkStringPtr& hkClassMemberAccessor::asStringPtr(int32_t index) const {
    return static_cast<hkStringPtr*>(m_address)[index];
}

hkClassMemberAccessor::SimpleArray& hkClassMemberAccessor::asSimpleArray(int32_t index) const {
    return static_cast<hkClassMemberAccessor::SimpleArray*>(m_address)[index];
}

hkClassMemberAccessor::HomogeneousArray& hkClassMemberAccessor::asHomogeneousArray(int32_t index) const {
    return static_cast<hkClassMemberAccessor::HomogeneousArray*>(m_address)[index];
}

// The struct member's object: its address, with the class record of the member's struct type.
hkClassAccessor* hkClassMemberAccessor::object(hkClassAccessor* arResult) const {
    const hkClass& klass = m_member->getStructClass();
    ::new (arResult) hkClassAccessor(m_address, klass);
    return arResult;
}

void hkClassMemberAccessor::connect(void* object, const hkClassMember* member) {
    int32_t m_offset = member->m_offset;
    m_member = member;
    m_address = static_cast<char*>(object) + m_offset;
}

hkBool* hkClassMemberAccessor::isOk(hkBool* apResult) const {
    apResult->m_bool = m_address != 0;
    return apResult;
}

void* hkClassMemberAccessor::asRaw() const {
    return m_address;
}

hkBool& hkClassMemberAccessor::asBool(int32_t index) {
    return *(static_cast<hkBool*>(m_address) + index);
}

int32_t& hkClassMemberAccessor::asInt32(int32_t index) const {
    return static_cast<int32_t*>(m_address)[index];
}

uint32_t& hkClassMemberAccessor::asUint32(int32_t index) const {
    return static_cast<uint32_t*>(m_address)[index];
}

int64_t& hkClassMemberAccessor::asInt64(int32_t index) const {
    return static_cast<int64_t*>(m_address)[index];
}

uint64_t& hkClassMemberAccessor::asUint64(int32_t index) const {
    return static_cast<uint64_t*>(m_address)[index];
}

int16_t& hkClassMemberAccessor::asInt16(int32_t index) const {
    return static_cast<int16_t*>(m_address)[index];
}

uint16_t& hkClassMemberAccessor::asUint16(int32_t index) const {
    return static_cast<uint16_t*>(m_address)[index];
}

int8_t& hkClassMemberAccessor::asInt8(int32_t index) const {
    return static_cast<int8_t*>(m_address)[index];
}

uint8_t& hkClassMemberAccessor::asUint8(int32_t index) const {
    return *(static_cast<uint8_t*>(m_address) + index);
}

hkClassMemberAccessor::Vector4& hkClassMemberAccessor::asVector4(int32_t index) {
    return static_cast<hkClassMemberAccessor::Vector4*>(m_address)[index];
}

hkClassMemberAccessor::Matrix3& hkClassMemberAccessor::asMatrix3(int32_t index) {
    return static_cast<hkClassMemberAccessor::Matrix3*>(m_address)[index];
}

hkClassMemberAccessor::Transform& hkClassMemberAccessor::asTransform(int32_t index) {
    return static_cast<hkClassMemberAccessor::Transform*>(m_address)[index];
}

hkClassMemberAccessor::Matrix3& hkClassMemberAccessor::asRotation(int32_t index) {
    return static_cast<hkClassMemberAccessor::Matrix3*>(m_address)[index];
}

hkClassMemberAccessor* hkClassMemberAccessor::member(hkClassMemberAccessor* apResult, const char* name) {
    ::new (apResult) hkClassMemberAccessor(m_address, m_member->getStructClass(), name);
    return apResult;
}

// The subtype's size in bytes: -1 for an enum or flags subtype, the struct class record's object size for a struct
// subtype, and the subtype's type size otherwise.
int32_t hkClassMember::getArrayMemberSize() const {
    if (getSubType() == TYPE_ENUM || getSubType() == TYPE_FLAGS) {
        return -1;
    }
    if (getSubType() == TYPE_STRUCT) {
        return getStructClass().getObjectSize();
    }
    return ClassMemberProperties[getSubType()].m_size;
}
