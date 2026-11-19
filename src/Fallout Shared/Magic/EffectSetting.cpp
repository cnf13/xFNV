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
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// Reads the name, model, description, icon, editor ID, object bound and DATA chunks and the counter effects (ESCE). The
// DATA record is cleared and given a speed of 1 before it is read, as the game does.
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
#if XFNV_64BIT
                // The whole record: the file's 0x48 bytes are the 32-bit record.
                ourMemset(&data, 0, sizeof(data));
#else
                ourMemset(&data, 0, sizeof(data));
#endif
                data.fSpeed = 1.0f;
                apFile->GetChunkData(&data, 0x48);
#if XFNV_64BIT
                // The file's record is 0x48 bytes (fewer in older files), with a 4-byte form ID in each pointer field.
                TESFile::WidenFormIDRecords(
                    &data, 1, apFile->GetChunkSize() < 0x48 ? apFile->GetChunkSize() : 0x48, sizeof(data),
                    {{8, offsetof(EffectSettingData, pAssociatedItem)},
                     {0x18, offsetof(EffectSettingData, pLight)},
                     {0x20, offsetof(EffectSettingData, pEffectShader)},
                     {0x24, offsetof(EffectSettingData, pEnchantEffect)},
                     {0x28, offsetof(EffectSettingData, pCastSound)},
                     {0x2C, offsetof(EffectSettingData, pBoltSound)},
                     {0x30, offsetof(EffectSettingData, pHitSound)},
                     {0x34, offsetof(EffectSettingData, pAreaSound)}}
                );
#endif
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
                        uiID, static_cast<TESForm*>(this)->GetFormEditorID()
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
    data.pCastSound =
        pCastSound ? reinterpret_cast<TESSound*>(static_cast<uintptr_t>(data.pCastSound->iFormID)) : nullptr;
    TESSound* pBoltSound = data.pBoltSound;
    data.pBoltSound =
        pBoltSound ? reinterpret_cast<TESSound*>(static_cast<uintptr_t>(data.pBoltSound->iFormID)) : nullptr;
    TESSound* pHitSound = data.pHitSound;
    data.pHitSound = pHitSound ? reinterpret_cast<TESSound*>(static_cast<uintptr_t>(data.pHitSound->iFormID)) : nullptr;
    TESSound* pAreaSound = data.pAreaSound;
    data.pAreaSound =
        pAreaSound ? reinterpret_cast<TESSound*>(static_cast<uintptr_t>(data.pAreaSound->iFormID)) : nullptr;
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
    data.pAssociatedItem =
        pAssociatedItem ? reinterpret_cast<TESForm*>(static_cast<uintptr_t>(data.pAssociatedItem->iFormID)) : nullptr;
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__AddChunkData(DATA_ID, &data, sizeof(EffectSettingData));
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
            uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pAssociatedItem));
            if (itempint) {
                TESFile* File = GetFile(-1);
                TESForm::AddCompileIndex(itempint, File);
                data.pAssociatedItem = TESForm::GetFormByNumericID(itempint);
                if (!data.pAssociatedItem) {
                    int32_t CastingType = iFormID;
                    const char* v17 = GetFormEditorID();
                    Warning(
                        "MASTERFILE: Could not find associated item (%08X) for Effect '%s' (%08X).", itempint, v17,
                        CastingType
                    );
                }
            }
        }
        uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pLight));
        if (itempInt) {
            TESFile* v2 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v2);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempInt);
            data.pLight = dynamic_cast<TESObjectLIGH*>(FormByNumericID);
            if (!data.pLight) {
                int32_t v26 = iFormID;
                const char* v18 = GetFormEditorID();
                Warning("MASTERFILE: Could not find light (%08X) for Effect '%s' (%08X).", itempInt, v18, v26);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pCastSound));
        if (itempInt) {
            TESFile* v4 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v4);
            TESForm* v5 = TESForm::GetFormByNumericID(itempInt);
            data.pCastSound = dynamic_cast<TESSound*>(v5);
            if (!data.pCastSound) {
                int32_t v27 = iFormID;
                const char* v19 = GetFormEditorID();
                Warning("MASTERFILE: Could not find cast sound (%08X) for Effect '%s' (%08X).", itempInt, v19, v27);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetBoltSound()));
        if (itempInt) {
            TESFile* v6 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v6);
            TESForm* v7 = TESForm::GetFormByNumericID(itempInt);
            data.pBoltSound = dynamic_cast<TESSound*>(v7);
            if (!GetBoltSound()) {
                int32_t v28 = iFormID;
                const char* v20 = GetFormEditorID();
                Warning("MASTERFILE: Could not find bolt sound (%08X) for Effect '%s' (%08X).", itempInt, v20, v28);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pHitSound));
        if (itempInt) {
            TESFile* v8 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v8);
            TESForm* v9 = TESForm::GetFormByNumericID(itempInt);
            data.pHitSound = dynamic_cast<TESSound*>(v9);
            if (!data.pHitSound) {
                int32_t v29 = iFormID;
                const char* v21 = GetFormEditorID();
                Warning("MASTERFILE: Could not find hit sound (%08X) for Effect '%s' (%08X).", itempInt, v21, v29);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pAreaSound));
        if (itempInt) {
            TESFile* v11 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v11);
            TESForm* v12 = TESForm::GetFormByNumericID(itempInt);
            data.pAreaSound = dynamic_cast<TESSound*>(v12);
            if (!data.pAreaSound) {
                int32_t v30 = iFormID;
                const char* v22 = GetFormEditorID();
                Warning("MASTERFILE: Could not find area sound (%08X) for Effect '%s' (%08X).", itempInt, v22, v30);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pEffectShader));
        if (itempInt) {
            TESFile* v13 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v13);
            TESForm* v14 = TESForm::GetFormByNumericID(itempInt);
            data.pEffectShader = dynamic_cast<TESEffectShader*>(v14);
            if (!data.pEffectShader) {
                int32_t v31 = iFormID;
                const char* v23 = GetFormEditorID();
                Warning("MASTERFILE: Could not find effect shader (%08X) for Effect '%s' (%08X).", itempInt, v23, v31);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pEnchantEffect));
        if (itempInt) {
            TESFile* v15 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v15);
            TESForm* v16 = TESForm::GetFormByNumericID(itempInt);
            data.pEnchantEffect = dynamic_cast<TESEffectShader*>(v16);
            if (!data.pEnchantEffect) {
                int32_t v32 = iFormID;
                const char* v24 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find enchant effect shader (%08X) for Effect '%s' (%08X).", itempInt, v24,
                    v32
                );
            }
        }
        SetInitialized(true);
    }
}

// Each field is copied directly where its getter and setter are trivial accessors; the float fields are copied through
// their setters.
void EffectSetting::Copy(TESForm* apCopy) {
    EffectSetting* psetting = dynamic_cast<EffectSetting*>(apCopy);
    if (psetting) {
        CopyAllComponents(apCopy);
        uint32_t uiFlags = psetting->data.iFlags;
        data.iFlags = uiFlags;
        SetBaseCost(psetting->data.fBaseCost);
        data.eAssociatedActorValue = psetting->GetAssociatedActorValue();
        data.iAssociatedSkill = psetting->data.iAssociatedSkill;
        data.iResistVariable = psetting->GetResistanceVariable();
        SetConstantEffectEnchantmentFactor(psetting->data.fCEEnchantFactor);
        SetConstantEffectBarterFactor(psetting->data.fCEBarterFactor);
        SetProjectileSpeed(psetting->data.fSpeed);
        int32_t EffectArchetype = psetting->GetEffectArchetype();
        SetEffectArchetype(EffectArchetype);
        data.pAssociatedItem = psetting->GetAssociatedItem();
        data.pLight = psetting->data.pLight;
        data.pEffectShader = psetting->data.pEffectShader;
        data.pEnchantEffect = psetting->data.pEnchantEffect;
        data.pCastSound = psetting->data.pCastSound;
        data.pBoltSound = psetting->GetBoltSound();
        data.pHitSound = psetting->data.pHitSound;
        data.pAreaSound = psetting->data.pAreaSound;
        BSSimpleList<EffectSetting*>* RecipeList = &psetting->counterEffects;
        SetCounterEffects(RecipeList);
        if (!GetTemporary() && !iFormID) {
            uint32_t NextID = pDataHandler->GetNextID();
            SetFormID(NextID, true);
        }
    }
}

bool EffectSetting::Compare(TESForm* apCompare) {
    EffectSetting* psetting = dynamic_cast<EffectSetting*>(apCompare);
    if (!psetting) {
        return true;
    }
    if (CompareAllComponents(apCompare)) {
        return true;
    }
    int32_t NumberOfItemsActivated = data.iFlags;
    if (NumberOfItemsActivated != psetting->data.iFlags) {
        return true;
    }
    double ColorRange = data.fBaseCost;
    if (ColorRange != psetting->data.fBaseCost) {
        return true;
    }
    ActorValue::Index AssociatedActorValue = GetAssociatedActorValue();
    if (AssociatedActorValue != psetting->GetAssociatedActorValue()) {
        return true;
    }
    TESForm* AssociatedItem = GetAssociatedItem();
    if (AssociatedItem != psetting->GetAssociatedItem()) {
        return true;
    }
    ActorValue::Index WaterSystem = data.iAssociatedSkill;
    if (WaterSystem != psetting->data.iAssociatedSkill) {
        return true;
    }
    ActorValue::Index ResistanceVariable = GetResistanceVariable();
    if (ResistanceVariable != psetting->GetResistanceVariable()) {
        return true;
    }
    TESObjectLIGH* v9 = data.pLight;
    if (v9 != psetting->data.pLight) {
        return true;
    }
    TESForm* FormTwo = data.pEffectShader;
    if (FormTwo != psetting->data.pEffectShader) {
        return true;
    }
    TESEffectShader* RefCount = data.pEnchantEffect;
    if (RefCount != psetting->data.pEnchantEffect) {
        return true;
    }
    double MaximumLeafAngle = data.fSpeed;
    if (MaximumLeafAngle != psetting->data.fSpeed) {
        return true;
    }
    TESSound* WaterType = data.pCastSound;
    if (WaterType != psetting->data.pCastSound) {
        return true;
    }
    TESSound* BoltSound = GetBoltSound();
    if (BoltSound != psetting->GetBoltSound()) {
        return true;
    }
    TESSound* WorldSpace = data.pHitSound;
    if (WorldSpace != psetting->data.pHitSound) {
        return true;
    }
    TESSound* OpenSound = data.pAreaSound;
    if (OpenSound != psetting->data.pAreaSound) {
        return true;
    }
    int32_t NumCounterEffects = GetNumCounterEffects();
    if (NumCounterEffects != psetting->GetNumCounterEffects()) {
        return true;
    }
    int32_t EffectArchetype = GetEffectArchetype();
    if (EffectArchetype != psetting->GetEffectArchetype()) {
        return true;
    }
    BSSimpleList<EffectSetting*>* ptmp1 = &counterEffects;
    for (BSSimpleList<EffectSetting*>* ptmp2 = &psetting->counterEffects; ptmp1 && ptmp2; ptmp2 = ptmp2->m_pkNext) {
        EffectSetting* m_item = ptmp1->m_item;
        if (m_item != ptmp2->m_item) {
            return true;
        }
        ptmp1 = ptmp1->m_pkNext;
    }
    return false;
}

const char* EffectSetting::GetObjectTypeName() {
    int32_t archType = (int32_t)GetEffectArchetype();
    return EffectArchetypes::GetName(archType);
}

bool EffectSetting::IsAssociatedActorValueFixed() {
    return (bool)((uint8_t)(GetAssociatedActorValue() != (-1)));
}

bool EffectSetting::CheckEffectFlag(uint32_t aeFlags) {
    return (bool)((uint8_t)((((int32_t)aeFlags) & ((int32_t)data.iFlags)) == ((int32_t)aeFlags)));
}

bool EffectSetting::IsFlagSet(EffectSetting::EffectFlag aiFlag) {
    return GetFlag(aiFlag);
}

bool EffectSetting::IsScriptEffect() {
    int32_t effectArchetype = (int32_t)GetEffectArchetype();
    return (bool)((int32_t)EffectArchetypes::CheckFlag(effectArchetype, 64));
}

ActorValue::Index EffectSetting::GetResistanceVariable() {
    return data.iResistVariable;
}

bool EffectSetting::IsAssociatedActorValueUsed() {
    int32_t effectArchetype = (int32_t)GetEffectArchetype();
    return (bool)((int32_t)EffectArchetypes::CheckFlag(effectArchetype, 2));
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

// The associated item's warning (the same literal) is passed here too, as the game does.
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
    int32_t uiEffectFlags;
    int32_t v4;
    if (abValue) {
        data.iFlags |= ((int32_t)aiFlag);
        uiEffectFlags = (int32_t)data.iFlags;
    }
    else {
        data.iFlags &= (~((int32_t)aiFlag));
        v4 = (int32_t)data.iFlags;
    }
    int32_t v5 = (int32_t)data.iFlags;
}

int32_t EffectSetting::IncAssociatedItemLoadedCount() {
    int32_t v1;
    if (iAssociatedItemLoadedCount >= 0) {
        v1 = iAssociatedItemLoadedCount + 1;
    }
    else {
        v1 = iAssociatedItemLoadedCount - 1;
    }
    iAssociatedItemLoadedCount = v1;
    return iAssociatedItemLoadedCount;
}

int32_t EffectSetting::QueueAssociatedItemFinished() {
    float x = (float)(double)iAssociatedItemLoadedCount;
    iAssociatedItemLoadedCount = (int32_t)NiAbs(x);
    return iAssociatedItemLoadedCount;
}

bool EffectSetting::IsAssociatedItemLoaded() {
    return (bool)((uint8_t)(iAssociatedItemLoadedCount > 0));
}

bool EffectSetting::IsAssociatedItemQueued() {
    return (bool)((uint8_t)(iAssociatedItemLoadedCount < 0));
}

int32_t EffectSetting::DecEffectLoadedCount() {
    if (iEffectLoadedCount < 0) {
        ++iEffectLoadedCount;
    }
    else {
        if (iEffectLoadedCount > 0) {
            --iEffectLoadedCount;
        }
    }
    return iEffectLoadedCount;
}

int32_t EffectSetting::QueueEffectFinished() {
    float x = (float)(double)iEffectLoadedCount;
    iEffectLoadedCount = (int32_t)NiAbs(x);
    return iEffectLoadedCount;
}

bool EffectSetting::IsEffectLoaded() {
    return (bool)((uint8_t)(iEffectLoadedCount > 0));
}

bool EffectSetting::IsEffectQueued() {
    return (bool)((uint8_t)(iEffectLoadedCount < 0));
}

int32_t EffectSetting::GetNumCounterEffects() {
    return (int32_t)data.iNumCounterEffects;
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
    return (bool)((uint8_t)EffectArchetypes::CheckFlag(effectArchetype, 0x7Cu));
}

void EffectSetting::SetEffectArchetype(int32_t a2) {
    data.eArchetype = static_cast<EffectArchetypes::ArchetypeID>(a2);
}

void EffectSetting::SetEffectName(const char* apEffectName) {
    int32_t v5 = GetEffectName().StrCmp(apEffectName, true);
    if (v5) {
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
        EffectSetting* a2 = apCounterEffects->m_item;
        if (!a2) {
            break;
        }
        counterEffects.AddHead(a2);
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
