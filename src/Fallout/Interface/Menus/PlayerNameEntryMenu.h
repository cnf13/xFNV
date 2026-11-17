#pragma once

#include "BSMenu/Menu.h"

class TESNPC;

bool IsValidPlayerName(const char* a1);

// The menu that asks for the player's name.
class PlayerNameEntryMenu : public Menu {
public:
    PlayerNameEntryMenu();
    ~PlayerNameEntryMenu() override;

    void DoIdle() override;

    void SetPlayerName(const char* a1);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    // The player's base form, which receives the name.
    TESNPC* pPlayerBase;
};
