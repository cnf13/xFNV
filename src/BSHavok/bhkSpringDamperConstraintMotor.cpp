#include "BSHavok/bhkSpringDamperConstraintMotor.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkLimitedForceConstraintMotor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"

NiRTTI bhkSpringDamperConstraintMotor::ms_RTTI("bhkSpringDamperConstraintMotor", nullptr);

bhkSpringDamperConstraintMotor::bhkSpringDamperConstraintMotor() {}

// This class adds nothing to destroy.
bhkSpringDamperConstraintMotor::~bhkSpringDamperConstraintMotor() {}

const NiRTTI* bhkSpringDamperConstraintMotor::GetRTTI() const {
    return &bhkSpringDamperConstraintMotor::ms_RTTI;
}

NiNode* bhkSpringDamperConstraintMotor::IsNode() {
    return nullptr;
}

BSFadeNode* bhkSpringDamperConstraintMotor::IsFadeNode() {
    return nullptr;
}

BSMultiBoundNode* bhkSpringDamperConstraintMotor::IsMultiBoundNode() {
    return nullptr;
}

NiGeometry* bhkSpringDamperConstraintMotor::IsGeometry() {
    return nullptr;
}

NiTriBasedGeom* bhkSpringDamperConstraintMotor::IsTriBasedGeom() {
    return nullptr;
}

NiTriStrips* bhkSpringDamperConstraintMotor::IsTriStrips() {
    return nullptr;
}

NiTriShape* bhkSpringDamperConstraintMotor::IsTriShape() {
    return nullptr;
}

BSSegmentedTriShape* bhkSpringDamperConstraintMotor::IsSegmentedTriShape() {
    return nullptr;
}

BSResizableTriShape* bhkSpringDamperConstraintMotor::IsResizableTriShape() {
    return nullptr;
}

NiParticles* bhkSpringDamperConstraintMotor::IsParticlesGeom() {
    return nullptr;
}

NiLines* bhkSpringDamperConstraintMotor::IsLinesGeom() {
    return nullptr;
}

bhkNiCollisionObject* bhkSpringDamperConstraintMotor::IsBhkNiCollisionObject() {
    return nullptr;
}

bhkBlendCollisionObject* bhkSpringDamperConstraintMotor::IsBhkBlendCollisionObject() {
    return nullptr;
}

bhkRigidBody* bhkSpringDamperConstraintMotor::IsBhkRigidBody() {
    return nullptr;
}

bhkLimitedHingeConstraint* bhkSpringDamperConstraintMotor::IsBhkLimitedHingeConstraint() {
    return nullptr;
}

// Appends the class name's viewer string, in Havok's heap context; the base's GetViewerStrings is not called.
void bhkSpringDamperConstraintMotor::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    char* pViewerString = NiGetViewerString(bhkSpringDamperConstraintMotor::ms_RTTI.GetName());
    pkStrings->Add(pViewerString);
}

void bhkSpringDamperConstraintMotor::LoadBinary(NiStream& arStream) {
    uint32_t v8;
    NiStream* v2 = &arStream;
    bhkLimitedForceConstraintMotor::LoadBinary(arStream, *this);
    NiBinaryStream* m_pkIstr = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v8 = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &m_springConstant, 4, &v8, 1);
    NiBinaryStream* v6 = v2->Istr();
    uint32_t (*v7)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v6->m_pfnRead;
    v8 = 4;
    v6->m_uiAbsoluteCurrentPos += v7(v6, &m_springDamping, 4, &v8, 1);
}

// The base's force limits, then the spring constant and damping, each one 4-byte component.
void bhkSpringDamperConstraintMotor::SaveBinary(NiStream& arStream) {
    bhkLimitedForceConstraintMotor::SaveBinary(arStream, *this);
    uint32_t uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&m_springConstant, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&m_springDamping, 4, &uiComponentSize, 1);
}
