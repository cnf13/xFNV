#include "BSHavok/bhkPositionConstraintMotor.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkLimitedForceConstraintMotor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

NiRTTI bhkPositionConstraintMotor::ms_RTTI("bhkPositionConstraintMotor", nullptr);

// Builds the motor through Havok's constructor with a current position of 0.
bhkPositionConstraintMotor::bhkPositionConstraintMotor() : hkpPositionConstraintMotor(0.0f) {}

// The class adds nothing to destroy.
bhkPositionConstraintMotor::~bhkPositionConstraintMotor() {}

const NiRTTI* bhkPositionConstraintMotor::GetRTTI() const {
    return &bhkPositionConstraintMotor::ms_RTTI;
}

// The type queries: a position motor is none of these Gamebryo or Bethesda classes.
NiNode* bhkPositionConstraintMotor::IsNode() {
    return nullptr;
}

BSFadeNode* bhkPositionConstraintMotor::IsFadeNode() {
    return nullptr;
}

BSMultiBoundNode* bhkPositionConstraintMotor::IsMultiBoundNode() {
    return nullptr;
}

NiGeometry* bhkPositionConstraintMotor::IsGeometry() {
    return nullptr;
}

NiTriBasedGeom* bhkPositionConstraintMotor::IsTriBasedGeom() {
    return nullptr;
}

NiTriStrips* bhkPositionConstraintMotor::IsTriStrips() {
    return nullptr;
}

NiTriShape* bhkPositionConstraintMotor::IsTriShape() {
    return nullptr;
}

BSSegmentedTriShape* bhkPositionConstraintMotor::IsSegmentedTriShape() {
    return nullptr;
}

BSResizableTriShape* bhkPositionConstraintMotor::IsResizableTriShape() {
    return nullptr;
}

NiParticles* bhkPositionConstraintMotor::IsParticlesGeom() {
    return nullptr;
}

NiLines* bhkPositionConstraintMotor::IsLinesGeom() {
    return nullptr;
}

bhkNiCollisionObject* bhkPositionConstraintMotor::IsBhkNiCollisionObject() {
    return nullptr;
}

bhkBlendCollisionObject* bhkPositionConstraintMotor::IsBhkBlendCollisionObject() {
    return nullptr;
}

bhkRigidBody* bhkPositionConstraintMotor::IsBhkRigidBody() {
    return nullptr;
}

bhkLimitedHingeConstraint* bhkPositionConstraintMotor::IsBhkLimitedHingeConstraint() {
    return nullptr;
}

// The class name as one viewer line; the base's lines are not added. The scope is charged to the Havok context.
void bhkPositionConstraintMotor::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    pkStrings->Add(pcString);
}

// The base's force limits, then the motor's four floats, each one 4-byte component of the load stream.
void bhkPositionConstraintMotor::LoadBinary(NiStream& arStream) {
    bhkLimitedForceConstraintMotor::LoadBinary(arStream, *this);
    uint32_t uiComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&m_tau, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&m_damping, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&m_proportionalRecoveryVelocity, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&m_constantRecoveryVelocity, 4, &uiComponentSize, 1);
}

// The base's force limits, then the motor's four floats, each one 4-byte component of the save stream.
void bhkPositionConstraintMotor::SaveBinary(NiStream& arStream) {
    bhkLimitedForceConstraintMotor::SaveBinary(arStream, *this);
    uint32_t uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&m_tau, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&m_damping, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&m_proportionalRecoveryVelocity, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&m_constantRecoveryVelocity, 4, &uiComponentSize, 1);
}
