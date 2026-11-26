#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <stdint.h>

class hkConstraintCinfo;
class NiCloningProcess;

// One ragdoll template: its name, the constraint infos with their types, and the rigid body settings (mass,
// restitution, friction, radius, material).
class bhkRagdollTemplateData : public NiObject {
public:
    bhkRagdollTemplateData();
    ~bhkRagdollTemplateData() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;

    // Gives apObj this template's rigid body settings, a new capsule shape and new constraints at afScale.
    void ReplaceRagdollData(NiAVObject* apObj, float afScale);
    const NiFixedString& GetName() const;

    // The stream's create function for this class.
    static NiObject* CreateObject();

    NiFixedString ObjectName;
    BSSimpleArray<hkConstraintCinfo*, 1024> ConstraintsA;
    BSSimpleArray<hkpConstraintData::ConstraintType, 1024> ConstraintTypeA;
    float fRBMass;
    float fRBRestitution;
    float fRBFriction;
    float fRBRadius;
    uint32_t uiRBMaterial;

    static NiRTTI ms_RTTI;
};

// Extra data on a skeleton node: the ragdoll templates (bhkRagdollTemplateData) the model offers.
class bhkRagdollTemplate : public NiExtraData {
public:
    bhkRagdollTemplate();
    ~bhkRagdollTemplate() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    virtual void CopyMembers(bhkRagdollTemplate* pDest, NiCloningProcess& arCloneData);

    using NiExtraData::CopyMembers;

    // The template data at aiIndex.
    bhkRagdollTemplateData* GetRagdollTemplateData(uint32_t aiIndex);
    // The number of templates.
    uint32_t QNumTemplates();

    // The stream's create function for this class.
    static NiObject* CreateObject();

    BSSimpleArray<NiPointer<bhkRagdollTemplateData>, 1024> RagdollTemplateA;

    static NiRTTI ms_RTTI;
};

inline bhkRagdollTemplateData* bhkRagdollTemplate::GetRagdollTemplateData(uint32_t aiIndex) {
    return RagdollTemplateA[aiIndex];
}
