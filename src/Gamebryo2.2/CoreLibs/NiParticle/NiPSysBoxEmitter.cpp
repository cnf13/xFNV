#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysBoxEmitter.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVolumeEmitter.h"

NiRTTI NiPSysBoxEmitter::ms_RTTI("NiPSysBoxEmitter", &NiPSysVolumeEmitter::ms_RTTI);

NiPSysBoxEmitter::NiPSysBoxEmitter()
	: NiPSysVolumeEmitter(), m_fEmitterWidth(0.0f), m_fEmitterHeight(0.0f), m_fEmitterDepth(0.0f) {}

NiPSysBoxEmitter::~NiPSysBoxEmitter() {}

NiObject* NiPSysBoxEmitter::CreateClone(NiCloningProcess& kCloning) {
	NiPSysBoxEmitter* pkObject = static_cast<NiPSysBoxEmitter*>(NiMemObject::operator new(sizeof(NiPSysBoxEmitter)));
	NiPSysBoxEmitter* pkObject_2 = pkObject;
	if (pkObject) {
		::new (pkObject) NiPSysBoxEmitter;
	} else {
		pkObject_2 = 0;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->m_fEmitterWidth = m_fEmitterWidth;
	pkObject_2->m_fEmitterHeight = m_fEmitterHeight;
	pkObject_2->m_fEmitterDepth = m_fEmitterDepth;
	return pkObject_2;
}

void NiPSysBoxEmitter::LoadBinary(NiStream& kStream) {
	NiPSysVolumeEmitter::LoadBinary(kStream);

	NiStreamLoadBinary(kStream, m_fEmitterWidth);
	NiStreamLoadBinary(kStream, m_fEmitterHeight);
	NiStreamLoadBinary(kStream, m_fEmitterDepth);
}

void NiPSysBoxEmitter::LinkObject(NiStream& kStream) {
	NiPSysVolumeEmitter::LinkObject(kStream);
}

bool NiPSysBoxEmitter::RegisterStreamables(NiStream& kStream) {
	return NiPSysVolumeEmitter::RegisterStreamables(kStream);
}

void NiPSysBoxEmitter::SaveBinary(NiStream& kStream) {
	NiPSysVolumeEmitter::SaveBinary(kStream);

	NiStreamSaveBinary(kStream, m_fEmitterWidth);
	NiStreamSaveBinary(kStream, m_fEmitterHeight);
	NiStreamSaveBinary(kStream, m_fEmitterDepth);
}

bool NiPSysBoxEmitter::IsEqual(NiObject* pkObject) {
	if (!NiPSysVolumeEmitter::IsEqual(pkObject)) {
		return false;
	}

	NiPSysBoxEmitter* pkOther = static_cast<NiPSysBoxEmitter*>(pkObject);
	if (m_fEmitterWidth != pkOther->m_fEmitterWidth) {
		return false;
	}
	if (m_fEmitterHeight != pkOther->m_fEmitterHeight) {
		return false;
	}
	if (m_fEmitterDepth != pkOther->m_fEmitterDepth) {
		return false;
	}
	return true;
}

void NiPSysBoxEmitter::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysVolumeEmitter::GetViewerStrings(pkStrings);

	pkStrings->Add(NiGetViewerString(NiPSysBoxEmitter::ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("Width", m_fEmitterWidth));
	pkStrings->Add(NiGetViewerString("Height", m_fEmitterHeight));
	pkStrings->Add(NiGetViewerString("Depth", m_fEmitterDepth));
}

void NiPSysBoxEmitter::ProcessClone(NiCloningProcess& kCloning) {
	NiPSysVolumeEmitter::ProcessClone(kCloning);
}

// The three randoms are drawn before any is used: the first sets the depth, the last the width, matching the game's
// right-to-left evaluation of the NiPoint3 constructor's arguments.
void NiPSysBoxEmitter::ComputeVolumeInitialPositionAndVelocity(NiTransform& kEmitterToPSys, NiPoint3& kPosition,
	NiPoint3& kVelocity) {
	float fDepthRandom = NiUnitRandom();
	float fHeightRandom = NiUnitRandom();
	float fWidthRandom = NiUnitRandom();
	kPosition.x = static_cast<float>((fWidthRandom - 0.5) * m_fEmitterWidth);
	kPosition.y = static_cast<float>((fHeightRandom - 0.5) * m_fEmitterHeight);
	kPosition.z = static_cast<float>((fDepthRandom - 0.5) * m_fEmitterDepth);

	kPosition = kEmitterToPSys * kPosition;
	const NiMatrix3& kRotate = kEmitterToPSys.m_Rotate;
	float fX = kVelocity.x;
	float fY = kVelocity.y;
	float fZ = kVelocity.z;
	kVelocity.x = kRotate.m_pEntry[0][0] * fX + kRotate.m_pEntry[0][1] * fY + kRotate.m_pEntry[0][2] * fZ;
	kVelocity.y = kRotate.m_pEntry[1][0] * fX + kRotate.m_pEntry[1][1] * fY + kRotate.m_pEntry[1][2] * fZ;
	kVelocity.z = kRotate.m_pEntry[2][0] * fX + kRotate.m_pEntry[2][1] * fY + kRotate.m_pEntry[2][2] * fZ;
}

NiObject* NiPSysBoxEmitter::CreateObject() {
	NiPSysBoxEmitter* pkObject = static_cast<NiPSysBoxEmitter*>(NiMemObject::operator new(sizeof(NiPSysBoxEmitter)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiPSysBoxEmitter;
}
