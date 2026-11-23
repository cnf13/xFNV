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

#include <cstdint>

bool IsValidPlayerName(const char* apName) {
    bool bHasNonSpace = false;
    char bInvalid = 0;
    if (apName) {
        while (!bInvalid && *apName) {
            uint8_t ucChar = *apName;
            Font::ConvertCharacter(ucChar);
            bInvalid = ucChar == 92 || ucChar == 126;
            Font* pFont = FontManager::Instance()->GetFont(0);
            if (pFont && pFont->pFontData) {
                if (pFont->pFontData->pFontLetters[ucChar].fWidth == 0.0) {
                    bInvalid = 1;
                }
                else {
                    bHasNonSpace = bHasNonSpace || ucChar != 32;
                }
            }

            pFont = FontManager::Instance()->GetFont(6);
            if (pFont && pFont->pFontData) {
                if (pFont->pFontData->pFontLetters[ucChar].fWidth == 0.0) {
                    bInvalid = 1;
                }
                else {
                    bHasNonSpace = bHasNonSpace || ucChar != 32;
                }
            }

            ++apName;
        }
    }
    return bHasNonSpace && !bInvalid;
}

PlayerNameEntryMenu::PlayerNameEntryMenu()
  : pPlayerBase(pPlayer ? dynamic_cast<TESNPC*>(pPlayer->GetObjectReference()) : nullptr) {
    if (pPlayerBase) {
        TextEditMenu::ResetText();
        TextEditMenu::Create(static_cast<const char*>(sEnterName), pPlayerBase->GetFullName(), &IsValidPlayerName);
    }
    else {
        SetPlayerName(nullptr);
    }
}

PlayerNameEntryMenu::~PlayerNameEntryMenu() {}

void PlayerNameEntryMenu::DoIdle() {
    if (TextEditMenu::GetText()) {
        SetPlayerName(TextEditMenu::GetText());
        TextEditMenu::ResetText();
    }
}

void PlayerNameEntryMenu::SetPlayerName(const char* apName) {
    if (apName) {
        pPlayerBase->SetFullName(apName);
        pPlayerBase->AddChange(CHANGE_ACTOR_BASE_FULLNAME);
        StatsMenu::UpdatePlayerName();
    }
    pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
    xFadeState = Interface::FADING_OUT;
}
