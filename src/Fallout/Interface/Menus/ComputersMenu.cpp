#include "Fallout/Interface/Menus/ComputersMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/HackingText.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <stdint.h>

ComputersMenu::AnimatingTextManager::AnimatingTextManager() {}

ComputersMenu::AnimatingTextManager::~AnimatingTextManager() {
	FlushList();
}

// The exe's cleanup-only C++ EH frame is left out.
void ComputersMenu::AnimatingTextManager::Update() {
	FNVD_INLINED_LOCAL(BSSoundHandle, apHandle, std::nullopt, std::nullopt);
	bool btextwraps = 0;
	while (!xAnimatingText.IsEmpty() && BSGetTickCount() > g_computersNextMainMenuTextDisplayTime) {
		if (!xAnimatingText.GetHead()->bIsTextRemaining) {
			int32_t v28 = 0;
			int32_t v26 = 0;
			if (ComputersMenu::pMe->eMode || *xAnimatingText.GetHead()->GetFullText() != "> "[0]) {
				xAnimatingText.GetHead()->Update();
				ComputersMenu::pMe->Sounds[1].Play(0);
			} else {
				uint32_t ioldlen = BSstrlen(xAnimatingText.GetHead()->pCurrentlyDisplayedText);
				xAnimatingText.GetHead()->Update();
				uint32_t inewlen = BSstrlen(xAnimatingText.GetHead()->pCurrentlyDisplayedText);
				for (int32_t j = 0; j < inewlen - ioldlen; ++j) {
					uint32_t v1 = xAnimatingText.GetHead()->uiDisplayRate;
					int32_t itime = j * v1 / (inewlen - ioldlen);
					uint32_t afValue = itime;
					BSAudioManager* v2 = &BSAudioManager::QInstance();
					BSSoundHandle* SoundHandleByName = v2->GetSoundHandleByName(&apHandle, "UIHackingCharSingle", 289u);
					SoundHandleByName->PlayAfter(afValue, 0);
				}
			}
			int32_t ix = 0;
			int32_t iy = 0;
			Tile* pcurrent = xAnimatingText.GetHead()->GetParent();
			btextwraps = pcurrent->GetValueQ(4026) != 0;
			for (Tile* ptile = pcurrent; ptile; ptile = ptile->pParent) {
				Tile* Parent = ComputersMenu::pMe->pTiles[6]->pParent;
				if (ptile == Parent) {
					break;
				}
				int32_t Float = static_cast<int32_t>(ptile->GetFloat(4001));
				ix += Float;
				int32_t v5 = static_cast<int32_t>(ptile->GetFloat(4002));
				iy += v5;
			}
			pcurrent->UpdateTile(0);
			if (ComputersMenu::pMe->lbMenuItems.IsListItem(pcurrent)) {
				pcurrent->UpdateChildren(0);
				double v17 = static_cast<double>(ix);
				int32_t v6 = Tile::AddUserTrait("_text_width", -1);
				float v16 = pcurrent->GetFloat(v6) + v17;
				ComputersMenu::pMe->pTiles[6]->Set(4001, v16, 1);
				double v15 = static_cast<double>(iy);
				int32_t v7 = Tile::AddUserTrait("_text_y", -1);
				float v14 = pcurrent->GetFloat(v7) + v15;
				ComputersMenu::pMe->pTiles[6]->Set(4002, v14, 1);
			} else {
				if (!btextwraps) {
					double v13 = static_cast<double>(ix);
					float v12 = pcurrent->GetFloat(4017) + v13;
					ComputersMenu::pMe->pTiles[6]->Set(4001, v12, 1);
					ComputersMenu::pMe->pTiles[6]->Set(4002, iy);
				}
			}
			break;
		}
		AnimatingText* pold = xAnimatingText.RemoveHead();
		if (ComputersMenu::pMe->eMode) {
			if (pold) {
				if (pold->GetParent() == ComputersMenu::pMe->pTiles[7]) {
					ComputersMenu::pMe->pTiles[15]->Set(4003, 1);
				}
			}
		} else {
			if (pold) {
				if (*pold->GetFullText() == "> "[0]) {
					ComputersMenu::pMe->Sounds[2].Play(0);
				}
			}
			g_computersNextMainMenuTextDisplayTime = BSGetTickCount() + 500;
		}
		AnimatingText* pold_2 = pold;
		AnimatingText* pold_3 = pold;
		if (pold) {
			delete pold_3;
		}
	}
	if (xAnimatingText.IsEmpty() || btextwraps) {
		float v11;
		if (ComputersMenu::pMe->pTiles[5]->IsVisible()) {
			v11 = ComputersMenu::pMe->pTiles[5]->GetFloat(4017);
		} else {
			v11 = 0.0f;
		}
		float v10 = ComputersMenu::pMe->pTiles[5]->GetFloat(4001) + v11;
		ComputersMenu::pMe->pTiles[6]->Set(4001, v10, 1);
		float afValuea = ComputersMenu::pMe->pTiles[5]->GetFloat(4002);
		ComputersMenu::pMe->pTiles[6]->Set(4002, afValuea, 1);
	}
}

uint32_t ComputersMenu::AnimatingTextManager::GetCount() {
	return xAnimatingText.GetSize();
}

void ComputersMenu::AnimatingTextManager::FlushList() {
	while (!xAnimatingText.IsEmpty()) {
		AnimatingText* pText = xAnimatingText.RemoveHead();
		if (pText) {
			delete pText;
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ComputersMenu::AnimatingTextManager::AddItem(const char* apString, Tile::Value* apStringTrait,
	Tile::Value* apVisTrait, int32_t aiWhere) {
	AnimatingText* pText;
	HackingText* apMemory = static_cast<HackingText*>(ourMalloc(sizeof(HackingText)));
	if (apMemory) {
		pText = ::new (apMemory) HackingText(apString);
	} else {
		pText = 0;
	}
	pText->SetTraits(apStringTrait, apVisTrait);
	if (aiWhere == -1) {
		xAnimatingText.AddTail(pText);
	} else {
		NiTListItem<AnimatingText*>* pIter = xAnimatingText.m_pkHead;
		for (int32_t i = 0; pIter && i < aiWhere; ++i) {
			pIter = xAnimatingText.GetNextPos(pIter);
		}
		if (pIter) {
			xAnimatingText.InsertBefore(pIter, pText);
		} else {
			xAnimatingText.AddTail(pText);
		}
	}
	pText->SetVisibility(0);
}

void ComputersMenu::AnimatingTextManager::ForceFinish() {
	for (NiTListItem<AnimatingText*>* pPos = xAnimatingText.m_pkHead; pPos;) {
		xAnimatingText.GetNext(pPos)->ForceFinish();
	}
}

void ComputersMenu::AnimatingTextManager::DeleteInstance() {
	AnimatingTextManager* pManager = pInstance;
	if (pManager) {
		delete pManager;
	}
	pInstance = nullptr;
}

// The exe's cleanup-only C++ EH frame is left out.
ComputersMenu::AnimatingTextManager* ComputersMenu::AnimatingTextManager::GetInstance() {
	if (!ComputersMenu::AnimatingTextManager::pInstance) {
		ComputersMenu::AnimatingTextManager* v1;
		ComputersMenu::AnimatingTextManager* apMemory =
			static_cast<ComputersMenu::AnimatingTextManager*>(ourMalloc(sizeof(ComputersMenu::AnimatingTextManager)));
		if (apMemory) {
			v1 = ::new (apMemory) ComputersMenu::AnimatingTextManager;
		} else {
			v1 = 0;
		}
		ComputersMenu::AnimatingTextManager::pInstance = v1;
	}
	return ComputersMenu::AnimatingTextManager::pInstance;
}

ComputersMenu::ComputersMenu()
	: Menu(), pTiles{}, uiResultDisplayTimeout(0), pCursorVis(nullptr), pTerminal(nullptr), pTerminalRef(nullptr),
	  eMode(CMM_INTRO), pNoteText(nullptr), bTutorial(0), pATManager(AnimatingTextManager::GetInstance()) {
	NoteActivationFn[0] = [](ComputersMenu* apThis, BGSNote* apNote) { apThis->PlayNoteSound(apNote); };
	NoteActivationFn[1] = [](ComputersMenu* apThis, BGSNote* apNote) { apThis->DisplayNoteText(apNote); };
	NoteActivationFn[2] = [](ComputersMenu* apThis, BGSNote* apNote) { apThis->ShowImage(apNote); };
	NoteActivationFn[3] = nullptr;
	BSAudioManager* pManager = &BSAudioManager::QInstance();
	BSSoundHandle kHandle;
	Sounds[0] = *pManager->GetSoundHandleByName(&kHandle, "UIHackingFanHumLP", 49u);
	Sounds[1] = *pManager->GetSoundHandleByName(&kHandle, "UIHackingCharScroll", 33u);
	Sounds[2] = *pManager->GetSoundHandleByName(&kHandle, "UIHackingCharEnter", 33u);
}

ComputersMenu::~ComputersMenu() {
	ComputersMenu::pMe = nullptr;
	ComputersMenu::AnimatingTextManager::DeleteInstance();
	for (int32_t i = 0; i < 3; ++i) {
		Sounds[i].Stop();
		Sounds[i].Release();
	}
	SystemColorManager::GetInstance()->SetColor(3, ComputersMenu::SaveColor);
}

void ComputersMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
	if (((uint32_t)aiID) < 16) {
		pTiles[aiID] = apTile;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ComputersMenu::DoClick(int32_t a1, Tile* a2) {
	FNVD_INLINED_LOCAL(BSStringT<char>, strtmp, std::nullopt, std::nullopt);
	if (!a2 && a1 >= 0 && a1 < 16) {
		a2 = pTiles[a1];
	}
	bool bretainReloadTerminal = 0;
	if (a1 == 2) {
		pATManager->ForceFinish();
		if (pNoteText) {
			DisplayNoteText(0);
			bretainReloadTerminal = 1;
		} else {
			pTiles[2]->Set(4003, 0);
			pTiles[1]->Set(4003, 1);
			if (g_bCurrentMenuEntryHasFlags2) {
				DisplayTerminal(0);
			} else {
				if (!pTiles[7]->IsVisible()) {
					pTiles[15]->Set(4003, 0);
					Tile::Value* pstringval = pTiles[7]->GetValueQ(4036);
					Tile::Value* Value = pTiles[7]->GetValue(4003);
					pATManager->AddItem(pstringval->strValue, pstringval, Value, -1);
				}
				lbMenuItems.Traverse(&ComputersMenu::Redisplay, 0, 0x7FFFFFFF);
				bDisplayFlag = 1;
			}
		}
		Sounds[2].Play(0);
	} else {
		if (a1 == 13 && eMode < ComputersMenu::CMM_NORMAL) {
			FinishIntro();
		} else {
			if (lbMenuItems.IsListItem(a2)) {
				Sounds[2].Play(0);
				TERMINAL_MENU_ITEM* pmenuitem = lbMenuItems.GetDataFor(a2);
				if (pmenuitem) {
					if (pmenuitem->pSubMenu) {
						pmenuitem->SetSelected(1);
						pmenuitem->RunResultScript(pTerminalRef);
						pTerminal = pmenuitem->pSubMenu;
						DisplayTerminal(1);
					} else {
						pmenuitem->SetSelected(1);
						pmenuitem->RunResultScript(pTerminalRef);
						if (pmenuitem->ResultText.String()) {
							Tile::Value* time = pTiles[5]->GetValue(4003);
							Tile::Value* ValueQ = pTiles[5]->GetValueQ(4036);
							const char* v4 = pmenuitem->ResultText.String();
							pATManager->AddItem(v4, ValueQ, time, 0);
							uint32_t v5 = BSGetTickCount();
							uiResultDisplayTimeout = 1000 * iComputersResultDisplayTimeout.Int() + v5;
						}
						if (pmenuitem->pDisplayNote) {
							if (pmenuitem->IsAutoAdd() && !pPlayer->HasNote(pmenuitem->pDisplayNote)) {
								pPlayer->AddNote(pmenuitem->pDisplayNote, 0);
								::new (&strtmp) BSStringT<char>;
								strtmp.SPrintF(sComputersAddedNote.String(), pmenuitem->pDisplayNote->GetFullName());
								Interface::ShowMessage(strtmp.pString, 0,
									"Interface\\Icons\\Message Icons\\glow_message_giftbox.dds", 0, 2.0f, 0);
								strtmp.~BSStringT<char>();
							}
							if (NoteActivationFn[pmenuitem->pDisplayNote->GetNoteType()]) {
								BGSNote* pDisplayNote = pmenuitem->pDisplayNote;
								NOTE_TYPE NoteType = pDisplayNote->GetNoteType();
								NoteActivationFn[NoteType](this, pDisplayNote);
							}
							g_bCurrentMenuEntryHasFlags2 = pmenuitem->ForcesRedraw();
							bretainReloadTerminal = 1;
						} else {
							if (pmenuitem->ForcesRedraw()) {
								DisplayTerminal(0);
							}
						}
					}
				} else {
					DisplayPreviousTerminal();
				}
			}
		}
	}
	if (!bretainReloadTerminal) {
		g_bCurrentMenuEntryHasFlags2 = 0;
	}
}

// eMode is compared signed against CMM_NORMAL, as the exe does.
void ComputersMenu::DoIdle() {
	if (bTutorial && !InterfaceManager::Instance()->xTutorialManager.HasBeenShown(Interface::TUT_TERMINAL)) {
		return;
	}
	if (xFadeState == 2) {
		return;
	}
	pATManager->Update();
	FlashCursor();
	if (!pATManager->GetCount()) {
		if (eMode >= CMM_NORMAL) {
			if (bDisplayFlag) {
				lbMenuItems.Traverse(&ComputersMenu::PostDisplay, 0, 0x7FFFFFFF);
				bDisplayFlag = false;
			}
		} else if (eMode == CMM_INTRO) {
			g_computersSkipMainMenuTime = BSGetTickCount() + 2000;
			eMode = CMM_WAIT;
		} else if (eMode == CMM_WAIT && BSGetTickCount() > static_cast<uint32_t>(g_computersSkipMainMenuTime)) {
			FinishIntro();
		}
	}
	if (uiResultDisplayTimeout && BSGetTickCount() > uiResultDisplayTimeout) {
		uiResultDisplayTimeout = 0;
		pTiles[5]->Set(Tile::kTileValue_Visible, 0);
	}
}

int32_t ComputersMenu::GetClass() {
	return 1057;
}

bool ComputersMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
	if ((!Interface::IsTopMenuID(Interface::eComputersMenu)) || (aiButton != 10)) {
		return (bool)0;
	}
	if (pTiles[2]->IsVisible()) {
		pNoteText = nullptr;
		DoClick(2, nullptr);
	} else {
		DisplayPreviousTerminal();
	}
	return (bool)1;
}

void ComputersMenu::DisplayPreviousTerminal() {
	(&TerminalStack)->RemoveHead();
	pTerminal = (&TerminalStack)->m_item;
	if (pTerminal) {
		DisplayTerminal(0);
	} else {
		ComputersMenu::Close();
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ComputersMenu::FlashCursor() {
	static float g_computersCursorBlinkInterval = [&] {
		int32_t v1 = Tile::AddUserTrait("_blink_interval", -1);
		return pTiles[6]->GetFloat(v1);
	}();
	if (BSGetTickCount() > g_computersNextCursorBlinkTime && pCursorVis) {
		if (pCursorVis->QFloat() == 1.0) {
			pCursorVis->SetFloat(0.0, 1);
		} else {
			pCursorVis->SetFloat(1.0, 1);
		}
		g_computersNextCursorBlinkTime =
			static_cast<int64_t>(static_cast<double>(BSGetTickCount()) + g_computersCursorBlinkInterval);
	} else {
		if (!pCursorVis) {
			pCursorVis = pTiles[6]->GetValue(4003);
		}
	}
}

void ComputersMenu::FinishIntro() {
	pTiles[13]->Set(4015, 0);
	pATManager->ForceFinish();
	pTiles[9]->Set(4003, 0);
	pTiles[10]->Set(4003, 0);
	pTiles[11]->Set(4003, 0);
	pTiles[12]->Set(4003, 0);
	DisplayTerminal(1);
	Tile::Value* afValue = pTiles[3]->GetValue(4003);
	Tile::Value* ValueQ = pTiles[3]->GetValueQ(4036);
	const char* v1 = sComputersHeader1.operator const char*();
	pATManager->AddItem(v1, ValueQ, afValue, 0);
	double Float = pTiles[3]->pParent->GetFloat(4017);
	const char* v2 = sComputersHeader1.String();
	float v19 = (Float - static_cast<double>(17 * BSstrlen(v2))) / 2.0;
	pTiles[3]->Set(4001, v19, 1);
	Tile::Value* afValuea = pTiles[4]->GetValue(4003);
	Tile::Value* v9 = pTiles[4]->GetValueQ(4036);
	const char* v3 = sComputersHeader2.operator const char*();
	pATManager->AddItem(v3, v9, afValuea, 1);
	double v18 = pTiles[4]->pParent->GetFloat(4017);
	const char* v4 = sComputersHeader2.String();
	float v17 = (v18 - static_cast<double>(17 * BSstrlen(v4))) / 2.0;
	pTiles[4]->Set(4001, v17, 1);
	Tile::Value* afValueb = pTiles[0]->GetValue(4003);
	Tile::Value* v10 = pTiles[0]->GetValueQ(4036);
	const char* ServerText = pTerminal->GetServerText();
	pATManager->AddItem(ServerText, v10, afValueb, 2);
	double v16 = pTiles[0]->pParent->GetFloat(4017);
	const char* v6 = pTerminal->GetServerText();
	float v15 = (v16 - static_cast<double>(17 * BSstrlen(v6))) / 2.0;
	pTiles[0]->Set(4001, v15, 1);
	Tile::Value* afValuec = pTiles[8]->GetValue(4003);
	Tile::Value* v7 = pTiles[8]->GetValueQ(4036);
	pATManager->AddItem("> ", v7, afValuec, -1);
	eMode = ComputersMenu::CMM_NORMAL;
}

// The exe's cleanup-only C++ EH frame is left out.
void ComputersMenu::DisplayTerminal(char a1) {
	FNVD_INLINED_LOCAL(BSStringT<char>, strtmp, std::nullopt, std::nullopt);
	if (pTerminal) {
		int32_t v10;
		::new (&strtmp) BSStringT<char>;
		pATManager->ForceFinish();
		// The exe calls RemoveAll through the list's vtable (slot 7, 0x758742); a by-value member call would be
		// devirtualised.
		static_cast<ListBox<TERMINAL_MENU_ITEM*>*>(&lbMenuItems)->RemoveAll();
		if (a1) {
			TerminalStack.AddHead(pTerminal);
		}
		if (iComputersDisplayRateMenus.Int()) {
			v10 = 1000u / iComputersDisplayRateMenus.Int();
		} else {
			v10 = 1000;
		}
		g_iComputersDisplayRateForCurrentMenu = v10;
		const char* pdesc = pTerminal->GetDescription(pTerminal, DESC_ID);
		Tile::Value* pstringval = pTiles[7]->GetValueQ(4036);
		if (pdesc) {
			if (*pdesc) {
				if (!pTiles[7]->IsVisible() || BSstrcmp(pdesc, pstringval->strValue)) {
					pTiles[15]->Set(4003, 0);
					Tile::Value* pstringval_2 = pTiles[7]->GetValue(4003);
					pATManager->AddItem(pdesc, pstringval, pstringval_2, -1);
				}
			}
		}
		for (TERMINAL_MENU_ITEM*& rpItem : pTerminal->GetMenuItemList()->ItemsUntilNull()) {
			if (rpItem->Conditions.IsTrue(pTerminalRef, 0)) {
				Tile* pnew = lbMenuItems.AddItem(rpItem, 0, 0, 0);
				int32_t v3 = Tile::AddUserTrait("_enabled", -1);
				pnew->Set(v3, 0);
				bool IsSelected = rpItem->IsSelected();
				pnew->Set(4054, IsSelected + 1);
				strtmp.SPrintF("%s%s", "> ", rpItem->ResponseText.String());
				Tile::Value* ValueQ = pnew->GetValueQ(4003);
				Tile::Value* v5 = pnew->GetValueQ(Tile::kTileValue_User0);
				pATManager->AddItem(strtmp.pString, v5, ValueQ, -1);
			}
			rpItem->SetSelected(0);
		}
		if (TerminalStack.m_pkNext) {
			Tile* pback = lbMenuItems.AddItem(0, 0, 0, 0);
			int32_t v6 = Tile::AddUserTrait("_enabled", -1);
			pback->Set(v6, 0);
			strtmp.SPrintF("%s%s", "> ", sComputersBack.String());
			Tile::Value* v9 = pback->GetValueQ(4003);
			Tile::Value* v7 = pback->GetValueQ(Tile::kTileValue_User0);
			pATManager->AddItem(strtmp.pString, v7, v9, -1);
		}
		bDisplayFlag = 1;
		strtmp.~BSStringT<char>();
	}
}

bool ComputersMenu::IsWellFormedMenu(int32_t& ai) {
	for (ai = 0; ai < 16; ++ai) {
		if (!pTiles[ai]) {
			return 0;
		}
	}
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out.
void ComputersMenu::DisplayIntro() {
	uint32_t v12;
	uint32_t v13;
	uint8_t apDest[16];
	FNVD_INLINED_LOCAL(BSStringT<char>, strtmp, std::nullopt, std::nullopt);
	::new (&strtmp) BSStringT<char>;
	if (iComputersDisplayRateMenus.UInt()) {
		v13 = 1000 / iComputersDisplayRateMenus.UInt();
	} else {
		v13 = 1000;
	}
	if (iHackingInputRate.UInt()) {
		v12 = 1000 / iHackingInputRate.UInt();
	} else {
		v12 = 1000;
	}
	int32_t v17 = v12;
	g_iComputersDisplayRateForCurrentMenu = v13;
	Tile::Value* Value = pTiles[9]->GetValue(4003);
	Tile::Value* ValueQ = pTiles[9]->GetValueQ(4036);
	const char* v1 = sHackingIntro01.operator const char*();
	pATManager->AddItem(v1, ValueQ, Value, -1);
	g_iComputersDisplayRateForCurrentMenu = v17;
	strtmp.SPrintF("%s%s", "> ", sComputersLogon.String());
	Tile::Value* v9 = pTiles[10]->GetValue(4003);
	Tile::Value* v2 = pTiles[10]->GetValueQ(4036);
	pATManager->AddItem(strtmp.pString, v2, v9, -1);
	g_iComputersDisplayRateForCurrentMenu = v13;
	Tile::Value* v10 = pTiles[11]->GetValue(4003);
	Tile::Value* v7 = pTiles[11]->GetValueQ(4036);
	const char* v3 = sHackingHeader2.operator const char*();
	pATManager->AddItem(v3, v7, v10, -1);
	g_iComputersDisplayRateForCurrentMenu = v17;
	ourMemset(apDest, 0, 16u);
	uint32_t HackMiniGameValue = pTerminal->GetHackMiniGameValue(pTerminalRef);
	ourMemset(apDest, 42, HackMiniGameValue);
	strtmp.SPrintF("%s%s", "> ", apDest);
	Tile::Value* v11 = pTiles[12]->GetValue(4003);
	Tile::Value* v5 = pTiles[12]->GetValueQ(4036);
	pATManager->AddItem(strtmp.pString, v5, v11, -1);
	strtmp.~BSStringT<char>();
}

// The exe's cleanup-only C++ EH frame is left out.
void ComputersMenu::PlayNoteSound(BGSNote* apNote) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	TESSound* NoteSound = apNote->GetNoteSound();
	uint32_t CastingType = NoteSound->iFormID;
	BSAudio* v3 = &BSAudio::QInstance();
	BSSoundHandle* SoundHandleByNumericID = v3->GetSoundHandleByNumericID(&a1, CastingType, 257u);
	SoundHandleByNumericID->Play(0);
}

// The exe's cleanup-only C++ EH frame is left out.
void ComputersMenu::DisplayNoteText(BGSNote* apNote) {
	int32_t v15;
	pATManager->ForceFinish();
	if (iComputersDisplayRateNotes.Int()) {
		v15 = 1000u / iComputersDisplayRateNotes.Int();
	} else {
		v15 = 1000;
	}
	g_iComputersDisplayRateForCurrentMenu = v15;
	pTiles[1]->Set(4003, 0);
	pTiles[2]->Set(4009, 0);
	pTiles[2]->Set(4003, 1);
	if (!this->pNoteText) {
		TESDescription* NoteText = apNote->GetNoteText();
		const char* v2 = NoteText->GetDescription(apNote, TNAM_ID);
		strNoteText.Set(v2, 0);
		this->pNoteText = strNoteText.pString;
	}
	double Float = pTiles[14]->GetFloat(4028);
	double v12 = pTiles[7]->GetFloat(4016);
	int32_t v11 = static_cast<int32_t>(pTiles[7]->GetFloat(4025)) - 1;
	FontManager* Singleton = FontManager::Instance();
	int32_t e = static_cast<int32_t>(Float - v12 / Singleton->GetFont(v11)->fFontHeight) + 1;
	double v4 = pTiles[14]->GetFloat(4026);
	FNVD_CONSTRUCTED_LOCAL(Font::TextData, a2, &Font::TextData::Destruct, static_cast<int32_t>(v4), 0, 0, e, 10);
	const char* pNoteText = this->pNoteText;
	int32_t v8 = static_cast<int32_t>(pTiles[14]->GetFloat(4025)) - 1;
	FontManager* v5 = FontManager::Instance();
	v5->GetFont(v8)->PrepText(pNoteText, a2);
	Tile::Value* Value = pTiles[14]->GetValue(4003);
	Tile::Value* ValueQ = pTiles[14]->GetValueQ(4036);
	pATManager->AddItem(a2.xNewText.pString, ValueQ, Value, -1);
	uint32_t Length = strNoteText.GetLength();
	// The exe calls the unsigned BSMIN (0x42F5A0, jae) with the character count.
	this->pNoteText += BSMIN(static_cast<uint32_t>(a2.GetCharCount()), Length);
	if (!*this->pNoteText) {
		this->pNoteText = 0;
	}
}

void ComputersMenu::ShowImage(BGSNote* a1) {
	pATManager->ForceFinish();
	pTiles[1]->Set(4003, 0);
	pTiles[2]->Set(4009, 255);
	pTiles[2]->Set(4003, 1);
	pTiles[14]->Set(4003, 0);
	if (pTerminal->HidesWelcomeTextWhenDisplayingImage()) {
		pTiles[7]->Set(4003, 0);
		pTiles[15]->Set(4003, 0);
	}
	const char* TextureName = a1->GetNoteImage()->GetTextureName();
	pTiles[2]->Set(4044, TextureName, 1);
}

Tile* ComputersMenu::Create(TESObjectREFR* apTerminalRef) {
	int32_t v5;
	FNVD_INLINED_LOCAL(NiColorA, apMemory, std::nullopt, std::nullopt);
	int32_t ClassStatic = ComputersMenu::GetClassStatic();
	Tile* pold_menu_tile = Menu::GetMenuByClass(ClassStatic);
	if (pold_menu_tile) {
		delete pold_menu_tile;
	}
	SystemColorManager* Instance = SystemColorManager::GetInstance();
	Instance->GetColor(3, ComputersMenu::SaveColor);
	BGSTerminal* pterminal = static_cast<BGSTerminal*>(apTerminalRef->GetObjectReference());
	if (pterminal->UsesAltColor()) {
		NiColorA* fA = ::new (&apMemory) NiColorA(1.0f, 1.0f, 1.0f, 0.0f);
		SystemColorManager* v3 = SystemColorManager::GetInstance();
		v3->SetColor(3, *fA);
	}
	float ftop_depth = Tile::GetMaximumDepth();
	InterfaceManager* v4 = InterfaceManager::Instance();
	Tile* ptile = v4->GetMenusRoot()->ReadFile("Data\\Menus\\computers_menu.xml");
	Menu* pmenu = ptile->GetMenu();
	if (pmenu && (v5 = pmenu->GetClass(), v5 == ComputersMenu::GetClassStatic())) {
		int32_t imissingIndex;
		pmenu->SetMenuTile(static_cast<TileMenu*>(ptile), 0);
		ComputersMenu::pMe = static_cast<ComputersMenu*>(pmenu);
		if (static_cast<ComputersMenu*>(pmenu)->ComputersMenu::IsWellFormedMenu(imissingIndex)) {
			if (apTerminalRef) {
				if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
					ptile->Set(4013, ftop_depth, 1);
				}
				int32_t fB = ComputersMenu::GetClassStatic();
				InterfaceManager* v7 = InterfaceManager::Instance();
				ComputersMenu::pMe->bTutorial = v7->xTutorialManager.ShowMessage(Interface::TUT_TERMINAL, fB, 512u);
				ComputersMenu::pMe->pTerminalRef = apTerminalRef;
				ComputersMenu::pMe->pTerminal = pterminal;
				ComputersMenu::pMe->lbMenuItems.Initialize(ComputersMenu::pMe->pTiles[1], "computers_file_template");
				ComputersMenu::pMe->Sounds->Play(0);
				if (ComputersMenu::pMe->pTerminal->IsUnlocked(ComputersMenu::pMe->pTerminalRef)) {
					ComputersMenu::pMe->FinishIntro();
				} else {
					ComputersMenu::pMe->DisplayIntro();
				}
				pmenu->PrepForVisibility(0);
				return ptile;
			} else {
				Warning("MENUS: Computers Menu Creation Failed... Missing terminal data ");
				return 0;
			}
		} else {
			Warning("MENUS: Computers Menu Creation Failed... Missing index = #%d", imissingIndex);
			return 0;
		}
	} else {
		if (pmenu) {
			if (pmenu->pMenu) {
				if (pmenu) {
					delete pmenu;
				}
			}
		}
		return 0;
	}
}

int32_t ComputersMenu::GetClassStatic() {
	return 1057;
}

void ComputersMenu::PostDisplay(Tile* apTile, TERMINAL_MENU_ITEM* apMenuItem) {
	if (apTile) {
		apTile->Set(Tile::AddUserTrait("_enabled", -1), 1);
	}
}

void ComputersMenu::Close() {
	if (ComputersMenu::pMe) {
		PlayTerminalShutdownSound();
		ComputersMenu::pMe->pMenu->Set(4033, 0.75f, static_cast<bool>(1));
		InterfaceManager* v0 = InterfaceManager::Instance();
		if (v0->GetCurrentRenderedMenu()) {
			InterfaceManager* v1 = InterfaceManager::Instance();
			NiNode* MenuRoot = static_cast<NiNode*>(v1->GetCurrentRenderedMenu()->GetMenuRoot());
			if (MenuRoot == ComputersMenu::pMe->pMenu->GetModel()) {
				InterfaceManager* v3 = InterfaceManager::Instance();
				FORenderedMenu* CurrentRenderedMenu = v3->GetCurrentRenderedMenu();
				FORenderedTerminal* pterm = dynamic_cast<FORenderedTerminal*>(CurrentRenderedMenu);
				pterm->FadeOut();
			}
		}
		ComputersMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
		ComputersMenu::pMe->StartFadeOut();
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ComputersMenu::Redisplay(Tile* apTile, TERMINAL_MENU_ITEM* apMenuItem) {
	if (apTile) {
		FNVD_CONSTRUCTED_LOCAL(BSStringT<char>, strtmp, &BSStringT<char>::Destruct, "> ");
		int32_t idefault = 1;
		bool bgo = 1;
		int32_t v2 = Tile::AddUserTrait("_enabled", -1);
		apTile->Set(v2, 0);
		if (apMenuItem && apMenuItem->Conditions.IsTrue(0, 0)) {
			if (apMenuItem->IsSelected()) {
				idefault = 2;
				apMenuItem->SetSelected(0);
			}
			strtmp.operator+=(static_cast<const char*>(apMenuItem->ResponseText.pString));
		} else {
			if (apMenuItem) {
				bgo = 0;
			} else {
				strtmp.operator+=(static_cast<const char*>(sComputersBack.String()));
			}
		}
		apTile->Set(4054, idefault);
		if (bgo) {
			Tile::Value* ValueQ = apTile->GetValueQ(4003);
			Tile::Value* v5 = apTile->GetValueQ(Tile::kTileValue_User0);
			char* v4 = strtmp.pString;
			ComputersMenu::AnimatingTextManager* Instance = ComputersMenu::AnimatingTextManager::GetInstance();
			Instance->AddItem(v4, v5, ValueQ, -1);
		}
	}
}

const char* BGSTerminal::GetServerText() {
	if (Data.cServerType >= 10) {
		return nullptr;
	}
	return s_TerminalServerTexts[Data.cServerType]->String();
}

HackingText::HackingText(const char* apString) {
	init();
	*this = apString;
}

// IDA names it SetStr. The exe calls BSStringT<char>::operator= (0x438390) out of line; src's is the shared inline
// template.
HackingText& HackingText::operator=(const char* apString) {
	strText = apString;
	ResetBuffer();
	return *this;
}

// The PDB's AnimatingText::GetFullText: the full text (strFullText's string).
const char* HackingText::GetFullText() {
	return strText.String();
}
