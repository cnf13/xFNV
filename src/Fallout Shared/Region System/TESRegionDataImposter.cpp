#include "Fallout Shared/Region System/TESRegionDataImposter.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "SDK/MSVC/rtti.h"

TESRegionDataImposter::TESRegionDataImposter() : TESRegionData() {}

// Copies apData's imposter list when apData is imposter data.
TESRegionDataImposter::TESRegionDataImposter(TESRegionDataImposter* apData) : TESRegionData(apData) {
    if (apData->GetID() == REGION_DATA_IMPOSTER_ID) {
        for (BSSimpleList<TESObjectREFR*>* pIter = &apData->m_Imposters; pIter; pIter = pIter->m_pkNext) {
            m_Imposters.AddTail(pIter->m_item);
        }
    }
}

TESRegionDataImposter::~TESRegionDataImposter() {
    ClearImposterList();
}

void TESRegionDataImposter::Save() {
    TESRegionData::Save();
    uint32_t uicount = m_Imposters.ItemsInList();
    if (uicount) {
        BSSimpleList<TESObjectREFR*>* tmpList = &m_Imposters;
        uint32_t* tmpArray = static_cast<uint32_t*>(
            ourMalloc(uicount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uicount : SIZE_MAX)
        );
        for (uint32_t i = 0; tmpList && i < uicount; ++i) {
            tmpArray[i] = tmpList->m_item->iFormID;
            tmpList = tmpList->m_pkNext;
        }
        TESForm::AddChunkArray(RDID_ID, tmpArray, uicount);
        ourFree(tmpArray);
    }
}

void TESRegionDataImposter::InitItem(TESForm* apOwner) {
    TESObjectREFR* pImposter = 0;
    BSSimpleList<TESObjectREFR*>* pList = &m_Imposters;
    while (pList) {
        if (pList->m_item) {
            TESFile* pownerFile;
            uint32_t nID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pList->m_item));
            if (apOwner) {
                pownerFile = apOwner->GetFile(-1);
            }
            else {
                pownerFile = 0;
            }
            TESForm::AddCompileIndex(nID, pownerFile);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(nID);
            pImposter = dynamic_cast<TESObjectREFR*>(FormByNumericID);
            pList->SetItem(pImposter);
        }
        if (pImposter) {
            pList = pList->m_pkNext;
        }
        else {
            pList->RemoveHead();
            if (!pList->m_item) {
                return;
            }
        }
    }
}

REGION_DATA_ID TESRegionDataImposter::GetID() const {
    return REGION_DATA_IMPOSTER_ID;
}

TESRegionData* TESRegionDataImposter::Copy() {
    void* v4 = ourMalloc(sizeof(TESRegionDataImposter));
    if (v4) {
        return ::new (v4) TESRegionDataImposter(this);
    }
    else {
        return nullptr;
    }
}

// A copy of this data with apRegionData blended into it.
TESRegionData* TESRegionDataImposter::Blend(const TESRegionData* apRegionData) {
    TESRegionData* pCopy = Copy();
    pCopy->BlendInto(apRegionData, 2u);
    return pCopy;
}

void TESRegionDataImposter::BlendInto(const TESRegionData* apRegionData, uint32_t aiTotalBlending) {
    if (apRegionData) {
        apRegionData->GetID();
    }
}

bool TESRegionDataImposter::Validate() {
    return true;
}

// The RDID chunk is an array of reference form IDs; each is added to the list as the pointer InitItem resolves later.
bool TESRegionDataImposter::LoadRegionDataImposter(TESFile* apFile) {
    if (!apFile) {
        return false;
    }
    if (apFile->GetTESChunk() != RDID_ID) {
        return false;
    }
    uint32_t uiSize = apFile->GetChunkSize();
    uint32_t uiCount = uiSize / sizeof(uint32_t);
    uint32_t* pIDs = static_cast<uint32_t*>(ourMalloc(uiCount * sizeof(uint32_t)));
    apFile->GetChunkData(pIDs, uiSize);
    for (uint32_t i = 0; i < uiCount; ++i) {
        uint32_t uiID = pIDs[i];
        if (apFile->QEndian()) {
            ENDIAN(uiID, false);
        }
        AddToImposterList(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiID)));
    }
    ourFree(pIDs);
    return true;
}

bool TESRegionDataImposter::AddToImposterList(TESObjectREFR* apNewImposter) {
    BSSimpleList<TESObjectREFR*>* Name = GetImposterList();
    if (Name->IsInList(apNewImposter)) {
        return 0;
    }
    m_Imposters.AddTail(apNewImposter);
    return 1;
}

bool TESRegionDataImposter::ClearImposterList() {
    (&m_Imposters)->RemoveAll();
    return 1;
}

void TESRegionDataImposter::SetImpostersVisible(bool abVisible, bool abInstantly) {
    for (BSSimpleList<TESObjectREFR*>* pIter = GetImposterList(); pIter; pIter = pIter->GetNext()) {
        if (pIter->GetItem()) {
            SetImposterVisible(pIter->GetItem(), abVisible, abInstantly);
        }
    }
}

void TESRegionDataImposter::SetImposterVisible(TESObjectREFR* apImposter, bool abVisible, bool abInstantly) {
    if (apImposter) {
        if (!apImposter->GetIsImposter()) {
            apImposter->SetIsImposter(1);
            int32_t CastingType = apImposter->iFormID;
            Warning(
                WARN_FORMS, "SetImposterVisible: ObjectREFR is not flagged as an imposter (%s:%X)",
                apImposter->GetFormEditorID(), CastingType
            );
        }
        BSFadeNode* pFadeNode = 0;
        if (apImposter->Get3D()) {
            NiNode* pNode = apImposter->Get3D();
            pFadeNode = pNode->IsFadeNode();
        }
        if (!pFadeNode) {
            abInstantly = 1;
        }
        if (abInstantly) {
            if (apImposter->Get3D()) {
                NiNode* v3 = apImposter->Get3D();
                v3->SetAppCulled(!abVisible);
                NiNode* v4 = apImposter->Get3D();
                v4->SetRelevantImposter(abVisible);
            }
        }
        else {
            if (abVisible) {
                NiNode* v5 = apImposter->Get3D();
                v5->SetAppCulled(0);
                pFadeNode->SetRelevantImposter(1);
            }
            else {
                pFadeNode->SetRelevantImposter(0);
            }
        }
    }
}

// The player's occupied regions, else the regions of the player's cell, each region's imposter data searched for
// apImposter. The region walk stops at the first empty node.
bool TESRegionDataImposter::IsImposterInActiveRegionList(TESObjectREFR* apImposter) {
    BGSWorldLocation kLocation;
    TESRegionList* pOccupiedRegions = pPlayer->GetAllOccupiedRegions();
    BSSimpleList<TESRegion*>* pIter =
        pOccupiedRegions ? static_cast<BSSimpleList<TESRegion*>*>(pOccupiedRegions) : nullptr;
    if (!pIter || !pIter->GetItem()) {
        BGSWorldLocation* pLocation = pPlayer->GetWorldLocation(&kLocation);
        TESRegionList* pCellRegions = pLocation->GetCell()->GetRegionList(true);
        pIter = pCellRegions ? static_cast<BSSimpleList<TESRegion*>*>(pCellRegions) : nullptr;
    }
    for (; pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        TESRegion* pRegion = pIter->GetItem();
        if (!pRegion) {
            continue;
        }
        TESRegionDataImposter* pImposterData =
            static_cast<TESRegionDataImposter*>(pRegion->pDataList->Find(REGION_DATA_IMPOSTER_ID));
        if (!pImposterData) {
            continue;
        }
        for (BSSimpleList<TESObjectREFR*>* pNode = pImposterData->GetImposterList(); pNode; pNode = pNode->GetNext()) {
            if (pNode->GetItem() && pNode->GetItem() == apImposter) {
                return true;
            }
        }
    }
    return false;
}
