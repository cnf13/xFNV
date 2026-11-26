#include "Fallout Shared/FOScriptFunctions.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/BSMultiBoundNode.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESConditionItem.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
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
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include <cstdint>

bool FOScriptFunctions::GetVATSValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    void* pParam1 = nullptr;
    char* pParam2 = nullptr;
    return Script::ParseParameters(
               apParameters,
               apCompiledParams,
               auiOffset,
               apRefObject,
               apContainer,
               apScript,
               apScriptLocals,
               &pParam1,
               &pParam2
           ) &&
        FOScriptFunctions::GetVATSValueConditionFunction(apRefObject, pParam1, pParam2, afReturnValue);
}

bool FOScriptFunctions::GetVATSValueConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    VATS_COMMAND* pAction;
    afReturnValue = 0.0;
    bool bhandled = false;
    if (pVATS.eMode == VATS::VATS_PLAYBACK) {
        pAction = pVATS.GetCurrentAction();
    }
    else if (pVATS.eMode == VATS::VATS_MENU_MODE || pVATS.eMode == VATS::VATS_SCAN_MODE) {
        pAction = VATSMenu::GetCurrentQueuedAction();
    }
    else {
        pAction = nullptr;
    }
    if (pAction) {
        // The script passes the value's index, and for some values the compared value, as integers in the parameter
        // slots.
        int32_t iValueIndex = static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam1));
        if (iValueIndex >= 0 && iValueIndex < 18) {
            switch (iValueIndex) {
                case 0: {
                    TESObjectWEAP* pCompareWeapon = static_cast<TESObjectWEAP*>(apParam2);
                    if (apParam2) {
                        if (pCompareWeapon == pAction->pHitData->pWeapon) {
                            afReturnValue = 1.0;
                        }
                    }
                    break;
                }
                case 1: {
                    BGSListForm* pList = static_cast<BGSListForm*>(apParam2);
                    TESObjectWEAP* pWeapon = pAction->pHitData->pWeapon;
                    if (apParam2 && pWeapon) {
                        TESForm* pForm = pWeapon;
                        if (pList->GetFormList()->IsInList(pForm)) {
                            afReturnValue = 1.0;
                        }
                    }

                    break;
                }
                case 2: {
                    // A form compared with the target's base object (TESForm::Compare), as in case 9.
                    TESForm* pForm = static_cast<TESForm*>(apParam2);
                    if (apParam2 &&
                        pAction->pTarget &&
                        pAction->pTarget->GetObjectReference() &&
                        !pForm->Compare(pAction->pTarget->GetObjectReference())) {
                        afReturnValue = 1.0;
                    }

                    break;
                }
                case 3: {
                    if (pAction->pTarget) {
                        BGSListForm* pList = static_cast<BGSListForm*>(apParam2);
                        if (apParam2) {
                            TESForm* pBaseObject = pAction->pTarget->GetObjectReference();
                            if (pList->GetFormList()->IsInList(pBaseObject)) {
                                afReturnValue = 1.0;
                            }
                        }
                    }
                    break;
                }
                case 4: {
                    afReturnValue = CombatUtilities::GetDistanceBetweenFacingActors(pPlayer, pAction->pTarget, false);
                    break;
                }
                case 5: {
                    if (static_cast<ActorValue::Index>(reinterpret_cast<intptr_t>(apParam2)) == pAction->eLimb) {
                        afReturnValue = 1.0;
                    }
                    break;
                }
                case 6: {
                    if (static_cast<ActionPoints::Action>(reinterpret_cast<intptr_t>(apParam2)) == pAction->eAction) {
                        afReturnValue = 1.0;
                    }
                    break;
                }
                case 7: {
                    afReturnValue = pAction->bSuccess ? 1.0f : 0.0f;
                    break;
                }
                case 8: {
                    afReturnValue = pAction->pHitData->IsFlagSet(HitData::HD_CRITICAL) ? 1.0f : 0.0f;
                    break;
                }
                case 9: {
                    if (apParam2 &&
                        pAction->pHitData->pCriticalEffect &&
                        !static_cast<TESForm*>(apParam2)->Compare(pAction->pHitData->pCriticalEffect)) {
                        afReturnValue = 1.0;
                    }

                    break;
                }
                case 10: {
                    if (pAction->pHitData->pCriticalEffect) {
                        BGSListForm* pList = static_cast<BGSListForm*>(apParam2);
                        if (apParam2) {
                            TESForm* pCriticalEffect = pAction->pHitData->pCriticalEffect;
                            if (pList->GetFormList()->IsInList(pCriticalEffect)) {
                                afReturnValue = 1.0;
                            }
                        }
                    }
                    break;
                }
                case 11: {
                    afReturnValue = pAction->pHitData->IsFlagSet(HitData::HD_FATAL) ? 1.0f : 0.0f;
                    break;
                }
                case 12: {
                    afReturnValue = pAction->pHitData->IsFlagSet(HitData::HD_EXPLODE_LIMB) ? 1.0f : 0.0f;
                    break;
                }
                case 13: {
                    afReturnValue = pAction->pHitData->IsFlagSet(HitData::HD_DISMEMBER_LIMB) ? 1.0f : 0.0f;
                    break;
                }
                case 14: {
                    afReturnValue = pAction->pHitData->IsFlagSet(HitData::HD_CRIPPLE_LIMB) ? 1.0f : 0.0f;
                    break;
                }
                case 15: {
                    if (pAction->pHitData->pWeapon) {
                        if (pAction->pHitData->pWeapon->GetWeaponType() ==
                            static_cast<WEAPON_TYPE>(reinterpret_cast<intptr_t>(apParam2))) {
                            afReturnValue = 1.0;
                        }
                    }
                    else if (!apParam2) {
                        afReturnValue = 1.0;
                    }
                    break;
                }
                case 16: {
                    afReturnValue = pAction->bStranger ? 1.0f : 0.0f;
                    afReturnValue = pAction->bMissFortune ? 1.0 : afReturnValue;
                    break;
                }
                case 17: {
                    afReturnValue = pAction->bParalyzingPalm ? 1.0f : 0.0f;
                    break;
                }
                default: {
                    break;
                }
            }
            bhandled = true;
        }
    }
    if (Interface::IsConsoleVisible()) {
        Interface::PrintLine(
            "GetVATSValue %d, %d -> %0.2f",
            static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam1)),
            static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam2)),
            afReturnValue
        );
    }
    return bhandled;
}

bool FOScriptFunctions::GetParamIsTESForm(FunctionData* apFuncData, uint32_t aiParam) {
    if (aiParam == 1) {
        switch (static_cast<int32_t>(apFuncData->uParam[0].uiNumber)) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 9:
            case 10: {
                return true;
            }
            default: {
                return false;
            }
        }
    }
    return false;
}
