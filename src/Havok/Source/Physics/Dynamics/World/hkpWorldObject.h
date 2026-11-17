#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"

#include <stdint.h>

class bhkWorldObject;
class hkMotionState;
class hkpShape;
class hkpWorld;
class hkStatisticsCollector;

union hkpPropertyValue {
    // The SDK's inline hkpPropertyValue::getPtr: the stored word, untyped raw data, as a pointer.
    void* getPtr() const {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_data));
    }

    // The reflection class hkpPropertyValueClass.
    static const hkClass& staticClass();

    uint64_t m_data;
};

// One entry of a world object's property list: a key and the value bound to it.
class hkpProperty {
public:
    static const hkClass& staticClass();

    uint32_t m_key;
    uint32_t m_alignmentPadding;
    hkpPropertyValue m_value;
};

// The AI module's data a world object can carry (hkpWorldObject::m_aiData). It is never allocated or extended, only
// used through hkReferencedObject's methods (setAiObjectData adds and removes references and stores it in m_aiData;
// getAiObjectData returns m_aiData), so it adds nothing to hkReferencedObject's layout.
class hkaiObjectData : public hkReferencedObject {};

// An object a Havok world tracks: the world it belongs to, the collision hierarchy others see it through,
// and the properties the game code hangs off it.
class hkpWorldObject : public hkReferencedObject {
public:
    // What the object's broad phase handle (m_collidable.m_broadPhaseHandle.m_type) says it is.
    enum BroadPhaseType {
        BROAD_PHASE_INVALID = 0x0,
        BROAD_PHASE_ENTITY = 0x1,
        BROAD_PHASE_PHANTOM = 0x2,
        BROAD_PHASE_BORDER = 0x3,
        BROAD_PHASE_MAX_ID = 0x4,
    };

    // Whether a property write also runs the engine's threading checks. This build's editProperty ignores it.
    enum MtChecks : int32_t {
        MULTI_THREADING_CHECKS_ENABLE = 0x0,
        MULTI_THREADING_CHECKS_IGNORE = 0x1,
    };

    // The reflected members of hkpWorldObject.
    using MembersArray = const hkClassMember[8];

    hkpWorldObject(const hkpShape* a1, hkpWorldObject::BroadPhaseType a2);
    hkpWorldObject(hkFinishLoadedObjectFlag flag);
    ~hkpWorldObject() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

    // A plain world object accepts any shape.
    virtual hkWorldOperation::Result setShape(const hkpShape*) {
        return hkWorldOperation::DONE;
    }
    virtual hkMotionState* getMotionState() = 0;

    // The value bound to key, zero when no entry has the key. mtCheck only drives the debug build's read check.
    hkpPropertyValue getProperty(uint32_t key, MtChecks mtCheck = MULTI_THREADING_CHECKS_ENABLE) const {
        for (int32_t i = 0; i < m_properties.m_size; ++i) {
            if (m_properties.m_data[i].m_key == key) {
                return m_properties.m_data[i].m_value;
            }
        }
        hkpPropertyValue returnValue;
        returnValue.m_data = 0;
        return returnValue;
    }

    // Binds a value to a key, appending an entry when the key is not taken.
    void addProperty(uint32_t key, hkpPropertyValue value);

    // Unbinds a key, closing the hole it leaves, and returns the value it carried.
    hkpPropertyValue removeProperty(uint32_t key);

    // Stores value in the entry of key, leaving the list alone when the key is absent; mtCheck is ignored.
    void editProperty(uint32_t key, hkpPropertyValue value, MtChecks mtCheck = MULTI_THREADING_CHECKS_ENABLE);

    hkBool isAddedToWorld() const {
        return m_world != nullptr;
    }
    // The game object the Havok object belongs to. m_userData holds the bhkWorldObject.
    bhkWorldObject* getUserData() const {
        return m_userData;
    }
    void setName(const char* name) {
        m_name = name;
    }
    // The world the object is in, m_world.
    hkpWorld* getWorld() const {
        return m_world;
    }
    const hkpCollidable* getCollidable() const {
        return &m_collidable;
    }
    // getMultiThreadCheck().markForRead() and unmarkForRead() are empty in the game's build, so nothing is marked.
    void markForRead() {}
    void unmarkForRead() {}
    void markForWriteImpl();
    void checkReadWrite();
    void checkReadOnly() const;
    // Havok SDK signature (hkpDynamics.lib, tied by reloc).
    void setTreeData(void* data);
    void* getTreeData() const;
    // Havok SDK signature (hkpDynamics.lib, tied by target).
    void unlockProperty(uint32_t key);
    // Makes m_properties a copy of otherObj's.
    void copyProperties(const hkpWorldObject* otherObj);
    void lockProperty(uint32_t key);
    hkaiObjectData* getAiObjectData();
    void setAiObjectData(hkaiObjectData* data);

    // Game code casts it to the game's world subclass where it needs that class.
    hkpWorld* m_world;
    bhkWorldObject* m_userData;
    hkpLinkedCollidable m_collidable;
    hkMultiThreadCheck m_multiThreadCheck;
    hkStringPtr m_name;
    hkArray<hkpProperty> m_properties;
    hkReferencedObject* m_aiData;
    void* m_treeData;

    static MembersArray Members;
};

// The reflection class of hkpWorldObject, which hkpPhantom and hkpEntity pass to the base statistics.
extern hkClass hkpWorldObjectClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpPropertyValueTypeInfo;
extern hkTypeInfo hkpPropertyTypeInfo;
extern hkClass hkpPropertyValueClass;
extern hkClass hkpPropertyClass;
