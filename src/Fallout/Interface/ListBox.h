#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout/Interface/MenuManager.h"

#include <stdint.h>

class BGSNote;
class BGSPerk;
class BGSQuestObjective;
class BGSSaveLoadFileEntry;
class ItemChange;
struct PerkRankData;
struct TERMINAL_MENU_ITEM;
class TESBoundObject;
class TESChallenge;
class TESConditionItem;
class TESObjectREFR;
class TESQuest;
class TESRecipe;
class TESRecipeComponent;
class TESReputation;
class Tile;

// Orders two inventory changes by their full names (_tcscmp): negative, 0 or positive.
int32_t SortByName(ItemChange* apItemA, ItemChange* apItemB);
// Orders two inventory changes by count (iNumber, +4, compared
// signed), like its neighbour SortByName orders them by name: -1, 0 or 1.
int32_t SortByCount(ItemChange* apItemA, ItemChange* apItemB);
// Orders two inventory changes by their objects' health
// (TESHealthForm::GetFormHealth through the vtable, compared unsigned): -1, 0 or 1.
int32_t SortByHealth(ItemChange* apItemA, ItemChange* apItemB);
// Whether the change's object is a weapon.
bool IsWeapon(ItemChange* apItem);
// Whether the change's object is apparel (CLOT or ARMO).
bool IsApparel(ItemChange* apItem);
// Whether the change's object is a misc item (MISC, AMMO, IMOD,
// COBJ, CHIP, CMNY or CCRD).
bool IsMiscItem(ItemChange* apItem);

// One entry of a list box: the tile showing it, the data it stands for and whether a filter hides it.
template <class T>
struct ListBoxItem {
    ListBoxItem(Tile* apTile, const T* apData) : pTile(apTile), data(*apData), bFiltered(false) {}

    Tile* pTile;
    T data;
    bool bFiltered;
};

// A menu list whose entries are tiles built from one template, each tied to a data value. The instantiations that are
// used are specialised next to the caller.
template <class T>
class ListBox : public ListBoxBase, public BSSimpleList<ListBoxItem<T>*> {
public:
    // The list's behaviour flags (bit indices into usFlags).
    enum Flag : int32_t {
        LBF_AUTO_UPDATE = 0,
        LBF_FROM_CONTAINER = 1,
    };

    // How SetSelection changes an item's _selected trait: set it, clear it or flip it.
    enum SelectMode : int32_t {
        LBSM_ON = 0,
        LBSM_OFF = 1,
        LBSM_TOGGLE = 2,
    };

    // Filter's test: whether the item with this data stays unfiltered.
    using FilterFunction = bool (*)(T aData);

    // Creates an empty list with LBF_AUTO_UPDATE set and appends it to the open list boxes.
    ListBox();

    bool Highlight(Tile* apHighlightItem) override;
    // The highlighted item's tile.
    void* GetHighlightedItem() override {
        return pHighlightedItem;
    }
    Tile* GetHighlightTarget(int32_t aeButton) override;
    bool BelongsTo(Menu* apMenu) override;
    // Scrolls the list so the highlighted item shows.
    void ScrollToHighlight() override;
    // The tile of GetAt(aiIndex, abLiteral), or null.
    Tile* GetTileAt(uint32_t aiIndex, bool abLiteral) override;
    virtual ~ListBox();
    // Frees every item and its tile.
    virtual void RemoveAll();
    // Orders the items with the comparison function.
    virtual void Sort(int32_t (*apCompareFn)(ListBoxItem<T>*, ListBoxItem<T>*));

    // Takes the container tile and the template name (kept as given, not copied) and finds the scroll bar, the
    // container's child(lb_scrollbar).
    void Initialize(Tile* apContainerRect, const char* apTemplateName);
    // Adds an item for aAssociatedData built from the template (the list's own when apcTemplateName is null) and
    // returns its tile; apCompareFn, when given, keeps the list sorted.
    Tile* AddItem(
        T aAssociatedData,
        const char* apcItemString,
        int32_t (*apCompareFn)(ListBoxItem<T>*, ListBoxItem<T>*),
        const char* apcTemplateName
    );
    // The number of items, filtered ones included. Unsigned, as BarterMenu::DoGamepad compares it unsigned.
    uint32_t NumItems() {
        return BSSimpleList<ListBoxItem<T>*>::ItemsInList();
    }
    // Renumbers the unfiltered items, recomputes the list height and refreshes the scroll bar and highlight.
    void Synchronize();
    // The tile of the item holding aData, or null.
    Tile* GetTileFor(const T& aData);
    // The number of items no filter hides: the item count less the container's _num_filtered trait.
    uint32_t NumUnfiltered();
    // The data of the item at auiIndex (counting the filtered items too when abIncludeFiltered), or a null T.
    T GetDataAt(uint32_t aiIndex, bool abLiteral);
    // Calls apTraversalFn with the tile and data of each item whose list index is in [aiStart, aiEnd); the walk
    // stops at the first empty node or the first index at or past aiEnd.
    void Traverse(void (*apTraversalFn)(Tile*, T), int32_t aiStart, int32_t aiEnd);
    // The data of the highlighted item, or a null T.
    T GetHighlightedData() const;
    // True when apTile is the tile of one of the items.
    bool IsListItem(Tile* apTile) const;
    // The data of the item whose tile is apTile; the instantiations that are called are specialised next to the caller.
    T GetDataFor(Tile* apTile) const;
    // Marks every item apFilterFn rejects as filtered; specialised next to the caller.
    void Filter(FilterFunction apFilterFn);
    // Removes the items whose data belongs to apRemoveObj.
    void RemoveItemGroup(TESBoundObject* apRemoveObj);
    // Fills the list from apContainer's inventory.
    void BuildFromContainer(
        TESObjectREFR* apContainer,
        bool (*apFilterFn)(T),
        int32_t (*apCompareFn)(ListBoxItem<T>*, ListBoxItem<T>*),
        void (*apInitFn)(Tile*, T),
        bool abNoStack,
        bool abBarter
    );
    // Enables or disables the list's container and items.
    void SetEnabled(bool abVal);
    // Clears the selection of every item (only the unfiltered ones when abUnfilteredOnly).
    void DeselectAll(bool abUnfilteredOnly);
    // True when the container exists and its _enabled trait is set.
    bool IsEnabled();
    // True when the container exists and is visible.
    bool IsVisible();
    // The number of items the container shows at once, from its trait (0 without a container).
    uint32_t NumVisible();
    // The list's scroll bar tile.
    Tile* GetScrollBar();
    // Sets or clears the LBF_AUTO_UPDATE flag (the engine's SetFlag(LBF_AUTO_UPDATE, abVal)).
    void SetAutoUpdate(bool abVal);
    // Sets or clears aeFlag in usFlags.
    void SetFlag(Flag aeFlag, bool abOn);
    // True when aeFlag is set in usFlags.
    bool GetFlag(Flag aeFlag);
    // Sets the tile's _y to the list height so far, measures the tile (its height trait, else its first text child's
    // string) and adds it to fTotalListHeight. AddItem calls it with the new item's tile when appending with
    // LBF_AUTO_UPDATE set.
    void CalculateTileHeight(Tile* apTile);
    // Remembers the highlighted index and the scroll bar position in fSavedHighlightIndex and fSavedScrollbarValue.
    void SaveListState();
    // Puts the scroll bar back to the position SaveListState saved.
    void RestoreListState(bool abPlayFocusSound);
    // Stores the highlight index and scroll bar value RestoreListState applies, then restores the list state.
    void SetListState(float afNewHighlightIndex, float afNewScrollbarValue);
    // True when apSelectItem exists and its _selected trait is set.
    bool IsSelected(Tile* apSelectItem);
    // The item at aiIndex: the aiIndex-th node when abLiteral, else the item whose tile's list index is aiIndex; null
    // when there is none.
    ListBoxItem<T>* GetAt(uint32_t aiIndex, bool abLiteral);
    // The first item's tile (GetTileAt(0, false)).
    Tile* GetFirstItem();
    // The last unfiltered item's tile (GetTileAt(NumUnfiltered() - 1, false)).
    Tile* GetLastItem();
    // True when the list holds no item.
    bool IsEmpty();
    // The number of items whose tile is selected.
    uint32_t NumSelected();
    // Unlinks and frees the item of node pkFirst, whose predecessor is pkSecond: moves the highlight off its tile,
    // counts it out of _num_filtered, deletes the tile (and the data when LBF_FROM_CONTAINER), then synchronizes
    // when LBF_AUTO_UPDATE is set; false when pkFirst is null.
    bool Remove(BSSimpleList<ListBoxItem<T>*>* pkFirst, BSSimpleList<ListBoxItem<T>*>* pkSecond);
    // Removes the item GetAt(aiIndex, abLiteral) finds.
    bool RemoveAt(uint32_t aiIndex, bool abLiteral);
    // Removes the item whose tile is apItem; false when there is none.
    bool RemoveItem(Tile* apItem);
    // SetSelection(apSelectItem, LBSM_ON).
    bool Select(Tile* apSelectItem);
    // SetSelection(apSelectItem, LBSM_OFF).
    bool Deselect(Tile* apSelectItem);
    // SetSelection(apSelectItem, LBSM_TOGGLE).
    bool ToggleSelection(Tile* apSelectItem);
    // Sets, clears or flips apSelectItem's _selected trait when the list is enabled and apSelectItem is one of its
    // items; true when the trait changed.
    bool SetSelection(Tile* apSelectItem, SelectMode aeMode);
    // Sets the container's _scroll_delta from the total list height and the visible height, and the scroll bar's
    // _number_of_items.
    void SetTotalListHeight();
    // Points the container's _highlight_y and _selected_height at the highlighted tile and gives the highlight box
    // and its edges the tile's speech challenge failure state; without a highlight, resets both traits.
    void UpdateHighlightBoxPos();
    // Shows or hides the container: its _visible trait (4003) set to abVal.
    void SetVisible(bool abVal);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    Tile* pContainerRect;
    Tile* pHighlightedItem;
    Tile* pScrollBar;
    const char* pTemplateName;
    uint16_t usNextIndex;
    float fTotalListHeight;
    float fSavedHighlightIndex;
    float fSavedScrollbarValue;
    uint16_t usFlags;
};

inline int32_t trait_selected_height_0;
inline int32_t trait_highlight_y_0;
inline int32_t PerkRankDataListBox_IsEnabled_iUT_Enabled;
inline int32_t PerkRankDataListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t PerkRankDataListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t trait_enabled_0;
inline int32_t selected_height;
inline int32_t highlight_y;
inline int32_t trait_enabled;

// The trait ids one instantiation's UpdateHighlightBoxPos creates on first use (function-local statics), and the guard
// word whose bits 0 and 1 mark them created.
inline int32_t trait_selected_height;
inline int32_t trait_highlight_y;
inline int32_t trait_enabled_1;
inline int32_t trait_enabled_2;
inline int32_t TESRecipeComponentListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t TESRecipeComponentListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t TESConditionItemListBox_IsEnabled_iUT_Enabled;
inline int32_t selected_height_0;
inline int32_t highlight_y_0;
inline int32_t ItemChangeListBox_IsEnabled_iUT_Enabled;
inline int32_t trait_enabled_3;
inline int32_t trait_selected_height_2;
inline int32_t trait_highlight_y_2;
inline int32_t IntListBox_IsEnabled_iUT_Enabled;
inline int32_t IntListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t IntListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t BGSPerkListBox_NumUnfiltered_iUT_num_filtered;
inline int32_t BGSPerkListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t BGSPerkListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t TESQuestListBox_SetEnabled_iUT_Enabled;
inline int32_t BGSNoteListBox_SetEnabled_iUT_Enabled;
inline int32_t TESObjectREFRListBox_Filter_iUT_num_filtered;
inline int32_t BGSQuestObjectiveListBox_Filter_iUT_num_filtered;
inline int32_t BGSQuestObjectiveListBox_SetEnabled_iUT_Enabled;
inline int32_t TESChallengeListBox_Filter_iUT_num_filtered;
inline int32_t TESChallengeListBox_SetEnabled_iUT_Enabled;
inline int32_t TESObjectREFRListBox_IsEnabled_iUT_Enabled;
inline int32_t BGSQuestObjectiveListBox_IsEnabled_iUT_Enabled;
inline int32_t TESChallengeListBox_IsEnabled_iUT_Enabled;
inline int32_t TESChallengeListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t TESChallengeListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t BGSSaveLoadFileEntryListBox_Filter_iUT_num_filtered;
inline int32_t BGSSaveLoadFileEntryListBox_NumUnfiltered_iUT_num_filtered;
inline int32_t BGSSaveLoadFileEntryListBox_SetEnabled_iUT_Enabled;
inline int32_t BGSSaveLoadFileEntryListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t BGSSaveLoadFileEntryListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t TESReputationListBox_NumUnfiltered_iUT_num_filtered;
inline int32_t num_filtered;
inline int32_t trait_selected_height_1;
inline int32_t trait_highlight_y_1;
inline int32_t TESRecipeComponentListBox_NumUnfiltered_iUT_num_filtered;
inline int32_t TESConditionItemListBox_NumUnfiltered_iUT_num_filtered;

inline int32_t data_11D8FA0;
inline int32_t TerminalMenuItemListBox_NumUnfiltered_iUT_num_filtered;
inline int32_t BGSPerkListBox_IsEnabled_iUT_Enabled;
inline int32_t BGSPerkListBox_SetSelection_iUT_Selected;
inline int32_t TESQuestListBox_IsEnabled_iUT_Enabled;

// The trait ids ListBox<TESQuest *>::UpdateHighlightBoxPos creates on first use (_selected_height, _highlight_y;
// function-local statics) and their guard word (bits 0 and 1).
inline int32_t TESQuestListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t TESQuestListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t BGSNoteListBox_NumUnfiltered_iUT_num_filtered;
inline int32_t BGSNoteListBox_IsEnabled_iUT_Enabled;

// The same statics of ListBox<BGSNote *>::UpdateHighlightBoxPos.
inline int32_t BGSNoteListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t BGSNoteListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t TESObjectREFRListBox_NumUnfiltered_iUT_num_filtered;
inline int32_t TESObjectREFRListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t TESObjectREFRListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t BGSQuestObjectiveListBox_NumUnfiltered_iUT_num_filtered;
inline int32_t BGSQuestObjectiveListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t BGSQuestObjectiveListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t TESChallengeListBox_NumUnfiltered_iUT_num_filtered;
inline int32_t TESQuestListBox_NumUnfiltered_iUT_num_filtered;

inline int32_t data_11DA4E8;
inline int32_t IntListBox_SetEnabled_iUT_Enabled;
inline int32_t _trait_num_filtered;
inline int32_t BGSNoteListBox_Filter_iUT_num_filtered;

template <class T>
ListBox<T>::ListBox()
  : ListBoxBase(), BSSimpleList<ListBoxItem<T>*>(), pContainerRect(nullptr), pHighlightedItem(nullptr),
    pScrollBar(nullptr), pTemplateName(nullptr), usNextIndex(0), fTotalListHeight(0.0f), fSavedHighlightIndex(0.0f),
    fSavedScrollbarValue(0.0f), usFlags(0) {
    SetFlag(LBF_AUTO_UPDATE, true);
    ListBoxBase::Append(this);
}

template <class T>
ListBox<T>::~ListBox() {
    ListBox<T>::RemoveAll();
    ListBoxBase::Remove(this);
}

// The generic members are defined in ListBox.inl and instantiated in ListBox.cpp only. The lists whose data type is
// nested in another class (ActorValue::Index, MiscStatManager::MiscStatID, StatsMenu::StatusEffect*,
// StartMenu::Option*) cannot be named here; their other users see no member body to instantiate either.
extern template class ListBox<int32_t>;
extern template class ListBox<uint32_t>;
extern template class ListBox<PerkRankData*>;
extern template class ListBox<TESReputation*>;
extern template class ListBox<TESRecipe*>;
extern template class ListBox<TESRecipeComponent*>;
extern template class ListBox<TESConditionItem*>;
extern template class ListBox<ItemChange*>;
extern template class ListBox<TERMINAL_MENU_ITEM*>;
extern template class ListBox<BGSPerk*>;
extern template class ListBox<TESQuest*>;
extern template class ListBox<BGSNote*>;
extern template class ListBox<TESObjectREFR*>;
extern template class ListBox<BGSQuestObjective*>;
extern template class ListBox<TESChallenge*>;
extern template class ListBox<BGSSaveLoadFileEntry*>;
