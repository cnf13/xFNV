#include "Fallout Shared/Magic/EffectSetting.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// Reads the name, model, description, icon, editor ID, object bound and DATA chunks and the counter effects (ESCE). The
// DATA record is cleared and given a speed of 1 before it is read.
bool EffectSetting::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != MGEF_ID) {
        return false;
    }
    LoadForm(apFile);
    for (;;) {
        CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }

        switch (eChunk) {
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(static_cast<TESIcon*>(this), apFile); break;
            case DESC_ID: TESDescription::LoadDescriptionData(static_cast<TESDescription*>(this), apFile); break;
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pEditorID, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case DATA_ID:
                // The whole record: the file's 0x48 bytes are the 32-bit record.
                ourMemset(&data, 0, sizeof(data));
                data.fSpeed = 1.0f;
                apFile->GetChunkData(&data, 0x48);
                // The file's record is 0x48 bytes (fewer in older files), with a 4-byte form ID in each pointer field.
                TESFile::WidenFormIDRecords(
                    &data,
                    1,
                    apFile->GetChunkSize() < 0x48 ? apFile->GetChunkSize() : 0x48,
                    sizeof(data),
                    {{8, offsetof(EffectSettingData, pAssociatedItem)},
                     {0x18, offsetof(EffectSettingData, pLight)},
                     {0x20, offsetof(EffectSettingData, pEffectShader)},
                     {0x24, offsetof(EffectSettingData, pEnchantEffect)},
                     {0x28, offsetof(EffectSettingData, pCastSound)},
                     {0x2C, offsetof(EffectSettingData, pBoltSound)},
                     {0x30, offsetof(EffectSettingData, pHitSound)},
                     {0x34, offsetof(EffectSettingData, pAreaSound)}}
                );
                if (apFile->QEndian()) {
                    data.Endian();
                }
                SetFlag(PLAYER_KNOWS, false);
                break;
            case ESCE_ID: {
                uint32_t uiID = 0;
                apFile->GetChunkData<uint32_t>(uiID);
                if (apFile->QEndian()) {
                    ENDIAN(uiID, false);
                }
                TESForm::AddCompileIndex(uiID, apFile);
                TESForm* pForm = TESForm::GetFormByNumericID(uiID);
                EffectSetting* pCounterEffect = dynamic_cast<EffectSetting*>(pForm);
                if (pCounterEffect) {
                    counterEffects.AddHead(pCounterEffect);
                    int32_t iNumCounterEffects = counterEffects.ItemsInList();
                    data.iNumCounterEffects = static_cast<int16_t>(iNumCounterEffects);
                }
                else {
                    Warning(
                        "FORMS: Invalid form ID (%d) for effect setting found while loading counter effects for %s.",
                        uiID,
                        static_cast<TESForm*>(this)->GetFormEditorID()
                    );
                }
                break;
            }
        }

        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

float NiAbs(float fValue) {
    return std::fabs(fValue);
}

void EffectSetting::EffectSettingData::Endian() {
    ENDIAN<uint32_t>(iFlags, false);
    ENDIAN<float>(fBaseCost, false);
    ENDIAN<ActorValue::Index>(eAssociatedActorValue, false);
    ENDIAN<TESForm*>(pAssociatedItem, false);
    ENDIAN<ActorValue::Index>(iAssociatedSkill, false);
    ENDIAN<ActorValue::Index>(iResistVariable, false);
    ENDIAN<int16_t>(iNumCounterEffects, false);
    ENDIAN<TESObjectLIGH*>(pLight, false);
    ENDIAN<float>(fSpeed, false);
    ENDIAN<TESEffectShader*>(pEffectShader, false);
    ENDIAN<TESEffectShader*>(pEnchantEffect, false);
    ENDIAN<TESSound*>(pCastSound, false);
    ENDIAN<TESSound*>(pBoltSound, false);
    ENDIAN<TESSound*>(pHitSound, false);
    ENDIAN<TESSound*>(pAreaSound, false);
    ENDIAN<float>(fCEEnchantFactor, false);
    ENDIAN<float>(fCEBarterFactor, false);
    ENDIAN<EffectArchetypes::ArchetypeID>(eArchetype, false);
}

EffectSetting::EffectSetting()
  : TESForm(), TESModel(), TESDescription(), TESFullName(), TESIcon(), pFilterValidationFunction(nullptr),
    pFilterValidationItem(nullptr), counterEffects(), iEffectLoadedCount(0), iAssociatedItemLoadedCount(0) {
    data.iFlags = 0;
    data.fBaseCost = 0.0f;
    data.pAssociatedItem = nullptr;
    data.eAssociatedActorValue = ActorValue::VALUE_NONE;
    data.iAssociatedSkill = ActorValue::VALUE_NONE;
    data.iResistVariable = ActorValue::VALUE_NONE;
    data.iNumCounterEffects = 0;
    data.pLight = nullptr;
    data.pEffectShader = nullptr;
    data.pEnchantEffect = nullptr;
    data.fSpeed = 1.0f;
    data.pCastSound = nullptr;
    data.pBoltSound = nullptr;
    data.pHitSound = nullptr;
    data.pAreaSound = nullptr;
    data.fCEEnchantFactor = fMagicDefaultCEEnchantFactor.Float();
    data.fCEBarterFactor = fMagicDefaultCEBarterFactor.Float();
    data.eArchetype = EffectArchetypes::ARCHETYPE_START;
    cFormType = MGEF_ID;
}

EffectSetting::~EffectSetting() {}

// The DATA record is written with the form IDs in its form pointers; the pointers are put back after the chunk is
// added.
void EffectSetting::Save() {
    StartForm();
    TESFullName::Save();
    TESDescription::Save();
    TESTexture::Save(ICON_ID);
    TESModel::Save(MODL_ID, MODT_ID);
    TESSound* pCastSound = data.pCastSound;
    data.pCastSound = pCastSound ? reinterpret_cast<TESSound*>(static_cast<uintptr_t>(data.pCastSound->iFormID))
                                 : nullptr;
    TESSound* pBoltSound = data.pBoltSound;
    data.pBoltSound = pBoltSound ? reinterpret_cast<TESSound*>(static_cast<uintptr_t>(data.pBoltSound->iFormID))
                                 : nullptr;
    TESSound* pHitSound = data.pHitSound;
    data.pHitSound = pHitSound ? reinterpret_cast<TESSound*>(static_cast<uintptr_t>(data.pHitSound->iFormID)) : nullptr;
    TESSound* pAreaSound = data.pAreaSound;
    data.pAreaSound = pAreaSound ? reinterpret_cast<TESSound*>(static_cast<uintptr_t>(data.pAreaSound->iFormID))
                                 : nullptr;
    TESEffectShader* pEffectShader = data.pEffectShader;
    data.pEffectShader = pEffectShader
        ? reinterpret_cast<TESEffectShader*>(static_cast<uintptr_t>(data.pEffectShader->iFormID))
        : nullptr;
    TESEffectShader* pEnchantEffect = data.pEnchantEffect;
    data.pEnchantEffect = pEnchantEffect
        ? reinterpret_cast<TESEffectShader*>(static_cast<uintptr_t>(data.pEnchantEffect->iFormID))
        : nullptr;
    TESObjectLIGH* pLight = data.pLight;
    data.pLight = pLight ? reinterpret_cast<TESObjectLIGH*>(static_cast<uintptr_t>(data.pLight->iFormID)) : nullptr;
    TESForm* pAssociatedItem = data.pAssociatedItem;
    data.pAssociatedItem = pAssociatedItem
        ? reinterpret_cast<TESForm*>(static_cast<uintptr_t>(data.pAssociatedItem->iFormID))
        : nullptr;
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    // The file's record is Load's 0x48 bytes, with a 4-byte form ID in each pointer field; the native layout is
    // spread back after the write.
    TESFile::NarrowFormIDRecords(
        &data,
        1,
        0x48,
        sizeof(data),
        {{8, offsetof(EffectSettingData, pAssociatedItem)},
         {0x18, offsetof(EffectSettingData, pLight)},
         {0x20, offsetof(EffectSettingData, pEffectShader)},
         {0x24, offsetof(EffectSettingData, pEnchantEffect)},
         {0x28, offsetof(EffectSettingData, pCastSound)},
         {0x2C, offsetof(EffectSettingData, pBoltSound)},
         {0x30, offsetof(EffectSettingData, pHitSound)},
         {0x34, offsetof(EffectSettingData, pAreaSound)}}
    );
    TESForm::__AddChunkData(DATA_ID, &data, 0x48);
    TESFile::WidenFormIDRecords(
        &data,
        1,
        0x48,
        sizeof(data),
        {{8, offsetof(EffectSettingData, pAssociatedItem)},
         {0x18, offsetof(EffectSettingData, pLight)},
         {0x20, offsetof(EffectSettingData, pEffectShader)},
         {0x24, offsetof(EffectSettingData, pEnchantEffect)},
         {0x28, offsetof(EffectSettingData, pCastSound)},
         {0x2C, offsetof(EffectSettingData, pBoltSound)},
         {0x30, offsetof(EffectSettingData, pHitSound)},
         {0x34, offsetof(EffectSettingData, pAreaSound)}}
    );
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    data.pCastSound = pCastSound;
    data.pBoltSound = pBoltSound;
    data.pHitSound = pHitSound;
    data.pAreaSound = pAreaSound;
    data.pEffectShader = pEffectShader;
    data.pEnchantEffect = pEnchantEffect;
    data.pLight = pLight;
    data.pAssociatedItem = pAssociatedItem;
    for (EffectSetting* psetting : counterEffects.ItemsUntilNull()) {
        TESForm::AddChunk(ESCE_ID, psetting->iFormID);
    }
    CloseForm();
}

// The form pointers hold form IDs after Load; each is resolved to its form.
void EffectSetting::InitItem() {
    if (!GetInitialized()) {
        if (!getArchType() || GetFlag(USE_ASSOCIATED_SKILL | USE_ASSOCIATED_ATTR)) {
            data.pAssociatedItem = nullptr;
        }
        else {
            uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pAssociatedItem));
            if (itempInt) {
                TESFile* pFile = GetFile(-1);
                TESForm::AddCompileIndex(itempInt, pFile);
                data.pAssociatedItem = TESForm::GetFormByNumericID(itempInt);
                if (!data.pAssociatedItem) {
                    int32_t iEffectFormID = iFormID;
                    const char* pcEditorID = GetFormEditorID();
                    Warning(
                        "MASTERFILE: Could not find associated item (%08X) for Effect '%s' (%08X).",
                        itempInt,
                        pcEditorID,
                        iEffectFormID
                    );
                }
            }
        }
        uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pLight));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            data.pLight = dynamic_cast<TESObjectLIGH*>(pForm);
            if (!data.pLight) {
                int32_t iEffectFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find light (%08X) for Effect '%s' (%08X).",
                    itempInt,
                    pcEditorID,
                    iEffectFormID
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pCastSound));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            data.pCastSound = dynamic_cast<TESSound*>(pForm);
            if (!data.pCastSound) {
                int32_t iEffectFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find cast sound (%08X) for Effect '%s' (%08X).",
                    itempInt,
                    pcEditorID,
                    iEffectFormID
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetBoltSound()));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            data.pBoltSound = dynamic_cast<TESSound*>(pForm);
            if (!GetBoltSound()) {
                int32_t iEffectFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find bolt sound (%08X) for Effect '%s' (%08X).",
                    itempInt,
                    pcEditorID,
                    iEffectFormID
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pHitSound));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            data.pHitSound = dynamic_cast<TESSound*>(pForm);
            if (!data.pHitSound) {
                int32_t iEffectFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find hit sound (%08X) for Effect '%s' (%08X).",
                    itempInt,
                    pcEditorID,
                    iEffectFormID
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pAreaSound));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            data.pAreaSound = dynamic_cast<TESSound*>(pForm);
            if (!data.pAreaSound) {
                int32_t iEffectFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find area sound (%08X) for Effect '%s' (%08X).",
                    itempInt,
                    pcEditorID,
                    iEffectFormID
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pEffectShader));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            data.pEffectShader = dynamic_cast<TESEffectShader*>(pForm);
            if (!data.pEffectShader) {
                int32_t iEffectFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find effect shader (%08X) for Effect '%s' (%08X).",
                    itempInt,
                    pcEditorID,
                    iEffectFormID
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pEnchantEffect));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            data.pEnchantEffect = dynamic_cast<TESEffectShader*>(pForm);
            if (!data.pEnchantEffect) {
                int32_t iEffectFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find enchant effect shader (%08X) for Effect '%s' (%08X).",
                    itempInt,
                    pcEditorID,
                    iEffectFormID
                );
            }
        }
        SetInitialized(true);
    }
}

// Each field is copied directly where its getter and setter are trivial accessors; the float fields are copied through
// their setters.
void EffectSetting::Copy(TESForm* apCopy) {
    EffectSetting* pSetting = dynamic_cast<EffectSetting*>(apCopy);
    if (pSetting) {
        CopyAllComponents(apCopy);
        uint32_t uiFlags = pSetting->data.iFlags;
        data.iFlags = uiFlags;
        SetBaseCost(pSetting->data.fBaseCost);
        data.eAssociatedActorValue = pSetting->GetAssociatedActorValue();
        data.iAssociatedSkill = pSetting->data.iAssociatedSkill;
        data.iResistVariable = pSetting->GetResistanceVariable();
        SetConstantEffectEnchantmentFactor(pSetting->data.fCEEnchantFactor);
        SetConstantEffectBarterFactor(pSetting->data.fCEBarterFactor);
        SetProjectileSpeed(pSetting->data.fSpeed);
        SetEffectArchetype(pSetting->GetEffectArchetype());
        data.pAssociatedItem = pSetting->GetAssociatedItem();
        data.pLight = pSetting->data.pLight;
        data.pEffectShader = pSetting->data.pEffectShader;
        data.pEnchantEffect = pSetting->data.pEnchantEffect;
        data.pCastSound = pSetting->data.pCastSound;
        data.pBoltSound = pSetting->GetBoltSound();
        data.pHitSound = pSetting->data.pHitSound;
        data.pAreaSound = pSetting->data.pAreaSound;
        SetCounterEffects(&pSetting->counterEffects);
        if (!GetTemporary() && !iFormID) {
            SetFormID(pDataHandler->GetNextID(), true);
        }
    }
}

bool EffectSetting::Compare(TESForm* apCompare) {
    EffectSetting* pSetting = dynamic_cast<EffectSetting*>(apCompare);
    if (!pSetting) {
        return true;
    }
    if (CompareAllComponents(apCompare)) {
        return true;
    }
    int32_t iEffectFlags = data.iFlags;
    if (iEffectFlags != pSetting->data.iFlags) {
        return true;
    }
    double dBaseCost = data.fBaseCost;
    if (dBaseCost != pSetting->data.fBaseCost) {
        return true;
    }
    ActorValue::Index eAssociatedActorValue = GetAssociatedActorValue();
    if (eAssociatedActorValue != pSetting->GetAssociatedActorValue()) {
        return true;
    }
    TESForm* pAssociatedItem = GetAssociatedItem();
    if (pAssociatedItem != pSetting->GetAssociatedItem()) {
        return true;
    }
    ActorValue::Index eAssociatedSkill = data.iAssociatedSkill;
    if (eAssociatedSkill != pSetting->data.iAssociatedSkill) {
        return true;
    }
    ActorValue::Index eResistVariable = GetResistanceVariable();
    if (eResistVariable != pSetting->GetResistanceVariable()) {
        return true;
    }
    TESObjectLIGH* pLight = data.pLight;
    if (pLight != pSetting->data.pLight) {
        return true;
    }
    TESForm* pEffectShader = data.pEffectShader;
    if (pEffectShader != pSetting->data.pEffectShader) {
        return true;
    }
    TESEffectShader* pEnchantEffect = data.pEnchantEffect;
    if (pEnchantEffect != pSetting->data.pEnchantEffect) {
        return true;
    }
    double dSpeed = data.fSpeed;
    if (dSpeed != pSetting->data.fSpeed) {
        return true;
    }
    TESSound* pCastSound = data.pCastSound;
    if (pCastSound != pSetting->data.pCastSound) {
        return true;
    }
    TESSound* pBoltSound = GetBoltSound();
    if (pBoltSound != pSetting->GetBoltSound()) {
        return true;
    }
    TESSound* pHitSound = data.pHitSound;
    if (pHitSound != pSetting->data.pHitSound) {
        return true;
    }
    TESSound* pAreaSound = data.pAreaSound;
    if (pAreaSound != pSetting->data.pAreaSound) {
        return true;
    }
    int32_t iNumCounterEffects = GetNumCounterEffects();
    if (iNumCounterEffects != pSetting->GetNumCounterEffects()) {
        return true;
    }
    int32_t iArchetype = GetEffectArchetype();
    if (iArchetype != pSetting->GetEffectArchetype()) {
        return true;
    }
    BSSimpleList<EffectSetting*>* pList1 = &counterEffects;
    for (BSSimpleList<EffectSetting*>* pList2 = &pSetting->counterEffects; pList1 && pList2;
         pList2 = pList2->m_pkNext) {
        EffectSetting* pEffect = pList1->m_item;
        if (pEffect != pList2->m_item) {
            return true;
        }
        pList1 = pList1->m_pkNext;
    }
    return false;
}

const char* EffectSetting::GetObjectTypeName() {
    int32_t archType = static_cast<int32_t>(GetEffectArchetype());
    return EffectArchetypes::GetName(archType);
}

bool EffectSetting::IsAssociatedActorValueFixed() {
    return GetAssociatedActorValue() != ActorValue::VALUE_NONE;
}

bool EffectSetting::CheckEffectFlag(uint32_t aeFlags) {
    return (aeFlags & data.iFlags) == aeFlags;
}

bool EffectSetting::IsFlagSet(EffectSetting::EffectFlag aiFlag) {
    return GetFlag(aiFlag);
}

bool EffectSetting::IsScriptEffect() {
    int32_t effectArchetype = static_cast<int32_t>(GetEffectArchetype());
    return EffectArchetypes::CheckFlag(effectArchetype, EffectArchetypes::USES_SCRIPT);
}

ActorValue::Index EffectSetting::GetResistanceVariable() {
    return data.iResistVariable;
}

bool EffectSetting::IsAssociatedActorValueUsed() {
    int32_t effectArchetype = static_cast<int32_t>(GetEffectArchetype());
    return EffectArchetypes::CheckFlag(effectArchetype, EffectArchetypes::USES_ASSOCIATED_ACTOR_VALUE);
}

bool EffectSetting::GetFlag(uint32_t aiFlag) {
    return (data.iFlags & aiFlag) != 0;
}

MagicSystem::ProjectileType EffectSetting::GetProjectileType() {
    return static_cast<MagicSystem::ProjectileType>(
        (IsFlagSet(PROJECTILE_TYPE_BIT0) ? 2 : 0) + IsFlagSet(PROJECTILE_TYPE_BIT1)
    );
}

// Re-reads the field for the result.
int32_t EffectSetting::QueueAssociatedItem() {
    if (iAssociatedItemLoadedCount <= 0) {
        --iAssociatedItemLoadedCount;
    }
    else {
        Warning("GAME EFFECTS: Trying to Queue up an Effect Associated Item which is already loaded");
    }
    return iAssociatedItemLoadedCount;
}

// The associated item's warning (the same literal) is passed here too.
int32_t EffectSetting::QueueEffect() {
    if (iEffectLoadedCount > 0) {
        Warning("GAME EFFECTS: Trying to Queue up an Effect Associated Item which is already loaded");
    }
    else {
        --iEffectLoadedCount;
    }
    return iEffectLoadedCount;
}

// Keeps both tests as bytes and returns their exclusive or.
bool EffectSetting::IsDetrimental(ActorValue::Index aeActorValue) {
    bool bDetrimental = IsFlagSet(DETRIMENTAL);
    uint8_t cPositiveDamage = 0;
    if (aeActorValue != -1) {
        cPositiveDamage = static_cast<uint8_t>(ActorValue::IsFlagSet(aeActorValue, ActorValue::DAMAGE_IS_POSITIVE));
    }
    return (bDetrimental && !cPositiveDamage) || (!bDetrimental && cPositiveDamage);
}

EffectArchetypes::ArchetypeID EffectSetting::GetEffectArchetype() {
    return data.eArchetype;
}

// A negative count moves further from 0.
int32_t EffectSetting::IncEffectLoadedCount() {
    if (iEffectLoadedCount < 0) {
        iEffectLoadedCount = iEffectLoadedCount - 1;
    }
    else {
        iEffectLoadedCount = iEffectLoadedCount + 1;
    }
    return iEffectLoadedCount;
}

void EffectSetting::SetFlag(EffectSetting::EffectFlag aiFlag, bool abValue) {
    if (abValue) {
        data.iFlags |= aiFlag;
    }
    else {
        data.iFlags &= ~aiFlag;
    }
}

int32_t EffectSetting::IncAssociatedItemLoadedCount() {
    int32_t iCount = iAssociatedItemLoadedCount >= 0 ? iAssociatedItemLoadedCount + 1 : iAssociatedItemLoadedCount - 1;
    iAssociatedItemLoadedCount = iCount;
    return iAssociatedItemLoadedCount;
}

int32_t EffectSetting::QueueAssociatedItemFinished() {
    float fCount = static_cast<float>(static_cast<double>(iAssociatedItemLoadedCount));
    iAssociatedItemLoadedCount = static_cast<int32_t>(NiAbs(fCount));
    return iAssociatedItemLoadedCount;
}

bool EffectSetting::IsAssociatedItemLoaded() {
    return iAssociatedItemLoadedCount > 0;
}

bool EffectSetting::IsAssociatedItemQueued() {
    return iAssociatedItemLoadedCount < 0;
}

int32_t EffectSetting::DecEffectLoadedCount() {
    if (iEffectLoadedCount < 0) {
        ++iEffectLoadedCount;
    }
    else if (iEffectLoadedCount > 0) {
        --iEffectLoadedCount;
    }
    return iEffectLoadedCount;
}

int32_t EffectSetting::QueueEffectFinished() {
    float fCount = static_cast<float>(static_cast<double>(iEffectLoadedCount));
    iEffectLoadedCount = static_cast<int32_t>(NiAbs(fCount));
    return iEffectLoadedCount;
}

bool EffectSetting::IsEffectLoaded() {
    return iEffectLoadedCount > 0;
}

bool EffectSetting::IsEffectQueued() {
    return iEffectLoadedCount < 0;
}

int32_t EffectSetting::GetNumCounterEffects() {
    return data.iNumCounterEffects;
}

float EffectSetting::GetProjectileSpeed() {
    return data.fSpeed;
}

TESSound* EffectSetting::GetBoltSound() {
    return data.pBoltSound;
}

void EffectSetting::SetBaseCost(float afBaseCost) {
    data.fBaseCost = afBaseCost;
}

void EffectSetting::SetConstantEffectEnchantmentFactor(float afCEEnchantFactor) {
    data.fCEEnchantFactor = afCEEnchantFactor;
}

void EffectSetting::SetConstantEffectBarterFactor(float afCEBarterFactor) {
    data.fCEBarterFactor = afCEBarterFactor;
}

void EffectSetting::SetProjectileSpeed(float afSpeed) {
    data.fSpeed = afSpeed;
}

bool EffectSetting::getArchType() {
    uint32_t effectArchetype = GetEffectArchetype();
    return EffectArchetypes::CheckFlag(effectArchetype, EffectArchetypes::ASSOCIATED_ITEM_MASK);
}

void EffectSetting::SetEffectArchetype(int32_t aiArchetype) {
    data.eArchetype = static_cast<EffectArchetypes::ArchetypeID>(aiArchetype);
}

void EffectSetting::SetEffectName(const char* apEffectName) {
    if (GetEffectName().StrCmp(apEffectName, true)) {
        SetFullName(apEffectName);
    }
}

int32_t EffectSetting::DecAssociatedItemLoadedCount() {
    if (iAssociatedItemLoadedCount >= 0) {
        if (iAssociatedItemLoadedCount > 0) {
            --iAssociatedItemLoadedCount;
        }
    }
    else {
        ++iAssociatedItemLoadedCount;
    }
    return iAssociatedItemLoadedCount;
}

void EffectSetting::SetCounterEffects(BSSimpleList<EffectSetting*>* apCounterEffects) {
    counterEffects.RemoveAll();
    data.iNumCounterEffects = 0;
    while (apCounterEffects) {
        EffectSetting* pEffect = apCounterEffects->m_item;
        if (!pEffect) {
            break;
        }
        counterEffects.AddHead(pEffect);
        ++data.iNumCounterEffects;
        apCounterEffects = apCounterEffects->m_pkNext;
    }
}

int32_t BSstrcmp(const char* apString1, const char* apString2) {
    return std::strcmp(apString1, apString2);
}

void TESFullName::SetFullName(const char* apFullName) {
    cFullName.Set(apFullName, 0);
}
