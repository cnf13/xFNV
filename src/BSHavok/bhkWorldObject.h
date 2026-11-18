#pragma once

// hkWorldObjectCinfo's native operator delete frees through MemoryManager_Singleton.
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkSerialized.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Reconstruction.hpp"

class bhkShape;
class CFilter;
class hkpRigidBody;
class hkWorldObjectCinfo;
class NiAVObject;
class NiNode;

#if XFNV_64BIT
// The module's base address, from the linker (the start of the image).
extern "C" const char __ImageBase;
#endif

// The Havok property key a world object's owner is bound under: the address of an RTTI object on 32-bit targets.
// Havok's keys are 32 bits (hkpProperty::m_key), so where an address has 64 bits the key is the RTTI object's offset
// from the module's base, unique per RTTI object.
inline uint32_t bhkPropertyKey(const NiRTTI& arRTTI) {
#if XFNV_64BIT
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&arRTTI) - reinterpret_cast<uintptr_t>(&__ImageBase));
#else
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&arRTTI));
#endif
}

// A Havok object that lives in a world (rigid bodies, phantoms): the wrapper around an hkpWorldObject.
class bhkWorldObject : public bhkSerializable {
public:
    // The bits of iFlags.
    enum FLAGS : int32_t {
        WIND = 0x1,
        DYNAMIC = 0x2,
        FIXEDCONSTRAINTS = 0x4,
        UNDERWATER = 0x8,
        BATCH_ADD = 0x10,
        BATCH_REMOVE = 0x20,
        DISABLED = 0x40,
        UNDER_PLACEABLE_WATER = 0x80,
        STICKING_TARGET = 0x100,
        KEYFRAMED_BY_LOADEDAREABOUND = 0x200,
    };

    bhkWorldObject();
    ~bhkWorldObject() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void Link(bool abAttach) override;
    // The world the wrapped object has been added to, null when it is in none.
    const hkpWorld* GethkWorld() const override;

    // Same as GethkWorld, but also answers for a null wrapper.
    hkpWorld* GethkWorldRw() override;

    bool Add(bhkWorld* apWorld) override;

    // Tells the world the object's collision filter info changed.
    virtual void UpdateCFilter() = 0;

    virtual bool ForceAdd(bhkWorld* apWorld);
    virtual NiAVObject* CreateDisplayGeometry(NiNode* apNode);
    virtual void DestroyDisplayGeometry();

    // Sets the collision filter info.
    void SetCollisionFilter(CFilter aiFilter);
    // The world's write mark around a change to the object, empty in this build.
    void MarkWrite() {}
    void UnMarkWrite() {}
    // The world's read mark around a query of the object, empty in this build.
    void MarkRead() {}
    void UnMarkRead() {}
    // The wrapped Havok object.
    hkpWorldObject* Gethk();
    // The wrapped Havok object, or null for a null this.
    hkpWorldObject* operator*();
    // Clears the bits of aiFlags in iFlags.
    void ClearFlags(uint32_t aiFlags);
    // Sets the bits of aiFlags in iFlags when abTF is true, else clears them.
    void SetFlags(uint32_t aiFlags, bool abTF);
    // Names the wrapped Havok object, inside the world's write marks.
    void SetName(const char* apName);
    // Whether the wrapped Havok object is in a world, false without one.
    bool IsAddedToWorld();
    // Copies this wrapper's members into apDestination for a clone.
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData);
    // Fills arInfo's world-object part from the wrapped object (its collision filter info, shape and broadphase type;
    // the property array emptied), inside the Havok memory context. Not virtual: the virtual CopyCinfo stays
    // bhkSerializable's empty one.
    void CopyCinfo(hkWorldObjectCinfo& arInfo);
    // The collidable of the wrapped object, null when there is none.
    const hkpCollidable* GetCollidable() const;
    void EnableCollision(bool abTF);
    NiAVObject* GetAVObject(uint32_t aiKey);

    // Writes the wrapped object's collision filter info (zero when there is none) to apResult and returns apResult.
    CFilter* GetCollisionFilter(CFilter* apResult) const;

    // Replaces the collision group (the high half of the filter info) and tells the world about it.
    void SetCollisionGroup(uint32_t aiGroup);

    // True when the wrapped object carries a property with the key.
    bool HasProperty(uint32_t aiKey);
    // Binds the scene graph object apObject to the wrapped object under the property aiKey; a zero key binds it under
    // the scene graph base's RTTI.
    void SetAVObject(NiAVObject* apObject, uint32_t aiKey);
    // Drops the reference the wrapped object's shape holds on the shape's bhkShape wrapper, inside the Havok memory
    // context.
    void RembhkShapeRef();
    // The bhk wrapper of the wrapped object's shape.
    bhkShape* GetShape();

    hkpShape* GethkShape();
    hkpCollidable* GetCollidableRw();
    uint32_t GetFlag(int32_t a2);
    int32_t IsFlagSet(int32_t a2);

    // Stores in *apResult the property apObject carries under auiKey, zero for no object, and
    // returns apResult.
    static hkpPropertyValue* GetProperty(hkpPropertyValue* apResult, const hkpWorldObject* apObject, uint32_t auiKey);
    // The wrapper apObject's user data points back to, null for a null apObject.
    static bhkWorldObject* Getbhk(const hkpWorldObject* apObj);

    uint32_t iFlags;

    static NiRTTI ms_RTTI;
    // Live wrappers of this class, counted by the constructors.
    static inline uint32_t iCount;
};

// The creation info a world object's wrapper builds the Havok object from.
class hkWorldObjectCinfo : public hkSerializableCinfo {
public:
    // One entry of the creation info's property list: a key and its 32-bit value.
    struct Property {
        uint32_t m_key;
        uint32_t m_value;
    };

    // The serialized form (bhkSerialized.h): the 32-bit layout above, 0x18 bytes.
    struct Serialized {
        uint32_t m_collisionFilterInfo;
        bhkSerializedPointer m_shape;
        int8_t m_broadPhaseType;
        uint8_t padding09[3];
        bhkSerializedArray m_properties;
    };

    hkWorldObjectCinfo();
    ~hkWorldObjectCinfo();

    // Fills this part from its serialized form; the shape is left null (LinkObject binds it).
    void ReadSerialized(const Serialized& arForm) {
        m_collisionFilterInfo = arForm.m_collisionFilterInfo;
        m_shape = nullptr;
        m_broadPhaseType.m_storage = arForm.m_broadPhaseType;
        arForm.m_properties.ToNative(m_properties);
    }

    // Frees the 16-byte aligned block whose offset byte sits just before it.
    static void operator delete(void* apMemory) {
        uint8_t* pMemory = static_cast<uint8_t*>(apMemory);
        MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
    }

    uint32_t m_collisionFilterInfo;
    const hkpShape* m_shape;
    hkEnum<hkpWorldObject::BroadPhaseType, int8_t> m_broadPhaseType;
    hkArray<Property> m_properties;
};

static_assert(offsetof(hkWorldObjectCinfo::Serialized, m_shape) == 0x4);
static_assert(offsetof(hkWorldObjectCinfo::Serialized, m_broadPhaseType) == 0x8);
static_assert(offsetof(hkWorldObjectCinfo::Serialized, m_properties) == 0xC);
static_assert(sizeof(hkWorldObjectCinfo::Serialized) == 0x18);
