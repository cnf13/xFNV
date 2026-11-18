#include "Fallout Shared/TESCreature.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSTouchSpellForm.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/CreatureSounds.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/TESAIForm.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimation.h"
#include "Fallout Shared/TESAttackDamageForm.h"
#include "Fallout Shared/TESAttributes.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelList.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSpellList.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

#include <bit>

// The container entry the last CNTO chunk added, which a following COED chunk gives its extra data.
static ContainerObject* plastloaded;

static SettingT<GameSettingCollection> fCreatureCalcDamage("fCreatureCalcDamage", 1.0f);
SettingT<GameSettingCollection> gs_fCreatureCalcCombat("fCreatureCalcCombat", 2.0f);

static SettingT<GameSettingCollection> fCreatureCalcMagic("fCreatureCalcMagic", 2.0f);
static SettingT<GameSettingCollection> fCreatureCalcStealth("fCreatureCalcStealth", 2.0f);

// The record's chunks: the components' chunks, the base stats, AI data, packages, factions, spells and inventory, the
// voice, template, death item, body part data and weapon list IDs (kept in the pointers until InitItem resolves them),
// the creature data, hand reach, turn speed, foot weight, base scale, blood impact material, sound level, combat style,
// impact data set, sound creature and the creature sounds (a CSDT chunk picks the sound type the following CSDI/CSDF
// chunks fill). The NAM0 and NAM1 chunks are read and dropped. A warning follows when the speed multiplier is zero.
// When a chunk is empty (malformed data), the NAM4, NAM5, VTCK, TPLT, CSDT, PNAM and ZNAM values are initialised to 0
// and the EDID buffer to "".
bool TESCreature::Load(TESFile* apFile) {
    if (static_cast<uint8_t>(apFile->GetTESForm()) != 0x2B) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    // The sound type the next CSDI/CSDF chunks load; set by a CSDT chunk.
    int32_t iSoundType = -1;
    // Every chunk keeps the loop going; a creature sound that fails to load skips the next NextChunk.
    bool goodchunk;
    bool skipnext;
    do {
        uint32_t chunkid = apFile->GetTESChunk();
        if (chunkid == NO_CHUNK) {
            break;
        }
        skipnext = false;
        goodchunk = true;
        switch (chunkid) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pMessage = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten, so it is cleared first; the game does not do this.
                pMessage[0] = '\0';
                apFile->GetChunkData(pMessage, 0x200);
                SetFormEditorID(pMessage);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(this, apFile); break;
            // The ID chunks below hold form IDs, which are kept in the pointers (untyped until InitItem resolves them).
            case SCRI_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                TESScriptableForm::SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(tempint)));
                TESScriptableForm::InitItem(this);
                break;
            }
            case COED_ID:
            case CNTO_ID: {
                // Reads the chunk code a second time to tell the two apart.
                uint32_t uiChunk = apFile->GetTESChunk();
                if (uiChunk == COED_ID) {
                    if (plastloaded) {
                        void* pMemory = ourMalloc(sizeof(ContainerItemExtra));
                        plastloaded->pItemExtra = pMemory ? ::new (pMemory) ContainerItemExtra : nullptr;
                        plastloaded->pItemExtra->Load(apFile);
                        plastloaded = nullptr;
                    }
                }
                else if (uiChunk == CNTO_ID) {
                    CONTAINER_OBJECT_FILE cfile;
                    ourMemset(&cfile, 0, sizeof(cfile));
                    apFile->GetChunkData(&cfile, 8);
                    if (apFile->QEndian()) {
                        cfile.Endian();
                    }
                    plastloaded = TESContainer::AddObject(&cfile);
                }
                break;
            }
            case SPLO_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                TESSpellList::AddSpell(reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case EITM_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                BGSTouchSpellForm::SetFormTouchSpell(reinterpret_cast<MagicItemForm*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case EAMT_ID: {
                uint16_t stempval = 0;
                // LoadWord reads into a signed word; the animation group is the same 2 bytes, unsigned.
                apFile->LoadWord(reinterpret_cast<int16_t*>(&stempval));
                BGSTouchSpellForm::SetFormTouchSpellAnimation(stempval);
                break;
            }
            case AIDT_ID: {
                AIDATA kAIData;
                ourMemset(&kAIData, 0, sizeof(kAIData));
                apFile->GetChunkData(&kAIData, 0x14);
                if (apFile->QEndian()) {
                    kAIData.Endian();
                }
                // Before version 4 the aggression levels above 1 were one lower.
                if (apFile->GetFormVersion() < 4 && kAIData.cAggression > 1) {
                    ++kAIData.cAggression;
                }
                // Before version 6 one aggression value also stood for the assistance.
                if (apFile->GetFormVersion() < 6) {
                    switch (kAIData.cAggression) {
                        case 0: kAIData.cAssistance = 0; break;
                        case 1:
                            kAIData.cAggression = 0;
                            kAIData.cAssistance = 1;
                            break;
                        case 2:
                            kAIData.cAggression = 1;
                            kAIData.cAssistance = 1;
                            break;
                        case 3:
                            kAIData.cAggression = 1;
                            kAIData.cAssistance = 2;
                            break;
                        case 4:
                            kAIData.cAggression = 2;
                            kAIData.cAssistance = 2;
                            break;
                        case 5:
                            kAIData.cAggression = 3;
                            kAIData.cAssistance = 0;
                            break;
                        default: break;
                    }
                }
                // Before version 7 the confidence scale ran the other way.
                if (apFile->GetFormVersion() < 7) {
                    kAIData.cConfidence = static_cast<uint8_t>(4 - kAIData.cConfidence);
                }
                TESAIForm::SetAIData(&kAIData);
                break;
            }
            case PKID_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                TESAIForm::AddPackage(reinterpret_cast<TESPackage*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case SNAM_ID: {
                FACTION_RANK temp;
                ourMemset(&temp, 0, sizeof(temp));
                apFile->GetChunkData(&temp, 8);
#if XFNV_64BIT
                // The file's record is 8 bytes, the faction's 4-byte form ID first.
                TESFile::WidenFormIDRecords(&temp, 1, 8, sizeof(temp), {{0, offsetof(FACTION_RANK, pFaction)}});
#endif
                if (apFile->QEndian()) {
                    temp.Endian();
                }
                TESActorBaseData::SetFactionRank(temp.pFaction, static_cast<int8_t>(temp.cRank));
                break;
            }
            case INAM_ID: {
                uint32_t itempint = 0;
                apFile->GetChunkData(itempint);
                pDeathItem = reinterpret_cast<TESLevItem*>(static_cast<uintptr_t>(itempint));
                break;
            }
            case VTCK_ID: {
                uint32_t itempint = 0;
                apFile->GetChunkData(itempint);
                pVoiceType = reinterpret_cast<BGSVoiceType*>(static_cast<uintptr_t>(itempint));
                break;
            }
            case TPLT_ID: {
                uint32_t itempint = 0;
                apFile->GetChunkData(itempint);
                pTemplateForm = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(itempint));
                break;
            }
            case ACBS_ID:
                apFile->GetChunkData(&actorData, 0x18);
                if (apFile->QEndian()) {
                    actorData.Endian();
                }
                if (apFile->GetFormVersion() < 8) {
                    actorData.sTemplateUseFlags = 0;
                }
                actorData.usBartergold = 0;
                break;
            case KFFZ_ID: TESAnimation::LoadAnimationChunk(this, apFile); break;
            case NIFT_ID:
            case NIFZ_ID: TESModelList::LoadModelListChunk(this, apFile); break;
            case DEST_ID:
            case DSTD_ID: BGSDestructibleObjectForm::LoadChunk(this, apFile); break;
            case DATA_ID:
                // LoadData reads the chunk's bytes into an untyped buffer, so the creature data is passed as a char
                // pointer.
                LoadData(apFile, reinterpret_cast<char*>(&Data), 4);
                break;
            case NAM0_ID:
            case NAM1_ID:
                if (apFile->GetChunkSize()) {
                    // The chunk is read into a stack buffer and dropped.
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                    apFile->GetChunkData(pBuffer, 0);
                }
                break;
            case RNAM_ID:
                if (apFile->GetChunkSize() == 4) {
                    uint32_t tempint = 0;
                    apFile->GetChunkData(tempint);
                    cHandReach = static_cast<uint8_t>(tempint);
                }
                else {
                    apFile->GetChunkData(&cHandReach, 1);
                }
                break;
            case TNAM_ID: apFile->GetChunkData(fTurnSpeed); break;
            case WNAM_ID: apFile->GetChunkData(fFootWeight); break;
            case BNAM_ID: apFile->GetChunkData(fBaseScale); break;
            case NAM4_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                eBloodImpactMaterial = static_cast<BGSImpactDataSet::IMPACT_MATERIAL_TYPE>(tempint);
                break;
            }
            case NAM5_ID: {
                // An unknown sound level (above 3, unsigned) loads as normal.
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                if (tempint > 3) {
                    eSoundLevel = SOUND_LEVEL_NORMAL;
                }
                else {
                    eSoundLevel = static_cast<SOUND_LEVEL>(tempint);
                }
                break;
            }
            case ZNAM_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                SetCombatStyle(reinterpret_cast<TESCombatStyle*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case PNAM_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                pBodyPartData = reinterpret_cast<BGSBodyPartData*>(static_cast<uintptr_t>(tempint));
                break;
            }
            case CNAM_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                SetImpactDataSet(reinterpret_cast<BGSImpactDataSet*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case LNAM_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                SetWeaponList(reinterpret_cast<BGSListForm*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case CSCR_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                SetSoundCreature(reinterpret_cast<TESCreature*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case CSDT_ID: {
                int32_t tempint = 0;
                apFile->GetChunkData(tempint);
                iSoundType = tempint;
                break;
            }
            case CSDI_ID:
            case CSDF_ID:
                if (!CreateCreatureSounds()->LoadCreatureSound(
                        apFile, static_cast<CREATURE_SOUND_TYPE>(iSoundType), this
                    )) {
                    skipnext = true;
                }
                break;
            default: break;
        }
    }
    while (goodchunk && (skipnext || apFile->NextChunk()));
    if (!TESActorBaseData::GetSpeedMultiplier()) {
        uint32_t uiFormID = GetFormID();
        Warning("MASTERFILE: Speed multiplier is zero on creature '%s' (%08X).", GetFormEditorID(), uiFormID);
    }
    return true;
}

TESCreature::TESCreature() {
    SetFormType(CREA_ID);
    TESCreature::InitializeData();
    SetResponsibility(50u, false);
    SetAggression(1u, false);
    SetConfidence(2u, false);
    SetEnergy(2u, false);
    if (!pDataHandler->GetSaveLoad()) {
        SetFactionRank(pCreatureFaction, 0);
    }
}

TESCreature::~TESCreature() {
    TESCreature::ClearData();
}

void TESCreature::InitializeData() {
    Data.eType = 0;
    Data.iCombat = 50;
    Data.iOther = 50;
    Data.iStealth = 50;
    cHandReach = 32;
    pImpactDataSet = nullptr;
    pWeaponList = nullptr;
    fTurnSpeed = 0.0f;
    fFootWeight = 3.0f;
    fBaseScale = 1.0f;
    pSoundCreature = nullptr;
    pCombatStyle = nullptr;
    pBodyPartData = nullptr;
    bEmbeddedWeaponChecked = false;
    eSoundLevel = SOUND_LEVEL_NORMAL;
    eBloodImpactMaterial = BGSImpactDataSet::IMP_MAT_ORGANIC;
    cCanUseHandToHand = -1;
    SetFlagBit(BASE_CREATURE_WALKS, true, false);
    SetRunsInLow(false, false);
    TESHealthForm::SetFormHealth(50);
}

// Frees the creature's own sounds when it has them, then marks unarmed attacks as not yet checked.
void TESCreature::ClearData() {
    if (GetFlag(BASE_USE_TEMPLATE)) {
        CreatureSounds* pSounds = pCreatureSounds;
        if (pSounds) {
            delete pSounds;
        }
        pCreatureSounds = nullptr;
    }
    cCanUseHandToHand = -1;
}

void TESCreature::Save() {
    StartForm();
    TESFullName::Save();
    TESModel::Save(MODL_ID, MODT_ID);
    TESSpellList::Save();
    BGSTouchSpellForm::SaveSubRecord();
    TESModelList::Save(NIFZ_ID, NIFT_ID);
    TESActorBaseData::Save();
    BGSDestructibleObjectForm::Save();
    if (TESForm::QEndianSwapOnSave()) {}
    __SaveData(&Data, 4u);
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::AddChunk(RNAM_ID, cHandReach);
    if (GetCombatStyle()) {
        TESCombatStyle* v1 = GetCombatStyle();
        TESForm::AddChunk(ZNAM_ID, v1->iFormID);
    }
    if (GetBodyPartData()) {
        BGSBodyPartData* v2 = GetBodyPartData();
        TESForm::AddChunk(PNAM_ID, v2->iFormID);
    }
    TESForm::AddChunk(TNAM_ID, std::bit_cast<uint32_t>(fTurnSpeed));
    TESForm::AddChunk(BNAM_ID, std::bit_cast<uint32_t>(fBaseScale));
    TESForm::AddChunk(WNAM_ID, std::bit_cast<uint32_t>(fFootWeight));
    // Both enums are saved as 4-byte values through the uint32_t overload.
    TESForm::AddChunk(NAM4_ID, static_cast<uint32_t>(eBloodImpactMaterial));
    TESForm::AddChunk(NAM5_ID, static_cast<uint32_t>(eSoundLevel));
    if (GetFlag(256u)) {
        CreatureSounds* psounds = GetCreatureSounds();
        if (psounds) {
            psounds->Save();
        }
    }
    else {
        TESCreature* pcrea = GetSoundCreature();
        if (pcrea) {
            TESForm::AddChunk(CSCR_ID, pcrea->iFormID);
        }
    }
    if (pImpactDataSet) {
        TESForm::AddChunk(CNAM_ID, pImpactDataSet->iFormID);
    }
    if (pWeaponList) {
        TESForm::AddChunk(LNAM_ID, pWeaponList->iFormID);
    }
    CloseForm();
}

uint16_t TESCreature::GetSaveSize(uint32_t aiFlags) {
    uint16_t SaveSize = TESActorBase::GetSaveSize(aiFlags);
    if ((aiFlags & 512) != 0) {
        SaveSize += 3;
    }
    return SaveSize;
}

// Saves the actor base's changes, then the three skills when the save changed them (change flag 0x200).
void TESCreature::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESActorBase::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(0x200)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Data.iCombat), 1, 0);
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Data.iOther), 1, 0);
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Data.iStealth), 1, 0);
    }
}

void TESCreature::SaveGame(uint32_t aiFlags) {
    TESActorBase::SaveGame(aiFlags);
    if ((aiFlags & 512) != 0) {
        SaveGameDataOLD(&Data.iCombat, 1);
        SaveGameDataOLD(&Data.iOther, 1);
        SaveGameDataOLD(&Data.iStealth, 1);
    }
}

// Loads the actor base's changes, then the three skills when the save changed them (change flag 0x200).
void TESCreature::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESActorBase::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(0x200)) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&Data.iCombat), 1);
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&Data.iOther), 1);
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&Data.iStealth), 1);
    }
}

void TESCreature::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESActorBase::LoadGame(aiFlags, aiCurrentFlags);
    if ((aiFlags & 512) != 0) {
        LoadGameDataOLD(&Data.iCombat, 1);
        LoadGameDataOLD(&Data.iOther, 1);
        LoadGameDataOLD(&Data.iStealth, 1);
    }
}

void TESCreature::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESActorBase::Revert(apLoadGameBuffer);
}

// Resolves the loaded form IDs of the components and the creature's own references, warning about missing or
// invalid ones; a creature with no health is flagged with form flag 0x80000.
void TESCreature::InitItem() {
    if (GetInitialized()) {
        return;
    }
    TESScriptableForm::InitItem(this);
    TESContainer::InitItem(this);
    TESSpellList::InitItem(this);
    BGSTouchSpellForm::InitItem(this);
    TESAIForm::InitItem(this);
    TESActorBaseData::InitItem(this);
    BGSDestructibleObjectForm::InitItem(this);
    if (actorData.iActorBaseFlags & BASE_CREATURE_BIPED) {
        actorData.iActorBaseFlags &= ~BASE_CREATURE_WALKS;
        actorData.iActorBaseFlags &= ~BASE_CREATURE_SWIMS;
        actorData.iActorBaseFlags &= ~BASE_CREATURE_FLIES;
        actorData.iActorBaseFlags &= ~BASE_CREATURE_NO_COMBAT_IN_WATER;
        actorData.iActorBaseFlags &= ~BASE_CREATURE_IMMOBILE;
    }

    if (!GetFlag(BASE_USE_TEMPLATE)) {
        uint32_t iSoundCreatureID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundCreature));
        if (iSoundCreatureID) {
            TESForm::AddCompileIndex(iSoundCreatureID, GetFile(-1));
            pSoundCreature = dynamic_cast<TESCreature*>(TESForm::GetFormByNumericID(iSoundCreatureID));
            if (!pSoundCreature) {
                const char* pEditorID = GetFormEditorID();
                uint32_t iFormID = GetFormID();
                Warning(
                    "MASTERFILE: Sound Creature (%08X) not found in InitItem for Creature (%08X) '%s'.",
                    iSoundCreatureID, iFormID, pEditorID
                );
            }
            else if (pSoundCreature == this || pSoundCreature->QTemplateForm()) {
                uint32_t iFormID = GetFormID();
                const char* pEditorID = GetFormEditorID();
                uint32_t iSoundFormID = pSoundCreature->GetFormID();
                const char* pSoundEditorID = pSoundCreature->GetFormEditorID();
                Warning(
                    "MASTERFILE: Invalid sound creature '%s' (%08X) on creature '%s' (%08X).", pSoundEditorID,
                    iSoundFormID, pEditorID, iFormID
                );
                pSoundCreature = nullptr;
            }
        }
    }

    if (pCombatStyle) {
        uint32_t iCombatStyleID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetCombatStyle()));
        if (iCombatStyleID) {
            TESForm::AddCompileIndex(iCombatStyleID, GetFile(-1));
            pCombatStyle = dynamic_cast<TESCombatStyle*>(TESForm::GetFormByNumericID(iCombatStyleID));
            if (!pCombatStyle) {
                const char* pEditorID = GetFormEditorID();
                uint32_t iFormID = GetFormID();
                Warning(
                    "MASTERFILE: Combat Style (%08X) not found in InitItem for Creature (%08X) '%s'.", iCombatStyleID,
                    iFormID, pEditorID
                );
            }
        }
    }
    if (!GetCombatStyle()) {
        SetCombatStyle(TESCombatStyle::GetDefaultCombatStyle());
    }

    if (pBodyPartData) {
        uint32_t iBodyPartDataID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pBodyPartData));
        if (iBodyPartDataID) {
            TESForm::AddCompileIndex(iBodyPartDataID, GetFile(-1));
            pBodyPartData = dynamic_cast<BGSBodyPartData*>(TESForm::GetFormByNumericID(iBodyPartDataID));
            if (!pBodyPartData) {
                const char* pEditorID = GetFormEditorID();
                uint32_t iFormID = GetFormID();
                Warning(
                    "MASTERFILE: BodyPartData (%08X) not found in InitItem for Creature (%08X) '%s'.", iBodyPartDataID,
                    iFormID, pEditorID
                );
            }
        }
    }

    uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pImpactDataSet));
    if (iID) {
        TESForm::AddCompileIndex(iID, GetFile(-1));
        pImpactDataSet = dynamic_cast<BGSImpactDataSet*>(TESForm::GetFormByNumericID(iID));
        if (!pImpactDataSet) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning(
                "MASTERFILE: Could not find impact data set (%08X) for weapon '%s' (%08X).", iID, pEditorID, iFormID
            );
        }
    }

    iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWeaponList));
    if (iID) {
        TESForm::AddCompileIndex(iID, GetFile(-1));
        pWeaponList = dynamic_cast<BGSListForm*>(TESForm::GetFormByNumericID(iID));
        if (!pWeaponList) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning("Could not find list form (%08X) for weapon '%s' (%08X).", iID, pEditorID, iFormID);
        }
    }

    TESHealthForm* pHealthForm = this;
    if (!pHealthForm->GetFormHealth()) {
        SetHasTemp3D(true);
    }
    SetInitialized(true);
}

// Copies every component and the creature data of a creature form; the sounds are either copied or shared through
// the same sound creature.
void TESCreature::Copy(TESForm* apCopy) {
    TESCreature* pCopy = dynamic_cast<TESCreature*>(apCopy);
    if (!pCopy) {
        return;
    }
    if (GetCreatureSounds()) {
        CreatureSounds* pSounds = pCreatureSounds;
        if (pSounds) {
            delete pSounds;
        }
    }
    pCreatureSounds = nullptr;
    TESAIForm::CopyComponent(apCopy);
    TESSpellList::CopyComponent(apCopy);
    BGSTouchSpellForm::CopyComponent(apCopy);
    TESActorBaseData::CopyComponent(apCopy);
    TESHealthForm::CopyComponent(apCopy);
    TESAttributes::CopyComponent(apCopy);
    TESFullName::CopyComponent(apCopy);
    TESModel::CopyComponent(apCopy);
    TESModelList::CopyComponent(apCopy);
    TESScriptableForm::CopyComponent(apCopy);
    TESContainer::CopyComponent(apCopy);
    TESAnimation::CopyComponent(apCopy);
    BGSDestructibleObjectForm::CopyComponent(apCopy);
    CREATURE_DATA* pData = pCopy->GetData();
    ourMemcpy(&Data, pData, sizeof(CREATURE_DATA));
    actorData.iActorBaseFlags = *pCopy->GetFlags();
    SetHandReach(pCopy->GetHandReach());
    SetTurnSpeed(pCopy->GetTurnSpeed());
    SetBaseScale(pCopy->GetBaseScale());
    SetFootWeight(pCopy->GetFootWeight());
    TESActorBaseData* pCopyData = pCopy;
    TESActorBaseData* pThisData = this;
    pThisData->SetBloodImpactMaterial(pCopyData->GetBloodImpactMaterial());
    if (pCopy->GetCreatureSounds()) {
        CreatureSounds* pSounds = CreateCreatureSounds();
        pSounds->Copy(pCopy->GetCreatureSounds());
    }
    else {
        SetSoundCreature(pCopy->GetSoundCreature());
    }
    eSoundLevel = pCopy->GetSoundLevel();
    SetImpactDataSet(pCopy->GetImpactDataSet());
    SetWeaponList(pCopy->GetWeaponList());
    SetCombatStyle(pCopy->GetCombatStyle());
    SetBodyPartData(pCopy->GetBodyPartData());
}

bool TESCreature::Compare(TESForm* apCompare) {
    TESCreature* pComp = dynamic_cast<TESCreature*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    CREATURE_DATA* Data = pComp->GetData();
    if (std::memcmp(&this->Data, Data, sizeof(this->Data))) {
        return true;
    }
    int32_t DisableLightUpdate = cHandReach;
    if (DisableLightUpdate != pComp->cHandReach) {
        return true;
    }
    double TurnSpeed = GetTurnSpeed();
    if (TurnSpeed != pComp->GetTurnSpeed()) {
        return true;
    }
    double BaseScale = fBaseScale;
    if (BaseScale != pComp->fBaseScale) {
        return true;
    }
    double FootWeight = GetFootWeight();
    if (FootWeight != pComp->GetFootWeight()) {
        return true;
    }
    BGSImpactDataSet::IMPACT_MATERIAL_TYPE v8 = GetBloodImpactMaterial();
    if (v8 != pComp->GetBloodImpactMaterial()) {
        return true;
    }
    TESCombatStyle* v9 = GetCombatStyle();
    if (v9 != pComp->GetCombatStyle()) {
        return true;
    }
    BGSBodyPartData* v10 = GetBodyPartData();
    if (v10 != pComp->GetBodyPartData()) {
        return true;
    }
    SOUND_LEVEL SoundLevel_0 = GetSoundLevel();
    if (SoundLevel_0 != pComp->GetSoundLevel()) {
        return true;
    }
    if (GetFlag(256u)) {
        CreatureSounds* psounds = GetCreatureSounds();
        CreatureSounds* pcompsounds = pComp->GetCreatureSounds();
        if (psounds) {
            if (!pcompsounds) {
                return true;
            }
            if (psounds->Compare(pcompsounds)) {
                return true;
            }
        }
        else {
            if (pcompsounds) {
                return true;
            }
        }
    }
    else {
        TESCreature* SoundCreature = GetSoundCreature();
        if (SoundCreature != pComp->GetSoundCreature()) {
            return true;
        }
    }
    BGSImpactDataSet* ImpactDataSet = GetImpactDataSet();
    if (ImpactDataSet != pComp->GetImpactDataSet()) {
        return true;
    }
    BGSListForm* WeaponList = GetWeaponList();
    return WeaponList != pComp->GetWeaponList();
}

bool TESCreature::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aicount
) {
    char msg[308];
    Actor* pactionActor = dynamic_cast<Actor*>(apActionRef);
    Actor* pactor = dynamic_cast<Actor*>(apItemActivated);
    if (!pactor || (!pactionActor || pactor->IsDying())) {
        return false;
    }
    BaseProcess* pprocess = pactor->pCurrentProcess;
    if (!pprocess) {
        return false;
    }
    if (pprocess->GetCommandingActor() && !pactor->IsDead(false)) {
        return false;
    }
    if (pprocess->GetFadeState()) {
        return false;
    }
    BaseProcess* CurrentAIProcess = pactionActor->pCurrentProcess;
    TESPackage* pactionpackage = CurrentAIProcess->GetPackageThatIsRunning();
    TESPackage* potherpackage = pprocess->GetPackageThatIsRunning();
    if (pactionActor != pPlayer) {
        TESPackage* peditorpack = (&pactionActor->m_Extra)->GetPackageExtra();
        if ((peditorpack) && (peditorpack->GetPackType() == PACKAGE_TYPE_DIALOGUE)) {
            TESObjectREFR* PackageExtraTarget = (&pactor->m_Extra)->GetPackageExtraTarget();
            (&pactionActor->m_Extra)->SetPackageExtra(peditorpack, 4, PackageExtraTarget, false, false, false);
            BaseProcess* v32 = pactor->pCurrentProcess;
            v32->PackageDone(pactionActor, peditorpack);
            if (peditorpack->GetOncePerDay()) {
                char Day = appCalendar.GetDay();
                pactionActor->SetRunOnceExtraPackage(peditorpack, Day);
            }
            pactor->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        }
    }
    FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
    if (pactor->IsUnConscious() ||
        (pactor->IsKnockedOut() || pactor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN)) {
        if (GetCreatureType() != 6) {
            const char* v14 = sNoTalkUnConscious.String();
            std::sprintf(msg, "%s %s", pactor->GetFullName(), v14);
            Interface::ShowMessage(
                msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
            );
        }
        return false;
    }
    if (pactor->IsFleeing(false)) {
        CombatController* pcombat = pactor->GetCombatController();
        if (!pcombat || (!pcombat->IsActoraCombatTarget(pPlayer) || pPlayer->GetBlocked())) {
            const char* v15 = sNoTalkFleeing.String();
            std::sprintf(msg, "%s %s", pactor->GetFullName(), v15);
            Interface::ShowMessage(
                msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false
            );
            return false;
        }
        return true;
    }
    if (pactor->IsDead(false)) {
        if (pactionActor == pPlayer) {
            Interface::QueueMenuCreate(
                Interface::OPEN_CONTAINER, apItemActivated, nullptr, nullptr, Interface::CMM_STANDARD, false
            );
        }
        else {
            if ((apObjecttoGet) && (!dynamic_cast<TESNPC*>(apObjecttoGet))) {
                apItemActivated->RemoveItem(
                    apObjecttoGet, nullptr, aicount, false, false, pactionActor, nullptr, nullptr, true, false
                );
            }
        }
        return true;
    }
    if (pactionActor != pPlayer && pactionActor->GetShouldAttackActor(pactor, false, ereaction, false)) {
        BaseProcess* v31 = pactionActor->pCurrentProcess;
        v31->EnterCombat(
            pactionActor, pactor, false, true, nullptr, false, false, true, 0, false, false, true, nullptr
        );
        return true;
    }
    if (pactionActor == pPlayer && (pactor->IsKnockedOut() && pactor->GetEssential())) {
        return false;
    }
    TESActorBaseData* v30 = static_cast<TESActorBaseData*>(pactor->GetActorBase());
    if (v30->GetAllowPickPocket() &&
        (!pactor->IsInCombat() &&
         (pactionActor == pPlayer &&
          (((pactor->IsKnockedOut() && !pactor->GetEssential()) ||
            (pPlayer->IsSneaking() && !pactor->bPlayerTeammate)) &&
           !pactor->GetGhost())))) {
        if (!pactor->IsKnockedOut()) {
            BaseProcess* pspeaker = pactor->pCurrentProcess;
            if (pspeaker->GetPickpocketed()) {
                const char* v16 = gs_sNoPickPocketAgain.String();
                std::sprintf(msg, "%s %s", pactor->GetFullName(), v16);
                Interface::ShowMessage(
                    msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false
                );
                return false;
            }
        }
        Interface::QueueMenuCreate(
            Interface::OPEN_CONTAINER, apItemActivated, nullptr, nullptr, Interface::CMM_PICKPOCKET, false
        );
        return true;
    }
    if (!pactor->CanTalk() && pactionActor == pPlayer) {
        if (!pactor->bPlayerTeammate || apActionRef != pPlayer) {
            return true;
        }
        if (Actor::IsConsciousAndNotAttackingPlayer(pactor)) {
            Interface::QueueMenuCreate(
                Interface::OPEN_COMPANION_WHEEL, apItemActivated, nullptr, nullptr, Interface::CMM_NONE, false
            );
        }
        return false;
    }

    if (!pactor->bPlayerTeammate || apActionRef != pPlayer) {
        DialogueItem* pgreetDialogue;
        TESTopic* pgreetTopic;
        if (pactionActor == pPlayer) {
            pgreetTopic = TESTopic::GetTopic(DT_TOPIC, 0);
        }
        else {
            pgreetTopic = nullptr;
        }
        if (pgreetTopic) {
            pgreetDialogue = pgreetTopic->CreateDialogueItem(pactor, pPlayer, nullptr, nullptr, false);
        }
        else {
            pgreetDialogue = nullptr;
        }
        if (pgreetDialogue &&
            (pgreetDialogue->pInfo->IsGoodbye() &&
             (pgreetDialogue->FirstResponse() && !pgreetDialogue->NextResponse()))) {
            pactor->SayForceGoodbyeGreetingToPlayer(pgreetTopic, false, false);
        }
        else {
            if (!pgreetDialogue) {
                return false;
            }
            if (pactor->IsInCombat() && (pactionActor == pPlayer && pactor->bPlayerTeammate)) {
                pVATS.QuitVATSPlayback(false, false);
                Interface::QueueMenuCreate(
                    Interface::OPEN_DIALOG, pactor, nullptr, nullptr, Interface::CMM_STANDARD, false
                );
            }
            else {
                if ((pactor) && (!pactor->IsInCombat() || pactor->IsKnockedOut())) {
                    BaseProcess* v25;
                    BaseProcess* v26;
                    if (pactionActor != pPlayer ||
                        (pactor->GetSitSleepState() == SIT_SLEEP_NORMAL ||
                         (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                          (pactor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                           ((v26 = pactor->pCurrentProcess, v26->GetFurnitureMarker()) &&
                            (v25 = pactor->pCurrentProcess, v25->GetFurnitureMarker()->cNumber > 19u)))))) {
                        TESTopic* ptopic = nullptr;
                        if (pactionActor == pPlayer) {
                            if (!pactor->IsPathingComplete()) {
                                BaseProcess* v24 = pactor->pCurrentProcess;
                                v24->EndMoveMessage(pactor);
                            }
                            pactionActor->StopAttack();
                            pactor->StopAttack();
                            if ((pactor->GetCurrentTarget() == pPlayer) && (potherpackage) &&
                                (potherpackage->GetPackType() == PACKAGE_TYPE_EXPLORE)) {
                                if (!potherpackage->GetPackageSchedule() ||
                                    potherpackage->GetPackageSchedule()->PSData.iDuration) {
                                    pprocess->AddToProcedureIndexRunning(pactor, 1);
                                }
                                else {
                                    pprocess->AddToProcedureIndexRunning(pactionActor, 2);
                                }
                            }

                            BSAudio* v9 = &BSAudio::QInstance();
                            v9->EndAllSoundsOfType(4u);
                            pactor->InterruptCast();
                            pactor->ForceIdleAnimation();
                            pactionActor->ForceIdleAnimation();
                            if (pprocess->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                                pactor->InitiateDialogue(
                                    pPlayer, nullptr, nullptr, true, false, false, nullptr, false, false
                                );
                            }
                            else {
                                pVATS.QuitVATSPlayback(false, false);
                                Actor* pact = pactionActor;
                                if (pactionActor == pPlayer) {
                                    pact = pactor;
                                }
                                Interface::QueueMenuCreate(
                                    Interface::OPEN_DIALOG, pact, nullptr, nullptr, Interface::CMM_STANDARD, false
                                );
                            }
                        }
                        else {
                            if (Interface::GetMenuModeType() == Interface::eDialogMenu || pactor != pPlayer) {
                                if ((pactor != pactionActor) && (pactor->GetCurrentPackage())) {
                                    TESPackage* CurrentPackage = pactor->GetCurrentPackage();
                                    if (CurrentPackage->GetPackType() != PACKAGE_TYPE_FOLLOW) {
                                        TESPackage* v13 = pactor->GetCurrentPackage();
                                        if (v13->GetPackType() != PACKAGE_TYPE_ESCORT) {
                                            pprocess->AddToProcedureIndexRunning(pactor, 1);
                                        }
                                    }
                                }

                                if (pactor != pactionActor && pactor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                                    NiPoint3 v48 = pactionActor->GetLocationOnReference();
                                    pactor->RequestRotate(v48, false);
                                }
                                if (pactionActor->IsinDialogue()) {
                                    BaseProcess* v17 = pactionActor->pCurrentProcess;
                                    v17->AddToProcedureIndexRunning(pactionActor, 1);
                                }
                                else {
                                    if (!pactionpackage->GetPackageSchedule() ||
                                        pactionpackage->GetPackageSchedule()->PSData.iDuration) {
                                        BaseProcess* v19 = pactionActor->pCurrentProcess;
                                        v19->AddToProcedureIndexRunning(pactionActor, 1);
                                    }
                                    else {
                                        BaseProcess* v20 = pactionActor->pCurrentProcess;
                                        v20->AddToProcedureIndexRunning(pactionActor, 2);
                                    }
                                    if (pactionActor->InitiateDialogue(
                                            pactor, nullptr, nullptr, true, false, false, nullptr, false, false
                                        )) {
                                        BaseProcess* v18 = pactionActor->pCurrentProcess;
                                        v18->AddToProcedureIndexRunning(pactionActor, 2);
                                        pprocess->AddToProcedureIndexRunning(pactor, 2);
                                    }
                                }
                            }
                            else {
                                BaseProcess* v23 = pactor->pCurrentProcess;
                                if (!v23->CanForceGreet(pactionActor)) {
                                    return false;
                                }
                                BSAudio* v10 = &BSAudio::QInstance();
                                v10->EndAllSoundsOfType(4u);
                                pactionActor->StopAttack();
                                pactor->StopAttack();
                                TESPackage* v11 = pactionActor->GetCurrentPackage();
                                DialoguePackage* ppack = dynamic_cast<DialoguePackage*>(v11);
                                if (ppack) {
                                    ptopic = ppack->ptopic;
                                }
                                if (!pactionpackage->GetPackageSchedule() ||
                                    pactionpackage->GetPackageSchedule()->PSData.iDuration) {
                                    BaseProcess* v21 = pactionActor->pCurrentProcess;
                                    v21->AddToProcedureIndexRunning(pactionActor, 1);
                                }
                                else {
                                    BaseProcess* v22 = pactionActor->pCurrentProcess;
                                    v22->AddToProcedureIndexRunning(pactionActor, 2);
                                }
                                pactor->InterruptCast();
                                pactor->ForceIdleAnimation();
                                pactionActor->ForceIdleAnimation();
                                pVATS.QuitVATSPlayback(false, false);
                                Actor* pact_2 = pactionActor;
                                if (pactionActor == pPlayer) {
                                    pact_2 = pactor;
                                }
                                Interface::QueueMenuCreate(
                                    Interface::OPEN_DIALOG, pact_2, ptopic, nullptr, Interface::CMM_STANDARD, false
                                );
                            }
                        }
                    }
                }
            }
        }
        delete pgreetDialogue;
        return true;
    }
    if (Actor::IsConsciousAndNotAttackingPlayer(pactor)) {
        Interface::QueueMenuCreate(
            Interface::OPEN_COMPANION_WHEEL, apItemActivated, nullptr, nullptr, Interface::CMM_NONE, false
        );
    }
    return false;
}

void TESCreature::UnClone3D(TESObjectREFR* apRequester) {
    // Releases each NIF of the creature's model list, found in the folder of its model.
    BSSimpleList<const char*>* pcreatureFileList = &nifFilenames;
    while (pcreatureFileList && !pcreatureFileList->IsEmpty()) {
        char pfullPathFilename[260];
        BSstrcpy(pfullPathFilename, 260, GetModel());
        char* pslash = std::strrchr(pfullPathFilename, 92);
        BSstrcpy(pslash + 1, 260 - (pslash - pfullPathFilename) - 1, pcreatureFileList->GetItem());
        pModelLoader->ReleaseFile(pfullPathFilename);
        pcreatureFileList = pcreatureFileList->GetNext();
    }
    TESObject::UnClone3D(apRequester);
}

NiAVObject* TESCreature::Clone3D(TESObjectREFR* apRequester) {
    if (GetUsesLeveledTemplate() && apRequester && !apRequester->HasLeveledCreatureCalced()) {
        CalcTemplateForRef(apRequester);
        TESBoundObject* pBase = apRequester->GetObjectReference();
        return pBase->Clone3D(apRequester);
    }
    return TESBoundObject::Clone3D(apRequester, false);
}

BGSBodyPartData* TESCreature::GetBodyPartData() {
    if (pBodyPartData) {
        return pBodyPartData;
    }
    return BGSBodyPartData::pDefaultBodyPartData;
}

void TESCreature::SetBodyPartData(BGSBodyPartData* apData) {
    pBodyPartData = apData;
}

void TESCreature::SetCombatStyle(TESCombatStyle* apCombatStyle) {
    pCombatStyle = apCombatStyle;
}

void TESCreature::SetActorValue(ActorValue::Index aiWhich, int32_t aiValue) {
    if (ActorValue::IsFlagSet(aiWhich, ActorValue::COMBAT_SKILL)) {
        SetCombatSkill(aiValue);
        AddChange(512);
    }
    else {
        if (aiWhich == ActorValue::UNARMED_DAMAGE) {
            if (aiValue >= 0) {
                if (aiValue <= 0xFFFF) {
                    SetDamage(aiValue);
                }
                else {
                    SetDamage(0xFFFF);
                }
            }
            else {
                SetDamage(0);
            }
        }
        else {
            TESActorBase::SetActorValue(aiWhich, aiValue);
        }
    }
}

void TESCreature::CopyFromTemplateForm(TESActorBase* apForceTemplate) {
    uint16_t scompositeflags;
    if (static_cast<TESActorBaseData*>(this)->pTemplateForm || apForceTemplate) {
        TESCreature defaultcreature;
        defaultcreature.pCombatStyle = TESCombatStyle::GetDefaultCombatStyle();
        TESCreature* pactor = nullptr;
        if (!static_cast<TESActorBaseData*>(this)->IsTemplateLeveled()) {
            pactor = static_cast<TESCreature*>(static_cast<TESActorBaseData*>(this)->pTemplateForm);
        }
        if ((apForceTemplate) && (apForceTemplate->GetFormType() == CREA_ID)) {
            pactor = static_cast<TESCreature*>(apForceTemplate);
        }

        if (pactor) {
            if ((pactor->pTemplateForm) && (!pactor->QCopiedTemplate())) {
                pactor->CopyFromTemplateForm(nullptr);
            }
        }
        else {
            pactor = &defaultcreature;
        }
        if (static_cast<TESActorBaseData*>(this)->GetTemplateUseFlag(TESActorBaseData::TU_SCRIPT)) {
            if (pactor) {
                TESScriptableForm::CopyComponent(&static_cast<TESScriptableForm&>(static_cast<TESActorBase&>(*pactor)));
            }
            else {
                TESScriptableForm::CopyComponent(nullptr);
            }
        }
        if (static_cast<TESActorBaseData*>(this)->GetTemplateUseFlag(TESActorBaseData::TU_INVENTORY)) {
            if (pactor) {
                TESContainer::CopyComponent(&static_cast<TESContainer&>(static_cast<TESActorBase&>(*pactor)));
            }
            else {
                TESContainer::CopyComponent(nullptr);
            }
        }
        if (static_cast<TESActorBaseData*>(this)->GetTemplateUseFlag(TESActorBaseData::TU_BASEDATA)) {
            TESFullName* v28;
            if (pactor) {
                v28 = &static_cast<TESFullName&>(static_cast<TESActorBase&>(*pactor));
            }
            else {
                v28 = nullptr;
            }
            TESFullName::CopyComponent(v28);
            uint32_t v33 = static_cast<TESActorBaseData*>(this)->actorData.iActorBaseFlags & 0xEF8FDDF5u;
            static_cast<TESActorBaseData*>(this)->SetFlags(
                static_cast<int32_t>(v33 | (pactor->actorData.iActorBaseFlags & 0x1070220Au))
            );
        }
        if (static_cast<TESActorBaseData*>(this)->GetTemplateUseFlag(TESActorBaseData::TU_MODEL)) {
            BGSDestructibleObjectForm* v25;
            TESAnimation* v26;
            TESModelList* v27;
            if (pactor) {
                TESModel::CopyComponent(&static_cast<TESModel&>(static_cast<TESActorBase&>(*pactor)));
            }
            else {
                TESModel::CopyComponent(nullptr);
            }
            if (pactor) {
                v27 = static_cast<TESModelList*>(pactor);
            }
            else {
                v27 = nullptr;
            }
            TESModelList::CopyComponent(v27);
            if (pactor) {
                v26 = &static_cast<TESAnimation&>(static_cast<TESActorBase&>(*pactor));
            }
            else {
                v26 = nullptr;
            }
            TESAnimation::CopyComponent(v26);
            BGSBodyPartData* v2 = pactor->GetBodyPartData();
            SetBodyPartData(v2);
            CopyBoundData(pactor);
            BGSImpactDataSet* ImpactDataSet = pactor->GetImpactDataSet();
            SetImpactDataSet(ImpactDataSet);
            BGSImpactDataSet::IMPACT_MATERIAL_TYPE v4 = pactor->GetBloodImpactMaterial();
            static_cast<TESActorBaseData*>(this)->SetBloodImpactMaterial(v4);
            if (pactor) {
                v25 = &static_cast<BGSDestructibleObjectForm&>(static_cast<TESActorBase&>(*pactor));
            }
            else {
                v25 = nullptr;
            }
            BGSDestructibleObjectForm::CopyComponent(v25);
            uint32_t v32 = static_cast<TESActorBaseData*>(this)->actorData.iActorBaseFlags & 0xB4707F8Eu;
            static_cast<TESActorBaseData*>(this)->SetFlags(
                static_cast<int32_t>(v32 | (pactor->actorData.iActorBaseFlags & 0x4B8F8071u))
            );
            if (pactor->GetCreatureSounds()) {
                CreatureSounds* psounds = CreateCreatureSounds();
                CreatureSounds* CreatureSounds_ = pactor->GetCreatureSounds();
                psounds->Copy(CreatureSounds_);
            }
            else {
                TESCreature* SoundCreature = pactor->GetSoundCreature();
                SetSoundCreature(SoundCreature);
            }
            eSoundLevel = pactor->GetSoundLevel();
        }
        if (!this->pSoundCreature) {
            if (pactor->GetCreatureSounds()) {
                CreatureSounds* psounds_2 = CreateCreatureSounds();
                CreatureSounds* v7 = pactor->GetCreatureSounds();
                psounds_2->Copy(v7);
            }
            else {
                TESCreature* v8 = pactor->GetSoundCreature();
                SetSoundCreature(v8);
            }
        }
        if (static_cast<TESActorBaseData*>(this)->GetTemplateUseFlag(TESActorBaseData::TU_TRAITS)) {
            static_cast<TESActorBaseData*>(this)->pDeathItem = pactor->pDeathItem;
            static_cast<TESActorBaseData*>(this)->SetVoiceType(pactor->GetVoiceType());
            float a2 = pactor->GetKarma();
            static_cast<TESActorBaseData*>(this)->SetKarma(a2);
            char CreatureType = pactor->GetCreatureType();
            SetCreatureType(CreatureType);
            pWeaponList = pactor->GetWeaponList();
            TESCombatStyle* v10 = pactor->GetCombatStyle();
            SetCombatStyle(v10);
        }
        if (static_cast<TESActorBaseData*>(this)->GetTemplateUseFlag(TESActorBaseData::TU_STATS)) {
            TESAttackDamageForm* v23;
            TESHealthForm* v24;
            uint16_t LevelExact = pactor->GetLevelExact();
            static_cast<TESActorBaseData*>(this)->SetLevel(LevelExact);
            bool MatchesPCLevel = pactor->GetMatchesPCLevel();
            static_cast<TESActorBaseData*>(this)->SetMatchesPCLevel(MatchesPCLevel);
            uint16_t CalcMin = pactor->GetCalcMin();
            static_cast<TESActorBaseData*>(this)->SetCalcLevelMin(CalcMin);
            uint16_t CalcLevelMax = pactor->GetCalcLevelMax();
            static_cast<TESActorBaseData*>(this)->SetCalcLevelMax(CalcLevelMax);
            if (pactor) {
                TESAttributes::CopyComponent(&static_cast<TESAttributes&>(static_cast<TESActorBase&>(*pactor)));
            }
            else {
                TESAttributes::CopyComponent(nullptr);
            }
            if (pactor) {
                v24 = &static_cast<TESHealthForm&>(static_cast<TESActorBase&>(*pactor));
            }
            else {
                v24 = nullptr;
            }
            TESHealthForm::CopyComponent(v24);
            if (pactor) {
                v23 = static_cast<TESAttackDamageForm*>(pactor);
            }
            else {
                v23 = nullptr;
            }
            TESAttackDamageForm::CopyComponent(v23);
            uint16_t SpeedMultiplier = pactor->GetSpeedMultiplier();
            static_cast<TESActorBaseData*>(this)->SetSpeedMult(SpeedMultiplier);
            uint16_t v16 = static_cast<TESActorBaseData*>(pactor)->GetFatigue();
            static_cast<TESActorBaseData*>(this)->SetFatigue(v16);
            uint8_t CombatSkill = pactor->GetCombatSkill();
            SetCombatSkill(CombatSkill);
            SetHandReach(pactor->GetHandReach());
            float a2a = pactor->GetTurnSpeed();
            SetTurnSpeed(a2a);
            float a2b = pactor->fBaseScale;
            SetBaseScale(a2b);
            float a2c = pactor->GetFootWeight();
            SetFootWeight(a2c);
        }
        if (static_cast<TESActorBaseData*>(this)->GetTemplateUseFlag(TESActorBaseData::TU_FACTIONS)) {
            if (pactor) {
                static_cast<TESActorBaseData*>(this)->CopyFactionList(pactor);
            }
            else {
                static_cast<TESActorBaseData*>(this)->CopyFactionList(nullptr);
            }
        }
        if (static_cast<TESActorBaseData*>(this)->GetTemplateUseFlag(TESActorBaseData::TU_SPELLS)) {
            TESSpellList* v22;
            if (pactor) {
                v22 = &static_cast<TESSpellList&>(static_cast<TESActorBase&>(*pactor));
            }
            else {
                v22 = nullptr;
            }
            TESSpellList::CopyComponent(v22);
            if (pactor) {
                BGSTouchSpellForm::CopyComponent(&static_cast<BGSTouchSpellForm&>(static_cast<TESActorBase&>(*pactor)));
            }
            else {
                BGSTouchSpellForm::CopyComponent(nullptr);
            }
        }
        if (static_cast<TESActorBaseData*>(this)->GetTemplateUseFlag(TESActorBaseData::TU_AIDATA)) {
            if (pactor) {
                CopyAIData(pactor);
            }
            else {
                CopyAIData(nullptr);
            }
        }
        if (static_cast<TESActorBaseData*>(this)->GetTemplateUseFlag(TESActorBaseData::TU_AIPACKAGES)) {
            if (pactor) {
                CopyAIPackageList(pactor);
            }
            else {
                CopyAIPackageList(nullptr);
            }
        }
        scompositeflags = 0;
        TESForm* pleveledtemplate = TESActorBaseData::GetLeveledTemplateInHierarchy(this, scompositeflags);
        if (pleveledtemplate) {
            static_cast<TESActorBaseData*>(this)->pTemplateForm = pleveledtemplate;
            static_cast<TESActorBaseData*>(this)->SetTemplateUseFlags(scompositeflags);
        }
        static_cast<TESActorBaseData*>(this)->SetCopiedTemplate(true);
    }
}

bool TESCreature::GetNoLeftArm() const {
    return GetFlag(BASE_CREATURE_NO_LEFT_ARM);
}

bool TESCreature::GetNoRightArm() const {
    return GetFlag(BASE_CREATURE_NO_RIGHT_ARM);
}

bool TESCreature::GetNoHead() const {
    return GetFlag(BASE_CREATURE_NO_HEAD);
}

bool TESCreature::GetNoShadow() const {
    return TESActorBaseData::GetFlag(0x80000);
}

bool TESCreature::GetNoBloodSpray() const {
    return TESActorBaseData::GetFlag(0x800);
}

void TESCreature::SetNoBloodSpray(bool abVal) {
    TESActorBaseData::SetFlagBit(0x800, abVal, true);
}

bool TESCreature::GetNoBloodDecal() const {
    return TESActorBaseData::GetFlag(0x1000);
}

void TESCreature::SetNoBloodDecal(bool abVal) {
    TESActorBaseData::SetFlagBit(0x1000, abVal, true);
}

void TESCreature::SetBloodImpactMaterial(BGSImpactDataSet::IMPACT_MATERIAL_TYPE aeMat) {
    eBloodImpactMaterial = aeMat;
}

uint16_t TESCreature::GetFatigue() const {
    uint16_t usFatigue = TESActorBaseData::GetFatigue();
    if (GetMatchesPCLevel()) {
        int16_t sLevel = static_cast<int16_t>(GetLevel());
        if (sLevel < 1) {
            sLevel = 1;
        }
        usFatigue = static_cast<uint16_t>(usFatigue * sLevel);
    }
    return usFatigue;
}

uint32_t TESCreature::GetFormHealth() const {
    if (GetMatchesPCLevel()) {
        int16_t slevel = GetLevel();
        if (slevel < 1) {
            slevel = 1;
        }
        return static_cast<uint16_t>(static_cast<const TESHealthForm*>(this)->iHealth * slevel);
    }

    return static_cast<uint16_t>(static_cast<const TESHealthForm*>(this)->iHealth);
}

int32_t TESCreature::GetActorValue(ActorValue::Index aiWhich) {
    if (static_cast<uint8_t>(ActorValue::IsFlagSet(aiWhich, ActorValue::COMBAT_SKILL))) {
        return GetCombatSkill();
    }
    if (aiWhich == ActorValue::UNARMED_DAMAGE) {
        TESAttackDamageForm* pDamageForm = this;
        return pDamageForm->GetAttackDamage();
    }
    return TESActorBase::GetActorValue(aiWhich);
}

// Unarmed damage. Once the idles are loaded, the first call outside a save load records whether the creature has
// hand-to-hand idles; without them the damage is 0. A creature matching the player's level adds
// fCreatureCalcDamage per level.
uint16_t TESCreature::GetAttackDamage() const {
    TESCreature* pThis = const_cast<TESCreature*>(this);
    if (pIdleManager->GetInitialized() && !pDataHandler->GetSaveLoad() && cCanUseHandToHand == -1) {
        TESModel* pModel = pThis;
        BSSimpleList<const char*>* pList = pIdleManager->GetRootFilenameList(pModel->GetModel(), AGW_HAND_TO_HAND);
        pThis->cCanUseHandToHand = pList && pList->GetItem();
    }
    if (!cCanUseHandToHand) {
        return 0;
    }
    if (!GetMatchesPCLevel()) {
        return TESAttackDamageForm::GetAttackDamage();
    }
    int16_t sLevel = static_cast<int16_t>(GetLevel());
    if (sLevel < 1) {
        sLevel = 1;
    }
    uint16_t usDamage = TESAttackDamageForm::GetAttackDamage();
    double dLevel = sLevel;
    // The product is truncated to 64 bits and the low 32 bits are added to the damage.
    int32_t iBonus = static_cast<int32_t>(static_cast<int64_t>(fCreatureCalcDamage.Float() * dLevel));
    return static_cast<uint16_t>(usDamage + iBonus);
}

bool TESCreature::IsHumanoidCreature() {
    char eType = GetCreatureType();
    return eType == CREATURE_TYPE_ABOMINATION || eType == CREATURE_TYPE_SUPERMUTANT ||
        eType == CREATURE_TYPE_FERAL_GHOUL || eType == CREATURE_TYPE_GIANT;
}

bool TESCreature::DoesFly() const {
    return GetFormType() == CREA_ID && GetFlag(BASE_CREATURE_FLIES);
}

void TESCreature::InitWorn(TESObjectREFR* apRef) {
    MobileObject* pMob = dynamic_cast<MobileObject*>(apRef);
    if (pMob && pMob->GetCurrentAIProcess()) {
        MiddleHighProcess* pProcess = dynamic_cast<MiddleHighProcess*>(pMob->GetCurrentAIProcess());
        if (pProcess) {
            BaseProcess* pBaseProcess = pProcess;
            pBaseProcess->RemoveAndAttachItems(pMob);
        }
    }
}

uint8_t TESCreature::GetCombatSkill() const {
    uint8_t iCombat = Data.iCombat;
    if (GetMatchesPCLevel()) {
        int16_t Level = GetLevel();
        if (Level < 1) {
            Level = 1;
        }
        iCombat += static_cast<int32_t>(gs_fCreatureCalcCombat.Float() * static_cast<double>(Level));
    }
    return iCombat;
}

// Removes everything apRef wears and clears its process's equipment, then equips the best weapon of any combat type
// that the creature can wear. Without abAllowChanges, the save game's thread change flag is off meanwhile.
void TESCreature::InitDefaultWorn(TESObjectREFR* apRef, bool abArmor, bool abWeapon, bool abAllowChanges) {
    bool bOldAllowChanges = true;
    if (!abAllowChanges) {
        bOldAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
    }
    if (apRef->IsActor()) {
        apRef->RemoveWeapon();
    }
    InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(apRef);
    pChanges->RemoveAllObjectsWorn(true, static_cast<Actor*>(apRef));
    Actor* pActor = dynamic_cast<Actor*>(apRef);
    if (pActor && pActor->GetCurrentAIProcess()) {
        pActor->GetCurrentAIProcess()->SetCurrentAmmo(nullptr);
        pActor->GetCurrentAIProcess()->SetCurrentShield(nullptr);
        pActor->GetCurrentAIProcess()->SetCurrentTorch(nullptr);
        pActor->GetCurrentAIProcess()->SetCurrentWeapon(nullptr, nullptr, false);
    }
    ItemChange* pBestWeapon = nullptr;
    float fBestValue = 0.0f;
    if (abWeapon && pChanges) {
        pBestWeapon = pChanges->GetBestWeapon(this, fBestValue, COMBAT_WEAPON_TYPE_COUNT, true);
    }
    if (pBestWeapon && CanWearItem(pBestWeapon->GetContainerObject()) &&
        !apRef->WearingObject(pBestWeapon->GetContainerObject())) {
        apRef->AddWornItem(pBestWeapon->GetContainerObject(), 1, nullptr, false);
    }
    if (pBestWeapon) {
        delete pBestWeapon;
    }
    if (!abAllowChanges) {
        pSaveLoadGame->SetThreadAllowChanges(bOldAllowChanges);
    }
    if (pActor && pActor->GetCurrentAIProcess()) {
        pActor->CastPermanentMagic(true);
    }
}

bool TESCreature::CanWearItem(TESForm* apItem) {
    int32_t FormType = apItem->GetFormType();
    if (FormType == 30 || (FormType > 39 && FormType <= 41)) {
        return true;
    }
    return apItem->GetFormType() == 40 && static_cast<TESObjectWEAP*>(apItem)->TESObjectWEAP::IsEmbeddedWeapon();
}

BGSListForm* TESCreature::GetWeaponList() {
    return pWeaponList;
}

void TESCreature::SetCombatSkill(uint8_t aiVal) {
    Data.iCombat = aiVal;
}

uint8_t TESCreature::GetHandReach() const {
    return cHandReach;
}

void TESCreature::SetHandReach(uint8_t acVal) {
    cHandReach = acVal;
}

float TESCreature::GetTurnSpeed() const {
    return fTurnSpeed;
}

void TESCreature::SetTurnSpeed(float afVal) {
    fTurnSpeed = afVal;
}

float TESCreature::GetFootWeight() const {
    return fFootWeight;
}

void TESCreature::SetFootWeight(float afVal) {
    fFootWeight = afVal;
}

void TESCreature::SetBaseScale(float afVal) {
    fBaseScale = afVal;
}

SOUND_LEVEL TESCreature::GetSoundLevel() const {
    return (SOUND_LEVEL)((int32_t)eSoundLevel);
}

SOUND_LEVEL TESCreature::GetSoundLevelCopy() {
    int32_t eSoundLevel;
    int32_t v3 = 0;
    eSoundLevel = (int32_t)this->eSoundLevel;
    switch (eSoundLevel) {
        case 0: {
            return static_cast<SOUND_LEVEL>((&iSoundLevelLoud)->UInt());
        }
        case 1: {
            return static_cast<SOUND_LEVEL>((&iSoundLevelNormal)->UInt());
        }
        case 2: {
            return static_cast<SOUND_LEVEL>((&iSoundLevelSilent)->UInt());
        }
    }
    return static_cast<SOUND_LEVEL>(v3);
}

BGSImpactDataSet* TESCreature::GetImpactDataSet() const {
    return pImpactDataSet;
}

void TESCreature::SetImpactDataSet(BGSImpactDataSet* apSet) {
    pImpactDataSet = apSet;
}

void TESCreature::SetWeaponList(BGSListForm* apSet) {
    pWeaponList = apSet;
}

TESCreature* TESCreature::GetSoundCreature() {
    if (GetFlag(BASE_USE_TEMPLATE)) {
        return nullptr;
    }
    return pSoundCreature;
}

// With own sounds, a non-null apCreature frees them and switches the creature to the sound creature; a null one
// leaves everything as it is.
void TESCreature::SetSoundCreature(TESCreature* apCreature) {
    if (GetFlag(BASE_USE_TEMPLATE)) {
        if (apCreature) {
            CreatureSounds* pSounds = pCreatureSounds;
            if (pSounds) {
                delete pSounds;
            }
            SetFlagBit(BASE_USE_TEMPLATE, false, true);
            pSoundCreature = apCreature;
        }
    }
    else {
        pSoundCreature = apCreature;
    }
}

CreatureSounds* TESCreature::GetCreatureSounds() {
    if (GetFlag(BASE_USE_TEMPLATE)) {
        return pCreatureSounds;
    }
    return nullptr;
}

// Switches the creature to its own sounds, creating them when it has none yet.
CreatureSounds* TESCreature::CreateCreatureSounds() {
    if (!GetFlag(BASE_USE_TEMPLATE) || !pCreatureSounds) {
        SetFlagBit(BASE_USE_TEMPLATE, true, true);
        void* pMemory = ourMalloc(sizeof(CreatureSounds));
        pCreatureSounds = pMemory ? ::new (pMemory) CreatureSounds : nullptr;
    }
    return pCreatureSounds;
}

// Picks from the creature's own sounds, else from its sound creature's, recursively.
TESSound* TESCreature::PickCreatureSound(CREATURE_SOUND_TYPE aeType) {
    if (GetFlag(BASE_USE_TEMPLATE)) {
        CreatureSounds* pSounds = GetCreatureSounds();
        if (pSounds) {
            return pSounds->PickCreatureSound(aeType);
        }
    }
    else {
        TESCreature* pSoundSource = GetSoundCreature();
        if (pSoundSource) {
            return pSoundSource->PickCreatureSound(aeType);
        }
    }
    return nullptr;
}

// After a removal the walk restarts at the list's head.
void TESCreature::CheckEmbeddedWeaponNodes(NiAVObject* ap3D) {
    if (bEmbeddedWeaponChecked || !ap3D) {
        return;
    }
    BSSimpleList<ContainerObject*>* pIter = TESContainer::GetObjectList();
    while (pIter) {
        ContainerObject* pCObj = pIter->GetItem();
        bool bRemoved = false;
        if (pCObj && pCObj->pObj->GetFormType() == WEAP_ID) {
            TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pCObj->pObj);
            if (pWeapon->IsEmbeddedWeapon() && !BSUtilities::GetObjectByName(ap3D, pWeapon->GetEmbeddedWeaponNode())) {
                RemoveContainerObject(pCObj);
                pIter = TESContainer::GetObjectList();
                bRemoved = true;
            }
        }
        if (!bRemoved) {
            pIter = pIter->GetNext();
        }
    }
    bEmbeddedWeaponChecked = true;
}

bool TESCreature::GetTiltFrontBack() {
    return TESActorBaseData::GetFlag(0x1000000);
}

bool TESCreature::GetTiltLeftRight() {
    return TESActorBaseData::GetFlag(0x2000000);
}

void TESCreature::SetCreatureType(char aeType) {
    Data.eType = aeType;
}

CREATURE_DATA* TESCreature::GetData() {
    return &Data;
}

ActorValueOwner::ActorValueOwner() {}

// The constructor only runs the bases' constructors.
TESActorBase::TESActorBase() {}

void TESActorBase::ModActorValue(ActorValue::Index aiWhich, int32_t aiValue) {
    int32_t v3 = GetActorValue(aiWhich);
    SetActorValue(aiWhich, aiValue + v3);
}

void TESActorBase::ModActorValue(ActorValue::Index aiWhich, float afValue) {
    float v3 = GetActorFloatValue(aiWhich) + afValue;
    SetActorValue(aiWhich, v3);
}
