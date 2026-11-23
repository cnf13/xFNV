// Kept out of ListBox.h: the bodies need Tile, Menu and InterfaceManager, whose headers include ListBox.h back.
#pragma once

// The generic members, instantiated in ListBox.cpp for every list.
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <cmath>
#include <new>
#include <type_traits>
#include <limits>

template <class T>
bool ListBox<T>::Highlight(Tile* apHighlightItem) {
    if (!IsEnabled()) {
        return false;
    }
    if (apHighlightItem &&
        (!IsListItem(apHighlightItem) || !apHighlightItem->IsTrue(Tile::AddUserTrait("_enabled", -1)))) {
        return false;
    }
    pHighlightedItem = apHighlightItem;
    UpdateHighlightBoxPos();
    return true;
}

template <class T>
Tile* ListBox<T>::GetHighlightTarget(int32_t aeButton) {
    // The step: -1 for up (1), +1 for down (2), a page of visible rows up (15) or down (16).
    int32_t iDelta = 0;
    switch (aeButton) {
        case 1: iDelta = -1; break;
        case 2: iDelta = 1; break;
        case 15:
            if (pContainerRect) {
                double dHeight = -pContainerRect->GetFloat(Tile::kTileValue_Height);
                float fRows = dHeight / pContainerRect->GetFloat(Tile::TextToTrait("_scroll_delta"));
                iDelta = static_cast<int32_t>(BSRoundToNearest(fRows, 1.0f));
            }
            break;
        case 16:
            if (pContainerRect) {
                double dHeight = pContainerRect->GetFloat(Tile::kTileValue_Height);
                float fRows = dHeight / pContainerRect->GetFloat(Tile::TextToTrait("_scroll_delta"));
                iDelta = static_cast<int32_t>(BSRoundToNearest(fRows, 1.0f));
            }
            break;
        default: break;
    }
    Tile* pTarget = nullptr;
    if (IsEnabled() && iDelta && usNextIndex) {
        int32_t iMaxIndex = usNextIndex - 1;
        int32_t iNewIndex;
        if (pHighlightedItem) {
            iNewIndex = iDelta + static_cast<int32_t>(pHighlightedItem->GetFloat(Tile::kTileValue_ListIndex));
            if (iNewIndex < 0) {
                iNewIndex = 0;
            }
            else if (iNewIndex > iMaxIndex) {
                iNewIndex = iMaxIndex;
            }
        }
        else {
            iNewIndex = iDelta <= 0 ? usNextIndex - 1 : 0;
        }
        // Steps over disabled items; past an end, takes the enabled item nearest to that end.
        while (true) {
            for (const BSSimpleList<ListBoxItem<T>*>* pNode = this; !pTarget && pNode && pNode->m_item;
                 pNode = pNode->m_pkNext) {
                if (static_cast<double>(iNewIndex) == pNode->m_item->pTile->GetFloat(Tile::kTileValue_ListIndex)) {
                    pTarget = pNode->m_item->pTile;
                }
            }
            if (!pTarget || pTarget->IsTrue(Tile::AddUserTrait("_enabled", -1))) {
                break;
            }
            pTarget = nullptr;
            if (iDelta > 0 && ++iNewIndex > iMaxIndex) {
                for (int32_t iIndex = iMaxIndex - 1;; --iIndex) {
                    if (pTarget && pTarget->IsTrue(Tile::AddUserTrait("_enabled", -1))) {
                        break;
                    }
                    if (iIndex < 0) {
                        break;
                    }
                    pTarget = GetTileAt(static_cast<uint32_t>(iIndex), false);
                }
                break;
            }
            if (iDelta < 0 && --iNewIndex < 0) {
                for (int32_t iIndex = 1;; ++iIndex) {
                    if (pTarget && pTarget->IsTrue(Tile::AddUserTrait("_enabled", -1))) {
                        break;
                    }
                    if (iIndex > iMaxIndex) {
                        break;
                    }
                    pTarget = GetTileAt(static_cast<uint32_t>(iIndex), false);
                }
                break;
            }
        }
    }
    return pHighlightedItem != pTarget ? pTarget : nullptr;
}

template <class T>
bool ListBox<T>::BelongsTo(Menu* apMenu) {
    return pContainerRect && apMenu == pContainerRect->GetMenu();
}

template <class T>
void ListBox<T>::ScrollToHighlight() {
    if (!pHighlightedItem || !pScrollBar || !pContainerRect ||
        !(pContainerRect->GetFloat(Tile::kTileValue_Height) > 0.0)) {
        return;
    }
    bool bAbove = pHighlightedItem->GetFloat(Tile::kTileValue_Y) < 0.0;
    double dBottom =
        pHighlightedItem->GetFloat(Tile::kTileValue_Height) + pHighlightedItem->GetFloat(Tile::kTileValue_Y);
    bool bBelow = pContainerRect->GetFloat(Tile::kTileValue_Height) < dBottom;
    bool bScrolled = bAbove || bBelow;
    while (pHighlightedItem->GetFloat(Tile::kTileValue_ListIndex) >= 0.0 && (bAbove || bBelow)) {
        float fOld = pScrollBar->GetFloat(Tile::TextToTrait("_current_value"));
        float fNew = static_cast<double>(bAbove ? -1 : 1) + fOld;
        pScrollBar->Set(Tile::TextToTrait("_current_value"), fNew, false);
        if (fOld == pScrollBar->GetFloat(Tile::TextToTrait("_current_value"))) {
            Warning(
                "MENUS: The scrollbar update in ScrollToHighlight() has hit an infinite loop trying to scroll to "
                "an "
                "impossible value."
            );
            break;
        }
        bAbove = pHighlightedItem->GetFloat(Tile::kTileValue_Y) < 0.0;
        dBottom = pHighlightedItem->GetFloat(Tile::kTileValue_Height) + pHighlightedItem->GetFloat(Tile::kTileValue_Y);
        bBelow = pContainerRect->GetFloat(Tile::kTileValue_Height) < dBottom;
    }
    if (bScrolled) {
        pScrollBar->Poke(Tile::TextToTrait("_SetInCode"), 1.0f);
    }
}

template <class T>
Tile* ListBox<T>::GetTileAt(uint32_t aiIndex, bool abLiteral) {
    ListBoxItem<T>* pItem = GetAt(aiIndex, abLiteral);
    return pItem ? pItem->pTile : nullptr;
}

template <class T>
void ListBox<T>::RemoveAll() {
    while (this->GetNext() && this->GetNext()->m_item) {
        BSSimpleList<ListBoxItem<T>*>* pAfter = this->GetNext()->m_pkNext;
        if (!bClosingAllSupportMenus) {
            delete this->GetNext()->m_item->pTile;
        }
        // Only a list BuildFromContainer filled owns its data, and it is deleted as an ItemChange; a data value that is
        // not a pointer is taken as one.
        if (GetFlag(LBF_FROM_CONTAINER)) {
            if constexpr (std::is_pointer_v<T>) {
                delete static_cast<ItemChange*>(static_cast<void*>(this->GetNext()->m_item->data));
            }
            else {
                delete reinterpret_cast<ItemChange*>(static_cast<uintptr_t>(this->GetNext()->m_item->data));
            }
        }
        ourFree(this->GetNext()->m_item);
        this->GetNext()->m_pkNext = nullptr;
        delete this->GetNext();
        this->m_pkNext = pAfter;
    }
    if (this->m_item) {
        if (!bClosingAllSupportMenus) {
            delete this->m_item->pTile;
        }
        if (GetFlag(LBF_FROM_CONTAINER)) {
            if constexpr (std::is_pointer_v<T>) {
                delete static_cast<ItemChange*>(static_cast<void*>(this->m_item->data));
            }
            else {
                delete reinterpret_cast<ItemChange*>(static_cast<uintptr_t>(this->m_item->data));
            }
        }
        ourFree(this->m_item);
    }
    this->m_item = nullptr;
    usNextIndex = 0;
    fTotalListHeight = 0.0f;
    if (pContainerRect) {
        pContainerRect->Set(Tile::TextToTrait("_num_filtered"), 0.0f, true);
    }
    SetTotalListHeight();
    pHighlightedItem = nullptr;
    UpdateHighlightBoxPos();
}

template <class T>
void ListBox<T>::Sort(int32_t (*apCompareFn)(ListBoxItem<T>*, ListBoxItem<T>*)) {
    BSSimpleList<ListBoxItem<T>*>::Sort(apCompareFn);
    Synchronize();
}

template <class T>
void ListBox<T>::Initialize(Tile* apContainerRect, const char* apTemplateName) {
    pContainerRect = apContainerRect;
    pTemplateName = apTemplateName;
    pScrollBar = Tile::GetTileByName(pContainerRect, "child(lb_scrollbar)");
}

template <class T>
Tile* ListBox<T>::AddItem(
    T aAssociatedData,
    const char* apcItemString,
    int32_t (*apCompareFn)(ListBoxItem<T>*, ListBoxItem<T>*),
    const char* apcTemplateName
) {
    if (!pContainerRect || (!pTemplateName && !apcTemplateName)) {
        return nullptr;
    }
    const char* pcTemplateName = apcTemplateName ? apcTemplateName : pTemplateName;
    Tile* pTile = pContainerRect->GetMenu()->RenderTemplate(pContainerRect, pcTemplateName, nullptr);
    if (!pTile->GetValueQ(Tile::kTileValue_ID)) {
        pTile->Set(Tile::kTileValue_ID, -1);
    }
    if (apcItemString) {
        pTile->Set(Tile::kTileValue_String, apcItemString, true);
    }
    ListBoxItem<T>* pItem = new ListBoxItem<T>(pTile, &aAssociatedData);
    if (apCompareFn) {
        BSSimpleList<ListBoxItem<T>*>::Insert(pItem, apCompareFn);
        if (GetFlag(LBF_AUTO_UPDATE)) {
            Synchronize();
        }
    }
    else {
        BSSimpleList<ListBoxItem<T>*>::AddTail(pItem);
        if (GetFlag(LBF_AUTO_UPDATE)) {
            CalculateTileHeight(pTile);
            SetTotalListHeight();
        }
        pTile->Set(Tile::kTileValue_ListIndex, static_cast<int32_t>(usNextIndex++));
    }
    // The first item sizes the container: its height times the number of visible items.
    if (NumItems() == 1 && pContainerRect->GetFloat(Tile::AddUserTrait("_number_of_visible_items", -1)) > 0.0) {
        Tile::Value* pValue = pContainerRect->GetValue(Tile::kTileValue_Height);
        pValue->ClearActions();
        pValue->AddAction(Tile::kTileValue_Copy, pTile, Tile::kTileValue_Height);
        pValue->AddAction(
            Tile::kTileValue_Mul, pContainerRect->GetFloat(Tile::AddUserTrait("_number_of_visible_items", -1))
        );
        pValue->CalculateValue(false);
    }
    return pTile;
}

template <class T>
void ListBox<T>::Synchronize() {
    usNextIndex = 0;
    fTotalListHeight = 0.0f;
    for (ListBoxItem<T>*& rpItem : static_cast<BSSimpleList<ListBoxItem<T>*>*>(this)->ItemsUntilNull()) {
        if (!rpItem->bFiltered) {
            rpItem->pTile->Set(Tile::kTileValue_ListIndex, static_cast<int32_t>(usNextIndex++));
            CalculateTileHeight(rpItem->pTile);
        }
    }
    SetTotalListHeight();
    UpdateHighlightBoxPos();
}

template <class T>
Tile* ListBox<T>::GetTileFor(const T& aData) {
    for (ListBoxItem<T>*& rpItem : static_cast<BSSimpleList<ListBoxItem<T>*>*>(this)->ItemsUntilNull()) {
        if (rpItem->data == aData) {
            return rpItem->pTile;
        }
    }
    return nullptr;
}

template <class T>
uint32_t ListBox<T>::NumUnfiltered() {
    static const int32_t iNumFilteredTrait = Tile::AddUserTrait("_num_filtered", -1);
    int32_t iItems = NumItems();
    return static_cast<uint32_t>(iItems - static_cast<int64_t>(pContainerRect->GetFloat(iNumFilteredTrait)));
}

template <class T>
T ListBox<T>::GetDataAt(uint32_t aiIndex, bool abLiteral) {
    ListBoxItem<T>* pItem = GetAt(aiIndex, abLiteral);
    return pItem ? pItem->data : T();
}

template <class T>
void ListBox<T>::Traverse(void (*apTraversalFn)(Tile*, T), int32_t aiStart, int32_t aiEnd) {
    for (BSSimpleList<ListBoxItem<T>*>* pNode = this; pNode && pNode->m_item &&
         static_cast<int32_t>(pNode->m_item->pTile->GetFloat(Tile::kTileValue_ListIndex)) < aiEnd;
         pNode = pNode->m_pkNext) {
        if (static_cast<int32_t>(pNode->m_item->pTile->GetFloat(Tile::kTileValue_ListIndex)) >= aiStart) {
            apTraversalFn(pNode->m_item->pTile, pNode->m_item->data);
        }
    }
}

template <class T>
T ListBox<T>::GetHighlightedData() const {
    return GetDataFor(pHighlightedItem);
}

template <class T>
bool ListBox<T>::IsListItem(Tile* apTile) const {
    for (ListBoxItem<T>* const& rpItem : static_cast<const BSSimpleList<ListBoxItem<T>*>*>(this)->ItemsUntilNull()) {
        if (rpItem->pTile == apTile) {
            return true;
        }
    }
    return false;
}

template <class T>
T ListBox<T>::GetDataFor(Tile* apTile) const {
    if (!apTile) {
        return T();
    }
    for (ListBoxItem<T>* const& rpItem : static_cast<const BSSimpleList<ListBoxItem<T>*>*>(this)->ItemsUntilNull()) {
        if (rpItem->pTile == apTile) {
            return rpItem->data;
        }
    }
    return T();
}

template <class T>
void ListBox<T>::Filter(FilterFunction apFilterFn) {
    static const int32_t iNumFilteredTrait = Tile::AddUserTrait("_num_filtered", -1);
    uint32_t uiFilterCount = 0;
    int32_t iSaveHLIndex = -1;
    usNextIndex = 0;
    fTotalListHeight = 0.0f;
    for (ListBoxItem<T>*& rpItem : static_cast<BSSimpleList<ListBoxItem<T>*>*>(this)->ItemsUntilNull()) {
        rpItem->bFiltered = apFilterFn && apFilterFn(rpItem->data);
        if (rpItem->bFiltered) {
            if (rpItem->pTile == pHighlightedItem) {
                iSaveHLIndex = static_cast<int32_t>(rpItem->pTile->GetFloat(Tile::kTileValue_ListIndex));
                pHighlightedItem = nullptr;
            }
            rpItem->pTile->Set(Tile::kTileValue_ListIndex, -1);
            ++uiFilterCount;
        }
        else {
            if (iSaveHLIndex >= 0 && usNextIndex == iSaveHLIndex) {
                pHighlightedItem = rpItem->pTile;
            }
            rpItem->pTile->Set(Tile::kTileValue_ListIndex, static_cast<int32_t>(usNextIndex++));
            CalculateTileHeight(rpItem->pTile);
        }
    }
    pContainerRect->Set(iNumFilteredTrait, uiFilterCount);
    SetTotalListHeight();
    if (!pHighlightedItem && iSaveHLIndex >= 0) {
        pHighlightedItem = GetTileAt(static_cast<uint32_t>(iSaveHLIndex), false);
    }
    if (!pHighlightedItem && iSaveHLIndex >= 0) {
        pHighlightedItem = GetLastItem();
    }
    UpdateHighlightBoxPos();
}

template <class T>
void ListBox<T>::SetEnabled(bool abVal) {
    static const int32_t iEnabledTrait = Tile::AddUserTrait("_enabled", -1);
    pContainerRect->Set(iEnabledTrait, static_cast<int32_t>(abVal));
}

template <class T>
void ListBox<T>::DeselectAll(bool abUnfilteredOnly) {
    for (ListBoxItem<T>*& rpItem : static_cast<BSSimpleList<ListBoxItem<T>*>*>(this)->ItemsUntilNull()) {
        if (!abUnfilteredOnly || !rpItem->bFiltered) {
            Deselect(rpItem->pTile);
        }
    }
}

template <class T>
bool ListBox<T>::IsEnabled() {
    static const int32_t iEnabledTrait = Tile::AddUserTrait("_enabled", -1);
    return pContainerRect && pContainerRect->IsTrue(iEnabledTrait);
}

template <class T>
bool ListBox<T>::IsVisible() {
    return pContainerRect && pContainerRect->IsVisible();
}

template <class T>
uint32_t ListBox<T>::NumVisible() {
    if (!pContainerRect) {
        return 0;
    }
    return static_cast<uint32_t>(
        static_cast<int64_t>(pContainerRect->GetFloat(Tile::AddUserTrait("_number_of_visible_items", -1)))
    );
}

template <class T>
Tile* ListBox<T>::GetScrollBar() {
    return pScrollBar;
}

template <class T>
void ListBox<T>::SetAutoUpdate(bool abVal) {
    SetFlag(LBF_AUTO_UPDATE, abVal);
}

template <class T>
void ListBox<T>::SetFlag(Flag aeFlag, bool abOn) {
    if (abOn) {
        usFlags |= 1 << aeFlag;
    }
    else {
        usFlags &= ~(1 << aeFlag);
    }
}

template <class T>
bool ListBox<T>::GetFlag(Flag aeFlag) {
    return (usFlags & (1 << aeFlag)) != 0;
}

template <class T>
void ListBox<T>::CalculateTileHeight(Tile* apTile) {
    apTile->Set(Tile::AddUserTrait("_y", -1), fTotalListHeight, true);
    float fTileHeight = apTile->GetFloat(Tile::kTileValue_Height);
    if (fTileHeight == 0.0) {
        // A tile without a height takes its last text child's: the text's height plus the tile's spacing.
        Tile* pTextTile = nullptr;
        NiTListItem<Tile*>* pPos = apTile->xChildren.m_pkTail;
        while (pPos && !pTextTile) {
            Tile* pChild = apTile->xChildren.GetPrev(pPos);
            if (pChild->GetType() == Tile::kTileValue_Text) {
                pTextTile = pChild;
            }
        }
        if (pTextTile) {
            float fWrapWidth = pTextTile->GetFloat(Tile::kTileValue_WrapWidth);
            if (!(fWrapWidth > 0.0)) {
                fWrapWidth = std::numeric_limits<float>::max();
            }
            int32_t iFont = static_cast<int32_t>(pTextTile->GetFloat(Tile::kTileValue_Font));
            const char* pcText = pTextTile->GetString(Tile::kTileValue_String);
            fTileHeight = FontManager::Instance()->CalculateStringDimensions(pcText, iFont, fWrapWidth, 0).y;
            fTileHeight = apTile->GetFloat(Tile::TextToTrait("_VerticalSpacing")) + fTileHeight;
        }
    }
    apTile->Set(Tile::kTileValue_Height, fTileHeight, false);
    fTotalListHeight = fTotalListHeight + fTileHeight;
}

template <class T>
void ListBox<T>::SaveListState() {
    fSavedHighlightIndex = pHighlightedItem ? pHighlightedItem->GetFloat(Tile::kTileValue_ListIndex) : -1.0f;
    fSavedScrollbarValue = pScrollBar ? pScrollBar->GetFloat(Tile::TextToTrait("_current_value")) : -1.0f;
}

template <class T>
void ListBox<T>::RestoreListState(bool abPlayFocusSound) {
    if (!pContainerRect || !pScrollBar) {
        return;
    }
    pScrollBar->Set(Tile::TextToTrait("_current_value"), fSavedScrollbarValue, false);
    pScrollBar->Poke(Tile::TextToTrait("_SetInCode"), 1.0f);
    float fLastIndex = static_cast<double>(NumUnfiltered()) - 1.0;
    Tile* pNewHighlight =
        GetTileAt(static_cast<uint32_t>(static_cast<int64_t>(BSMIN(fSavedHighlightIndex, fLastIndex))), false);
    InterfaceManager::Instance()->SetCurrentFocusTarget(pNewHighlight, Tile::kTileValue_Mouseover, abPlayFocusSound);
    Highlight(pNewHighlight);
    // Without a controller, the menu gets the leave event of the new highlight and the highlight is dropped.
    if (!InterfaceManager::Has360Controller()) {
        if (pNewHighlight) {
            Menu* pMenu = pContainerRect->GetMenuTile()->GetMenu();
            pMenu->DoLeave(static_cast<int32_t>(pNewHighlight->GetFloat(Tile::kTileValue_ID)), pNewHighlight);
        }
        Highlight(nullptr);
    }
}

template <class T>
void ListBox<T>::SetListState(float afNewHighlightIndex, float afNewScrollbarValue) {
    fSavedHighlightIndex = afNewHighlightIndex;
    fSavedScrollbarValue = afNewScrollbarValue;
    RestoreListState(true);
}

template <class T>
bool ListBox<T>::IsSelected(Tile* apSelectItem) {
    static const int32_t iSelectedTrait = Tile::AddUserTrait("_selected", -1);
    return iSelectedTrait && apSelectItem && apSelectItem->IsTrue(iSelectedTrait);
}

template <class T>
ListBoxItem<T>* ListBox<T>::GetAt(uint32_t aiIndex, bool abLiteral) {
    if (BSSimpleList<ListBoxItem<T>*>::IsEmpty()) {
        return nullptr;
    }
    const BSSimpleList<ListBoxItem<T>*>* pNode = this;
    if (abLiteral) {
        uint32_t uiCount = 0;
        while (pNode && uiCount++ != aiIndex) {
            pNode = pNode->m_pkNext;
        }
    }
    else {
        while (pNode && static_cast<double>(aiIndex) != pNode->m_item->pTile->GetFloat(Tile::kTileValue_ListIndex)) {
            pNode = pNode->m_pkNext;
        }
    }
    return pNode ? pNode->m_item : nullptr;
}

template <class T>
Tile* ListBox<T>::GetFirstItem() {
    return GetTileAt(0, false);
}

template <class T>
Tile* ListBox<T>::GetLastItem() {
    return GetTileAt(NumUnfiltered() - 1, false);
}

template <class T>
bool ListBox<T>::IsEmpty() {
    return BSSimpleList<ListBoxItem<T>*>::IsEmpty();
}

template <class T>
uint32_t ListBox<T>::NumSelected() {
    uint32_t uiCount = 0;
    for (ListBoxItem<T>*& rpItem : static_cast<BSSimpleList<ListBoxItem<T>*>*>(this)->ItemsUntilNull()) {
        if (IsSelected(rpItem->pTile)) {
            ++uiCount;
        }
    }
    return uiCount;
}

template <class T>
bool ListBox<T>::Remove(BSSimpleList<ListBoxItem<T>*>* pkFirst, BSSimpleList<ListBoxItem<T>*>* pkSecond) {
    if (!pkFirst) {
        return false;
    }
    // The highlight moves to the next item, else to the previous one.
    if (pkFirst->m_item->pTile == pHighlightedItem) {
        if (pkFirst->m_pkNext) {
            pHighlightedItem = pkFirst->m_pkNext->m_item->pTile;
        }
        else {
            pHighlightedItem = (!pkSecond || pkSecond == pkFirst) ? nullptr : pkSecond->m_item->pTile;
        }
        if (pHighlightedItem) {
            InterfaceManager::Instance()->SetCurrentFocusTarget(pHighlightedItem, Tile::kTileValue_Mouseover, true);
        }
    }
    if (pContainerRect && pkFirst->m_item->bFiltered) {
        float fFiltered = pContainerRect->GetFloat(Tile::TextToTrait("_num_filtered")) - 1.0;
        pContainerRect->Set(Tile::TextToTrait("_num_filtered"), fFiltered, true);
    }
    delete pkFirst->m_item->pTile;
    // Only a list BuildFromContainer filled owns its data, and it is deleted as an ItemChange; a data value that is not
    // a pointer is taken as one.
    if (GetFlag(LBF_FROM_CONTAINER)) {
        if constexpr (std::is_pointer_v<T>) {
            delete static_cast<ItemChange*>(static_cast<void*>(pkFirst->m_item->data));
        }
        else {
            delete reinterpret_cast<ItemChange*>(static_cast<uintptr_t>(pkFirst->m_item->data));
        }
    }
    ourFree(pkFirst->m_item);
    // The head node is the list itself: the next node's item moves into it and that node is freed.
    if (pkFirst == static_cast<BSSimpleList<ListBoxItem<T>*>*>(this)) {
        if (BSSimpleList<ListBoxItem<T>*>* pNext = this->GetNext()) {
            this->m_pkNext = pNext->m_pkNext;
            this->m_item = pNext->m_item;
            pNext->m_pkNext = nullptr;
            delete pNext;
        }
        else {
            this->m_item = nullptr;
        }
    }
    else {
        pkSecond->m_pkNext = pkFirst->m_pkNext;
        pkFirst->m_pkNext = nullptr;
        delete pkFirst;
    }
    if (GetFlag(LBF_AUTO_UPDATE)) {
        Synchronize();
    }
    return true;
}

template <class T>
bool ListBox<T>::RemoveAt(uint32_t aiIndex, bool abLiteral) {
    if (BSSimpleList<ListBoxItem<T>*>::IsEmpty()) {
        return false;
    }
    BSSimpleList<ListBoxItem<T>*>* pNode = this;
    BSSimpleList<ListBoxItem<T>*>* pPrev = this;
    if (abLiteral) {
        uint32_t uiCount = 0;
        while (pNode && uiCount++ != aiIndex) {
            pPrev = pNode;
            pNode = pNode->m_pkNext;
        }
    }
    else {
        while (pNode && static_cast<double>(aiIndex) != pNode->m_item->pTile->GetFloat(Tile::kTileValue_ListIndex)) {
            pPrev = pNode;
            pNode = pNode->m_pkNext;
        }
    }
    return Remove(pNode, pPrev);
}

template <class T>
bool ListBox<T>::RemoveItem(Tile* apItem) {
    if (!apItem || BSSimpleList<ListBoxItem<T>*>::IsEmpty()) {
        return false;
    }
    BSSimpleList<ListBoxItem<T>*>* pNode = this;
    BSSimpleList<ListBoxItem<T>*>* pPrev = this;
    while (pNode && pNode->m_item->pTile != apItem) {
        pPrev = pNode;
        pNode = pNode->m_pkNext;
    }
    return Remove(pNode, pPrev);
}

template <class T>
bool ListBox<T>::Select(Tile* apSelectItem) {
    return SetSelection(apSelectItem, LBSM_ON);
}

template <class T>
bool ListBox<T>::Deselect(Tile* apSelectItem) {
    return SetSelection(apSelectItem, LBSM_OFF);
}

template <class T>
bool ListBox<T>::ToggleSelection(Tile* apSelectItem) {
    return SetSelection(apSelectItem, LBSM_TOGGLE);
}

template <class T>
bool ListBox<T>::SetSelection(Tile* apSelectItem, SelectMode aeMode) {
    if (!IsEnabled() || !apSelectItem || !IsListItem(apSelectItem)) {
        return false;
    }
    static const int32_t iSelectedTrait = Tile::AddUserTrait("_selected", -1);
    bool bSelected = apSelectItem->IsTrue(iSelectedTrait);
    switch (aeMode) {
        case LBSM_ON:
            if (!bSelected) {
                apSelectItem->Set(iSelectedTrait, 1);
                return true;
            }
            break;
        case LBSM_OFF:
            if (bSelected) {
                apSelectItem->Set(iSelectedTrait, 0);
                return true;
            }
            break;
        case LBSM_TOGGLE: apSelectItem->Set(iSelectedTrait, static_cast<int32_t>(!bSelected)); return true;
    }
    return false;
}

template <class T>
void ListBox<T>::SetTotalListHeight() {
    if (!pContainerRect) {
        return;
    }
    // The scroll bar gets one position per page of the list's overflow, and the container the height of a step.
    float fListHeight = pContainerRect->GetFloat(Tile::kTileValue_Height);
    int32_t iNumPages = 1;
    if (fTotalListHeight > 0.0 && fListHeight > 0.0) {
        if (fListHeight < static_cast<double>(fTotalListHeight)) {
            float fPages = static_cast<double>(NumUnfiltered()) * (fTotalListHeight - fListHeight) / fTotalListHeight;
            iNumPages = BSMAX(static_cast<int32_t>(std::ceil(static_cast<double>(fPages))) + 1, 2);
        }
        if (iNumPages > 1) {
            float fDelta = (fTotalListHeight - fListHeight) / static_cast<double>(iNumPages - 1);
            pContainerRect->Set(Tile::TextToTrait("_scroll_delta"), fDelta, true);
        }
    }
    if (pScrollBar) {
        pScrollBar->Set(Tile::TextToTrait("_number_of_items"), iNumPages);
    }
}

template <class T>
void ListBox<T>::UpdateHighlightBoxPos() {
    static const int32_t iHighlightYTrait = Tile::AddUserTrait("_highlight_y", -1);
    static const int32_t iSelectedHeightTrait = Tile::AddUserTrait("_selected_height", -1);
    if (!pContainerRect) {
        return;
    }
    if (!pHighlightedItem) {
        pContainerRect->Set(iHighlightYTrait, -1);
        pContainerRect->Set(iSelectedHeightTrait, 0);
        return;
    }
    Tile::Value* pHighlightY = pContainerRect->GetValueQ(iHighlightYTrait);
    Tile::Value* pSelectedHeight = pContainerRect->GetValueQ(iSelectedHeightTrait);
    pHighlightY->AddAction(Tile::kTileValue_Copy, pHighlightedItem, Tile::kTileValue_Y);
    pHighlightY->CalculateValue(false);
    pSelectedHeight->AddAction(Tile::kTileValue_Copy, pHighlightedItem, Tile::kTileValue_Height);
    pSelectedHeight->CalculateValue(false);
    // The highlight box and its four edges show the highlighted tile's speech challenge failure state.
    bool bFailure = pHighlightedItem->GetSpeechChallengeFailure();
    Tile* pBox = Tile::GetTileByName(pContainerRect, "lb_highlight_box");
    if (!pBox) {
        return;
    }
    if (Tile* pTop = pBox->GetChildByName("top")) {
        pTop->SetSpeechChallengeFailure(bFailure);
        pTop->AddNeedsUpdate(8);
        Tile* pBottom = pBox->GetChildByName("bottom");
        pBottom->SetSpeechChallengeFailure(bFailure);
        pBottom->AddNeedsUpdate(8);
        Tile* pLeft = pBox->GetChildByName("left");
        pLeft->SetSpeechChallengeFailure(bFailure);
        pLeft->AddNeedsUpdate(8);
        Tile* pRight = pBox->GetChildByName("right");
        pRight->SetSpeechChallengeFailure(bFailure);
        pRight->AddNeedsUpdate(8);
    }
    pBox->SetSpeechChallengeFailure(bFailure);
    pBox->AddNeedsUpdate(8);
}

template <class T>
void ListBox<T>::SetVisible(bool abVal) {
    pContainerRect->Set(Tile::kTileValue_Visible, static_cast<int32_t>(abVal));
}
