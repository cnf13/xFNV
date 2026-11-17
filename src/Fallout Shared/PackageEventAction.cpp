#include "Fallout Shared/PackageEventAction.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/SCRIPT_REFERENCED_OBJECT.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "SDK/MSVC/rtti.h"

#include <stdint.h>

PackageEventAction::PackageEventAction() : pIdle(nullptr), pResultScript(nullptr), pTopic(nullptr), eType(PEAT_BEGIN) {}

// Clears the idle and topic and deletes the result script.
PackageEventAction::~PackageEventAction() {
    pIdle = nullptr;
    pTopic = nullptr;
    delete pResultScript;
}

void PackageEventAction::Copy(PackageEventAction* apCopy) {
    pIdle = apCopy->pIdle;
    pTopic = apCopy->pTopic;
    SetType(apCopy->eType);
    if (apCopy->pResultScript) {
        if (!pResultScript) {
            Script* v2;
            Script* apMemory = static_cast<Script*>(ourMalloc(sizeof(Script)));
            if (apMemory) {
                v2 = ::new (apMemory) Script;
            }
            else {
                v2 = 0;
            }
            pResultScript = v2;
            pResultScript->SetTemporary();
        }
        pResultScript->CopyResultScript(apCopy->pResultScript);
    }
    else {
        if (pResultScript) {
            delete pResultScript;
        }
        pResultScript = 0;
    }
}

void PackageEventAction::Load(TESFile* apFile) {
    // The local variable the last SLSD chunk created; the SCVR chunks that follow fill its
    // name.
    static ScriptVariable* pLastVariable;
    pIdle = nullptr;
    pTopic = nullptr;
    if (pResultScript) {
        delete pResultScript;
        pResultScript = nullptr;
    }
    void* pScriptMemory = ourMalloc(sizeof(Script));
    Script* pScript = nullptr;
    if (pScriptMemory) {
        pScript = ::new (pScriptMemory) Script;
    }
    pResultScript = pScript;
    bool bDone = false;
    while (true) {
        uint32_t uiChunk = apFile->GetTESChunk();
        switch (uiChunk) {
            case SCHR_ID:
                apFile->GetChunkData(pResultScript->GetHeader(), 0);
                if (apFile->QEndian()) {
                    pResultScript->GetHeader()->Endian();
                }
                break;
            case SCVR_ID:
            case SLSD_ID:
                // The chunk is read again: an SLSD chunk makes the variable, a SCVR one names it.
                if (apFile->GetTESChunk() == SLSD_ID) {
                    void* pVariableMemory = ourMalloc(sizeof(ScriptVariable));
                    ScriptVariable* pVariable = nullptr;
                    if (pVariableMemory) {
                        pVariable = ::new (pVariableMemory) ScriptVariable;
                    }
                    pLastVariable = pVariable;
                    pVariable->Load(apFile);
                    pResultScript->GetVariableList()->AddTail(pVariable);
                }
                else if (pLastVariable) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                    apFile->GetChunkData(pBuffer, 0x200);
                    pLastVariable->cName.Set(pBuffer, 0);
                }
                break;
            case SCRO_ID: {
                void* pRefMemory = ourMalloc(sizeof(Script::RefVariable));
                Script::RefVariable* pRef = nullptr;
                if (pRefMemory) {
                    pRef = ::new (pRefMemory) Script::RefVariable;
                }
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                // The chunk holds a form ID that stands in for the form.
                pRef->form = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID));
                pResultScript->GetReferencedObjectList()->AddTail(pRef);
                break;
            }
            case SCDA_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
                ourMemset(pBuffer, 0, uiSize);
                apFile->GetChunkData(pBuffer, 0);
                pResultScript->SetCompileData(uiSize, pBuffer);
                break;
            }
            case SCRV_ID: {
                void* pRefMemory = ourMalloc(sizeof(Script::RefVariable));
                Script::RefVariable* pRef = nullptr;
                if (pRefMemory) {
                    pRef = ::new (pRefMemory) Script::RefVariable;
                }
                uint32_t uiVariableID = 0;
                apFile->GetChunkData<uint32_t>(uiVariableID);
                pRef->varIdx = uiVariableID;
                pResultScript->GetReferencedObjectList()->AddTail(pRef);
                break;
            }
            case INAM_ID: {
                uint32_t uiIdleID = 0;
                apFile->GetChunkData<uint32_t>(uiIdleID);
                // The chunk holds a form ID that stands in for the idle.
                pIdle = reinterpret_cast<TESIdleForm*>(static_cast<uintptr_t>(uiIdleID));
                break;
            }
            case TNAM_ID: {
                uint32_t uiTopicID = 0;
                apFile->GetChunkData<uint32_t>(uiTopicID);
                // The chunk holds a form ID that stands in for the topic; it ends the action's chunks.
                pTopic = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(uiTopicID));
                bDone = true;
                break;
            }
        }
        if (bDone) {
            break;
        }
        apFile->NextChunk();
    }
    pResultScript->SetInitialized(false);
    if (!pResultScript->GetCompiledData()) {
        delete pResultScript;
        pResultScript = nullptr;
    }
}

void PackageEventAction::InitItem(TESForm* apOwner) {
    uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->QIdle()));
    TESFile* File = apOwner->GetFile(-1);
    TESForm::AddCompileIndex(tempint, File);
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempint);
    TESIdleForm* pidle = dynamic_cast<TESIdleForm*>(FormByNumericID);
    if (pidle) {
        pIdle = pidle;
    }
    else {
        if (tempint) {
            if (apOwner->GetFormEditorIDLength()) {
                const char* v6 = apOwner->GetFormEditorID();
                Warning("MASTERFILE: Unable to find Idle (%08X) on owner object \"%s\".", tempint, v6);
            }
            else {
                int32_t CastingType = apOwner->iFormID;
                Warning("MASTERFILE: Unable to find Idle (%08X) on owner object (%08X).", tempint, CastingType);
            }
            pIdle = 0;
        }
    }
    tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->QTopic()));
    TESFile* v4 = apOwner->GetFile(-1);
    TESForm::AddCompileIndex(tempint, v4);
    TESForm* v5 = TESForm::GetFormByNumericID(tempint);
    TESTopic* ptopic = dynamic_cast<TESTopic*>(v5);
    if (ptopic) {
        pTopic = ptopic;
    }
    else {
        if (tempint) {
            if (apOwner->GetFormEditorIDLength()) {
                const char* v8 = apOwner->GetFormEditorID();
                Warning("MASTERFILE: Unable to find Topic (%08X) on owner object \"%s\".", tempint, v8);
            }
            else {
                int32_t v9 = apOwner->iFormID;
                Warning("MASTERFILE: Unable to find Topic (%08X) on owner object (%08X).", tempint, v9);
            }
            pTopic = 0;
        }
    }
    if (pResultScript) {
        int32_t iprevwarnings = MessageHandler::QWarningCount();
        MessageHandler::IncDisableWarningCount(1);
        MessageHandler::SetWarningCount(0);
        pResultScript->SetTemporary();
        pResultScript->InitItem(apOwner);
        (&pResultScript->m_header)->bIsQuestScript = 0;
        MessageHandler::IncDisableWarningCount(0);
        int32_t iwarnings = MessageHandler::QWarningCount();
        MessageHandler::SetWarningCount(iprevwarnings);
        if (iwarnings) {
            int32_t v10 = apOwner->iFormID;
            Warning(
                "MASTERFILE: Errors were encountered during InitItem for result script on eventaction for package "
                "'%s' (%08X)\n\nSee Warnings file for more information.",
                apOwner->GetFormEditorID(), v10
            );
        }
    }
}

void PackageEventAction::Save(CHUNK_ID aeChunk) {
    uint32_t v2;
    uint32_t v3;
    TESForm::AddChunk(aeChunk);
    if (pIdle) {
        v3 = pIdle->TESForm::GetFormID();
    }
    else {
        v3 = 0;
    }
    TESForm::AddChunk(INAM_ID, v3);
    if (pTopic) {
        v2 = pTopic->TESForm::GetFormID();
    }
    else {
        v2 = 0;
    }
    if (pResultScript) {
        pResultScript->SaveResultScript();
    }
    TESForm::AddChunk(TNAM_ID, v2);
}

bool PackageEventAction::Compare(PackageEventAction* apCompare) {
    if (this->QIdle() != apCompare->pIdle) {
        return 1;
    }
    if (this->QTopic() != apCompare->pTopic) {
        return 1;
    }
    if (!pResultScript && !apCompare->pResultScript) {
        return 0;
    }
    if (!pResultScript || !apCompare->pResultScript) {
        return 1;
    }
    return pResultScript->CompareResultScripts(apCompare->pResultScript);
}

void TESPackageOnBeginEndOrChange_Save(PackageEventAction* apThis, [[maybe_unused]] void* apEdx, CHUNK_ID a2) {
    uint32_t v2;
    uint32_t v3;
    TESForm::AddChunk(a2);
    if (apThis->pIdle) {
        v3 = apThis->pIdle->TESForm::GetFormID();
    }
    else {
        v3 = 0;
    }
    TESForm::AddChunk(INAM_ID, v3);
    if (apThis->pTopic) {
        v2 = apThis->pTopic->TESForm::GetFormID();
    }
    else {
        v2 = 0;
    }
    if (apThis->pResultScript) {
        apThis->pResultScript->SaveResultScript();
    }
    TESForm::AddChunk(TNAM_ID, v2);
}
