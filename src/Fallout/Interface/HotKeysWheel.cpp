#include "Fallout/Interface/HotKeysWheel.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Tile.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "SDK/MSVC/rtti.h"

HotKeysWheel::~HotKeysWheel() {
	InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(pPlayer);
	for (int32_t aiKey = 0; aiKey < 8; ++aiKey) {
		ItemChange* HotKeyItem = InventoryChanges_->GetHotKeyItem(aiKey);
		if (HotKeyItem) {
			if (HotKeyItem->pContainerObj) {
				TESModelTextureSwap* BipedTESModel_0 =
					TESBipedModelForm::GetBipedTESModel(HotKeyItem->pContainerObj, pPlayer);
				if (!BipedTESModel_0) {
					BipedTESModel_0 = dynamic_cast<TESModelTextureSwap*>(HotKeyItem->pContainerObj);
				}
				if (BipedTESModel_0) {
					const char* v1 = BipedTESModel_0->GetModel();
					pModelLoader->ReleaseFile(v1);
				}
			}
		}
		if (HotKeyItem) {
			delete HotKeyItem;
		}
	}
}

void HotKeysWheel::Initialize(Tile* apWheel) {
	BSStringT<char> apString;
	if (apWheel) {
		pWheel = apWheel;
		for (int32_t i = 0; i < 8; ++i) {
			apString.SPrintF("HK_Item_%d", i);
			char* v2 = apString.pString;
			pHotkeyTilesA[i] = pWheel->GetChildByName(v2);
		}
		iUT_HighlightedHotkey = Tile::AddUserTrait("_SelectedHotkey", -1);
		iUT_HighlightedText = Tile::AddUserTrait("_SelectedText", -1);
		bSelectingHotkey = 0;
		pWheel->Set(iUT_HighlightedHotkey, -1);
		pWheel->Set(iUT_HighlightedText, "", static_cast<bool>(1));
	} else {
		Warning("HotKeyWheel: Hot key wheel being initialized with a NULL parent tile.  This...should not...be!");
	}
}

bool HotKeysWheel::GetVisible() {
	return (bool)((uint8_t)pWheel->Tile::IsTrue(4003));
}

void HotKeysWheel::SetVisible(bool abVisible) {
	if (abVisible) {
		UpdateHotkeyList();
		pWheel->Tile::Set(4003, 1);
	} else {
		pWheel->Tile::Set(4003, 0);
	}
}

void HotKeysWheel::UpdateHotkeyList() {
	InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pPlayer);
	BSStringT<char> strtemp;
	if (pinventory) {
		ItemChange** pcurrHotkeyedItem = pinventory->GetHotKeyItem(0, 7);
		for (int32_t ihotkeyCt = 0; ihotkeyCt < 8; ++ihotkeyCt) {
			if (ihotkeyCt != 1) {
				ItemChange* pcurrHotkeyedItem_2 = pcurrHotkeyedItem[ihotkeyCt];
				if (pcurrHotkeyedItem_2) {
					strtemp.operator=("Interface\\Icons\\");
					BGSEquipType::EQUIP_TYPE EquipType = BGSEquipType::GetEquipType(pcurrHotkeyedItem_2->pContainerObj);
					const char* EquipTypeIconFilename = BGSEquipType::GetEquipTypeIconFilename(EquipType);
					strtemp.operator+=(EquipTypeIconFilename);
					char* v7 = strtemp.pString;
					int32_t v3 = Tile::TextToTrait("_HotKeyIcon");
					pHotkeyTilesA[ihotkeyCt]->Set(v3, v7, 1);
					int32_t v4 = Tile::TextToTrait("_HotKeyAssigned");
					pHotkeyTilesA[ihotkeyCt]->Set(v4, 1);
					if (pcurrHotkeyedItem_2->iNumber <= 1) {
						const char* FullName = pcurrHotkeyedItem_2->GetFullName();
						strtemp.operator=(FullName);
					} else {
						uint32_t FormHealth_0 = pcurrHotkeyedItem_2->iNumber;
						strtemp.SPrintF("%s (%d)", pcurrHotkeyedItem_2->GetFullName(), FormHealth_0);
					}
					pHotkeyTilesA[ihotkeyCt]->Set(4036, strtemp.pString, 1);
				} else {
					int32_t v6 = Tile::TextToTrait("_HotKeyAssigned");
					pHotkeyTilesA[ihotkeyCt]->Set(v6, 0);
					pHotkeyTilesA[ihotkeyCt]->Set(4036, "", 1);
				}
			}
		}
		for (int32_t i = 0; i < 8; ++i) {
			ItemChange* pcurrHotkeyedItem_3 = pcurrHotkeyedItem[i];
			if (pcurrHotkeyedItem_3) {
				delete pcurrHotkeyedItem_3;
			}
		}
		ourFree(pcurrHotkeyedItem);
	}
}

int32_t HotKeysWheel::MoveHighlight(int32_t aiNewIndex) {
	static int32_t inewKeyPressed = -1;
	static int32_t ikeyPressStart;
	int32_t a3 = -1;
	if (aiNewIndex == 1) {
		return -1;
	}
	if (InterfaceManager::Has360Controller()) {
		if (aiNewIndex < 0) {
			int32_t v9 = InterfaceManager::CheckMenuButton(Interface::eButtonDPadDown, 0);
			int32_t v11 = InterfaceManager::CheckMenuButton(Interface::eButtonDPadUp, 0);
			int32_t v8 = InterfaceManager::CheckMenuButton(Interface::eButtonDPadLeft, 0);
			int32_t v10 = InterfaceManager::CheckMenuButton(Interface::eButtonDPadRight, 0);
			bool v6 = v9 == 2 || !v9;
			bool v5 = v11 == 2 || !v11;
			bool v4 = v8 == 2 || !v8;
			bool v3 = v10 == 2 || !v10;
			if (v6) {
				if (v4) {
					a3 = 5;
				} else {
					if (v3) {
						a3 = 7;
					} else {
						a3 = 6;
					}
				}
			} else {
				if (v5) {
					if (v4) {
						a3 = 0;
					} else {
						if (v3) {
							a3 = 2;
						}
					}
				} else {
					if (v4) {
						a3 = 3;
					} else {
						if (v3) {
							a3 = 4;
						}
					}
				}
			}
		}
		int32_t Float = static_cast<int32_t>(pWheel->GetFloat(iUT_HighlightedHotkey));
		if (a3 == Float) {
			return a3;
		}
		if (a3 != inewKeyPressed) {
			inewKeyPressed = a3;
			ikeyPressStart = appTimer.GetTime();
		}
		if (!(static_cast<double>(appTimer.GetTime() - ikeyPressStart) > 50.0f) && (a3 != -1 && Float != -1)) {
			return a3;
		}
	} else {
		if (aiNewIndex >= 0) {
			a3 = aiNewIndex;
		}
	}
	HighlightHotkey(a3);
	return a3;
}

void HotKeysWheel::UseHotkeyItem() {
	ExtraDataList* pextra;
	ItemChange* pitem;
	InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pPlayer);
	if (pinventory) {
		pitem = pinventory->GetHotKeyItem(iLastHighlightedHotkey);
	} else {
		pitem = 0;
	}
	if (pitem && pitem->pExtraObjectList) {
		pextra = pitem->pExtraObjectList->m_item;
	} else {
		pextra = 0;
	}
	bool v9 = pitem &&
		(pitem->pContainerObj->GetFormType() == WEAP_ID &&
			static_cast<TESObjectWEAP*>(pitem->pContainerObj)->TESObjectWEAP::IsThrownWeapon());
	if (bSelectingHotkey) {
		if (pinventory) {
			if (pitem) {
				if ((pPlayer->GetAnimAction() == ANIM_ACTION_NONE || pitem->pContainerObj->GetFormType() != ARMO_ID) &&
					(pPlayer->GetAnimAction() == ANIM_ACTION_NONE || pitem->pContainerObj->GetFormType() != WEAP_ID)) {
					int32_t v2;
					TESBoundObject* FormFlags = pitem->pContainerObj;
					if (FormFlags == BGSDefaultObjectManager::GetDefaultObject(DO_STIMPAK) &&
						(v2 = pPlayer->GetActorValue(ActorValue::HEALTH),
							v2 == pPlayer->GetPermanentActorValue(ActorValue::HEALTH))) {
						const char* v3 = sFullHealth.operator const char*();
						Interface::ShowMessage(v3, 0, 0, 0, 2.0f, 0);
					} else {
						if (pitem->GetWorn(0)) {
							uint32_t aiCount;
							if (v9) {
								aiCount = pitem->iNumber;
							} else {
								aiCount = 1;
							}
							TESBoundObject* v5 = pitem->pContainerObj;
							pPlayer->QueueUnEquipObject(v5, aiCount, pextra, 0, 0, 1);
						} else {
							uint32_t count;
							if (v9) {
								count = pitem->iNumber;
							} else {
								count = 1;
							}
							TESBoundObject* v4 = pitem->pContainerObj;
							pPlayer->QueueEquipObject(v4, count, pextra, 0, 0, 1);
						}
						pPlayer->UpdatePlayer3d();
						BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
						CurrentAIProcess->SetShouldRefreshMagicShadersEffects(1, 0, 0);
						pPlayer->ResetArmorRating();
						delete pitem;
					}
				}
			}
		}
	}
}

void HotKeysWheel::SetHotkeyItem(ItemChange* apHotkeyItem) {
	InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(pPlayer);
	if (apHotkeyItem) {
		if (bSelectingHotkey) {
			if (InventoryChanges_) {
				ExtraDataList* v5;
				ItemChange* HotKeyItem = InventoryChanges_->GetHotKeyItem(iLastHighlightedHotkey);
				if (apHotkeyItem->pExtraObjectList) {
					v5 = apHotkeyItem->pExtraObjectList->m_item;
				} else {
					v5 = 0;
				}
				if (HotKeyItem) {
					InventoryChanges_->RemoveHotKeyItem(HotKeyItem, iLastHighlightedHotkey);
					if (HotKeyItem->pContainerObj) {
						TESModelTextureSwap* BipedTESModel_0 =
							TESBipedModelForm::GetBipedTESModel(HotKeyItem->pContainerObj, pPlayer);
						if (!BipedTESModel_0) {
							BipedTESModel_0 = dynamic_cast<TESModelTextureSwap*>(HotKeyItem->pContainerObj);
						}
						if (BipedTESModel_0) {
							const char* v2 = BipedTESModel_0->GetModel();
							pModelLoader->ReleaseFile(v2);
						}
					}
				}
				InventoryChanges_->SetHotKeyItem(apHotkeyItem, v5, iLastHighlightedHotkey);
				if (apHotkeyItem->pContainerObj) {
					TESModelTextureSwap* v7 = TESBipedModelForm::GetBipedTESModel(apHotkeyItem->pContainerObj, pPlayer);
					if (!v7) {
						v7 = dynamic_cast<TESModelTextureSwap*>(apHotkeyItem->pContainerObj);
					}
					if (v7) {
						const char* v3 = v7->GetModel();
						pModelLoader->LoadFile(v3, LOD_MULT_NONE, 1, 0, 0, 0);
					}
				}
				UpdateHotkeyList();
				TESBoundObject* FormFlags = apHotkeyItem->pContainerObj;
				pPlayer->PlayPickUpSound(FormFlags, 1, 0);
				bSelectingHotkey = 0;
				if (HotKeyItem) {
					delete HotKeyItem;
				}
			}
		}
	}
}

int32_t HotKeysWheel::QNumHotkeys() {
	return ciNumHotkeys;
}

void HotKeysWheel::HighlightHotkey(int32_t a3) {
	const char* src;
	pWheel->Set(iUT_HighlightedHotkey, a3);
	if ((a3 < 0) || (a3 == 1)) {
		pWheel->Set(iUT_HighlightedText, "", (bool)1);
	} else {
		src = pHotkeyTilesA[a3]->GetString(4036);
		pWheel->Set(iUT_HighlightedText, src, (bool)1);
	}
	if (a3 >= 0) {
		iLastHighlightedHotkey = a3;
		bSelectingHotkey = (bool)1;
	}
}

bool HotKeysWheel::IsDPadPressed() {
	return (bool)((uint8_t)(Interface::Has360Controller() &&
		((InterfaceManager::CheckMenuButton(Interface::eButtonDPadDown, 0) != ((-1))) ||
			((InterfaceManager::CheckMenuButton(Interface::eButtonDPadUp, 0) != ((-1))) ||
				((InterfaceManager::CheckMenuButton(Interface::eButtonDPadLeft, 0) != ((-1))) ||
					(InterfaceManager::CheckMenuButton(Interface::eButtonDPadRight, 0) != ((-1))))))));
}
