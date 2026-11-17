#include "Gamebryo2.2/CoreLibs/NiAnimation/BSRotAccumTransfInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"

NiRTTI BSRotAccumTransfInterpolator::ms_RTTI("BSRotAccumTransfInterpolator", &NiTransformInterpolator::ms_RTTI);

BSRotAccumTransfInterpolator::BSRotAccumTransfInterpolator()
	: NiTransformInterpolator(static_cast<NiTransformData*>(nullptr)), PrevKeyRot(NiQuaternion_IDENTITY) {}

// m_spData is released by NiTransformInterpolator's member destructor.
BSRotAccumTransfInterpolator::~BSRotAccumTransfInterpolator() {}

NiObject* BSRotAccumTransfInterpolator::CreateClone(NiCloningProcess& kCloning) {
	BSRotAccumTransfInterpolator* pkObject =
		static_cast<BSRotAccumTransfInterpolator*>(NiMemObject::operator new(sizeof(BSRotAccumTransfInterpolator)));
	BSRotAccumTransfInterpolator* pkObject_2 = pkObject;
	if (pkObject) {
		::new (pkObject) BSRotAccumTransfInterpolator;
	} else {
		pkObject_2 = 0;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->PrevKeyRot = PrevKeyRot;
	return pkObject_2;
}

// CreateClone (0xA43E90) is left with the exe: it constructs the clone, and the constructor is not reconstructed.
// The four stream and compare slots are jumps to the base bodies; nothing of this class is streamed or compared.
void BSRotAccumTransfInterpolator::LoadBinary(NiStream& arStream) {
	NiTransformInterpolator::LoadBinary(arStream);
}

// Starts accumulating from the first rotation key.
void BSRotAccumTransfInterpolator::LinkObject(NiStream& arStream) {
	NiTransformInterpolator::LinkObject(arStream);

	if (m_spData && m_spData->m_usNumRotKeys)
		PrevKeyRot = m_spData->m_pkRotKeys->m_quat;
}

bool BSRotAccumTransfInterpolator::RegisterStreamables(NiStream& arStream) {
	return NiTransformInterpolator::RegisterStreamables(arStream);
}

void BSRotAccumTransfInterpolator::SaveBinary(NiStream& arStream) {
	NiTransformInterpolator::SaveBinary(arStream);
}

bool BSRotAccumTransfInterpolator::IsEqual(NiObject* apObject) {
	return NiTransformInterpolator::IsEqual(apObject);
}

// NiTransformInterpolator::Update, except for the rotation when there is a target: the change between the previous
// and the current key rotation is applied on top of the target's current local rotation, instead of replacing it.
bool BSRotAccumTransfInterpolator::Update(float afTime, NiObjectNET* apInterpTarget, NiQuatTransform& arValue) {
	if (bPose) {
		m_fLastTime = afTime;
		arValue = m_kTransformValue;
		return true;
	}

	if (afTime == m_fLastTime) {
		arValue = m_kTransformValue;
		return !arValue.IsTransformInvalid();
	}

	uint32_t uiNumKeys;
	NiAnimationKey::KeyType eType;
	uint8_t ucSize;

	NiPosKey* pkPosKeys = GetPosData(uiNumKeys, eType, ucSize);
	if (uiNumKeys > 0) {
		uint32_t uiLastIdx = m_usLastTransIdx;
		m_kTransformValue.m_kTranslate = NiPosKey::GenInterp(afTime, pkPosKeys, eType, uiNumKeys, uiLastIdx, ucSize);
		m_usLastTransIdx = static_cast<uint16_t>(uiLastIdx);
	}

	NiRotKey* pkRotKeys = GetRotData(uiNumKeys, eType, ucSize);
	if (uiNumKeys > 0) {
		uint32_t uiLastIdx = m_usLastRotIdx;
		NiQuaternion kRot = NiRotKey::GenInterp(afTime, pkRotKeys, eType, uiNumKeys, uiLastIdx, ucSize);
		if (apInterpTarget) {
			NiQuaternion kDelta = kRot * NiQuaternion::UnitInverse(PrevKeyRot);
			NiQuaternion kLocal;
			kLocal.FromRotation(static_cast<NiAVObject*>(apInterpTarget)->m_kLocal.m_Rotate);
			kLocal = kLocal * kDelta;
			kLocal.Normalize();
			m_kTransformValue.m_kRotate = kLocal;
		} else {
			m_kTransformValue.m_kRotate = kRot;
		}
		PrevKeyRot = kRot;
		m_usLastRotIdx = static_cast<uint16_t>(uiLastIdx);
	}

	NiFloatKey* pkScaleKeys = GetScaleData(uiNumKeys, eType, ucSize);
	if (uiNumKeys > 0) {
		uint32_t uiLastIdx = m_usLastScaleIdx;
		m_kTransformValue.m_fScale = NiFloatKey::GenInterp(afTime, pkScaleKeys, eType, uiNumKeys, uiLastIdx, ucSize);
		m_usLastScaleIdx = static_cast<uint16_t>(uiLastIdx);
	}

	arValue = m_kTransformValue;
	if (arValue.IsTransformInvalid())
		return false;

	m_fLastTime = afTime;
	return true;
}

NiObject* BSRotAccumTransfInterpolator::CreateObject() {
	BSRotAccumTransfInterpolator* pkObject =
		static_cast<BSRotAccumTransfInterpolator*>(NiMemObject::operator new(sizeof(BSRotAccumTransfInterpolator)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) BSRotAccumTransfInterpolator;
}
