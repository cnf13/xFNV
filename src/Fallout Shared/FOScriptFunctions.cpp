#include "Fallout Shared/FOScriptFunctions.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/TLSData.h"
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
#include "Reconstruction.hpp"

#include <stdint.h>

bool GetVATSValueFunction(const SCRIPT_PARAMETER* apParameters, const char* apCompiledParams,
	TESObjectREFR* apRefObject, TESObjectREFR* apContainer, Script* apScript, ScriptLocals* apScriptLocals,
	double& afReturnValue, uint32_t& auiOffset) {
	char* a3;
	void* a8;
	char v10 = 1;
	a8 = 0;
	a3 = 0;
	return Script::ParseParameters(apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript,
			   apScriptLocals, &a8, &a3) &&
		Cmd_GetVATSValue(apRefObject, a8, a3, afReturnValue);
}

bool Cmd_GetVATSValue(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
	VATS_COMMAND* CurrentAction;
	TESForm* pCriticalEffect;
	TESForm* ObjectReference;
	TESForm* item;
	afReturnValue = 0.0;
	bool v29 = 0;
	if (pVATS.eMode == VATS::VATS_PLAYBACK) {
		CurrentAction = pVATS.GetCurrentAction();
	} else {
		VATS_COMMAND* CurrentQueuedAction;
		if (pVATS.eMode == VATS::VATS_MENU_MODE || pVATS.eMode == VATS::VATS_SCAN_MODE) {
			CurrentQueuedAction = VATSMenu::GetCurrentQueuedAction();
		} else {
			CurrentQueuedAction = 0;
		}
		CurrentAction = CurrentQueuedAction;
	}
	if (CurrentAction) {
		// The script passes the value's index, and for some values the compared value, as integers in the parameter
		// slots.
		int32_t v28 = static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam1));
		if (v28 >= 0 && v28 < 18) {
			switch (v28) {
			case 0: {
				TESObjectWEAP* v23 = static_cast<TESObjectWEAP*>(apParam2);
				if (apParam2) {
					TESObjectWEAP* pWeapon = CurrentAction->pHitData->pWeapon;
					if (v23 == pWeapon) {
						afReturnValue = 1.0;
					}
				}
				break;
			}
			case 1: {
				BGSListForm* v26 = static_cast<BGSListForm*>(apParam2);
				TESObjectWEAP* v27 = CurrentAction->pHitData->pWeapon;
				if (apParam2) {
					if (v27) {
						item = v27;
						// The exe calls the pointer list's shared out-of-line IsInList (0x5F65D0).
						if (v26->GetFormList()->IsInList(item)) {
							afReturnValue = 1.0;
						}
					}
				}
				break;
			}
			case 2: {
				// A form compared with the target's base object (TESForm::Compare, slot 67), as in case 9.
				TESForm* v20 = static_cast<TESForm*>(apParam2);
				if (apParam2) {
					if (CurrentAction->pTarget) {
						if (CurrentAction->pTarget->GetObjectReference()) {
							if (!v20->Compare(CurrentAction->pTarget->GetObjectReference())) {
								afReturnValue = 1.0;
							}
						}
					}
				}
				break;
			}
			case 3: {
				if (CurrentAction->pTarget) {
					BGSListForm* v25 = static_cast<BGSListForm*>(apParam2);
					if (apParam2) {
						ObjectReference = CurrentAction->pTarget->GetObjectReference();
						// The exe calls the pointer list's shared out-of-line IsInList (0x5F65D0).
						if (v25->GetFormList()->IsInList(ObjectReference)) {
							afReturnValue = 1.0;
						}
					}
				}
				break;
			}
			case 4: {
				afReturnValue = CombatUtilities::GetDistanceBetweenFacingActors(pPlayer, CurrentAction->pTarget, 0);
				break;
			}
			case 5: {
				if (static_cast<ActorValue::Index>(reinterpret_cast<intptr_t>(apParam2)) == CurrentAction->eLimb) {
					afReturnValue = 1.0;
				}
				break;
			}
			case 6: {
				if (static_cast<ActionPoints::Action>(reinterpret_cast<intptr_t>(apParam2)) == CurrentAction->eAction) {
					afReturnValue = 1.0;
				}
				break;
			}
			case 7: {
				float v13;
				if (CurrentAction->bSuccess) {
					v13 = 1.0f;
				} else {
					v13 = 0.0f;
				}
				afReturnValue = v13;
				break;
			}
			case 8: {
				float v12;
				if (CurrentAction->pHitData->IsFlagSet(HitData::HD_CRITICAL)) {
					v12 = 1.0f;
				} else {
					v12 = 0.0f;
				}
				afReturnValue = v12;
				break;
			}
			case 9: {
				if (apParam2) {
					if (CurrentAction->pHitData->pCriticalEffect) {
						if (!static_cast<TESForm*>(apParam2)->Compare(CurrentAction->pHitData->pCriticalEffect)) {
							afReturnValue = 1.0;
						}
					}
				}
				break;
			}
			case 10: {
				if (CurrentAction->pHitData->pCriticalEffect) {
					BGSListForm* v24 = static_cast<BGSListForm*>(apParam2);
					if (apParam2) {
						pCriticalEffect = CurrentAction->pHitData->pCriticalEffect;
						// The exe calls the pointer list's shared out-of-line IsInList (0x5F65D0).
						if (v24->GetFormList()->IsInList(pCriticalEffect)) {
							afReturnValue = 1.0;
						}
					}
				}
				break;
			}
			case 11: {
				float v11;
				if (CurrentAction->pHitData->IsFlagSet(HitData::HD_FATAL)) {
					v11 = 1.0f;
				} else {
					v11 = 0.0f;
				}
				afReturnValue = v11;
				break;
			}
			case 12: {
				float v10;
				if (CurrentAction->pHitData->IsFlagSet(HitData::HD_EXPLODE_LIMB)) {
					v10 = 1.0f;
				} else {
					v10 = 0.0f;
				}
				afReturnValue = v10;
				break;
			}
			case 13: {
				float v9;
				if (CurrentAction->pHitData->IsFlagSet(HitData::HD_DISMEMBER_LIMB)) {
					v9 = 1.0f;
				} else {
					v9 = 0.0f;
				}
				afReturnValue = v9;
				break;
			}
			case 14: {
				float v8;
				if (CurrentAction->pHitData->IsFlagSet(HitData::HD_CRIPPLE_LIMB)) {
					v8 = 1.0f;
				} else {
					v8 = 0.0f;
				}
				afReturnValue = v8;
				break;
			}
			case 15: {
				if (CurrentAction->pHitData->pWeapon) {
					if (CurrentAction->pHitData->pWeapon->GetWeaponType() ==
						static_cast<WEAPON_TYPE>(reinterpret_cast<intptr_t>(apParam2))) {
						afReturnValue = 1.0;
					}
				} else {
					if (!apParam2) {
						afReturnValue = 1.0;
					}
				}
				break;
			}
			case 16: {
				double v6;
				float v7;
				if (CurrentAction->bStranger) {
					v7 = 1.0f;
				} else {
					v7 = 0.0f;
				}
				afReturnValue = v7;
				if (CurrentAction->bMissFortune) {
					v6 = 1.0;
				} else {
					v6 = afReturnValue;
				}
				afReturnValue = v6;
				break;
			}
			case 17: {
				float v5;
				if (CurrentAction->bParalyzingPalm) {
					v5 = 1.0f;
				} else {
					v5 = 0.0f;
				}
				afReturnValue = v5;
				break;
			}
			default: {
				break;
			}
			}
			v29 = 1;
		}
	}
	if (Interface::IsConsoleVisible()) {
		Interface::PrintLine("GetVATSValue %d, %d -> %0.2f", static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam1)),
			static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam2)), afReturnValue);
	}
	return v29;
}

bool FOScriptFunctions::GetParamIsTESForm(FunctionData* apFuncData, uint32_t aiParam) {
	uint8_t v3 = 0;
	if (((uint32_t)aiParam) == 1) {
		switch ((int32_t)apFuncData->uParam[0].uiNumber) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 9:
		case 10: {
			v3 = 1;
			break;
		}
		default: {
			return (bool)v3;
		}
		}
	}
	return (bool)v3;
}
