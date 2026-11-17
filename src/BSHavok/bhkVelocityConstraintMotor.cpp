#include "BSHavok/bhkVelocityConstraintMotor.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkLimitedForceConstraintMotor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

NiRTTI bhkVelocityConstraintMotor::ms_RTTI("bhkVelocityConstraintMotor", nullptr);

// hkpVelocityConstraintMotor's constructor sets the motor's defaults; this class adds nothing.
bhkVelocityConstraintMotor::bhkVelocityConstraintMotor() {}

// The exe's slot 0 is hkBaseObject's deleting destructor: nothing of this class's own to undo.
bhkVelocityConstraintMotor::~bhkVelocityConstraintMotor() {}

const NiRTTI* bhkVelocityConstraintMotor::GetRTTI() const {
	return &bhkVelocityConstraintMotor::ms_RTTI;
}

// The motor is none of the Gamebryo types the queries below ask for.
NiNode* bhkVelocityConstraintMotor::IsNode() {
	return nullptr;
}

BSFadeNode* bhkVelocityConstraintMotor::IsFadeNode() {
	return nullptr;
}

BSMultiBoundNode* bhkVelocityConstraintMotor::IsMultiBoundNode() {
	return nullptr;
}

NiGeometry* bhkVelocityConstraintMotor::IsGeometry() {
	return nullptr;
}

NiTriBasedGeom* bhkVelocityConstraintMotor::IsTriBasedGeom() {
	return nullptr;
}

NiTriStrips* bhkVelocityConstraintMotor::IsTriStrips() {
	return nullptr;
}

NiTriShape* bhkVelocityConstraintMotor::IsTriShape() {
	return nullptr;
}

BSSegmentedTriShape* bhkVelocityConstraintMotor::IsSegmentedTriShape() {
	return nullptr;
}

BSResizableTriShape* bhkVelocityConstraintMotor::IsResizableTriShape() {
	return nullptr;
}

NiParticles* bhkVelocityConstraintMotor::IsParticlesGeom() {
	return nullptr;
}

NiLines* bhkVelocityConstraintMotor::IsLinesGeom() {
	return nullptr;
}

bhkNiCollisionObject* bhkVelocityConstraintMotor::IsBhkNiCollisionObject() {
	return nullptr;
}

bhkBlendCollisionObject* bhkVelocityConstraintMotor::IsBhkBlendCollisionObject() {
	return nullptr;
}

bhkRigidBody* bhkVelocityConstraintMotor::IsBhkRigidBody() {
	return nullptr;
}

bhkLimitedHingeConstraint* bhkVelocityConstraintMotor::IsBhkLimitedHingeConstraint() {
	return nullptr;
}

// The class name as one viewer line; the base's lines are not added. The scope is charged to the Havok context; the
// exe's cleanup-only C++ EH frame is left out.
void bhkVelocityConstraintMotor::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	char* pcString = NiGetViewerString(ms_RTTI.GetName());
	pkStrings->Add(pcString);
}

void bhkVelocityConstraintMotor::LoadBinary(NiStream& arStream) {
	uint32_t v10;
	NiStream* v2 = &arStream;
	bhkLimitedForceConstraintMotor::LoadBinary(arStream, *this);
	NiBinaryStream* m_pkIstr = v2->Istr();
	uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
	v10 = 4;
	m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &m_tau, 4, &v10, 1);
	NiBinaryStream* v6 = v2->Istr();
	uint32_t (*v7)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v6->m_pfnRead;
	v10 = 4;
	v6->m_uiAbsoluteCurrentPos += v7(v6, &m_velocityTarget, 4, &v10, 1);
	NiBinaryStream* v8 = v2->Istr();
	uint32_t (*v9)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v8->m_pfnRead;
	v10 = 1;
	v8->m_uiAbsoluteCurrentPos += v9(v8, &m_useVelocityTargetFromConstraintTargets, 1, &v10, 1);
}

// The base's force limits, then tau and the velocity target (4-byte components) and the 1-byte flag. The exe reloads
// the stream before each write and keeps the component size in its dead stream-argument slot.
void bhkVelocityConstraintMotor::SaveBinary(NiStream& arStream) {
	bhkLimitedForceConstraintMotor::SaveBinary(arStream, *this);
	uint32_t uiComponentSize = 4;
	arStream.m_pkOstr->BinaryWrite(&m_tau, 4, &uiComponentSize, 1);
	uiComponentSize = 4;
	arStream.m_pkOstr->BinaryWrite(&m_velocityTarget, 4, &uiComponentSize, 1);
	uiComponentSize = 1;
	arStream.m_pkOstr->BinaryWrite(&m_useVelocityTargetFromConstraintTargets, 1, &uiComponentSize, 1);
}
