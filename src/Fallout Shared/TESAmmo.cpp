#include "Fallout Shared/TESAmmo.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/BGSClipRoundsForm.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/TESAmmoEffect.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> iMaxArrowsInQuiver("iMaxArrowsInQuiver", 20);

bool TESAmmo::Load(TESFile* apFile) {
    uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (cFormType != AMMO_ID) {
        return false;
    }
    GetAmmoEffectList()->RemoveAll();
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case MODL_ID:
            case MODD_ID:
            case MOSD_ID:
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModelTextureSwap*>(this), apFile); break;
            case MODS_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
                apFile->GetChunkData(pBuffer, uiChunkSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)
                    ->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, usFormVersion);
                break;
            }
            case ICON_ID: TESTexture::LoadTextureChunk(static_cast<TESIcon*>(this), apFile); break;
            case MICO_ID: BGSMessageIcon::LoadMessageIcon(static_cast<BGSMessageIcon*>(this), apFile); break;
            case SCRI_ID: {
                uint32_t uiScriptID = 0;
                apFile->GetChunkData<uint32_t>(uiScriptID);
                TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
                // The chunk holds a form ID that stands in for the script until the item is initialised.
                pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
                pScriptableForm->InitItem(this);
                break;
            }
            case DEST_ID:
            case DSTD_ID:
                BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
                break;
            case DATA_ID:
                // LoadData takes the chunk's raw bytes, which here are the AMMO_DATA struct.
                LoadData(apFile, reinterpret_cast<char*>(&data), sizeof(data));
                break;
            case DAT2_ID:
                // The chunk's raw bytes are the AMMO_DATA_NV struct.
                apFile->GetChunkData(reinterpret_cast<char*>(&NVdata), 0x14);
                // The file's record is 0x14 bytes, with 4-byte form IDs at 4 and 0xC.
                TESFile::WidenFormIDRecords(
                    &NVdata,
                    1,
                    0x14,
                    sizeof(NVdata),
                    {{4, offsetof(AMMO_DATA_NV, pProjectileOverride)},
                     {0xC, offsetof(AMMO_DATA_NV, pAmmoConsumedObject)}}
                );
                if (apFile->QEndian()) {
                    NVdata.Endian();
                }
                break;
            case ONAM_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                cDesc.Set(pBuffer, 0);
                break;
            }
            case QNAM_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                cAbbrev.Set(pBuffer, 0);
                break;
            }
            case YNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData<uint32_t>(uiSoundID);
                // The chunk holds a form ID that stands in for the sound until the item is initialised.
                static_cast<BGSPickupPutdownSounds*>(this)->SetPickupSound(
                    reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID))
                );
                break;
            }
            case ZNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData<uint32_t>(uiSoundID);
                // The chunk holds a form ID that stands in for the sound until the item is initialised.
                static_cast<BGSPickupPutdownSounds*>(this)->SetPutdownSound(
                    reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID))
                );
                break;
            }
            case RCIL_ID: {
                uint32_t uiEffectID = 0;
                apFile->GetChunkData<uint32_t>(uiEffectID);
                // The chunk holds a form ID that stands in for the effect until the item is initialised.
                TESAmmoEffect* pEffect = reinterpret_cast<TESAmmoEffect*>(static_cast<uintptr_t>(uiEffectID));
                GetAmmoEffectList()->AddTail(pEffect);
                break;
            }
        }
        if (uiChunk == DATA_ID && apFile->QEndian()) {
            data.Endian();
        }
    }
    while (apFile->NextChunk());
    return true;
}

void AMMO_DATA::Endian() {
    ENDIAN(fSpeed, false);
}

void AMMO_DATA_NV::Endian() {
    ENDIAN(fWeightOverride, false);
    ENDIAN(fAmmoConsumedChance, false);
}

TESAmmo::TESAmmo() {
    SetFormType(AMMO_ID);
    TESAmmo::InitializeData();
}

TESAmmo::~TESAmmo() {}

// Zeroes both data blocks and empties the short name and the abbreviation.
void TESAmmo::InitializeData() {
    ourMemset(&data, 0, sizeof(data));
    ourMemset(&NVdata, 0, sizeof(NVdata));
    cDesc.Set(nullptr, 0);
    cAbbrev.Set(nullptr, 0);
}

void TESAmmo::Save() {
    StartForm();
    TESFullName::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    TESTexture::Save(ICON_ID);
    BGSMessageIcon::Save(MICO_ID);
    TESScriptableForm::Save();
    BGSDestructibleObjectForm::Save();
    BGSPickupPutdownSounds::Save();
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    __SaveData(&data, sizeof(AMMO_DATA));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    BGSProjectile* psaveProjectile = NVdata.pProjectileOverride;
    TESForm* psaveAmmoConsumed = NVdata.pAmmoConsumedObject;
    // The file holds form IDs where memory holds the pointers: for the write, each pointer field holds its form's ID,
    // then the pointers are put back.
    BGSProjectile* pProjectileID;
    if (psaveProjectile) {
        pProjectileID = reinterpret_cast<BGSProjectile*>(static_cast<uintptr_t>(NVdata.pProjectileOverride->iFormID));
    }
    else {
        pProjectileID = nullptr;
    }
    NVdata.pProjectileOverride = pProjectileID;
    TESForm* pAmmoConsumedID;
    if (NVdata.pAmmoConsumedObject) {
        pAmmoConsumedID = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(NVdata.pAmmoConsumedObject->iFormID));
    }
    else {
        pAmmoConsumedID = nullptr;
    }
    NVdata.pAmmoConsumedObject = pAmmoConsumedID;
    if (TESForm::QEndianSwapOnSave()) {
        NVdata.Endian();
    }
    // The file's record is Load's 0x14 bytes, with 4-byte form IDs at 4 and 0xC; the native layout is spread back
    // after the write.
    TESFile::NarrowFormIDRecords(
        &NVdata,
        1,
        0x14,
        sizeof(NVdata),
        {{4, offsetof(AMMO_DATA_NV, pProjectileOverride)}, {0xC, offsetof(AMMO_DATA_NV, pAmmoConsumedObject)}}
    );
    TESForm::__AddChunkData(DAT2_ID, &NVdata, 0x14);
    TESFile::WidenFormIDRecords(
        &NVdata,
        1,
        0x14,
        sizeof(NVdata),
        {{4, offsetof(AMMO_DATA_NV, pProjectileOverride)}, {0xC, offsetof(AMMO_DATA_NV, pAmmoConsumedObject)}}
    );
    if (TESForm::QEndianSwapOnSave()) {
        NVdata.Endian();
    }
    NVdata.pProjectileOverride = psaveProjectile;
    NVdata.pAmmoConsumedObject = psaveAmmoConsumed;
    if (cDesc.GetLength()) {
        uint32_t uiLength = cDesc.GetLength();
        TESForm::AddChunkArray(ONAM_ID, cDesc.pString, uiLength + 1);
    }
    if (cAbbrev.GetLength()) {
        uint32_t uiLength = cAbbrev.GetLength();
        TESForm::AddChunkArray(QNAM_ID, cAbbrev.pString, uiLength + 1);
    }
    BSSimpleList<TESAmmoEffect*>* pAmmoEffectList = GetAmmoEffectList();
    while (pAmmoEffectList && pAmmoEffectList->m_item) {
        TESAmmoEffect* pAmmoEffect = pAmmoEffectList->m_item;
        pAmmoEffectList = pAmmoEffectList->m_pkNext;
        TESForm::AddChunk(RCIL_ID, pAmmoEffect->iFormID);
    }
    CloseForm();
}

uint16_t TESAmmo::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
    return static_cast<uint16_t>(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
}

void TESAmmo::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    TESValueForm::SaveGame(aiFlags);
}

void TESAmmo::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    static_cast<TESValueForm*>(this)->SaveGame(apSaveGameBuffer);
}

void TESAmmo::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESAmmo::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    static_cast<TESValueForm*>(this)->LoadGame(apLoadGameBuffer);
}

// Until InitItem runs, the projectile, the consumed object and each ammo effect hold the form IDs Load read; each is
// resolved to its form here.
void TESAmmo::InitItem() {
    if (!GetInitialized()) {
        TESScriptableForm::InitItem(this);
        BGSDestructibleObjectForm::InitItem(this);
        TESModelTextureSwap::InitItem(this);
        BGSPickupPutdownSounds::InitItem(this);
        uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetProjectile()));
        if (itempInt) {
            TESForm::AddCompileIndex(itempInt, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            SetProjectile(dynamic_cast<BGSProjectile*>(pForm));
            if (!GetProjectile()) {
                int32_t iAmmoID = iFormID;
                Warning(
                    "MASTERFILE: Could not find projectile (%08X) on ammo '%s' (%08X).",
                    itempInt,
                    GetFormEditorID(),
                    iAmmoID
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetAmmoConsumedForm()));
        if (itempInt) {
            TESForm::AddCompileIndex(itempInt, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            SetAmmoConsumedForm(pForm);
            if (!GetAmmoConsumedForm()) {
                int32_t iAmmoID = iFormID;
                Warning(
                    "MASTERFILE: Could not find ammo consumed form (%08X) on ammo '%s' (%08X).",
                    itempInt,
                    GetFormEditorID(),
                    iAmmoID
                );
            }
        }
        BSSimpleList<TESAmmoEffect*>* pAmmoEffectList = GetAmmoEffectList();
        while (pAmmoEffectList && pAmmoEffectList->m_item) {
            itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAmmoEffectList->m_item));
            if (itempInt) {
                TESForm::AddCompileIndex(itempInt, GetFile(-1));
                TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
                TESAmmoEffect* pAmmoEffect = dynamic_cast<TESAmmoEffect*>(pForm);
                if (!pAmmoEffect) {
                    int32_t iAmmoID = iFormID;
                    Warning(
                        "MASTERFILE: Could not find Ammo Effect object (%08X) for ammo '%s' (%08X).",
                        itempInt,
                        GetFormEditorID(),
                        iAmmoID
                    );
                }
                pAmmoEffectList->m_item = pAmmoEffect;
            }
            pAmmoEffectList = pAmmoEffectList->m_pkNext;
        }
        SetInitialized(true);
    }
}

void TESAmmo::Copy(TESForm* apCopy) {
    TESAmmo* pCopy = dynamic_cast<TESAmmo*>(apCopy);
    if (pCopy) {
        CopyAllComponents(apCopy);
        ourMemcpy(&data, &pCopy->data, sizeof(AMMO_DATA));
        AMMO_DATA_NV* pSourceNVdata = &pCopy->NVdata;
        ourMemcpy(&NVdata, pSourceNVdata, sizeof(AMMO_DATA_NV));
        cFormType = pCopy->GetFormType();
        cDesc.Set(pCopy->cDesc.pString, 0);
        cAbbrev.Set(pCopy->cAbbrev.pString, 0);
        while (!GetAmmoEffectList()->IsEmpty()) {
            GetAmmoEffectList()->RemoveHead();
        }
        for (BSSimpleList<TESAmmoEffect*>* pIter = pCopy->GetAmmoEffectList(); pIter && !pIter->IsEmpty();
             pIter = pIter->GetNext()) {
            TESAmmoEffect* pAmmoEffect = pIter->GetItem();
            GetAmmoEffectList()->AddTail(pAmmoEffect);
        }
    }
}

bool TESAmmo::Compare(TESForm* apCompare) {
    TESAmmo* pComp = dynamic_cast<TESAmmo*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (std::memcmp(&data, &pComp->data, sizeof(data))) {
        return true;
    }
    if (std::memcmp(&NVdata, &pComp->NVdata, sizeof(NVdata))) {
        return true;
    }
    if (cAbbrev.StrCmp(pComp->cAbbrev, true)) {
        return true;
    }
    if (cDesc.StrCmp(pComp->cDesc, true)) {
        return true;
    }
    int32_t iCount = GetAmmoEffectList()->ItemsInList();
    if (iCount != pComp->GetAmmoEffectList()->ItemsInList()) {
        return true;
    }
    BSSimpleList<TESAmmoEffect*>* pAmmoEffects = GetAmmoEffectList();
    BSSimpleList<TESAmmoEffect*>* pCompAmmoEffects = pComp->GetAmmoEffectList();
    while (pCompAmmoEffects && !pCompAmmoEffects->IsEmpty()) {
        if (!pAmmoEffects || pAmmoEffects->IsEmpty()) {
            return true;
        }
        TESAmmoEffect* pAmmoEffect = pAmmoEffects->m_item;
        if (pAmmoEffect->Compare(pCompAmmoEffects->m_item)) {
            return true;
        }
        pCompAmmoEffects = pCompAmmoEffects->m_pkNext;
        pAmmoEffects = pAmmoEffects->m_pkNext;
    }
    return false;
}

NiAVObject* TESAmmo::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
    int32_t icount;
    NiAVObject* pobj = TESBoundObject::Clone3D(pRequester, deepCopy);
    if (pRequester) {
        icount = pRequester->m_Extra.GetCount();
    }
    else {
        icount = 1;
    }
    if (pobj && icount < iMaxArrowsInQuiver.Int()) {
        NiAVObject* pchild;
        if (icount == 1) {
            pchild = pobj->GetObjectByName(FixedStrings::QArrowQuiver());
            if (pchild) {
                pchild->m_pkParent->DetachChild(pchild);
            }
            bhkRigidBody::RemoveAllActionConstraints(pobj);
        }
        else {
            for (int32_t n = icount; n < iMaxArrowsInQuiver.Int(); ++n) {
                BSStringT<char> arrowName;
                arrowName.SPrintF("Arrow%d", n);
                pchild = pobj->GetObjectByName(static_cast<const char*>(arrowName.pString));
                if (pchild) {
                    pchild->m_pkParent->DetachChild(pchild);
                }
            }
        }
    }

    return pobj;
}

const char* TESAmmo::GetShortName() {
    return cDesc.pString;
}

void TESAmmo::BuildMenuString(char* astrResult, uint32_t auiResultLen) {
    char cEffectString[260];
    bool bNeedNewLine = false;
    *astrResult = 0;
    cEffectString[0] = 0;
    BSSimpleList<TESAmmoEffect*>* pAmmoEffectList = GetAmmoEffectList();
    while (pAmmoEffectList && pAmmoEffectList->m_item) {
        pAmmoEffectList->m_item->BuildMenuString(cEffectString, 256u);
        TESAmmoEffect* pEffect = pAmmoEffectList->m_item;
        pAmmoEffectList = pAmmoEffectList->m_pkNext;
        pEffect->BuildMenuString(cEffectString, 256u);
        if (BSstrlen(cEffectString)) {
            if (bNeedNewLine) {
                BSstrcat(astrResult, static_cast<int32_t>(auiResultLen), "\n");
                BSstrcat(astrResult, static_cast<int32_t>(auiResultLen), cEffectString);
            }
            else {
                BSstrcat(astrResult, static_cast<int32_t>(auiResultLen), cEffectString);
                bNeedNewLine = true;
            }
        }
    }
}

const char* TESAmmo::GetAbbrev() {
    return cAbbrev.pString;
}

BGSProjectile* TESAmmo::GetProjectile() {
    return NVdata.pProjectileOverride;
}

BSSimpleList<TESAmmoEffect*>* TESAmmo::GetAmmoEffectList() {
    return &pAmmoEffectList;
}

void TESAmmo::SetAmmoConsumedForm(TESForm* apAmmoConsumed) {
    NVdata.pAmmoConsumedObject = apAmmoConsumed;
}

NiFixedString& FixedStrings::QArrowQuiver() {
    return *FixedStrings::ArrowQuiver;
}
