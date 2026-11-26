#pragma once

#include "BSHavok/bhkAction.h"

class hkpRigidBody;
struct hkUnaryActionCinfo;

// A Havok action that acts on one entity; adds no data to bhkAction.
class bhkUnaryAction : public bhkAction {
public:
    bhkUnaryAction();
    ~bhkUnaryAction() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    bool Add(bhkWorld* apWorld) override;

    // Runs the action's entity through apWorld.
    void ProcessEntity(bhkWorld* apWorld);
    void CopyMembers(bhkSerializable* pDest, NiCloningProcess& arCloneData);
    // Fills arInfo from the wrapped action, as bhkAction::CopyCinfo does for the user data.
    void CopyCinfo(hkUnaryActionCinfo& arInfo);

    static NiRTTI ms_RTTI;
    static inline uint32_t iCount;
};

// The creation info every unary action's info starts with: the action's user data and the entity it acts on.
struct hkUnaryActionCinfo : public hkActionCinfo {
    // The serialized form (bhkSerialized.h): the 32-bit layout, 8 bytes.
    struct Serialized : hkActionCinfo::Serialized {
        bhkSerializedPointer m_body;
    };

    // Fills this part from its serialized form; the body is left null (LinkObject binds it).
    void ReadSerialized(const Serialized& arForm) {
        hkActionCinfo::ReadSerialized(arForm);
        m_body = nullptr;
    }

    hkpRigidBody* m_body;
};

static_assert(offsetof(hkUnaryActionCinfo::Serialized, m_body) == 0x4);
static_assert(sizeof(hkUnaryActionCinfo::Serialized) == 0x8);

inline bhkUnaryAction::bhkUnaryAction() {
    ++iCount;
}
