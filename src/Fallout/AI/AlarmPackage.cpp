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
            apSaveGameBuffer->SaveDataEndian(&ccrimeType, sizeof(ccrimeType), 0);
            apSaveGameBuffer->SaveDataEndian(&icrimeIndex, sizeof(icrimeIndex), 0);
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
        apLoadGameBuffer->LoadData(&ccrimeType, sizeof(ccrimeType));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&icrimeIndex), sizeof(icrimeIndex));
        Crime* pcrime = ReferenceProcessLists.GetCrimeByIndex(
            static_cast<PackageNS::CRIME_TYPE>(ccrimeType),
            icrimeIndex
        );
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
    uint16_t istartSize = isize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        isize += 6;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize - istartSize,
                pheader->iFormID,
                pform->GetFormEditorID(),
                pheader->iFlags,
                491,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize - istartSize,
                491,
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    uint16_t icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, sizeof(icount));
    // The walk deliberately starts at null, not at Crimes, so it saves no crime and the count stays
    // 0.
    for (BSSimpleList<Crime*>* plist = nullptr; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        Crime* pcrime = plist->m_item;
        char ccrimeType = pcrime->ecrime;
        pSaveLoadGameOLD->SaveGameDataOLD(&ccrimeType, sizeof(ccrimeType));
        uint16_t icrimeIndex = ReferenceProcessLists.GetCrimeIndex(
            static_cast<PackageNS::CRIME_TYPE>(ccrimeType),
            pcrime
        );
        pSaveLoadGameOLD->SaveGameDataOLD(&icrimeIndex, sizeof(icrimeIndex));
        ++icount;
    }
    *pcount = icount;
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* peditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(icurrentBufferPosition - itempStartPosition),
                pheader->iFormID,
                peditorID,
                iFlags,
                526,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(icurrentBufferPosition - itempStartPosition),
                526,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp",
                526
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
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        // 'BLOK'.
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp",
                    534,
                    pheader->iFormID,
                    pform->GetFormEditorID(),
                    pheader->cVersion,
                    pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp",
                    534,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    LoadGameDataOLD(&icount, sizeof(icount));
    for (int32_t i = 0; i < icount; ++i) {
        pSaveLoadGameOLD->LoadGameDataOLD(&ccrimeType, sizeof(ccrimeType));
        pSaveLoadGameOLD->LoadGameDataOLD(&icrimeIndex, sizeof(icrimeIndex));
        ReferenceProcessLists.GetCrimeByIndex(static_cast<PackageNS::CRIME_TYPE>(ccrimeType), icrimeIndex);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* iblockEnd = istartBufferPosition + iblockSize;
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= iblockEnd) {
                if (icurrentBufferPosition < iblockEnd) {
                    uint32_t iFlags = pheader->iFlags;
                    int32_t cVersion = pheader->cVersion;
                    const char* peditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(iblockEnd - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp",
                        554,
                        pheader->iFormID,
                        peditorID,
                        cVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pheader->iFlags;
                int32_t cVersion = pheader->cVersion;
                const char* peditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - iblockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp",
                    554,
                    pheader->iFormID,
                    peditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (icurrentBufferPosition <= iblockEnd) {
            if (icurrentBufferPosition < iblockEnd) {
                int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    static_cast<int32_t>(iblockEnd - icurrentBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp",
                    554,
                    iCurrentVersion
                );
            }
        }
        else {
            int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(icurrentBufferPosition - iblockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp",
                554,
                iCurrentVersion
            );
        }
    }
}

void AlarmPackage::RemoveCrime(Crime* apcrime, Actor* apActor) {
    Crimes.Remove(apcrime);
    if (Crimes.IsEmpty()) {
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
    if (pDataHandler && !pDataHandler->GetClearingData()) {
        ClearActorKnowList();
        ReferenceProcessLists.RemoveCrimeGlobalCrimeList(this);
        if (pcrimetarget && pcrimetarget->m_Extra.GetPlayerCrimeList()) {
            Crime* pcrime = this;
            pcrimetarget->m_Extra.GetPlayerCrimeList()->Remove(pcrime);
        }

        for (BSSimpleList<TESFaction*>* plist = pListofEnemyFactions; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            TESFaction* pfac = plist->m_item;
            if (!ReferenceProcessLists.IsEnemyFactionofOtherCrime(pfac, this)) {
                pfac->SetPCAnEnemy(false);
            }
        }
        Actor* pact = static_cast<Actor*>(pcrimetarget);
        if (pact && pact->IsActor()) {
            BSSimpleList<Crime*>* pcrimelist = pact->m_Extra.GetPlayerCrimeList();
            if (pcrimelist) {
                Crime* pcrime = this;
                pcrimelist->Remove(pcrime);
            }
        }

        // prespond is never advanced, so a non-empty result loops for ever.
        BSSimpleList<TESObjectREFR*>* prespond = ReferenceProcessLists.GetActorsProcessCrimeAlarm(this, true);
        while (prespond) {
            pact = static_cast<Actor*>(prespond->m_item);
            if (pact->IsAlarmed()) {
                pact->EndInterruptPackage(false);
            }
            BSSimpleList<Crime*>* pcrimelist = pact->m_Extra.GetPlayerCrimeList();
            if (pcrimelist) {
                Crime* pcrime = this;
                pcrimelist->Remove(pcrime);
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
    TESObjectREFR* ptarget = nullptr;
    if (pcrimetarget) {
        TESForm* pform = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pcrimetarget)));
        ptarget = dynamic_cast<TESObjectREFR*>(pform);
    }
    pcrimetarget = ptarget;
    Actor* pactor = nullptr;
    if (pcriminal) {
        TESForm* pform = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pcriminal)));
        pactor = dynamic_cast<Actor*>(pform);
    }
    pcriminal = pactor;
    TESForm* pownership = nullptr;
    if (pOwnership) {
        pownership = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOwnership)));
    }
    pOwnership = pownership;
    BSSimpleList<Actor*>* plist = &pActorsknowofCrime;
    BSSimpleList<Actor*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        Actor* pForm = plist->m_item;
        bool bremove = false;
        if (pForm) {
            TESForm* pform = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
            pForm = dynamic_cast<Actor*>(pform);
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
                plastList->Remove(plist->m_item);
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
    return pActorsknowofCrime.IsInList(apActor);
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
        BSSimpleList<Crime*>* pcrimelist = pact->m_Extra.GetPlayerCrimeList();
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
        pListofEnemyFactions = new BSSimpleList<TESFaction*>;
    }
    pListofEnemyFactions->AddHead(apFaction);
}

uint16_t Crime::GetSaveSize() {
    int16_t iheaderSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iheaderSize = 6;
    }
    uint16_t isize = iheaderSize + 32 + 4 * pActorsknowofCrime.ItemsInList() + 2;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize,
                pheader->iFormID,
                pform->GetFormEditorID(),
                pheader->iFlags,
                245,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize,
                245,
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&breported, sizeof(breported));
    pSaveLoadGameOLD->SaveGameDataOLD(&bguardrespond, sizeof(bguardrespond));
    pSaveLoadGameOLD->SaveGameDataOLD(&ecrime, sizeof(ecrime));
    pSaveLoadGameOLD->SaveGameDataOLD(&inumberstolen, sizeof(inumberstolen));
    pSaveLoadGameOLD->SaveGameDataOLD(&iCrimeNumber, sizeof(iCrimeNumber));
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
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, sizeof(icount));
    for (BSSimpleList<Actor*>* plist = &pActorsknowofCrime; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        Actor* pactor = plist->m_item;
        iactorID = pactor->iFormID;
        pSaveLoadGameOLD->SaveNumericID(&iactorID, 4u);
        ++icount;
    }
    *pcount = icount;
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* peditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(icurrentBufferPosition - itempStartPosition),
                pheader->iFormID,
                peditorID,
                iFlags,
                301,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(icurrentBufferPosition - itempStartPosition),
                301,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\AlarmPackage.cpp",
                301
            );
        }
        *pblockSize = static_cast<uint16_t>(icurrentPosition - iblockStart);
    }
}

void Crime::Save(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&iRefCount, sizeof(iRefCount), 0);
    apSaveGameBuffer->SaveDataEndian(&ecrime, sizeof(ecrime), 0);
    apSaveGameBuffer->SaveDataEndian(&breported, sizeof(breported), 0);
    apSaveGameBuffer->SaveDataEndian(&inumberstolen, sizeof(inumberstolen), 0);
    apSaveGameBuffer->SaveDataEndian(&iCrimeNumber, sizeof(iCrimeNumber), 0);
    apSaveGameBuffer->SaveDataEndian(&bguardrespond, sizeof(bguardrespond), 0);
    apSaveGameBuffer->SaveDataEndian(&iCrimeStamp, sizeof(iCrimeStamp), 0);
    fCrimeTimeStamp.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(pcrimetarget, 0);
    apSaveGameBuffer->SaveFormID(pcriminal, 0);
    apSaveGameBuffer->SaveFormID(pObjectStolen, 0);
    apSaveGameBuffer->SaveFormID(pOwnership, 0);
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (Actor* pForm : pActorsknowofCrime) {
        if (pForm) {
            apSaveGameBuffer->SaveFormID(pForm, 0);
            ++isaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    isaveGameCount = 0;
    isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (TESFaction* pfaction : *pListofEnemyFactions) {
        if (pfaction) {
            apSaveGameBuffer->SaveFormID(pfaction, 0);
            ++isaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
}

void Crime::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    Actor* pForm;
    apLoadGameBuffer->LoadData(&iRefCount, sizeof(iRefCount));
    apLoadGameBuffer->LoadData(&ecrime, sizeof(ecrime));
    apLoadGameBuffer->LoadData(&breported, sizeof(breported));
    apLoadGameBuffer->LoadData(&inumberstolen, sizeof(inumberstolen));
    apLoadGameBuffer->LoadData(&iCrimeNumber, sizeof(iCrimeNumber));
    apLoadGameBuffer->LoadData(&bguardrespond, sizeof(bguardrespond));
    apLoadGameBuffer->LoadData(&iCrimeStamp, sizeof(iCrimeStamp));
    fCrimeTimeStamp.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadFormID(pcrimetarget);
    apLoadGameBuffer->LoadFormID(pcriminal);
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pform = TESForm::GetFormByNumericID(uiFormID);
    pObjectStolen = dynamic_cast<TESBoundObject*>(pform);
    apLoadGameBuffer->LoadFormID(pOwnership);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        pForm = nullptr;
        // The witness list holds form IDs until Crime::InitItem resolves them.
        apLoadGameBuffer->LoadFormID(pForm);
        pActorsknowofCrime.AddHead(pForm);
    }
    this->pListofEnemyFactions = new BSSimpleList<TESFaction*>;
    isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        uiFormID = apLoadGameBuffer->LoadFormID();
        pform = TESForm::GetFormByNumericID(uiFormID);
        TESFaction* pfaction = dynamic_cast<TESFaction*>(pform);
        if (pfaction) {
            this->pListofEnemyFactions->AddHead(pfaction);
        }
    }
    if (!this->pListofEnemyFactions->m_item) {
        if (BSSimpleList<TESFaction*>* pfactions = this->pListofEnemyFactions) {
            delete pfactions;
        }
        this->pListofEnemyFactions = nullptr;
    }
}
