#pragma once

#include "BSMenu/Menu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout/Interface/ListBox.h"

#include <stdint.h>

char ToggleSelection(ListBox<ActorValue::Index>* apThis, [[maybe_unused]] void* apEdx, Tile* apTile);

class CharGenMenu : public Menu {
public:
    // What the menu assigns points to (eItemType).
    enum ITEM_TYPE : int32_t {
        IT_ATTRIBUTE = 0,
        IT_TAG_SKILL = 1,
        NUM_TYPES = 2,
    };

    CharGenMenu();
    ~CharGenMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoEnter(int32_t, Tile*) override;
    int32_t GetClass() override;
    bool DoGamepad(int32_t, float) override;
    bool DoGamepadUpEvent(int32_t, float) override;

    char HasTiles();
    uint32_t SetupSpecialPoints();
    void SetupSkillPoints();
    void SetDistributePointsString();
    void SetDisplayString(Tile* apTile);

    // Creates the menu; Create_ is a wrapper that first checks that the interface manager exists and has been
    // initialised.
    static Tile* Create(int32_t aiItemType, int32_t aiMaxPoints, bool abOnlyAddValues, bool abShowTaggedSkills);
    // Creates the menu once the interface manager is initialised; it returns Create's tile, else null.
    static Tile* Create_(int32_t aiItemType, int32_t aiMaxPoints, bool abOnlyAddValues, bool abShowTaggedSkills);
    // The menu's id, eCharGenMenu.
    static int32_t GetClassStatic();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // The attribute list's ListBox::Traverse visitor that DoClick's reset button (id 5, IT_ATTRIBUTE) runs; gives the
    // tile's added points back. aeIndex is not read.
    static void ResetAttributesFunc(Tile* apTile, ActorValue::Index aeIndex);
    static void Close();
    static void ResetSkillsFunc(Tile* apTile, ActorValue::Index aiSkill);
    static void SetAttributesFunc(Tile* apTile, ActorValue::Index aeIndex);
    static void TagSkillsFunc(Tile* apTile, ActorValue::Index aeIndex);

    ITEM_TYPE eItemType;
    Tile* pTiles[7];
    int32_t iCurrPoints;
    int32_t iMaxPoints;
    float fPrevHighlightIndex;
    uint8_t bOnlyAddValues;
    uint8_t bShowTaggedSkills;
    ListBox<ActorValue::Index> lbItemList;

    static inline CharGenMenu* pMe;
    static inline int32_t iUT_AddedValue;
};

inline int32_t g_CharGenMenuTrait_BaseValue;
inline int32_t g_CharGenMenuTrait_ExtraValue;
inline int32_t aiIndex;
inline int32_t g_CharGenMenuTrait_CurrPoints;
inline int32_t g_CharGenMenuTrait_MaxPoints;
inline int32_t g_CharGenMenuTrait_selected;
