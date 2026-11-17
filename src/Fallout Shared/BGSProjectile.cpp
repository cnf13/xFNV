#include "Fallout Shared/BGSProjectile.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/BSClonePools.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/GrenadeProjectile.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// Reads the DATA record, the muzzle-flash model (NAM1's path through the model's virtual setter, NAM2's texture
// chunk), the destruction chunks, the full name, the model chunks and the sound level. The exe stores GetTESForm's
// byte and compares it zero-extended, hence the uint8_t.
// The exe reads this slot uninitialised when the chunk is empty (malformed data); src initialises it to "" (the EDID
// alloca'd buffer: one zeroed byte, allocated only for an empty chunk).
bool BGSProjectile::Load(TESFile* apFile) {
	uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (ucFormType != PROJ_ID) {
		return false;
	}
	ClearData();
	LoadForm(apFile);
	SetInitialized(false);
	for (;;) {
		CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
		if (eChunk == NO_CHUNK) {
			break;
		}

		switch (eChunk) {
		case DSTD_ID:
		case DEST_ID:
			BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
			break;
		case DATA_ID:
			// LoadData takes the record as raw bytes; the exe reads the chunk straight into the DATA member.
			LoadData(apFile, reinterpret_cast<char*>(&Data), 0x54);
#if FNVD_64BIT
			// The file's record is 0x54 bytes, with a 4-byte form ID in each pointer field.
			TESFile::WidenFormIDRecords(&Data, 1, 0x54, sizeof(Data),
				{{0x10, offsetof(BGSProjectileData, pLight)}, {0x14, offsetof(BGSProjectileData, pMuzzleFlashLight)},
					{0x24, offsetof(BGSProjectileData, pExplosionType)},
					{0x28, offsetof(BGSProjectileData, pActiveSoundLoop)},
					{0x38, offsetof(BGSProjectileData, pCountdownSound)},
					{0x3C, offsetof(BGSProjectileData, pDeactivateSound)},
					{0x40, offsetof(BGSProjectileData, pDefaultWeaponSource)}});
#endif
			break;
		case NAM1_ID: {
			if (apFile->GetChunkSize()) {
				// The exe calls GetChunkSize a second time for the buffer: its _alloca (__alloca_probe_16,
				// 0xEC5EC0) reserves a 16-byte aligned buffer of the chunk's size.
				char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
				apFile->GetChunkData(pBuffer, 0);
				// The exe calls the model's virtual setter through slot 6 of its vtable.
				TESModel* pMuzzleFlashModel = &MuzzleFlashModel;
				pMuzzleFlashModel->SetModel(pBuffer);
			}
			break;
		}
		case NAM2_ID:
			if (apFile->GetChunkSize()) {
				TESModel::LoadModelTextureChunk(&MuzzleFlashModel, apFile);
			}
			break;
		case EDID_ID: {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// Not in the exe: an empty chunk leaves the buffer unwritten (see above).
			pEditorID[0] = '\0';
			apFile->GetChunkData(pEditorID, 0x200);
			static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
			break;
		}
		case OBND_ID:
			static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile);
			break;
		case VNAM_ID: {
			uint32_t uiSoundLevel = 0;
			apFile->GetChunkData(uiSoundLevel);
			// The exe compares unsigned: above 3 the sound level falls back to 1.
			if (uiSoundLevel > 3) {
				eSoundLevel = SOUND_LEVEL_NORMAL;
			} else {
				eSoundLevel = static_cast<SOUND_LEVEL>(uiSoundLevel);
			}
			break;
		}
		case MODL_ID:
		case MODT_ID: {
			// The exe null-checks the form before taking the model subobject's address.
			TESModel* pModel = static_cast<TESModel*>(this);
			TESModel::LoadModelChunk(pModel, apFile);
			break;
		}
		case FULL_ID:
			TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile);
			break;
		}

		if (eChunk == DATA_ID && apFile->QEndian()) {
			Data.Endian();
		}

		if (!apFile->NextChunk()) {
			break;
		}
	}
	return true;
}

BGSProjectileData::BGSProjectileData() {
	Initialize();
}

void BGSProjectileData::Initialize() {
	iFlags = 0x10000;
	fGravity = 0.0f;
	fSpeed = 1000.0f;
	fRange = 10000.0f;
	fTracerChance = 0.0f;
	fExplosionProximity = 0.0f;
	fExplosionTimer = 0.0f;
	pLight = nullptr;
	pMuzzleFlashLight = nullptr;
	pExplosionType = nullptr;
	pActiveSoundLoop = nullptr;
	fMuzzleFlashDuration = 0.0f;
	fFadeOutTime = 0.5f;
	fForce = 0.0f;
	pCountdownSound = nullptr;
	pDeactivateSound = nullptr;
	pDefaultWeaponSource = nullptr;
	fRotationX = 0.0f;
	fRotationY = 0.0f;
	fRotationZ = 0.0f;
	fBounceMultiplier = 1.0f;
}

void BGSProjectileData::Endian() {
	ENDIAN(iFlags, false);
	ENDIAN(fGravity, false);
	ENDIAN(fSpeed, false);
	ENDIAN(fRange, false);
	ENDIAN(fTracerChance, false);
	ENDIAN(fExplosionProximity, false);
	ENDIAN(fExplosionTimer, false);
	ENDIAN(fMuzzleFlashDuration, false);
	ENDIAN(fFadeOutTime, false);
	ENDIAN(fForce, false);
	ENDIAN(fRotationX, false);
	ENDIAN(fRotationY, false);
	ENDIAN(fRotationZ, false);
	ENDIAN(fBounceMultiplier, false);
}

BGSProjectile::BGSProjectile() {
	SetFormType(PROJ_ID);
	BGSProjectile::InitializeData();
}

BGSProjectile::~BGSProjectile() {
	BGSProjectile::ClearData();
}

void BGSProjectile::InitializeData() {
	GetData()->Initialize();
	TESModel* pMuzzleFlashModel = &MuzzleFlashModel;
	pMuzzleFlashModel->InitializeDataComponent();
	eSoundLevel = SOUND_LEVEL_NORMAL;
}

void BGSProjectile::ClearData() {
	TESModel* pMuzzleFlashModel = &MuzzleFlashModel;
	pMuzzleFlashModel->ClearDataComponent();
}

// The DATA chunk holds form IDs: each form pointer is overwritten with its form's ID for the write, then restored.
void BGSProjectile::Save() {
	TESObjectWEAP* v3;
	TESSound* v4;
	TESSound* v5;
	TESSound* v6;
	BGSExplosion* v7;
	TESObjectLIGH* v8;
	TESObjectLIGH* CastingType;
	StartForm();
	TESFullName::Save();
	TESModel::Save(MODL_ID, MODT_ID);
	BGSDestructibleObjectForm::Save();
	TESObjectLIGH* pLight = Data.pLight;
	if (pLight) {
		// EXE-CAST: uint32_t (a form ID) as TESObjectLIGH*
		CastingType = reinterpret_cast<TESObjectLIGH*>(static_cast<uintptr_t>(Data.pLight->iFormID));
	} else {
		CastingType = 0;
	}
	Data.pLight = CastingType;
	TESObjectLIGH* pMuzzleFlashLight = Data.pMuzzleFlashLight;
	if (pMuzzleFlashLight) {
		// EXE-CAST: uint32_t (a form ID) as TESObjectLIGH*
		v8 = reinterpret_cast<TESObjectLIGH*>(static_cast<uintptr_t>(Data.pMuzzleFlashLight->iFormID));
	} else {
		v8 = 0;
	}
	Data.pMuzzleFlashLight = v8;
	BGSExplosion* psaveMuzzleLight = Data.pExplosionType;
	if (psaveMuzzleLight) {
		// EXE-CAST: uint32_t (a form ID) as BGSExplosion*
		v7 = reinterpret_cast<BGSExplosion*>(static_cast<uintptr_t>(Data.pExplosionType->iFormID));
	} else {
		v7 = 0;
	}
	Data.pExplosionType = v7;
	TESSound* pActiveSoundLoop = Data.pActiveSoundLoop;
	if (pActiveSoundLoop) {
		// EXE-CAST: uint32_t (a form ID) as TESSound*
		v6 = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(Data.pActiveSoundLoop->iFormID));
	} else {
		v6 = 0;
	}
	Data.pActiveSoundLoop = v6;
	TESSound* psaveActiveSound = Data.pCountdownSound;
	if (psaveActiveSound) {
		// EXE-CAST: uint32_t (a form ID) as TESSound*
		v5 = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(Data.pCountdownSound->iFormID));
	} else {
		v5 = 0;
	}
	Data.pCountdownSound = v5;
	TESSound* pDeactivateSound = Data.pDeactivateSound;
	if (pDeactivateSound) {
		// EXE-CAST: uint32_t (a form ID) as TESSound*
		v4 = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(Data.pDeactivateSound->iFormID));
	} else {
		v4 = 0;
	}
	Data.pDeactivateSound = v4;
	TESObjectWEAP* pDefaultWeaponSource = Data.pDefaultWeaponSource;
	if (pDefaultWeaponSource) {
		// EXE-CAST: uint32_t (a form ID) as TESObjectWEAP*
		v3 = reinterpret_cast<TESObjectWEAP*>(static_cast<uintptr_t>(Data.pDefaultWeaponSource->iFormID));
	} else {
		v3 = 0;
	}
	Data.pDefaultWeaponSource = v3;
	if (TESForm::QEndianSwapOnSave()) {
		Data.Endian();
	}
	__SaveData(&Data, 84u);
	if (TESForm::QEndianSwapOnSave()) {
		Data.Endian();
	}
	Data.pLight = pLight;
	Data.pMuzzleFlashLight = pMuzzleFlashLight;
	Data.pExplosionType = psaveMuzzleLight;
	Data.pActiveSoundLoop = pActiveSoundLoop;
	Data.pCountdownSound = psaveActiveSound;
	Data.pDeactivateSound = pDeactivateSound;
	Data.pDefaultWeaponSource = pDefaultWeaponSource;
	if (MuzzleFlashModel.GetModelLength()) {
		MuzzleFlashModel.Save(NAM1_ID, NAM2_ID);
	} else {
		uint32_t v2 = MuzzleFlashModel.GetModelLength() + 1;
		const char* v1 = MuzzleFlashModel.GetModel();
		TESForm::AddChunkArray(NAM1_ID, v1, v2);
	}
	// The PDB's int overload: the exe pushes the enum's dword to the 4-byte AddChunk (0x485910).
	TESForm::AddChunk(VNAM_ID, eSoundLevel);
	CloseForm();
}

// Load leaves each form's ID in its pointer field; InitItem replaces it with the form.
void BGSProjectile::InitItem() {
	if (!GetInitialized()) {
		BGSDestructibleObjectForm::InitItem(this);
		uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pLight));
		if (itempInt) {
			TESFile* File = GetFile(-1);
			TESForm::AddCompileIndex(itempInt, File);
			TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempInt);
			Data.pLight = dynamic_cast<TESObjectLIGH*>(FormByNumericID);
		}
		itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pMuzzleFlashLight));
		if (itempInt) {
			TESFile* v3 = GetFile(-1);
			TESForm::AddCompileIndex(itempInt, v3);
			TESForm* v4 = TESForm::GetFormByNumericID(itempInt);
			Data.pMuzzleFlashLight = dynamic_cast<TESObjectLIGH*>(v4);
		}
		itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetExplosion()));
		if (itempInt) {
			TESFile* v5 = GetFile(-1);
			TESForm::AddCompileIndex(itempInt, v5);
			TESForm* v6 = TESForm::GetFormByNumericID(itempInt);
			Data.pExplosionType = dynamic_cast<BGSExplosion*>(v6);
		}
		itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pActiveSoundLoop));
		if (itempInt) {
			TESFile* v7 = GetFile(-1);
			TESForm::AddCompileIndex(itempInt, v7);
			TESForm* v8 = TESForm::GetFormByNumericID(itempInt);
			Data.pActiveSoundLoop = dynamic_cast<TESSound*>(v8);
		}
		itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pCountdownSound));
		if (itempInt) {
			TESFile* v10 = GetFile(-1);
			TESForm::AddCompileIndex(itempInt, v10);
			TESForm* v11 = TESForm::GetFormByNumericID(itempInt);
			Data.pCountdownSound = dynamic_cast<TESSound*>(v11);
		}
		itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pDeactivateSound));
		if (itempInt) {
			TESFile* v13 = GetFile(-1);
			TESForm::AddCompileIndex(itempInt, v13);
			TESForm* v14 = TESForm::GetFormByNumericID(itempInt);
			Data.pDeactivateSound = dynamic_cast<TESSound*>(v14);
		}
		itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetDefaultWeaponSource()));
		if (itempInt) {
			TESFile* v15 = GetFile(-1);
			TESForm::AddCompileIndex(itempInt, v15);
			TESForm* v16 = TESForm::GetFormByNumericID(itempInt);
			Data.pDefaultWeaponSource = dynamic_cast<TESObjectWEAP*>(v16);
		}
		if (!GetHasExplosion()) {
			Data.pExplosionType = 0;
		}
		SetInitialized(1);
	}
}

void BGSProjectile::Copy(TESForm* apCopy) {
	BGSProjectile* pcopy = dynamic_cast<BGSProjectile*>(apCopy);
	if (pcopy) {
		CopyAllComponents(pcopy);
		MuzzleFlashModel.CopyComponent(&pcopy->MuzzleFlashModel);
		eSoundLevel = pcopy->eSoundLevel;
		ourMemcpy(&Data, &pcopy->Data, sizeof(Data));
	}
}

bool BGSProjectile::Compare(TESForm* apCompare) {
	BGSProjectile* pcomp = dynamic_cast<BGSProjectile*>(apCompare);
	if (!pcomp) {
		return 1;
	}
	if (CompareAllComponents(pcomp)) {
		return 1;
	}
	if (std::memcmp(&Data, &pcomp->Data, sizeof(Data))) {
		return 1;
	}
	if (MuzzleFlashModel.CompareComponent(&pcomp->MuzzleFlashModel)) {
		return 1;
	}
	return eSoundLevel != pcomp->eSoundLevel;
}

bool BGSProjectile::Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
	TESBoundObject* apObjecttoGet, int32_t aiCount) {
	Actor* v10 = dynamic_cast<Actor*>(apActionRef);
	Projectile* v11 = dynamic_cast<Projectile*>(apItemActivated);
	if ((!v10) || (!v11)) {
		return (bool)0;
	}
	BGSProjectile* actorBase = v11->GetProjectileBase();
	bool vCondition1;
	vCondition1 =
		actorBase->GetCanTurnOff() && ((!v11->IsFlagSet(Projectile::TURNED_OFF)) && v11->TurnOff(v10, (bool)0));
	if (vCondition1) {
		return (bool)1;
	}
	BGSProjectile* v8 = v11->GetProjectileBase();
	return (bool)((uint8_t)(v8->GetCanPickup() &&
		TESBoundObject::Activate(apItemActivated, apActionRef, abIdFlag, apObjecttoGet, aiCount)));
}

// The exe's cleanup-only C++ EH frame is left out.
NiAVObject* BGSProjectile::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
	NiAVObject* pav = 0;
	if (pRequester) {
		int32_t MotionType = GetMotionType();
		if (MotionType == MOTION_BEAM || MotionType == MOTION_CONTINUOUS_BEAM) {
			pav = TESBoundObject::Clone3D(pRequester, 1);
		} else {
			Projectile* pprojectile = static_cast<Projectile*>(pRequester);
			if (pprojectile->pSecondaryItemChange && pprojectile->GetSecondaryItemChange()->pContainerObj) {
				TESBoundObject* FormFlags = pprojectile->GetSecondaryItemChange()->pContainerObj;
				NiAVObject* ploadedArt = FormFlags->LoadGraphics(0);
				FNVD_CONSTRUCTED_LOCAL(BSCloningProcess, cloneData, &BSCloningProcess::Destruct, 1.0f);
				if (ploadedArt) {
					pav = static_cast<NiAVObject*>(ploadedArt->Clone(cloneData));
				}
			} else {
				NiAVObject* ploadedArt_2 = LoadGraphics(pRequester);
				if (ploadedArt_2) {
					uint32_t CastingType = pRequester->iFormID;
					if (pDataHandler->IsFormIDCreated(CastingType) && !pSaveLoadGame->GetSaveGameLoading()) {
						BSClonePools* Singleton = BSClonePools::GetInstance(1);
						pav = Singleton->RequestClone(ploadedArt_2);
					}
				}
				if (!pav) {
					pav = TESBoundObject::Clone3D(pRequester, deepCopy);
					if (ploadedArt_2) {
						TESObject::UnClone3D(pRequester);
					}
				}
			}
		}
	}
	if (pav) {
		if (pav->IsFadeNode()) {
			BSFadeNode* v5 = pav->IsFadeNode();
			v5->SetIgnoreFade(1);
		}
	}
	return pav;
}

void BGSProjectile::UnClone3D(TESObjectREFR* apRequester) {
	ItemChange* v2;
	ItemChange* v3;
	TESObject* formFlags;
	uint32_t motionType;
	const char* apModelFileName;
	if (apRequester) {
		motionType = GetMotionType();
		if ((motionType == 262144) ||
			((motionType == 1048576) ||
				((!static_cast<Projectile*>(apRequester)->pSecondaryItemChange) ||
					((v2 = static_cast<Projectile*>(apRequester)->pSecondaryItemChange),
						(!v2->ItemChange::GetItemChangeObject()))))) {
			TESObject::UnClone3D(apRequester);
		} else {
			v3 = static_cast<Projectile*>(apRequester)->pSecondaryItemChange;
			formFlags = v3->ItemChange::GetItemChangeObject();
			apModelFileName = formFlags->GetModelFileName(nullptr);
			pModelLoader->ReleaseFile(apModelFileName);
		}
	}
}

void BGSProjectile::QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
	if (GetModelLength()) {
		TESModel* apModel;
		if (this == nullptr) {
			apModel = 0;
		} else {
			apModel = static_cast<TESModel*>(this);
		}
		ENUM_LOD_MULT LODMult = TES::GetLODMult(this);
		pModelLoader->QueueModel(apModel, aePriority, apParent, LODMult, 1, 0, 0);
	}
	if (MuzzleFlashModel.GetModelLength()) {
		pModelLoader->QueueModel(&MuzzleFlashModel, aePriority, apParent, LOD_MULT_ACTORS, 1, 0, 0);
	}
	if (GetExplosion()) {
		BGSPreloadable* v4 = static_cast<BGSPreloadable*>(GetExplosion());
		v4->QueueFiles(aePriority, apParent);
	}
}

BGSExplosion* BGSProjectile::GetExplosion() {
	return Data.pExplosionType;
}

BGSProjectileData* BGSProjectile::GetData() {
	return &Data;
}

bool BGSProjectile::GetHasExplosion() {
	return IsFlagSet(EXPLOSION);
}

bool BGSProjectile::IsFlagSet(BGSProjectileFlags aeFlag) {
	return (GetData()->iFlags & aeFlag) != 0;
}

BGSProjectile::BGSProjectileFlags BGSProjectile::GetMotionType() {
	return static_cast<BGSProjectileFlags>(Data.iFlags & MOTIONTYPES);
}

// How many projectiles to clone ahead for apSourceWeapon: none for beams, flames and continuous beams, 5 without a
// weapon, else three times the shots per volley capped by the clip (except for arcing ones), times 4 when pinning limbs.
int32_t BGSProjectile::GetPreCloneCount(TESObjectWEAP* apSourceWeapon) {
	int32_t iCount = 5;
	if (GetMotionType() == MOTION_BEAM || GetMotionType() == MOTION_CONTINUOUS_BEAM ||
		GetMotionType() == MOTION_FLAMETHROWER) {
		iCount = 0;
	} else if (apSourceWeapon) {
		int32_t iShots =
			(int32_t)apSourceWeapon->GetShotsPerSecond() * apSourceWeapon->GetNumProjectiles(false, false, nullptr);
		int32_t iPreClone = iShots * 3;
		if (GetMotionType() != MOTION_GRENADE) {
			iPreClone = BSMIN(iPreClone, apSourceWeapon->GetFormClipRounds(false));
		}
		if (IsFlagSet(PINS_LIMBS)) {
			iPreClone *= 4;
		}
		iCount = iPreClone;
	}
	return iCount;
}

TESObjectWEAP* BGSProjectile::GetDefaultWeaponSource() {
	return Data.pDefaultWeaponSource;
}

float BGSProjectile::GetAltTimer() const {
	return Data.fExplosionTimer;
}

float BGSProjectile::GetSpeed() {
	return Data.fSpeed;
}

TESModel& BGSProjectile::GetMuzzleFlashModel() {
	return MuzzleFlashModel;
}

float BGSProjectile::GetFadeDuration() const {
	return Data.fFadeOutTime;
}

bool BGSProjectile::GetHitScan() {
	return IsFlagSet(HITSCAN);
}

bool BGSProjectile::GetRotates() {
	return IsFlagSet(HAS_ROTATION);
}

bool BGSProjectile::GetHasMuzzleFlash() {
	return IsFlagSet(MUZZLE_FlASH);
}

float BGSProjectile::GetRange() {
	return Data.fRange;
}

float BGSProjectile::GetGravity() {
	if (IsFlagSet(MOTION_GRENADE)) {
		return (float)1.0;
	} else {
		return Data.fGravity;
	}
}

ENUM_FORM_ID BGSProjectile::GetProjectileRefFormType() {
	int32_t projFlags = (int32_t)GetMotionType();
	if (projFlags > 262144) {
		if (projFlags == 524288) {
			return (ENUM_FORM_ID)64;
		}
		if (projFlags == 1048576) {
			return (ENUM_FORM_ID)105;
		}
	} else {
		switch (projFlags) {
		case 262144: {
			return (ENUM_FORM_ID)63;
		}
		case 65536: {
			return (ENUM_FORM_ID)61;
		}
		case 131072: {
			return (ENUM_FORM_ID)62;
		}
		}
	}
	return (ENUM_FORM_ID)0;
}

bool BGSProjectile::GetPassThroughSmTransp() {
	return (bool)((uint8_t)IsFlagSet(PASS_SM_TRANSPARENT));
}

bool BGSProjectile::DecideIsTracer() {
	uint32_t v2 = (uint32_t)((int32_t)(GetTracerChance() * 100.0));
	return (bool)((uint8_t)((Random() % 100) < v2));
}

float BGSProjectile::GetTracerChance() {
	return Data.fTracerChance;
}

// The exe's cleanup-only C++ EH frame is left out.
GrenadeProjectile* BGSProjectile::InitLiveGrenade(BGSProjectile* apProjectile, Actor* apSource, TESObjectREFR* apTarget,
	TESObjectWEAP* apGrenade) {
	GrenadeProjectile* v10 = 0;
	if (apProjectile) {
		if (apProjectile->GetMotionType() == BGSProjectile::MOTION_GRENADE) {
			GrenadeProjectile* v7;
			GrenadeProjectile* pGrenadeProjectile =
				static_cast<GrenadeProjectile*>(ourMalloc(sizeof(GrenadeProjectile)));
			if (pGrenadeProjectile) {
				v7 = ::new (pGrenadeProjectile)
					GrenadeProjectile(apProjectile, apSource, apGrenade, NiPoint3_ZERO, 0.0f, 0.0f);
			} else {
				v7 = 0;
			}
			v10 = v7;
		}
	}
	if (v10) {
		if (apSource) {
			if (apSource->IsActor()) {
				ActorCause* v6;
				ActorCause* pActorCause = static_cast<ActorCause*>(ourMalloc(sizeof(ActorCause)));
				if (pActorCause) {
					v6 = ::new (pActorCause) ActorCause(apSource);
				} else {
					v6 = 0;
				}
				v10->SetActorCause(v6);
			}
		}
		v10->SetFlag(Projectile::ACTIVE_IN_INVENTORY, 1);
		v10->pDesiredTarget = apTarget;
		v10->Initialize();
		float* v4 = &fProjectileInventoryGrenadeTimer.Float();
		v10->SetExplosionTimer(*v4);
	}
	return v10;
}

float TESObjectWEAP::GetShotsPerSecond() const {
	return data.fShotsPerSec;
}
