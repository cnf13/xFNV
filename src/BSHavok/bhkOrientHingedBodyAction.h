#pragma once

#include "BSHavok/bhkUnaryAction.h"
#include "BSHavok/hkOrientHingedBodyAction.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkOrientHingedBodyAction;
class hkOrientHingedBodyActionCinfo;
class NiCloningProcess;

// The scene graph wrapper of hkOrientHingedBodyAction (the hinged-body orienting action); it adds no data of its own.
class bhkOrientHingedBodyAction : public bhkUnaryAction {
public:
    bhkOrientHingedBodyAction();
    ~bhkOrientHingedBodyAction() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    // This override reads the serialized form instead of reading the cinfo raw through bhkSerializable::LoadCInfo.
    int32_t LoadCInfo(NiStream& arStream) override;
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    virtual void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData);
    using bhkUnaryAction::CopyMembers;

    // Copies the action's hinge axis, forward axis, strength and damping into arInfo.
    void CopyCinfo(hkOrientHingedBodyActionCinfo& arInfo);
    void SetTargetDir(const hkVector4& arDir);
    void SetEnabled(bool abEnable);
    hkOrientHingedBodyAction* operator*();

    // The class loader's factory: a new object, or null when the allocation fails.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    // The live wrapper count, which the destructor decrements.
    static inline int32_t iCount;
};

inline hkOrientHingedBodyAction* bhkOrientHingedBodyAction::operator*() {
    if (this) {
        return static_cast<hkOrientHingedBodyAction*>(GethkObject());
    }

    return nullptr;
}

// The creation info of an hkOrientHingedBodyAction: the unary action's user data and entity, then the hinge and forward
// axes, the spring's strength and its damping.
class hkOrientHingedBodyActionCinfo : public hkUnaryActionCinfo {
public:
    // The serialized form (bhkSerialized.h): the 32-bit layout, 0x40 bytes.
    struct Serialized : hkUnaryActionCinfo::Serialized {
        uint8_t padding08[8];
        hkVector4 HingeAxisLS;
        hkVector4 ForwardLS;
        float fStrength;
        float fDamping;
        uint8_t padding38[8];
    };

    hkOrientHingedBodyActionCinfo();

    // Fills the info from its serialized form; the user data and the body are left empty.
    void ReadSerialized(const Serialized& arForm) {
        hkUnaryActionCinfo::ReadSerialized(arForm);
        HingeAxisLS = arForm.HingeAxisLS;
        ForwardLS = arForm.ForwardLS;
        fStrength = arForm.fStrength;
        fDamping = arForm.fDamping;
    }

    hkVector4 HingeAxisLS;
    hkVector4 ForwardLS;
    float fStrength;
    float fDamping;
};

static_assert(offsetof(hkOrientHingedBodyActionCinfo::Serialized, HingeAxisLS) == 0x10);
static_assert(offsetof(hkOrientHingedBodyActionCinfo::Serialized, ForwardLS) == 0x20);
static_assert(offsetof(hkOrientHingedBodyActionCinfo::Serialized, fDamping) == 0x34);
static_assert(sizeof(hkOrientHingedBodyActionCinfo::Serialized) == 0x40);

inline void bhkOrientHingedBodyAction::SetEnabled(bool abEnable) {
    (**this)->SetEnabled(abEnable);
}
