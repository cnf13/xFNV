#include "Fallout Shared/TESCondition.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/TESConditionItem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESObjectREFR.h"

TESCondition::TESCondition() : BSSimpleList<void*>(nullptr) {}

TESCondition::~TESCondition() {
    ClearAllConditionItems();
}

TESConditionItem* TESCondition::GetConditionItem(int32_t aiIndex) {
    int32_t iCount = 0;
    for (BSSimpleList<void*>* pList = this; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        if (iCount == aiIndex) {
            return static_cast<TESConditionItem*>(pList->m_item);
        }
        ++iCount;
    }
    return nullptr;
}

// The condition is a BSSimpleList<void*>; its items are condition items.
void TESCondition::Copy(TESCondition* apCopy, TESForm* apOwner) {
    ClearAllConditionItems();
    BSSimpleList<void*>* pList = this;
    for (BSSimpleList<void*>* pCopyList = apCopy; pCopyList && !pCopyList->IsEmpty(); pCopyList = pCopyList->m_pkNext) {
        TESConditionItem* pCondItem;
        pCondItem = new TESConditionItem;
        pCondItem->Copy(static_cast<TESConditionItem*>(pCopyList->m_item), apOwner);
        pList->AddTail(pCondItem);
        if (pList->m_pkNext) {
            pList = pList->m_pkNext;
        }
    }
}

void TESCondition::AddItem(TESConditionItem* apConditionItem) {
    AddTail(apConditionItem);
}

bool TESCondition::IsTrue(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef) {
    bool bDispositionFailure = false;
    return IsTrueFull(apActionRef, apTargetRef, &bDispositionFailure, false);
}

// Tests the head for emptiness before any null test of it.
bool TESCondition::IsTrueFull(
    TESObjectREFR* apActionRef,
    TESObjectREFR* apTargetRef,
    bool* apDispositionFailure,
    bool abEssentialPropertiesOnly
) {
    BSSimpleList<void*>* pList = this;
    if (pList->IsEmpty()) {
        *apDispositionFailure = false;
        return true;
    }
    bool bRetVal = true;
    bool bBlockVal = false;
    bool bOrBlock = false;
    bool bDispFailInBlock = false;
    bool bNonDispFailInBlock = false;
    *apDispositionFailure = false;
    bool bNonDispositionFailure = false;
    if (apTargetRef) {
        TESObjectREFR* pXTarg = apTargetRef->GetXTarget();
        if (pXTarg) {
            apTargetRef = pXTarg;
        }
    }
    while (pList) {
        if (pList->IsEmpty()) {
            break;
        }
        TESConditionItem* pCond = static_cast<TESConditionItem*>(pList->m_item);
        bool bPass = false;
        if (abEssentialPropertiesOnly) {
            // The compares of the zero-extended function index are signed.
            int32_t iFunction = pCond->Data.kFunctionData.usFunction;
            if (iFunction < 68 || (iFunction > 72 && iFunction != 372)) {
                bPass = true;
            }
        }
        bool bDispositionItemFailure = false;
        if (bOrBlock) {
            if (!bBlockVal) {
                bool bItemVal = bPass ? bPass : pCond->IsTrue(apActionRef, apTargetRef, bDispositionItemFailure);
                bBlockVal = bItemVal;
                bDispFailInBlock = bDispFailInBlock || bDispositionItemFailure;
                if (!bDispositionItemFailure && !bBlockVal) {
                    bNonDispFailInBlock = true;
                }
            }
            if (!pCond->GetOr()) {
                bOrBlock = false;
                bRetVal = bRetVal && bBlockVal;
                if (!bRetVal) {
                    if (!bDispFailInBlock || bNonDispFailInBlock) {
                        bNonDispositionFailure = true;
                    }
                    else {
                        *apDispositionFailure = true;
                    }
                }
            }
        }
        else {
            if (pCond->GetOr()) {
                bBlockVal = pCond->IsTrue(apActionRef, apTargetRef, bDispositionItemFailure);
                bOrBlock = true;
                bNonDispFailInBlock = false;
                bDispFailInBlock = bDispositionItemFailure;
                if (!bDispositionItemFailure) {
                    bNonDispFailInBlock = !bBlockVal;
                }
            }
            else {
                bBlockVal = bPass || pCond->IsTrue(apActionRef, apTargetRef, bDispositionItemFailure);
                bRetVal = bRetVal && bBlockVal;
                if (bDispositionItemFailure) {
                    *apDispositionFailure = true;
                }
                else if (!bBlockVal) {
                    bNonDispositionFailure = true;
                }
            }
        }
        if (!bRetVal && !bOrBlock && !*apDispositionFailure) {
            break;
        }
        pList = pList->m_pkNext;
    }
    if (bOrBlock) {
        bRetVal = bRetVal && bBlockVal;
        if (!bRetVal) {
            if (!bDispFailInBlock || bNonDispFailInBlock) {
                bNonDispositionFailure = true;
            }
            else {
                *apDispositionFailure = true;
            }
        }
    }
    if (bRetVal || bNonDispositionFailure) {
        *apDispositionFailure = false;
    }
    return bRetVal;
}

void TESCondition::Load(TESFile* apFile) {
    if (apFile && apFile->GetTESChunk() == CIDA_ID) {
        TESConditionItem* pCondItem;
        pCondItem = new TESConditionItem;
        pCondItem->Load(apFile);
        AddItem(pCondItem);
    }
}

void TESCondition::Save() {
    // The condition is a BSSimpleList<void*>; its items are condition items.
    for (BSSimpleList<void*>* pList = this; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        static_cast<TESConditionItem*>(pList->m_item)->Save();
    }
}

void TESCondition::InitItem(TESForm* apOwner) {
    for (BSSimpleList<void*>* pNode = this; pNode != nullptr && !pNode->IsEmpty(); pNode = pNode->GetNext()) {
        static_cast<TESConditionItem*>(pNode->GetItem())->InitItem(apOwner);
    }
}

void TESCondition::ClearAllConditionItems() {
    BSSimpleList<void*>* pHead = this;
    while (pHead != nullptr && !pHead->IsEmpty()) {
        TESConditionItem* pItem = static_cast<TESConditionItem*>(pHead->GetItem());
        if (pItem != nullptr) {
            delete pItem;
        }
        pHead->RemoveHead();
    }
}

bool TESCondition::Compare(TESCondition* apCompare) {
    BSSimpleList<void*>* pList = this;
    for (BSSimpleList<void*>* pCompList = apCompare; pCompList && !pCompList->IsEmpty();
         pCompList = pCompList->m_pkNext) {
        if (!pList || pList->IsEmpty()) {
            return true;
        }
        TESConditionItem* pItem = static_cast<TESConditionItem*>(pList->m_item);
        if (pItem->Compare(static_cast<TESConditionItem*>(pCompList->m_item))) {
            return true;
        }
        pList = pList->m_pkNext;
    }
    return pList && !pList->IsEmpty();
}

// The comparisons read the second value against the first (afVal2 <op> afVal1); the
// unordered case reads as true everywhere except CC_EQUAL, following the FPU flags.
bool TESCondition::CheckValue(ENUM_COMPARISON_CONDITION aiCondition, float afVal1, float afVal2) {
    const bool bUnordered = afVal1 != afVal1 || afVal2 != afVal2;
    switch (aiCondition) {
        case CC_EQUAL: return afVal2 == afVal1;
        case CC_NOT_EQUAL: return afVal2 != afVal1;
        case CC_GREATER_THAN: return bUnordered || afVal2 < afVal1;
        case CC_GREATER_THAN_EQUAL: return bUnordered || afVal2 <= afVal1;
        case CC_LESS_THAN: return bUnordered || afVal2 > afVal1;
        case CC_LESS_THAN_EQUAL: return bUnordered || afVal2 >= afVal1;
        default: return true;
    }
}
