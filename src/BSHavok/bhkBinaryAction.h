#pragma once

#include "BSHavok/bhkAction.h"

class bhkRigidBody;
struct hkBinaryActionCinfo;
class hkpRigidBody;
class NiCloningProcess;

// A Havok action between two rigid bodies; it adds no data of its own. SetRigidBodyA, SetRigidBodyB and CopyMembers are
// new virtuals, in this order.
class bhkBinaryAction : public bhkAction {
public:
    bhkBinaryAction() {
        ++iCount;
    }

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    bool Add(bhkWorld* apWorld) override;
    bool Remove() override;
    virtual void SetRigidBodyA(bhkRigidBody* apBody);
    virtual void SetRigidBodyB(bhkRigidBody* apBody);
    virtual void CopyMembers(bhkSerializable* pDest, NiCloningProcess& arCloneData);
    ~bhkBinaryAction() override;

    bhkRigidBody* GetRigidBody(char acBody);
    bhkRigidBody* GetRigidBodyB();
    // bhkAction::CopyCinfo, then the wrapped action's two bodies (+0x18, +0x1C) into m_bodyA and m_bodyB, or nulls
    // without an action. Not virtual: the derived actions' CopyCinfo call it qualified,
    // as bhkUnaryAction's calls bhkAction's.
    void CopyCinfo(hkBinaryActionCinfo& arInfo);

    static NiRTTI ms_RTTI;
    static inline uint32_t iCount;
};

// The creation info every binary action's starts with: hkActionCinfo's user data and the two bodies. It is put here for
// its other user, the angular dashpot.
struct hkBinaryActionCinfo : public hkActionCinfo {
    hkpRigidBody* m_bodyA;
    hkpRigidBody* m_bodyB;
};
