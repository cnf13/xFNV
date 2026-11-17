#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cmath>

NiRTTI NiBillboardNode::ms_RTTI("NiBillboardNode", &NiNode::ms_RTTI);

NiBillboardNode::NiBillboardNode() : NiNode(0), m_uFlags(0), m_fSavedTime(0.0f) {
	SetMode(ROTATE_ABOUT_UP);
	SetUpdateControllers(true);
}

const NiRTTI* NiBillboardNode::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* NiBillboardNode::CreateClone(NiCloningProcess& kCloning) {
	NiBillboardNode* pkObject = static_cast<NiBillboardNode*>(NiMemObject::operator new(sizeof(NiBillboardNode)));
	NiBillboardNode* pkObject_2 = pkObject;
	if (pkObject) {
		::new (pkObject) NiBillboardNode;
	} else {
		pkObject_2 = 0;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->NiBillboardNode::m_uFlags = NiBillboardNode::m_uFlags;
	return pkObject_2;
}

// Before 10.0.1.2 the face mode sat in the NiAVObject flags; before 4.2.0.3 those flags had one bit fewer below it.
// A loaded node always updates its controllers.
void NiBillboardNode::LoadBinary(NiStream& kStream) {
	NiNode::LoadBinary(kStream);
	if (kStream.m_uiNifFileVersion >= 0xA000102) {
		NiStreamLoadBinary(kStream, m_uFlags);
		m_uFlags |= UPDATECONTROLLERS_MASK;
	} else {
		uint16_t usFlags = kStream.m_usNiAVObjectFlags;
		m_uFlags = usFlags;
		if (kStream.m_uiNifFileVersion < 0x4020003) {
			m_uFlags = uint16_t((usFlags & 0x1FFF) | ((usFlags & 0xE000) * 2));
		}
		m_uFlags >>= 6;
		m_uFlags |= UPDATECONTROLLERS_MASK;
	}
}

void NiBillboardNode::LinkObject(NiStream& kStream) {
	NiNode::LinkObject(kStream);
}

bool NiBillboardNode::RegisterStreamables(NiStream& kStream) {
	return NiNode::RegisterStreamables(kStream);
}

void NiBillboardNode::SaveBinary(NiStream& kStream) {
	NiNode::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_uFlags);
}

bool NiBillboardNode::IsEqual(NiObject* pkObject) {
	if (!NiNode::IsEqual(pkObject)) {
		return false;
	}
	return ((m_uFlags ^ static_cast<NiBillboardNode*>(pkObject)->m_uFlags) & FACE_MODE_MASK) == 0;
}

void NiBillboardNode::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiNode::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(GetViewerString("m_eMode", FaceMode(m_uFlags & FACE_MODE_MASK)));
}

void NiBillboardNode::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
	m_fSavedTime = UpdateData.fTime;
	if (UpdateData.bUpdateControllers) {
		m_uFlags |= UPDATECONTROLLERS_MASK;
	} else {
		m_uFlags &= ~UPDATECONTROLLERS_MASK;
	}
	NiNode::UpdateDownwardPass(UpdateData, uFlags);
	UpdateWorldBound();
}

void NiBillboardNode::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
	m_fSavedTime = UpdateData.fTime;
	NiNode::UpdateSelectedDownwardPass(UpdateData, uFlags);
	UpdateWorldBound();
}

void NiBillboardNode::UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
	m_fSavedTime = UpdateData.fTime;
	NiNode::UpdateRigidDownwardPass(UpdateData, uFlags);
	UpdateWorldBound();
}

// The node can face any way, so the bound is re-centred on the node and grown to cover every orientation.
void NiBillboardNode::UpdateWorldBound() {
	if (!m_kChildren.m_usESize) {
		return;
	}
	if (!m_kWorldBound) {
		SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(BSNiBound))));
		m_kWorldBound->m_kCenter = NiPoint3_ZERO;
		m_kWorldBound->m_fRadius = 0.0f;
	}
	NiBound* pkBound = m_kWorldBound;
	float fX = static_cast<float>(static_cast<double>(pkBound->m_kCenter.x) - m_kWorld.m_Translate.x);
	float fY = static_cast<float>(static_cast<double>(pkBound->m_kCenter.y) - m_kWorld.m_Translate.y);
	float fZ = static_cast<float>(static_cast<double>(pkBound->m_kCenter.z) - m_kWorld.m_Translate.z);
	float fRadius = pkBound->m_fRadius;
	pkBound->m_kCenter = m_kWorld.m_Translate;
	float fSum =
		static_cast<float>(static_cast<double>(fX) * fX + static_cast<double>(fY) * fY + static_cast<double>(fZ) * fZ);
	float fRoot = static_cast<float>(std::sqrt(fSum));
	pkBound->m_fRadius = static_cast<float>(static_cast<double>(fRoot) + fRadius);
}

void NiBillboardNode::OnVisible(NiCullingProcess& kCuller) {
	RotateToCamera(kCuller.m_pkCamera);
	NiNode::OnVisible(kCuller);
}

void NiBillboardNode::RotateToCamera(const NiCamera* pkCamera) {
	NiPoint3 kCamLoc__kCamD__kCamR;
	float v48;
	NiPoint3 kCamU;
	NiPoint3 kCamR__kCamD;
	NiPoint3 kCamU_2;
	float v52;
	NiMatrix3 kFaceMat;
	NiUpdateData v54;
	NiPoint3 kCamU_3;
	NiPoint3 kCamR_2;
	NiPoint3 kCamR;
	NiPoint3 kCamD_2;
	NiPoint3 kCamD;
	NiPoint3 kCamU_4;
	NiTransform kParentXform;
	NiTransform kDest;
	NiNode* m_pkParent = this->m_pkParent;
	if (m_pkParent) {
		kParentXform = m_pkParent->m_kWorld;
	} else {
		kParentXform.MakeIdentity();
	}
	NiTransform* v4 = ::new (&kDest) NiTransform(kParentXform * m_kLocal);
	uint16_t m_uFlags = this->NiBillboardNode::m_uFlags;
	NiTransform* p_m_kWorld = &m_kWorld;
	m_kWorld = *v4;
	int32_t v7 = m_uFlags & 7;
	kFaceMat = NiMatrix3_IDENTITY;
	switch (v7) {
	case 0:
	case 3: {
		double v20;
		if (v7 == 3) {
			if (!RotateToCenter(pkCamera, m_kWorld.m_Translate, kCamLoc__kCamD__kCamR, kCamU_2, kCamR__kCamD)) {
				break;
			}
		} else {
			kCamLoc__kCamD__kCamR.x = pkCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
			kCamLoc__kCamD__kCamR.y = pkCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
			kCamLoc__kCamD__kCamR.z = pkCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
			kCamU_2.x = -kCamLoc__kCamD__kCamR.x;
			kCamLoc__kCamD__kCamR.x = kCamU_2.x;
			kCamU_2.y = -kCamLoc__kCamD__kCamR.y;
			kCamLoc__kCamD__kCamR.y = kCamU_2.y;
			kCamU_2.z = -kCamLoc__kCamD__kCamR.z;
			double v8 = pkCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
			kCamLoc__kCamD__kCamR.z = kCamU_2.z;
			kCamR__kCamD.x = v8;
			double v9 = pkCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
			kCamU_2.x = kCamR__kCamD.x;
			kCamR__kCamD.y = v9;
			double v10 = pkCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
			kCamU_2.y = kCamR__kCamD.y;
			kCamR__kCamD.z = v10;
			double v11 = pkCamera->m_kWorld.m_Rotate.m_pEntry[0][2];
			kCamU_2.z = kCamR__kCamD.z;
			kCamU.x = v11;
			kCamU.y = pkCamera->m_kWorld.m_Rotate.m_pEntry[1][2];
			kCamU.z = pkCamera->m_kWorld.m_Rotate.m_pEntry[2][2];
			kCamR__kCamD = kCamU;
		}
		NiPoint3* v12 = ::new (&kCamD) NiPoint3(kCamLoc__kCamD__kCamR * m_kWorld.m_Rotate);
		float y = v12->y;
		kCamLoc__kCamD__kCamR.x = v12->x;
		float z = v12->z;
		kCamLoc__kCamD__kCamR.y = y;
		kCamLoc__kCamD__kCamR.z = z;
		NiPoint3* v15 = ::new (&kCamU_3) NiPoint3(kCamU_2 * m_kWorld.m_Rotate);
		float v16 = v15->y;
		kCamU_2.x = v15->x;
		float v17 = v15->z;
		kCamU_2.y = v16;
		kCamU_2.z = v17;
		NiPoint3* v18 = ::new (&kCamR) NiPoint3(kCamR__kCamD * m_kWorld.m_Rotate);
		float x = v18->x;
		kCamR__kCamD.y = v18->y;
		kCamR__kCamD.x = x;
		kCamR__kCamD.z = v18->z;
		v48 = kCamU_2.y * kCamU_2.y + kCamR__kCamD.y * kCamR__kCamD.y;
		v48 = std::sqrt((double)v48);
		if (!(v48 > 1e-06f)) {
			kCamU.x = -kCamR__kCamD.x;
			kCamU.y = -kCamR__kCamD.y;
			kCamU.z = -kCamR__kCamD.z;
			kFaceMat.m_pEntry[0][0] = kCamU.x;
			kFaceMat.m_pEntry[1][0] = kCamU.y;
			kFaceMat.m_pEntry[2][0] = kCamU.z;
			kCamU.x = -kCamU_2.x;
			kCamU.y = -kCamU_2.y;
			kCamU.z = -kCamU_2.z;
			kFaceMat.m_pEntry[0][1] = kCamU.x;
			kFaceMat.m_pEntry[1][1] = kCamU.y;
			v20 = kCamU.z;
		} else {
			v48 = 1.0 / v48;
			v52 = v48 * kCamU_2.y;
			v48 = v48 * -kCamR__kCamD.y;
			kFaceMat.m_pEntry[0][0] = kCamR__kCamD.x * v52 + kCamU_2.x * v48;
			kFaceMat.m_pEntry[1][0] = v48 * kCamU_2.y + v52 * kCamR__kCamD.y;
			kFaceMat.m_pEntry[2][0] = kCamU_2.z * v48 + kCamR__kCamD.z * v52;
			kFaceMat.m_pEntry[0][1] = kCamU_2.x * v52 - kCamR__kCamD.x * v48;
			kFaceMat.m_pEntry[1][1] = kCamU_2.y * v52 - kCamR__kCamD.y * v48;
			v20 = v52 * kCamU_2.z - v48 * kCamR__kCamD.z;
		}
		kFaceMat.m_pEntry[2][1] = v20;
		kFaceMat.m_pEntry[0][2] = kCamLoc__kCamD__kCamR.x;
		kFaceMat.m_pEntry[1][2] = kCamLoc__kCamD__kCamR.y;
		kFaceMat.m_pEntry[2][2] = kCamLoc__kCamD__kCamR.z;
		break;
	}
	case 1: {
		kCamU.x = pkCamera->m_kWorld.m_Translate.x - m_kWorld.m_Translate.x;
		kCamU.y = pkCamera->m_kWorld.m_Translate.y - m_kWorld.m_Translate.y;
		kCamU.z = pkCamera->m_kWorld.m_Translate.z - m_kWorld.m_Translate.z;
		float fScalar = m_kWorld.m_fScale;
		::new (&kCamLoc__kCamD__kCamR) NiPoint3((kCamU * m_kWorld.m_Rotate) / fScalar);
		v52 = kCamLoc__kCamD__kCamR.z * kCamLoc__kCamD__kCamR.z + kCamLoc__kCamD__kCamR.x * kCamLoc__kCamD__kCamR.x;
		v52 = std::sqrt((double)v52);
		v48 = v52;
		if (!(v52 < 1e-12f)) {
			v48 = 1.0 / v52;
			kCamLoc__kCamD__kCamR.x = kCamLoc__kCamD__kCamR.x * v48;
			kCamLoc__kCamD__kCamR.z = v48 * kCamLoc__kCamD__kCamR.z;
			kFaceMat.m_pEntry[0][0] = kCamLoc__kCamD__kCamR.z;
			kFaceMat.m_pEntry[1][0] = 0.0f;
			kFaceMat.m_pEntry[2][0] = -kCamLoc__kCamD__kCamR.x;
			kFaceMat.m_pEntry[0][1] = 0.0f;
			kFaceMat.m_pEntry[1][1] = 1.0f;
			kFaceMat.m_pEntry[2][1] = 0.0f;
			kFaceMat.m_pEntry[1][2] = 0.0f;
			kFaceMat.m_pEntry[0][2] = kCamLoc__kCamD__kCamR.x;
			kFaceMat.m_pEntry[2][2] = kCamLoc__kCamD__kCamR.z;
		}
		break;
	}
	case 2:
	case 4: {
		if (v7 == 4) {
			if (!RotateToCenter(pkCamera, m_kWorld.m_Translate, kCamR__kCamD, kCamU_2, kCamLoc__kCamD__kCamR)) {
				break;
			}
		} else {
			kCamU.x = pkCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
			kCamU.y = pkCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
			kCamU.z = pkCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
			kCamLoc__kCamD__kCamR.x = -kCamU.x;
			kCamR__kCamD.x = kCamLoc__kCamD__kCamR.x;
			kCamLoc__kCamD__kCamR.y = -kCamU.y;
			kCamR__kCamD.y = kCamLoc__kCamD__kCamR.y;
			kCamLoc__kCamD__kCamR.z = -kCamU.z;
			double v22 = pkCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
			kCamR__kCamD.z = kCamLoc__kCamD__kCamR.z;
			kCamU.x = v22;
			double v23 = pkCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
			kCamU_2.x = kCamU.x;
			kCamU.y = v23;
			double v24 = pkCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
			kCamU_2.y = kCamU.y;
			kCamU.z = v24;
			double v25 = pkCamera->m_kWorld.m_Rotate.m_pEntry[0][2];
			kCamU_2.z = kCamU.z;
			kCamU.x = v25;
			double v26 = pkCamera->m_kWorld.m_Rotate.m_pEntry[1][2];
			kCamLoc__kCamD__kCamR.x = kCamU.x;
			kCamU.y = v26;
			kCamU.z = pkCamera->m_kWorld.m_Rotate.m_pEntry[2][2];
			kCamLoc__kCamD__kCamR.y = kCamU.y;
			kCamLoc__kCamD__kCamR.z = kCamU.z;
		}
		NiPoint3* v27 = ::new (&kCamD_2) NiPoint3(kCamR__kCamD * m_kWorld.m_Rotate);
		float v28 = v27->x;
		float v29 = v27->y;
		float v30 = v27->z;
		kCamR__kCamD.x = v28;
		kCamR__kCamD.y = v29;
		kCamR__kCamD.z = v30;
		NiPoint3* v31 = ::new (&kCamU_4) NiPoint3(kCamU_2 * m_kWorld.m_Rotate);
		float v32 = v31->x;
		float v33 = v31->y;
		float v34 = v31->z;
		kCamU_2.x = v32;
		kCamU_2.y = v33;
		kCamU_2.z = v34;
		NiPoint3* v35 = ::new (&kCamR_2) NiPoint3(kCamLoc__kCamD__kCamR * m_kWorld.m_Rotate);
		float v36 = v35->x;
		float v37 = v35->y;
		float v38 = v35->z;
		kCamLoc__kCamD__kCamR.x = v36;
		kFaceMat.m_pEntry[0][0] = v36;
		kCamLoc__kCamD__kCamR.y = v37;
		kCamLoc__kCamD__kCamR.z = v38;
		kFaceMat.m_pEntry[1][0] = v37;
		kFaceMat.m_pEntry[2][0] = v38;
		kFaceMat.m_pEntry[0][1] = kCamU_2.x;
		kFaceMat.m_pEntry[1][1] = kCamU_2.y;
		kFaceMat.m_pEntry[2][1] = kCamU_2.z;
		kFaceMat.m_pEntry[0][2] = kCamR__kCamD.x;
		kFaceMat.m_pEntry[1][2] = kCamR__kCamD.y;
		kFaceMat.m_pEntry[2][2] = kCamR__kCamD.z;
		break;
	}
	case 5: {
		p_m_kWorld->m_Rotate.m_pEntry[0][0] = 1.0f;
		m_kWorld.m_Rotate.m_pEntry[1][0] = 0.0f;
		m_kWorld.m_Rotate.m_pEntry[2][0] = 0.0f;
		m_kWorld.m_Rotate.m_pEntry[0][1] = 0.0f;
		m_kWorld.m_Rotate.m_pEntry[2][1] = 0.0f;
		m_kWorld.m_Rotate.m_pEntry[1][1] = 1.0f;
		m_kWorld.m_Rotate.m_pEntry[0][2] = 0.0f;
		m_kWorld.m_Rotate.m_pEntry[1][2] = 0.0f;
		m_kWorld.m_Rotate.m_pEntry[2][2] = 1.0f;
		kCamU.x = pkCamera->m_kWorld.m_Translate.x - m_kWorld.m_Translate.x;
		kCamU.y = pkCamera->m_kWorld.m_Translate.y - m_kWorld.m_Translate.y;
		kCamU.z = pkCamera->m_kWorld.m_Translate.z - m_kWorld.m_Translate.z;
		float fScalara = m_kWorld.m_fScale;
		::new (&kCamLoc__kCamD__kCamR) NiPoint3((kCamU * m_kWorld.m_Rotate) / fScalara);
		v52 = kCamLoc__kCamD__kCamR.y * kCamLoc__kCamD__kCamR.y + kCamLoc__kCamD__kCamR.x * kCamLoc__kCamD__kCamR.x;
		v52 = std::sqrt((double)v52);
		v48 = v52;
		if (!(v52 < 1e-12f)) {
			v48 = 1.0 / v52;
			kCamLoc__kCamD__kCamR.x = kCamLoc__kCamD__kCamR.x * v48;
			kCamLoc__kCamD__kCamR.y = v48 * kCamLoc__kCamD__kCamR.y;
			kFaceMat.m_pEntry[0][0] = -kCamLoc__kCamD__kCamR.y;
			kFaceMat.m_pEntry[1][0] = kCamLoc__kCamD__kCamR.x;
			kFaceMat.m_pEntry[2][0] = 0.0f;
			kFaceMat.m_pEntry[2][1] = 0.0f;
			kFaceMat.m_pEntry[0][2] = 0.0f;
			kFaceMat.m_pEntry[1][2] = 0.0f;
			kFaceMat.m_pEntry[0][1] = kCamLoc__kCamD__kCamR.x;
			kFaceMat.m_pEntry[1][1] = kCamLoc__kCamD__kCamR.y;
			kFaceMat.m_pEntry[2][2] = 1.0f;
		}
		break;
	}
	default: {
		break;
	}
	}
	// The product is built in kDest's slot, then copied into the world rotation.
	m_kWorld.m_Rotate = m_kWorld.m_Rotate * kFaceMat;
	double m_fSavedTime = this->m_fSavedTime;
	v54.fTime = m_fSavedTime;
	uint32_t v43 = 0;
	v54.bUpdateControllers = (this->NiBillboardNode::m_uFlags & 8) != 0;
	v54.bParallelUpdate = 0;
	v54.bFoundParticles = 0;
	v54.bFoundMorphController = 0;
	for (v54.bSceneGraphChange = 0; v43 < m_kChildren.m_usSize; ++v43) {
		NiAVObject* pkChild = m_kChildren.m_pBase[v43].m_pObject;
		if (pkChild) {
			pkChild->UpdateDownwardPass(v54, 0);
		}
	}
}

void NiBillboardNode::SetMode(FaceMode eMode) {
	SetField(eMode, 7, 0);
}

bool NiBillboardNode::RotateToCenter(const NiCamera* pkCamera, const NiPoint3& kWorldTranslate, NiPoint3& kCamD,
	NiPoint3& kCamU, NiPoint3& kCamR) {
	NiPoint3 v21;
	NiPoint3 result;
	v21.x = pkCamera->m_kWorld.m_Translate.x - kWorldTranslate.x;
	v21.y = pkCamera->m_kWorld.m_Translate.y - kWorldTranslate.y;
	v21.z = pkCamera->m_kWorld.m_Translate.z - kWorldTranslate.z;
	float pkCamerab = v21.x * v21.x + v21.y * v21.y + v21.z * v21.z;
	if (pkCamerab < 0.001f) {
		return 0;
	}
	v21.Unitize();
	float v22 = pkCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
	float v23 = pkCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
	float v24 = pkCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
	result.x = -v22;
	double v8 = v23;
	kCamD.x = result.x;
	result.y = -v8;
	double v9 = v24;
	kCamD.y = result.y;
	result.z = -v9;
	kCamD.z = result.z;
	result.x = pkCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
	result.y = pkCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
	float y = result.y;
	double v11 = pkCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
	kCamU.x = result.x;
	result.z = v11;
	float z = result.z;
	kCamU.y = y;
	kCamU.z = z;
	result.x = pkCamera->m_kWorld.m_Rotate.m_pEntry[0][2];
	result.y = pkCamera->m_kWorld.m_Rotate.m_pEntry[1][2];
	float v13 = result.y;
	result.z = pkCamera->m_kWorld.m_Rotate.m_pEntry[2][2];
	float v14 = result.z;
	kCamR.x = result.x;
	kCamR.y = v13;
	kCamR.z = v14;
	float pkCameraa = kCamD.y * v21.y + kCamD.x * v21.x + kCamD.z * v21.z;
	if (pkCameraa < 0.9999989867210388) {
		NiMatrix3 v26;
		result = v21.UnitCross(kCamD);
		float a5 = result.z;
		float a4 = result.y;
		float a3 = result.x;
		float fAngle = NiACos(pkCameraa);
		v26.MakeRotation(fAngle, a3, a4, a5);
		float v15 = v21.y;
		float v16 = v21.z;
		kCamD.x = v21.x;
		kCamD.y = v15;
		kCamD.z = v16;
		kCamU = (result = v26 * kCamU);
		kCamR = (result = v26 * kCamR);
	}
	return 1;
}

void NiBillboardNode::SetField(uint16_t uVal, uint16_t uMask, uint16_t uPos) {
	m_uFlags = (uint16_t)((uVal << uPos) | ((~uMask) & m_uFlags));
}

uint16_t NiBillboardNode::GetField(uint16_t uMask, uint16_t uPos) const {
	return (m_uFlags & uMask) >> uPos;
}

void NiBillboardNode::SetBit(bool bVal, uint16_t uMask) {
	if (bVal) {
		m_uFlags |= uMask;
	} else {
		m_uFlags &= (~uMask);
	}
}

void NiBillboardNode::SetUpdateControllers(bool bUpdateControllers) {
	SetBit(bUpdateControllers, 8u);
}

// Adds a BSROTATE_ABOUT_UP case; an unknown mode leaves the buffer unwritten, as the game does.
char* NiBillboardNode::GetViewerString(const char* pcPrefix, FaceMode eMode) {
	size_t stLength = strlen(pcPrefix) + 22;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	switch (eMode) {
	case ALWAYS_FACE_CAMERA:
		NiSprintf(pcString, stLength, "%s = ALWAYS_FACE_CAMERA", pcPrefix);
		break;
	case ROTATE_ABOUT_UP:
		NiSprintf(pcString, stLength, "%s = ROTATE_ABOUT_UP", pcPrefix);
		break;
	case RIGID_FACE_CAMERA:
		NiSprintf(pcString, stLength, "%s = RIGID_FACE_CAMERA", pcPrefix);
		break;
	case ALWAYS_FACE_CENTER:
		NiSprintf(pcString, stLength, "%s = ALWAYS_FACE_CENTER", pcPrefix);
		break;
	case RIGID_FACE_CENTER:
		NiSprintf(pcString, stLength, "%s = RIGID_FACE_CENTER", pcPrefix);
		break;
	case BSROTATE_ABOUT_UP:
		NiSprintf(pcString, stLength, "%s = BSROTATE_ABOUT_UP", pcPrefix);
		break;
	}
	return pcString;
}

NiObject* NiBillboardNode::CreateObject() {
	NiBillboardNode* pkObject = static_cast<NiBillboardNode*>(NiMemObject::operator new(sizeof(NiBillboardNode)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiBillboardNode;
}
