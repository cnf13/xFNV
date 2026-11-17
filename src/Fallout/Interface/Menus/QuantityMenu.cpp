#include "Fallout/Interface/Menus/QuantityMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"

void QuantityMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
	if (aiID >= 0 && aiID < 6) {
		pTiles[aiID] = apTile;
	}
}

// Handles the menu's buttons: 1 and 2 step the quantity down and up within [0, max]; 4 (accept) and 5 (cancel)
// report the quantity (or 0) to the owner's callback and close the menu.
// The exe stores pTiles[aiID] into its own apTarget argument slot when apTarget is null and aiID is in [0, 6);
// nothing reads it afterwards.
void QuantityMenu::DoClick(int32_t aiID, Tile* apTarget) {
	float fQuantity = pTiles[3]->GetFloat(4036);
	if (!apTarget && aiID >= 0 && aiID < 6)
		apTarget = pTiles[aiID];
	switch (aiID) {
	case 4:
	case 5:
		if (fpResultCallback) {
			int32_t iQuantity;
			if (aiID == 4)
				iQuantity = FloatToInt(fQuantity);
			else
				iQuantity = 0;
			// The exe calls the callback cdecl (add esp, 4 after the call at 0x7ABECC).
			fpResultCallback(iQuantity);
		}
		Close();
		break;
	case 1:
	case 2: {
		float fMax = pTiles[0]->GetFloat(4100);
		float fStep;
		if (aiID == 2)
			fStep = 1.0f;
		else
			fStep = -1.0f;
		// The sum is stored to a float local before BSMAX (fstp dword).
		float fNew = fQuantity + fStep;
		pTiles[0]->Set(4102, BSMIN(BSMAX(fNew, 0.0f), fMax), true);
		pTiles[0]->Set(4080, 0.0f, true);
		break;
	}
	default:
		break;
	}
}

void QuantityMenu::DoIdle() {
	float v2 = pTiles[3]->GetFloat(4036) - fPrevQuantity;
	double x_4 = (double)NiAbs(v2);
	if (pTiles[0]->GetFloat(4103) <= x_4) {
		Interface::PlayMenuSound(4);
		fPrevQuantity = pTiles[3]->GetFloat(4036);
	}
}

int32_t QuantityMenu::GetClass() {
	return 1016;
}

bool QuantityMenu::DoGamepad(int32_t a1, float a2) {
	bool bhandled = 0;
	switch (a1) {
	case 13:
	case 14: {
		float fjumpSize_2;
		float fquantity = pTiles[3]->GetFloat(4036);
		float fmaxAmount = pTiles[0]->GetFloat(Tile::kTileValue_User0);
		float fjumpSize = pTiles[0]->GetFloat(Tile::kTileValue_User4);
		if (a1 == 14) {
			fjumpSize_2 = fjumpSize;
		} else {
			fjumpSize_2 = -fjumpSize;
		}
		float v10 = fquantity + fjumpSize_2;
		float a22 = BSMAX(v10, 0.0f);
		float afB = BSMIN(a22, fmaxAmount);
		pTiles[0]->Set(Tile::kTileValue_User2, afB, static_cast<bool>(1));
		pTiles[0]->Set(4080, 0.0f, static_cast<bool>(1));
		bhandled = 1;
		break;
	}
	case 15:
	case 16: {
		float fstepSize_2;
		float fquantity_2 = pTiles[3]->GetFloat(4036);
		float fmaxAmount_2 = pTiles[0]->GetFloat(Tile::kTileValue_User0);
		float fstepSize = pTiles[0]->GetFloat(Tile::kTileValue_User3);
		if (a1 == 16) {
			fstepSize_2 = fstepSize;
		} else {
			fstepSize_2 = -fstepSize;
		}
		float v8 = fquantity_2 + fstepSize_2;
		float a2a = BSMAX(v8, 0.0f);
		float afBa = BSMIN(a2a, fmaxAmount_2);
		pTiles[0]->Set(Tile::kTileValue_User2, afBa, static_cast<bool>(1));
		pTiles[0]->Set(4080, 0.0f, static_cast<bool>(1));
		bhandled = 1;
		break;
	}
	default: {
		return bhandled;
	}
	}
	return bhandled;
}

bool QuantityMenu::DoGamepadUpEvent(int32_t a2, float a3) {
	int8_t v4 = 0;
	if (a2 == 10) {
		DoClick(5, nullptr);
		return (bool)1;
	}
	return (bool)v4;
}

bool QuantityMenu::IsWellFormedMenu() {
	for (int32_t i = 0; i < 6; ++i) {
		if (!pTiles[i]) {
			return 0;
		}
	}
	return 1;
}

// The exe reaches the menu's tile through the shared getter 0x726070.
void QuantityMenu::Close() {
	if (pMe) {
		pMe->GetMenuTile()->Set(6002, 1);
		if (Interface::GetCurrentRenderedMenu())
			pMe->InstantFadeOut();
		else
			pMe->StartFadeOut();
	}
}

// The exe's cleanup-only C++ EH frame is left out.
TileMenu* QuantityMenu::Create(int32_t aiMaxAmount, QuantityMenu::ResultCallback afpCallback, int32_t aiStartAmount) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	InterfaceManager* pin = InterfaceManager::Instance();
	bool boverMenu = 0;
	int32_t iClass = GetClass_007ABDA0();
	Tile* pold_menu_tile = Menu::GetMenuByClass(iClass);
	if (pold_menu_tile) {
		delete pold_menu_tile;
	}
	if (Interface::GetTopMenuID()) {
		boverMenu = 1;
	}
	Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\quantity_menu.xml");
	QuantityMenu::pMe = static_cast<QuantityMenu*>(ptile->GetMenu());
	if (QuantityMenu::pMe && QuantityMenu::pMe->GetClass() == GetClass_007ABDA0()) {
		QuantityMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), 0);
		if (QuantityMenu::pMe->IsWellFormedMenu()) {
			BSAudio* v8 = &BSAudio::QInstance();
			BSSoundHandle* SoundHandleByName = v8->GetSoundHandleByName(&a1, "UIPopUpMessageWindow", 289u);
			SoundHandleByName->Play(0);
			if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
				float afValue = Tile::GetMaximumDepth();
				ptile->Set(4013, afValue, 1);
			}
			int32_t istartQuantity = BSMIN(aiStartAmount, aiMaxAmount);
			int32_t istepSize = BSMAX(aiMaxAmount / 20, 1);
			int32_t ijumpSize = BSMAX(aiMaxAmount / 4, 1);
			if (istepSize > 10) {
				istepSize -= istepSize % 10;
			}
			QuantityMenu::pMe->pTiles[0]->Set(Tile::kTileValue_User0, aiMaxAmount);
			QuantityMenu::pMe->pTiles[0]->Set(Tile::kTileValue_User2, istartQuantity);
			QuantityMenu::pMe->pTiles[0]->Set(Tile::kTileValue_User3, istepSize);
			QuantityMenu::pMe->pTiles[0]->Set(Tile::kTileValue_User4, ijumpSize);
			QuantityMenu::pMe->pTiles[0]->Set(4080, 0.0f, 1);
			QuantityMenu::fpResultCallback = afpCallback;
			const char* afValuea = sHowMany.String();
			Tile* ChildByName = ptile->GetChildByName("QM_HowManyText");
			ChildByName->Set(4036, afValuea, 1);
			const char* v10 = sOk.String();
			QuantityMenu::pMe->pTiles[4]->Set(4036, v10, 1);
			const char* v11 = sCancel.String();
			QuantityMenu::pMe->pTiles[5]->Set(4036, v11, 1);
			if (boverMenu) {
				float v15 = fPopUpBackgroundOpacity.Float() * 255.0;
				Tile* v12 = QuantityMenu::pMe->pMenu->GetChildByName("QM_Background");
				v12->Set(4009, v15, 1);
			}
			QuantityMenu::pMe->PrepForVisibility(0);
			return static_cast<TileMenu*>(ptile);
		} else {
			Warning("MENUS: Quantity Menu Creation Failed... Are your menu and art resources up to date?");
			return 0;
		}
	} else {
		if (QuantityMenu::pMe) {
			if (QuantityMenu::pMe->pMenu) {
				if (QuantityMenu::pMe) {
					delete QuantityMenu::pMe;
				}
			}
		}
		return 0;
	}
}
