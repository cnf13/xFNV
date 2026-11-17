#include "Fallout Shared/TESCondition.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/TESConditionItem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "SDK/MSVC/ehvec.h"

// BSSimpleList<void*>::IsEmpty: a node is empty when it holds no item and no next node.
static bool BSSimpleList_IsEmpty(const BSSimpleList<void*>* apList) {
	return apList->GetItem() == nullptr && apList->GetNext() == nullptr;
}

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

// The condition is src's BSSimpleList<void*>; its items are condition items. The exe's cleanup-only C++ EH frame is
// left out.
void TESCondition::Copy(TESCondition* apCopy, TESForm* apOwner) {
	ClearAllConditionItems();
	BSSimpleList<void*>* pList = this;
	for (BSSimpleList<void*>* pCopyList = apCopy; pCopyList && !pCopyList->IsEmpty(); pCopyList = pCopyList->m_pkNext) {
		TESConditionItem* pCondItem;
		void* pMemory = ourMalloc(sizeof(TESConditionItem));
		if (pMemory) {
			pCondItem = ::new (pMemory) TESConditionItem;
		} else {
			pCondItem = nullptr;
		}
		void* pNewItem = pCondItem;
		pCondItem->Copy(static_cast<TESConditionItem*>(pCopyList->m_item), apOwner);
		pList->AddTail(pNewItem);
		if (pList->m_pkNext) {
			pList = pList->m_pkNext;
		}
	}
}

// The exe hands the address of its argument slot to the shared pointer-list AddTail (0x905820).
void TESCondition::AddItem(TESConditionItem* apConditionItem) {
	void* pItem = apConditionItem;
	AddTail(pItem);
}

bool TESCondition::IsTrue(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef) {
	bool bDispositionFailure = false;
	return IsTrueFull(apActionRef, apTargetRef, &bDispositionFailure, false);
}

// The exe tests the head for emptiness before any null test of it.
bool TESCondition::IsTrueFull(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef, bool* apDispositionFailure,
	bool abEssentialPropertiesOnly) {
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
		TESObjectREFR* pxtarg = apTargetRef->GetXTarget();
		if (pxtarg) {
			apTargetRef = pxtarg;
		}
	}
	while (pList) {
		if (pList->IsEmpty()) {
			break;
		}
		TESConditionItem* pCond = static_cast<TESConditionItem*>(pList->m_item);
		bool bPass = false;
		if (abEssentialPropertiesOnly) {
			// Signed compares (jl/jle) of the zero-extended function index.
			int32_t iFunction = pCond->Data.kFunctionData.usFunction;
			if (iFunction < 68 || (iFunction > 72 && iFunction != 372)) {
				bPass = true;
			}
		}
		bool bDispositionItemFailure = false;
		if (bOrBlock) {
			if (!bBlockVal) {
				bool bItemVal;
				if (bPass) {
					bItemVal = bPass;
				} else {
					bItemVal = pCond->IsTrue(apActionRef, apTargetRef, bDispositionItemFailure);
				}
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
					} else {
						*apDispositionFailure = true;
					}
				}
			}
		} else {
			if (pCond->GetOr()) {
				bBlockVal = pCond->IsTrue(apActionRef, apTargetRef, bDispositionItemFailure);
				bOrBlock = true;
				bNonDispFailInBlock = false;
				bDispFailInBlock = bDispositionItemFailure;
				if (!bDispositionItemFailure) {
					bNonDispFailInBlock = !bBlockVal;
				}
			} else {
				bBlockVal = bPass || pCond->IsTrue(apActionRef, apTargetRef, bDispositionItemFailure);
				bRetVal = bRetVal && bBlockVal;
				if (bDispositionItemFailure) {
					*apDispositionFailure = true;
				} else {
					if (!bBlockVal) {
						bNonDispositionFailure = true;
					}
				}
			}
		}
		if (!bRetVal && (!bOrBlock && !*apDispositionFailure)) {
			break;
		}
		pList = pList->m_pkNext;
	}
	if (bOrBlock) {
		bRetVal = bRetVal && bBlockVal;
		if (!bRetVal) {
			if (!bDispFailInBlock || bNonDispFailInBlock) {
				bNonDispositionFailure = true;
			} else {
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
	TESConditionItem* v2;
	void* v4;
	if (apFile) {
		if (apFile->GetTESChunk() == 1094997059) {
			v4 = ourMalloc(sizeof(TESConditionItem));
			if (v4) {
				v2 = ::new (v4) TESConditionItem;
			} else {
				v2 = nullptr;
			}
			v2->Load(apFile);
			AddItem(v2);
		}
	}
}

void TESCondition::Save() {
	// The condition is src's BSSimpleList<void*>; its items are condition items.
	for (BSSimpleList<void*>* pList = this; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
		static_cast<TESConditionItem*>(pList->m_item)->Save();
	}
}

void TESCondition::InitItem(TESForm* apOwner) {
	for (BSSimpleList<void*>* node = this; node != nullptr && !BSSimpleList_IsEmpty(node); node = node->GetNext()) {
		static_cast<TESConditionItem*>(node->GetItem())->InitItem(apOwner);
	}
}

void TESCondition::ClearAllConditionItems() {
	BSSimpleList<void*>* head = this;
	while (head != nullptr && !BSSimpleList_IsEmpty(head)) {
		TESConditionItem* item = static_cast<TESConditionItem*>(head->GetItem());
		if (item != nullptr) {
			delete item;
		}
		head->RemoveHead();
	}
}

// The array branch reads the element count the array allocation stores in the dword in front of this (its cookie:
// raw memory the exe reads untyped), and frees from that cookie.
void* TESCondition::DeleteArray(uint32_t auiFlags) {
	if ((auiFlags & 2) != 0) {
		rt::eh_vector_destructor_iterator(this, sizeof(TESCondition), static_cast<int32_t>(fnvd::arrayCount(this)),
			&fnvd::destructElement<TESCondition>);
		if ((auiFlags & 1) != 0) {
			ourFree(fnvd::arrayBlock(this));
		}
		return fnvd::arrayBlock(this);
	} else {
		this->~TESCondition();
		if ((auiFlags & 1) != 0) {
			ourFree(this);
		}
		return this;
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
	case CC_EQUAL:
		return afVal2 == afVal1;
	case CC_NOT_EQUAL:
		return afVal2 != afVal1;
	case CC_GREATER_THAN:
		return bUnordered || afVal2 < afVal1;
	case CC_GREATER_THAN_EQUAL:
		return bUnordered || afVal2 <= afVal1;
	case CC_LESS_THAN:
		return bUnordered || afVal2 > afVal1;
	case CC_LESS_THAN_EQUAL:
		return bUnordered || afVal2 >= afVal1;
	default:
		return true;
	}
}
