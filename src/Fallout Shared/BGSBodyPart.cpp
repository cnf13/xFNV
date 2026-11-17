#include "Fallout Shared/BGSBodyPart.h"

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkLimitedHingeConstraint.h"
#include "BSHavok/bhkMalleableConstraint.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/ParticleShaderProperty.h"
#include "Fallout Shared/BGSDebris.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelPSA.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// Reads the part's names, models and data. The chunks come in a fixed order and a missing one fails the load;
// NAM2 and NAM3 are read and dropped, as the exe does.
bool BGSBodyPart::Load(TESFile* apFile) {
	if (!apFile) {
		return false;
	}
	if (apFile->GetTESChunk() == BPTN_ID) {
		// the exe's _alloca (__alloca_probe_16): a 16-byte aligned buffer of the chunk's size
		char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
		apFile->GetChunkData(pBuffer, 0);
		PartName.Set(pBuffer, 0);
		if (!apFile->NextChunk()) {
			return false;
		}
	}
	if (apFile->GetTESChunk() == PNAM_ID) {
		TESModel::LoadModelChunk(&PSAModel, apFile);
		if (!apFile->NextChunk()) {
			return false;
		}
	}
	if (apFile->GetTESChunk() != BPNN_ID) {
		return false;
	}
	{
		char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
		apFile->GetChunkData(pBuffer, 0);
		NodeName.Set(pBuffer, 0);
	}
	if (!apFile->NextChunk()) {
		return false;
	}
	if (apFile->GetTESChunk() != BPNT_ID) {
		return false;
	}
	{
		char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
		apFile->GetChunkData(pBuffer, 0);
		TargetName.Set(pBuffer, 0);
	}
	if (!apFile->NextChunk()) {
		return false;
	}
	if (apFile->GetTESChunk() == BPNI_ID) {
		char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
		apFile->GetChunkData(pBuffer, 0);
		IKStartNodeName.Set(pBuffer, 0);
		if (!apFile->NextChunk()) {
			return false;
		}
	}
	if (apFile->GetTESChunk() != BPND_ID) {
		return false;
	}
	// the exe swaps the data both before and after reading it
	if (apFile->QEndian()) {
		Data.Endian();
	}
	apFile->GetChunkData(&Data, 0x54u);
#if FNVD_64BIT
	// The file's record is 0x54 bytes, with a 4-byte form ID in each pointer field.
	TESFile::WidenFormIDRecords(&Data, 1, 0x54, sizeof(Data),
		{{0x0C, offsetof(PART_DATA, pExplosionGenericDebris)}, {0x10, offsetof(PART_DATA, pExplosion)},
			{0x20, offsetof(PART_DATA, pDismemberGenericDebris)}, {0x24, offsetof(PART_DATA, pDismemberExplosion)},
			{0x44, offsetof(PART_DATA, pDismemberImpactDataSet)},
			{0x48, offsetof(PART_DATA, pExplosionImpactDataSet)}});
#endif
	if (apFile->QEndian()) {
		Data.Endian();
	}
	uint8_t ucZeroScale = 0;
	if (Data.fExplosionSpecialDebrisScale == 0.0) {
		Data.fExplosionSpecialDebrisScale = 0.1f;
		ucZeroScale = 1;
	}
	if (Data.fDismemberGenericDebrisScale == 0.0) {
		Data.fDismemberGenericDebrisScale = 0.1f;
		ucZeroScale = 1;
	}
	if (Data.fExplosionGenericDebrisScale == 0.0) {
		Data.fExplosionGenericDebrisScale = 0.1f;
		ucZeroScale = 1;
	}
	if (ucZeroScale) {
		Warning("MODELS: Debris scale value of zero loaded.  Please set to non-zero value in editor.");
	}
	if (!apFile->NextChunk()) {
		return false;
	}
	if (apFile->GetTESChunk() != NAM1_ID) {
		return false;
	}
	if (apFile->GetChunkSize()) {
		char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
		apFile->GetChunkData(pBuffer, 0);
		// the exe calls the model's vtable slot 6 (SetModel) through the part's model subobject
		TESModel* pDebrisModel = &ExplosionSpecialDebris;
		pDebrisModel->SetModel(pBuffer);
	}
	if (apFile->GetFormVersion() < 3) {
		if (!apFile->NextChunk() || apFile->GetTESChunk() != NAM2_ID) {
			return apFile->GetFormVersion() < 2;
		}
		if (apFile->GetChunkSize()) {
			// the exe reads the chunk into a buffer it never looks at
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
		}
		if (!apFile->NextChunk() || apFile->GetTESChunk() != NAM3_ID) {
			return apFile->GetFormVersion() < 2;
		}
		if (apFile->GetChunkSize()) {
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
		}
	}
	if (!apFile->NextChunk() || apFile->GetTESChunk() != NAM4_ID) {
		return apFile->GetFormVersion() < 2;
	}
	if (apFile->GetChunkSize()) {
		char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
		apFile->GetChunkData(pBuffer, 0);
		GoreObjectName.Set(pBuffer, 0);
	}
	if (!apFile->NextChunk() || apFile->GetTESChunk() != NAM5_ID) {
		return true;
	}
	if (apFile->GetChunkSize()) {
		TESModel::LoadModelTextureChunk(&ExplosionSpecialDebris, apFile);
	}
	return true;
}

PART_DATA::PART_DATA() {}

void PART_DATA::Endian() {
	ENDIAN(fDamageMult, false);
	ENDIAN(fTrackingMaxAngle, false);
	ENDIAN(fExplosionGenericDebrisScale, false);
	ENDIAN(fDismemberGenericDebrisScale, false);
	ENDIAN(kGoreTranslate.x, false);
	ENDIAN(kGoreTranslate.y, false);
	ENDIAN(kGoreTranslate.z, false);
	ENDIAN(kGoreRotate.x, false);
	ENDIAN(kGoreRotate.y, false);
	ENDIAN(kGoreRotate.z, false);
	ENDIAN(fExplosionSpecialDebrisScale, false);
}

void PART_DATA::Initialize() {
	cHealthPercent = 5;
	fDamageMult = 1.0f;
	cActorValue = -1;
	cToHitChance = 100;
	cExplosionChance = 0;
	cExplosionGenericDebrisCount = 0;
	pExplosionGenericDebris = 0;
	pExplosion = 0;
	fTrackingMaxAngle = 0.0f;
	fExplosionGenericDebrisScale = 1.0f;
	fExplosionSpecialDebrisScale = 1.0f;
	cDismemberGenericDebrisCount = 0;
	pDismemberGenericDebris = 0;
	pDismemberExplosion = 0;
	fDismemberGenericDebrisScale = 1.0f;
	kGoreTranslate = NiPoint3_ZERO;
	kGoreRotate = NiPoint3_ZERO;
	pDismemberImpactDataSet = 0;
	pExplosionImpactDataSet = 0;
	cDismemberDecalCount = 0;
	cExplosionDecalCount = 0;
}

BGSBodyPart::BGSBodyPart() {
	ourMemset(&Data, 0, sizeof(PART_DATA));
	Data.Initialize();
	InitializeData();
}

BGSBodyPart::~BGSBodyPart() {
	ClearData();
}

void BGSBodyPart::QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
	if (ExplosionSpecialDebris.GetModelLength()) {
		pModelLoader->QueueModel(&ExplosionSpecialDebris, aePriority, apParent, LOD_MULT_OBJECTS, static_cast<bool>(1),
			static_cast<bool>(0), static_cast<bool>(0));
	}
	if (Data.pExplosionGenericDebris) {
		Data.pExplosionGenericDebris->QueueFiles(aePriority, apParent);
	}
	if (Data.pExplosion) {
		Data.pExplosion->QueueFiles(aePriority, apParent);
	}
	if (Data.pDismemberGenericDebris) {
		Data.pDismemberGenericDebris->QueueFiles(aePriority, apParent);
	}
	if (Data.pDismemberExplosion) {
		Data.pDismemberExplosion->QueueFiles(aePriority, apParent);
	}
	if (Data.pDismemberImpactDataSet) {
		Data.pDismemberImpactDataSet->QueueFiles(aePriority, apParent);
	}
	if (Data.pExplosionImpactDataSet) {
		Data.pExplosionImpactDataSet->QueueFiles(aePriority, apParent);
	}
}

// Resolves the form IDs the loader stored in the pointer fields to their forms.
void BGSBodyPart::InitItem(TESForm* apParentForm) {
	if (apParentForm) {
		if (!apParentForm->GetInitialized()) {
			uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pExplosion));
			if (itempInt) {
				TESFile* File = apParentForm->GetFile(-1);
				TESForm::AddCompileIndex(itempInt, File);
				TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempInt);
				Data.pExplosion = dynamic_cast<BGSExplosion*>(FormByNumericID);
			}
			itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pExplosionGenericDebris));
			if (itempInt) {
				TESFile* v4 = apParentForm->GetFile(-1);
				TESForm::AddCompileIndex(itempInt, v4);
				TESForm* v5 = TESForm::GetFormByNumericID(itempInt);
				Data.pExplosionGenericDebris = dynamic_cast<BGSDebris*>(v5);
			}
			itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pDismemberExplosion));
			if (itempInt) {
				TESFile* v6 = apParentForm->GetFile(-1);
				TESForm::AddCompileIndex(itempInt, v6);
				TESForm* v7 = TESForm::GetFormByNumericID(itempInt);
				Data.pDismemberExplosion = dynamic_cast<BGSExplosion*>(v7);
			}
			itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pDismemberGenericDebris));
			if (itempInt) {
				TESFile* v8 = apParentForm->GetFile(-1);
				TESForm::AddCompileIndex(itempInt, v8);
				TESForm* v9 = TESForm::GetFormByNumericID(itempInt);
				Data.pDismemberGenericDebris = dynamic_cast<BGSDebris*>(v9);
			}
			itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetDismemberImpactDataSet()));
			if (itempInt) {
				TESFile* v10 = apParentForm->GetFile(-1);
				TESForm::AddCompileIndex(itempInt, v10);
				TESForm* v11 = TESForm::GetFormByNumericID(itempInt);
				Data.pDismemberImpactDataSet = dynamic_cast<BGSImpactDataSet*>(v11);
			}
			itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pExplosionImpactDataSet));
			if (itempInt) {
				TESFile* v12 = apParentForm->GetFile(-1);
				TESForm::AddCompileIndex(itempInt, v12);
				TESForm* v13 = TESForm::GetFormByNumericID(itempInt);
				Data.pExplosionImpactDataSet = dynamic_cast<BGSImpactDataSet*>(v13);
			}
		}
	}
}

BGSBodyPart::LIMB_ENUM BGSBodyPart::GetPartType() {
	return (LIMB_ENUM)Data.cType;
}

bool BGSBodyPart::IsIKBiped() {
	return ((Data.cFlags & 4) != 0);
}

bool BGSBodyPart::DoesHeadtracking() {
	return ((Data.cFlags & 32) != 0);
}

char BGSBodyPart::GetActorValue() {
	return Data.cActorValue;
}

uint8_t BGSBodyPart::GetHealthPercent() {
	return Data.cHealthPercent;
}

bool BGSBodyPart::IsExplodeChanceAbsolute() {
	return ((Data.cFlags & 64) != 0);
}

uint8_t BGSBodyPart::GetExplosionChance() {
	return Data.cExplosionChance;
}

void BGSBodyPart::InitializeData() {
	(&ExplosionSpecialDebris)->InitializeDataComponent();
	(&PSAModel)->InitializeDataComponent();
}

void BGSBodyPart::ClearData() {
	(&ExplosionSpecialDebris)->ClearDataComponent();
	(&PSAModel)->ClearDataComponent();
}

bool BGSBodyPart::IsIKData() {
	return ((Data.cFlags & 2) != 0);
}

uint8_t BGSBodyPart::GetDebrisCount() {
	return Data.cExplosionGenericDebrisCount;
}

uint8_t BGSBodyPart::GetSeveredDecalCount() {
	return Data.cDismemberDecalCount;
}

bool BGSBodyPart::IsExplodablePart() {
	return ((Data.cFlags & 8) != 0);
}

bool BGSBodyPart::IsSeverablePart() {
	return ((Data.cFlags & 1) != 0);
}

const char* BGSBodyPart::GetPartName() {
	return PartName.String();
}

BGSImpactDataSet* BGSBodyPart::GetDismemberImpactDataSet() {
	return Data.pDismemberImpactDataSet;
}

NiPoint3* BGSBodyPart::GetGoreTranslate(NiPoint3* apResult) {
	*apResult = Data.kGoreTranslate;
	return apResult;
}

const char* BGSBodyPart::GetGoreObjectName() {
	return GoreObjectName.String();
}

NiPoint3* BGSBodyPart::GetGoreRotate(NiPoint3* apResult) {
	*apResult = Data.kGoreRotate;
	return apResult;
}

void BGSBodyPart::UnhideLimb(BGSBodyPart::LIMB_ENUM aeLimb, NiAVObject* apObj) {
	if (apObj) {
		if (apObj->IsNode()) {
			NiNode* v8 = apObj->IsNode();
			uint32_t a1 = 0;
			for (uint32_t i = v8->GetArrayCount(); a1 < i; i = v8->GetArrayCount()) {
				NiAVObject* v3 = v8->GetAt(a1);
				BGSBodyPart::UnhideLimb(aeLimb, v3);
				++a1;
			}
		} else {
			if (apObj->IsGeometry()) {
				NiGeometry* v5 = apObj->IsGeometry();
				NiObject* pkObject = v5->GetSkinInstance();
				BSDismemberSkinInstance* v4 =
					static_cast<BSDismemberSkinInstance*>(NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI,
						pkObject));
				if (v4) {
					v4->SetBodyPartEnabled(aeLimb, 1);
					v4->SetBodyPartEnabled(aeLimb + 200, 0);
				}
			}
		}
	}
}

void BGSBodyPart::HideLimb(BGSBodyPart::LIMB_ENUM aeLimb, NiAVObject* apObj) {
	if (apObj) {
		if (apObj->IsNode()) {
			NiNode* pNode = apObj->IsNode();
			uint32_t a1 = 0;
			for (uint32_t i = pNode->GetArrayCount(); a1 < i; i = pNode->GetArrayCount()) {
				NiAVObject* v3 = pNode->GetAt(a1);
				BGSBodyPart::HideLimb(aeLimb, v3);
				++a1;
			}
		} else {
			if (apObj->IsGeometry()) {
				NiGeometry* v5 = apObj->IsGeometry();
				NiObject* pkObject = v5->GetSkinInstance();
				BSDismemberSkinInstance* v4 =
					static_cast<BSDismemberSkinInstance*>(NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI,
						pkObject));
				if (v4) {
					v4->SetBodyPartEnabled(aeLimb, 0);
					v4->SetBodyPartEnabled(aeLimb + 200, 1);
				}
			}
		}
	}
}

void BGSBodyPart::ShowLimb(BGSBodyPart::LIMB_ENUM usBodyPart, NiObject* a2) {
	NiNode* pnode = a2->IsNode();
	uint32_t n = 0;
	while (pnode && n < pnode->GetArrayCount()) {
		NiSkinInstance* pskin;
		NiGeometry* ptri;
		if (pnode->GetAt(n)) {
			NiAVObject* v4 = pnode->GetAt(n);
			ptri = v4->IsGeometry();
		} else {
			ptri = a2->IsGeometry();
		}
		++n;
		if (ptri) {
			pskin = ptri->GetSkinInstance();
		} else {
			pskin = 0;
		}
		BSDismemberSkinInstance* pdSkinInstance =
			static_cast<BSDismemberSkinInstance*>(NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI,
				pskin));
		if (pdSkinInstance) {
			for (BGSBodyPart::LIMB_ENUM eindex = BGSBodyPart::LIMB_TORSO; eindex < BGSBodyPart::LIMB_COUNT;
				eindex = static_cast<BGSBodyPart::LIMB_ENUM>(eindex + 1)) {
				if (eindex == usBodyPart) {
					pdSkinInstance->SetBodyPartEnabled(usBodyPart + 100, 1);
				} else {
					pdSkinInstance->SetBodyPartEnabled(eindex, 0);
					pdSkinInstance->SetBodyPartEnabled(eindex + 200, 0);
				}
				pdSkinInstance->SetBodyPartEnabled(1000 * eindex, 0);
			}
		}
	}
}

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
uint8_t GetGenericDebrisCount(BGSBodyPart* apThis, [[maybe_unused]] void* apEdx) {
	return apThis->Data.cDismemberGenericDebrisCount;
}

// The constraint's creation info, from the hkSerializableCinfo CreateCinfo returns, null for none (thiscall, ret 0,
// 0x5E4AB0). The downcast goes through a pointer: the exe tests the result for null before subtracting the base's
// offset.
hkLimitedHingeConstraintCinfo& bhkLimitedHingeConstraint::GetCinfo() {
	bool bc;
	hkSerializableCinfo* pCinfo = &CreateCinfo(bc);
	return *static_cast<hkLimitedHingeConstraintCinfo*>(pCinfo);
}

hkMalleableConstraintCinfo& bhkMalleableConstraint::GetCinfo() {
	bool bc;
	hkSerializableCinfo* pCinfo = &CreateCinfo(bc);
	return *static_cast<hkMalleableConstraintCinfo*>(pCinfo);
}

TESModelPSA::TESModelPSA() : TESModel() {}

TESModelPSA::~TESModelPSA() {}
