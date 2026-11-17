#include "Fallout/Interface/Menus/PlayerNameEntryMenu.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/TextEntryMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Reconstruction.hpp"

#include <stdint.h>

bool IsValidPlayerName(const char* a1) {
	bool v10 = 0;
	char v9 = 0;
	if (a1) {
		while (!v9 && *a1) {
			uint8_t acChar = *a1;
			Font::ConvertCharacter(acChar);
			bool v5 = acChar == 92 || acChar == 126;
			v9 = v5;
			Font* Font_ = FontManager::Instance()->GetFont(0);
			if (Font_) {
				if (Font_->pFontData) {
					if (Font_->pFontData->pFontLetters[acChar].fWidth == 0.0) {
						v9 = 1;
					} else {
						bool v4 = v10 || acChar != 32;
						v10 = v4;
					}
				}
			}
			Font* v7 = FontManager::Instance()->GetFont(6);
			if (v7) {
				if (v7->pFontData) {
					if (v7->pFontData->pFontLetters[acChar].fWidth == 0.0) {
						v9 = 1;
					} else {
						bool v3 = v10 || acChar != 32;
						v10 = v3;
					}
				}
			}
			++a1;
		}
	}
	return v10 && !v9;
}

PlayerNameEntryMenu::PlayerNameEntryMenu()
	: pPlayerBase(pPlayer ? dynamic_cast<TESNPC*>(pPlayer->GetObjectReference()) : 0) {
	if (pPlayerBase) {
		TextEditMenu::ResetText();
		TextEditMenu::Create(sEnterName.operator const char*(), pPlayerBase->GetFullName(), &IsValidPlayerName);
	} else {
		SetPlayerName(0);
	}
}

PlayerNameEntryMenu::~PlayerNameEntryMenu() {}

void PlayerNameEntryMenu::DoIdle() {
	const char* v2;
	const char* result = TextEditMenu::GetText();
	if (result) {
		v2 = TextEditMenu::GetText();
		SetPlayerName(v2);
		TextEditMenu::ResetText();
		return;
	}
}

void PlayerNameEntryMenu::SetPlayerName(const char* a1) {
	if (a1) {
		pPlayerBase->SetFullName(a1);
		pPlayerBase->AddChange(32);
		StatsMenu::UpdatePlayerName();
	}
	pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
	xFadeState = Interface::FADING_OUT;
}
