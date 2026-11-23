#include "Fallout Shared/TESObjectBOOK.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

// The game settings this file defines, in initialization order.
SettingT<GameSettingCollection> fBookPerkBonus("fBookPerkBonus", 1.0f);

TESObjectBOOK::TESObjectBOOK() {
    SetFormType(25u);
    TESObjectBOOK::InitializeData();
}

TESObjectBOOK::~TESObjectBOOK() {}

void TESObjectBOOK::InitializeData() {
    ourMemset(&Data, 0, sizeof(Data));
    Data.eTeaches = 0xFF;
    static_cast<TESEnchantableForm*>(this)->SetCastingType(MagicSystem::CAST_ONCE);
}

// Reads the book's chunks: its name, model, icon, script, enchantment and description components, the destructible
// data and the pickup and putdown sounds, then its own two data bytes.
bool TESObjectBOOK::Load(TESFile* apFile) {
    if (static_cast<uint8_t>(apFile->GetTESForm()) != BOOK_ID) {
        return false;
    }
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
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case MODL_ID:
            case MODD_ID:
            case MOSD_ID:
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModelTextureSwap*>(this), apFile); break;
            case MODS_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiSize));
                apFile->GetChunkData(pBuffer, uiSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
                break;
            }
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            case MICO_ID: BGSMessageIcon::LoadMessageIcon(this, apFile); break;
            case SCRI_ID: {
                uint32_t uiScriptID = 0;
                apFile->GetChunkData(uiScriptID);
                TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
                pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
                pScriptableForm->InitItem(this);
                break;
            }
            case ENAM_ID:
            case EITM_ID: {
                uint32_t uiEnchantmentID = 0;
                apFile->GetChunkData(uiEnchantmentID);
                static_cast<TESEnchantableForm*>(this)->SetFormEnchanting(
                    reinterpret_cast<EnchantmentItem*>(static_cast<uintptr_t>(uiEnchantmentID))
                );
                break;
            }
            case ANAM_ID:
            case EAMT_ID: {
                int16_t sAmount = 0;
                apFile->LoadWord(&sAmount);
                static_cast<TESEnchantableForm*>(this)->SetFormEnchantmentAmount(static_cast<uint16_t>(sAmount));
                break;
            }
            case DESC_ID: TESDescription::LoadDescriptionData(this, apFile); break;
            case DSTD_ID:
            case DEST_ID: BGSDestructibleObjectForm::LoadChunk(this, apFile); break;
            case YNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                static_cast<BGSPickupPutdownSounds*>(this)->SetPickupSound(
                    reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID))
                );
                break;
            }
            case ZNAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                static_cast<BGSPickupPutdownSounds*>(this)->SetPutdownSound(
                    reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID))
                );
                break;
            }
            case DATA_ID: LoadData(apFile, reinterpret_cast<char*>(&Data), sizeof(Data)); break;
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESObjectBOOK::Save() {
    StartForm();
    TESFullName::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    TESTexture::Save(ICON_ID);
    BGSMessageIcon::Save(MICO_ID);
    TESScriptableForm::Save();
    SaveSubRecord();
    TESDescription::Save();
    BGSDestructibleObjectForm::Save();
    BGSPickupPutdownSounds::Save();
    if (TESForm::QEndianSwapOnSave()) {}
    __SaveData(&Data, sizeof(Data));
    if (TESForm::QEndianSwapOnSave()) {}
    CloseForm();
}

uint16_t TESObjectBOOK::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
    uint16_t usSize = static_cast<uint16_t>(
        static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize
    );
    if ((aiFlags & CHANGE_BOOK_TEACHES_SKILL) != 0) {
        ++usSize;
    }
    return usSize;
}

void TESObjectBOOK::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    TESValueForm::SaveGame(aiFlags);
    if ((aiFlags & CHANGE_BOOK_TEACHES_SKILL) != 0) {
        pSaveLoadGameOLD->SaveGameDataOLD(&Data.eTeaches, sizeof(Data.eTeaches));
    }
}

// Saves the book's changes into the save buffer: the base form's and the value's, then the skill it teaches when
// that changed.
void TESObjectBOOK::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    TESValueForm::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_BOOK_TEACHES_SKILL)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Data.eTeaches), sizeof(Data.eTeaches), 0);
    }
}

void TESObjectBOOK::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
    if ((aiFlags & CHANGE_BOOK_TEACHES_SKILL) != 0) {
        pSaveLoadGameOLD->LoadGameDataOLD(&Data.eTeaches, sizeof(Data.eTeaches));
    }
}

// Loads the same changes back from the load buffer.
void TESObjectBOOK::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    TESValueForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_BOOK_TEACHES_SKILL)) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&Data.eTeaches), sizeof(Data.eTeaches));
    }
}

void TESObjectBOOK::InitItem() {
    if (!TESForm::GetInitialized()) {
        static_cast<TESScriptableForm*>(this)->InitItem(this);
        static_cast<TESEnchantableForm*>(this)->InitItem(this);
        static_cast<BGSDestructibleObjectForm*>(this)->InitItem(this);
        static_cast<TESModelTextureSwap*>(this)->InitItem(this);
        static_cast<BGSPickupPutdownSounds*>(this)->InitItem(this);
        if (static_cast<int8_t>(Data.eTeaches) != -1) {
            int32_t iSkill = ActorValue::ToActorValue(ActorValue::SKILL, Data.eTeaches);
            if (iSkill < 32 || iSkill > 46) {
                Data.eTeaches = static_cast<uint8_t>(-1);
            }
        }
        TESForm::SetInitialized(true);
    }
}

void TESObjectBOOK::Copy(TESForm* apCopy) {
    TESObjectBOOK* pBook = dynamic_cast<TESObjectBOOK*>(apCopy);
    if (pBook) {
        CopyAllComponents(apCopy);
        Data = pBook->Data;
    }
}

bool TESObjectBOOK::Compare(TESForm* apCompare) {
    TESObjectBOOK* pBook = dynamic_cast<TESObjectBOOK*>(apCompare);
    if (!pBook) {
        return true;
    }
    if (TESForm::CompareAllComponents(pBook)) {
        return true;
    }
    return std::memcmp(&Data, &pBook->Data, sizeof(Data)) != 0;
}

bool TESObjectBOOK::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjectToGet,
    int32_t aiCount
) {
    return TESBoundObject::Activate(apItemActivated, apActionRef, abIdFlag, apObjectToGet, aiCount);
}

bool TESObjectBOOK::GetCantTake() {
    return (Data.cFlags & OBJ_BOOK::FLAG_CANT_TAKE) != 0;
}

bool TESObjectBOOK::IsScroll() {
    return (Data.cFlags & OBJ_BOOK::FLAG_SCROLL) != 0;
}

// The skill byte is tested signed against -1; ToActorValue reads its argument as a signed char.
ActorValue::Index TESObjectBOOK::GetTeaches() {
    if (static_cast<int8_t>(Data.eTeaches) == -1) {
        return ActorValue::VALUE_NONE;
    }

    return ActorValue::ToActorValue(ActorValue::SKILL, static_cast<char>(Data.eTeaches));
}

bool TESObjectBOOK::Read(TESObjectREFR* apActionRef) {
    bool bResult = false;
    Actor* pActor = dynamic_cast<Actor*>(apActionRef);
    if (!pActor->IsPc()) {
        return bResult;
    }
    if (!pActor->IsInCombat() || Interface::IsInMenuMode()) {
        ActorValue::Index eTeaches = GetTeaches();
        if (ActorValue::IsSkill(eTeaches) && pPlayer->GetPermanentActorValue(eTeaches) < 200) {
            float fSkillPoints = fBookPerkBonus.Float();
            BSStringT<char> strTemp;
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADJUST_BOOK_SKILL_POINTS, pPlayer, &fSkillPoints);
            pPlayer->PermanentModActorValue(eTeaches, fSkillPoints, static_cast<Actor*>(nullptr));
            MiscStatManager::Increment(MiscStatManager::MS_BOOKS_READ);
            int32_t iSkillPoints = static_cast<int32_t>(fSkillPoints);
            const char* pSkillName = ActorValue::GetActorValueName(eTeaches);
            strTemp.SPrintF(sSkillIncreasedNum.String(), pSkillName, iSkillPoints);
            Interface::ShowMessage(strTemp.pString, 1, nullptr, nullptr, 2.0f, false);
            bResult = true;
        }
        return bResult;
    }

    Interface::ShowMessage(sCanNotReadBook.String(), 0, nullptr, nullptr, 2.0f, false);
    return bResult;
}
