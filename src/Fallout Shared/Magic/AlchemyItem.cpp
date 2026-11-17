#include "Fallout Shared/Magic/AlchemyItem.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicItemObject.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/AI/Actor.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// Swaps the cost, the addiction, its chance and the consumption sound read from a big-endian file; the flags at +4
// are left alone.
void AlchemyItemData::Endian() {
    ENDIAN(iCostOverride, false);
    ENDIAN(pAddictionItem, false);
    ENDIAN(fAddictionChance, false);
    ENDIAN(pConsumptionSound, false);
}

AlchemyItem::AlchemyItem()
  : MagicItemObject(), TESModelTextureSwap(), TESIcon(), BGSMessageIcon(), TESScriptableForm(), TESWeightForm(),
    BGSEquipType(), BGSDestructibleObjectForm(), BGSPickupPutdownSounds(), MessageIcon() {
    SetFormType(ALCH_ID);
    AlchemyItem::InitializeData();
}

AlchemyItem::~AlchemyItem() {
    AlchemyItem::ClearData();
}

void AlchemyItem::InitializeData() {
    data.iCostOverride = (-1);
    data.iFlags = (uint8_t)0;
    data.pAddictionItem = nullptr;
    data.fAddictionChance = (float)0.0;
    data.pConsumptionSound = nullptr;
    TESForm::InitializeDataComponent();
}

void AlchemyItem::ClearData() {
    static_cast<EffectItemList*>(this)->RemoveAllEffects();
    TESForm::InitializeDataComponent();
}

void AlchemyItem::InitItem() {
    if (!GetInitialized()) {
        TESScriptableForm::InitItem(this);
        BGSDestructibleObjectForm::InitItem(this);
        TESModelTextureSwap::InitItem(this);
        MagicItemObject::InitItem();
        BGSPickupPutdownSounds::InitItem(this);
        // The loaded addiction item is still its form ID, kept in the pointer field until resolved here.
        uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetAddictionItem()));
        if (itempInt) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempInt);
            SetAddictionItem(dynamic_cast<SpellItem*>(FormByNumericID));
            if (!GetAddictionItem()) {
                int32_t CastingType = iFormID;
                const char* v5 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Failed to find addiction item (%08X) for ingestible '%s' (%08X).", itempInt, v5,
                    CastingType
                );
            }
        }
        // The loaded consumption sound is still its form ID, likewise.
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetConsumptionSound()));
        if (itempInt) {
            TESFile* v3 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v3);
            TESForm* v4 = TESForm::GetFormByNumericID(itempInt);
            SetConsumptionSound(dynamic_cast<TESSound*>(v4));
            if (!GetConsumptionSound()) {
                int32_t v8 = iFormID;
                const char* v6 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Failed to find consumption sound (%08X) for ingestible '%s' (%08X).", itempInt, v6, v8
                );
            }
        }
    }
}

bool AlchemyItem::IsAutoCalc() {
    return !IsFlagSet(COST_OVERRIDE);
}

void AlchemyItem::SetAutoCalc(bool abValue) {
    SetFlag(COST_OVERRIDE, !abValue);
}

bool AlchemyItem::IsFood() {
    return IsFlagSet(AlchemyItem::FOOD_ITEM) && !static_cast<EffectItemList*>(this)->CanBePoison();
}

float AlchemyItem::GetCost(Actor* apActor) {
    if (IsFlagSet(AlchemyItem::COST_OVERRIDE)) {
        return static_cast<float>(GetCostOverride());
    }
    else {
        return static_cast<EffectItemList*>(this)->EffectItemList::GetCost(0);
    }
}

void AlchemyItem::Endian() {
    (&data)->Endian();
}

CHUNK_ID AlchemyItem::GetChunkID() {
    return static_cast<CHUNK_ID>(1414090309);
}

uint32_t AlchemyItem::GetDataSize() {
    return sizeof(AlchemyItemData);
}

bool AlchemyItem::CompareMagicItemData(MagicItem* apCompare) {
    AlchemyItem* palchemy = dynamic_cast<AlchemyItem*>(apCompare);
    if (!palchemy) {
        return 1;
    }
    const void* v3 = palchemy->GetData();
    return std::memcmp(&data, v3, sizeof(data)) != 0;
}

// Copies the item data of another alchemy item.
void AlchemyItem::CopyMagicItemData(MagicItem* apCopy) {
    AlchemyItem* pCopy = dynamic_cast<AlchemyItem*>(apCopy);
    if (pCopy) {
        // Fetches the source's data before this item's.
        void* pSource = pCopy->GetData();
        void* pDest = GetData();
        ourMemcpy(pDest, pSource, sizeof(AlchemyItemData));
    }
}

// Writes the item's components: the model, the icon, the message icon, the script, the destruction data, the
// pickup and putdown sounds, the equip type, then the form's own data.
void AlchemyItem::SaveMagicItemComponents() {
    static_cast<TESModelTextureSwap*>(this)->Save(MODL_ID, MODT_ID, MODS_ID);
    static_cast<TESIcon*>(this)->Save(ICON_ID);
    static_cast<BGSMessageIcon*>(this)->Save(MICO_ID);
    static_cast<TESScriptableForm*>(this)->Save();
    static_cast<BGSDestructibleObjectForm*>(this)->Save();
    static_cast<BGSPickupPutdownSounds*>(this)->Save();
    static_cast<BGSEquipType*>(this)->Save();
    TESForm::SaveData();
}

// Writes the item data chunk with the addiction spell and the consumption sound swapped for their form IDs, then puts
// the pointers back. Each form ID is stored in the pointer field itself (raw value, not a pointer), hence the
// integer-to-pointer casts.
void AlchemyItem::SaveMagicItemData() {
    SpellItem* pAddiction = GetAddictionItem();
    if (GetAddictionItem()) {
        SetAddictionItem(reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(GetAddictionItem()->GetFormID())));
    }
    else {
        SetAddictionItem(nullptr);
    }
    TESSound* pSound = GetConsumptionSound();
    if (GetConsumptionSound()) {
        SetConsumptionSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(GetConsumptionSound()->GetFormID())));
    }
    else {
        SetConsumptionSound(nullptr);
    }
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__AddChunkData(GetChunkID(), &data, 0x14);
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    SetAddictionItem(pAddiction);
    SetConsumptionSound(pSound);
}

// Reads one chunk of the alchemy item's record: the model and its texture swaps, the icon, the message icon, the
// script, the equip type, the pickup and putdown sounds, the destruction data and the item data.
void AlchemyItem::LoadMagicItemChunk(TESFile* apFile, CHUNK_ID apChunkID) {
    switch (apChunkID) {
        case MODL_ID:
        case MODD_ID:
        case MODT_ID:
        case MOSD_ID: {
            TESModel* pModel = static_cast<TESModel*>(static_cast<TESModelTextureSwap*>(this));
            TESModel::LoadModelChunk(pModel, apFile);
            return;
        }
        case MODS_ID: {
            uint32_t uiChunkSize = apFile->GetChunkSize();
            // A 16-byte aligned stack buffer of the chunk's size.
            char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
            apFile->GetChunkData(pBuffer, uiChunkSize);
            uint16_t usFormVersion = apFile->GetFormVersion();
            static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, usFormVersion);
            return;
        }
        case ICON_ID: {
            TESTexture* pTexture = static_cast<TESTexture*>(static_cast<TESIcon*>(this));
            TESTexture::LoadTextureChunk(pTexture, apFile);
            return;
        }
        case MICO_ID: {
            BGSMessageIcon* pIcon = static_cast<BGSMessageIcon*>(this);
            BGSMessageIcon::LoadMessageIcon(pIcon, apFile);
            return;
        }
        case SCRI_ID: {
            uint32_t uiScript = 0;
            apFile->GetChunkData(uiScript);
            static_cast<TESScriptableForm*>(this)->SetFormScript(reinterpret_cast<Script*>(uiScript));
            static_cast<TESScriptableForm*>(this)->InitItem(this);
            return;
        }
        case ETYP_ID: {
            uint32_t uiEquipType = 0;
            apFile->GetChunkData(uiEquipType);
            static_cast<BGSEquipType*>(this)->SetEquipType(static_cast<EQUIP_TYPE>(uiEquipType));
            return;
        }
        case DEST_ID:
        case DSTD_ID: {
            BGSDestructibleObjectForm* pDestruct = static_cast<BGSDestructibleObjectForm*>(this);
            BGSDestructibleObjectForm::LoadChunk(pDestruct, apFile);
            return;
        }
        case YNAM_ID: {
            uint32_t uiPickupSound = 0;
            apFile->GetChunkData(uiPickupSound);
            static_cast<BGSPickupPutdownSounds*>(this)->SetPickupSound(reinterpret_cast<TESSound*>(uiPickupSound));
            return;
        }
        case ZNAM_ID: {
            uint32_t uiPutdownSound = 0;
            apFile->GetChunkData(uiPutdownSound);
            static_cast<BGSPickupPutdownSounds*>(this)->SetPutdownSound(reinterpret_cast<TESSound*>(uiPutdownSound));
            return;
        }
        case DATA_ID: TESForm::LoadData(apFile, nullptr, 0); return;
    }
}

bool AlchemyItem::IsFlagSet(AlchemyFlag aiFlag) {
    return (aiFlag & data.iFlags) != 0;
}

void AlchemyItem::SetFlag(AlchemyFlag aiFlag, bool abValue) {
    uint8_t v3;
    if (abValue) {
        v3 = (uint8_t)(((int8_t)aiFlag) | data.iFlags);
    }
    else {
        v3 = (uint8_t)((~((int8_t)aiFlag)) & data.iFlags);
    }
    data.iFlags = v3;
    uint8_t ucAlchFlags = data.iFlags;
}

SpellItem* AlchemyItem::GetAddictionItem() {
    return data.pAddictionItem;
}

void AlchemyItem::SetAddictionItem(SpellItem* apObject) {
    data.pAddictionItem = apObject;
}

TESSound* AlchemyItem::GetConsumptionSound() {
    return data.pConsumptionSound;
}

void AlchemyItem::SetConsumptionSound(TESSound* apObject) {
    data.pConsumptionSound = apObject;
}

bool AlchemyItem::IsWater() {
    BGSListForm* pFormList = static_cast<BGSListForm*>(TESForm::GetFormByEditorID("WaterLIST"));
    bool result = false;
    if (pFormList) {
        TESForm* item = this;
        if ((&pFormList->ListOfForms)->IsInList(item)) {
            return true;
        }
    }
    return result;
}

void BGSEquipType::SetEquipType(BGSEquipType::EQUIP_TYPE aeType) {
    eEquipType = aeType;
}

BSSimpleList<TESForm*>* BGSListForm::GetFormList() {
    return &ListOfForms;
}

void BGSPickupPutdownSounds::SetPickupSound(TESSound* apSound) {
    pPickupSound = apSound;
}

void BGSPickupPutdownSounds::SetPutdownSound(TESSound* apSound) {
    pPutdownSound = apSound;
}

BaseFormComponent::BaseFormComponent() {}

// The warning printer reads no argument and returns 0, as the game does.
int32_t Warning(const char* apMessage, ...) {
    return 0;
}

MagicItem::MagicItem() : TESFullName(), EffectItemList() {}

MagicItemObject::MagicItemObject() : TESBoundObject(), MagicItem() {}

MagicItemObject::~MagicItemObject() {}

TESFullName::~TESFullName() {}

TESIcon::TESIcon() : TESTexture() {}

TESIcon::~TESIcon() {}
