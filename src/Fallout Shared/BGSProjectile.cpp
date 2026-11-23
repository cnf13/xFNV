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
#include "xfnv/Compiler.h"

// Reads the DATA record, the muzzle-flash model (NAM1's path through the model's virtual setter, NAM2's texture chunk),
// the destruction chunks, the full name, the model chunks and the sound level. GetTESForm's byte is stored and compared
// zero-extended, hence the uint8_t.
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
                // LoadData takes the record as raw bytes; the chunk is read straight into the DATA member.
                LoadData(apFile, reinterpret_cast<char*>(&Data), 0x54);
                // The file's record is 0x54 bytes, with a 4-byte form ID in each pointer field.
                TESFile::WidenFormIDRecords(
                    &Data,
                    1,
                    0x54,
                    sizeof(Data),
                    {{0x10, offsetof(BGSProjectileData, pLight)},
                     {0x14, offsetof(BGSProjectileData, pMuzzleFlashLight)},
                     {0x24, offsetof(BGSProjectileData, pExplosionType)},
                     {0x28, offsetof(BGSProjectileData, pActiveSoundLoop)},
                     {0x38, offsetof(BGSProjectileData, pCountdownSound)},
                     {0x3C, offsetof(BGSProjectileData, pDeactivateSound)},
                     {0x40, offsetof(BGSProjectileData, pDefaultWeaponSource)}}
                );
                break;
            case NAM1_ID: {
                if (apFile->GetChunkSize()) {
                    // GetChunkSize is called a second time for the buffer, which is a 16-byte aligned stack buffer of
                    // the chunk's size.
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                    apFile->GetChunkData(pBuffer, 0);
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
                // A 16-byte aligned buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // Cleared first because an empty chunk leaves the buffer unwritten; the game itself does not clear it.
                pEditorID[0] = '\0';
                apFile->GetChunkData(pEditorID, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile); break;
            case VNAM_ID: {
                uint32_t uiSoundLevel = 0;
                apFile->GetChunkData(uiSoundLevel);
                // The compare is unsigned: above 3 the sound level falls back to 1.
                if (uiSoundLevel > 3) {
                    eSoundLevel = SOUND_LEVEL_NORMAL;
                }
                else {
                    eSoundLevel = static_cast<SOUND_LEVEL>(uiSoundLevel);
                }
                break;
            }
            case MODL_ID:
            case MODT_ID: {
                TESModel* pModel = static_cast<TESModel*>(this);
                TESModel::LoadModelChunk(pModel, apFile);
                break;
            }
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
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
    StartForm();
    TESFullName::Save();
    TESModel::Save(MODL_ID, MODT_ID);
    BGSDestructibleObjectForm::Save();
    TESObjectLIGH* pLight = Data.pLight;
    TESObjectLIGH* pLightID = nullptr;
    if (pLight) {
        // A uint32_t form ID is stored as a TESObjectLIGH*, deliberately.
        pLightID = reinterpret_cast<TESObjectLIGH*>(static_cast<uintptr_t>(Data.pLight->iFormID));
    }
    Data.pLight = pLightID;
    TESObjectLIGH* pMuzzleFlashLight = Data.pMuzzleFlashLight;
    TESObjectLIGH* pMuzzleFlashLightID = nullptr;
    if (pMuzzleFlashLight) {
        // A uint32_t form ID is stored as a TESObjectLIGH*, deliberately.
        pMuzzleFlashLightID = reinterpret_cast<TESObjectLIGH*>(static_cast<uintptr_t>(Data.pMuzzleFlashLight->iFormID));
    }
    Data.pMuzzleFlashLight = pMuzzleFlashLightID;
    BGSExplosion* psaveMuzzleLight = Data.pExplosionType;
    BGSExplosion* pExplosionID = nullptr;
    if (psaveMuzzleLight) {
        // Stores a uint32_t form ID in a BGSExplosion* field.
        pExplosionID = reinterpret_cast<BGSExplosion*>(static_cast<uintptr_t>(Data.pExplosionType->iFormID));
    }
    Data.pExplosionType = pExplosionID;
    TESSound* pActiveSoundLoop = Data.pActiveSoundLoop;
    TESSound* pActiveSoundLoopID = nullptr;
    if (pActiveSoundLoop) {
        // Stores a uint32_t form ID in a TESSound* field.
        pActiveSoundLoopID = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(Data.pActiveSoundLoop->iFormID));
    }
    Data.pActiveSoundLoop = pActiveSoundLoopID;
    TESSound* psaveActiveSound = Data.pCountdownSound;
    TESSound* pCountdownSoundID = nullptr;
    if (psaveActiveSound) {
        // Stores a uint32_t form ID in a TESSound* field.
        pCountdownSoundID = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(Data.pCountdownSound->iFormID));
    }
    Data.pCountdownSound = pCountdownSoundID;
    TESSound* pDeactivateSound = Data.pDeactivateSound;
    TESSound* pDeactivateSoundID = nullptr;
    if (pDeactivateSound) {
        // Stores a uint32_t form ID in a TESSound* field.
        pDeactivateSoundID = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(Data.pDeactivateSound->iFormID));
    }
    Data.pDeactivateSound = pDeactivateSoundID;
    TESObjectWEAP* pDefaultWeaponSource = Data.pDefaultWeaponSource;
    TESObjectWEAP* pWeaponSourceID = nullptr;
    if (pDefaultWeaponSource) {
        // Stores a form ID (uint32_t) in the TESObjectWEAP* slot.
        pWeaponSourceID = reinterpret_cast<TESObjectWEAP*>(static_cast<uintptr_t>(Data.pDefaultWeaponSource->iFormID));
    }
    Data.pDefaultWeaponSource = pWeaponSourceID;
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    // The file keeps the form IDs as 4 bytes; pack the record to its layout for the write and restore it after.
    TESFile::NarrowFormIDRecords(
        &Data,
        1,
        0x54,
        sizeof(Data),
        {{0x10, offsetof(BGSProjectileData, pLight)},
         {0x14, offsetof(BGSProjectileData, pMuzzleFlashLight)},
         {0x24, offsetof(BGSProjectileData, pExplosionType)},
         {0x28, offsetof(BGSProjectileData, pActiveSoundLoop)},
         {0x38, offsetof(BGSProjectileData, pCountdownSound)},
         {0x3C, offsetof(BGSProjectileData, pDeactivateSound)},
         {0x40, offsetof(BGSProjectileData, pDefaultWeaponSource)}}
    );
    __SaveData(&Data, 84u);
    TESFile::WidenFormIDRecords(
        &Data,
        1,
        0x54,
        sizeof(Data),
        {{0x10, offsetof(BGSProjectileData, pLight)},
         {0x14, offsetof(BGSProjectileData, pMuzzleFlashLight)},
         {0x24, offsetof(BGSProjectileData, pExplosionType)},
         {0x28, offsetof(BGSProjectileData, pActiveSoundLoop)},
         {0x38, offsetof(BGSProjectileData, pCountdownSound)},
         {0x3C, offsetof(BGSProjectileData, pDeactivateSound)},
         {0x40, offsetof(BGSProjectileData, pDefaultWeaponSource)}}
    );
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
    }
    else {
        uint32_t uiLength = MuzzleFlashModel.GetModelLength() + 1;
        const char* pModel = MuzzleFlashModel.GetModel();
        TESForm::AddChunkArray(NAM1_ID, pModel, uiLength);
    }
    TESForm::AddChunk(VNAM_ID, eSoundLevel);
    CloseForm();
}

// Load leaves each form's ID in its pointer field; InitItem replaces it with the form.
void BGSProjectile::InitItem() {
    if (!GetInitialized()) {
        BGSDestructibleObjectForm::InitItem(this);
        uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pLight));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            Data.pLight = dynamic_cast<TESObjectLIGH*>(pForm);
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pMuzzleFlashLight));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            Data.pMuzzleFlashLight = dynamic_cast<TESObjectLIGH*>(pForm);
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetExplosion()));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            Data.pExplosionType = dynamic_cast<BGSExplosion*>(pForm);
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pActiveSoundLoop));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            Data.pActiveSoundLoop = dynamic_cast<TESSound*>(pForm);
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pCountdownSound));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            Data.pCountdownSound = dynamic_cast<TESSound*>(pForm);
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pDeactivateSound));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            Data.pDeactivateSound = dynamic_cast<TESSound*>(pForm);
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetDefaultWeaponSource()));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            Data.pDefaultWeaponSource = dynamic_cast<TESObjectWEAP*>(pForm);
        }
        if (!GetHasExplosion()) {
            Data.pExplosionType = nullptr;
        }
        SetInitialized(true);
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
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (std::memcmp(&Data, &pcomp->Data, sizeof(Data))) {
        return true;
    }
    if (MuzzleFlashModel.CompareComponent(&pcomp->MuzzleFlashModel)) {
        return true;
    }
    return eSoundLevel != pcomp->eSoundLevel;
}

bool BGSProjectile::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aiCount
) {
    Actor* pActor = dynamic_cast<Actor*>(apActionRef);
    Projectile* pProjectile = dynamic_cast<Projectile*>(apItemActivated);
    if (!pActor || !pProjectile) {
        return false;
    }
    BGSProjectile* pProjectileBase = pProjectile->GetProjectileBase();
    if (pProjectileBase->GetCanTurnOff() &&
        (!pProjectile->IsFlagSet(Projectile::TURNED_OFF) && pProjectile->TurnOff(pActor, false))) {
        return true;
    }
    pProjectileBase = pProjectile->GetProjectileBase();
    return pProjectileBase->GetCanPickup() &&
        TESBoundObject::Activate(apItemActivated, apActionRef, abIdFlag, apObjecttoGet, aiCount);
}

NiAVObject* BGSProjectile::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
    NiAVObject* pav = nullptr;
    if (pRequester) {
        int32_t iMotionType = GetMotionType();
        if (iMotionType == MOTION_BEAM || iMotionType == MOTION_CONTINUOUS_BEAM) {
            pav = TESBoundObject::Clone3D(pRequester, true);
        }
        else {
            Projectile* pprojectile = static_cast<Projectile*>(pRequester);
            if (pprojectile->pSecondaryItemChange && pprojectile->GetSecondaryItemChange()->pContainerObj) {
                TESBoundObject* pContainerObj = pprojectile->GetSecondaryItemChange()->pContainerObj;
                NiAVObject* ploadedArt = pContainerObj->LoadGraphics(nullptr);
                BSCloningProcess cloneData(1.0f);
                if (ploadedArt) {
                    pav = static_cast<NiAVObject*>(ploadedArt->Clone(cloneData));
                }
            }
            else {
                NiAVObject* ploadedArt = LoadGraphics(pRequester);
                if (ploadedArt) {
                    uint32_t uiFormID = pRequester->iFormID;
                    if (pDataHandler->IsFormIDCreated(uiFormID) && !pSaveLoadGame->GetSaveGameLoading()) {
                        pav = BSClonePools::GetInstance(true)->RequestClone(ploadedArt);
                    }
                }
                if (!pav) {
                    pav = TESBoundObject::Clone3D(pRequester, deepCopy);
                    if (ploadedArt) {
                        TESObject::UnClone3D(pRequester);
                    }
                }
            }
        }
    }
    if (pav && pav->IsFadeNode()) {
        pav->IsFadeNode()->SetIgnoreFade(true);
    }

    return pav;
}

void BGSProjectile::UnClone3D(TESObjectREFR* apRequester) {
    if (apRequester) {
        uint32_t uiMotionType = GetMotionType();
        if (uiMotionType == MOTION_BEAM ||
            uiMotionType == MOTION_CONTINUOUS_BEAM ||
            !static_cast<Projectile*>(apRequester)->pSecondaryItemChange ||
            !static_cast<Projectile*>(apRequester)->pSecondaryItemChange->ItemChange::GetItemChangeObject()) {
            TESObject::UnClone3D(apRequester);
        }
        else {
            TESObject* pObject = static_cast<Projectile*>(apRequester)
                                     ->pSecondaryItemChange->ItemChange::GetItemChangeObject();
            const char* pModelFileName = pObject->GetModelFileName(nullptr);
            pModelLoader->ReleaseFile(pModelFileName);
        }
    }
}

void BGSProjectile::QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    if (GetModelLength()) {
        TESModel* pModel;
        if (this == nullptr) {
            pModel = nullptr;
        }
        else {
            pModel = static_cast<TESModel*>(this);
        }
        ENUM_LOD_MULT eLODMult = TES::GetLODMult(this);
        pModelLoader->QueueModel(pModel, aePriority, apParent, eLODMult, true, false, false);
    }
    if (MuzzleFlashModel.GetModelLength()) {
        pModelLoader->QueueModel(&MuzzleFlashModel, aePriority, apParent, LOD_MULT_ACTORS, true, false, false);
    }
    if (GetExplosion()) {
        BGSPreloadable* pExplosion = static_cast<BGSPreloadable*>(GetExplosion());
        pExplosion->QueueFiles(aePriority, apParent);
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
// weapon, else three times the shots per volley capped by the clip (except for arcing ones), times 4 when pinning
// limbs.
int32_t BGSProjectile::GetPreCloneCount(TESObjectWEAP* apSourceWeapon) {
    int32_t iCount = 5;
    if (GetMotionType() == MOTION_BEAM ||
        GetMotionType() == MOTION_CONTINUOUS_BEAM ||
        GetMotionType() == MOTION_FLAMETHROWER) {
        iCount = 0;
    }
    else if (apSourceWeapon) {
        int32_t iShots = static_cast<int32_t>(apSourceWeapon->GetShotsPerSecond()) *
            apSourceWeapon->GetNumProjectiles(false, false, nullptr);
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
        return 1.0f;
    }

    return Data.fGravity;
}

ENUM_FORM_ID BGSProjectile::GetProjectileRefFormType() {
    int32_t projFlags = GetMotionType();
    if (projFlags > 262144) {
        if (projFlags == 524288) {
            return PFLA_ID;
        }
        if (projFlags == 1048576) {
            return PCBE_ID;
        }
    }
    else {
        switch (projFlags) {
            case 262144: {
                return PBEA_ID;
            }
            case 65536: {
                return PMIS_ID;
            }
            case 131072: {
                return PGRE_ID;
            }
        }
    }
    return NO_FORM;
}

bool BGSProjectile::GetPassThroughSmTransp() {
    return IsFlagSet(PASS_SM_TRANSPARENT);
}

bool BGSProjectile::DecideIsTracer() {
    uint32_t uiTracerChance = static_cast<uint32_t>(static_cast<int32_t>(GetTracerChance() * 100.0));
    return Random() % 100 < uiTracerChance;
}

float BGSProjectile::GetTracerChance() {
    return Data.fTracerChance;
}

GrenadeProjectile* BGSProjectile::InitLiveGrenade(
    BGSProjectile* apProjectile,
    Actor* apSource,
    TESObjectREFR* apTarget,
    TESObjectWEAP* apGrenade
) {
    GrenadeProjectile* pGrenade = nullptr;
    if (apProjectile && apProjectile->GetMotionType() == BGSProjectile::MOTION_GRENADE) {
        pGrenade = new GrenadeProjectile(apProjectile, apSource, apGrenade, NiPoint3::ZERO, 0.0f, 0.0f);
    }

    if (pGrenade) {
        if (apSource && apSource->IsActor()) {
            ActorCause* pCause = new ActorCause(apSource);
            pGrenade->SetActorCause(pCause);
        }

        pGrenade->SetFlag(Projectile::ACTIVE_IN_INVENTORY, true);
        pGrenade->pDesiredTarget = apTarget;
        pGrenade->Initialize();
        pGrenade->SetExplosionTimer(fProjectileInventoryGrenadeTimer.Float());
    }
    return pGrenade;
}

float TESObjectWEAP::GetShotsPerSecond() const {
    return data.fShotsPerSec;
}
