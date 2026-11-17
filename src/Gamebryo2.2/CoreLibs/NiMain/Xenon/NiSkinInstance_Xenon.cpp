#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/WeaponObject.h"
#include "BSShader/ParticleShaderProperty.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

#include <stdint.h>

// The first bone's bound is moved into world space; each further bone's is merged in. A bone whose world scale is
// exactly 1 (an ordered compare: NaN takes the full update) only has its bound translated, its rotation ignored, with
// each sum rounded to float. The world bound is then moved into skin space: the root parent's inverse world transform,
// composed under the skin data's root-parent-to-skin transform, with the product written over the inverse.
void NiSkinInstance::UpdateModelBound(NiBound& arBound) {
	const NiSkinData::BoneData* pkBoneData = m_spSkinData.m_pObject->m_pkBoneData;
	arBound.Update(pkBoneData[0].m_kBound, m_ppkBones[0]->m_kWorld);
	uint32_t uiBones = m_spSkinData.m_pObject->m_uiBones;
	for (uint32_t i = 1; i < uiBones; i++) {
		NiAVObject* pkBone = m_ppkBones[i];
		const NiTransform& kWorld = pkBone->m_kWorld;
		const NiBound& kSourceBound = pkBoneData[i].m_kBound;
		NiBound kBoneBound;
		if (kWorld.m_fScale == 1.0f) {
			kBoneBound.m_fRadius = kSourceBound.m_fRadius;
			kBoneBound.m_kCenter.x = kWorld.m_Translate.x + kSourceBound.m_kCenter.x;
			kBoneBound.m_kCenter.y = kWorld.m_Translate.y + kSourceBound.m_kCenter.y;
			kBoneBound.m_kCenter.z = kWorld.m_Translate.z + kSourceBound.m_kCenter.z;
		} else {
			kBoneBound.Update(kSourceBound, kWorld);
		}
		arBound.Merge(&kBoneBound);
	}
	NiTransform kWorldToSkin;
	m_pkRootParent->m_kWorld.InvertNonUniform(kWorldToSkin);
	kWorldToSkin = m_spSkinData.m_pObject->m_kRootParentToSkin * kWorldToSkin;
	arBound.Update(kWorldToSkin);
}
