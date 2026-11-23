#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyCinfo.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"

#include <bit>
#include <xmmintrin.h>

namespace {
    // Each member's offset in this struct (-1: no default), then the defaults of contactAngleSensitivity and
    // maxCastIterations.
    struct hkpCharacterProxyCinfo_DefaultStruct {
        int32_t s_defaultOffsets[19];
        float m_contactAngleSensitivity;
        int32_t m_maxCastIterations;
    };
} // namespace

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpCharacterProxyCinfoTypeInfo = {
    "hkpCharacterProxyCinfo",
    nullptr,
    cleanupLoadedObjecthkpCharacterProxyCinfo,
    nullptr,
    &typeid(hkpCharacterProxyCinfo),
    sizeof(hkpCharacterProxyCinfo)
};

// The member table of hkpCharacterProxyCinfo's class record, as the Havok SDK's class file has it.
using hkpCharacterProxyCinfoClass_MembersArray = const hkClassMember[19];
static hkpCharacterProxyCinfoClass_MembersArray hkpCharacterProxyCinfoClass_Members{
    {"position", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"velocity", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
    {"dynamicFriction",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     nullptr},
    {"staticFriction", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x24, nullptr},
    {"keepContactTolerance",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x28,
     nullptr},
    {"up", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30, nullptr},
    {"extraUpStaticFriction",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x40,
     nullptr},
    {"extraDownStaticFriction",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x44,
     nullptr},
    {"shapePhantom",
     &hkpShapePhantomClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x48,
     nullptr},
    {"keepDistance", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4C, nullptr},
    {"contactAngleSensitivity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x50,
     nullptr},
    {"userPlanes", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x54, nullptr},
    {"maxCharacterSpeedForSolver",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x58,
     nullptr},
    {"characterStrength",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x5C,
     nullptr},
    {"characterMass", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x60, nullptr},
    {"maxSlope", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x64, nullptr},
    {"penetrationRecoverySpeed",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x68,
     nullptr},
    {"maxCastIterations",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x6C,
     nullptr},
    {"refreshManifoldInCheckSupport",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x70,
     nullptr}
};

// The defaults the class record names.
static const hkpCharacterProxyCinfo_DefaultStruct hkpCharacterProxyCinfo_Default{
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x4C, -1, -1, -1, -1, -1, -1, 0x50, -1},
    10.0f,
    10
};
hkClass hkpCharacterProxyCinfoClass(
    "hkpCharacterProxyCinfo",
    nullptr,
    128,
    nullptr,
    0,
    nullptr,
    0,
    hkpCharacterProxyCinfoClass_Members,
    19,
    &hkpCharacterProxyCinfo_Default,
    nullptr,
    0,
    0
);

// Zero position and velocity, up along +y, and the SDK's default frictions, tolerances and limits.
hkpCharacterProxyCinfo::hkpCharacterProxyCinfo()
  : m_position(0.0f, 0.0f, 0.0f, 0.0f), m_velocity(0.0f, 0.0f, 0.0f, 0.0f), m_dynamicFriction(1.0f),
    m_staticFriction(0.0f), m_keepContactTolerance(0.1f), m_up(0.0f, 1.0f, 0.0f, 0.0f), m_extraUpStaticFriction(0.0f),
    m_extraDownStaticFriction(0.0f), m_shapePhantom(nullptr), m_keepDistance(0.05f), m_contactAngleSensitivity(10.0f),
    m_userPlanes(4), m_maxCharacterSpeedForSolver(10.0f), m_characterStrength(HK_REAL_MAX), m_characterMass(0.0f),
    m_maxSlope(1.5707964f), m_penetrationRecoverySpeed(1.0f), m_maxCastIterations(10),
    m_refreshManifoldInCheckSupport(false) {}

const hkClass& hkpCharacterProxyCinfo::staticClass() {
    return hkpCharacterProxyCinfoClass;
}
