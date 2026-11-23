#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"

class hkClass;
class hkTypeInfo;

// A broad phase handle with the object's type, its owner's offset in the collidable and its collision filter.
class hkpTypedBroadPhaseHandle : public hkpBroadPhaseHandle {
public:
    enum {
        OFFSET_INVALID = 0x7F,
    };

    // The reflection's member table, hkpTypedBroadPhaseHandle::Members.
    using MembersArray = const hkClassMember[4];

    int32_t getType() const;
    void SetCollisionFilterInfo(uint32_t info);
    void* getOwner() const;

    // The SDK's inline setOwner: the owner's byte offset from the handle, in a signed byte.
    void setOwner(void* o) {
        m_ownerOffset = static_cast<int8_t>(static_cast<char*>(o) - reinterpret_cast<char*>(this));
    }

    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();

    int8_t m_type;
    int8_t m_ownerOffset;
    int8_t m_objectQualityType;
    uint32_t m_collisionFilterInfo;

    static MembersArray Members;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpTypedBroadPhaseHandleTypeInfo;
