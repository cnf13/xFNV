#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/hkEntityCinfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBodyCinfo.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <bit>
#include <stdint.h>
#include <xmmintrin.h>

class bhkRigidBody551Cinfo;
class BSSpinLock;
class hkAabb;
class NiAVObject;
class NiPoint3;
class NiQuaternion;

// The creation info a bhkRigidBody builds its Havok body from: the entity info plus the Havok one for the body.
class bhkRigidBodyCinfo : public hkEntityCinfo {
public:
    // The serialized form (bhkSerialized.h): the 32-bit layout, 0xE0 bytes, which files from stream version 35 on
    // hold (bhkRigidBody::LoadCInfo; the game's own files are older and hold bhkRigidBody551Cinfo's).
    struct Serialized : hkEntityCinfo::Serialized {
        uint8_t padding1C[4];
        // hkpRigidBodyCinfo, at 0x20.
        uint32_t m_collisionFilterInfo;
        bhkSerializedPointer m_shape;
        bhkSerializedPointer m_localFrame;
        int8_t m_collisionResponse;
        uint16_t m_contactPointCallbackDelay;
        hkVector4 m_position;
        hkQuaternion m_rotation;
        hkVector4 m_linearVelocity;
        hkVector4 m_angularVelocity;
        hkMatrix3 m_inertiaTensor;
        hkVector4 m_centerOfMass;
        float m_mass;
        float m_linearDamping;
        float m_angularDamping;
        float m_gravityFactor;
        float m_friction;
        float m_restitution;
        float m_maxLinearVelocity;
        float m_maxAngularVelocity;
        float m_allowedPenetrationDepth;
        int8_t m_motionType;
        int8_t m_enableDeactivation;
        int8_t m_solverDeactivation;
        int8_t m_qualityType;
        int8_t m_autoRemoveLevel;
        uint8_t m_responseModifierFlags;
        int8_t m_numShapeKeysInContactPointProperties;
        int8_t m_forceCollideOntoPpu;
    };

    bhkRigidBodyCinfo();
    ~bhkRigidBodyCinfo();

    // Sets the Havok body's motion type, an hkpMotion::MotionType.
    void SetMotionType(hkpMotion::MotionType aeType);
    // Sets the Havok body's rotation through hkQuaternion::operator=.
    void SetRotation(const hkQuaternion& arRot);
    // Stores arPos as the Havok body's position, hk.m_position.
    void SetPosition(const hkVector4& arPos);
    void SetFriction(float afFriction);
    // Sets the Havok body's mass, hk.m_mass.
    void SetMass(float afMass);
    void SetCollisionFilter(uint32_t aiFilter);
    // Sets the shape of the creation info and of the Havok body from a game shape's Havok
    // shape, the null-safe Gethk.
    void SetShape(const bhkShape* apShape);
    // Sets the shape of the creation info and of the Havok body from a Havok shape.
    void SetShape(const hkpShape* apShape);
    void SetRestitution(float afRest);
    // Byte-swaps every element of the creation info in place.
    void Endian();
    void DestroyPropertiesArray();
    void Scale(float afScale);
    // The quality type's name, null for a value without one.
    const char* GetQualityTypeString();
    void ConvertFromOld(bhkRigidBody551Cinfo* apOldCInfo);
    // Fills the info from its serialized form; the shapes and the local frame are left null (the world object's link
    // binds the shape, Init copies it into hk).
    void ReadSerialized(const Serialized& arForm);

    // Byte-swaps every element of arMatrix in place.
    static void EndianMatrix(hkMatrix3& arMatrix);

    // Returns afValue with its four bytes reversed.
    static float SwapFloat(float afValue) {
        union {
            float f;
            uint32_t ui;
        } kBits;
        kBits.f = afValue;
        kBits.ui = std::byteswap<uint32_t>(kBits.ui);
        return kBits.f;
    }

    hkpRigidBodyCinfo hk;
};

static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_collisionFilterInfo) == 0x20);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_contactPointCallbackDelay) == 0x2E);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_position) == 0x30);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_centerOfMass) == 0xA0);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_mass) == 0xB0);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_allowedPenetrationDepth) == 0xD0);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_motionType) == 0xD4);
static_assert(offsetof(bhkRigidBodyCinfo::Serialized, m_forceCollideOntoPpu) == 0xDB);
static_assert(sizeof(bhkRigidBodyCinfo::Serialized) == 0xE0);

void GetTotalMassData(NiAVObject* apObject, hkVector4* arMassCenter, float* apMass);
NiQuaternion* HK2NI_QUATERNION(NiQuaternion* apNiQuat, hkQuaternion* apHkQuat);

// A simulated solid body: the wrapper around an hkpRigidBody, plus the constraints that act on it.
class bhkRigidBody : public bhkEntity {
public:
    // The bare body that bhkRigidBodyT's constructors start from, without Init.
    bhkRigidBody();
    bhkRigidBody(bhkRigidBodyCinfo& arInfo);
    ~bhkRigidBody() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    bhkRigidBody* IsBhkRigidBody() const override {
        return const_cast<bhkRigidBody*>(this);
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    bool Add(bhkWorld* apWorld) override;
    bool Remove() override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override {
        return 0xE0;
    }
    int32_t LoadCInfo(NiStream& arStream) override;
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void Kill() override;
    bool ForceAdd(bhkWorld* apWorld) override;
    NiAVObject* CreateDisplayGeometry(NiNode* apNode) override;
    // The body's position, rotation and centers of mass in Havok units, written to the argument and returned.
    virtual hkVector4& GetPosition(hkVector4& arPos);
    virtual hkQuaternion& GetRotation(hkQuaternion& arRot);
    virtual void SetPosition(const hkVector4& arPos);
    virtual void SetRotation(const hkQuaternion& arRot);

    virtual bool SetMotionType(hkpMotion::MotionType aeMotion);

    virtual void SetPosRot(const hkVector4& arPos, const hkQuaternion& arRot);
    virtual hkVector4& GetCenterOfMassLocal(hkVector4& arCOM);
    virtual hkVector4& GetCenterOfMassInWorld(hkVector4& arCOM);
    virtual hkTransform& GetTransform(hkTransform& arTrans);
    virtual void GetAabb(hkAabb& arOut);
    virtual void SetHavokAssertState(bool abEnabled);

    float GetMass();
    // Moves the keyframed body toward arNewPos and arNewRot over 1 / afInvDeltaTime seconds
    // with afVelGain.
    void ApplyKeyframe(const hkVector4& arNewPos, const hkQuaternion& arNewRot, float afInvDeltaTime, float afVelGain);
    // The mass of the body's dynamic motion, 0 without one.
    float GetDynamicMass();
    // The wrapped body's stored dynamic motion, or null.
    hkpMotion* GetStoredDynamicMotion();
    // The Havok body.
    hkpRigidBody* Gethk() const;
    // The Havok body, or null for a null this.
    hkpRigidBody* operator*() const;
    // The body's linear velocity, or the zero vector without a Havok body.
    const hkVector4& GetLinearVelocity();
    // The body's angular velocity, or the zero vector without a Havok body.
    const hkVector4& GetAngularVelocity();
    // The body's angular velocity converted to game units without scaling, written to arPos.
    void GetAngularVelocity(NiPoint3& arPos);
    // Set the body's velocities, or apply an impulse, inside the world's write marks; nothing without a Havok body.
    void SetLinearVelocity(const hkVector4& arVel);
    void SetAngularVelocity(const hkVector4& arVel);
    void ApplyLinearImpulse(const hkVector4& arImpulse);
    void ApplyAngularImpulse(const hkVector4& arImpulse);
    // The impulse in game units, converted to Havok units and applied through the hkVector4 overload above.
    void ApplyAngularImpulse(const NiPoint3& arImpulse);
    // Adds an action or a constraint to the body; MissileProjectile::ProcessImpale passes a bhkHingeConstraint.
    void AddAction(bhkSerializable* apAction);
    // Activates or deactivates the body.
    void SetActive(bool abTF);
    // Fills arInfo from the wrapped body: bhkWorldObject's part, the Havok body's cinfo, the stored dynamic motion's
    // mass, inertia and damping, and the velocity limits. Not virtual: the virtual CopyCinfo stays bhkSerializable's
    // empty one.
    void CopyCinfo(bhkRigidBodyCinfo& arInfo);
    // The body's shape bounds in its own frame, or a 0.001 box without a shape.
    void GetAabbLocal(hkAabb& arOut);
    // Declared after the virtuals: an earlier same-named non-virtual would make the MS ABI group the virtual
    // SetPosition(const hkVector4&) ahead of GetPosition, changing the vtable order. Moves the body to the game-unit
    // position arPos.
    void SetPosition(const NiPoint3& arPos);
    // Writes the body's position in game units to arPos.
    void GetPosition(NiPoint3& arPos);
    // Writes the body's rotation as a Gamebryo quaternion to arRot.
    void GetRotation(NiQuaternion& arRot);
    // Normalizes the Gamebryo rotation arRot into a Havok one and sets it through the virtual SetRotation.
    void SetRotation(const NiQuaternion& arRot);
    // Sets the linear velocity from game units.
    void SetLinearVelocity(const NiPoint3& arVel);
    // Sets the angular velocity from arVel, unscaled.
    void SetAngularVelocity(const NiPoint3& arVel);
    // Whether the body is awake in its world.
    bool IsActive();
    void SethkMaxLinearVelocity(float afVel);
    // Copies this body's members onto apDestination.
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData);
    // The number of non-null entries of kActionConstrants.
    int32_t CountActions();
    // Returns the address of kActionConstrants.
    BSSimpleList<NiPointer<bhkSerializable>>* GetActionConstraints();
    void RemoveActionConstraints();
    float GethkMaxLinearVelocity();
    const float GetMaxAngularVelocity();
    void AddActConToWorld(bhkWorld* apWorld);
    void ApplyHardKeyframe(const NiPoint3& arPos, const NiQuaternion& arRot, BSSpinLock* apSetPosLock);
    void Fix();
    // The impulse is in game units: it is converted with NI2HK, then applied through ApplyLinearImpulse.
    void ApplyLinearImpulseNi(const NiPoint3& arImpulse);
    void RemoveActionConstraintsFromWorld();
    void UnFix(int32_t abAddRemainingConstraints);

    // Makes apObject's body fixed or dynamic, then its children's. abFix and abAddRemainingConstraints are int32_t
    // rather than bool because they are passed on as the caller's whole 32-bit values; bool would be zero-extended.
    static void SetFixed(NiAVObject* apObject, int32_t abFix, int32_t abAddRemainingConstraints);
    // Whether the motion type aeMotion is a dynamic one.
    static bool IsMotionTypeDynamic(const hkpMotion::MotionType& areMotion);
    // The class loader's factory: returns a newly constructed body.
    static NiObject* CreateObject();
    // Adds the MOTION line for aeType to the viewer strings.
    static void AddMotionType(NiTPrimitiveArray<char*>* apStrings, hkpMotion::MotionType aeType);
    // The MOTION line for a motion type outside the known set.
    static void AddMotionTypeUnknown(NiTPrimitiveArray<char*>* apStrings);
    // The heaviest body in ap3D's subtree: its children's heaviest, or its own body when that is at least as heavy.
    static bhkRigidBody* FindHeaviestRigidBody(NiAVObject* ap3D);
    static void RemoveAllActionConstraints(NiAVObject* apObject);
    static void CalculateCOMAndMassRecursive(NiAVObject* apObject, __m128* arMassCenter, float* apMass);

    // Holds constraints and actions alike: bhkRigidBody::AddAction adds to the head of the list,
    // WeaponObject::Constrain takes them off the head, and Kill releases them all.
    BSSimpleList<NiPointer<bhkSerializable>> kActionConstrants;

    static NiRTTI ms_RTTI;
    // Live bodies, counted by the constructors.
    static inline uint32_t iCount;
    // Bodies at rest are added to a world deactivated (bipeds excepted).
    static inline bool bStartInactive = true;
};

NiSmartPointer(bhkRigidBody);

// The mass is rounded to a float local and returned from there.
inline float bhkRigidBody::GetMass() {
    hkpRigidBody* pBody = **this;
    float fMass = pBody ? pBody->getMass() : 0.0f;
    return fMass;
}

inline const hkVector4& bhkRigidBody::GetLinearVelocity() {
    hkpRigidBody* pBody = **this;
    return pBody ? pBody->getLinearVelocity() : hkVector4::getZero();
}

inline const hkVector4& bhkRigidBody::GetAngularVelocity() {
    hkpRigidBody* pBody = **this;
    return pBody ? pBody->getAngularVelocity() : hkVector4::getZero();
}

inline void bhkRigidBody::SetLinearVelocity(const hkVector4& arVel) {
    hkpRigidBody* pBody = **this;
    if (pBody) {
        MarkWrite();
        pBody->setLinearVelocity(arVel);
        UnMarkWrite();
    }
}

inline void bhkRigidBody::SetAngularVelocity(const hkVector4& arVel) {
    hkpRigidBody* pBody = **this;
    if (pBody) {
        MarkWrite();
        pBody->setAngularVelocity(arVel);
        UnMarkWrite();
    }
}

inline float bhkRigidBody::GethkMaxLinearVelocity() {
    hkUFloat8 apMemory;
    hkpRigidBody* pb = **this;
    if (pb) {
        hkUFloat8* p_m_maxLinearVelocity;
        hkpMotion* pdm = pb->getStoredDynamicMotion();
        if (pdm) {
            p_m_maxLinearVelocity = &pdm->m_motionState.m_maxLinearVelocity;
        }
        else {
            float f = pb->getMaxLinearVelocity();
            apMemory = hkUFloat8(f);
            p_m_maxLinearVelocity = &apMemory;
        }
        return *p_m_maxLinearVelocity;
    }

    return 0.0f;
}

inline void bhkRigidBody::ApplyLinearImpulse(const hkVector4& arImpulse) {
    hkpRigidBody* pBody = **this;
    if (pBody) {
        MarkWrite();
        pBody->applyLinearImpulse(arImpulse);
        UnMarkWrite();
    }
}

inline void bhkRigidBody::GetAngularVelocity(NiPoint3& arPos) {
    HK2NINS(arPos, GetAngularVelocity());
}

// Sets the body's linear velocity from a Gamebryo vector (converted by NI2HK, with the world scale).
inline void bhkRigidBody::SetLinearVelocity(const NiPoint3& arVel) {
    hkVector4 hkv;
    this->SetLinearVelocity(NI2HK(hkv, arVel));
}

// Sets the body's angular velocity from a Gamebryo vector (converted by NI2HKNS, without the world scale).
inline void bhkRigidBody::SetAngularVelocity(const NiPoint3& arVel) {
    hkVector4 hkv;
    this->SetAngularVelocity(NI2HKNS(hkv, arVel));
}

inline bool bhkRigidBody::IsActive() {
    hkpEntity* pEntity = **this;
    if (!pEntity) {
        return false;
    }
    return pEntity->isActive();
}

inline void bhkRigidBody::ApplyAngularImpulse(const hkVector4& arImpulse) {
    hkpRigidBody* pBody = **this;
    if (pBody) {
        MarkWrite();
        pBody->applyAngularImpulse(arImpulse);
        UnMarkWrite();
    }
}

inline void bhkRigidBody::ApplyAngularImpulse(const NiPoint3& arImpulse) {
    hkVector4 hkImpulse;
    this->ApplyAngularImpulse(NI2HK(hkImpulse, arImpulse));
}

inline void bhkRigidBodyCinfo::SetShape(const hkpShape* apShape) {
    m_shape = apShape;
    hk.m_shape = apShape;
}

inline void bhkRigidBodyCinfo::SetMotionType(hkpMotion::MotionType aeType) {
    hk.m_motionType = aeType;
}

inline void bhkRigidBodyCinfo::SetRotation(const hkQuaternion& arRot) {
    hk.m_rotation = arRot;
}

inline void bhkRigidBodyCinfo::SetShape(const bhkShape* apShape) {
    m_shape = **apShape;
    hk.m_shape = m_shape;
}
