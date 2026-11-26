#pragma once

#include "BSCore/NewOverloads.h"
#include "BSMenu/Menu.h"

class Actor;
class TESObjectBOOK;
class TESObjectREFR;

class BookMenu : public Menu {
public:
    BookMenu();
    ~BookMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoDownClick(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    // DoEnter and DoLeave are inherited, not overridden.
    void DoStillDown(int32_t, Tile*) override;
    void DoWheelMove(int32_t, Tile*) override;
    // DoIdle is inherited, not overridden.
    bool DoKey(int32_t) override;
    int32_t GetClass() override;

    bool IsWellFormedMenu();
    void Close();
    void SetupTextAndButtons(TESObjectBOOK* apBook, Actor* apActor);
    void UseWorkbook();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static Tile* Create(TESObjectBOOK* apBook, TESObjectREFR* apRef);

    Tile* pScrollBar;
    Tile* pScrollMarker;
    // The reference being read.
    TESObjectREFR* pBook;
    // The book form being read.
    TESObjectBOOK* pScroll;
    float fScrollOffset;
};
