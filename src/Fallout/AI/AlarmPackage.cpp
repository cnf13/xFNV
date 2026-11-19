#include "Fallout/AI/AlarmPackage.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"

AlarmPackage::AlarmPackage() {}

AlarmPackage::AlarmPackage(Crime* apCrime) {
    if (apCrime) {
        Crimes.AddHead(apCrime);
    }
}

AlarmPackage::~AlarmPackage() {
    Crimes.RemoveAll();
}

void AlarmPackage::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    char ccrimeType;
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (Crime* pcrime : Crimes) {
        if (pcrime) {
            uint16_t icrimeIndex;
            ccrimeType = pcrime->ecrime;
            icrimeIndex = ReferenceProcessLists.GetCrimeIndex(static_cast<PackageNS::CRIME_TYPE>(ccrimeType), pcrime);
            apSaveGameBuffer->SaveDataEndian(&ccrimeType, 1u, 0);
            apSaveGameBuffer->SaveDataEndian(&icrimeIndex, 2u, 0);
            ++isaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
}

void AlarmPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    char ccrimeType;
    uint16_t icrimeIndex;
    TESPackage::LoadGame(apLoadGameBuffer);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        ccrimeType = 0;
        icrimeIndex = 0;
        apLoadGameBuffer->LoadData(&ccrimeType, 1u);
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&icrimeIndex), 2u);
        Crime* pcrime =
            ReferenceProcessLists.GetCrimeByIndex(static_cast<PackageNS::CRIME_TYPE>(ccrimeType), icrimeIndex);
        if (pcrime) {
            Crimes.AddHead(pcrime);
        }
    }
}

void AlarmPackage::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::InitLoadGame(apLoadGameBuffer);
}

void AlarmPackage::Copy(TESForm* apCopy) {
    TESPackage::Copy(apCopy);
    Crimes.RemoveAll();
    AlarmPackage* pcopy = dynamic_cast<AlarmPackage*>(apCopy);
    if (pcopy) {
        for (BSSimpleList<Crime*>* plist = &pcopy->Crimes; plist && plist->m_item; plist = plist->m_pkNext) {
            Crimes.AddHead(plist->m_item);
        }
    }
}

uint16_t AlarmPackage::GetSaveSize() {
    uint16_t isize = TESPackage::GetSaveSize();
    uint16_t v4 = isize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        isize += 6;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize - v4,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 491,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize - v4, 491,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
    }
    return isize;
}

void AlarmPackage::SaveGame() {
    uint32_t iblockHeader;
    TESPackage::SaveGame();
    uint16_t iblockSize = 0;
    uint16_t* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // 'BLOK'.
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    uint16_t icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
    // The walk deliberately starts at null, not at Crimes, as the game does, so it saves no crime and the count stays
    // 0.
    for (BSSimpleList<Crime*>* plist = nullptr; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        Crime* pcrime = plist->m_item;
        char ccrimeType = pcrime->ecrime;
        pSaveLoadGameOLD->SaveGameDataOLD(&ccrimeType, 1u);
        uint16_t icrimeIndex =
            ReferenceProcessLists.GetCrimeIndex(static_cast<PackageNS::CRIME_TYPE>(ccrimeType), pcrime);
        pSaveLoadGameOLD->SaveGameDataOLD(&icrimeIndex, 2u);
        ++icount;
    }
    *pcount = icount;
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v1 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v1, iFlags, 526,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 526,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The two buffer positions are compared and subtracted as unsigned addresses.
        uintptr_t icurrentPosition = reinterpret_cast<uintptr_t>(pSaveLoadGameOLD->m_pBuffer);
        uintptr_t iblockStart = reinterpret_cast<uintptr_t>(pblockSize);
        if (icurrentPosition > iblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp", 526
            );
        }
        *pblockSize = static_cast<uint16_t>(icurrentPosition - iblockStart);
    }
}

void AlarmPackage::LoadGame() {
    char ccrimeType;
    uint16_t icrimeIndex;
    uint32_t iblockHeader;
    uint16_t icount;
    TESPackage::LoadGame();
    uint16_t iblockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
        // 'BLOK'.
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp", 534, pheader->iFormID,
                    pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp", 534,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    LoadGameDataOLD(&icount, 2);
    for (int32_t i = 0; i < icount; ++i) {
        pSaveLoadGameOLD->LoadGameDataOLD(&ccrimeType, 1u);
        pSaveLoadGameOLD->LoadGameDataOLD(&icrimeIndex, 2u);
        Crime* pcrime =
            ReferenceProcessLists.GetCrimeByIndex(static_cast<PackageNS::CRIME_TYPE>(ccrimeType), icrimeIndex);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* iblockEnd = istartBufferPosition + iblockSize;
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= iblockEnd) {
                if (icurrentBufferPosition < iblockEnd) {
                    uint32_t iFlags = pheader_2->iFlags;
                    int32_t cVersion = pheader_2->cVersion;
                    const char* v2 = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(iblockEnd - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp", 554,
                        pheader_2->iFormID, v2, cVersion, iFlags
                    );
                }
            }
            else {
                uint32_t v5 = pheader_2->iFlags;
                int32_t v3 = pheader_2->cVersion;
                const char* v1 = pform_2->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - iblockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp", 554, pheader_2->iFormID,
                    v1, v3, v5
                );
            }
        }
        else {
            if (icurrentBufferPosition <= iblockEnd) {
                if (icurrentBufferPosition < iblockEnd) {
                    int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(iblockEnd - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp", 554, CurrentVersion
                    );
                }
            }
            else {
                int32_t v7 = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(icurrentBufferPosition - iblockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp", 554, v7
                );
            }
        }
    }
}

void AlarmPackage::RemoveCrime(Crime* apcrime, Actor* apActor) {
    (&Crimes)->Remove(apcrime);
    bool vCondition1;
    vCondition1 = (&Crimes)->IsEmpty();
    if (vCondition1) {
        apActor->EndInterruptPackage(false);
    }
}

bool AlarmPackage::IsCrimeInList(Crime* apCrime) {
    return Crimes.IsInList(apCrime);
}

void AlarmPackage::AddCrime(Crime* apcrime) {
    Crimes.AddHead(apcrime);
}

Crime::Crime()
  : iRefCount(0), ecrime(PackageNS::CRIME_TYPE_NONE), pcrimetarget(nullptr), pcriminal(nullptr), breported(false),
    pObjectStolen(nullptr), inumberstolen(0), pOwnership(nullptr), iCrimeNumber(0), iCrimeStamp(-1) {
    CombatTimer kTimer;
    kTimer.SetStartTime(-0x1.fffffe0000000p+127f);
    fCrimeTimeStamp.fTimeStamp = kTimer.fStartTime;
}

Crime::Crime(
    uint32_t aeCrime,
    TESObjectREFR* apTarget,
    Actor* apActor,
    TESBoundObject* apObject,
    int32_t aiNumber,
    TESForm* apOwnership
)
  : iRefCount(0), ecrime(static_cast<PackageNS::CRIME_TYPE>(aeCrime)), pcrimetarget(apTarget), pcriminal(apActor),
    breported(false), pObjectStolen(apObject), inumberstolen(aiNumber), pOwnership(apOwnership),
    pListofEnemyFactions(nullptr) {
    if (apTarget) {
        apTarget->SetTargeted(true);
    }
    iCrimeNumber = ReferenceProcessLists.GetCrimeNumber();
    int32_t iMonthDay = appCalendar.GetDay() | (static_cast<int32_t>(appCalendar.GetMonth()) << 9);
    iCrimeStamp = (appCalendar.GetYear() << 13) | iMonthDay;
    CombatTimer kTimer;
    kTimer.SetStartTime(CombatTimer::GetTimer());
    fCrimeTimeStamp.fTimeStamp = kTimer.fStartTime;
}

Crime::~Crime() {
    if ((pDataHandler) && (!pDataHandler->GetClearingData())) {
        ClearActorKnowList();
        ReferenceProcessLists.RemoveCrimeGlobalCrimeList(this);
        if ((pcrimetarget) && (pcrimetarget->m_Extra.GetPlayerCrimeList())) {
            Crime* item = this;
            pcrimetarget->m_Extra.GetPlayerCrimeList()->Remove(item);
        }

        for (BSSimpleList<TESFaction*>* plist = pListofEnemyFactions; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            TESFaction* pfac = plist->m_item;
            if (!ReferenceProcessLists.IsEnemyFactionofOtherCrime(pfac, this)) {
                pfac->SetPCAnEnemy(false);
            }
        }
        Actor* pact = static_cast<Actor*>(pcrimetarget);
        if ((pact) && (pact->IsActor())) {
            BSSimpleList<Crime*>* pcrimelist = pact->m_Extra.GetPlayerCrimeList();
            if (pcrimelist) {
                Crime* v3 = this;
                pcrimelist->Remove(v3);
            }
        }

        // prespond is never advanced, so a non-empty result loops for ever, as the game does.
        BSSimpleList<TESObjectREFR*>* prespond = ReferenceProcessLists.GetActorsProcessCrimeAlarm(this, true);
        while (prespond) {
            Actor* pact_2 = static_cast<Actor*>(prespond->m_item);
            if (pact_2->IsAlarmed()) {
                pact_2->EndInterruptPackage(false);
            }
            BSSimpleList<Crime*>* pcrimelist_2 = pact_2->m_Extra.GetPlayerCrimeList();
            if (pcrimelist_2) {
                Crime* v2 = this;
                pcrimelist_2->Remove(v2);
            }
        }
    }

    if (pListofEnemyFactions) {
        pListofEnemyFactions->RemoveAll();
        delete pListofEnemyFactions;
    }
    pActorsknowofCrime.RemoveAll();
}

void Crime::InitItem(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESForm* v6;
    Actor* v7;
    TESObjectREFR* v8;
    if (pcrimetarget) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pcrimetarget)));
        v8 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    }
    else {
        v8 = nullptr;
    }
    pcrimetarget = v8;
    if (pcriminal) {
        TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pcriminal)));
        v7 = dynamic_cast<Actor*>(v3);
    }
    else {
        v7 = nullptr;
    }
    pcriminal = v7;
    if (pOwnership) {
        v6 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOwnership)));
    }
    else {
        v6 = nullptr;
    }
    pOwnership = v6;
    BSSimpleList<Actor*>* plist = &pActorsknowofCrime;
    BSSimpleList<Actor*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        Actor* pForm = plist->m_item;
        bool bremove = false;
        if (pForm) {
            TESForm* v4 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
            pForm = dynamic_cast<Actor*>(v4);
            if (pForm) {
                plist->SetItem(pForm);
            }
            else {
                bremove = true;
            }
        }
        else {
            bremove = true;
        }
        if (bremove) {
            if (plastList) {
                BSSimpleList<Actor*>* v5 = plist;
                plastList->Remove(v5->m_item);
                plist = plastList->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
        else {
            plastList = plist;
            plist = plist->m_pkNext;
        }
    }
    if (pcrimetarget) {
        pcrimetarget->SetTargeted(true);
    }
}

bool Crime::DoesActorKnowaboutCrime(Actor* apActor) {
    return (bool)((int32_t)(&pActorsknowofCrime)->IsInList(apActor));
}

bool Crime::ShouldCrimeBeRemoved() {
    // GetMonth, GetDay, GetYear are called in this order; GetDay's char is sign-extended.
    int32_t idate = static_cast<int32_t>(appCalendar.GetMonth()) << 9;
    idate |= appCalendar.GetDay();
    idate |= static_cast<int32_t>(appCalendar.GetYear() << 13);
    bool bflag = false;
    if (idate - iCrimeStamp >= iCoolDownTimerSetting.Int()) {
        bflag = true;
    }
    else if (iRefCount < 1) {
        bflag = true;
    }
    else {
        BSSimpleList<Actor*>* plist = &pActorsknowofCrime;
        if (plist == nullptr || plist->IsEmpty()) {
            bflag = true;
        }
        else {
            bool bdead = true;
            while (plist && plist->m_item && bdead) {
                Actor* pact = plist->m_item;
                if (!pact->IsDead(false)) {
                    bdead = false;
                }
                plist = plist->m_pkNext;
            }
            if (bdead) {
                bflag = true;
            }
        }
    }
    return bflag;
}

void Crime::ClearActorKnowList() {
    for (Actor* pact : pActorsknowofCrime.ItemsUntilNull()) {
        BSSimpleList<Crime*>* pcrimelist = (&pact->m_Extra)->GetPlayerCrimeList();
        if (pcrimelist) {
            Crime* item = this;
            pcrimelist->Remove(item);
        }
    }
    iRefCount = 0;
}

CombatTimeStamp Crime::GetTimeStamp() {
    return fCrimeTimeStamp;
}

void Crime::AddtoActorKnowList(Actor* apActor) {
    if (!pActorsknowofCrime.IsInList(apActor)) {
        pActorsknowofCrime.AddHead(apActor);
        ++iRefCount;
    }
}

void Crime::RemoveActorKnowList(Actor* apActor) {
    pActorsknowofCrime.Remove(apActor);
    --iRefCount;
}

int32_t Crime::GetRefCount() {
    if (pActorsknowofCrime.IsEmpty()) {
        return 0;
    }

    return pActorsknowofCrime.ItemsInList();
}

void Crime::AddEnemyFactions(TESFaction* apFaction) {
    if (!pListofEnemyFactions) {
        BSSimpleList<TESFaction*>* v2;
        BSSimpleList<TESFaction*>* v4 =
            static_cast<BSSimpleList<TESFaction*>*>(ourMalloc(sizeof(BSSimpleList<TESFaction*>)));
        if (v4) {
            v2 = ::new (v4) BSSimpleList<TESFaction*>;
        }
        else {
            v2 = nullptr;
        }
        pListofEnemyFactions = v2;
    }
    pListofEnemyFactions->AddHead(apFaction);
}

uint16_t Crime::GetSaveSize() {
    int16_t v5 = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        v5 = 6;
    }
    uint16_t isize = v5 + 32 + 4 * pActorsknowofCrime.ItemsInList() + 2;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 245,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize, 245,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
    }
    return isize;
}

void Crime::SaveGame() {
    uint32_t iactorID;
    uint32_t iblockHeader;
    uint16_t icount;
    uint16_t iblockSize;
    uint32_t icriminalID;
    uint32_t icrimetargetID;
    uint32_t iownershipID;
    uint32_t iobjectStolenID;
    iblockSize = 0;
    uint16_t* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // 'BLOK'.
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&breported, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(&bguardrespond, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(&ecrime, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&inumberstolen, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&iCrimeNumber, 4u);
    icrimetargetID = 0;
    if (pcrimetarget) {
        icrimetargetID = pcrimetarget->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&icrimetargetID, 4u);
    icriminalID = 0;
    if (pcriminal) {
        icriminalID = pcriminal->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&icriminalID, 4u);
    iobjectStolenID = 0;
    if (pObjectStolen) {
        iobjectStolenID = pObjectStolen->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&iobjectStolenID, 4u);
    iownershipID = 0;
    if (pOwnership) {
        iownershipID = pOwnership->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&iownershipID, 4u);
    icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
    for (BSSimpleList<Actor*>* plist = &pActorsknowofCrime; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        Actor* pactor = plist->m_item;
        iactorID = pactor->iFormID;
        pSaveLoadGameOLD->SaveNumericID(&iactorID, 4u);
        ++icount;
    }
    *pcount = icount;
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v1 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v1, iFlags, 301,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 301,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The two buffer positions are compared and subtracted as unsigned addresses.
        uintptr_t icurrentPosition = reinterpret_cast<uintptr_t>(pSaveLoadGameOLD->m_pBuffer);
        uintptr_t iblockStart = reinterpret_cast<uintptr_t>(pblockSize);
        if (icurrentPosition > iblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp", 301
            );
        }
        *pblockSize = static_cast<uint16_t>(icurrentPosition - iblockStart);
    }
}

void Crime::Save(BGSSaveGameBuffer* a2) {
    a2->SaveDataEndian(this, 4u, 0);
    a2->SaveDataEndian(&ecrime, 4u, 0);
    a2->SaveDataEndian(&breported, 1u, 0);
    a2->SaveDataEndian(&inumberstolen, 4u, 0);
    a2->SaveDataEndian(&iCrimeNumber, 4u, 0);
    a2->SaveDataEndian(&bguardrespond, 1u, 0);
    a2->SaveDataEndian(&iCrimeStamp, 4u, 0);
    fCrimeTimeStamp.SaveGame(a2);
    a2->SaveFormID(pcrimetarget, 0);
    a2->SaveFormID(pcriminal, 0);
    a2->SaveFormID(pObjectStolen, 0);
    a2->SaveFormID(pOwnership, 0);
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = a2->StartVariableSizedValue();
    for (Actor* pForm : pActorsknowofCrime) {
        if (pForm) {
            a2->SaveFormID(pForm, 0);
            ++isaveGameCount;
        }
    }
    a2->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    uint32_t isaveGameCount_2 = 0;
    uint32_t isaveGameBufferPos_2 = a2->StartVariableSizedValue();
    for (TESFaction* pForm_2 : *pListofEnemyFactions) {
        if (pForm_2) {
            a2->SaveFormID(pForm_2, 0);
            ++isaveGameCount_2;
        }
    }
    a2->SaveVariableSizedValue(isaveGameCount_2, isaveGameBufferPos_2);
}

void Crime::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    BSSimpleList<TESFaction*>* v6;
    Actor* pForm;
    apLoadGameBuffer->LoadData(this, 4u);
    apLoadGameBuffer->LoadData(&ecrime, 4u);
    apLoadGameBuffer->LoadData(&breported, 1u);
    apLoadGameBuffer->LoadData(&inumberstolen, 4u);
    apLoadGameBuffer->LoadData(&iCrimeNumber, 4u);
    apLoadGameBuffer->LoadData(&bguardrespond, 1u);
    apLoadGameBuffer->LoadData(&iCrimeStamp, 4u);
    fCrimeTimeStamp.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadFormID(pcrimetarget);
    apLoadGameBuffer->LoadFormID(pcriminal);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pObjectStolen = dynamic_cast<TESBoundObject*>(FormByNumericID);
    apLoadGameBuffer->LoadFormID(pOwnership);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        pForm = nullptr;
        // The witness list holds form IDs until Crime::InitItem resolves them.
        apLoadGameBuffer->LoadFormID(pForm);
        pActorsknowofCrime.AddHead(pForm);
    }
    BSSimpleList<TESFaction*>* v9 =
        static_cast<BSSimpleList<TESFaction*>*>(ourMalloc(sizeof(BSSimpleList<TESFaction*>)));
    if (v9) {
        v6 = ::new (v9) BSSimpleList<TESFaction*>;
    }
    else {
        v6 = nullptr;
    }
    this->pListofEnemyFactions = v6;
    uint32_t isaveGameCount_2 = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop_2 = 0; iloop_2 < isaveGameCount_2; ++iloop_2) {
        uint32_t v4 = apLoadGameBuffer->LoadFormID();
        TESForm* v5 = TESForm::GetFormByNumericID(v4);
        TESFaction* pForm_2 = dynamic_cast<TESFaction*>(v5);
        if (pForm_2) {
            this->pListofEnemyFactions->AddHead(pForm_2);
        }
    }
    if (!this->pListofEnemyFactions->m_item) {
        if (BSSimpleList<TESFaction*>* pfactions = this->pListofEnemyFactions) {
            delete pfactions;
        }
        this->pListofEnemyFactions = nullptr;
    }
}
