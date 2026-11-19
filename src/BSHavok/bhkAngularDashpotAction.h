#pragma once

#include "BSHavok/bhkBinaryAction.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"

struct hkAngularDashpotActionCinfo;

// An action that damps the relative rotation of two bodies.
class bhkAngularDashpotAction : public bhkBinaryAction {
public:
    bhkAngularDashpotAction();
    ~bhkAngularDashpotAction() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    uint32_t QCinfoSize() const override;
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // Fills apInfo from the wrapped hkpAngularDashpotAction.
    void CopyCinfo(hkAngularDashpotActionCinfo& arInfo);

    // The wrapped action's rotation, or the identity quaternion when there is no action.
    const hkQuaternion& GethkRotation() const {
        return (this && phkObject) ? static_cast<hkpAngularDashpotAction*>(phkObject)->m_rotation
                                   : hkQuaternionIdentity;
    }

    // The class loader's factory: returns a new action.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    // Live actions, counted by the constructors.
    static inline uint32_t iCount;
};

// The creation info of an angular dashpot action. Its constructor also stores the identity quaternion.
struct hkAngularDashpotActionCinfo : public hkBinaryActionCinfo {
    hkAngularDashpotActionCinfo();

    hkQuaternion m_rotation;
    float m_strength;
    float m_damping;
};
