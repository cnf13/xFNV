#include "BSHavok/hkLiquidAction.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"

#include <bit>
#include <xmmintrin.h>

// The reflected members of hkLiquidAction.
using hkLiquidActionClass_MembersArray = const hkClassMember[1];
static hkLiquidActionClass_MembersArray hkLiquidActionClass_Members{
    {"Force", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x20, nullptr}
};
hkClass hkLiquidActionClass(
    "hkLiquidAction",
    &hkpUnaryActionClass,
    48,
    nullptr,
    0,
    nullptr,
    0,
    hkLiquidActionClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkLiquidActionTypeInfo = {"hkLiquidAction",        finishLoadedObjecthkLiquidAction,
                                     cleanupLoadedObject,     getVtablehkLiquidAction(),
                                     &typeid(hkLiquidAction), sizeof(hkLiquidAction)};

// The type info's finish function: the finish constructor for a non-null p.
void finishLoadedObjecthkLiquidAction(void* p, int32_t finishing) {
    if (p) {
        ::new (p) hkLiquidAction(finishing);
    }
}

// Constructs a finish-constructed object in a stack buffer (no destructor runs) and returns its vptr.
const void* getVtablehkLiquidAction() {
    alignas(16) uint8_t object[sizeof(hkLiquidAction)];
    ::new (object) hkLiquidAction(0);
    // The vptr is not a declared field: the buffer is untyped memory, and its first word is read as `u.ptr`.
    return *reinterpret_cast<const void* const*>(object);
}

void cleanupLoadedObject(void* p) {
    static_cast<hkMemoryAllocator*>(p)->~hkMemoryAllocator();
}

// Force is (0, 0, 0, 0.0).
hkLiquidAction::hkLiquidAction(hkpRigidBody* entityA, uint32_t userData) : hkpUnaryAction(entityA, userData) {
    Force.m_quad = _mm_setr_ps(0.0f, 0.0f, 0.0f, zero);
}

// The finish constructor of a loaded action: Force stays as loaded.
hkLiquidAction::hkLiquidAction(int32_t aiFinishing) : hkpUnaryAction(aiFinishing) {}

void hkLiquidAction::applyAction(const hkStepInfo& stepInfo) {
    hkVector4 v5;
    float m_deltaTime = stepInfo.m_deltaTime;
    hkpRigidBody* ra = static_cast<hkpRigidBody*>(m_entity);
    v5.m_quad = ra->m_motion.m_motionState.m_transform.m_translation.m_quad;
    ra->activate();
    ra->m_motion.applyForce(m_deltaTime, Force, v5);
}

hkpAction* hkLiquidAction::clone(
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& newEntities,
    const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms
) const {
    hkpAction* result;
    TLSData* v4 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v4->eMemContext;
    v4->eMemContext = MEM_CONTEXT_HAVOK;
    if (newEntities.m_size != 2 || newPhantoms.m_size) {
        result = 0;
    }
    else {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkLiquidAction* v8 = static_cast<hkLiquidAction*>(Instance->m_heap->blockAlloc(sizeof(hkLiquidAction)));
        v8->m_memSizeAndFlags = sizeof(hkLiquidAction);
        result = ::new (v8) hkLiquidAction(static_cast<hkpRigidBody*>(*(&newEntities)->begin()), m_userData);
        static_cast<hkLiquidAction*>(result)->Force = Force;
    }
    v4->eMemContext = eMemContext;
    return result;
}
