#include "Fallout Shared/MediaLocationController.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MediaSet.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESClimate.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

#include <bit>

MediaLocationController::MediaLocationController()
  : TESForm(), TESFullName(), uiLocationDelay(0), uiLayerTime(0), uiLoopTime(0), uiMediaStartTime(0), bInTension(false),
    bInCombat(false), bIsDay(false), bIsConditional(false), uiFoundHostileActors(0), uiFoundFactionActors(0),
    eCurrentFactionReaction(FIGHT_REACTION_ENEMY), cCurrentLayer(0), pCurrentMediaSet(nullptr),
    MediaLocationControllerName(), pFaction(nullptr), rFactionConditional(FIGHT_REACTION_ENEMY), pAudioMarker(nullptr),
    iFlags(4), fLayerTwoPercent(0.33f), fLayerThreePercent(0.66f), fLocationDelay(0.0f), uiDayStart(64),
    uiNightStart(336) {
    SetFormType(0x70u);
    lConditionalNeutralList.RemoveAll();
    lConditionalAllyList.RemoveAll();
    lConditionalFriendList.RemoveAll();
    lConditionalEnemyList.RemoveAll();
    lLocationList.RemoveAll();
    lBattleList.RemoveAll();
}

MediaLocationController::~MediaLocationController() {}

void MediaLocationController::InitializeData() {
    fLayerTwoPercent = 0.33f;
    fLayerThreePercent = 0.66f;
    fLocationDelay = 60.0f;
    uiDayStart = 64;
    uiNightStart = 336;
    lConditionalNeutralList.RemoveAll();
    lConditionalAllyList.RemoveAll();
    lConditionalFriendList.RemoveAll();
    lConditionalEnemyList.RemoveAll();
    lLocationList.RemoveAll();
    lBattleList.RemoveAll();
    pFaction = nullptr;
    rFactionConditional = FIGHT_REACTION_ENEMY;
    pAudioMarker = nullptr;
}

void MediaLocationController::ClearData() {
    lConditionalNeutralList.RemoveAll();
    lConditionalAllyList.RemoveAll();
    lConditionalFriendList.RemoveAll();
    lConditionalEnemyList.RemoveAll();
    lLocationList.RemoveAll();
    lBattleList.RemoveAll();
}

bool MediaLocationController::Load(TESFile* apFile) {
    uint32_t uiValue = 0;
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            // The float chunks hold the field's four bytes, which are read straight into the field.
            case NAM1_ID: apFile->GetChunkData<uint32_t>(iFlags); break;
            case NAM2_ID: apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fLayerTwoPercent)); break;
            case NAM3_ID: apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fLayerThreePercent)); break;
            // NAM4 is the location delay and NAM7 the retrigger delay.
            case NAM4_ID: apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fLocationDelay)); break;
            case NAM5_ID: apFile->GetChunkData<uint32_t>(uiDayStart); break;
            case NAM6_ID: apFile->GetChunkData<uint32_t>(uiNightStart); break;
            case NAM7_ID: apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fRetriggerDelay)); break;
            case FNAM_ID:
                apFile->GetChunkData<uint32_t>(uiValue);
                rFactionConditional = static_cast<FIGHT_REACTION>(uiValue);
                break;
            // The list chunks hold a media set's form ID, which stands in for the set until it is resolved.
            case GNAM_ID: {
                apFile->GetChunkData<uint32_t>(uiValue);
                MediaSet* pSet = reinterpret_cast<MediaSet*>(static_cast<uintptr_t>(uiValue));
                lBattleList.AddHead(pSet);
                break;
            }
            case HNAM_ID: {
                apFile->GetChunkData<uint32_t>(uiValue);
                MediaSet* pSet = reinterpret_cast<MediaSet*>(static_cast<uintptr_t>(uiValue));
                lConditionalNeutralList.AddHead(pSet);
                break;
            }
            case LNAM_ID: {
                apFile->GetChunkData<uint32_t>(uiValue);
                MediaSet* pSet = reinterpret_cast<MediaSet*>(static_cast<uintptr_t>(uiValue));
                lLocationList.AddHead(pSet);
                break;
            }
            case RNAM_ID:
                apFile->GetChunkData<uint32_t>(uiValue);
                pFaction = reinterpret_cast<TESFaction*>(static_cast<uintptr_t>(uiValue));
                break;
            case XNAM_ID: {
                apFile->GetChunkData<uint32_t>(uiValue);
                MediaSet* pSet = reinterpret_cast<MediaSet*>(static_cast<uintptr_t>(uiValue));
                lConditionalFriendList.AddHead(pSet);
                break;
            }
            case YNAM_ID: {
                apFile->GetChunkData<uint32_t>(uiValue);
                MediaSet* pSet = reinterpret_cast<MediaSet*>(static_cast<uintptr_t>(uiValue));
                lConditionalEnemyList.AddHead(pSet);
                break;
            }
            case ZNAM_ID: {
                apFile->GetChunkData<uint32_t>(uiValue);
                MediaSet* pSet = reinterpret_cast<MediaSet*>(static_cast<uintptr_t>(uiValue));
                lConditionalAllyList.AddHead(pSet);
                break;
            }
        }
    }
    while (apFile->NextChunk());
    return true;
}

void MediaLocationController::Save() {
    StartForm();
    TESFullName::Save();
    TESForm::AddChunk(NAM1_ID, iFlags);
    TESForm::AddChunk(NAM2_ID, std::bit_cast<uint32_t>(fLayerTwoPercent));
    TESForm::AddChunk(NAM3_ID, std::bit_cast<uint32_t>(fLayerThreePercent));
    TESForm::AddChunk(NAM4_ID, std::bit_cast<uint32_t>(fLocationDelay));
    TESForm::AddChunk(NAM5_ID, uiDayStart);
    TESForm::AddChunk(NAM6_ID, uiNightStart);
    TESForm::AddChunk(NAM7_ID, std::bit_cast<uint32_t>(fRetriggerDelay));
    BSSimpleList<MediaSet*>* pList = &lConditionalNeutralList;
    if (!lConditionalNeutralList.IsEmpty()) {
        while (pList) {
            MediaSet* pSet = pList->m_item;
            if (!pSet) {
                break;
            }
            TESForm::AddChunk(HNAM_ID, pSet->iFormID);
            pList = pList->m_pkNext;
        }
    }
    pList = &lConditionalAllyList;
    if (!lConditionalAllyList.IsEmpty()) {
        while (pList) {
            MediaSet* pSet = pList->m_item;
            if (!pSet) {
                break;
            }
            TESForm::AddChunk(ZNAM_ID, pSet->iFormID);
            pList = pList->m_pkNext;
        }
    }
    pList = &lConditionalFriendList;
    if (!lConditionalFriendList.IsEmpty()) {
        while (pList) {
            MediaSet* pSet = pList->m_item;
            if (!pSet) {
                break;
            }
            TESForm::AddChunk(XNAM_ID, pSet->iFormID);
            pList = pList->m_pkNext;
        }
    }
    pList = &lConditionalEnemyList;
    if (!lConditionalEnemyList.IsEmpty()) {
        while (pList) {
            MediaSet* pSet = pList->m_item;
            if (!pSet) {
                break;
            }
            TESForm::AddChunk(YNAM_ID, pSet->iFormID);
            pList = pList->m_pkNext;
        }
    }
    pList = &lLocationList;
    if (!lLocationList.IsEmpty()) {
        while (pList) {
            MediaSet* pSet = pList->m_item;
            if (!pSet) {
                break;
            }
            TESForm::AddChunk(LNAM_ID, pSet->iFormID);
            pList = pList->m_pkNext;
        }
    }
    pList = &lBattleList;
    if (!lBattleList.IsEmpty()) {
        while (pList) {
            MediaSet* pSet = pList->m_item;
            if (!pSet) {
                break;
            }
            TESForm::AddChunk(GNAM_ID, pSet->iFormID);
            pList = pList->m_pkNext;
        }
    }
    if (pFaction) {
        TESForm::AddChunk(RNAM_ID, pFaction->iFormID);
    }
    TESForm::AddChunk(FNAM_ID, rFactionConditional);
    CloseForm();
}

void MediaLocationController::InitItem() {
    if (!GetInitialized()) {
        InitItemList(&lConditionalNeutralList);
        InitItemList(&lConditionalAllyList);
        InitItemList(&lConditionalFriendList);
        InitItemList(&lConditionalEnemyList);
        InitItemList(&lLocationList);
        InitItemList(&lBattleList);
        if (pFaction) {
            uint32_t temp = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFaction));
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(temp, pFile);
            pFaction = pDataHandler->GetFaction(temp);
            if (!pFaction) {
                Warning("MASTERFILE: Could not find reputation (%08X) for MediaLocationController", pFaction);
            }
        }
        SetInitialized(true);
    }
}

void MediaLocationController::Copy(TESForm* apCopy) {
    MediaLocationController* pcopy = dynamic_cast<MediaLocationController*>(apCopy);
    if (pcopy) {
        CopyAllComponents(pcopy);
        iFlags = pcopy->iFlags;
        fLayerTwoPercent = pcopy->fLayerTwoPercent;
        fLayerThreePercent = pcopy->fLayerThreePercent;
        fLocationDelay = pcopy->fLocationDelay;
        fRetriggerDelay = pcopy->fRetriggerDelay;
        uiDayStart = pcopy->uiDayStart;
        uiNightStart = pcopy->uiNightStart;
        pFaction = pcopy->pFaction;
        rFactionConditional = pcopy->rFactionConditional;
        pAudioMarker = pcopy->pAudioMarker;
        lConditionalNeutralList.RemoveAll();
        for (BSSimpleList<MediaSet*>* tList = &pcopy->lConditionalNeutralList; tList && tList->m_item;
             tList = tList->m_pkNext) {
            lConditionalNeutralList.AddHead(tList->m_item);
        }
        lConditionalAllyList.RemoveAll();
        for (BSSimpleList<MediaSet*>* tList = &pcopy->lConditionalAllyList; tList && tList->m_item;
             tList = tList->m_pkNext) {
            lConditionalAllyList.AddHead(tList->m_item);
        }
        lConditionalFriendList.RemoveAll();
        for (BSSimpleList<MediaSet*>* tList = &pcopy->lConditionalFriendList; tList && tList->m_item;
             tList = tList->m_pkNext) {
            lConditionalFriendList.AddHead(tList->m_item);
        }
        lConditionalEnemyList.RemoveAll();
        for (BSSimpleList<MediaSet*>* tList = &pcopy->lConditionalEnemyList; tList && tList->m_item;
             tList = tList->m_pkNext) {
            lConditionalEnemyList.AddHead(tList->m_item);
        }
        lLocationList.RemoveAll();
        for (BSSimpleList<MediaSet*>* tList = &pcopy->lLocationList; tList && tList->m_item; tList = tList->m_pkNext) {
            lLocationList.AddHead(tList->m_item);
        }
        lBattleList.RemoveAll();
        for (BSSimpleList<MediaSet*>* tList = &pcopy->lBattleList; tList && tList->m_item; tList = tList->m_pkNext) {
            lBattleList.AddHead(tList->m_item);
        }
    }
}

bool MediaLocationController::Compare(TESForm* apCompare) {
    MediaLocationController* pcomp = dynamic_cast<MediaLocationController*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (iFlags != pcomp->iFlags) {
        return true;
    }
    if (pFaction != pcomp->pFaction) {
        return true;
    }
    if (rFactionConditional != pcomp->rFactionConditional) {
        return true;
    }
    if (pAudioMarker != pcomp->pAudioMarker) {
        return true;
    }
    if (pcomp->fLayerTwoPercent != fLayerTwoPercent) {
        return true;
    }
    if (pcomp->fLayerThreePercent != fLayerThreePercent) {
        return true;
    }
    if (pcomp->fLocationDelay != fLocationDelay) {
        return true;
    }
    if (pcomp->fRetriggerDelay != fRetriggerDelay) {
        return true;
    }
    if (uiDayStart != pcomp->uiDayStart) {
        return true;
    }
    if (uiNightStart != pcomp->uiNightStart) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (!pcomp->lConditionalNeutralList.m_item) {
        return true;
    }
    if (!lConditionalNeutralList.m_item) {
        return true;
    }
    int32_t iCount = pcomp->lConditionalNeutralList.ItemsInList();
    if (iCount != lConditionalNeutralList.ItemsInList()) {
        return true;
    }
    for (MediaSet*& rpItem : lConditionalNeutralList.ItemsUntilNull()) {
        bool bFound = false;
        for (MediaSet*& rpCompItem : pcomp->lConditionalNeutralList.ItemsUntilNull()) {
            MediaSet* pSet = rpItem;
            if (!pSet->Compare(rpCompItem)) {
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            return true;
        }
    }
    if (!pcomp->lConditionalAllyList.m_item) {
        return true;
    }
    if (!lConditionalAllyList.m_item) {
        return true;
    }
    iCount = pcomp->lConditionalAllyList.ItemsInList();
    if (iCount != lConditionalAllyList.ItemsInList()) {
        return true;
    }
    for (MediaSet*& rpItem : lConditionalAllyList.ItemsUntilNull()) {
        bool bFound = false;
        for (MediaSet*& rpCompItem : pcomp->lConditionalAllyList.ItemsUntilNull()) {
            MediaSet* pSet = rpItem;
            if (!pSet->Compare(rpCompItem)) {
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            return true;
        }
    }
    if (!pcomp->lConditionalFriendList.m_item) {
        return true;
    }
    if (!lConditionalFriendList.m_item) {
        return true;
    }
    iCount = pcomp->lConditionalFriendList.ItemsInList();
    if (iCount != lConditionalFriendList.ItemsInList()) {
        return true;
    }
    for (MediaSet*& rpItem : lConditionalFriendList.ItemsUntilNull()) {
        bool bFound = false;
        for (MediaSet*& rpCompItem : pcomp->lConditionalFriendList.ItemsUntilNull()) {
            MediaSet* pSet = rpItem;
            if (!pSet->Compare(rpCompItem)) {
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            return true;
        }
    }
    if (!pcomp->lConditionalEnemyList.m_item) {
        return true;
    }
    if (!lConditionalEnemyList.m_item) {
        return true;
    }
    iCount = pcomp->lConditionalEnemyList.ItemsInList();
    if (iCount != lConditionalEnemyList.ItemsInList()) {
        return true;
    }
    for (MediaSet*& rpItem : lConditionalEnemyList.ItemsUntilNull()) {
        bool bFound = false;
        for (MediaSet*& rpCompItem : pcomp->lConditionalEnemyList.ItemsUntilNull()) {
            MediaSet* pSet = rpItem;
            if (!pSet->Compare(rpCompItem)) {
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            return true;
        }
    }
    if (!pcomp->lLocationList.m_item) {
        return true;
    }
    if (!lLocationList.m_item) {
        return true;
    }
    iCount = pcomp->lLocationList.ItemsInList();
    if (iCount != lLocationList.ItemsInList()) {
        return true;
    }
    for (MediaSet*& rpItem : lLocationList.ItemsUntilNull()) {
        bool bFound = false;
        for (MediaSet*& rpCompItem : pcomp->lLocationList.ItemsUntilNull()) {
            MediaSet* pSet = rpItem;
            if (!pSet->Compare(rpCompItem)) {
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            return true;
        }
    }
    if (!pcomp->lBattleList.m_item) {
        return true;
    }
    if (!lBattleList.m_item) {
        return true;
    }
    iCount = pcomp->lBattleList.ItemsInList();
    if (iCount != lBattleList.ItemsInList()) {
        return true;
    }
    for (MediaSet*& rpItem : lBattleList.ItemsUntilNull()) {
        bool bFound = false;
        for (MediaSet*& rpCompItem : pcomp->lBattleList.ItemsUntilNull()) {
            MediaSet* pSet = rpItem;
            if (!pSet->Compare(rpCompItem)) {
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            return true;
        }
    }
    return false;
}

bool MediaLocationController::GetDefaultTime() {
    return ((iFlags >> 6) & 1) != 0;
}

void MediaLocationController::InitItemList(BSSimpleList<MediaSet*>* plist) {
    BSSimpleList<MediaSet*>* plastlist = nullptr;
    while (plist && !plist->IsEmpty()) {
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plist->m_item));
        TESFile* pFile = GetFile(-1);
        TESForm::AddCompileIndex(tempint, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(tempint);
        MediaSet* pSet = dynamic_cast<MediaSet*>(pForm);
        if (pSet) {
            plist->SetItem(pSet);
            plastlist = plist;
            plist = plist->m_pkNext;
        }
        else {
            BSStringT<char> tempstring;
            GetFormDetailedString(tempstring);
            Warning("MASTERFILE: Could not find MediaSet (%08X) for %s.", tempint, tempstring.pString);
            if (plastlist) {
                // Until InitItem resolves them, the list's pointer slots hold form IDs.
                MediaSet* item = reinterpret_cast<MediaSet*>(static_cast<uintptr_t>(tempint));
                plastlist->Remove(item);
                plist = plastlist->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
    }
}

bool MediaLocationController::GetDayNight() {
    double dDayStart;
    double sunsetBegin;
    double currentGameHour;
    double dSunriseMid;
    double dSuriseBegin;
    Sky* pSky = TES::pInstance->TES::GetSky();
    if (GetDefaultTime()) {
        if (!pSky) {
            return true;
        }
        dSuriseBegin = pSky->GetSunriseBegin();
        dSunriseMid = (pSky->GetSunriseEnd() + dSuriseBegin) / 2.0;
        if (!(pSky->GetCurrentGameHour() >= dSunriseMid)) {
            return false;
        }
        currentGameHour = pSky->GetCurrentGameHour();
        sunsetBegin = pSky->GetSunsetBegin();
        if ((pSky->GetSunsetEnd() + sunsetBegin) / 2.0 >= currentGameHour) {
            return true;
        }
        return false;
    }

    if (!pSky) {
        return true;
    }
    dDayStart = uiDayStart * 0.09375;
    return pSky->GetCurrentGameHour() >= dDayStart && uiNightStart * 0.09375 >= pSky->GetCurrentGameHour();
}

MediaLocationController::DEADREPFLAGS MediaLocationController::GetDeadRep() {
    return static_cast<MediaLocationController::DEADREPFLAGS>(iFlags & DEADREP_MASK);
}

MediaLocationController::LOOPFLAGS MediaLocationController::GetLoop() {
    return static_cast<MediaLocationController::LOOPFLAGS>((iFlags >> 4) & 3);
}

bool MediaLocationController::GetDay() {
    return bIsDay;
}

MediaSet* MediaLocationController::GetRandomListItem(BSSimpleList<MediaSet*>* list) {
    BSSimpleList<MediaSet*>* pIter = list;
    MediaSet* pMediaSet = nullptr;
    if (!list || !pIter->m_item) {
        return nullptr;
    }
    while (pIter && pIter->m_item) {
        if (!RandomInt(0, 4) || !pMediaSet) {
            pMediaSet = pIter->m_item;
        }
        pIter = pIter->m_pkNext;
    }
    if (pMediaSet) {
        list->Remove(pMediaSet);
        list->AddTail(pMediaSet);
    }
    return pMediaSet;
}

bool MediaLocationController::Update(bool abInCombat) {
    FIGHT_REACTION eReaction;
    // Tests the marker only when pPlayer is null, calling the getter on null.
    if (!pPlayer && pPlayer->GetClosestAudioMarkerInfo() && pPlayer->GetClosestAudioMarkerInfo()->pMarkerRef) {
        return false;
    }
    if (FalloutAudioMedia::pCurrentMediaController != this) {
        if (pCurrentMediaSet) {
            pCurrentMediaSet->Reset();
        }
        pCurrentMediaSet = nullptr;
    }
    FalloutAudioMedia::pCurrentMediaController = this;
    uint32_t ticks = BSAudio::QInstance().GetSynchTime();
    if (!bIsActive) {
        if (pCurrentMediaSet) {
            pCurrentMediaSet->Reset();
        }
        pCurrentMediaSet = nullptr;
    }
    if (pFaction) {
        TESFaction* pPlayerFaction = static_cast<TESFaction*>(
            BGSDefaultObjectManager::GetDefaultObject(DO_PLAYER_FACTION)
        );
        eReaction = static_cast<FIGHT_REACTION>(pFaction->GetFactionReaction(pPlayerFaction));
    }
    else {
        eReaction = FIGHT_REACTION_NEUTRAL;
    }
    AudioMarkerInfo* pnAudioMarker = pPlayer->GetClosestAudioMarkerInfo();
    float fRadius = pnAudioMarker->pMarkerRef->GetRadius();
    float fRadiusSqr = fRadius * fRadius;
    NiPoint3 Loc = pnAudioMarker->pMarkerRef->GetLocationOnReference();
    NiPoint3 Dist = Loc - pPlayer->GetLocationOnReference();
    float XYDist = Dist.SqrXYLength();
    float fPlayerRadius = 1000.0f;
    if (fRadiusSqr >= static_cast<double>(XYDist)) {
        fPlayerRadius = XYDist / fRadiusSqr;
    }
    // Walks the compass targets under kPerceivedActorsLock; PlayerCharacter.h says why.
    PlayerCharacter::kPerceivedActorsLock.Lock(nullptr);
    BSSimpleList<CompassTarget*>* pNearList = pPlayer->GetListofPerceivedActors();
    bool bFoundHostile = false;
    bool bLastFoundFactionActors = uiFoundFactionActors != 0;
    uiFoundFactionActors = 0;
    uiFoundHostileActors = 0;
    while (pNearList && !pNearList->IsEmpty()) {
        CompassTarget* pact = pNearList->m_item;
        if (pact && pact->pTarget) {
            if (pFaction && pact->pTarget->IsInFaction(pFaction)) {
                ++uiFoundFactionActors;
                bFoundHostile = true;
                if (pact->bIsHostile) {
                    eReaction = FIGHT_REACTION_ENEMY;
                }
            }

            if (pact->bIsHostile) {
                ++uiFoundHostileActors;
            }
        }

        pNearList = pNearList->m_pkNext;
    }
    PlayerCharacter::kPerceivedActorsLock.Unlock();
    bool bChangeDead = bLastFoundFactionActors != (uiFoundFactionActors != 0);
    int32_t iIsDay = this->bIsDay;
    bool bChangeDay = iIsDay != GetDayNight();
    bool bChangeReaction = eCurrentFactionReaction != eReaction;
    if (static_cast<int32_t>(eReaction) == GetDeadRep()) {
        bChangeDead = false;
    }
    bInCombat = abInCombat;
    bInTension = uiFoundHostileActors != 0;
    this->bIsDay = GetDayNight();
    eCurrentFactionReaction = eReaction;
    fCurrentPlayerRadius = fPlayerRadius;
    if (fCurrentPlayerRadius < 100.0 && (uiLocationDelay > ticks && !bIsActive)) {
        return false;
    }
    bIsActive = false;
    if (pCurrentMediaSet) {
        bIsActive = pCurrentMediaSet->Update(ticks, abInCombat, this);
    }
    if (bChangeDay || (bChangeReaction || bChangeDead)) {
        if (pCurrentMediaSet) {
            pCurrentMediaSet->Reset();
        }
        pCurrentMediaSet = nullptr;
    }
    if (!bIsActive && fPlayerRadius < 100.0) {
        if (pCurrentMediaSet) {
            pCurrentMediaSet->Reset();
            pCurrentMediaSet = nullptr;
        }
        if (!pCurrentMediaSet) {
            if (bFoundHostile) {
                switch (eReaction) {
                    case FIGHT_REACTION_NEUTRAL: {
                        pCurrentMediaSet = GetRandomListItem(&lConditionalNeutralList);
                        break;
                    }
                    case FIGHT_REACTION_ENEMY: {
                        pCurrentMediaSet = GetRandomListItem(&lConditionalEnemyList);
                        break;
                    }
                    case FIGHT_REACTION_ALLY: {
                        pCurrentMediaSet = GetRandomListItem(&lConditionalAllyList);
                        break;
                    }
                    case FIGHT_REACTION_FRIEND: {
                        pCurrentMediaSet = GetRandomListItem(&lConditionalFriendList);
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            else {
                switch (GetDeadRep()) {
                    case MediaLocationController::DEADFACTION_CONDITIONAL_NEUTRAL: {
                        pCurrentMediaSet = GetRandomListItem(&lConditionalNeutralList);
                        break;
                    }
                    case MediaLocationController::DEADFACTION_CONDITIONAL_ENEMY: {
                        pCurrentMediaSet = GetRandomListItem(&lConditionalEnemyList);
                        break;
                    }
                    case MediaLocationController::DEADFACTION_CONDITIONAL_ALLY: {
                        pCurrentMediaSet = GetRandomListItem(&lConditionalAllyList);
                        break;
                    }
                    case MediaLocationController::DEADFACTION_CONDITIONAL_FRIEND: {
                        pCurrentMediaSet = GetRandomListItem(&lConditionalFriendList);
                        break;
                    }
                    case MediaLocationController::DEADFACTION_LOCATION: {
                        pCurrentMediaSet = GetRandomListItem(&lLocationList);
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
        }
    }
    if ((!pCurrentMediaSet || (pCurrentMediaSet->Type && pCurrentMediaSet->Type != MediaSet::LAYER_DUNGEON)) &&
        fCurrentPlayerRadius < 100.0 &&
        abInCombat) {
        if (pCurrentMediaSet) {
            pCurrentMediaSet->Reset();
            pCurrentMediaSet = nullptr;
        }
        if (!FalloutAudioMedia::pCurrentBattleMediaSet) {
            pCurrentMediaSet = GetRandomListItem(&lBattleList);
        }
    }
    if (!bIsActive && pCurrentMediaSet) {
        bIsActive = pCurrentMediaSet->Update(ticks, abInCombat, this);
    }

    if (bIsActive) {
        uiLocationDelay = static_cast<int64_t>(static_cast<double>(ticks) + fLocationDelay * 1000.0);
    }
    if (!bIsActive && fCurrentPlayerRadius > 1.0) {
        FalloutAudioMedia::pCurrentMediaController = nullptr;
    }
    return bIsActive;
}

uint32_t BSAudio::GetSynchTime() {
    return SynchTimer.iMilliseconds;
}

uint8_t TESClimate::GetTransTime(TransTime aeTT) {
    if (aeTT < NUM_TRANS_TIMES && aeTT >= SUNRISE_BEGIN) {
        return ucData[aeTT];
    }
    return 0;
}
