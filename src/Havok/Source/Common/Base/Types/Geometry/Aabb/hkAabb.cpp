#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "xfnv/Compiler.h"

#include <bit>

static void cleanupLoadedObjecthkAabbUint32(void* p);
static void cleanupLoadedObjecthkAabb(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkAabbTypeInfo = {"hkAabb", nullptr, cleanupLoadedObjecthkAabb, nullptr, &typeid(hkAabb), sizeof(hkAabb)};
hkTypeInfo hkAabbUint32TypeInfo =
    {"hkAabbUint32", nullptr, cleanupLoadedObjecthkAabbUint32, nullptr, &typeid(hkAabbUint32), sizeof(hkAabbUint32)};

// hkAabbClass's and hkAabbUint32Class's member tables.
using hkAabbClass_MemberArray = const hkClassMember[2];
static hkAabbClass_MemberArray hkAabbClass_Members{
    {"min", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"max", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x10, nullptr}
};
using hkAabbUint32Class_MemberArray = const hkClassMember[6];
static hkAabbUint32Class_MemberArray hkAabbUint32Class_Members{
    {"min",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     3,
     hkClassMember::ALIGN_16,
     0x0,
     nullptr},
    {"expansionMin", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 3, 0, 0xC, nullptr},
    {"expansionShift", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, 0, 0xF, nullptr},
    {"max", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 3, 0, 0x10, nullptr},
    {"expansionMax", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 3, 0, 0x1C, nullptr},
    {"shapeKeyByte", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, 0, 0x1F, nullptr}
};
hkClass hkAabbClass("hkAabb", nullptr, 32, nullptr, 0, nullptr, 0, hkAabbClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkAabbUint32Class(
    "hkAabbUint32",
    nullptr,
    32,
    nullptr,
    0,
    nullptr,
    0,
    hkAabbUint32Class_Members,
    6,
    nullptr,
    nullptr,
    0,
    0
);

static void cleanupLoadedObjecthkAabbUint32(void* p) {
    ;
}

static void cleanupLoadedObjecthkAabb(void* p) {
    ;
}

// hkVector4's default constructor stores nothing, so both corners keep what the memory held.
hkAabb::hkAabb() {}

hkAabb::hkAabb(const hkVector4& min, const hkVector4& max) : m_min(min), m_max(max) {}

hkBool hkAabb::isValid() {
    int32_t v2 = 0;
    while ((std::bit_cast<uint32_t>(XFNV_M128_F32(m_min.m_quad, v2)) & 0x7F800000) != 0x7F800000 &&
           ((std::bit_cast<uint32_t>(XFNV_M128_F32(m_max.m_quad, v2)) & 0x7F800000) != 0x7F800000 &&
            !(XFNV_M128_F32(m_max.m_quad, v2) < static_cast<double>(XFNV_M128_F32(m_min.m_quad, v2))))) {
        if (++v2 >= 3) {
            return 1;
        }
    }
    return 0;
}

const hkClass& hkAabb::staticClass() {
    return hkAabbClass;
}

const hkClass& hkAabbUint32::staticClass() {
    return hkAabbUint32Class;
}
