#include "Fallout Shared/TESConditionFunctions.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/BGSVoiceType.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/SpecificItemCollector.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESCasinoChips.h"
#include "Fallout Shared/TESChallenge.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestStage.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESReputation.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/GuardPackage.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/Procedure.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/SandBoxPackage.h"
#include "Fallout/AI/TrespassPackage.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/BlackJackMenu.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/ConcussionEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cstdint>
#include <mutex>

// Conditions are also tested on the AI threads and the loader: the lock keeps pInCellRef, pInCell and fInCell one
// entry, so a test never takes another pair's answer.
static std::mutex kInCellLock;

uint32_t RandomUnsignedInt(uint32_t aiMax) {
    BSRandom* Singleton = BSRandom::GetSingleton();
    return Singleton->UnsignedInt(aiMax);
}

bool Cmd_GetPCIsClass(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    return ::Cmd_GetIsClass(pPlayer, apParam1, apParam2, afReturnValue);
}

bool Cmd_GetPCIsRace(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    return Script::GetIsRaceConditionFunction(pPlayer, apParam1, apParam2, afReturnValue);
}

bool Cmd_GetPCIsSex(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    return Script::GetIsSexConditionFunction(pPlayer, apParam1, apParam2, afReturnValue);
}

bool Cmd_GetPCInFaction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    return Script::GetInFactionConditionFunction(pPlayer, apParam1, apParam2, afReturnValue);
}

bool Cmd_GetInWorldspace(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    TESWorldSpace* v6 = nullptr;
    if ((apParam1) && (static_cast<TESForm*>(apParam1)->GetFormType() == 65)) {
        v6 = static_cast<TESWorldSpace*>(apParam1);
    }

    TESObjectREFR* v7 = nullptr;
    if (apParam2) {
        int32_t FormType = static_cast<TESObjectREFR*>(apParam2)->GetFormType();
        if (FormType >= 58 && (FormType <= 64 || FormType == 105)) {
            v7 = static_cast<TESObjectREFR*>(apParam2);
        }
    }
    if (!v7) {
        v7 = apRefObject;
    }
    if ((v6) && (v7->GetWorldSpace() == v6)) {
        afReturnValue = 1.0;
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetInWorldspace >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_GetIsClass(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    TESNPC* ObjectReference = nullptr;
    if ((apRefObject) && (apRefObject->GetObjectReference()->GetFormType() == NPC__ID)) {
        ObjectReference = static_cast<TESNPC*>(apRefObject->GetObjectReference());
    }

    TESClass* v6 = nullptr;
    if ((apParam1) && (static_cast<TESForm*>(apParam1)->GetFormType() == CLAS_ID)) {
        v6 = static_cast<TESClass*>(apParam1);
    }

    if ((ObjectReference) && (v6) && (ObjectReference->GetClass() == v6)) {
        afReturnValue = 1.0;
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetIsClass >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_GetIsCreatureType(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    TESCreature* v6 = nullptr;
    if (apRefObject) {
        TESBoundObject* ObjectReference = apRefObject->GetObjectReference();
        if (ObjectReference->GetFormType() == 43) {
            v6 = static_cast<TESCreature*>(apRefObject->GetObjectReference());
        }
    }
    if ((v6) && (v6->GetCreatureType() == reinterpret_cast<intptr_t>(apParam1))) {
        afReturnValue = 1.0;
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetIsCreatureType >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_GetQuestVariable_Eval(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    TESForm* pForm = static_cast<TESForm*>(apParam1);
    if (pForm) {
        ScriptLocals* pLocals = nullptr;
        switch (pForm->GetFormType()) {
            case REFR_ID:
            case ACHR_ID:
            case ACRE_ID:
            case PMIS_ID:
            case PGRE_ID:
            case PBEA_ID:
            case PFLA_ID:
            case PCBE_ID: {
                TESObjectREFR* pRef = static_cast<TESObjectREFR*>(pForm);
                if (pRef->m_Extra.GetReferencePointer()) {
                    TESObjectREFR* pContainer = pRef->m_Extra.GetReferencePointer();
                    uint32_t uiFormID = pRef->iFormID;
                    ItemChange* pItem = pContainer->GetInventoryItem(pRef->GetObjectReference(), uiFormID);
                    if (pItem) {
                        pLocals = pItem->GetScriptLocals();
                        delete pItem;
                    }
                }
                else {
                    pLocals = pRef->GetScriptVariables();
                }
                break;
            }
            case QUST_ID: {
                pLocals = static_cast<TESQuest*>(pForm)->pScriptLocals;
                break;
            }
            default: {
                break;
            }
        }
        if ((pLocals) && (pLocals->m_pMasterScript)) {
            // The script passes the variable's index as an integer in the parameter slot.
            afReturnValue = pLocals->GetVariable(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apParam2)), nullptr);
        }
    }
    return true;
}

bool Cmd_GetQuestCompleted(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    TESQuest* pQuest = static_cast<TESQuest*>(apParam1);
    if (pQuest && pQuest->GetCompleted()) {
        afReturnValue = 1.0;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetQuestCompleted >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_GetPlayerControlsDisabled(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if (pPlayer->GetControlsDisabled(
            static_cast<PlayerCharacter::DISABLE_PLAYER_CONTROLS>(reinterpret_cast<intptr_t>(apParam1))
        )) {
        afReturnValue = 1.0;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetPlayerControlsDisabled: %0.2f", afReturnValue);
        char PCControlFlags = pPlayer->GetPCControlFlags();
        for (int32_t i = 0; i < 7; ++i) {
            if ((PCControlFlags & 1) != 0) {
                Interface::PrintLine("     %s - Disabled", pPlayerControls[i]);
            }
            else {
                Interface::PrintLine("     %s - Enabled", pPlayerControls[i]);
            }
            PCControlFlags >>= 1;
        }
    }
    return true;
}

bool Cmd_GetPCExpelled(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if ((apParam1) && (static_cast<TESFaction*>(apParam1)->GetPCExpelled())) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        if (0.0 == afReturnValue) {
            Interface::PrintLine("PC is not expelled.");
        }
        else {
            Interface::PrintLine("PC is expelled.");
        }
    }
    return true;
}

bool Cmd_GetPCFactionMurder(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if ((apParam1) && (static_cast<TESFaction*>(apParam1)->GetPCHasMurdereredMember())) {
        afReturnValue = 1.0;
    }

    if (GetThreadTLSData()->bSystemOutput) {
        if (0.0 == afReturnValue) {
            Interface::PrintLine("PC did not Murder a faction member.");
        }
        else {
            Interface::PrintLine("PC murdered a faction member.");
        }
    }
    return true;
}

bool Cmd_IsInCriticalStage(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    // The script passes the stage as an integer in the parameter slot.
    ACTOR_CRITICAL_STAGE eStage = static_cast<ACTOR_CRITICAL_STAGE>(reinterpret_cast<uintptr_t>(apParam1));
    if (apRefObject->IsActor() && static_cast<Actor*>(apRefObject)->eCriticalStage == eStage) {
        afReturnValue = 1.0;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v5;
        if (1.0 == afReturnValue) {
            v5 = "IS";
        }
        else {
            v5 = "IS NOT";
        }
        Interface::PrintLine("Actor %s in stage %s.", v5, pCriticalStageNames[eStage]);
    }
    return true;
}

bool Cmd_IsOwner_Eval(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    TESForm* apObj = static_cast<TESForm*>(apParam1);
    if (!apParam1) {
        apObj = pPlayer->GetObjectReference();
    }
    if ((apRefObject) && (apObj)) {
        ExtraDataList* Extra = apRefObject->GetExtra();
        if (Extra->GetOwner() == apObj) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        if (0.0 == afReturnValue) {
            const char* FullName = TESFullName::GetFullName(apObj);
            Interface::PrintLine("%s is not the owner", FullName);
        }
        else {
            const char* v5 = TESFullName::GetFullName(apObj);
            Interface::PrintLine("%s is the owner", v5);
        }
    }
    return true;
}

bool Cmd_IsCellOwner(TESObjectREFR* apRefObject, void* apParam1, TESForm* apParam2, double& afReturnValue) {
    if (static_cast<TESObjectCELL*>(apParam1)->GetOwner() == apParam2) {
        afReturnValue = 1.0;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        if (0.0 == afReturnValue) {
            Interface::PrintLine("%s is not the owner", TESFullName::GetFullName(apParam2));
        }
        else {
            Interface::PrintLine("%s is the owner", TESFullName::GetFullName(apParam2));
        }
    }
    return true;
}

bool Cmd_GetPCMiscStat(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue =
        (double)MiscStatManager::GetVal(static_cast<MiscStatManager::MiscStatID>(reinterpret_cast<intptr_t>(apParam1)));
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Player misc stat value %.02f", afReturnValue);
    }
    return true;
}

bool Cmd_Exists(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    if (apRefObject && (apRefObject != apParam1)) {
        afReturnValue = 1.0;
    }
    else {
        afReturnValue = 0.0;
    }
    return true;
}

bool Cmd_GetIsAlignment(
    TESObjectREFR* apRefObject,
    TESActorBaseData::ALIGNMENT apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        ActorValueOwner* pOwner = static_cast<Actor*>(apRefObject);
        float afKarma = pOwner->GetActorFloatValue(ActorValue::KARMA);
        if (TESActorBaseData::GetAlignmentForKarma(afKarma) == apParam1) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        const char* v6;
        if (!(afReturnValue > 0.0)) {
            v6 = "false";
        }
        else {
            v6 = "true";
        }
        Interface::PrintLine("GetIsAlignment '%s' >> %s", TESActorBaseData::pAlignmentNames[apParam1], v6);
    }
    return true;
}

bool Cmd_GetConcussed(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor()) && (get_bConcussion_11DBE50())) {
        // The MagicTarget base is used with no null test, as the game does.
        Actor* pActor = static_cast<Actor*>(apRefObject);
        if (pActor->HasArchType(EffectArchetypes::CONCUSSION, 0)) {
            afReturnValue = 1.0;
        }
        if (GetThreadTLSData()->bSystemOutput) {
            if (!(afReturnValue > 0.0)) {
                Interface::PrintLine("%s is not concussed ", apRefObject->GetFullName());
            }
            else {
                Interface::PrintLine("%s is concussed", apRefObject->GetFullName());
            }
        }
    }

    return true;
}

char get_bConcussion_11DBE50() {
    return bConcussion_11DBE50;
}

bool Cmd_GetForceHitReaction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    const char* fullName;
    double v6;
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)->GetForceHitReaction();
        if (Script::GetThreadSystemOutput()) {
            v6 = afReturnValue;
            fullName = apRefObject->GetFullName();
            Interface::PrintLine("GetForceHitReactionConditionFunction: %s >> %0.2f", fullName, v6);
        }
    }

    return true;
}

bool Cmd_GetReputation(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    const char* fullName_0;
    double v6;
    if (apParam1) {
        afReturnValue = (double)static_cast<TESReputation*>(apParam1)->GetReputationValue(
            static_cast<ReputationType>(reinterpret_cast<intptr_t>(apParam2))
        );
        if (Script::GetThreadSystemOutput()) {
            v6 = afReturnValue;
            fullName_0 = static_cast<TESFullName*>(static_cast<TESReputation*>(apParam1))->TESFullName::GetFullName();
            Interface::PrintLine("GetReputation: %s >> %0.2f", fullName_0, v6);
        }
    }
    return true;
}

bool Cmd_GetReputationPct(TESObjectREFR* apRefObject, void* apParam1, ReputationType apParam2, double& afReturnValue) {
    TESReputation* pReputation = static_cast<TESReputation*>(apParam1);
    if (pReputation) {
        afReturnValue = pReputation->GetReputationPct(apParam2);
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "GetReputation: %s >> %0.2f", static_cast<TESFullName*>(pReputation)->TESFullName::GetFullName(),
                afReturnValue
            );
        }
    }
    return true;
}

bool Cmd_GetReputationThreshold(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    const char* fullName_0;
    double v6;
    if (apParam1) {
        afReturnValue = (double)static_cast<TESReputation*>(apParam1)->GetReputationThreshold(
            static_cast<char>(reinterpret_cast<intptr_t>(apParam2))
        );
        if (Script::GetThreadSystemOutput()) {
            v6 = afReturnValue;
            fullName_0 = static_cast<TESFullName*>(static_cast<TESReputation*>(apParam1))->TESFullName::GetFullName();
            Interface::PrintLine("GetReputation: %s >> %0.2f", fullName_0, v6);
        }
    }
    return true;
}

bool Cmd_GetObjectiveCompleted_Eval(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    double v5;
    BGSQuestObjective* objective;
    if (apParam1) {
        objective = static_cast<TESQuest*>(apParam1)->GetObjective(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apParam2))
        );
        if (objective) {
            if (objective->GetCompleted()) {
                v5 = 1.0;
            }
            else {
                v5 = 0.0;
            }
            afReturnValue = v5;
            if (Script::GetThreadSystemOutput()) {
                Interface::PrintLine("GetObjectiveCompleted >> %0.2f", afReturnValue);
            }
        }
    }
    return true;
}

bool Cmd_PlayerInRegion(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    BSSimpleList<TESRegion*>* v4;
    uint32_t v5;
    TESRegionList* v7;
    BSSimpleList<TESRegion*>* v8;
    TESRegionList* i;
    afReturnValue = 0.0;
    for (i = pPlayer->GetAllOccupiedRegions(); i && static_cast<BSSimpleList<TESRegion*>*>(i)->m_item; i = v7) {
        v4 = static_cast<BSSimpleList<TESRegion*>*>(i);
        v5 = v4->m_item->TESForm::GetFormID();
        bool vCondition1;
        vCondition1 = v5 == static_cast<TESForm*>(apParam1)->TESForm::GetFormID();
        if (vCondition1) {
            afReturnValue = 1.0;
            return true;
        }
        v8 = static_cast<BSSimpleList<TESRegion*>*>(i)->m_pkNext;
        if (v8) {
            v7 = static_cast<TESRegionList*>(v8);
        }
        else {
            v7 = nullptr;
        }
    }
    return true;
}

void CFilter::SetGroup(uint32_t aiGroup) {
    iFilter = static_cast<uint16_t>(iFilter);
    iFilter |= aiGroup << 16;
}

const char* BGSBodyPart::GetTargetName() {
    return TargetName.String();
}

// The quest script's GetSecondsPassed buffer.
float Script::GetQuestScriptGetSecondsBuffer() {
    return fQuestScriptGetSecondsBuffer;
}

// The condition passes the actor value index in its first void* parameter (an integer the caller stores in the
// pointer, not an address). A disabled actor answers with its base form's permanent value.
bool Script::GetPermanentActorValueConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Actor* pActor = nullptr;
    if (apRefObject != nullptr && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }
    if (pActor != nullptr) {
        double fValue;
        if (pActor->GetDisabled()) {
            ActorValueOwner* pOwner = pActor->GetActorBase();
            fValue = pOwner->GetPermanentActorFloatValue(
                static_cast<ActorValue::Index>(reinterpret_cast<intptr_t>(apParam1))
            );
        }
        else {
            fValue = pActor->GetPermanentActorFloatValue(
                static_cast<ActorValue::Index>(reinterpret_cast<intptr_t>(apParam1))
            );
        }
        afReturnValue = fValue;
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine(
                "GetActorValue: %s >> %0.2f",
                ActorValue::GetActorValueScriptName(
                    static_cast<ActorValue::Index>(reinterpret_cast<intptr_t>(apParam1))
                ),
                afReturnValue
            );
        }
    }
    return true;
}

// The GetThreatRatio condition: the actor's combat strength over the target's, both asked for at -1 damage per second.
// The ratio is a double division of the two floats.
bool Script::GetThreatRatioConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject != nullptr && apRefObject->IsActor() && apParam1 != nullptr) {
        float fActorStrength = static_cast<Actor*>(apRefObject)->CalculateCombatStrength(-1.0f);
        float fTargetStrength = static_cast<Actor*>(apParam1)->CalculateCombatStrength(-1.0f);
        afReturnValue = static_cast<double>(fActorStrength) / static_cast<double>(fTargetStrength);
        if (GetThreadSystemOutput()) {
            const char* pTargetName = static_cast<Actor*>(apParam1)->GetFormEditorID();
            const char* pActorName = apRefObject->GetFormEditorID();
            Interface::PrintLine("GetThreatRatio: '%s' to '%s' >> %.2f", pActorName, pTargetName, afReturnValue);
        }
    }
    else if (GetThreadSystemOutput()) {
        Interface::PrintLine<>("GetThreatRatio >> FAILED");
    }
    return true;
}

bool Script::GetWeaponAnimTypeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    if ((pactor) && (pactor->pCurrentProcess)) {
        BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
        ItemChange* pweapdata = CurrentAIProcess->GetCurrentWeapon();
        TESObjectWEAP* pweap = nullptr;
        if ((pweapdata) && (pweapdata->pContainerObj) && (pweapdata->pContainerObj->GetFormType() == WEAP_ID)) {
            pweap = static_cast<TESObjectWEAP*>(pweapdata->pContainerObj);
        }

        if (pweap) {
            // The table's entry is converted as a signed 32-bit integer.
            afReturnValue = static_cast<double>(static_cast<int32_t>(weaponToAnimGroupWeapon[pweap->GetWeaponType()]));
        }
        else {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Get Weapon Anim >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetCurrentAIProcedureConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    if ((pactor) && (pactor->pCurrentProcess) && (pactor->GetCurrentPackage())) {
        CombatController* pcombatController = static_cast<CombatController*>(pactor->GetCurrentPackage());
        BaseProcess* pproc = pactor->pCurrentProcess;
        uint32_t Count = pcombatController->ePROCEDURE_TYPE;
        PROCEDURE v9 = Procedure_Action[Count][pproc->GetProcedureIndexRunning()];
        if (pcombatController->GetPackType() == PACKAGE_TYPE_COMBAT && pcombatController->IsFleeing()) {
            v9 = PROCEDURE_FLEE;
        }
        switch (v9) {
            case PROCEDURE_TRAVEL: {
                afReturnValue = 0.0;
                break;
            }
            case PROCEDURE_WANDER: {
                afReturnValue = 7.0;
                break;
            }
            case PROCEDURE_ACTIVATE: {
                afReturnValue = 1.0;
                break;
            }
            case PROCEDURE_ACQUIRE: {
                afReturnValue = 2.0;
                break;
            }
            case PROCEDURE_SLEEP: {
                afReturnValue = 8.0;
                break;
            }
            case PROCEDURE_EAT: {
                afReturnValue = 10.0;
                break;
            }
            case PROCEDURE_FOLLOW: {
                afReturnValue = 11.0;
                break;
            }
            case PROCEDURE_ESCORT: {
                afReturnValue = 12.0;
                break;
            }
            case PROCEDURE_ALARM: {
                afReturnValue = 14.0;
                break;
            }
            case PROCEDURE_COMBAT: {
                afReturnValue = 13.0;
                break;
            }
            case PROCEDURE_FLEE: {
                afReturnValue = 16.0;
                break;
            }
            case PROCEDURE_YIELD: {
                afReturnValue = 18.0;
                break;
            }
            case PROCEDURE_DIALOGUE: {
                afReturnValue = 4.0;
                break;
            }
            case PROCEDURE_WAIT: {
                afReturnValue = 3.0;
                break;
            }
            case PROCEDURE_TRAVEL_TARGET: {
                afReturnValue = 19.0;
                break;
            }
            case PROCEDURE_PURSUE: {
                afReturnValue = 15.0;
                break;
            }
            case PROCEDURE_GREET: {
                afReturnValue = 5.0;
                break;
            }
            case PROCEDURE_CREATE_FOLLOW: {
                afReturnValue = 20.0;
                break;
            }
            case PROCEDURE_OBSERVE_COMBAT: {
                afReturnValue = 9.0;
                break;
            }
            case PROCEDURE_GREET_DEAD: {
                afReturnValue = 6.0;
                break;
            }
            case PROCEDURE_GET_UP: {
                afReturnValue = 21.0;
                break;
            }
            case PROCEDURE_MOUNT_HORSE: {
                afReturnValue = 22.0;
                break;
            }
            case PROCEDURE_DISMOUNT_HORSE: {
                afReturnValue = 23.0;
                break;
            }
            case PROCEDURE_DO_NOTHING: {
                afReturnValue = 24.0;
                break;
            }
            case PROCEDURE_ACCOMPANY: {
                afReturnValue = 27.0;
                break;
            }
            case PROCEDURE_USE_ITEM_AT: {
                afReturnValue = 28.0;
                break;
            }
            case PROCEDURE_FEED: {
                afReturnValue = 29.0;
                break;
            }
            case PROCEDURE_AMBUSH_WAIT: {
                afReturnValue = 30.0;
                break;
            }
            case PROCEDURE_SURFACE: {
                afReturnValue = 31.0;
                break;
            }
            case PROCEDURE_WAIT_FOR_SPELL: {
                afReturnValue = 32.0;
                break;
            }
            case PROCEDURE_CHOOSE_CAST: {
                afReturnValue = 33.0;
                break;
            }
            case PROCEDURE_FLEE_NON_COMBAT: {
                afReturnValue = 34.0;
                break;
            }
            case PROCEDURE_REMOVE_WORN_ITEMS: {
                afReturnValue = 35.0;
                break;
            }
            case PROCEDURE_SEARCH: {
                afReturnValue = 36.0;
                break;
            }
            case PROCEDURE_CLEAR_MOUNT_POSITION: {
                afReturnValue = 37.0;
                break;
            }
            case PROCEDURE_SUMMON_CREATURE_DEFEND: {
                afReturnValue = 38.0;
                break;
            }
            case PROCEDURE_AVOID_RADIATION: {
                afReturnValue = 39.0;
                break;
            }
            case PROCEDURE_UNEQUIP_ARMOR: {
                afReturnValue = 40.0;
                break;
            }
            case PROCEDURE_TAKE_BACK_ITEM: {
                afReturnValue = 47.0;
                break;
            }
            case PROCEDURE_SANDBOX: {
                SandBoxActorPackageData* psandData =
                    static_cast<SandBoxActorPackageData*>(pproc->GetInstanceDataThatIsRunning());
                afReturnValue = 45.0;
                if (psandData) {
                    uint32_t esandState = psandData->eState;
                    if (esandState == 3) {
                        uint32_t uiproc = psandData->eProcedure + 1;
                        if (uiproc < 7) {
                            afReturnValue = rdata_10357C8[uiproc];
                        }
                    }
                    else {
                        if (esandState < 4) {
                            afReturnValue = rdata_10357B8[esandState];
                        }
                    }
                }
                break;
            }
            case PROCEDURE_USE_IDLE_MARKER: {
                afReturnValue = 46.0;
                break;
            }
            case PROCEDURE_PATROL: {
                afReturnValue = 41.0;
                break;
            }
            case PROCEDURE_GUARD: {
                afReturnValue = 44.0;
                break;
            }
            case PROCEDURE_DIALOGUE_ACTIVATE: {
                afReturnValue = 43.0;
                break;
            }
            case PROCEDURE_USE_WEAPON: {
                afReturnValue = 42.0;
                break;
            }
            case PROCEDURE_MOVEMENT_BLOCKED: {
                afReturnValue = 49.0;
                break;
            }
            case PROCEDURE_CANNIBAL_FEED: {
                afReturnValue = 50.0;
                break;
            }
            case PROCEDURE_DONE: {
                afReturnValue = 17.0;
                break;
            }
            default: {
                afReturnValue = -1.0;
                break;
            }
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Procedure >> %0.2f", afReturnValue);
    }
    return true;
}

// The GetActorCrimePlayerEnemy condition: an actor whose player crime list is not empty is the player's enemy. The
// extra and the list are fetched twice, as the game does; a null reference or a non-actor answers without printing.
bool Script::GetActorCrimePlayerEnemyConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject == nullptr || !apRefObject->IsActor()) {
        return true;
    }
    TESObjectREFR* pRef = apRefObject;
    if (pRef->GetExtra()->GetPlayerCrimeList() != nullptr) {
        BSSimpleList<Crime*>* pCrimeList = pRef->GetExtra()->GetPlayerCrimeList();
        if (!pCrimeList->IsEmpty()) {
            afReturnValue = 1.0;
        }
    }
    if (GetThreadSystemOutput()) {
        if (afReturnValue == 0.0) {
            Interface::PrintLine("This actor is not an enemy of player because of crime");
        }
        else {
            Interface::PrintLine("This actor is an enemy of player because of crime");
        }
    }
    return true;
}

bool Script::GetLOSConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        Actor* pAttacker = nullptr;
        if (apRefObject->IsActor()) {
            pAttacker = static_cast<Actor*>(apRefObject);
        }
        TESObjectREFR* pTarget = static_cast<TESObjectREFR*>(apParam1);
        bool bInSight = false;
        if ((pAttacker) && (pTarget)) {
            if (pAttacker == pPlayer) {
                bool bInFrustum = false;
                SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
                NiNode* pcamnode = WorldSceneGraph->GetCameraRoot();
                NiCamera* pcam = Main::GetWorldSceneGraph()->GetCamera();
                BSFadeNode* pfadenode;
                if (pTarget->Get3D()) {
                    pfadenode = pTarget->Get3D()->IsFadeNode();
                }
                else {
                    pfadenode = nullptr;
                }
                if (pfadenode && NodeInFrustum(pfadenode, pcam)) {
                    bInFrustum = true;
                }
                else {
                    if (pTarget->IsMarker()) {
                        NiBound kBound;
                        kBound.SetCenter(pTarget->GetLocationOnReference());
                        kBound.SetRadius(1.0f);
                        if (BoundInFrustum(kBound, pcam)) {
                            bInFrustum = true;
                        }
                    }
                }
                if (bInFrustum) {
                    float fzs[3];
                    alignas(16) CFilter icolFilter;
                    alignas(16) NiPoint3 position = pcamnode->GetTranslate();
                    bhkPickData pickRes;
                    MobileObject* pmob = nullptr;
                    if ((pTarget) && (pTarget->IsMobileObject())) {
                        pmob = static_cast<MobileObject*>(pTarget);
                    }

                    SpecificItemCollector collector(L_LOS, pmob);
                    pickRes.SetClosestCollector(&collector);
                    NiPoint3 destPoint = pTarget->GetLocationOnReference();
                    pickRes.NiSetFrom(position);
                    pPlayer->GetCollisionFilter(&icolFilter.iFilter);
                    CFilter fltr(0);
                    fltr.SetLayer(L_LOS);
                    uint32_t Group = icolFilter.GetGroup();
                    fltr.SetGroup(Group);
                    pickRes.SetFilter(fltr);
                    alignas(16) NiPoint3 kBoundMax = pTarget->GetBoundMax();
                    NiPoint3 kBoundMin = pTarget->GetBoundMin();
                    float fTargetHeight = kBoundMax.z - kBoundMin.z;
                    fzs[0] = fTargetHeight * 0.75 + destPoint.z;
                    fzs[1] = fTargetHeight * 0.5 + destPoint.z;
                    fzs[2] = fTargetHeight * 0.25 + destPoint.z;
                    for (int32_t i = 0; i < 3; ++i) {
                        TESObjectREFR* phitref;
                        destPoint.z = fzs[i];
                        pickRes.NiSetTo(destPoint);
                        NiAVObject* phit = TES::pInstance->Pick(pickRes);
                        if (phit) {
                            phitref = TESObjectREFR::FindReferenceFor3D(phit);
                        }
                        else {
                            phitref = nullptr;
                        }
                        if (!phit || phitref == pTarget) {
                            bInSight = true;
                            break;
                        }
                    }
                    if (bInSight) {
                        afReturnValue = 1.0;
                    }
                    else {
                        afReturnValue =
                            static_cast<double>(pAttacker->LineOfSight(false, pTarget, true, nullptr, false));
                    }
                }
            }
            else {
                afReturnValue = static_cast<double>(pAttacker->LineOfSight(false, pTarget, true, nullptr, false));
            }
            if (GetThreadTLSData()->bSystemOutput) {
                if (0.0 == afReturnValue) {
                    const char* a2a = pTarget->GetFullName();
                    Interface::PrintLine("%s can't see %s", pAttacker->GetFullName(), a2a);
                }
                else {
                    const char* a2 = pTarget->GetFullName();
                    Interface::PrintLine("%s sees %s", pAttacker->GetFullName(), a2);
                }
            }
        }
    }
    return true;
}

bool Script::GetDisabledConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    // The parameter is the calling script's locals (Cmd_GetDisabled_Execute passes its event list there).
    ScriptLocals* pScriptLocals = static_cast<ScriptLocals*>(apParam1);
    if (apRefObject) {
        afReturnValue = 0.0;
        if ((apRefObject->GetDisabled() && !Script::IsPendingEnabledReference(apRefObject)) ||
            Script::IsPendingDisabledReference(apRefObject)) {
            afReturnValue = 1.0;
        }
    }
    else {
        if (pScriptLocals) {
            afReturnValue = static_cast<double>((pScriptLocals->m_cFlags & 1) != 0);
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetDisabled >> %0.f", afReturnValue);
    }
    return true;
}

bool Script::GetLockedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        LockData* Lock = apRefObject->GetLock();
        if (Lock) {
            if (Lock->IsBroken()) {
                afReturnValue = 2.0;
            }
            else {
                if (Lock->IsLocked()) {
                    afReturnValue = 1.0;
                }
            }
        }
        else {
            TESBoundObject* ObjectReference = apRefObject->GetObjectReference();
            if (ObjectReference->GetFormType() == 23) {
                if (pPlayer->GetTerminalAccess(apRefObject) == PlayerCharacter::TAT_LOCKED) {
                    afReturnValue = 2.0;
                }
                else {
                    BGSTerminal* v6 = static_cast<BGSTerminal*>(apRefObject->GetObjectReference());
                    if (!v6->IsUnlocked(apRefObject)) {
                        afReturnValue = 1.0;
                    }
                }
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetLocked >> %0.f", afReturnValue);
    }
    return true;
}

bool Script::GetLockLevelConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        LockData* Lock = apRefObject->GetLock();
        if (Lock) {
            afReturnValue = static_cast<double>(Lock->GetLevel(apRefObject));
        }
        else {
            TESBoundObject* ObjectReference = apRefObject->GetObjectReference();
            if (ObjectReference->GetFormType() == 23) {
                BGSTerminal* v6 = static_cast<BGSTerminal*>(apRefObject->GetObjectReference());
                if (v6->IsUnlocked(apRefObject)) {
                    afReturnValue = -1.0;
                }
                else {
                    afReturnValue = static_cast<double>(v6->GetHackDifficultyLockLevel(apRefObject));
                }
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetLockLevel >> %0.f", afReturnValue);
    }
    return true;
}

bool Script::GetVampireConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        apRefObject->IsActor();
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetVampire >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetClothingValueConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Character* v6 = nullptr;
    if (apRefObject) {
        TESBoundObject* ObjectReference = apRefObject->GetObjectReference();
        if (ObjectReference->GetFormType() == 42) {
            v6 = static_cast<Character*>(apRefObject);
        }
    }
    if (v6) {
        afReturnValue = v6->GetClothingValue();
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetClothingValue >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetSecondsPassedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    // The parameter is the running script.
    Script* pscript = static_cast<Script*>(apParam1);
    if (pscript && pscript->GetHeader()->bIsQuestScript && pscript->GetQuestScriptGetSecondsBuffer() > 0.0) {
        afReturnValue = pscript->GetQuestScriptGetSecondsBuffer();
    }
    else {
        afReturnValue = appTimer.GetSeconds();
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetSecondsPassed >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsRaceConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESNPC* ObjectReference = nullptr;
    if ((apRefObject) && (apRefObject->GetObjectReference()->GetFormType() == NPC__ID)) {
        ObjectReference = static_cast<TESNPC*>(apRefObject->GetObjectReference());
    }

    TESRace* v6 = nullptr;
    if ((apParam1) && (static_cast<TESForm*>(apParam1)->GetFormType() == RACE_ID)) {
        v6 = static_cast<TESRace*>(apParam1);
    }

    if ((ObjectReference) && (v6) && (ObjectReference->GetRace() == v6)) {
        afReturnValue = 1.0;
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetIsRace >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetInFactionConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESFaction* pfac = nullptr;
    if ((apParam1) && (static_cast<TESForm*>(apParam1)->GetFormType() == FACT_ID)) {
        pfac = static_cast<TESFaction*>(apParam1);
    }

    if (apRefObject == pInFactionRef && pfac == pFaction) {
        afReturnValue = fInFaction;
    }
    else {
        Actor* pact = nullptr;
        if ((apRefObject) && (apRefObject->IsActor())) {
            pact = static_cast<Actor*>(apRefObject);
        }

        if (!pact) {
            return true;
        }
        if (pact->IsInFaction(pfac)) {
            afReturnValue = 1.0;
        }
        pInFactionRef = (void*)apRefObject;
        pFaction = (void*)pfac;
        fInFaction = afReturnValue;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetInFaction >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetArmorRatingUpperConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    TESObjectREFR* v8 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v8 = apRefObject;
    }

    afReturnValue = 0.0;
    if (v8) {
        InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(apRefObject);
        if (InventoryChanges_) {
            ItemChange* WornItem = InventoryChanges_->GetWornItem(BO_UPPER_BODY, false);
            if (WornItem) {
                TESObjectARMO* FormFlags = nullptr;
                if ((WornItem->pContainerObj) && (WornItem->pContainerObj->GetFormType() == ARMO_ID)) {
                    FormFlags = static_cast<TESObjectARMO*>(WornItem->pContainerObj);
                }

                if (FormFlags) {
                    if (FormFlags->GetArmorWeight()) {
                        afReturnValue = 2.0;
                    }
                    else {
                        afReturnValue = 1.0;
                    }
                }
                delete WornItem;
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Armor Rating upper body is %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetDetectedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    bool bsearch;
    afReturnValue = 0.0;
    int32_t inumber = 0;
    int32_t ilight = -1;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    Actor* pdetector = nullptr;
    if ((apParam1) && (static_cast<TESObjectREFR*>(apParam1)->IsActor())) {
        pdetector = static_cast<Actor*>(apParam1);
    }

    if ((pactor) && (pdetector) && (pactor->pCurrentProcess) && (pdetector->pCurrentProcess)) {
        bool blinesight;
        bool bincombat = false;
        bsearch = false;
        if (pdetector == pPlayer) {
            bincombat = pPlayer->IsPlayerCharacterInCombat(&bsearch);
        }
        else {
            bincombat = pdetector->IsInCombat();
        }
        inumber = pactor->GetDetectionLevelAgainstActor(false, pdetector, blinesight, false, bincombat, false, nullptr);
        if (inumber > 0) {
            afReturnValue = 1.0;
        }
        BaseProcess* CurrentAIProcess = pdetector->pCurrentProcess;
        ilight = static_cast<int32_t>(CurrentAIProcess->GetActorLightLevel());
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetDetected >> %i and light %i", inumber, ilight);
    }
    return true;
}

bool Script::GetDeadConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESObjectREFR* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = apRefObject;
    }

    if ((v5) && (v5->IsDead(true))) {
        afReturnValue = 1.0;
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetDead >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetItemCountConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject->HasContainer()) {
        InventoryChanges* pchange;
        TESBoundObject* pobject = nullptr;
        if ((apParam1) && (static_cast<TESForm*>(apParam1)->IsBoundObject())) {
            pobject = static_cast<TESBoundObject*>(apParam1);
        }

        if (apRefObject) {
            pchange = InventoryChanges::GetInventoryChanges(apRefObject);
        }
        else {
            pchange = nullptr;
        }
        if (pchange) {
            if (pobject) {
                int32_t ObjectCount = pchange->GetObjectCount(pobject);
                afReturnValue = static_cast<double>(std::abs(ObjectCount));
            }
            else {
                if ((apParam1) && (static_cast<TESForm*>(apParam1)->GetFormType() == FLST_ID)) {
                    BGSListForm* pformlist = static_cast<BGSListForm*>(apParam1);
                    BSSimpleList<TESForm*>* plist = pformlist->GetFormList();
                    while (plist && !plist->IsEmpty()) {
                        TESForm* pcheckform = plist->m_item;
                        plist = plist->m_pkNext;
                        if (pcheckform->IsBoundObject()) {
                            int32_t v6 = pchange->GetObjectCount(static_cast<TESBoundObject*>(pcheckform));
                            afReturnValue = static_cast<double>(std::abs(v6)) + afReturnValue;
                        }
                    }
                }
            }
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("GetItemCount >> %0.2f", afReturnValue);
        }
        return true;
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Calling Reference is not a Container Object");
    }
    return true;
}

bool Script::GetEquippedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* ptarget = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        ptarget = static_cast<Actor*>(apRefObject);
    }

    TESBoundObject* pobject = static_cast<TESBoundObject*>(apParam1);
    if (ptarget && apParam1) {
        BSSimpleList<TESForm*>* plist = nullptr;
        if (static_cast<TESForm*>(apParam1)->GetFormType() == FLST_ID) {
            BGSListForm* pformlist = static_cast<BGSListForm*>(apParam1);
            plist = pformlist->GetFormList();
            pobject = static_cast<TESBoundObject*>(plist->m_item);
        }
        while (pobject) {
            if (pobject->IsBoundObject()) {
                InventoryChanges* pchange = InventoryChanges::GetInventoryChanges(ptarget);
                if ((pchange) && (pchange->WearingObject(pobject, false))) {
                    afReturnValue = 1.0;
                    break;
                }
            }
            if (plist) {
                plist = plist->m_pkNext;
            }
            if (plist) {
                pobject = static_cast<TESBoundObject*>(plist->m_item);
            }
            else {
                pobject = nullptr;
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetEquipped >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetSleepingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESObjectREFR* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = apRefObject;
    }

    if (v5) {
        switch (v5->GetSitSleepState()) {
            case SIT_SLEEP_LOAD_SLEEP_IDLE: {
                goto LABEL_9;
            }
            case SIT_SLEEP_WANT_TO_SLEEP:
            case SIT_SLEEP_WAITING_FOR_SLEEP_ANIM: {
                goto LABEL_8;
            }
            case SIT_SLEEP_IS_SLEEPING: {
                goto LABEL_7;
            }
            case SIT_SLEEP_WANT_TO_WAKE: {
                afReturnValue = afReturnValue + 1.0;
            LABEL_7:;
                afReturnValue = afReturnValue + 1.0;
            LABEL_8:;
                afReturnValue = afReturnValue + 1.0;
            LABEL_9:;
                afReturnValue = afReturnValue + 1.0;
                break;
            }
            default: {
                break;
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetSleeping >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetSittingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESObjectREFR* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = apRefObject;
    }

    if (v5) {
        switch (v5->GetSitSleepState()) {
            case SIT_SLEEP_LOAD_SIT_IDLE: {
                goto LABEL_9;
            }
            case SIT_SLEEP_WANT_TO_SIT:
            case SIT_SLEEP_WAITING_FOR_SIT_ANIM: {
                goto LABEL_8;
            }
            case SIT_SLEEP_IS_SITTING: {
                goto LABEL_7;
            }
            case SIT_SLEEP_WANT_TO_STAND: {
                afReturnValue = afReturnValue + 1.0;
            LABEL_7:;
                afReturnValue = afReturnValue + 1.0;
            LABEL_8:;
                afReturnValue = afReturnValue + 1.0;
            LABEL_9:;
                afReturnValue = afReturnValue + 1.0;
                break;
            }
            default: {
                break;
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetSitting >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetQuestRunningConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESQuest* pquest = static_cast<TESQuest*>(apParam1);
    if (pquest && pquest->GetEnabled()) {
        afReturnValue = 1.0;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetQuestRunning >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsCreatureConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        TESBoundObject* ObjectReference = apRefObject->GetObjectReference();
        if (ObjectReference->GetFormType() == 43) {
            afReturnValue = 1.0;
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetIsCreature >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsPlayableRaceConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (pIsPlayableRaceRef == apRefObject) {
        afReturnValue = fIsPlayableRace;
    }
    else {
        afReturnValue = 0.0;
        TESNPC* pnpc = nullptr;
        if ((apRefObject) && (apRefObject->GetObjectReference()->GetFormType() == NPC__ID)) {
            pnpc = static_cast<TESNPC*>(apRefObject->GetObjectReference());
        }

        if (pnpc) {
            TESRace* prace = pnpc->GetRace();
            if ((prace) && (prace->GetPlayable())) {
                afReturnValue = 1.0;
            }
        }
        pIsPlayableRaceRef = apRefObject;
        fIsPlayableRace = afReturnValue;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetIsPlayableRace >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsInListConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    BGSListForm* pformlist = static_cast<BGSListForm*>(apParam1);
    if ((apRefObject) && (pformlist)) {
        TESBoundObject* pobj = (&apRefObject->m_Extra)->GetLevCreaOriginalBase();
        if (!pobj) {
            pobj = apRefObject->GetObjectReference();
        }
        if (pobj) {
            TESForm* item = pobj;
            if ((&pformlist->ListOfForms)->IsInList(item)) {
                afReturnValue = 1.0;
            }
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsInList >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsUsedItemConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESForm* plistform = static_cast<TESForm*>(apParam1);
    if (apParam1) {
        if (plistform->IsObject() && TESIdleManager::GetUsedItem() == plistform) {
            afReturnValue = 1.0;
        }
        else {
            if (plistform->GetFormType() == FLST_ID) {
                BGSListForm* pformlist = static_cast<BGSListForm*>(plistform);
                TESForm* pusedItem = TESIdleManager::GetUsedItem();
                if (pformlist->GetFormList()->IsInList(pusedItem)) {
                    afReturnValue = 1.0;
                }
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v6;
        if (plistform) {
            v6 = plistform->GetFormEditorID();
        }
        else {
            v6 = "UNKNOWN";
        }
        Interface::PrintLine("GetIsUsedItem '%s' >> %0.2f", v6, afReturnValue);
    }
    return true;
}

bool Script::GetRandomPercentConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = static_cast<double>(RandomUnsignedInt(100u));
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetRandomPercent >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetArmorRatingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    if (pactor) {
        afReturnValue = pactor->GetActorFloatValue(ActorValue::DAMAGE_RESISTANCE);
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Armor Rating: %0.2f", afReturnValue);
        }
    }
    return true;
}

bool Script::GetDeadCountConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apParam1) {
        afReturnValue = static_cast<double>(TES::pInstance->GetDeadCount(static_cast<TESActorBase*>(apParam1)));
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Dead Count: %0.2f", afReturnValue);
        }
    }
    return true;
}

bool Script::GetKnockedStateConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    if (pactor) {
        MiddleHighProcess* pprocess = dynamic_cast<MiddleHighProcess*>(pactor->pCurrentProcess);
        if (pprocess) {
            switch (pprocess->GetKnockState()) {
                case KNOCK_STATE_NORMAL:
                case KNOCK_STATE_QUEUED:
                case KNOCK_STATE_GET_UP: {
                    afReturnValue = 0.0;
                    break;
                }
                case KNOCK_STATE_OUT:
                case KNOCK_STATE_OUT_LEAD_IN: {
                    afReturnValue = 1.0;
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Get Knocked State >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsGuardConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        TESNPC* pNPC = nullptr;
        if (apRefObject->GetObjectReference()->GetFormType() == NPC__ID) {
            pNPC = static_cast<TESNPC*>(apRefObject->GetObjectReference());
        }
        if ((pNPC) && (pNPC->GetClass())) {
            TESClass* Class = pNPC->GetClass();
            if (Class->IsGuard()) {
                afReturnValue = 1.0;
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Is Guard >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsTalkingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* v6 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v6 = static_cast<Actor*>(apRefObject);
    }

    if (v6) {
        if (v6->GetIsTalking()) {
            afReturnValue = 1.0;
        }
    }
    else {
        Actor* MobileObjectExtra = static_cast<Actor*>(apRefObject->GetMobileObjectExtra());
        if ((MobileObjectExtra) && (MobileObjectExtra->IsTalking())) {
            afReturnValue = 1.0;
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Talking >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetTrespassWarningLevelConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = -1.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    if (pactor) {
        TESPackage* pbasepack = pactor->GetCurrentPackage();
        if (pbasepack) {
            PACKAGE_TYPE PackType = pbasepack->GetPackType();
            if (PackType == PACKAGE_TYPE_GUARD) {
                BaseProcess* pproc = pactor->pCurrentProcess;
                if (pproc) {
                    ActorPackageData* pdata = pproc->GetInstanceDataThatIsRunning();
                    if (pdata) {
                        // A guard package's running data is a GuardActorPackageData.
                        afReturnValue = static_cast<double>(static_cast<GuardActorPackageData*>(pdata)->iWarningCount);
                    }
                }
            }
            else {
                if (PackType == PACKAGE_TYPE_TRESPASS) {
                    // A trespass package is a TrespassPackage; its warning count is converted as a signed 32-bit value.
                    TrespassPackage* ptrespass = static_cast<TrespassPackage*>(pbasepack);
                    afReturnValue = static_cast<double>(ptrespass->iwarning);
                }
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Procedure >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsTrespassingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    if ((pactor) && (pactor->IsTresPassing())) {
        afReturnValue = 1.0;
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Is Trespassing Value %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetGroupMemberCountConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        CombatGroup* pgroup = static_cast<Actor*>(apRefObject)->GetCombatGroup();
        if (pgroup) {
            afReturnValue = static_cast<double>(pgroup->GetMemberCount());
        }
    }

    return true;
}

// The CanHaveFlames condition: the reference's 3D carries a BSXFlags extra whose addon bit is set. The extra is looked
// up under BSXFlags::Tag; the answer passes through a float, as the game does.
bool Script::CanHaveFlamesConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject != nullptr) {
        const NiFixedString* pTag = &BSXFlags::Tag();
        NiNode* pNode = apRefObject->Get3D();
        NiExtraData* pExtra = pNode->GetExtraData(*pTag);
        if (pExtra != nullptr) {
            NiObject* pObject = NiObject::DynamicCastStatic(&BSXFlags::ms_RTTI, pExtra);
            BSXFlags* pFlags = static_cast<BSXFlags*>(pObject);
            float fResult;
            if (pFlags->QAddon() != 0) {
                fResult = 1.0f;
            }
            else {
                fResult = 0.0f;
            }
            afReturnValue = fResult;
        }
    }
    if (GetThreadSystemOutput()) {
        Interface::PrintLine("CanHaveFlames >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetDayOfWeekConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    afReturnValue = appCalendar.GetDayOfTheWeek();
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetDayOfWeek >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetDetectionLevelConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        afReturnValue = 0.0;
        Actor* pActor = nullptr;
        if (apRefObject->IsActor()) {
            pActor = static_cast<Actor*>(apRefObject);
        }
        if ((pActor) && (apParam1)) {
            afReturnValue = static_cast<double>(pActor->GetDetectionLevelValue(static_cast<Actor*>(apParam1)));
            if (GetThreadTLSData()->bSystemOutput) {
                double v6 = afReturnValue;
                const char* FullName = static_cast<TESObjectREFR*>(apParam1)->GetFullName();
                Interface::PrintLine("%s detects %s at level %.02f", pActor->GetFullName(), FullName, v6);
            }
        }
    }
    return true;
}

bool Script::GetVATSTargetHeightConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (pVATS.eMode == VATS::VATS_PLAYBACK) {
        VATS_COMMAND* pcommand = pVATS.GetCurrentAction();
        if ((pcommand) && (ActionPoints::IsAttackAction(pcommand->eAction))) {
            Actor* pactor = static_cast<Actor*>(pcommand->pTarget);
            ActorValue::Index etargetLimb = pcommand->eLimb;
            if (pactor) {
                NiPoint3 ecx0;
                NiNode* ptarget3d = pactor->Get3D();
                char v16 = 0;
                if (etargetLimb != ActorValue::VALUE_NONE && pactor->IsActor()) {
                    TESActorBase* ActorBase = pactor->GetActorBase();
                    BGSBodyPartData* pbodyPartData = ActorBase->GetBodyPartData();
                    if (pbodyPartData) {
                        BGSBodyPart* pbodyPart = pbodyPartData->GetBodyPart_005E5130(etargetLimb);
                        if (pbodyPart) {
                            const char* TargetName = pbodyPart->GetTargetName();
                            NiNode* v4 = pactor->Get3D();
                            NiAVObject* ptarget = BSUtilities::GetObjectByName(v4, TargetName);
                            if (ptarget) {
                                ecx0 = ptarget->GetWorldTranslate();
                                v16 = 1;
                            }
                        }
                    }
                }
                if (!v16) {
                    const NiPoint3* Center;
                    if (ptarget3d) {
                        Center = &(&ptarget3d->GetWorldBound())->m_kCenter;
                    }
                    else {
                        Center = &pactor->GetLocationOnReference();
                    }
                    ecx0 = *Center;
                }
                const NiPoint3* v6 = &pPlayer->GetLocationOnReference();
                afReturnValue = ecx0.z - v6->z;
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("VATS Target Height >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetCannibalConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject == pPlayer) {
        afReturnValue = static_cast<double>(pPlayer->GetCannibal());
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Player is in Cannibal action >> %0.2f", afReturnValue);
    }
    return true;
}

// The IsTimePassing condition: one while the player sleeps or rests. The result is written only on that path, so
// the caller's own value stays otherwise; a zero answer prints the "not passing" line.
bool Script::IsTimePassingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (pPlayer->IsSleepingorResting()) {
        afReturnValue = 1.0;
    }
    if (GetThreadSystemOutput()) {
        if (afReturnValue == 0.0) {
            Interface::PrintLine("Time is not passing");
        }
        else {
            Interface::PrintLine("Time is passing");
        }
    }
    return true;
}

bool Script::IsActorEvilConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Actor* v7 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v7 = static_cast<Actor*>(apRefObject);
    }

    afReturnValue = 0.0;
    if (v7) {
        if (v7->IsEvil()) {
            afReturnValue = 1.0;
        }
        if (!GetThreadTLSData()->bSystemOutput || 0.0 == afReturnValue) {
            if (GetThreadTLSData()->bSystemOutput) {
                const char* FullName = v7->GetFullName();
                Interface::PrintLine("%s  is not evil ", FullName);
            }
        }
        else {
            const char* v4 = v7->GetFullName();
            Interface::PrintLine("%s  is evil ", v4);
        }
    }
    return true;
}

bool Script::IsActorVictimConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    TESObjectREFR* v8 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v8 = apRefObject;
    }

    afReturnValue = 0.0;
    if ((v8) && (v8->IsNpc())) {
        TESForm* ObjectReference = v8->GetObjectReference();
        if ((ObjectReference) && (ObjectReference == GetThreadTLSData()->pCrimeVictim)) {
            afReturnValue = 1.0;
        }

        if (!GetThreadTLSData()->bSystemOutput || 0.0 == afReturnValue) {
            if (GetThreadTLSData()->bSystemOutput) {
                const char* FullName = v8->GetFullName();
                Interface::PrintLine("%s  is not a crime victim ", FullName);
            }
        }
        else {
            const char* v4 = v8->GetFullName();
            Interface::PrintLine("%s  crime victim ", v4);
        }
    }

    return true;
}

bool Script::GetIdleDoneOnceConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    if ((pactor) && (pactor->pCurrentProcess)) {
        BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
        if (CurrentAIProcess->GetIdleDoneOnce()) {
            afReturnValue = 1.0;
        }
    }

    return true;
}

bool Script::IsInDangerousWaterConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor() && static_cast<Actor*>(apRefObject)->IsInWater()) &&
        (apRefObject->GetParentCell())) {
        TESObjectCELL* ParentCell = apRefObject->GetParentCell();
        if (ParentCell->GetWaterType()) {
            TESObjectCELL* v5 = apRefObject->GetParentCell();
            TESWaterForm* WaterType = v5->GetWaterType();
            if (WaterType->IsDangerous()) {
                afReturnValue = 1.0;
            }
        }
    }

    return true;
}

bool Script::GetTimeDeadConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsDead(false) && apRefObject->IsActor())) {
        Actor* pactor = static_cast<Actor*>(apRefObject);
        if (pactor->pCurrentProcess) {
            BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
            float fdeadtime = CurrentAIProcess->GetDeathTime();
            if (fdeadtime > 0.0) {
                float fcurrtime = static_cast<float>(appCalendar.GetHoursPassed());
                afReturnValue = fcurrtime - fdeadtime;
            }
        }
    }

    return true;
}

bool Script::GetCauseofDeathConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = -1.0;
    if (apRefObject->IsActor()) {
        afReturnValue = static_cast<double>(apRefObject->GetCauseOfDeath());
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Cause of death is >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetKillingBlowLimbConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = -1.0;
    if (apRefObject->IsActor()) {
        afReturnValue = static_cast<double>(static_cast<Actor*>(apRefObject)->Actor::GetKillingBlowLimb());
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Last hit limb is >> %0.0f", afReturnValue);
    }
    return true;
}

bool Script::HasFriendDispositionConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        int32_t v4 = static_cast<Actor*>(apRefObject)->GetDisposition(pPlayer, nullptr);
        if (v4 >= iDialogueDispositionFriendValue.Int()) {
            afReturnValue = 1.0;
        }
    }

    return true;
}

bool Script::GetGroupTargetCountConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        CombatGroup* pgroup = static_cast<Actor*>(apRefObject)->GetCombatGroup();
        if (pgroup) {
            afReturnValue = static_cast<double>(pgroup->GetTargetCount());
        }
    }

    return true;
}

bool Script::HasPerkConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        int32_t v5 =
            static_cast<Actor*>(apRefObject)->GetPerkRank(static_cast<BGSPerk*>(apParam1), apParam2 != nullptr);
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("Perk Rank >> %d", v5);
        }
        if (v5) {
            afReturnValue = 1.0;
        }
    }

    return true;
}

// The GetActorFactionPlayerEnemy condition: the actor's factions' reaction to the player, and whether that reaction
// comes from a crime. The reaction itself is dropped; only the crime flag counts.
bool Script::GetActorFactionPlayerEnemyConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject == nullptr || !apRefObject->IsActor()) {
        return true;
    }
    Actor* pActor = static_cast<Actor*>(apRefObject);
    bool bFromCrime = false;
    pActor->GetFactionFightReaction(pPlayer, bFromCrime);
    if (bFromCrime) {
        afReturnValue = 1.0;
    }
    if (GetThreadSystemOutput()) {
        if (afReturnValue == 0.0) {
            Interface::PrintLine("This actor is not an enemy of player because of  a faction crime");
        }
        else {
            Interface::PrintLine("This actor's faction is an enemy of player because of crime");
        }
    }
    return true;
}

bool Script::GetIsUsedItemEquipTypeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    // The parameter holds the equip type as an integer: it is compared with GetEquipType's result and indexes the
    // names.
    uint32_t uiEquipType = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apParam1));
    afReturnValue = 0.0;
    if (TESIdleManager::GetUsedItem()) {
        TESObject* UsedItem = TESIdleManager::GetUsedItem();
        if (static_cast<uint32_t>(BGSEquipType::GetEquipType(UsedItem)) == uiEquipType) {
            afReturnValue = 1.0;
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        const char* v6;
        if (!(afReturnValue > 0.0)) {
            v6 = "false";
        }
        else {
            v6 = "true";
        }
        Interface::PrintLine("GetIsUsedItemEquipType '%s' >> %s", BGSEquipType::pEquipTypeNames[uiEquipType], v6);
    }
    return true;
}

bool Script::GetAggroRadiusViolatedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pactor = static_cast<Actor*>(apRefObject);
        if ((pactor->pCurrentProcess) && (pactor->pCurrentProcess->GetRunOncePackage())) {
            TESPackage* v4 = pactor->pCurrentProcess->GetRunOncePackage();
            if (v4->GetPackType() == PACKAGE_TYPE_GUARD) {
                afReturnValue = 1.0;
            }
        }

        if (GetThreadTLSData()->bSystemOutput) {
            if (!(afReturnValue > 0.0)) {
                Interface::PrintLine("%s does not have his aggro radius violated ", apRefObject->GetFullName());
            }
            else {
                Interface::PrintLine("%s has his aggro radius violated ", apRefObject->GetFullName());
            }
        }
    }

    return true;
}

bool Script::GetMapMarkerVisibleConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        MapMarkerData* MapMarkerData_ = apRefObject->GetMapMarkerData();
        if ((MapMarkerData_) && (MapMarkerData_->GetVisible())) {
            afReturnValue = 1.0;
            if (MapMarkerData_->GetTravelLoc()) {
                afReturnValue = 2.0;
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("marker state = %.0f", afReturnValue);
    }
    return true;
}

bool Script::GetWeaponHealthPercConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pactor = static_cast<Actor*>(apRefObject);
        if (pactor->pCurrentProcess) {
            ItemChange* pweapon = pactor->pCurrentProcess->GetCurrentWeapon();
            if (!pweapon || pweapon->pContainerObj == pFists) {
                afReturnValue = 100.0;
            }
            else {
                afReturnValue = pweapon->GetItemHealth(true);
            }
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetWeaponHealthPerc >> %0.2f", afReturnValue);
    }
    return true;
}

// The process comes from MobileObject::GetCurrentAIProcess and the dialogue target from a trivial one-field getter; the
// greeting state is read with BaseProcess::GetGreetingFlag.
bool Script::IsGreetingPlayerConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pactor = static_cast<Actor*>(apRefObject);
        if ((pactor->pCurrentProcess) && (pactor->pCurrentProcess->GetGreetingFlag()) &&
            (pactor->pDialogueItemTarget == pPlayer)) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsGreeting the player >> %.2f", afReturnValue);
    }
    return true;
}

// The GetXPForNextLevel condition: the experience the next level needs minus the experience the player has, while the
// player is below the maximum level. Subtracts as integers and converts the difference to a double.
bool Script::GetXPForNextLevelConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    int32_t iLevel = pPlayer->GetLevel();
    if (iLevel < static_cast<int32_t>(iMaxCharacterLevel.UInt())) {
        // The player pointer is converted to ActorValueOwner without a null test, as the game does: a reference
        // conversion, which does not test.
        ActorValueOwner& rOwner = *pPlayer;
        int32_t iExperience = rOwner.GetActorValue(ActorValue::EXPERIENCE_POINTS);
        int32_t iNeeded = GamePlayFormulas::GetRequiredExperiencePoints(iLevel + 1);
        afReturnValue = static_cast<double>(iNeeded - iExperience);
        if (GetThreadSystemOutput()) {
            Interface::PrintLine("GetXPForNextLevel >> %.2f", afReturnValue);
        }
        return true;
    }
    if (GetThreadSystemOutput()) {
        Interface::PrintLine<>("Player already at maximum level.");
    }
    return true;
}

bool Script::GetIgnoreCrimeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pactor = static_cast<Actor*>(apRefObject);
        if (pactor->GetIgnoreCrime()) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        if (!(afReturnValue > 0.0)) {
            Interface::PrintLine("%s is not ignoring crime", apRefObject->GetFullName());
        }
        else {
            Interface::PrintLine("%s is ignoring crime", apRefObject->GetFullName());
        }
    }
    return true;
}

bool Script::IsGoreDisabledConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (bDisableAllGore.Bool()) {
        afReturnValue = 1.0;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsGoreDisabled >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetAngleConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (apRefObject) {
        const NiPoint3* AngleOnReference = &apRefObject->GetAngleOnReference();
        if (apParam1 == reinterpret_cast<void*>(static_cast<uintptr_t>(88))) {
            afReturnValue = AngleOnReference->x * 57.2957763671875;
        }
        else {
            if (apParam1 == reinterpret_cast<void*>(static_cast<uintptr_t>(89))) {
                afReturnValue = AngleOnReference->y * 57.2957763671875;
            }
            else {
                if (apParam1 == reinterpret_cast<void*>(static_cast<uintptr_t>(90))) {
                    afReturnValue = AngleOnReference->z * 57.2957763671875;
                }
            }
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("GetAngle: %c >> %0.2f", apParam1, afReturnValue);
        }
    }
    return true;
}

bool Script::GetStartingPosConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    // The parameter holds the axis character ('X', 'Y' or 'Z') as an integer; the whole 32-bit value is compared.
    int32_t iAxis = static_cast<int32_t>(reinterpret_cast<uintptr_t>(apParam1));
    if (apRefObject) {
        NiPoint3 pos = apRefObject->GetStartingLocation();
        switch (iAxis) {
            case 88: {
                afReturnValue = pos.x;
                break;
            }
            case 89: {
                afReturnValue = pos.y;
                break;
            }
            case 90: {
                afReturnValue = pos.z;
                break;
            }
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("GetStartingPos: %c >> %0.2f", iAxis, afReturnValue);
        }
    }
    return true;
}

bool Script::GetStartingAngleConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    // The parameter holds the axis character ('X', 'Y' or 'Z') as an integer; the whole 32-bit value is compared.
    int32_t cAxis = static_cast<int32_t>(reinterpret_cast<uintptr_t>(apParam1));
    if (apRefObject) {
        NiPoint3 pos = apRefObject->GetStartingAngle();
        switch (cAxis) {
            case 88: {
                afReturnValue = pos.x * 57.2957763671875;
                break;
            }
            case 89: {
                afReturnValue = pos.y * 57.2957763671875;
                break;
            }
            case 90: {
                afReturnValue = pos.z * 57.2957763671875;
                break;
            }
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("GetStartingAngle: %c >> %0.2f", cAxis, afReturnValue);
        }
    }
    return true;
}

bool Script::GetIsUsedItemTypeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apParam1) && (TESIdleManager::GetUsedItem())) {
        TESObject* UsedItem = TESIdleManager::GetUsedItem();
        // The parameter holds the form type as an integer; the whole parameter value is compared with GetFormType's
        // result.
        if (static_cast<uintptr_t>(UsedItem->GetFormType()) == reinterpret_cast<uintptr_t>(apParam1)) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetIsUsedItemType >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetHasNoteConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (pPlayer->HasNote(static_cast<BGSNote*>(apParam1))) {
        afReturnValue = 1.0;
    }
    return true;
}

bool Script::GetHitLocationConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = -1.0;
    if (apRefObject != nullptr && apRefObject->IsActor()) {
        Actor* actor = static_cast<Actor*>(apRefObject);
        if (actor->GetLastHitData() != nullptr) {
            HitData* hitData = actor->GetLastHitData();
            // eDamageLimb is read as a signed 32-bit value.
            afReturnValue = static_cast<double>(static_cast<int32_t>(hitData->eDamageLimb));
        }
    }
    return true;
}

bool Script::GetMenuModeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(reinterpret_cast<intptr_t>(apParam1));
    if (eMenu == 0) {
        afReturnValue = Interface::IsInMenuMode();
    }
    else if (bUseOldMenuModeScriptFunction) {
        if (Interface::IsTopMenuID(eMenu)) {
            afReturnValue = 1.0;
        }
    }
    else if (Interface::IsMenuIDVisible(eMenu, 0)) {
        afReturnValue = 1.0;
    }
    if (GetThreadSystemOutput()) {
        Interface::PrintLine("MenuMode %d >> %0.2f", eMenu, afReturnValue);
    }
    return true;
}

bool Script::GetInCellConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESForm* pTargetCell = nullptr;
    if (apParam1 != nullptr && static_cast<TESForm*>(apParam1)->GetFormType() == CELL_ID) {
        pTargetCell = static_cast<TESForm*>(apParam1);
    }
    TESObjectREFR* pRef = nullptr;
    if (apParam2 != nullptr) {
        // the reference form types: REFR through PFLA, and PCBE
        int32_t eType = static_cast<TESForm*>(apParam2)->GetFormType();
        if (eType >= REFR_ID && (eType <= PFLA_ID || eType == PCBE_ID)) {
            pRef = static_cast<TESObjectREFR*>(apParam2);
        }
    }
    if (pRef == nullptr) {
        pRef = apRefObject;
    }
    if (pRef != nullptr) {
        bool bCached;
        {
            std::lock_guard<std::mutex> kLock(kInCellLock);
            bCached = pRef == pInCellRef && pInCell == pTargetCell;
            if (bCached) {
                afReturnValue = fInCell;
            }
        }
        if (!bCached) {
            TESForm* pParentCell = pRef != nullptr ? pRef->GetParentCell() : nullptr;
            if (pParentCell != nullptr && pTargetCell != nullptr) {
                uint32_t uiLength = pTargetCell->GetFormEditorIDLength();
                const char* pTargetName = pTargetCell->GetFormEditorID();
                const char* pParentName = pParentCell->GetFormEditorID();
                if (_strnicmp(pParentName, pTargetName, uiLength) == 0) {
                    afReturnValue = 1.0;
                }
            }
            std::lock_guard<std::mutex> kLock(kInCellLock);
            pInCellRef = pRef;
            pInCell = pTargetCell;
            fInCell = static_cast<float>(afReturnValue);
        }
    }
    if (GetThreadSystemOutput()) {
        Interface::PrintLine("GetInCell >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetDistanceConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (apRefObject != nullptr && apParam1 != nullptr) {
        afReturnValue = apRefObject->GetDistanceFromReference(static_cast<TESObjectREFR*>(apParam1), true, false);
    }
    if (GetThreadSystemOutput()) {
        Interface::PrintLine("GetDistance >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetActorValueConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Actor* pActor = nullptr;
    if (apRefObject != nullptr && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }
    if (pActor == nullptr) {
        return true;
    }
    // a disabled actor answers with its base form's value
    if (!pActor->GetDisabled()) {
        ActorValueOwner* pOwner = pActor;
        afReturnValue =
            pOwner->GetActorFloatValue(static_cast<ActorValue::Index>(reinterpret_cast<intptr_t>(apParam1)));
    }
    else {
        ActorValueOwner* pOwner = pActor->GetActorBase();
        afReturnValue =
            pOwner->GetActorFloatValue(static_cast<ActorValue::Index>(reinterpret_cast<intptr_t>(apParam1)));
    }
    if (GetThreadSystemOutput()) {
        Interface::PrintLine(
            "GetActorValue: %s >> %0.2f",
            ActorValue::GetActorValueScriptName(static_cast<ActorValue::Index>(reinterpret_cast<intptr_t>(apParam1))),
            afReturnValue
        );
    }
    return true;
}

bool Script::GetTalkedToPCConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESObjectREFR* pRef;
    if (apRefObject != nullptr) {
        pRef = apRefObject->IsActor() ? apRefObject : nullptr;
    }
    else {
        pRef = static_cast<TESObjectREFR*>(apParam1);
    }
    if (pRef != nullptr && pRef->GetHasSpokenFlag()) {
        afReturnValue = 1.0;
    }
    if (GetThreadSystemOutput()) {
        Interface::PrintLine("GetTalkedToPC >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetShouldAttackConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    int32_t iResult = 0;
    Actor* pTarget = nullptr;
    if (apParam1 != nullptr) {
        int32_t eType = static_cast<TESObjectREFR*>(apParam1)->GetFormType();
        if (eType >= ACHR_ID && eType <= ACRE_ID) {
            pTarget = static_cast<Actor*>(apParam1);
        }
    }
    Actor* pActor = nullptr;
    if (apRefObject != nullptr && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }
    if (pActor != nullptr && pTarget != nullptr) {
        bool bTargetInCombat = pTarget->Actor::IsInCombat();
        // the disposition is computed and dropped
        pActor->GetDisposition(pTarget, nullptr);
        // an actor that wants to help a target in combat answers without touching the result
        if (bTargetInCombat && pTarget->GetCombatController() != nullptr &&
            pCombatManager->GetWantToHelpTarget(pActor, pTarget)) {
            return true;
        }
        FIGHT_REACTION eReaction = FIGHT_REACTION_NEUTRAL;
        if (pActor->GetShouldAttackActor(pTarget, false, eReaction, false)) {
            iResult = 100;
        }
        afReturnValue = iResult;
    }
    if (GetThreadSystemOutput()) {
        Interface::PrintLine("GetShouldAttack >> %i", iResult);
    }
    return true;
}

bool Script::GetIsSexConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESActorBase* pBase = nullptr;
    if (apRefObject != nullptr && apRefObject->GetObjectReference()->GetFormType() == NPC__ID) {
        pBase = static_cast<TESActorBase*>(apRefObject->GetObjectReference());
    }
    if (pBase != nullptr && pBase->GetSex() == static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam1))) {
        afReturnValue = 1.0;
    }
    if (GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsSex >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsIDConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESForm* pObject = nullptr;
    if (apParam1 != nullptr && static_cast<TESForm*>(apParam1)->IsBoundObject()) {
        pObject = static_cast<TESForm*>(apParam1);
    }
    if (apRefObject != nullptr && pObject != nullptr) {
        TESForm* pBase = nullptr;
        if (apRefObject == pOPTidref) {
            pBase = static_cast<TESForm*>(pOPTidobj);
        }
        else {
            // a leveled creature counts as its original base form
            pBase = apRefObject->GetExtra()->GetLevCreaOriginalBase();
            if (pBase == nullptr) {
                pBase = apRefObject->GetObjectReference();
            }
            pOPTidref = apRefObject;
            pOPTidobj = pBase;
        }
        if (pBase == pObject) {
            afReturnValue = 1.0;
        }
    }
    if (GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsID >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsInCombatConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if (apRefObject != nullptr && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }
    if (pActor != nullptr && pActor->Actor::IsInCombat()) {
        afReturnValue = 1.0;
    }
    bool bAreAllCombatTargetsSearching = false;
    if (pActor == pPlayer) {
        afReturnValue = pPlayer->IsPlayerCharacterInCombat(&bAreAllCombatTargetsSearching);
    }
    if (GetThreadSystemOutput()) {
        if (afReturnValue == 0.0) {
            Interface::PrintLine("%s is not in combat", TESFullName::GetFullName(pActor));
        }
        else {
            Interface::PrintLine("%s is in combat", TESFullName::GetFullName(pActor));
        }
    }
    return true;
}

bool Script::GetWalkSpeedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Actor* pActor = nullptr;
    if (apRefObject != nullptr && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }
    if (pActor != nullptr) {
        afReturnValue = pActor->GetWalkSpeed();
        if (GetThreadSystemOutput()) {
            Interface::PrintLine("GetWalkSpeed >> %0.2f", afReturnValue);
        }
    }
    return true;
}

void Script::ClearCachedValues() {
    pDispositionActor = nullptr;
    pDispositionTarget = nullptr;
    fDisposition = 0.0f;
    {
        std::lock_guard<std::mutex> kLock(kInCellLock);
        pInCellRef = nullptr;
        pInCell = nullptr;
        fInCell = 0.0f;
    }
    pFaction = nullptr;
    pInFactionRef = nullptr;
    fInFaction = 0.0f;
    pOPTidobj = nullptr;
    pOPTidref = nullptr;
    pOPTvoice = nullptr;
    pOPTvoiceref = nullptr;
    pIsPlayableRaceRef = nullptr;
    fIsPlayableRace = 0.0f;
}

bool Script::IsPC1stPersonConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (!pPlayer->Is3rdPerson()) {
        afReturnValue = 1.0;
    }
    return true;
}

bool Script::GetCurrentTimeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = (double)(&appCalendar)->GetHour();
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetCurrentTime >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsCurrentFurnitureRefConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    BaseProcess* pBaseProcess;
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor) {
        pBaseProcess = pActor->MobileObject::GetCurrentAIProcess();
        if (static_cast<const void*>(pBaseProcess->GetCurrentFurniture()) == static_cast<const void*>(apParam1)) {
            afReturnValue = 1.0;
        }
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsCurrentFurnitureRef>> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsRefConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    int32_t formType;
    afReturnValue = 0.0;
    TESForm* v6 = nullptr;
    if (apParam1) {
        formType = (int32_t)static_cast<TESForm*>(apParam1)->GetFormType();
        if ((formType >= 58) && ((formType <= 64) || (formType == 105))) {
            v6 = static_cast<TESForm*>(apParam1);
        }
    }
    if ((apRefObject) && (v6) && (static_cast<const void*>(apRefObject) == static_cast<const void*>(v6))) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsRef >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetUnconsciousConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    const char* v4;
    const char* fullName;
    afReturnValue = 0.0;
    Actor* v7 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v7 = static_cast<Actor*>(apRefObject);
    }

    if (v7) {
        if (v7->IsUnConscious()) {
            afReturnValue = 1.0;
        }
        if (Script::GetThreadSystemOutput()) {
            if (0.0 == afReturnValue) {
                fullName = v7->TESObjectREFR::GetFullName();
                Interface::PrintLine("%s is not unconscious", fullName);
            }
            else {
                v4 = v7->TESObjectREFR::GetFullName();
                Interface::PrintLine("%s is unconscious", v4);
            }
        }
    }
    return true;
}

// The heading is subtracted from the direction's angle at extended precision before the store, so both go to double.
// The mobile reference's location is fetched before apParam1's.
bool Script::GetHeadingAngleConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    MobileObject* pMobile = nullptr;
    if ((apRefObject) && (apRefObject->IsMobileObject())) {
        pMobile = static_cast<MobileObject*>(apRefObject);
    }

    TESObjectREFR* pTarget = static_cast<TESObjectREFR*>(apParam1);
    if (pMobile && pTarget) {
        const NiPoint3* pMobileLocation = &pMobile->GetLocationOnReference();
        NiPoint3 kDirection = pTarget->GetLocationOnReference().operator-(*pMobileLocation);
        float fZAngle = GetZAngleFromVector(kDirection);
        afReturnValue = static_cast<double>(fZAngle) - static_cast<double>(pMobile->GetHeading(false));
        while (afReturnValue < -3.1415927410125732) {
            afReturnValue = afReturnValue + 6.2831854820251465;
        }
        while (afReturnValue > 3.1415927410125732) {
            afReturnValue = afReturnValue - 6.2831854820251465;
        }
        afReturnValue = afReturnValue * 57.2957763671875;
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Heading Angle: %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetTalkingActivatorActorConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (!apRefObject) {
        return true;
    }
    TESBoundObject* objectReference = apRefObject->TESObjectREFR::GetObjectReference();
    if (((int32_t)objectReference->TESForm::GetFormType()) != 22) {
        return true;
    }
    TESBoundObject* v6 = apRefObject->TESObjectREFR::GetObjectReference();
    bool vCondition1;
    vCondition1 = static_cast<const void*>(static_cast<TESObjectCONT*>(v6)->GetCloseSound()) ==
        static_cast<const void*>(static_cast<uint8_t*>(apParam1));
    if (vCondition1) {
        afReturnValue = 1.0;
    }
    return true;
}

bool Script::GetPlayerTeammateCountConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    afReturnValue = (double)pPlayer->GetTeammateCount();
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Player teammate count  >> %d", (int32_t)afReturnValue);
    }
    return true;
}

bool Script::IsPlayerGrabbedRefConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apParam1) && (static_cast<const void*>(pPlayer->GetGrabObject()) == static_cast<const void*>(apParam1))) {
        afReturnValue = 1.0;
    }

    return true;
}

bool Script::GetVATSRightAreaFreeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)
                            ->GetVATSAreaFree(static_cast<TESObjectREFR*>(apParam1), 1.5707963705062866f);
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("VATSRightAreaFree >> %.2f", afReturnValue);
    }
    return true;
}

bool Script::GetVATSLeftAreaFreeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)
                            ->GetVATSAreaFree(static_cast<TESObjectREFR*>(apParam1), 4.71238899230957f);
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("VATSLeftAreaFree >> %.2f", afReturnValue);
    }
    return true;
}

bool Script::GetVATSBackAreaFreeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)
                            ->GetVATSAreaFree(static_cast<TESObjectREFR*>(apParam1), 3.1415927410125732f);
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("VATSBackAreaFree >> %.2f", afReturnValue);
    }
    return true;
}

bool Script::GetVATSFrontAreaFreeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue =
            (double)static_cast<Actor*>(apRefObject)->GetVATSAreaFree(static_cast<TESObjectREFR*>(apParam1), 0.0);
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("VATSFrontAreaFree >> %.2f", afReturnValue);
    }
    return true;
}

bool Script::GetVATSRightTargetVisibleConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)
                            ->GetVATSTargetVisible(static_cast<TESObjectREFR*>(apParam1), 1.5707963705062866f);
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("VATSRightTargetVisible >> %.2f", afReturnValue);
    }
    return true;
}

bool Script::GetVATSLeftTargetVisibleConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)
                            ->GetVATSTargetVisible(static_cast<TESObjectREFR*>(apParam1), 4.71238899230957f);
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("VATSLeftTargetVisible >> %.2f", afReturnValue);
    }
    return true;
}

bool Script::GetVATSBackTargetVisibleConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)
                            ->GetVATSTargetVisible(static_cast<TESObjectREFR*>(apParam1), 3.1415927410125732f);
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("VATSBackTargetVisible >> %.2f", afReturnValue);
    }
    return true;
}

bool Script::GetVATSFrontTargetVisibleConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue =
            (double)static_cast<Actor*>(apRefObject)->GetVATSTargetVisible(static_cast<TESObjectREFR*>(apParam1), 0.0);
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("VATSFrontTargetVisible >> %.2f", afReturnValue);
    }
    return true;
}

bool Script::ChallengeLockedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (apParam1) {
        afReturnValue = (double)static_cast<TESChallenge*>(apParam1)->GetUnlocked();
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("ChallengeLocked? >> %f", afReturnValue);
    }
    return true;
}

bool Script::GetChallengeCompletedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (apParam1) {
        afReturnValue = (double)static_cast<TESChallenge*>(apParam1)->GetCompleted();
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("ChallengeComplete? >> %f", afReturnValue);
    }
    return true;
}

bool Script::GetIsCurrentPackageConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    TESPackage* ppackage = nullptr;
    if ((apParam1) && (static_cast<TESForm*>(apParam1)->GetFormType() == PACK_ID)) {
        ppackage = static_cast<TESPackage*>(apParam1);
    }

    if ((pactor) && (ppackage)) {
        TESPackage* pactorpack = pactor->GetPackageSetAsPcurrent();
        if ((pactorpack) && (pactorpack->GetIsCreated()) && ((&pactor->m_Extra)->GetPackageExtra())) {
            pactorpack = (&pactor->m_Extra)->GetPackageExtra();
        }

        if (pactorpack == ppackage) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetIsCurrentPackage >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsCurrentFurnitureObjConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    BGSListForm* plistform = static_cast<BGSListForm*>(apParam1);
    if ((pactor) && (plistform)) {
        BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
        TESObjectREFR* pref = CurrentAIProcess->GetCurrentFurniture();
        if (pref) {
            if (plistform->GetFormType() == FURN_ID) {
                if (pref->GetObjectReference() == apParam1) {
                    afReturnValue = 1.0;
                }
            }
            else {
                if (plistform->GetFormType() == FLST_ID) {
                    TESForm* item = pref->GetObjectReference();
                    if ((&plistform->ListOfForms)->IsInList(item)) {
                        afReturnValue = 1.0;
                    }
                }
            }
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsCurrentFurnitureObj>> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetStageConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESQuest* pquest = static_cast<TESQuest*>(apParam1);
    if (pquest) {
        afReturnValue = static_cast<double>(pquest->GetCurrentStage());
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetStage >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetStageDoneConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESQuest* pquest = static_cast<TESQuest*>(apParam1);
    // The second parameter holds the stage index as an integer; only its low byte is passed.
    uint8_t ucstage = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(apParam2));
    if (pquest && pquest->IsStageDone(ucstage)) {
        afReturnValue = 1.0;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetStageDone >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetFactionRankDifferenceConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pact = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pact = static_cast<Actor*>(apRefObject);
    }

    // The second parameter is the other actor.
    Actor* ptarget = static_cast<Actor*>(apParam2);
    if (!pact || !ptarget) {
        return true;
    }
    TESActorBase* pactorbase1 = pact->GetActorBase();
    TESActorBase* pactorbase2 = ptarget->GetActorBase();
    TESFaction* pfac = nullptr;
    if ((apParam1) && (static_cast<TESForm*>(apParam1)->GetFormType() == FACT_ID)) {
        pfac = static_cast<TESFaction*>(apParam1);
    }

    if ((pactorbase1) && (pactorbase2) && (pfac)) {
        int32_t irank1 = pactorbase1->GetFactionRank(pfac, pact == pPlayer);
        int32_t irank2 = pactorbase2->GetFactionRank(pfac, ptarget == pPlayer);
        if (irank1 != -1 && irank2 != -1) {
            afReturnValue = static_cast<double>(irank1 - irank2);
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetFactionRankDifference >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetInSameCellConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    TESObjectCELL* ptestcell;
    TESObjectCELL* prefcell;
    afReturnValue = 0.0;
    TESObjectREFR* ptestref = nullptr;
    if (apParam1) {
        int32_t FormType = static_cast<TESForm*>(apParam1)->GetFormType();
        if (FormType >= 58 && (FormType <= 64 || FormType == 105)) {
            ptestref = static_cast<TESObjectREFR*>(apParam1);
        }
    }
    if (apRefObject) {
        prefcell = apRefObject->GetParentCell();
    }
    else {
        prefcell = nullptr;
    }
    if (ptestref) {
        ptestcell = ptestref->GetParentCell();
    }
    else {
        ptestcell = nullptr;
    }
    if ((prefcell) && (ptestcell) && (prefcell == ptestcell)) {
        afReturnValue = 1.0;
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetInSameCell >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsVoiceTypeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        BGSVoiceType* pcompvoice = nullptr;
        if (apRefObject == pOPTvoiceref) {
            pcompvoice = static_cast<BGSVoiceType*>(pOPTvoice);
        }
        else {
            int32_t FormType = apRefObject->GetObjectReference()->GetFormType();
            if (FormType == 22) {
                pcompvoice = static_cast<BGSTalkingActivator*>(apRefObject->GetObjectReference())
                                 ->BGSTalkingActivator::GetVoiceType();
            }
            else {
                if (FormType > 41 && FormType <= 43) {
                    TESActorBase* pab = static_cast<TESActorBase*>(apRefObject->GetObjectReference());
                    pcompvoice = pab->GetVoiceType();
                }
            }
            pOPTvoiceref = apRefObject;
            pOPTvoice = pcompvoice;
        }
        if (pcompvoice == apParam1) {
            afReturnValue = 1.0;
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetIsVoiceType >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetFactionRankConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = -1.0;
    Actor* pact = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pact = static_cast<Actor*>(apRefObject);
    }

    if (!pact) {
        return true;
    }
    pact->GetActorBase();
    TESFaction* pfac = nullptr;
    // The condition parameters arrive untyped; a form parameter is a TESForm.
    if ((apParam1) && (static_cast<TESForm*>(apParam1)->GetFormType() == FACT_ID)) {
        pfac = static_cast<TESFaction*>(apParam1);
    }

    if (pfac) {
        afReturnValue = static_cast<double>(pact->GetFactionRank(pfac, pact == pPlayer));
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetFactionRank >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetGlobalValueConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = -1.0;
    TESGlobal* pglobal = nullptr;
    if ((apParam1) && (static_cast<TESForm*>(apParam1)->GetFormType() == GLOB_ID)) {
        pglobal = static_cast<TESGlobal*>(apParam1);
    }

    if (pglobal) {
        afReturnValue = pglobal->fValue;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetGlobalValue >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetDispositionConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    TESObjectREFR* ptargetRef = static_cast<TESObjectREFR*>(apParam1);
    Actor* ptarget = nullptr;
    if ((ptargetRef) && (ptargetRef->IsActor())) {
        ptarget = static_cast<Actor*>(ptargetRef);
    }

    if ((pactor) && (ptarget)) {
        if (pactor == pDispositionActor && ptarget == pDispositionTarget) {
            afReturnValue = fDisposition;
        }
        else {
            afReturnValue = static_cast<double>(pactor->GetDisposition(ptarget, nullptr));
            pDispositionTarget = ptarget;
            pDispositionActor = pactor;
            fDisposition = afReturnValue;
        }
        if (GetThreadTLSData()->bSystemOutput) {
            double v6 = afReturnValue;
            const char* FullName = ptarget->GetFullName();
            Interface::PrintLine("%.20s disposition to %.20s is %.1f", pactor->GetFullName(), FullName, v6);
        }
    }

    return true;
}

// The condition's second parameter holds the crime type itself, not a form: it is tested as a signed integer in 0..4
// and CRIME_TYPE_NONE is passed for any other value.
bool Script::GetCrimeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    int32_t iCrimeType = static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam2));
    Actor* pcriminal = nullptr;
    if (apParam1) {
        int32_t FormType = static_cast<TESForm*>(apParam1)->GetFormType();
        if (FormType >= 58 && (FormType <= 64 || FormType == 105)) {
            pcriminal = static_cast<Actor*>(apParam1);
        }
    }
    Actor* ptarget = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        ptarget = static_cast<Actor*>(apRefObject);
    }

    if ((ptarget) && (pcriminal)) {
        if (iCrimeType > -1 && iCrimeType < 5) {
            if (ReferenceProcessLists.IsCrimeinGlobalCrimeList(
                    ptarget, pcriminal, ptarget, static_cast<PackageNS::CRIME_TYPE>(iCrimeType), nullptr, -1
                )) {
                afReturnValue = 1.0;
            }
        }
        else {
            if (ReferenceProcessLists.IsCrimeinGlobalCrimeList(
                    ptarget, pcriminal, ptarget, PackageNS::CRIME_TYPE_NONE, nullptr, -1
                )) {
                afReturnValue = 1.0;
            }
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetCrime >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsSpellTargetConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESObjectREFR* v6 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v6 = apRefObject;
    }

    if ((v6) && (apParam1)) {
        MagicTarget* v4 = v6->GetMagicTarget();
        if (v4->IsSpellTarget(static_cast<MagicItem*>(apParam1), true)) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("Is Spell Target >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetSpellUsageNumberConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    if ((pactor) && (apParam1)) {
        afReturnValue = pactor->GetSpellUsageCount(static_cast<MagicItem*>(apParam1));
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetSpellUsageNum >> %0.2f", afReturnValue);
    }
    return true;
}

// The condition's first parameter holds the player action itself, not a form.
bool Script::IsPlayerActionActiveConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    afReturnValue =
        static_cast<double>(pPlayer->IsActionInList(static_cast<PLAYER_ACTION>(reinterpret_cast<uintptr_t>(apParam1))));
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsActionInList >> %0.2f", afReturnValue);
    }
    return true;
}

// The condition's first parameter holds the limb index itself, not a form: the pointer's value is used as the integer.
bool Script::IsLimbGoneConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) &&
        (apRefObject->IsActor() &&
         apRefObject->GetDismembered(static_cast<BGSBodyPart::LIMB_ENUM>(reinterpret_cast<uintptr_t>(apParam1))))) {
        afReturnValue = 1.0;
    }

    return true;
}

bool Script::GetKillerConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pactor = static_cast<Actor*>(apRefObject);
        if (pactor->IsDead(false) && apParam1 == pactor->pMyKiller) {
            afReturnValue = 1.0;
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("IsKiller >> %0.2f", afReturnValue);
        }
    }

    return true;
}

bool Script::GetKillerObjectConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    BGSListForm* pformlist = static_cast<BGSListForm*>(apParam1);
    Actor* pactor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pactor = static_cast<Actor*>(apRefObject);
    }

    if ((pactor) && (pactor->pCurrentProcess) && (pactor->IsDead(false)) && (pformlist)) {
        ExtraDismemberedLimbs* pDeathStruct = (&pactor->m_Extra)->GetDismembermentExtra();
        TESForm* pweap = pDeathStruct->pDeathObject;
        if ((pweap) && ((&pformlist->ListOfForms)->IsInList(pweap))) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsKillerObject >> %0.2f", afReturnValue);
    }
    return true;
}

// The first parameter is a faction; the reaction is read through its TESReactionForm base.
bool Script::GetFactionCombatReactionConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if ((apParam2) && (apParam1)) {
        afReturnValue = static_cast<double>(
            static_cast<TESFaction*>(apParam1)->GetReaction_0048C1B0(static_cast<TESForm*>(apParam2))
        );
    }

    return true;
}

bool Script::GetIsObjectTypeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    // The parameter holds the form type as an integer; the whole parameter value is compared with GetFormType's result.
    if (static_cast<uintptr_t>(apRefObject->GetObjectReference()->GetFormType()) ==
        reinterpret_cast<uintptr_t>(apParam1)) {
        afReturnValue = 1.0;
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("GetIsObjectType is  >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetObjectiveDisplayedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (apParam1) {
        // The script passes the objective's index in the parameter's pointer slot.
        BGSQuestObjective* pobjective = static_cast<TESQuest*>(apParam1)->GetObjective(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apParam2))
        );
        if (pobjective) {
            double v5;
            if (pobjective->GetDisplayed()) {
                v5 = 1.0;
            }
            else {
                v5 = 0.0;
            }
            afReturnValue = v5;
            if (GetThreadTLSData()->bSystemOutput) {
                Interface::PrintLine("GetObjectiveDisplayed >> %0.2f", afReturnValue);
            }
        }
    }
    return true;
}

bool Script::IsPlayerTagSkillConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        // The script passes the skill's actor value index in the parameter's pointer slot.
        ActorValue::Index eSkill = static_cast<ActorValue::Index>(reinterpret_cast<intptr_t>(apParam1));
        if (apRefObject->IsActor() && ActorValue::IsSkill(eSkill)) {
            TESClass* Class = pPlayer->GetClass();
            if (Class->IsTagSkill(eSkill)) {
                afReturnValue = 1.0;
            }
        }
    }
    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsPlayerTagSkill >> %.0f", afReturnValue);
    }
    return true;
}

// The casino's form ID and its data come from one-field getters (trivial accessors).
bool Script::GetCasinoWinningsLevelConditionalFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (!apParam1) {
        return false;
    }
    TESCasino* pCasino = static_cast<TESCasino*>(apParam1);
    for (BSSimpleList<CasinoData*>* pcl = pPlayer->GetCasinoDataList(); pcl && pcl->m_item; pcl = pcl->m_pkNext) {
        CasinoData* m_item = pcl->m_item;
        if (m_item->casinoID == pCasino->iFormID) {
            int32_t iMax = pCasino->data.iMaxWinnings;
            int32_t iCurrentWinnings = pcl->m_item->iTotalWinnings;
            afReturnValue = 0.0;
            for (int32_t i = 0; i < 4; ++i) {
                if (static_cast<double>(iCurrentWinnings) >=
                    static_cast<double>(iMax) * (static_cast<double>(i + 1) * 0.25)) {
                    afReturnValue = static_cast<double>(i + 1);
                }
            }
        }
    }
    return true;
}

bool Sky::IsSnowing() {
    if (pCurrentWeather && pCurrentWeather->IsSnow() &&
        fCurrentWeatherPct > pCurrentWeather->GetWeatherData(TESWeather::WD_BEGIN_PRECIP, 0.999f, 0.0f)) {
        return true;
    }
    if (pLastWeather && pLastWeather->IsSnow() &&
        fCurrentWeatherPct < pLastWeather->GetWeatherData(TESWeather::WD_END_PRECIP, 1.0f, 0.001f)) {
        return true;
    }
    return false;
}

char TESCreature::GetCreatureType() const {
    return Data.eType;
}

ScriptLocals* TESQuest::GetScriptLocals() {
    return pScriptLocals;
}

bool TESRace::GetPlayable() {
    return (Data.iFlags & RACE_DATA::RACE_FLAG_PLAYABLE) != 0;
}

bool TESWeather::IsSnow() const {
    return (ucWeatherData[11] & 8) != 0;
}

bool Actor::GetForceHitReaction() {
    return bForceHitReaction;
}

uint32_t CombatGroup::GetMemberCount() {
    return MemberArray.iSize;
}

BSSimpleList<CasinoData*>* PlayerCharacter::GetCasinoDataList() {
    return pListofCasinoData;
}
