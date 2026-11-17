#include "Fallout/Interface/Menus/ContainerMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSAmmoForm.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/BarterMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/InventoryMenu.h"
#include "Fallout/Interface/Menus/QuantityMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/tchar.h"

ContainerMenu::ContainerMenu()
	: Menu(), iValueStolen(0), bPickpocketFail(false), bPickPocketTallied(false), eItemsFilterMode(FM_ALL),
	  eContainerFilterMode(FM_ALL), eFilterSound(MENU_SOUND_NEXT_PREVIOUS), plbCurrListbox(nullptr),
	  m_hCurrentReactivityBark(0u) {
	ContainerMenu::pCurrItem = nullptr;
}

ContainerMenu::~ContainerMenu() {
	ContainerMenu::pMe = nullptr;
}

void ContainerMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
	if (aiID >= 0 && aiID < 19) {
		pTiles[aiID] = apTile;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ContainerMenu::DoClick(int32_t aiID, Tile* apTarget) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	if (!apTarget && aiID >= 0 && aiID < 19) {
		apTarget = pTiles[aiID];
	}
	switch (aiID) {
	case 0:
	case 1:
	case 2: {
		Interface::PlayMenuSound(eFilterSound);
		do {
			eItemsFilterMode = static_cast<ContainerMenu::FILTER_MODE>(eItemsFilterMode + 2 * (aiID != 0) - 1);
			if (eItemsFilterMode >= ContainerMenu::FM_ALL) {
				if (eItemsFilterMode > ContainerMenu::FM_AMMO) {
					eItemsFilterMode = ContainerMenu::FM_ALL;
				}
			} else {
				eItemsFilterMode = ContainerMenu::FM_AMMO;
			}
			SetFilterModeTitle(pTiles[1]);
			lbItems.Filter(&ContainerMenu::FilterModeFunc);
		} while (!lbItems.NumUnfiltered() && eItemsFilterMode);
		break;
	}
	case 5:
	case 6:
	case 7: {
		Interface::PlayMenuSound(eFilterSound);
		do {
			eContainerFilterMode = static_cast<ContainerMenu::FILTER_MODE>(eContainerFilterMode + 2 * (aiID != 5) - 1);
			if (eContainerFilterMode >= ContainerMenu::FM_ALL) {
				if (eContainerFilterMode > ContainerMenu::FM_AMMO) {
					eContainerFilterMode = ContainerMenu::FM_ALL;
				}
			} else {
				eContainerFilterMode = ContainerMenu::FM_AMMO;
			}
			SetFilterModeTitle(pTiles[6]);
			lbContainer.Filter(&ContainerMenu::FilterModeFunc);
		} while (!lbContainer.NumUnfiltered() && eContainerFilterMode);
		break;
	}
	case 10: {
		lbContainer.Traverse(&ContainerMenu::TakeAllFn, 0, 0x7FFFFFFF);
		BSAudio* v7 = &BSAudio::QInstance();
		BSSoundHandle* SoundHandleByName = v7->GetSoundHandleByName(&a1, "UIItemTakeAll", 289u);
		SoundHandleByName->Play(0);
		ContainerMenu::Close();
		break;
	}
	case 11: {
		ContainerMenu::Close();
		break;
	}
	case 20: {
		if (ContainerMenu::pCurrItem) {
			bool IsHardcore;
			bMouseOverEnabled = 0;
			bool v9 =
				(TESContainer::IsGold(ContainerMenu::pCurrItem->pContainerObj) ||
					(IsHardcore = pPlayer->IsHardcore(),
						TESWeightForm::GetFormWeight(ContainerMenu::pCurrItem->pContainerObj, IsHardcore) <= 0.0)) &&
				((eContainerMode == Interface::CMM_STANDARD || eContainerMode == Interface::CMM_PICKPOCKET) &&
					plbCurrListbox == &lbContainer);
			int32_t FormHealth_0 = ContainerMenu::pCurrItem->iNumber;
			if (FormHealth_0 <= iInventoryAskQuantityAt.Int() ||
				(IsPlacingMine(ContainerMenu::pCurrItem, pPlayer, pContainerRef) || v9)) {
				if (v9) {
					uint32_t v6 = ContainerMenu::pCurrItem->iNumber;
					ContainerMenu::ItemSelectCallback(v6);
				} else {
					ContainerMenu::ItemSelectCallback(1);
				}
			} else {
				plbCurrListbox->SaveListState();
				plbCurrListbox->Highlight(0);
				uint32_t v5 = ContainerMenu::pCurrItem->iNumber;
				QuantityMenu::Create(v5, &ContainerMenu::ItemSelectCallback, 0x7FFFFFFF);
			}
		}
		break;
	}
	default: {
		return;
	}
	}
}

void ContainerMenu::DoEnter(int32_t aiID, Tile* apTarget) {
	if (!apTarget && aiID >= 0 && aiID < 19) {
		apTarget = pTiles[aiID];
	}
	switch (aiID) {
	case 20: {
		if (apTarget) {
			bMouseOverEnabled = 1;
			if (plbCurrListbox == &lbItems || apTarget->pParent != pTiles[4]) {
				if (plbCurrListbox != &lbContainer && apTarget->pParent == pTiles[8]) {
					DoEnter(22, 0);
				}
			} else {
				DoEnter(21, 0);
			}
			ContainerMenu::pCurrItem = plbCurrListbox->GetHighlightedData();
			if (ContainerMenu::pCurrItem) {
				InventoryMenu::SetupItemDataTile(ContainerMenu::pCurrItem, pTiles[13]);
				pTiles[13]->Set(4003, 1);
				const char* IconOne = ContainerMenu::pCurrItem->GetIconOne(0);
				pTiles[12]->Set(4044, IconOne, 1);
				pTiles[12]->Set(4003, 1);
				pTiles[9]->Set(Tile::kTileValue_User0, (plbCurrListbox != &lbItems) + 1);
				pTiles[9]->Set(4003, 1);
			}
		}
		break;
	}
	case 21: {
		DoLeave(20, 0);
		// The exe calls Highlight through the list's vtable (slot 0).
		static_cast<ListBoxBase*>(&lbContainer)->Highlight(nullptr);
		plbCurrListbox = &lbItems;
		break;
	}
	case 22: {
		DoLeave(20, 0);
		static_cast<ListBoxBase*>(&lbItems)->Highlight(nullptr);
		plbCurrListbox = &lbContainer;
		break;
	}
	}
}

void ContainerMenu::DoIdle() {
	if (m_nTimeToClearSubtitle) {
		if (BSGetTickCount() > m_nTimeToClearSubtitle) {
			m_nTimeToClearSubtitle = 0;
			Tile* pTile = Tile::GetTileByName(ContainerMenu::pMe->pMenu, "CM_Subtitle");
			if (pTile) {
				pTile->Set(4036, "", 1);
			}
		}
	}
	if (lbContainer.NumItems()) {
		if (bMouseOverEnabled) {
			if (ContainerMenu::pCurrItem && ContainerMenu::pCurrItem->pContainerObj->GetFormType() == WEAP_ID) {
				InterfaceManager::UpdatePipBoyInventoryEffects(pTiles[TI_DAM], pTiles[TI_DPS], pTiles[TI_STRREQ],
					ContainerMenu::pCurrItem, 0);
			} else {
				if (ContainerMenu::pCurrItem) {
					if (ContainerMenu::pCurrItem->pContainerObj->GetFormType() == ARMO_ID) {
						Tile* pTileFade2;
						if (!ContainerMenu::pCurrItem ||
							static_cast<TESObjectARMO*>(ContainerMenu::pCurrItem->pContainerObj)
									->TESObjectARMO::GetDamageThreshold() == 0.0) {
							pTileFade2 = 0;
						} else {
							pTileFade2 = pTiles[TI_DT];
						}
						if (!ContainerMenu::pCurrItem ||
							static_cast<TESObjectARMO*>(ContainerMenu::pCurrItem->pContainerObj)
									->TESObjectARMO::GetArmorRating() == 0.0) {
							InterfaceManager::UpdatePipBoyInventoryEffects(0, pTileFade2, 0, ContainerMenu::pCurrItem,
								0);
						} else {
							InterfaceManager::UpdatePipBoyInventoryEffects(pTiles[TI_DR], pTileFade2, 0,
								ContainerMenu::pCurrItem, 0);
						}
					}
				}
			}
		}
	}
}

bool ContainerMenu::DoGamepad(int32_t aiButton, float afValue) {
	bool bresult = 0;
	if (Interface::IsTopMenuID(Interface::eContainerMenu)) {
		switch (aiButton) {
		case 3:
		case 4: {
			if (eContainerMode != 4) {
				if (plbCurrListbox == &lbItems) {
					lbItems.SaveListState();
					DoClick(aiButton != 4 ? 2 : 0, 0);
					lbItems.RestoreListState(0);
				} else {
					lbContainer.SaveListState();
					DoClick(2 * (aiButton != 4) + 5, 0);
					lbContainer.RestoreListState(0);
				}
				bresult = 1;
			}
			break;
		}
		case 13: {
			if (!lbItems.GetHighlightedItem()) {
				if (lbItems.NumItems()) {
					lbContainer.SaveListState();
					DoEnter(21, 0);
					lbItems.RestoreListState(1);
					bresult = 1;
				}
			}
			break;
		}
		case 14: {
			if (!lbContainer.GetHighlightedItem()) {
				if (lbContainer.NumItems()) {
					lbItems.SaveListState();
					DoEnter(22, 0);
					lbContainer.RestoreListState(1);
					bresult = 1;
				}
			}
			break;
		}
		default: {
			return bresult;
		}
		}
	}
	return bresult;
}

bool ContainerMenu::DoKey(int32_t aiKey) {
	uint8_t v3 = 0;
	if (aiKey == 32) {
		ContainerMenu::Close();
		return (bool)1;
	}
	return (bool)v3;
}

void ContainerMenu::DoLeave(int32_t aiID, Tile* apTarget) {
	pTiles[TI_ITEM_INFO]->Tile::Set(4003, 0);
	pTiles[TI_ITEM_ICON]->Tile::Set(4003, 0);
	pTiles[TI_ARROW_ICON]->Tile::Set(4003, 0);
}

bool ContainerMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
	int8_t v4 = 0;
	if ((aiButton != 11) || ((eContainerMode == 4) || ((eContainerMode == 3) || (xFadeState != 1)))) {
		if (aiButton == 10) {
			ContainerMenu::Close();
			return (bool)1;
		}
	} else {
		DoClick(10, nullptr);
		return (bool)1;
	}
	return (bool)v4;
}

void ContainerMenu::DoWheelMove(int32_t aiID, Tile* apTarget) {
	InterfaceManager* singleton;
	if ((aiID == 1) || (aiID == 6)) {
		singleton = InterfaceManager::Instance();
		if (singleton->GetMouseWheel() >= 0) {
			DoClick((aiID != 1) ? 7 : 2, nullptr);
		} else {
			DoClick((aiID != 1) ? 5 : 0, nullptr);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ContainerMenu::UpdateLists(TESBoundObject* apUpdateObj) {
	FNVD_INLINED_LOCAL(BSStringT<char>, strtemp, std::nullopt, std::nullopt);
	float fMaxWeight;
	float fCurrWeight;
	char Src[68];
	int32_t pnewHighlight = 0;
	if (plbCurrListbox) {
		if (plbCurrListbox->GetHighlightedItem()) {
			plbCurrListbox->SaveListState();
		}
	}
	if (eContainerMode == Interface::CMM_ROCK_N_LOAD) {
		BSSimpleArray<ItemChange*, 1024>* prockItLauncherItems = pPlayer->GetRockItAmmoArray();
		// The exe calls RemoveAll (slot 7) and Sort (slot 8) through the lists' vtables; calls on the members
		// themselves would be devirtualised, so they go through pointers.
		ListBox<ItemChange*>* pContainerList = &lbContainer;
		pContainerList->RemoveAll();
		for (int32_t iitemCt = 0; iitemCt < prockItLauncherItems->iSize; ++iitemCt) {
			const char* FullName = (*prockItLauncherItems)[iitemCt]->GetFullName();
			lbContainer.AddItem((*prockItLauncherItems)[iitemCt], FullName, 0, 0);
		}
		if (apUpdateObj) {
			InventoryChanges* pplayerInventory = InventoryChanges::GetInventoryChanges(pPlayer);
			UpdateItemInList(apUpdateObj, pplayerInventory, &lbItems);
		} else {
			lbItems.BuildFromContainer(pPlayer, &ContainerMenu::FirstPassFilterFunc, 0, 0, 0, 0);
		}
	} else {
		if (apUpdateObj) {
			TESAmmo* pammoObj;
			InventoryChanges* pcontainerInventory;
			InventoryChanges* pplayerInventory_2 = InventoryChanges::GetInventoryChanges(pPlayer);
			if (pContainerRef) {
				pcontainerInventory = InventoryChanges::GetInventoryChanges(pContainerRef);
			} else {
				pcontainerInventory = 0;
			}
			if (apUpdateObj->GetFormType() == WEAP_ID) {
				TESAmmo* pammoObj_2;
				if (pContainerRef->IsActor()) {
					pammoObj_2 = static_cast<TESObjectWEAP*>(apUpdateObj)
									 ->TESObjectWEAP::GetCurrentAmmo(static_cast<Actor*>(pContainerRef));
				} else {
					pammoObj_2 = static_cast<TESObjectWEAP*>(apUpdateObj)->TESObjectWEAP::GetCurrentAmmo(0);
				}
				pammoObj = pammoObj_2;
			} else {
				pammoObj = 0;
			}
			UpdateItemInList(apUpdateObj, pplayerInventory_2, &lbItems);
			if (pammoObj) {
				UpdateItemInList(pammoObj, pplayerInventory_2, &lbItems);
			}
			if (eContainerMode == Interface::CMM_TEAMMATE) {
				lbContainer.BuildFromContainer(pContainerRef, &ContainerMenu::FirstPassFilterFunc, 0, 0, 0, 0);
			} else {
				UpdateItemInList(apUpdateObj, pcontainerInventory, &lbContainer);
				if (pammoObj) {
					UpdateItemInList(pammoObj, pcontainerInventory, &lbContainer);
				}
			}
		} else {
			lbItems.BuildFromContainer(pPlayer, &ContainerMenu::FirstPassFilterFunc, 0, 0, 0, 0);
			lbContainer.BuildFromContainer(pContainerRef, &ContainerMenu::FirstPassFilterFunc, 0, 0, 0, 0);
		}
	}
	for (TESBoundObject*& rpItem : ActiveMineList.ItemsUntilNull()) {
		if (!apUpdateObj || apUpdateObj == rpItem) {
			ItemChange* pnewItem;
			ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
			if (apMemory) {
				pnewItem = ::new (apMemory) ItemChange(rpItem, 1);
			} else {
				pnewItem = 0;
			}
			ItemChange* aAssociatedData = pnewItem;
			::new (&strtemp) BSStringT<char>;
			const char* afHealth = pnewItem->GetFullName();
			strtemp.SPrintF("%s %s", sActiveMineDescription.String(), afHealth);
			Tile* pmineTile = lbContainer.AddItem(pnewItem, strtemp.pString, 0, 0);
			pmineTile->xChildren.GetTail()->Set(4009, 128.0f, 1);
			strtemp.~BSStringT<char>();
		}
	}
	if (pContainerRef) {
		if (pContainerRef->IsActor() && pContainerRef->IsDead(0)) {
			TESObjectREFR* pref = (&pContainerRef->m_Extra)->GetDroppedWeapon();
			if (pref) {
				if (!pref->GetDisabledOrDeleted() && (!apUpdateObj || apUpdateObj == pref->GetObjectReference())) {
					ItemChange* pnewItem_2;
					ItemChange* v23 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
					if (v23) {
						pnewItem_2 = ::new (v23) ItemChange(pref->GetObjectReference(), 1);
					} else {
						pnewItem_2 = 0;
					}
					const char* v5 = pnewItem_2->GetFullName();
					Tile* pnewTile = lbContainer.AddItem(pnewItem_2, v5, 0, 0);
					if (pnewItem_2->pExtraObjectList) {
						BSSimpleList<ExtraDataList*>* v15;
						BSSimpleList<ExtraDataList*>* v22 =
							static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
						if (v22) {
							v15 = ::new (v22) BSSimpleList<ExtraDataList*>;
						} else {
							v15 = 0;
						}
						pnewItem_2->pExtraObjectList = v15;
					}
					if (!pnewItem_2->pExtraObjectList->m_item) {
						ExtraDataList* pextra;
						ExtraDataList* v21 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
						if (v21) {
							pextra = ::new (v21) ExtraDataList;
						} else {
							pextra = 0;
						}
						ExtraDataList* pextra_2 = pextra;
						pnewItem_2->pExtraObjectList->AddHead(pextra_2);
					}
					pnewItem_2->pExtraObjectList->m_item->SetItemDropper(pref);
					float afHealtha = pref->GetHealth();
					pnewItem_2->pExtraObjectList->m_item->SetHealth(afHealtha);
				}
			}
		}
	}
	lbItems.Traverse(&ContainerMenu::InitFunc, 0, 0x7FFFFFFF);
	lbItems.Filter(&ContainerMenu::FilterModeFunc);
	ListBox<ItemChange*>* pItemList = &lbItems;
	pItemList->Sort(&ContainerMenu::SortFunc);
	lbContainer.Traverse(&ContainerMenu::InitFunc, 0, 0x7FFFFFFF);
	lbContainer.Filter(&ContainerMenu::FilterModeFunc);
	ListBox<ItemChange*>* pContainerList = &lbContainer;
	pContainerList->Sort(&ContainerMenu::SortFunc);
	pPlayer->GetInventoryEncumbrance(fCurrWeight, fMaxWeight);
	float afHealthb = BSRoundToNearest(fMaxWeight, 1.0f);
	int32_t afHealthc = FloatToInt(afHealthb);
	float afVal = BSRoundToNearest(fCurrWeight, 1.0f);
	int32_t afVala = FloatToInt(afVal);
	BSsprintf(Src, 64, "%s   %d/%d", sInventoryWeight.String(), afVala, afHealthc);
	ContainerMenu::pMe->pTiles[3]->Set(4036, Src, 1);
	if (!plbCurrListbox && InterfaceManager::Has360Controller()) {
		plbCurrListbox = &lbContainer;
	}
	if (plbCurrListbox) {
		if (plbCurrListbox->NumUnfiltered() ||
			(eFilterSound = MENU_SOUND_NONE,
				(DoClick(&lbItems != plbCurrListbox ? 7 : 2, 0),
					(eFilterSound = MENU_SOUND_NEXT_PREVIOUS, plbCurrListbox->NumUnfiltered())))) {
			plbCurrListbox->RestoreListState(1);
		} else {
			DoEnter((plbCurrListbox == &lbItems) + 21, 0);
			if (InterfaceManager::Has360Controller()) {
				Tile* FirstItem = plbCurrListbox->GetFirstItem();
				InterfaceManager* v6 = InterfaceManager::Instance();
				v6->SetCurrentFocusTarget(FirstItem, 4035, 1);
			}
		}
	}
}

int32_t ContainerMenu::GetStolenValue(ItemChange* a1, int32_t a2, bool a3) {
	TESForm* powner;
	float someFloat = a1->GetItemValue();
	int32_t iitemValue = FloatToInt(someFloat);
	int32_t ireturnVal = 0;
	if (pContainerRef->IsActor()) {
		powner = (TESForm*)pContainerRef;
	} else {
		powner = pContainerRef->GetOwner();
	}
	for (BSSimpleList<ExtraDataList*>* pextraObj = a1->pExtraObjectList; a2 > 0 && (pextraObj && pextraObj->m_item);
		pextraObj = pextraObj->m_pkNext) {
		TESForm* pextraOwner = pextraObj->m_item->GetOwner();
		if ((!pextraOwner && a3) || (pextraOwner && pextraObj->m_item->GetOwner() == powner)) {
			ireturnVal += iitemValue * pextraObj->m_item->GetCount();
		}
		a2 -= pextraObj->m_item->GetCount();
	}
	if (a3) {
		ireturnVal += iitemValue * a2;
	}
	return ireturnVal;
}

// The exe's cleanup-only C++ EH frame is left out.
void ContainerMenu::PlayReactivityBark(TESTopic* aRelatedTopic) {
	FNVD_INLINED_LOCAL(BSStringT<char>, v10, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v11, std::nullopt, std::nullopt);
	Actor* pactor;
	FNVD_INLINED_LOCAL(BSSoundHandle, a2, std::nullopt, std::nullopt);
	TESTopicInfo* pInfo = 0;
	Actor* pActionActor = (Actor*)pPlayer;
	if (ContainerMenu::pMe->pContainerRef && ContainerMenu::pMe->pContainerRef->IsActor()) {
		pactor = dynamic_cast<Actor*>(ContainerMenu::pMe->pContainerRef);
	} else {
		pactor = 0;
	}
	if (aRelatedTopic) {
		pInfo = aRelatedTopic->GetMatchingInfo(pactor, pActionActor);
	}
	if (pInfo) {
		FNVD_CONSTRUCTED_LOCAL(DialogueItem, Exchange, &DialogueItem::Destruct, pInfo->pOwnerQuest, aRelatedTopic,
			pInfo, pactor);
		Exchange.FirstResponse();
		DialogueResponse* pResponse = Exchange.GetCurrentResponse();
		if (pResponse) {
			if (pResponse->pVoiceSound) {
				uint32_t isoundFileID = pResponse->pVoiceSound->iFormID;
				BSSoundHandle* v16 = pactor->PlaySound(&a2, isoundFileID, 0, 258u, 1);
				this->m_hCurrentReactivityBark.operator=(*v16);
			} else {
				PlayerCharacter* v9 = pPlayer;
				TESIdleForm* ListenerIdle = pResponse->pListenIdle;
				TESIdleForm* SpeakerIdle = pResponse->pSpeakerIdle;
				uint32_t Length = pResponse->cText.GetLength();
				uint32_t CastingType = pResponse->uiEmotion;
				DIALOGUE_EMOTION FormFlags = pResponse->eEmotion;
				char* v3 = pResponse->GetVoice()->pString;
				float fdelay = pactor->SpeakSoundFunction(v3, this->m_hCurrentReactivityBark, FormFlags, CastingType,
					Length, SpeakerIdle, ListenerIdle, v9, 1, 1, 0, 0, 1);
			}
			BSSoundHandle* v20 = &v11;
			BSSoundHandle* v14 = &v11.operator=(this->m_hCurrentReactivityBark);
			BSStringT<char>* Text = &pResponse->cText;
			BSStringT<char>* v19 = &v10;
			BSStringT<char>* v12 = ::new (&v10) BSStringT<char>(*Text);
			this->DisplayContainerMenuSubtitle(v10, v11);
			// The exe builds the by-value argument in place and the callee releases it; natively C++ passes a copy
			// of this raw-storage local, so the local is destroyed here.
			v10.~BSStringT<char>();
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out. The duration is a float the exe keeps in memory ([ebp-0x10]):
// volatile, so that the sound's integer duration is rounded to a float before the sum, as the exe's fstp dword does.
void ContainerMenu::DisplayContainerMenuSubtitle(BSStringT<char> asText, BSSoundHandle ahSound) {
	Actor* v4;
	if (ContainerMenu::pMe->pContainerRef && ContainerMenu::pMe->pContainerRef->IsActor()) {
		v4 = dynamic_cast<Actor*>(ContainerMenu::pMe->pContainerRef);
	} else {
		v4 = 0;
	}
	if (asText.GetLength()) {
		if (gs_bGeneralSubtitles_Gameplay.Bool() || (v4 && static_cast<uint8_t>(v4->GetShowSubtitles()))) {
			Tile* pTile = Tile::GetTileByName(ContainerMenu::pMe->pMenu, "CM_Subtitle");
			if (pTile) {
				pTile->Set(Tile::kTileValue_String, asText.pString, 1);
			}
			float fSubtitleDuration = static_cast<float>(ahSound.GetDuration());
			if (fSubtitleDuration == 0.0) {
				double v3 = static_cast<double>(std::strlen(asText.pString));
				fSubtitleDuration = fNoticeTextTimePerCharacter.Float() * v3 * 1000.0;
			}
			m_nTimeToClearSubtitle = static_cast<int64_t>(static_cast<double>(BSGetTickCount()) + fSubtitleDuration);
		}
	}
}

bool ContainerMenu::PickpocketCheck(Actor* apActor, int32_t aiStealValue) {
	bool bResult = true;
	if (!apActor || aiStealValue <= 0 || apActor->IsKnockedOut()) {
		return bResult;
	}
	int32_t iTargetSneak = static_cast<ActorValueOwner*>(apActor)->GetActorValue(ActorValue::SNEAK);
	int32_t iPlayerSneak = static_cast<ActorValueOwner*>(pPlayer)->GetClampedActorValue(ActorValue::SNEAK);
	uint32_t uiChance =
		AiFormulas::ComputePickpocketSuccess(iPlayerSneak, iTargetSneak, static_cast<float>(aiStealValue));
	pPlayer->UseSkill(ActorValue::SNEAK, 1);
	if (BSRandom::GetSingleton()->UnsignedInt(100) < uiChance) {
		if (plbCurrListbox == &lbContainer && !bPickpocketFail) {
			MiscStatManager::Increment(MiscStatManager::MS_POCKETS_PICKED);
			bPickpocketFail = true;
		}
	} else {
		pPlayer->PickpocketAlarm(apActor, nullptr, 0);
		Interface::ShowMessage(sPickpocketFail.String(), 2, nullptr, nullptr, 2.0f, false);
		bResult = false;
		bPickPocketTallied = true;
	}
	// The exe calls GetActorBase twice and drops the first result.
	apActor->GetActorBase();
	ActorValueOwner* pBaseValues = apActor->GetActorBase();
	TESActorBaseData::ALIGNMENT eAlignment =
		TESActorBaseData::GetAlignmentForKarma(pBaseValues->GetActorFloatValue(ActorValue::KARMA));
	if (eAlignment == TESActorBaseData::ALIGNMENT_EVIL || eAlignment == TESActorBaseData::ALIGNMENT_VERY_EVIL) {
		return bResult;
	}
	pPlayer->RewardKarma(static_cast<int32_t>(fKarmaModStealing.Float()));
	return bResult;
}

bool ContainerMenu::IsWellFormedMenu() {
	for (int32_t i = 0; i < 19; ++i) {
		if (!pTiles[i]) {
			return 0;
		}
	}
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out.
void ContainerMenu::PlayContainerSound(TESObjectREFR* apContainer, bool abOpenSound) {
	FNVD_INLINED_LOCAL(BSSoundHandle, out, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	TESSound* psound = 0;
	if (apContainer) {
		int32_t FormType = apContainer->GetObjectReference()->GetFormType();
		if (FormType == 27) {
			if (abOpenSound) {
				psound = static_cast<TESObjectCONT*>(apContainer->GetObjectReference())->TESObjectCONT::GetOpenSound();
			} else {
				psound = static_cast<TESObjectCONT*>(apContainer->GetObjectReference())->TESObjectCONT::GetCloseSound();
			}
		} else {
			if (FormType > 41 && FormType <= 43) {
				if (abOpenSound) {
					BSAudio* v3 = &BSAudio::QInstance();
					BSSoundHandle* SoundHandleByName = v3->GetSoundHandleByName(&a1, "DRSBodyGenericOpen", 289u);
					SoundHandleByName->Play(0);
				} else {
					BSAudio* v4 = &BSAudio::QInstance();
					BSSoundHandle* v9 = v4->GetSoundHandleByName(&arHandle, "DRSBodyGenericClose", 289u);
					v9->Play(0);
				}
			}
		}
		if (psound) {
			const char* Filename = psound->GetFilename();
			BSAudio* v5 = &BSAudio::QInstance();
			BSSoundHandle* SoundHandleByFilename = v5->GetSoundHandleByFilename(&out, Filename, 289u, psound);
			SoundHandleByFilename->Play(0);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ContainerMenu::InitContainerMode() {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	switch (eContainerMode) {
	case Interface::CMM_STANDARD:
	case Interface::CMM_PICKPOCKET: {
		if (pContainerRef) {
			const char* FullName = pContainerRef->GetFullName();
			SetContainerTitle(FullName);
			PlayContainerSound(pContainerRef, 1);
		}
		break;
	}
	case Interface::CMM_TEAMMATE: {
		if (pContainerRef) {
			const char* v2 = pContainerRef->GetFullName();
			SetContainerTitle(v2);
			BSAudio* v3 = &BSAudio::QInstance();
			BSSoundHandle* SoundHandleByName = v3->GetSoundHandleByName(&a1, "DRSTraderOpen", 289u);
			SoundHandleByName->Play(0);
			DialogMenu::ServiceStart();
			TESForm* FormByEditorID = TESForm::GetFormByEditorID("FollowersTrade");
			TESTopic* pTopic = dynamic_cast<TESTopic*>(FormByEditorID);
			ContainerMenu::pMe->PlayReactivityBark(pTopic);
		}
		pTiles[TI_TAKE_ALL_BTN]->Set(4003, 0);
		break;
	}
	case Interface::CMM_ROCK_N_LOAD: {
		const char* v5 = sJunk.String();
		pTiles[TI_ITEMS_TITLE]->Set(4036, v5, 1);
		if (pPlayer->GetCurrentWeapon()) {
			TESObjectWEAP* CurrentWeapon = pPlayer->GetCurrentWeapon();
			SetContainerTitle(CurrentWeapon->GetFullName());
		}
		pTiles[TI_TAKE_ALL_BTN]->Set(4003, 0);
		pTiles[TI_ITEMS_FILTER_LEFT]->Set(4003, 0);
		pTiles[TI_ITEMS_FILTER_RIGHT]->Set(4003, 0);
		pTiles[TI_CONTAINER_FILTER_LEFT]->Set(4003, 0);
		pTiles[TI_CONTAINER_FILTER_RIGHT]->Set(4003, 0);
		break;
	}
	default: {
		return;
	}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ContainerMenu::UpdateItemInList(TESBoundObject* apObj, InventoryChanges* apInventory,
	ListBox<ItemChange*>* apListbox) {
	FNVD_INLINED_LOCAL(BSSimpleList<ItemChange*>, pnewItemGroup, std::nullopt, std::nullopt);
	::new (&pnewItemGroup) BSSimpleList<void*>;
	if (apInventory) {
		if (apObj) {
			if (apListbox) {
				apInventory->GetItemGroup(apObj, &pnewItemGroup);
				apListbox->RemoveItemGroup(apObj);
				for (ItemChange*& rpItem : pnewItemGroup.ItemsUntilNull()) {
					const char* FullName = rpItem->GetFullName();
					apListbox->AddItem(rpItem, FullName, 0, 0);
				}
				pnewItemGroup.RemoveAll();
			}
		}
	}
	pnewItemGroup.~BSSimpleList<ItemChange*>();
}

bool ContainerMenu::IsPlacingMine(ItemChange* apItem, TESObjectREFR* apRefFrom, TESObjectREFR* apRefTo) {
	BGSProjectile* Projectile_;
	if (eContainerMode != Interface::CMM_PICKPOCKET ||
		(plbCurrListbox != &lbItems ||
			(apRefFrom != pPlayer ||
				(!pPlayer->IsSneaking() ||
					(!apRefTo->IsActor() ||
						(apRefTo->IsDead(0) ||
							(apRefTo->IsChild(0) ||
								(apItem->pContainerObj->GetFormType() != WEAP_ID || !ActiveMineList.IsEmpty())))))))) {
		return 0;
	}
	Actor* ptoActor = 0;
	if (apRefTo->IsActor()) {
		ptoActor = static_cast<Actor*>(apRefTo);
		if (static_cast<Actor*>(apRefTo)->Actor::GetEssential()) {
			return 0;
		}
		if (static_cast<Actor*>(apRefTo)->GetCurrentAIProcess()) {
			BaseProcess* pprocess = static_cast<Actor*>(apRefTo)->GetCurrentAIProcess();
			if (pprocess->GetPlantedExplosive()) {
				return 0;
			}
		}
	}
	TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(apItem->pContainerObj);
	if (pweapon->IsThrownWeapon() &&
		(pweapon->GetProjectile(ptoActor) &&
			(Projectile_ = pweapon->GetProjectile(ptoActor),
				Projectile_->GetMotionType() == BGSProjectile::MOTION_GRENADE))) {
		return 1;
	} else {
		return 0;
	}
}

void ContainerMenu::SwitchContainer(Tile* apTile, ItemChange* apItem, int32_t aiAmountSwitched) {
	if (apItem) {
		if (apTile) {
			ExtraDataList* pextraFrom;
			TESObjectREFR* prefFrom;
			PlayerCharacter* prefTo;
			if (plbCurrListbox == &lbItems) {
				prefTo = static_cast<PlayerCharacter*>(pContainerRef);
			} else {
				prefTo = pPlayer;
			}
			TESObjectREFR* prefTo_2 = (TESObjectREFR*)prefTo;
			if (plbCurrListbox == &lbItems) {
				prefFrom = (TESObjectREFR*)pPlayer;
			} else {
				prefFrom = pContainerRef;
			}
			bool bisPlacingMine = IsPlacingMine(apItem, prefFrom, prefTo_2);
			if (apItem->pExtraObjectList) {
				pextraFrom = apItem->pExtraObjectList->m_item;
			} else {
				pextraFrom = 0;
			}
			if (bisPlacingMine) {
				TESObjectWEAP* FormFlags = static_cast<TESObjectWEAP*>(apItem->pContainerObj);
				BGSProjectile* Projectile_ =
					static_cast<TESObjectWEAP*>(apItem->pContainerObj)->TESObjectWEAP::GetProjectile(0);
				BGSProjectile::InitLiveGrenade(Projectile_, static_cast<Actor*>(prefFrom), prefTo_2, FormFlags);
				TESBoundObject* a2 = apItem->pContainerObj;
				ActiveMineList.AddHead(a2);
				TESBoundObject* v8 = apItem->pContainerObj;
				prefFrom->RemoveItem(v8, pextraFrom, aiAmountSwitched, 0, 0, 0, 0, 0, 1, 0);
			} else {
				TESObjectREFR* v5;
				TESObjectREFR* v6;
				if (!pextraFrom ||
					(!pextraFrom->GetItemDropper() ||
						((v5 = pextraFrom->GetItemDropper(), v5->GetDisabled()) ||
							(v6 = pextraFrom->GetItemDropper(), v6->GetDelete())))) {
					if (eContainerMode == Interface::CMM_ROCK_N_LOAD) {
						int32_t iswitchCounter = aiAmountSwitched;
						BSSimpleList<ExtraDataList*>* pextraObj_2 = apItem->pExtraObjectList;
						while (iswitchCounter > 0) {
							int32_t iamountToSwitch;
							ExtraDataList* pextra;
							if (pextraObj_2) {
								pextra = pextraObj_2->m_item;
							} else {
								pextra = 0;
							}
							ExtraDataList* pextra_2 = pextra;
							if (pextra) {
								iamountToSwitch = pextra_2->GetCount();
							} else {
								iamountToSwitch = iswitchCounter;
							}
							int32_t iamountToSwitch_2 = BSMIN(iamountToSwitch, iswitchCounter);
							if (prefTo_2 == pPlayer) {
								ExtraDataList* m_item;
								TESBoundObject* v10 = apItem->pContainerObj;
								ItemChange* premovedItem =
									pPlayer->RemoveRockItAmmo(v10, pextra_2, iamountToSwitch_2, 1);
								if (premovedItem->pExtraObjectList) {
									m_item = premovedItem->pExtraObjectList->m_item;
								} else {
									m_item = 0;
								}
								uint32_t FormHealth_0 = premovedItem->iNumber;
								TESBoundObject* v11 = premovedItem->pContainerObj;
								pPlayer->AddObjecttoContainer(v11, m_item, FormHealth_0);
								if (premovedItem) {
									delete premovedItem;
								}
								pextraObj_2 = apItem->pExtraObjectList;
							} else {
								if (prefFrom == pPlayer) {
									BSSimpleList<ExtraDataList*>* pextraObj;
									TESBoundObject* v12 = apItem->pContainerObj;
									pPlayer->AddRockItAmmo(v12, pextra_2, iamountToSwitch_2);
									prefFrom->RemoveItem(apItem->pContainerObj, pextra_2, iamountToSwitch_2, 0, 0, 0, 0,
										0, 1, 0);
									if (pextraObj_2) {
										pextraObj = pextraObj_2->m_pkNext;
									} else {
										pextraObj = 0;
									}
									pextraObj_2 = pextraObj;
								}
							}
							iswitchCounter -= iamountToSwitch_2;
						}
					} else {
						prefFrom->RemoveItem(apItem->pContainerObj, pextraFrom, aiAmountSwitched,
							eContainerMode == Interface::CMM_PICKPOCKET, 0, prefTo_2, 0, 0, 1, 0);
					}
				} else {
					TESObjectREFR* ItemDropper = pextraFrom->GetItemDropper();
					pPlayer->PickUpObject(ItemDropper, 1, 0);
				}
			}
		}
	}
}

void ContainerMenu::SetFilterModeTitle(Tile* apTitleTile) {
	int32_t eItemsFilterMode;
	if (apTitleTile == pTiles[TI_ITEMS_TITLE]) {
		eItemsFilterMode = this->eItemsFilterMode;
	} else {
		eItemsFilterMode = eContainerFilterMode;
	}
	const char* v2 = sLanguage.String();
	bool v12 = BSstrcmp(v2, "ENGLISH") != 0;
	if (pContainerRef) {
		if (apTitleTile) {
			char Dst[268];
			switch (eItemsFilterMode) {
			case 0: {
				if (apTitleTile == pTiles[TI_ITEMS_TITLE]) {
					const char* v3 = sInventoryItems.String();
					BSstrcpy(Dst, 260, v3);
					if (!v12) {
						BSstrupr(Dst, 260);
					}
					apTitleTile->Set(4036, Dst, static_cast<bool>(1));
				} else {
					const char* FullName = pContainerRef->GetFullName();
					SetContainerTitle(FullName);
				}
				break;
			}
			case 1: {
				const char* v5 = sInventoryWeapons.String();
				BSstrcpy(Dst, 260, v5);
				if (!v12) {
					BSstrupr(Dst, 260);
				}
				break;
			}
			case 2: {
				const char* v6 = sInventoryApparel.String();
				BSstrcpy(Dst, 260, v6);
				if (!v12) {
					BSstrupr(Dst, 260);
				}
				break;
			}
			case 3: {
				const char* v7 = sInventoryAid.String();
				BSstrcpy(Dst, 260, v7);
				if (!v12) {
					BSstrupr(Dst, 260);
				}
				break;
			}
			case 4: {
				const char* v8 = sInventoryMisc.String();
				BSstrcpy(Dst, 260, v8);
				if (!v12) {
					BSstrupr(Dst, 260);
				}
				break;
			}
			case 5: {
				const char* v9 = sInventoryAmmo.String();
				BSstrcpy(Dst, 260, v9);
				if (!v12) {
					BSstrupr(Dst, 260);
				}
				break;
			}
			default: {
				break;
			}
			}
			if (eItemsFilterMode) {
				apTitleTile->Set(4036, Dst, static_cast<bool>(1));
			}
		}
	}
}

void ContainerMenu::SetContainerTitle(const char* astrTitle) {
	NiPoint3 stringDims;
	if (astrTitle) {
		if (*astrTitle) {
			int32_t ifontNum = static_cast<int32_t>(pTiles[TI_CONTAINER_TITLE]->GetFloat(4025));
			FontManager* Singleton = FontManager::Instance();
			Singleton->CalculateStringDimensions(&stringDims, astrTitle, ifontNum, 3.4028235e+38f, 0);
			float fmaxWidth = pTiles[TI_CONTAINER_TITLE]->pParent->GetFloat(4017) - 80.0;
			if (pTiles[TI_CONTAINER_FILTER_LEFT]->IsTrue(4003)) {
				fmaxWidth = fmaxWidth - 35.0;
			}
			if (!(fmaxWidth < static_cast<double>(stringDims.x))) {
				pTiles[TI_CONTAINER_TITLE]->Set(4036, astrTitle, 1);
			} else {
				char* newTitle = static_cast<char*>(ourMalloc(260u));
				FontLetter* plettersA = FontManager::Instance()->GetFont(ifontNum - 1)->pFontData->pFontLetters;
				int32_t icurrIndex = 0;
				ourMemset(newTitle, 0, 260u);
				float fnewWidth =
					(plettersA[46].fLeadingEdge + plettersA[46].fWidth + plettersA[46].fSpacing) * 3.0 + 0.0f;
				while (fmaxWidth >= static_cast<double>(fnewWidth) && icurrIndex < 260) {
					newTitle[icurrIndex] = astrTitle[icurrIndex];
					fnewWidth = plettersA[astrTitle[icurrIndex]].fLeadingEdge +
						plettersA[astrTitle[icurrIndex]].fWidth + plettersA[astrTitle[icurrIndex]].fSpacing + fnewWidth;
					++icurrIndex;
				}
				BSstrcat(newTitle, 260, "...");
				pTiles[TI_CONTAINER_TITLE]->Set(4036, newTitle, 1);
				ourFree(newTitle);
			}
		}
	}
}

TileMenu* ContainerMenu::Create(TESObjectREFR* apContainer, Interface::CONTAINER_MENU_MODE aeContainerMode) {
	int32_t v4;
	ContainerMenu* Menu_;
	InterfaceManager* pim = InterfaceManager::Instance();
	InterfaceManager::SetContainerMenuQueued(0);
	if (!apContainer && aeContainerMode != Interface::CMM_ROCK_N_LOAD) {
		return 0;
	}
	uint32_t MenuID = ContainerMenu::GetClassStatic();
	Tile* pold_menu_tile = Menu::GetMenuByClass(MenuID);
	if (pold_menu_tile) {
		delete pold_menu_tile;
	}
	Tile* ptile = pim->GetMenusRoot()->ReadFile("Data\\Menus\\container_menu.xml");
	if (ptile) {
		Menu_ = static_cast<ContainerMenu*>(ptile->GetMenu());
	} else {
		Menu_ = 0;
	}
	ContainerMenu::pMe = Menu_;
	if (Menu_ && (v4 = ContainerMenu::pMe->GetClass(), v4 == ContainerMenu::GetClassStatic())) {
		ContainerMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), 0);
		if (ContainerMenu::pMe->IsWellFormedMenu()) {
			if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
				float afValue = Tile::GetMaximumDepth();
				ptile->Set(4013, afValue, 1);
			}
			uint32_t v5 = ContainerMenu::GetClassStatic();
			pim->xTutorialManager.ShowMessage(Interface::TUT_CONTAINER, v5, 512u);
			ContainerMenu::pMe->lbItems.Initialize(ContainerMenu::pMe->pTiles[4], "CM_list_template");
			ContainerMenu::pMe->lbContainer.Initialize(ContainerMenu::pMe->pTiles[8], "CM_list_template");
			const char* v6 = sInventoryItems.String();
			ContainerMenu::pMe->pTiles[1]->Set(4036, v6, 1);
			const char* v7 = sTakeAll.String();
			ContainerMenu::pMe->pTiles[10]->Set(4036, v7, 1);
			const char* v8 = sExit.String();
			ContainerMenu::pMe->pTiles[11]->Set(4036, v8, 1);
			Tile* pTile = Tile::GetTileByName(ContainerMenu::pMe->pMenu, "CM_Subtitle");
			if (pTile) {
				pTile->Set(4036, "", 1);
			}
			ContainerMenu::pMe->pContainerRef = apContainer;
			ContainerMenu::pMe->eContainerMode = aeContainerMode;
			ContainerMenu::pMe->InitContainerMode();
			int32_t a2 = Tile::TextToTrait("_Title");
			if (ContainerMenu::pMe->pTiles[14]) {
				const char* afValuea = sInventoryDamage.String();
				ContainerMenu::pMe->pTiles[14]->Set(a2, afValuea, 1);
			}
			if (ContainerMenu::pMe->pTiles[15]) {
				const char* afValueb = sInventoryDamagePerSecond.String();
				ContainerMenu::pMe->pTiles[15]->Set(a2, afValueb, 1);
			}
			if (ContainerMenu::pMe->pTiles[16]) {
				const char* afValuec = sInventoryStrReq.String();
				ContainerMenu::pMe->pTiles[16]->Set(a2, afValuec, 1);
			}
			if (ContainerMenu::pMe->pTiles[17]) {
				const char* afValued = sInventoryDamageResistance.String();
				ContainerMenu::pMe->pTiles[17]->Set(a2, afValued, 1);
			}
			if (ContainerMenu::pMe->pTiles[18]) {
				const char* afValuee = sInventoryDamageThreshold.String();
				ContainerMenu::pMe->pTiles[18]->Set(a2, afValuee, 1);
			}
			ContainerMenu::pMe->PrepForVisibility(0);
			ContainerMenu::pMe->UpdateLists(0);
			return static_cast<TileMenu*>(ptile);
		} else {
			Warning("MENUS: Container Menu Creation Failed... Are your menu and art resources up to date?");
			return 0;
		}
	} else {
		if (ContainerMenu::pMe) {
			if (ContainerMenu::pMe->pMenu) {
				if (ContainerMenu::pMe) {
					delete ContainerMenu::pMe;
				}
			}
		}
		return 0;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ContainerMenu::Close() {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	if (ContainerMenu::pMe) {
		ContainerMenu::pMe->m_hCurrentReactivityBark.FadeOutAndRelease(100u);
		switch (ContainerMenu::pMe->eContainerMode) {
		case Interface::CMM_STANDARD: {
			goto LABEL_3;
		}
		case Interface::CMM_PICKPOCKET: {
			TESObjectREFR* pContainerRef;
			if (ContainerMenu::pMe->pContainerRef->IsActor()) {
				pContainerRef = ContainerMenu::pMe->pContainerRef;
			} else {
				pContainerRef = 0;
			}
			if (!pContainerRef && ContainerMenu::pMe->iValueStolen > 0) {
				TESForm* Owner = ContainerMenu::pMe->pContainerRef->GetOwner();
				pPlayer->StealAlarm(ContainerMenu::pMe->pContainerRef, 0, 0, ContainerMenu::pMe->iValueStolen, Owner);
			}
		LABEL_3:;
			ContainerMenu::pMe->PlayContainerSound(ContainerMenu::pMe->pContainerRef, 0);
			break;
		}
		case Interface::CMM_TEAMMATE: {
			bool bstartedWithWeapon = 0;
			if (ContainerMenu::pMe->pContainerRef->IsActor()) {
				if (ContainerMenu::pMe->pContainerRef->GetAnimation()) {
					// The exe's container ref is an actor here (IsActor above).
					if (static_cast<Actor*>(ContainerMenu::pMe->pContainerRef)->GetCurrentAIProcess()) {
						BaseProcess* pprocess =
							static_cast<Actor*>(ContainerMenu::pMe->pContainerRef)->GetCurrentAIProcess();
						bstartedWithWeapon = pprocess->GetCurrentWeapon() != nullptr;
					}
				}
			}
			if (ContainerMenu::pMe->pContainerRef->IsCreature()) {
				TESCreature* pcreature =
					static_cast<TESCreature*>(ContainerMenu::pMe->pContainerRef->GetObjectReference());
				pcreature->InitDefaultWorn(ContainerMenu::pMe->pContainerRef, 1, 1, 1);
			} else {
				TESNPC* pnpc = static_cast<TESNPC*>(ContainerMenu::pMe->pContainerRef->GetObjectReference());
				BipedAnim* v1 = ContainerMenu::pMe->pContainerRef->GetBiped();
				pnpc->InitParts(ContainerMenu::pMe->pContainerRef, v1, 1);
			}
			if (bstartedWithWeapon &&
				static_cast<Actor*>(ContainerMenu::pMe->pContainerRef)->Actor::GetWantWeaponDrawn()) {
				BaseProcess* pprocess = static_cast<Actor*>(ContainerMenu::pMe->pContainerRef)->GetCurrentAIProcess();
				if (!pprocess->GetCurrentWeapon()) {
					static_cast<Actor*>(ContainerMenu::pMe->pContainerRef)->Actor::SetWantWeaponDrawn(0);
				}
			}
			BSAudio* v2 = &BSAudio::QInstance();
			BSSoundHandle* SoundHandleByName = v2->GetSoundHandleByName(&a1, "DRSTraderClose", 289u);
			SoundHandleByName->Play(0);
			DialogMenu::ServiceEnd(0);
			break;
		}
		case Interface::CMM_ROCK_N_LOAD: {
			if (!pVATS.IsInVats()) {
				pPlayer->UseActionPoints(ActionPoints::ACTION_RELOAD);
			}
			break;
		}
		default: {
			break;
		}
		}
		ContainerMenu::pMe->ActiveMineList.RemoveAll();
		ContainerMenu::pMe->pMenu->Set(6002, 1);
		ContainerMenu::pMe->StartFadeOut();
		BaseProcess* v3 = pPlayer->pCurrentProcess;
		if (v3->Get3DUpdateFlag(RESET_MODEL)) {
			pPlayer->UpdatePlayer3d();
		}
		HUDMainMenu::RefreshInfoForRef();
	}
}

uint32_t ContainerMenu::GetClassStatic() {
	return 0x3F0;
}

void ContainerMenu::ForceContainerUpdate() {
	if (ContainerMenu::pMe) {
		ContainerMenu::pMe->UpdateLists(nullptr);
	}
}

void ContainerMenu::TakeAllFn(Tile* a1, ItemChange* a2) {
	if (a2) {
		if (ContainerMenu::pMe) {
			if (a1) {
				if (!ContainerMenu::pMe->bPickPocketTallied) {
					TESObjectREFR* v2;
					TESObjectREFR* v3;
					ExtraDataList* m_item = 0;
					if (a2->pExtraObjectList) {
						m_item = a2->pExtraObjectList->m_item;
					}
					if (!m_item ||
						(!m_item->GetItemDropper() ||
							((v2 = m_item->GetItemDropper(), v2->GetDisabled()) ||
								(v3 = m_item->GetItemDropper(), v3->GetDelete())))) {
						if (a1->xChildren.GetTail()->GetFloat(4009) != 128.0) {
							ExtraDataList* v10;
							if (ContainerMenu::pMe->eContainerMode == Interface::CMM_PICKPOCKET) {
								Actor* pContainerRef;
								if (ContainerMenu::pMe->pContainerRef->IsActor()) {
									pContainerRef = static_cast<Actor*>(ContainerMenu::pMe->pContainerRef);
								} else {
									pContainerRef = 0;
								}
								uint32_t FormHealth_0 = a2->iNumber;
								int32_t aiStealValue = ContainerMenu::pMe->GetStolenValue(a2, FormHealth_0, 1);
								ContainerMenu::pMe->iValueStolen += aiStealValue;
								ContainerMenu::pMe->bPickPocketTallied =
									!ContainerMenu::pMe->PickpocketCheck(pContainerRef, aiStealValue);
							}
							if (a2->pExtraObjectList) {
								v10 = a2->pExtraObjectList->m_item;
							} else {
								v10 = 0;
							}
							PlayerCharacter* v9 = pPlayer;
							bool v8 = ContainerMenu::pMe->eContainerMode == Interface::CMM_PICKPOCKET;
							uint32_t v7 = a2->iNumber;
							TESBoundObject* FormFlags = a2->pContainerObj;
							ContainerMenu::pMe->pContainerRef->RemoveItem(FormFlags, v10, v7, v8, 0, v9, 0, 0, 1, 0);
						}
					} else {
						TESObjectREFR* ItemDropper = m_item->GetItemDropper();
						pPlayer->PickUpObject(ItemDropper, 1, 0);
					}
				}
			}
		}
	}
}

bool ContainerMenu::FirstPassFilterFunc(ItemChange* apItem) {
	bool bisFiltered = 0;
	if (ContainerMenu::pMe) {
		TESBoundObject* pobj;
		if (apItem) {
			pobj = apItem->pContainerObj;
		} else {
			pobj = 0;
		}
		if (ContainerMenu::pMe->eContainerMode == Interface::CMM_ROCK_N_LOAD) {
			TESObjectWEAP* CurrentWeapon = pPlayer->GetCurrentWeapon();
			BGSListForm* AmmoFormList = CurrentWeapon->GetAmmoFormList();
			return AmmoFormList->GetItemIndex(pobj) < 0;
		} else {
			TESAmmo* pammo;
			TESObjectWEAP* pweap;
			TESBipedModelForm* pbipedForm = TESBipedModelForm::GetFormAsBipedModel(pobj);
			TESFullName* pfull = dynamic_cast<TESFullName*>(pobj);
			if (apItem->pContainerObj->GetFormType() == WEAP_ID) {
				pweap = static_cast<TESObjectWEAP*>(apItem->pContainerObj);
			} else {
				pweap = 0;
			}
			if (apItem->pContainerObj->GetFormType() == AMMO_ID) {
				pammo = static_cast<TESAmmo*>(apItem->pContainerObj);
			} else {
				pammo = 0;
			}
			if (!pobj ||
				((pbipedForm && !pbipedForm->GetPlayable()) ||
					(!pfull ||
						(!pfull->GetFullNameLength() ||
							((pweap && !pweap->GetIsPlayable()) || (pammo && !pammo->GetIsPlayable())))))) {
				return 1;
			}
		}
	}
	return bisFiltered;
}

int32_t ContainerMenu::SortFunc(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>* apItem2) {
	const char* String = apItem2->pTile->GetString(4036);
	const char* v2 = apItem1->pTile->GetString(4036);
	return _tcscmp(v2, String);
}

void ContainerMenu::InitFunc(Tile* apTile, ItemChange* apItem) {
	if (apItem->GetWorn(0)) {
		apTile->xChildren.GetHead()->Set(4003, 1);
	}
	if (apItem->iNumber > 1) {
		char Src[132];
		uint32_t FormHealth_0 = apItem->iNumber;
		BSsprintf(Src, 128, "%s (%d)", apItem->GetFullName(), FormHealth_0);
		apTile->Set(4036, Src, 1);
	}
	const char* FullName = apItem->GetFullName();
	apTile->SetName(FullName);
	const char* String = apTile->GetString(4036);
	apTile->xChildren.GetTail()->Set(4036, String, 1);
	apTile->Set(4016, 0.0f, 1);
}

void ContainerMenu::ItemSelectCallback(int32_t aiAmountSelected) {
	TESObjectREFR* prefFrom;
	Actor* pactor;
	Tile* pcurrTile = ContainerMenu::pMe->plbCurrListbox->GetTileFor(ContainerMenu::pCurrItem);
	if (ContainerMenu::pMe->pContainerRef && ContainerMenu::pMe->pContainerRef->IsActor()) {
		pactor = static_cast<Actor*>(ContainerMenu::pMe->pContainerRef);
	} else {
		pactor = 0;
	}
	bool bcancelTransfer = 0;
	if (ContainerMenu::pMe->plbCurrListbox == &ContainerMenu::pMe->lbItems) {
		prefFrom = (TESObjectREFR*)pPlayer;
	} else {
		prefFrom = ContainerMenu::pMe->pContainerRef;
	}
	PlayerCharacter* v22 = static_cast<PlayerCharacter*>(prefFrom);
	if (!aiAmountSelected || (!ContainerMenu::pCurrItem || !pcurrTile)) {
		bcancelTransfer = 1;
	}
	if (pcurrTile->xChildren.GetTail()->GetFloat(4009) == 128.0) {
		bcancelTransfer = 1;
	}
	if (ContainerMenu::pMe->eContainerMode == Interface::CMM_TEAMMATE &&
		ContainerMenu::pMe->plbCurrListbox == &ContainerMenu::pMe->lbItems) {
		if (pactor) {
			double WeightInContainer = pactor->GetWeightInContainer(0);
			bool IsHardcore = pPlayer->IsHardcore();
			double v9 = TESWeightForm::GetFormWeight(ContainerMenu::pCurrItem->pContainerObj, IsHardcore) *
					static_cast<double>(aiAmountSelected) +
				WeightInContainer;
			if (pactor->GetMaxCarryWeight() <= v9) {
				char msg[268];
				const char* v5 = sTeammateOverencumbered.String();
				BSsprintf(msg, 260, "%s %s", pactor->GetFullName(), v5);
				Interface::ShowMessage(msg, 2, 0, 0, 2.0f, 0);
				bcancelTransfer = 1;
				TESForm* FormByEditorID = TESForm::GetFormByEditorID("FollowersOverburdened");
				TESTopic* pTopic = dynamic_cast<TESTopic*>(FormByEditorID);
				ContainerMenu::pMe->PlayReactivityBark(pTopic);
			}
		}
	}
	if (ContainerMenu::pMe) {
		if (ContainerMenu::pMe->eContainerMode == Interface::CMM_PICKPOCKET) {
			TESBoundObject* ItemOwnership;
			if ((v22 == pPlayer && pactor) ||
				(v22 != pPlayer &&
					(ItemOwnership = static_cast<TESBoundObject*>(ContainerMenu::pCurrItem->GetItemOwnership()),
						ItemOwnership != pPlayer->GetObjectReference()))) {
				bool bfromContainer = ContainerMenu::pMe->plbCurrListbox == &ContainerMenu::pMe->lbContainer;
				int32_t istealValue = (bfromContainer ? 1 : -1) *
					ContainerMenu::pMe->GetStolenValue(ContainerMenu::pCurrItem, aiAmountSelected, bfromContainer);
				ContainerMenu::pMe->iValueStolen += istealValue;
				if (!bfromContainer) {
					istealValue = 1;
				}
				if (pactor) {
					if (!ContainerMenu::pMe->PickpocketCheck(pactor, istealValue)) {
						bcancelTransfer = 1;
						ContainerMenu::Close();
					}
				}
			}
		}
	}
	if (!bcancelTransfer) {
		TESBoundObject* pcurrItemObj = ContainerMenu::pCurrItem->pContainerObj;
		BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
		ItemChange* pammo = CurrentAIProcess->GetCurrentAmmo();
		pPlayer->PlayPickUpSound(pcurrItemObj, 1, 0);
		ContainerMenu::pMe->SwitchContainer(pcurrTile, ContainerMenu::pCurrItem, aiAmountSelected);
		ContainerMenu::pMe->UpdateLists(pcurrItemObj);
		if (pammo) {
			TESBoundObject* FormFlags = pammo->pContainerObj;
			InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(pPlayer);
			if (!InventoryChanges_->GetObjectCount(FormFlags)) {
				BaseProcess* v7 = pPlayer->pCurrentProcess;
				v7->SetCurrentAmmo(0);
			}
		}
	}
}

bool ContainerMenu::FilterModeFunc(ItemChange* apItem) {
	ContainerMenu::FILTER_MODE emode;
	bool v13 = 0;
	ENUM_FORM_ID eid_2 = apItem->pContainerObj->GetFormType();
	char v12 = 1;
	if (ContainerMenu::pMe->lbItems.GetTileFor(apItem)) {
		emode = ContainerMenu::pMe->eItemsFilterMode;
	} else {
		ContainerMenu::pMe->lbContainer.GetTileFor(apItem);
		emode = ContainerMenu::pMe->eContainerFilterMode;
		v12 = 0;
	}
	switch (emode) {
	case ContainerMenu::FM_WEAPONS: {
		v13 = eid_2 != WEAP_ID;
		break;
	}
	case ContainerMenu::FM_APPAREL: {
		bool v7 = eid_2 != ARMO_ID && eid_2 != CLOT_ID;
		v13 = v7;
		break;
	}
	case ContainerMenu::FM_AID: {
		bool v6 = eid_2 != INGR_ID && (eid_2 != ALCH_ID && eid_2 != BOOK_ID);
		v13 = v6;
		break;
	}
	case ContainerMenu::FM_MISC: {
		bool v5 = eid_2 == WEAP_ID ||
			(eid_2 == ARMO_ID ||
				(eid_2 == CLOT_ID ||
					(eid_2 == INGR_ID ||
						(eid_2 == ALCH_ID ||
							(eid_2 == AMMO_ID ||
								(eid_2 == BOOK_ID || (eid_2 == CHIP_ID || (eid_2 == CCRD_ID || eid_2 == CMNY_ID))))))));
		v13 = v5;
		break;
	}
	case ContainerMenu::FM_AMMO: {
		v13 = eid_2 != AMMO_ID;
		break;
	}
	default: {
		break;
	}
	}
	if (eid_2 == AMMO_ID) {
		BGSListForm* pList = static_cast<BGSListForm*>(TESForm::GetFormByEditorID("RegeneratingAmmo"));
		TESAmmo* pammo = static_cast<TESAmmo*>(apItem->pContainerObj);
		if (pList) {
			if (pList->GetItemIndex(pammo) >= 0) {
				v13 = 1;
			}
		}
		if (pammo) {
			if (!pammo->GetIsPlayable()) {
				v13 = 1;
			}
		}
	}
	if (v12) {
		bool v3 = true;
		if (!v13 && eid_2 != KEYM_ID) {
			TESBoundObject* FormFlags = apItem->pContainerObj;
			if (!FormFlags->GetQuestObject()) {
				return false;
			}
		}
		return v3;
	} else {
		if (ContainerMenu::pMe->eContainerMode == Interface::CMM_PICKPOCKET) {
			return v13 || apItem->GetWorn(0);
		}
	}
	return v13;
}
