#pragma once

#include "BSHavok/bhkBinaryAction.h"
#include "Fallout Shared/GlobalFunc.h"

// The creation info of a dashpot action: the binary action's bodies, the two pivot points and the dashpot's
// strength and damping.
struct hkDashpotActionCinfo : public hkBinaryActionCinfo {
    hkDashpotActionCinfo();

    bhkVector4 m_pivotA;
    bhkVector4 m_pivotB;
    float m_strength;
    float m_damping;
};

// An action that damps the relative motion of two points on two bodies.
class bhkDashpotAction : public bhkBinaryAction {
public:
    bhkDashpotAction();
    ~bhkDashpotAction() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    uint32_t QCinfoSize() const override;
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void KillCinfo(bool abCreated) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;

    // bhkBinaryAction::CopyCinfo, then the wrapped Havok action's pivots, strength and damping into arCinfo.
    // It hides bhkBinaryAction::CopyCinfo.
    void CopyCinfo(hkDashpotActionCinfo& arInfo);

    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    // Number of live dashpot actions.
    static inline uint32_t iCount;
};
