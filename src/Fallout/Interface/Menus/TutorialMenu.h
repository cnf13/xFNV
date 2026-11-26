#pragma once

#include "BSCore/NewOverloads.h"
#include "BSMenu/Menu.h"

class BGSMessage;

class TutorialMenu : public Menu {
public:
    TutorialMenu();
    ~TutorialMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    int32_t GetClass() override;
    bool DoGamepad(int32_t, float) override;
    bool DoGamepadUpEvent(int32_t, float) override;

    void NextPage();
    void PrevPage();
    void SetPage(uint32_t auiPage);
    // Shows apMessage's name and description.
    void SetTitleAndText(BGSMessage* apMessage);
    // Sets trait 0xFAF of pTiles[2] to iCurrentPage > 0 and of pTiles[1] to iCurrentPage + 1 < iNumPages.
    void UpdatePrevNext();

    // Hides the open tutorial menu, fades it out and goes back in the start menu.
    static void Close();
    // Opens the tutorial menu on apMessage.
    static bool Create(BGSMessage* apMessage, bool abHelpManual);
    // The menu id, 0x423 (eTutorialMenu).
    static int32_t GetClassStatic();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    Tile* pTiles[8];
    uint32_t iCurrentPage;
    uint32_t iNumPages;
    BSSimpleList<TESForm*>* pHelpManual;

    static inline TutorialMenu* pMe;
    static SettingT<INIPrefSettingCollection> bHelpEnabled;
};
