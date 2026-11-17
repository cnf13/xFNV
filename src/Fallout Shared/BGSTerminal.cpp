#include "Fallout Shared/BGSTerminal.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/LockLevel.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/Menus/HackingMenu.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

TerminalServerTextArray s_TerminalServerTexts{&sTerminalServerText1, &sTerminalServerText2, &sTerminalServerText3,
                                              &sTerminalServerText4, &sTerminalServerText5, &sTerminalServerText6,
                                              &sTerminalServerText7, &sTerminalServerText8, &sTerminalServerText9,
                                              &sTerminalServerText10};

// Static local of BGSTerminal::Load: the script variable the SLSD/SCVR chunks being read belong to.
static ScriptVariable* pNewVariable;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> fHackLevelMult("fHackLevelMult", 0.25f);
SettingT<GameSettingCollection> sTerminalLocked("sTerminalLocked", "You cannot hack this computer.");
SettingT<GameSettingCollection> sTerminalServerText1("sTerminalServerText1", "-Server 1-");
SettingT<GameSettingCollection> sTerminalServerText2("sTerminalServerText2", "-Server 2-");
SettingT<GameSettingCollection> sTerminalServerText3("sTerminalServerText3", "-Server 3-");
SettingT<GameSettingCollection> sTerminalServerText4("sTerminalServerText4", "-Server 4-");
SettingT<GameSettingCollection> sTerminalServerText5("sTerminalServerText5", "-Server 5-");
SettingT<GameSettingCollection> sTerminalServerText6("sTerminalServerText6", "-Server 6-");
SettingT<GameSettingCollection> sTerminalServerText7("sTerminalServerText7", "-Server 7-");
SettingT<GameSettingCollection> sTerminalServerText8("sTerminalServerText8", "-Server 8-");
SettingT<GameSettingCollection> sTerminalServerText9("sTerminalServerText9", "-Server 9-");
SettingT<GameSettingCollection> sTerminalServerText10("sTerminalServerText10", "-Server 10-.");
SettingT<GameSettingCollection>
    sHackIneligible("sHackIneligible", "A %s skill of %d is required to hack this terminal.");

TERMINAL_MENU_ITEM::TERMINAL_MENU_ITEM() : pDisplayNote(nullptr), pSubMenu(nullptr), cFlags(0) {
    ResultScript.SetInitialized(false);
    ResultScript.SetTemporary();
    ResultScript.m_header.bIsQuestScript = false;
}

bool TERMINAL_MENU_ITEM::IsAutoAdd() {
    return pDisplayNote && (cFlags & 1);
}

bool TERMINAL_MENU_ITEM::ForcesRedraw() {
    return (cFlags & 2) != 0;
}

// Clears the script header's quest-script flag, then runs the script on apActivator with apActivator's locals as a
// partial script.
void TERMINAL_MENU_ITEM::RunResultScript(TESObjectREFR* apActivator) {
    ResultScript.GetHeader()->bIsQuestScript = false;
    ScriptLocals* pLocals = apActivator->TESObjectREFR::GetExtra()->GetScriptLocals();
    ResultScript.Run(apActivator, pLocals, nullptr, true);
}

void TERMINAL_MENU_ITEM::SetSelected(bool abVal) {
    uint8_t v2;
    if (abVal) {
        v2 = (uint8_t)(cFlags | 4);
    }
    else {
        v2 = (uint8_t)(cFlags & 251);
    }
    cFlags = v2;
}

bool TERMINAL_MENU_ITEM::IsSelected() const {
    return ((cFlags & 4) != 0);
}

void TERMINAL_MENU_ITEM::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BGSTerminal::BGSTerminal() {
    SetFormType(23u);
    BGSTerminal::InitializeData();
}

BGSTerminal::~BGSTerminal() {
    BGSTerminal::ClearData();
}

// Clears the password and the terminal data after the activator's defaults.
void BGSTerminal::InitializeData() {
    TESObjectACTI::InitializeData();
    pPassword = nullptr;
    ourMemset(&Data, 0, sizeof(Data));
}

void BGSTerminal::ClearData() {
    TESForm::InitializeDataComponent();
    ClearMenuItemList();
}

// Reads the terminal's chunks. ITXT starts a new menu item; the item chunks after it (its texts, result script,
// conditions, note, submenu and flags) fill that item. When a chunk is empty (malformed data), the EDID, ITXT, RNAM and
// SCVR buffers are initialised to "" (one zeroed byte, allocated only for an empty chunk).
bool BGSTerminal::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    TERMINAL_MENU_ITEM* pItem = nullptr;
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case EDID_ID: {
                // A 16-byte aligned buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten, so it is cleared first.
                pEditorID[0] = '\0';
                apFile->GetChunkData(pEditorID, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case MODL_ID:
            case MODD_ID:
            case MOSD_ID:
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case MODS_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
                apFile->GetChunkData(pBuffer, uiSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
                break;
            }
            case SCRI_ID: {
                uint32_t uiScript = 0;
                apFile->GetChunkData<uint32_t>(uiScript);
                TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
                // Stores the chunk's value in the pointer field unchanged.
                pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScript)));
                pScriptableForm->InitItem(this);
                break;
            }
            case DSTD_ID:
            case DEST_ID:
                BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
                break;
            case DESC_ID: TESDescription::LoadDescriptionData(static_cast<TESDescription*>(this), apFile); break;
            case SNAM_ID: {
                uint32_t uiSoundLoop = 0;
                apFile->GetChunkData<uint32_t>(uiSoundLoop);
                // Stores the chunk's value in the pointer field unchanged.
                SetLoopingSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundLoop)));
                break;
            }
            case PNAM_ID: {
                uint32_t uiPassword = 0;
                apFile->GetChunkData<uint32_t>(uiPassword);
                // Stores the chunk's value in the pointer field unchanged.
                SetPassword(reinterpret_cast<BGSNote*>(static_cast<uintptr_t>(uiPassword)));
                break;
            }
            case DNAM_ID: apFile->GetChunkData(&Data, 4); break;
            case ITXT_ID: {
                // A 16-byte aligned buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten, so it is cleared first.
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                void* pMemory = ourMalloc(sizeof(TERMINAL_MENU_ITEM));
                TERMINAL_MENU_ITEM* pNewItem = pMemory ? ::new (pMemory) TERMINAL_MENU_ITEM : nullptr;
                pNewItem->ResponseText.Set(pBuffer, 0);
                AddMenuItem(pNewItem);
                pItem = pNewItem;
                break;
            }
            case RNAM_ID:
                if (pItem) {
                    // A 16-byte aligned buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                    // An empty chunk leaves the buffer unwritten, so it is cleared first.
                    pBuffer[0] = '\0';
                    apFile->GetChunkData(pBuffer, 0);
                    pItem->ResultText.Set(pBuffer, 0);
                }
                break;
            case INAM_ID:
                if (pItem) {
                    uint32_t uiDisplayNote = 0;
                    apFile->GetChunkData<uint32_t>(uiDisplayNote);
                    // Stores the chunk's value in the pointer field unchanged.
                    pItem->pDisplayNote = reinterpret_cast<BGSNote*>(static_cast<uintptr_t>(uiDisplayNote));
                }
                break;
            case TNAM_ID:
                if (pItem) {
                    uint32_t uiSubMenu = 0;
                    apFile->GetChunkData<uint32_t>(uiSubMenu);
                    // Stores the chunk's value in the pointer field unchanged.
                    pItem->pSubMenu = reinterpret_cast<BGSTerminal*>(static_cast<uintptr_t>(uiSubMenu));
                }
                break;
            case ANAM_ID:
                if (pItem) {
                    apFile->GetChunkData(&pItem->cFlags, 1);
                }
                break;
            case CIDA_ID:
                if (pItem) {
                    pItem->Conditions.Load(apFile);
                }
                break;
            // The result script's chunks go to the current item, which is deliberately not null-checked, as the game
            // does.
            case SCHR_ID:
                apFile->GetChunkData(pItem->ResultScript.GetHeader(), 0);
                if (apFile->QEndian()) {
                    pItem->ResultScript.GetHeader()->Endian();
                }
                break;
            case SCDA_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                ourMemset(pBuffer, 0, uiChunkSize);
                apFile->GetChunkData(pBuffer, 0);
                pItem->ResultScript.SetCompileData(uiChunkSize, pBuffer);
                break;
            }
            case SLSD_ID:
            case SCVR_ID:
                // Reads the chunk code again here.
                if (apFile->GetTESChunk() == SLSD_ID) {
                    void* pMemory = ourMalloc(sizeof(ScriptVariable));
                    pNewVariable = pMemory ? ::new (pMemory) ScriptVariable : nullptr;
                    pNewVariable->Load(apFile);
                    pItem->ResultScript.GetVariableList()->AddTail(pNewVariable);
                }
                else if (pNewVariable) {
                    // A 16-byte aligned buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                    // An empty chunk leaves the buffer unwritten, so it is cleared first.
                    pBuffer[0] = '\0';
                    apFile->GetChunkData(pBuffer, 0x200);
                    pNewVariable->cName.Set(pBuffer, 0);
                }
                break;
            case SCRO_ID: {
                void* pMemory = ourMalloc(sizeof(Script::RefVariable));
                Script::RefVariable* pReferenced = pMemory ? ::new (pMemory) Script::RefVariable : nullptr;
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                pReferenced->form = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID));
                pItem->ResultScript.GetReferencedObjectList()->AddTail(pReferenced);
                break;
            }
            case SCRV_ID: {
                void* pMemory = ourMalloc(sizeof(Script::RefVariable));
                Script::RefVariable* pReferenced = pMemory ? ::new (pMemory) Script::RefVariable : nullptr;
                uint32_t uiVariableID = 0;
                apFile->GetChunkData(uiVariableID);
                pReferenced->varIdx = uiVariableID;
                pItem->ResultScript.GetReferencedObjectList()->AddTail(pReferenced);
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSTerminal::Save() {
    StartForm();
    TESFullName::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    TESScriptableForm::Save();
    BGSDestructibleObjectForm::Save();
    TESDescription::Save();
    if (pSoundLoop) {
        TESForm::AddChunk(SNAM_ID, pSoundLoop->iFormID);
    }
    if (pPassword) {
        TESForm::AddChunk(PNAM_ID, pPassword->iFormID);
    }
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::__AddChunkData(DNAM_ID, &Data, 4u);
    if (TESForm::QEndianSwapOnSave()) {}
    BSSimpleList<TERMINAL_MENU_ITEM*>* plist = GetMenuItemList();
    while (plist && !plist->IsEmpty()) {
        TERMINAL_MENU_ITEM* pitem = plist->m_item;
        plist = plist->m_pkNext;
        if (pitem->ResponseText.pString) {
            uint32_t v1 = BSstrlen(pitem->ResponseText.pString);
            TESForm::AddChunkArray(ITXT_ID, pitem->ResponseText.pString, v1 + 1);
        }
        else {
            TESForm::AddChunk(ITXT_ID, 0u);
        }
        if (pitem->ResultText.String()) {
            const char* v2 = pitem->ResultText.String();
            uint32_t v4 = BSstrlen(v2) + 1;
            const char* v3 = pitem->ResultText.String();
            TESForm::AddChunkArray(RNAM_ID, v3, v4);
        }
        else {
            TESForm::AddChunk(RNAM_ID, 0u);
        }
        TESForm::AddChunk(ANAM_ID, pitem->cFlags);
        if (pitem->pDisplayNote) {
            TESForm::AddChunk(INAM_ID, pitem->pDisplayNote->iFormID);
        }
        if (pitem->pSubMenu) {
            TESForm::AddChunk(TNAM_ID, pitem->pSubMenu->iFormID);
        }
        pitem->ResultScript.SaveResultScript();
        pitem->Conditions.Save();
    }
    CloseForm();
}

void BGSTerminal::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESObjectACTI::SaveGame(apSaveGameBuffer);
}

// Saves before version 0xD carry one more byte for a changed terminal (change flag 0x80000000); it is read and
// dropped.
void BGSTerminal::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectACTI::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(0x80000000) && apLoadGameBuffer->GetVersion() < 0xD) {
        uint8_t cUnused;
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cUnused), 1);
    }
}

void BGSTerminal::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {}

// Resolves the password note and each menu item's display note and submenu from their loaded form IDs, then
// initialises each item's result script and conditions, warning about anything that fails.
void BGSTerminal::InitItem() {
    if (GetInitialized()) {
        return;
    }
    TESObjectACTI::InitItem();
    // Until InitItem runs, the loader leaves raw form IDs in the pointer fields; they are read here as integers.
    uint32_t aiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPassword));
    if (aiID) {
        TESForm::AddCompileIndex(aiID, GetFile(-1));
        pPassword = dynamic_cast<BGSNote*>(TESForm::GetFormByNumericID(aiID));
        if (!pPassword) {
            Warning(
                "MASTERFILE: Unable to find password (%08X) on terminal '%s' (%08X).", aiID, GetFormEditorID(),
                GetFormID()
            );
        }
    }

    int32_t iItem = 0;
    BSSimpleList<TERMINAL_MENU_ITEM*>* pIter = GetMenuItemList();
    while (pIter && !pIter->IsEmpty()) {
        TERMINAL_MENU_ITEM* pItem = pIter->GetItem();
        pIter = pIter->GetNext();
        if (pItem->pDisplayNote) {
            aiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pItem->pDisplayNote));
            TESForm::AddCompileIndex(aiID, GetFile(-1));
            pItem->pDisplayNote = dynamic_cast<BGSNote*>(TESForm::GetFormByNumericID(aiID));
            if (!pItem->pDisplayNote) {
                Warning(
                    "MASTERFILE: Unable to find display note (%08X) for menu item %i on terminal '%s' (%08X).", aiID,
                    iItem, GetFormEditorID(), GetFormID()
                );
            }
        }
        if (pItem->pSubMenu) {
            aiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pItem->pSubMenu));
            TESForm::AddCompileIndex(aiID, GetFile(-1));
            pItem->pSubMenu = dynamic_cast<BGSTerminal*>(TESForm::GetFormByNumericID(aiID));
            if (!pItem->pSubMenu) {
                Warning(
                    "MASTERFILE: Unable to find submenu (%08X) for menu item %i on terminal '%s' (%08X).", aiID, iItem,
                    GetFormEditorID(), GetFormID()
                );
            }
        }

        uint32_t uiOldCount = MessageHandler::QWarningCount();
        MessageHandler::IncDisableWarningCount(true);
        MessageHandler::SetWarningCount(0);
        pItem->ResultScript.InitItem(this);
        MessageHandler::IncDisableWarningCount(false);
        uint32_t uiNewCount = MessageHandler::QWarningCount();
        MessageHandler::SetWarningCount(uiOldCount);
        if (uiNewCount) {
            Warning(
                "MASTERFILE: Errors were encountered during InitItem for result script on menu item %d for "
                "terminal '%s' (%08X)\n\nSee Warnings file for more information.",
                iItem, GetFormEditorID(), GetFormID()
            );
        }

        uiOldCount = MessageHandler::QWarningCount();
        MessageHandler::IncDisableWarningCount(true);
        MessageHandler::SetWarningCount(0);
        pItem->Conditions.InitItem(this);
        MessageHandler::IncDisableWarningCount(false);
        uiNewCount = MessageHandler::QWarningCount();
        MessageHandler::SetWarningCount(uiOldCount);
        if (uiNewCount) {
            Warning(
                "MASTERFILE: Errors were encountered during InitItem for conditions on menu item %d for "
                "terminal '%s' (%08X)\n\nSee Warnings file for more information.",
                iItem, GetFormEditorID(), GetFormID()
            );
        }
        ++iItem;
    }
    SetInitialized(true);
}

void BGSTerminal::Copy(TESForm* apCopy) {
    BGSTerminal* pcopy = dynamic_cast<BGSTerminal*>(apCopy);
    if (pcopy) {
        pPassword = pcopy->pPassword;
        ourMemcpy(&Data, &pcopy->Data, sizeof(Data));
        ClearMenuItemList();
        BSSimpleList<TERMINAL_MENU_ITEM*>* plist = pcopy->GetMenuItemList();
        while (plist && !plist->IsEmpty()) {
            TERMINAL_MENU_ITEM* pitem;
            TERMINAL_MENU_ITEM* pcopyitem = plist->m_item;
            plist = plist->m_pkNext;
            TERMINAL_MENU_ITEM* apMemory = static_cast<TERMINAL_MENU_ITEM*>(ourMalloc(sizeof(TERMINAL_MENU_ITEM)));
            if (apMemory) {
                pitem = ::new (apMemory) TERMINAL_MENU_ITEM;
            }
            else {
                pitem = nullptr;
            }
            pitem->ResponseText.Copy(pcopyitem->ResponseText);
            pitem->ResultText.Copy(pcopyitem->ResultText);
            pitem->pDisplayNote = pcopyitem->pDisplayNote;
            pitem->pSubMenu = pcopyitem->pSubMenu;
            pitem->cFlags = pcopyitem->cFlags;
            pitem->ResultScript.CopyResultScript(&pcopyitem->ResultScript);
            pitem->Conditions.Copy(&pcopyitem->Conditions, nullptr);
            AddMenuItem(pitem);
        }
        TESObjectACTI::Copy(pcopy);
    }
}

bool BGSTerminal::Compare(TESForm* apCompare) {
    BGSTerminal* pcomp = dynamic_cast<BGSTerminal*>(apCompare);
    if (!pcomp) {
        return true;
    }
    BGSNote* CriticalQueuedRefCount = pPassword;
    if (CriticalQueuedRefCount != pcomp->pPassword) {
        return true;
    }
    if (std::memcmp(&Data, &pcomp->Data, sizeof(Data))) {
        return true;
    }
    BSSimpleList<TERMINAL_MENU_ITEM*>* plist = GetMenuItemList();
    BSSimpleList<TERMINAL_MENU_ITEM*>* pcomplist = pcomp->GetMenuItemList();
    int32_t v4 = plist->ItemsInList();
    if (v4 != pcomplist->ItemsInList()) {
        return true;
    }
    while (pcomplist && (!pcomplist->IsEmpty() && (plist && !plist->IsEmpty()))) {
        TERMINAL_MENU_ITEM* pcompitem = pcomplist->m_item;
        TERMINAL_MENU_ITEM* pitem = plist->m_item;
        if (static_cast<uint8_t>(pitem->ResponseText.IsNotEqual(pcompitem->ResponseText))) {
            return true;
        }
        if (static_cast<uint8_t>(pitem->ResultText.IsNotEqual(pcompitem->ResultText))) {
            return true;
        }
        if (pitem->ResultScript.CompareResultScripts(&pcompitem->ResultScript)) {
            return true;
        }
        if (pitem->Conditions.Compare(&pcompitem->Conditions)) {
            return true;
        }
        if (pitem->pDisplayNote != pcompitem->pDisplayNote) {
            return true;
        }
        if (pitem->pSubMenu != pcompitem->pSubMenu) {
            return true;
        }
        if (pitem->cFlags != pcompitem->cFlags) {
            return true;
        }
        pcomplist = pcomplist->m_pkNext;
        plist = plist->m_pkNext;
    }
    return TESObjectACTI::Compare(pcomp);
}

bool BGSTerminal::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIDFlag,
    TESBoundObject* apObjectToGet,
    int32_t aiCount
) {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    if (Interface::IsPipBoyOpening()) {
        return false;
    }
    if (BGSOpenCloseForm::GetOpenState(apItemActivated) == BGSOpenCloseForm::OBJECT_OPEN) {
        HandleOpenFinish(apItemActivated, apActionRef);
    }
    else {
        if (apActionRef == pPlayer &&
            pPlayer->GetTerminalAccess(apItemActivated) == PlayerCharacter::TAT_HACK_INELIGIBLE) {
            BSStringT<char> strmsg;
            float time = GetMinSkillRequiredToHack(apItemActivated);
            int32_t afTime = static_cast<int32_t>(BSRoundToNearest(time, 1.0f));
            const char* timea = ActorValue::GetActorValueName(ActorValue::SCIENCE);
            strmsg.SPrintF(sHackIneligible.operator const char*(), timea, afTime);
            Interface::ShowMessage(
                strmsg.pString, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_angry.dds", nullptr, 2.0f,
                false
            );
            BSAudio* v7 = &BSAudio::QInstance();
            BSSoundHandle* SoundHandleByName = v7->GetSoundHandleByName(&a1, "UIPopUpMessageGeneral", 289u);
            SoundHandleByName->Play(false);
        }
        else {
            if (apActionRef == pPlayer && BGSTerminal::IsTerminalRefInUse(apItemActivated)) {
                const char* v8 = sObjectInUse.String();
                Interface::ShowMessage(v8, 0, nullptr, nullptr, 2.0f, false);
                return false;
            }
            Actor* pActionActor = static_cast<Actor*>(apActionRef);
            if ((apItemActivated->GetOwner()) && (!apItemActivated->IsAnOwner(pActionActor, true) && !IsUnlocked())) {
                bool bkarma = false;
                TESForm* powner = apItemActivated->GetOwner();
                int32_t pactor = 0;
                if (powner->GetFormType() == FACT_ID) {
                    bkarma = !static_cast<TESFaction*>(powner)->TESFaction::GetIsEvil();
                }
                else {
                    if (powner) {
                        float afTimea = static_cast<ActorValueOwner*>(static_cast<TESActorBase*>(powner))
                                            ->GetActorFloatValue(ActorValue::KARMA);
                        TESActorBaseData::ALIGNMENT ealign = TESActorBaseData::GetAlignmentForKarma(afTimea);
                        if (ealign != TESActorBaseData::ALIGNMENT_EVIL) {
                            bkarma = ealign != TESActorBaseData::ALIGNMENT_VERY_EVIL;
                        }
                    }
                }
                if (bkarma) {
                    double v9 = fKarmaModStealing.Float();
                    pPlayer->RewardKarma(static_cast<int32_t>(v9));
                }
            }

            BGSOpenCloseForm::HandleActivate(apItemActivated, apActionRef, nullptr);
        }
    }
    return true;
}

// When the player opens the terminal: the computers menu once it is open to them, the hacking game when it is locked
// but hackable, else the "hack ineligible" message with the science skill it needs. Then the open/close activation.
void BGSTerminal::HandleOpenFinish(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef) {
    if (!apItemActivated || apActionRef != pPlayer) {
        return;
    }
    switch (pPlayer->GetTerminalAccess(apItemActivated)) {
        case PlayerCharacter::TAT_HAS_NOTE:
        case PlayerCharacter::TAT_HACKED:
            Interface::QueueMenuCreate(
                Interface::OPEN_COMPUTERS, apItemActivated, nullptr, nullptr, Interface::CMM_STANDARD, false
            );
            break;
        case PlayerCharacter::TAT_LOCKED:
        case PlayerCharacter::TAT_HACK_ELIGIBLE:
            Interface::QueueMenuCreate(
                Interface::OPEN_HACKING, apItemActivated, nullptr, nullptr, Interface::CMM_STANDARD, false
            );
            break;
        case PlayerCharacter::TAT_HACK_INELIGIBLE: {
            alignas(BSStringT<char>) uint8_t aMessageStorage[sizeof(BSStringT<char>)];
            BSStringT<char>* pMessage = ::new (aMessageStorage) BSStringT<char>;
            // _ftol2_sse truncates the rounded skill.
            int32_t iSkill = static_cast<int32_t>(BSRoundToNearest(GetMinSkillRequiredToHack(apItemActivated), 1.0f));
            const char* pSkillName = ActorValue::GetActorValueName(ActorValue::SCIENCE);
            const char* pFormat = static_cast<const char*>(sHackIneligible);
            pMessage->SPrintF(pFormat, pSkillName, iSkill);
            Interface::ShowMessage(
                pMessage->String(), 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f,
                false
            );
            pMessage->~BSStringT<char>();
            break;
        }
        default: break;
    }
    BGSOpenCloseForm::HandleActivate(apItemActivated, apActionRef, nullptr);
}

float BGSTerminal::GetMinSkillRequiredToHack(TESObjectREFR* apItemActivated) {
    switch (GetHackDifficultyLockLevel(apItemActivated)) {
        case 0: return fHackingMinSkillVeryEasy.Float();
        case 1: return fHackingMinSkillEasy.Float();
        case 2: return fHackingMinSkillAverage.Float();
        case 3: return fHackingMinSkillHard.Float();
        case 4: return fHackingMinSkillVeryHard.Float();
        default: return 3.4028235e+38f;
    }
}

// The level is kept in a signed byte, sign-extended for the -1 test and the return; the terminal's own difficulty is
// the fallback when no reference state overrides it.
LOCK_LEVEL BGSTerminal::GetRawDifficulty(TESObjectREFR* apRef) {
    int8_t cLevel = (int8_t)-1;
    if (apRef) {
        cLevel = apRef->TESObjectREFR::GetExtra()->GetTerminalLockState();
    }
    if (cLevel < 0) {
        cLevel = (int8_t)Data.cDifficulty;
    }
    return static_cast<LOCK_LEVEL>(cLevel);
}

void BGSTerminal::ClearMenuItemList() {
    while (!MenuItemList.IsEmpty()) {
        TERMINAL_MENU_ITEM* v2 = MenuItemList.m_item;
        MenuItemList.RemoveHead();
        if (v2) {
            delete v2;
        }
    }
}

LOCK_LEVEL BGSTerminal::GetHackDifficultyLockLevel(TESObjectREFR* apRef) {
    uint32_t ilevel = GetRawDifficulty(apRef);
    if (IsLeveled()) {
        uint32_t icalclevel;
        if (apRef) {
            icalclevel = apRef->GetCalcLevel(false);
        }
        else {
            icalclevel = pPlayer->GetLevel();
        }
        ilevel += static_cast<int64_t>(fHackLevelMult.Float() * static_cast<double>(icalclevel));
        if (ilevel > 4) {
            return LL_VERYHARD;
        }
    }
    return static_cast<LOCK_LEVEL>(ilevel);
}

void BGSTerminal::SetPassword(BGSNote* apPassword) {
    pPassword = apPassword;
}

// Appends a menu item; a null item is ignored.
void BGSTerminal::AddMenuItem(TERMINAL_MENU_ITEM* apItem) {
    if (apItem) {
        MenuItemList.AddTail(apItem);
    }
}

BSSimpleList<TERMINAL_MENU_ITEM*>* BGSTerminal::GetMenuItemList() {
    return &MenuItemList;
}

bool BGSTerminal::IsLeveled() {
    return ((Data.cFlags & 1) != 0);
}

bool BGSTerminal::IsUnlocked() {
    return ((Data.cFlags & 2) != 0);
}

// apRef's terminal state overrides the base flag: -2 unlocks it, any level but -1 locks it.
bool BGSTerminal::IsUnlocked(TESObjectREFR* apRef) {
    bool bUnlocked = IsUnlocked();
    if (apRef) {
        int8_t cLockedLevel = apRef->GetExtra()->GetTerminalLockState();
        if (cLockedLevel == -2) {
            bUnlocked = true;
        }
        else if (cLockedLevel != -1) {
            bUnlocked = false;
        }
    }
    return bUnlocked;
}

uint32_t BGSTerminal::GetHackMiniGameValue(TESObjectREFR* apRef) {
    int32_t v4 = 2 * GetHackDifficultyLockLevel(apRef);
    int32_t v5 = (int32_t)((v4 + (TESForm::GetFormID() & 1)) + 4);
    if (v5 > 12) {
        return (uint32_t)12;
    }
    return (uint32_t)v5;
}

bool BGSTerminal::IsHackEligible(TESObjectREFR* apItemActivated, float afSkill) {
    return (bool)((uint8_t)(afSkill >= GetMinSkillRequiredToHack(apItemActivated)));
}

bool BGSTerminal::UsesAltColor() {
    return ((Data.cFlags & 4) != 0);
}

bool BGSTerminal::HidesWelcomeTextWhenDisplayingImage() {
    return ((Data.cFlags & 8) != 0);
}

void BGSTerminal::SetRawDifficulty(TESObjectREFR* apRef, LOCK_LEVEL aeVal) {
    ExtraDataList* v3;
    ExtraDataList* extra;
    if (apRef) {
        if ((((uint32_t)((int32_t)aeVal)) != ((uint32_t)Data.cDifficulty)) || IsUnlocked()) {
            extra = apRef->TESObjectREFR::GetExtra();
            extra->SetTerminalLockState(aeVal);
        }
        else {
            v3 = apRef->TESObjectREFR::GetExtra();
            v3->SetTerminalLockState(0xFFu);
        }
    }
}

void BGSTerminal::SetUnlocked(TESObjectREFR* apRef) {
    if (apRef) {
        if (IsUnlocked()) {
            ExtraDataList* Extra = apRef->GetExtra();
            Extra->SetTerminalLockState(-1);
        }
        else {
            ExtraDataList* v3 = apRef->GetExtra();
            v3->SetTerminalLockState(-2);
        }
    }
}

bool BGSTerminal::IsTerminalRefInUse(TESObjectREFR* apRef) {
    int8_t v3 = 0;
    if (!apRef) {
        return (bool)v3;
    }
    TESObjectREFR* linkedRef = apRef->GetLinkedRef();
    if (!linkedRef) {
        return (bool)v3;
    }
    if (linkedRef->IsFurniture()) {
        return (((int8_t)linkedRef->HasFreeMarker(true)) == 0);
    }
    return (bool)v3;
}

void BGSTerminal::ResetState(TESObjectREFR* apRef) {
    ExtraDataList* extraDataList;
    if (apRef) {
        extraDataList = apRef->TESObjectREFR::GetExtra();
        extraDataList->BaseExtraList::RemoveExtra(EXTRA_TERMINALSTATE);
    }
}

void BGSTerminal::SetLockedOut(TESObjectREFR* a1, char a2) {
    ExtraDataList* extra;
    ExtraDataList* v3;
    uint8_t isLockedOut;
    uint8_t v5;
    if (a1) {
        extra = a1->TESObjectREFR::GetExtra();
        isLockedOut = extra->GetTerminalState();
        if (a2) {
            v5 = (uint8_t)(isLockedOut + 1);
        }
        else {
            v5 = 0;
        }
        v3 = a1->TESObjectREFR::GetExtra();
        v3->SetTerminalState(v5);
        a1->AddChange((int32_t)0x80000000u);
    }
}

SCRIPT_HEADER* Script::GetHeader() {
    return &m_header;
}

BSSimpleList<Script::RefVariable*>* Script::GetReferencedObjectList() {
    return &listRefObjects;
}
