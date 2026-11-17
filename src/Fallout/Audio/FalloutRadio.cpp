#include "Fallout/Audio/FalloutRadio.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Audio/RadioEntry.h"
#include "Fallout/Dialogue/Conversation.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "platform/System.h"

#include <cmath>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> iRadioUpdateInterval("iRadioUpdateInterval:Audio", 75);
SettingT<INISettingCollection> iRadioStationTimeout("iRadioStationTimeout:Audio", 1000);
SettingT<INISettingCollection> fRadioStaticAtOuterRadiusPct("fRadioStaticAtOuterRadiusPct:Audio", 0.1f);
SettingT<INISettingCollection> fRadioDialogMute("fRadioDialogMute:Audio", 0.5f);
SettingT<INISettingCollection> fCreatureRadioMin("fCreatureRadioMin:Audio", 200.0f);
SettingT<INISettingCollection> fCreatureRadioMax("fCreatureRadioMax:Audio", 2000.0f);
BSSoundHandle FalloutRadio::PipboyVoiceHndl;
BSSoundHandle FalloutRadio::PipboyStaticHndl;
BSSimpleList<FalloutRadio::FORadioStation*> FalloutRadio::ActiveStationList;
BSSimpleList<FalloutRadio::FORadioReceiver*> FalloutRadio::ActiveRadioList;
BSSimpleList<TESObjectREFR*> FalloutRadio::DiscoveredStationList;
BSCriticalSection FalloutRadio::RadioUpdateCriticalSection;

FalloutRadio::FORadioReceiver::FORadioReceiver() {}

FalloutRadio::FORadioReceiver::~FORadioReceiver() {}

void FalloutRadio::FORadioReceiver::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESForm* formByNumericID;
    TESObjectREFR* v3;
    if (pRadio) {
        formByNumericID = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pRadio));
        v3 = dynamic_cast<TESObjectREFR*>(formByNumericID);
    }
    else {
        v3 = nullptr;
    }
    pRadio = v3;
}

void FalloutRadio::FORadioReceiver::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveFormID(pRadio, (uint32_t)0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bShouldBePlaying)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bJustStarted)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bInRange)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bIsActor)), 1u, 0);
}

void FalloutRadio::FORadioReceiver::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    // Loads the saved form ID into the pointer field, to be resolved after the load.
    apLoadGameBuffer->LoadFormID(pRadio);
    apLoadGameBuffer->LoadData(&bShouldBePlaying, 1u);
    apLoadGameBuffer->LoadData(&bJustStarted, 1u);
    apLoadGameBuffer->LoadData(&bInRange, 1u);
    apLoadGameBuffer->LoadData(&bIsActor, 1u);
}

void FalloutRadio::FORadioReceiver::operator delete(void* apMemory) {
    ourFree(apMemory);
}

FalloutRadio::StationState::StationState()
  : pConvo(nullptr), pNextConvo(nullptr), iCurrentItemStartTime(0), iCurrentItemDuration(0), iStationFlags(0) {}

FalloutRadio::StationState::~StationState() {
    if (pNextConvo != pConvo) {
        delete pNextConvo;
    }
    pNextConvo = nullptr;
    delete pConvo;
    pConvo = nullptr;
}

bool FalloutRadio::StationState::ShouldStartPaused() {
    return ((iStationFlags & 256) != 0);
}

bool FalloutRadio::StationState::IsContBroadcast() {
    return ((iStationFlags & 32) != 0);
}

void FalloutRadio::StationState::SetJustTurnedOn(bool abVal) {
    uint32_t uiStationFlags;
    uint32_t v3;
    if (abVal) {
        iStationFlags |= 2u;
        uiStationFlags = iStationFlags;
    }
    else {
        iStationFlags &= (~2u);
        v3 = iStationFlags;
    }
    uint32_t v4 = iStationFlags;
}

void FalloutRadio::StationState::SetTransmitting(bool abVal) {
    uint32_t uiStationFlags;
    uint32_t v3;
    if (abVal) {
        iStationFlags |= 16;
        uiStationFlags = iStationFlags;
    }
    else {
        iStationFlags &= (~0x10u);
        v3 = iStationFlags;
    }
    uint32_t v4 = iStationFlags;
}

bool FalloutRadio::StationState::OutOfPBRange() {
    return ((iStationFlags & 4) != 0);
}

bool FalloutRadio::StationState::IsPlayingBreakIn() {
    return ((iStationFlags & 128) != 0);
}

uint8_t FalloutRadio::StationState::GetStationPower() {
    return cStationPower;
}

void FalloutRadio::StationState::SetPlayingSong(bool abVal) {
    uint32_t uiStationFlags;
    uint32_t v3;
    if (abVal) {
        iStationFlags |= 8;
        uiStationFlags = iStationFlags;
    }
    else {
        iStationFlags &= (~8u);
        v3 = iStationFlags;
    }
    uint32_t v4 = iStationFlags;
}

void FalloutRadio::StationState::SetPlayingStatic(bool abVal) {
    uint32_t uiStationFlags;
    uint32_t v3;
    if (abVal) {
        iStationFlags |= 64;
        uiStationFlags = iStationFlags;
    }
    else {
        iStationFlags &= (~0x40u);
        v3 = iStationFlags;
    }
    uint32_t v4 = iStationFlags;
}

void FalloutRadio::StationState::SetOutOfPBRange(bool abVal) {
    uint32_t uiStationFlags;
    uint32_t v3;
    if (abVal) {
        iStationFlags |= 4;
        uiStationFlags = iStationFlags;
    }
    else {
        iStationFlags &= (~4u);
        v3 = iStationFlags;
    }
    uint32_t v4 = iStationFlags;
}

void FalloutRadio::StationState::SetShouldBePlaying(bool abVal) {
    uint32_t uiFlags;
    uint32_t v3;
    if (abVal) {
        iStationFlags |= 1;
        uiFlags = iStationFlags;
    }
    else {
        iStationFlags &= (~1u);
        v3 = iStationFlags;
    }
    uint32_t v4 = iStationFlags;
}

void FalloutRadio::StationState::SetContBroadcast(bool abVal) {
    FalloutRadio::StationState* result;
    uint32_t uiFlags;
    uint32_t v4;
    if (abVal) {
        result = this;
        iStationFlags |= 32;
        uiFlags = iStationFlags;
    }
    else {
        iStationFlags &= (~0x20u);
        result = this;
        v4 = iStationFlags;
    }
    uint32_t v5 = iStationFlags;
}

bool FalloutRadio::StationState::ShouldBePlaying() {
    return ((iStationFlags & 1) != 0);
}

bool FalloutRadio::StationState::IsPlayingSong() {
    return ((iStationFlags & 8) != 0);
}

void FalloutRadio::StationState::SetPlayingBreakIn(bool abVal) {
    if (abVal) {
        iStationFlags |= 0x80;
    }
    else {
        iStationFlags &= ~0x80u;
    }
}

bool FalloutRadio::StationState::IsTransmitting() {
    return (((int32_t)(iStationFlags & 16)) != 0);
}

void FalloutRadio::StationState::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESObjectREFR* a1;
    char apData;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCurrentItemStartTime), 4u);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCurrentItemDuration), 4u);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cStationPower), 1u);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cTargetPower), 1u);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iStationFlags), 4u);
    uint32_t VariableSizedValue = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < VariableSizedValue; ++i) {
        a1 = nullptr;
        apLoadGameBuffer->LoadFormID(a1);
        FalloutRadio::FORadioReceiver* a2 = FalloutRadio::GetActiveReceiver(a1);
        if (a2) {
            StationUsers.AddHead(a2);
        }
    }
    apData = 0;
    apLoadGameBuffer->LoadData(&apData, 1u);
    if (apData) {
        Conversation* v2;
        Conversation* apMemory = static_cast<Conversation*>(ourMalloc(sizeof(Conversation)));
        if (apMemory) {
            v2 = ::new (apMemory) Conversation;
        }
        else {
            v2 = 0;
        }
        pConvo = v2;
        pConvo->LoadGameBGS(static_cast<BGSLoadFormBuffer*>(apLoadGameBuffer));
    }
}

void FalloutRadio::StationState::SetStartPaused(bool abVal) {
    if (abVal) {
        this->iStationFlags |= 256;
        uint32_t iStationFlags = this->iStationFlags;
    }
    else {
        this->iStationFlags &= ~256u;
        uint32_t v3 = this->iStationFlags;
    }
    uint32_t v4 = this->iStationFlags;
}

void FalloutRadio::StationState::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    bool bhasConvo;
    int32_t imodifiedStartTime;
    imodifiedStartTime = BSAudio::QInstance().GetSynchTime() - iCurrentItemStartTime;
    if (iCurrentItemDuration <= 0) {
        imodifiedStartTime = 0;
    }
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&imodifiedStartTime), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iCurrentItemDuration), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cStationPower), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cTargetPower), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iStationFlags), 4u, 0);
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (FalloutRadio::FORadioReceiver* preceiver : StationUsers) {
        if (preceiver) {
            apSaveGameBuffer->SaveFormID(preceiver->pRadio, 0u);
            ++isaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    bhasConvo = pConvo != 0;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bhasConvo), 1u, 0);
    if (bhasConvo) {
        pConvo->SaveGameBGS(static_cast<BGSSaveFormBuffer*>(apSaveGameBuffer));
    }
}

void FalloutRadio::StationState::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    if (pConvo) {
        pConvo->InitItem(apLoadGameBuffer);
    }
    BSSimpleList<FalloutRadio::FORadioReceiver*>* plist = &StationUsers;
    BSSimpleList<FalloutRadio::FORadioReceiver*>* plastList = 0;
    while (plist && !plist->IsEmpty()) {
        bool bremove;
        FalloutRadio::FORadioReceiver* preceiver = plist->m_item;
        if (preceiver) {
            bremove = preceiver->pRadio == 0;
        }
        else {
            bremove = 1;
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
}

void FalloutRadio::StationState::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    if ((&BSAudio::QInstance())->GetSynchTime() >= iCurrentItemStartTime && iCurrentItemDuration > 0) {
        iCurrentItemStartTime = (&BSAudio::QInstance())->GetSynchTime() - iCurrentItemStartTime;
    }
    else {
        iCurrentItemStartTime = 0;
        iCurrentItemDuration = 1;
        if (pConvo) {
            delete pConvo;
        }
        pConvo = 0;
    }
    for (BSSimpleList<FalloutRadio::FORadioReceiver*>* users = &StationUsers;
         users && (!users->IsEmpty() && users->m_item); users = users->m_pkNext) {
        if (!users->m_item->pRadio ||
            (!users->m_item->pRadio->GetObjectReference() ||
             (users->m_item->pRadio->GetObjectReference()->GetFormType() == ACTI_ID &&
              !static_cast<TESObjectACTI*>(users->m_item->pRadio->GetObjectReference())->pRadioStation))) {
            users->m_item->Hndl.Stop();
            users->m_item->Hndl.Release();
            users->m_item->StaticHndl.Stop();
            users->m_item->StaticHndl.Release();
            users->m_item->bShouldBePlaying = 0;
        }
    }
}

// The constructor deliberately leaves pStation unset, as the game does.
FalloutRadio::FORadioStation::FORadioStation() {}

FalloutRadio::FORadioStation::~FORadioStation() {}

void FalloutRadio::FORadioStation::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadFormID(pStation);
    CurrentState.LoadGame(apLoadGameBuffer);
}

void FalloutRadio::FORadioStation::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveFormID(pStation, (uint32_t)0);
    CurrentState.SaveGame(apSaveGameBuffer);
}

void FalloutRadio::FORadioStation::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESObjectREFR* v3;
    if (pStation) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pStation)));
        v3 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    }
    else {
        v3 = 0;
    }
    pStation = v3;
    CurrentState.InitLoadGame(apLoadGameBuffer);
}

void FalloutRadio::FORadioStation::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    CurrentState.FinishLoadGame(apLoadGameBuffer);
}

void FalloutRadio::FORadioStation::operator delete(void* apMemory) {
    ourFree(apMemory);
}

bool FalloutRadio::GetBroadcastState(TESObjectREFR* apTargetStation) {
    if (FalloutRadio::bShutdown) {
        return 0;
    }
    bool bretValue = 0;
    FalloutRadio::FORadioStation* pstation =
        FalloutRadio::GetActiveStation(static_cast<BGSTalkingActivator*>(apTargetStation->GetObjectReference()));
    if (pstation) {
        return pstation->CurrentState.IsContBroadcast();
    }
    return bretValue;
}

void FalloutRadio::StartRadioConversation(TESObjectREFR* apTargetStation, TESTopic* apTopic) {
    FalloutRadio::FORadioStation* pstation = 0;
    if (!FalloutRadio::bShutdown) {
        TESTopic* ptopicToUse;
        for (FalloutRadio::FORadioStation*& rpItem : FalloutRadio::ActiveStationList.ItemsUntilNull()) {
            if (rpItem->pStation == apTargetStation) {
                pstation = rpItem;
                break;
            }
        }
        if (!pstation) {
            pstation = FalloutRadio::InitStation(apTargetStation, 0);
        }
        if (apTopic) {
            ptopicToUse = apTopic;
        }
        else {
            ptopicToUse = TESTopic::GetTopic(DT_RADIO, 0);
        }
        if (pstation) {
            if (!pstation->pStation->GetDisabled()) {
                bool bwasContBroadcast = pstation->CurrentState.IsContBroadcast();
                FalloutRadio::RadioUpdateCriticalSection.Enter("FalloutRadio::StartRadioConversation");
                if (pstation->CurrentState.pConvo) {
                    while (pstation->CurrentState.pConvo->GetNext()) {
                        ;
                    }
                }
                if (pstation->CurrentState.IsTransmitting()) {
                    pstation->CurrentState.SetContBroadcast(0);
                    pstation->CurrentState.iCurrentItemDuration = 1;
                    FalloutRadio::UpdateStation(pstation);
                }
                if (FalloutRadio::pCurrentPipboyStation == pstation && FalloutRadio::bPipboyRadioEnabled) {
                    FalloutRadio::PipboyVoiceHndl.Stop();
                    FalloutRadio::PipboyVoiceHndl.Release();
                    if (FalloutRadio::GetIsDialogueOnlyRadio()) {
                        FalloutAudioMedia::MediaClose();
                        FalloutAudioMedia::MediaSetRadio(1);
                        FalloutAudioMedia::MediaOpen(MEDIA_RADIO, 0, 1000u, 0, 0, 0.0f, 0);
                        FalloutAudioMedia::MediaPlay();
                    }
                }
                for (FalloutRadio::FORadioReceiver* puser : pstation->CurrentState.StationUsers.ItemsUntilNull()) {
                    puser->Hndl.Stop();
                    puser->Hndl.Release();
                }
                FalloutRadio::GetNextConversation(pstation, ptopicToUse);
                pstation->CurrentState.iCurrentItemDuration = 0;
                pstation->CurrentState.pConvo->PrecacheSounds();
                pstation->CurrentState.pConvo->FirstItem();
                if (pstation->CurrentState.pConvo->GetCurrentItem()) {
                    if (FalloutRadio::pCurrentPipboyStation == pstation && FalloutRadio::bPipboyRadioEnabled) {
                        FalloutRadio::PipboyVoiceHndl.Stop();
                        FalloutRadio::PipboyVoiceHndl.Release();
                        if (FalloutRadio::GetIsDialogueOnlyRadio()) {
                            FalloutAudioMedia::MediaClose();
                            FalloutAudioMedia::MediaSetRadio(1);
                            FalloutAudioMedia::MediaOpen(MEDIA_RADIO, 0, 1000u, 0, 0, 0.0f, 0);
                            FalloutAudioMedia::MediaPlay();
                        }
                    }
                    pstation->CurrentState.pConvo->GetCurrentItem()->FirstResponse();
                }
                else {
                    Conversation* apThis = pstation->CurrentState.pConvo;
                    if (apThis) {
                        delete apThis;
                    }
                    pstation->CurrentState.pConvo = 0;
                    pstation->CurrentState.iCurrentItemDuration = 1;
                }
                if (pstation->CurrentState.iCurrentItemDuration > 1) {
                    pstation->CurrentState.iCurrentItemDuration = 0;
                }
                pstation->CurrentState.iCurrentItemStartTime = BSAudio::QInstance().GetSynchTime() + 50;
                pstation->CurrentState.SetPlayingBreakIn(1);
                pstation->CurrentState.SetContBroadcast(bwasContBroadcast);
                FalloutRadio::RadioUpdateCriticalSection.Leave();
            }
        }
    }
}

void FalloutRadio::ClearInactiveReceivers(bool bRemoveAll) {
    BSSimpleList<FalloutRadio::FORadioReceiver*>* pactReceivers = &FalloutRadio::ActiveRadioList;
    FalloutRadio::FORadioReceiver* pemitter = 0;
    while (pactReceivers && !pactReceivers->IsEmpty()) {
        if (pactReceivers->m_item->bShouldBePlaying && !bRemoveAll) {
            pactReceivers = pactReceivers->m_pkNext;
        }
        else {
            pemitter = pactReceivers->m_item;
            FalloutRadio::ActiveRadioList.Remove(pactReceivers->m_item);
            pactReceivers = &FalloutRadio::ActiveRadioList;
            for (BSSimpleList<FalloutRadio::FORadioStation*>* pactStations = &FalloutRadio::ActiveStationList;
                 pactStations && !pactStations->IsEmpty(); pactStations = pactStations->m_pkNext) {
                FalloutRadio::FORadioStation* pstation = pactStations->m_item;
                if (pstation) {
                    while (pstation->CurrentState.StationUsers.IsInList(pemitter)) {
                        pemitter->Hndl.Stop();
                        pemitter->Hndl.Release();
                        pstation->CurrentState.StationUsers.Remove(pemitter);
                    }
                }
            }
            if (pemitter) {
                delete pemitter;
            }
        }
    }
}

void FalloutRadio::UpdateStations(bool abUpdateSignals) {
    FNVD_INLINED_LOCAL(BSSoundHandle, alertSound_2, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, alertSound, std::nullopt, std::nullopt);
    BSSimpleList<TESObjectREFR*> stationlist;
    BSSimpleList<float> stationDistances;
    if (FalloutRadio::bShutdown) {}
    else {
        if (abUpdateSignals) {
            BGSTalkingActivator::GetRadioStationsInRangeOfRef(pPlayer, &stationlist, &stationDistances);
            for (TESObjectREFR*& rpItem : stationlist.ItemsUntilNull()) {
                if ((!rpItem->GetDisabled() || Script::IsPendingEnabledReference(rpItem)) &&
                    !FalloutRadio::DiscoveredStationList.IsInList(rpItem)) {
                    if (!FalloutRadio::bFirstUpdate) {
                        BSStringT<char> strtemp;
                        BSAudio* v1 = &BSAudio::QInstance();
                        v1->GetSoundHandleByName(
                            &alertSound, "UIRadioSignalFound",
                            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_RADIO
                        );
                        alertSound.Play(0);
                        const char* FullName = rpItem->GetFullName();
                        strtemp.SPrintF(sRadioStationDiscovered.operator const char*(), FullName);
                        Interface::ShowMessage(
                            strtemp.pString, 0, "Interface\\Icons\\Message Icons\\glow_message_radio_tower.dds", 0,
                            2.0f, 0
                        );
                    }
                    FalloutRadio::DiscoveredStationList.AddHead(rpItem);
                }
            }
            FalloutRadio::bFirstUpdate = 0;
        }
        NiPlane* v23 = 0;
        NiPlane* v25 = 0;
        bool bmatch = 0;
        float fdistance = 3.4028235e+38f;
        float v16 = 0.0f;
        float v18 = 0.0f;
        uint8_t v14 = 0;
        float fstaticDistance = 1.0 - fRadioStaticAtOuterRadiusPct.Float();
        for (BSSimpleList<FalloutRadio::FORadioStation*>* pactStations = &FalloutRadio::ActiveStationList;
             pactStations && !pactStations->IsEmpty(); pactStations = pactStations->m_pkNext) {
            BSSimpleList<TESObjectREFR*>* stations = &stationlist;
            BSSimpleList<float>* pdistances = &stationDistances;
            bmatch = 0;
            fdistance = 3.4028235e+38f;
            FalloutRadio::FORadioStation* pstation = pactStations->m_item;
            if (pstation) {
                while (abUpdateSignals && (stations && pdistances)) {
                    BSSimpleList<TESObjectREFR*>* v2 = stations;
                    if (v2->m_item == pstation->pStation) {
                        bmatch = 1;
                        uint8_t isignalPower = 100;
                        pstation->CurrentState.SetOutOfPBRange(0);
                        fdistance = pdistances->m_item;
                        float fradioRadius = pstation->pStation->GetRadioStationRadius();
                        if (pstation->pStation->GetRadioStaticPct() != 0.0) {
                            fstaticDistance = 1.0 - pstation->pStation->GetRadioStaticPct() / 100.0;
                        }
                        if (pstation->pStation->GetRadioStationRangeType() == RADIO_RANGE_RADIUS &&
                            fradioRadius > 0.0 && fdistance > 0.0) {
                            float fstaticRatio = fdistance / fradioRadius;
                            if (fstaticDistance < static_cast<double>(fstaticRatio)) {
                                isignalPower = 100 -
                                    static_cast<int32_t>(
                                                   (fstaticRatio - fstaticDistance) / (1.0 - fstaticDistance) * 100.0
                                    );
                            }
                        }
                        pstation->CurrentState.cTargetPower = isignalPower;
                        stations = 0;
                        pdistances = 0;
                    }
                    else {
                        stations = stations->m_pkNext;
                        BSSimpleList<float>* FormHealth_0 = pdistances->m_pkNext;
                    }
                }
                if (abUpdateSignals && !bmatch) {
                    pstation->CurrentState.SetOutOfPBRange(1);
                    pstation->CurrentState.cTargetPower = 0;
                }
                if (bmatch && FalloutRadio::pLostPipboyStation == pstation) {
                    BSStringT<char> strtemp_2;
                    BSAudio* v3 = &BSAudio::QInstance();
                    v3->GetSoundHandleByName(
                        &alertSound_2, "UIRadioSignalFound",
                        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                            BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_RADIO
                    );
                    alertSound_2.Play(0);
                    const char* v5 = pstation->pStation->GetFullName();
                    strtemp_2.SPrintF(sRadioStationDiscovered.operator const char*(), v5);
                    Interface::ShowMessage(
                        strtemp_2.pString, 0, "Interface\\Icons\\Message Icons\\glow_message_radio_tower.dds", 0, 2.0f,
                        0
                    );
                    FalloutRadio::PipboyRadioEnable(1);
                    FalloutRadio::TunePipboyToStation(pstation->pStation, 0);
                    FalloutRadio::pLostPipboyStation = 0;
                }
                FalloutRadio::UpdateStation(pstation);
            }
        }
        stationDistances.RemoveAll();
        stationlist.RemoveAll();
    }
}

FalloutRadio::FORadioStation* FalloutRadio::GetActiveStation(BGSTalkingActivator* apRequestedStation) {
    FalloutRadio::FORadioStation* preturnValue = 0;
    BSSimpleList<FalloutRadio::FORadioStation*>* pactStations = &FalloutRadio::ActiveStationList;
    RADIO_RANGE_TYPE erangeType = RADIO_RANGE_RADIUS;
    if (FalloutRadio::bShutdown) {
        return 0;
    }
    while (pactStations && !pactStations->IsEmpty()) {
        if (apRequestedStation == pactStations->m_item->pStation->GetObjectReference() &&
            (!preturnValue ||
             ((preturnValue->pStation->GetDisabled() && !pactStations->m_item->pStation->GetDisabled()) ||
              ((erangeType == RADIO_RANGE_RADIUS && pactStations->m_item->pStation->GetRadioStationRangeType()) ||
               (erangeType != RADIO_RANGE_EVERYWHERE &&
                pactStations->m_item->pStation->GetRadioStationRangeType() == RADIO_RANGE_EVERYWHERE))))) {
            preturnValue = pactStations->m_item;
            erangeType = preturnValue->pStation->GetRadioStationRangeType();
        }
        pactStations = pactStations->m_pkNext;
    }
    return preturnValue;
}

bool FalloutRadio::SetIsDialogueOnlyRadio(bool abDialogueOnly) {
    FalloutRadio::bIsDialogueOnlyRadio = (bool)((uint8_t)abDialogueOnly);
    return (bool)((uint8_t)abDialogueOnly);
}

bool FalloutRadio::Update(bool abForceStationUpdate) {
    bool v4 = 0;
    if (!FalloutRadio::iLastRadioUpdate) {
        FalloutRadio::iLastRadioUpdate = appTimer.GetTime();
    }
    uint32_t v1 = appTimer.GetTime() - FalloutRadio::iLastRadioUpdate;
    if (v1 > iRadioUpdateInterval.UInt() && !Interface::GetLoadingMenuVisible()) {
        v4 = 1;
        FalloutRadio::iLastRadioUpdate = appTimer.GetTime();
    }
    if (pPlayer) {
        if (pPlayer->GetParentCell()) {
            TESObjectCELL* ParentCell = pPlayer->GetParentCell();
            if (ParentCell->GetInterior()) {
                v4 = !FalloutRadio::bWasInInterior;
                FalloutRadio::bWasInInterior = 1;
            }
        }
    }
    if (abForceStationUpdate) {
        v4 = 1;
    }
    if ((FalloutRadio::iQueuedState & 1) != 0) {
        FalloutRadio::iRadioState &= ~2u;
        FalloutRadio::iRadioState |= 1u;
    }
    else {
        if ((FalloutRadio::iQueuedState & 2) != 0) {
            FalloutRadio::iRadioState &= ~1u;
            FalloutRadio::iRadioState |= 2u;
        }
    }
    FalloutRadio::iQueuedState = 0;
    FalloutRadio::ClearInactiveReceivers(0);
    if (!FalloutRadio::pRadioAttenuationModel) {
        FalloutRadio::pRadioAttenuationModel = pDataHandler->GetSound("AMLRadio");
    }
    FalloutRadio::UpdateStations(v4);
    FalloutRadio::ClearInactiveReceivers(0);
    FalloutRadio::iRadioState = 0;
    return 1;
}

void FalloutRadio::ActivateRadio(TESObjectREFR* apRadio, bool abOn) {
    if (!FalloutRadio::bShutdown) {
        FalloutRadio::FORadioReceiver* a2[2];
        BGSTalkingActivator* prequestedStation = 0;
        a2[0] = FalloutRadio::GetActiveReceiver(apRadio);
        a2[1] = 0;
        char v6 = 0;
        if (!a2[0] || ((!abOn || !a2[0]->bShouldBePlaying) && (abOn || a2[0]->bShouldBePlaying))) {
            if (apRadio) {
                if (apRadio->GetObjectReference()->GetFormType() == ACTI_ID) {
                    prequestedStation = static_cast<TESObjectACTI*>(apRadio->GetObjectReference())->pRadioStation;
                }
            }
            if (prequestedStation) {
                FalloutRadio::FORadioStation* pstation = FalloutRadio::GetActiveStation(prequestedStation);
                if (abOn) {
                    if (pstation || (pstation = FalloutRadio::InitStation(apRadio, 0)) != 0) {
                        if (!a2[0]) {
                            FalloutRadio::FORadioReceiver* pemitter;
                            FalloutRadio::FORadioReceiver* apMemory = static_cast<FalloutRadio::FORadioReceiver*>(
                                ourMalloc(sizeof(FalloutRadio::FORadioReceiver))
                            );
                            if (apMemory) {
                                pemitter = ::new (apMemory) FalloutRadio::FORadioReceiver;
                            }
                            else {
                                pemitter = 0;
                            }
                            a2[0] = pemitter;
                            v6 = 1;
                        }
                        a2[0]->pRadio = apRadio;
                        a2[0]->bShouldBePlaying = 1;
                        a2[0]->bJustStarted = 1;
                        a2[0]->bIsActor = 0;
                        if (v6) {
                            FalloutRadio::ActiveRadioList.AddHead(*a2);
                        }
                        pstation->CurrentState.StationUsers.AddHead(*a2);
                        pstation->CurrentState.SetShouldBePlaying(1);
                        if (pstation->CurrentState.ShouldStartPaused()) {
                            pstation->CurrentState.iCurrentItemStartTime = (&BSAudio::QInstance())->GetSynchTime() + 50;
                            pstation->CurrentState.SetStartPaused(0);
                        }
                    }
                }
                else {
                    if (a2[0]) {
                        a2[0]->Hndl.Stop();
                        a2[0]->Hndl.Release();
                        a2[0]->StaticHndl.Stop();
                        a2[0]->StaticHndl.Release();
                        a2[0]->bShouldBePlaying = 0;
                        FalloutRadio::ClearInactiveReceivers(0);
                    }
                }
            }
        }
    }
}

bool FalloutRadio::IsPipboyRadioEnabled() {
    return (bool)((int8_t)FalloutRadio::bPipboyRadioEnabled);
}

void FalloutRadio::SetBroadcastState(TESObjectREFR* apTargetStation, bool abEnabled) {
    if (!FalloutRadio::bShutdown) {
        FalloutRadio::FORadioStation* pstation =
            FalloutRadio::GetActiveStation(static_cast<BGSTalkingActivator*>(apTargetStation->GetObjectReference()));
        FalloutRadio::RadioUpdateCriticalSection.Enter("FalloutRadio::SetBroadcastState");
        static_cast<TESObjectACTI*>(apTargetStation->GetObjectReference())
            ->TESObjectACTI::SetContinuousBroadcast(abEnabled);
        if (pstation) {
            if (pstation->CurrentState.IsContBroadcast() && !abEnabled) {
                for (BSSimpleList<FalloutRadio::FORadioReceiver*>* puser = &pstation->CurrentState.StationUsers;
                     puser && !puser->IsEmpty(); puser = puser->m_pkNext) {
                    if (puser->m_item->Hndl.IsValid()) {
                        puser->m_item->Hndl.Stop();
                        puser->m_item->Hndl.Release();
                    }
                }
                if (FalloutRadio::pCurrentPipboyStation == pstation && FalloutRadio::bPipboyRadioEnabled) {
                    FalloutRadio::PipboyVoiceHndl.Stop();
                    FalloutRadio::PipboyVoiceHndl.Release();
                    if (!FalloutRadio::GetIsDialogueOnlyRadio()) {
                        FalloutAudioMedia::MediaClose();
                        FalloutAudioMedia::MediaSetRadio(1);
                        FalloutAudioMedia::MediaOpen(MEDIA_RADIO, 0, 1000u, 0, 0, 0.0f, 0);
                        FalloutAudioMedia::MediaPlay();
                    }
                }
                pstation->CurrentState.SetTransmitting(0);
                Conversation* apThis = pstation->CurrentState.pConvo;
                if (apThis) {
                    delete apThis;
                }
                pstation->CurrentState.pConvo = 0;
            }
            pstation->CurrentState.SetContBroadcast(abEnabled);
        }
        FalloutRadio::RadioUpdateCriticalSection.Leave();
    }
}

void FalloutRadio::NewCellEntered() {
    FalloutRadio::bWasInInterior = (bool)0;
}

void FalloutRadio::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    BSSimpleList<FalloutRadio::FORadioStation*>* plist = &FalloutRadio::ActiveStationList;
    BSSimpleList<FalloutRadio::FORadioStation*>* plastList = 0;
    while (plist && !plist->IsEmpty()) {
        FalloutRadio::FORadioStation* pstation = plist->m_item;
        bool bremove = 0;
        if (pstation) {
            pstation->FinishLoadGame(apLoadGameBuffer);
        }
        else {
            bremove = 1;
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
    FalloutRadio::PipboyRadioEnable(FalloutRadio::bPipboyRadioEnabled);
    if (FalloutRadio::pCurrentPipboyStation) {
        FalloutRadio::PipboyUpdate(1);
    }
}

void FalloutRadio::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {
    FalloutRadio::pCurrentPipboyStation = 0;
    FalloutRadio::pLostPipboyStation = 0;
    FalloutRadio::iLastRadioUpdate = 1;
    FalloutRadio::iAvailableStationCount = 0;
    FalloutRadio::bPipboyRadioEnabled = 0;
    FalloutRadio::bFirstUpdate = 1;
    FalloutRadio::cLastPipboyFilename[0] = 0;
    while (!FalloutRadio::ActiveStationList.IsEmpty()) {
        RadioEntry* m_item = (&FalloutRadio::ActiveStationList)->m_item;
        if (m_item) {
            delete m_item;
        }
        FalloutRadio::ActiveStationList.RemoveHead();
    }
    while (!FalloutRadio::ActiveRadioList.IsEmpty()) {
        FalloutRadio::FORadioReceiver* preceiver = (&FalloutRadio::ActiveRadioList)->m_item;
        if (preceiver->Hndl.IsValid()) {
            preceiver->Hndl.Stop();
            preceiver->Hndl.Release();
        }
        if (preceiver) {
            delete preceiver;
        }
        FalloutRadio::ActiveRadioList.RemoveHead();
    }
    FalloutRadio::DiscoveredStationList.RemoveAll();
    if (FalloutRadio::PipboyVoiceHndl.IsValid()) {
        FalloutRadio::PipboyVoiceHndl.Stop();
        FalloutRadio::PipboyVoiceHndl.Release();
    }
    if (FalloutRadio::PipboyStaticHndl.IsValid()) {
        FalloutRadio::PipboyStaticHndl.Stop();
        FalloutRadio::PipboyStaticHndl.Release();
    }
}

void FalloutRadio::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    for (FalloutRadio::FORadioReceiver* preceiver : FalloutRadio::ActiveRadioList) {
        if (preceiver) {
            preceiver->InitLoadGame(apLoadGameBuffer);
        }
    }
    BSSimpleList<FalloutRadio::FORadioStation*>* plist_2 = &FalloutRadio::ActiveStationList;
    BSSimpleList<FalloutRadio::FORadioStation*>* plastList = 0;
    while (plist_2 && !plist_2->IsEmpty()) {
        FalloutRadio::FORadioStation* pstation = plist_2->m_item;
        bool bremove = 0;
        if (pstation) {
            pstation->InitLoadGame(apLoadGameBuffer);
            if (!pstation->pStation) {
                delete pstation;
                pstation = 0;
                bremove = 1;
            }
        }
        else {
            bremove = 1;
        }
        if (bremove) {
            if (plastList) {
                plastList->Remove(plist_2->m_item);
                plist_2 = plastList->m_pkNext;
            }
            else {
                plist_2->RemoveHead();
            }
        }
        else {
            plastList = plist_2;
            plist_2 = plist_2->m_pkNext;
        }
    }
    BSSimpleList<FalloutRadio::FORadioReceiver*>* plist_3 = &FalloutRadio::ActiveRadioList;
    BSSimpleList<FalloutRadio::FORadioReceiver*>* plastList_2 = 0;
    while (plist_3 && !plist_3->IsEmpty()) {
        FalloutRadio::FORadioReceiver* preceiver_2 = plist_3->m_item;
        bool bremove_2 = 0;
        if (preceiver_2) {
            if (!preceiver_2->pRadio) {
                delete preceiver_2;
                preceiver_2 = 0;
                bremove_2 = 1;
            }
        }
        else {
            bremove_2 = 1;
        }
        if (bremove_2) {
            if (plastList_2) {
                plastList_2->Remove(plist_3->m_item);
                plist_3 = plastList_2->m_pkNext;
            }
            else {
                plist_3->RemoveHead();
            }
        }
        else {
            plastList_2 = plist_3;
            plist_3 = plist_3->m_pkNext;
        }
    }
    // LoadGame left each station's base object (its BGSTalkingActivator) in these slots, which serve as untyped storage
    // until they are resolved to the active station here.
    if (FalloutRadio::pCurrentPipboyStation) {
        FalloutRadio::pCurrentPipboyStation =
            FalloutRadio::GetActiveStation(reinterpret_cast<BGSTalkingActivator*>(FalloutRadio::pCurrentPipboyStation));
    }
    if (FalloutRadio::pLostPipboyStation) {
        FalloutRadio::pLostPipboyStation =
            FalloutRadio::GetActiveStation(reinterpret_cast<BGSTalkingActivator*>(FalloutRadio::pLostPipboyStation));
    }
    if (apLoadGameBuffer->GetVersion() >= 10u) {
        BSSimpleList<TESObjectREFR*>* plist_4 = &FalloutRadio::DiscoveredStationList;
        BSSimpleList<TESObjectREFR*>* plastList_3 = 0;
        while (plist_4 && !plist_4->IsEmpty()) {
            TESObjectREFR* pForm = plist_4->m_item;
            bool bremove_3 = 0;
            if (pForm) {
                // LoadGame left the form ID in the pointer slot, which is used as untyped storage here.
                TESForm* FormByNumericID =
                    TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
                pForm = dynamic_cast<TESObjectREFR*>(FormByNumericID);
                if (pForm) {
                    plist_4->SetItem(pForm);
                }
                else {
                    bremove_3 = 1;
                }
            }
            else {
                bremove_3 = 1;
            }
            if (bremove_3) {
                if (plastList_3) {
                    BSSimpleList<TESObjectREFR*>* v2 = plist_4;
                    plastList_3->Remove(v2->m_item);
                    plist_4 = plastList_3->m_pkNext;
                }
                else {
                    plist_4->RemoveHead();
                }
            }
            else {
                plastList_3 = plist_4;
                plist_4 = plist_4->m_pkNext;
            }
        }
    }
}

// Enabling opens and plays the radio media unless only dialogue radio runs; disabling closes the radio media and
// stops the pip-boy voice and static.
bool FalloutRadio::PipboyRadioEnable(bool abEnable) {
    if (bShutdown) {
        return false;
    }
    if (abEnable) {
        if (!bIsDialogueOnlyRadio) {
            FalloutAudioMedia::MediaClose();
            FalloutAudioMedia::MediaSetRadio(true);
            FalloutAudioMedia::MediaOpen(MEDIA_RADIO, nullptr, 1000, false, false, 0.0f, 0);
            FalloutAudioMedia::MediaPlay();
        }
    }
    else {
        if (FalloutAudioMedia::MediaIsRadio()) {
            FalloutAudioMedia::MediaClose();
            FalloutAudioMedia::MediaSetRadio(false);
        }
        PipboyVoiceHndl.Stop();
        PipboyVoiceHndl.Release();
        pCurrentPipboyStation = nullptr;
        PipboyStaticHndl.Stop();
        PipboyStaticHndl.Release();
    }
    bPipboyRadioEnabled = abEnable;
    return bPipboyRadioEnabled;
}

void FalloutRadio::UpdateStation(FalloutRadio::FORadioStation* pStation) {
    FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, out, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    NiPoint3 listenerLocation;
    bool bsong = 0;
    bool bleadIn = 0;
    bool v61 = 0;
    bool bstaticPlaying = 0;
    int32_t v62 = 0;
    char v58 = 100;
    bool bpipboyUpdated = 0;
    bool bforcePipboyChange = 0;
    if (pStation) {
        if (pStation->pStation) {
            bool bdisabledStation = pStation->pStation->GetDisabled();
            bool bsystemPausing = (FalloutRadio::iRadioState & 1) != 0;
            bool v60 = (FalloutRadio::iRadioState & 2) != 0;
            bool bpipboyStation = pStation == FalloutRadio::pCurrentPipboyStation;
            float fmaxUpdateDistance = 3000.0f;
            BSAudio* v1 = &BSAudio::QInstance();
            v1->GetListenerPosition(&listenerLocation);
            FalloutRadio::RadioUpdateCriticalSection.Enter("FalloutRadio::UpdateStation");
            DialogueItem* pcurrentItem =
                pStation->CurrentState.pConvo ? pStation->CurrentState.pConvo->GetCurrentItem() : 0;
            DialogueItem* pcurrentItem_2 = pcurrentItem;
            BSSimpleList<FalloutRadio::FORadioReceiver*>* pusers = &pStation->CurrentState.StationUsers;
            if (!FalloutRadio::bShutdown) {
                NiPoint3 loc;
                // The station's sound template: the radio attenuation model unless the activator has its own.
                TESSound* pTemplate;
                if (!bpipboyStation && pStation->CurrentState.StationUsers.IsEmpty()) {
                    goto LABEL_10;
                }
                pTemplate = static_cast<TESSound*>(FalloutRadio::pRadioAttenuationModel);
                if (pStation->pStation) {
                    TESObjectACTI* pact = dynamic_cast<TESObjectACTI*>(pStation->pStation->GetObjectReference());
                    if (pact) {
                        if (pact->pRadioTemplate) {
                            pTemplate = pact->pRadioTemplate;
                        }
                    }
                }
                if (bdisabledStation || (bsystemPausing || v60)) {
                    for (BSSimpleList<FalloutRadio::FORadioReceiver*>* puser = &pStation->CurrentState.StationUsers;
                         puser && !puser->IsEmpty(); puser = puser->m_pkNext) {
                        if (puser->m_item->Hndl.IsValid()) {
                            if (bdisabledStation) {
                                puser->m_item->Hndl.Stop();
                                puser->m_item->Hndl.Release();
                                puser->m_item->StaticHndl.Stop();
                                puser->m_item->StaticHndl.Release();
                            }
                            else {
                                if (bsystemPausing) {
                                    puser->m_item->Hndl.Pause();
                                    puser->m_item->StaticHndl.Pause();
                                }
                                else {
                                    if (v60 && puser->m_item->bShouldBePlaying) {
                                        puser->m_item->Hndl.Play(0);
                                        puser->m_item->StaticHndl.Play(0);
                                    }
                                }
                            }
                        }
                    }
                    if (bpipboyStation) {
                        if (bdisabledStation) {
                            FalloutRadio::PipboyRadioEnable(0);
                        }
                        if (FalloutRadio::pCurrentPipboyStation &&
                            FalloutRadio::pCurrentPipboyStation->CurrentState.IsPlayingSong()) {
                            if (bsystemPausing) {
                                FalloutRadio::PipboyStaticHndl.Pause();
                                FalloutAudioMedia::MediaPause();
                            }
                            else {
                                if (v60) {
                                    FalloutRadio::PipboyStaticHndl.Play(0);
                                    FalloutAudioMedia::MediaUnPause();
                                }
                            }
                        }
                        else {
                            if (bsystemPausing) {
                                FalloutRadio::PipboyStaticHndl.Pause();
                                FalloutRadio::PipboyVoiceHndl.Pause();
                            }
                            else {
                                if (v60) {
                                    FalloutRadio::PipboyStaticHndl.Play(0);
                                    FalloutRadio::PipboyVoiceHndl.Play(0);
                                }
                            }
                        }
                    }
                LABEL_10:;
                    FalloutRadio::RadioUpdateCriticalSection.Leave();
                    return;
                }
                if (pStation->CurrentState.iCurrentItemDuration <= 0 &&
                    pStation == FalloutRadio::pCurrentPipboyStation) {
                    if (FalloutRadio::PipboyVoiceHndl.IsValid()) {
                        pStation->CurrentState.iCurrentItemDuration = FalloutRadio::PipboyVoiceHndl.GetDuration();
                    }
                    else {
                        if (pcurrentItem_2) {
                            if (pcurrentItem_2->GetCurrentResponse()->pVoiceSound) {
                                if (FalloutAudioMedia::MediaGetDuration(MEDIA_RADIO)) {
                                    pStation->CurrentState.iCurrentItemDuration =
                                        FalloutAudioMedia::MediaGetDuration(MEDIA_RADIO);
                                }
                            }
                        }
                    }
                }
                uint32_t icurrentTime = (&BSAudio::QInstance())->GetSynchTime();
                if (pStation->CurrentState.iCurrentItemStartTime > icurrentTime + 100) {
                    pStation->CurrentState.iCurrentItemStartTime = icurrentTime;
                }
                if (pStation->CurrentState.iCurrentItemDuration > 0 &&
                    pStation->CurrentState.iCurrentItemDuration + pStation->CurrentState.iCurrentItemStartTime <
                        icurrentTime) {
                    bool bisNext = 0;
                    if (pStation->CurrentState.pConvo) {
                        if (pStation->CurrentState.pConvo->GetCurrentItem()) {
                            if (pStation->CurrentState.pConvo->GetNext()) {
                                bisNext = 1;
                                if (pStation->CurrentState.pConvo->GetCurrentItem()->pInfo) {
                                    if (!pStation->CurrentState.pConvo->GetCurrentItem()->pInfo->IsRunImmediately()) {
                                        pStation->CurrentState.pConvo->GetCurrentItem()->pInfo->RunResult(
                                            TIRS_BEGIN, pStation->pStation
                                        );
                                    }
                                }
                                pStation->CurrentState.pConvo->GetCurrentItem()->FirstResponse();
                                pStation->CurrentState.iCurrentItemStartTime =
                                    (&BSAudio::QInstance())->GetSynchTime() + 50;
                                pStation->CurrentState.iCurrentItemDuration = 0;
                                if (pStation->CurrentState.StationUsers.IsEmpty() &&
                                    !FalloutRadio::pCurrentPipboyStation) {
                                    pStation->CurrentState.SetStartPaused(1);
                                }
                            }
                            else {
                                Conversation* pConvo = pStation->CurrentState.pConvo;
                                if (pConvo) {
                                    delete pConvo;
                                }
                                pStation->CurrentState.pConvo = 0;
                                pcurrentItem_2 = 0;
                                pStation->CurrentState.iCurrentItemStartTime = 0;
                                pStation->CurrentState.iCurrentItemDuration = 0;
                                if (pStation->CurrentState.IsPlayingBreakIn()) {
                                    pStation->CurrentState.SetPlayingBreakIn(0);
                                }
                            }
                            for (pusers = &pStation->CurrentState.StationUsers; pusers && !pusers->IsEmpty();
                                 pusers = pusers->m_pkNext) {
                                pusers->m_item->Hndl.Stop();
                                pusers->m_item->Hndl.Release();
                            }
                            if (pStation == FalloutRadio::pCurrentPipboyStation) {
                                FalloutRadio::PipboyVoiceHndl.Stop();
                                FalloutRadio::PipboyVoiceHndl.Release();
                                if (!FalloutRadio::GetIsDialogueOnlyRadio()) {
                                    FalloutAudioMedia::MediaClose();
                                    FalloutAudioMedia::MediaSetRadio(1);
                                    FalloutAudioMedia::MediaOpen(MEDIA_RADIO, 0, 1000u, 0, 0, 0.0f, 0);
                                    FalloutAudioMedia::MediaPlay();
                                }
                            }
                        }
                    }
                    if (!pStation->CurrentState.pConvo || !pStation->CurrentState.pConvo->GetCurrentItem()) {
                        if (pStation->CurrentState.IsContBroadcast()) {
                            pStation->CurrentState.SetTransmitting(1);
                            FalloutRadio::GetNextConversation(pStation, 0);
                            pStation->CurrentState.pConvo->PrecacheSounds();
                            pStation->CurrentState.pConvo->FirstItem();
                            if (pStation->CurrentState.pConvo->GetCurrentItem()) {
                                pcurrentItem_2 = pStation->CurrentState.pConvo->GetCurrentItem();
                                if (pStation->CurrentState.pConvo->GetCurrentItem()->pInfo) {
                                    if (!pStation->CurrentState.pConvo->GetCurrentItem()->pInfo->IsRunImmediately()) {
                                        pStation->CurrentState.pConvo->GetCurrentItem()->pInfo->RunResult(
                                            TIRS_BEGIN, pStation->pStation
                                        );
                                    }
                                }
                                pStation->CurrentState.pConvo->GetCurrentItem()->FirstResponse();
                                pStation->CurrentState.iCurrentItemStartTime =
                                    (&BSAudio::QInstance())->GetSynchTime() + 50;
                                pStation->CurrentState.iCurrentItemDuration = 0;
                                for (pusers = &pStation->CurrentState.StationUsers; pusers && !pusers->IsEmpty();
                                     pusers = pusers->m_pkNext) {
                                    pusers->m_item->Hndl.Stop();
                                    pusers->m_item->Hndl.Release();
                                }
                                if (pStation == FalloutRadio::pCurrentPipboyStation) {
                                    FalloutRadio::PipboyVoiceHndl.Stop();
                                    FalloutRadio::PipboyVoiceHndl.Release();
                                }
                            }
                        }
                        else {
                            pStation->CurrentState.SetTransmitting(0);
                        }
                    }
                }
                if (pStation->CurrentState.pConvo) {
                    if (pStation->CurrentState.pConvo->GetCurrentItem()) {
                        pcurrentItem_2 = pStation->CurrentState.pConvo->GetCurrentItem();
                        if (!pcurrentItem_2->GetCurrentResponse() && !pcurrentItem_2->NextResponse()) {
                            pStation->CurrentState.iCurrentItemStartTime = (&BSAudio::QInstance())->GetSynchTime() + 50;
                            pStation->CurrentState.iCurrentItemDuration = 1;
                        }
                    }
                }
                pStation->CurrentState.cStationPower = pStation->CurrentState.cTargetPower;
                uint8_t isignalStrength = pStation->CurrentState.GetStationPower();
                if (isignalStrength < 100u) {
                    bstaticPlaying = 1;
                }
                pusers = &pStation->CurrentState.StationUsers;
                while (1) {
                    while (1) {
                        if (!pusers || pusers->IsEmpty()) {
                            if (pStation == FalloutRadio::pCurrentPipboyStation && !bpipboyUpdated) {
                                FalloutRadio::PipboyUpdate(0);
                            }
                            FalloutRadio::RadioUpdateCriticalSection.Leave();
                            return;
                        }
                        if (pusers->m_item->bShouldBePlaying) {
                            break;
                        }
                    LABEL_104:;
                        pusers = pusers->m_pkNext;
                    }
                    if (pStation->CurrentState.iCurrentItemDuration <= 0 && pusers->m_item->Hndl.IsValid()) {
                        if (pusers->m_item->Hndl.GetDuration()) {
                            pStation->CurrentState.iCurrentItemDuration = pusers->m_item->Hndl.GetDuration();
                        }
                    }
                    loc.x = 3.4028235e+38f;
                    loc.y = 3.4028235e+38f;
                    loc.z = 3.4028235e+38f;
                    if (pusers->m_item) {
                        if (pusers->m_item->pRadio) {
                            break;
                        }
                    }
                    if (!pusers->m_item || pusers->m_item->pRadio) {
                        TESWorldSpace* WorldSpace;
                        TESObjectCELL* ParentCell;
                        char Str[268];
                    LABEL_115:;
                        if ((pPlayer->GetWorldSpace() &&
                             (WorldSpace = pPlayer->GetWorldSpace(),
                              WorldSpace != pusers->m_item->pRadio->GetWorldSpace())) ||
                            (pPlayer->GetParentCell()->GetInterior() &&
                             (ParentCell = pusers->m_item->pRadio->GetParentCell(),
                              ParentCell != pPlayer->GetParentCell()))) {
                            pusers->m_item->Hndl.SetVolume(0.0f);
                            pusers->m_item->bShouldBePlaying = 0;
                        }
                        if (pusers->m_item) {
                            if (pusers->m_item->Hndl.IsValid()) {
                                pusers->m_item->Hndl.SetPosition(loc.x, loc.y, loc.z);
                            }
                        }
                        if (!bstaticPlaying || pusers->m_item->StaticHndl.IsValid()) {
                            if (bstaticPlaying) {
                                float v30 = static_cast<double>(100 - isignalStrength) / 100.0;
                                float Ya = std::powf(v30, 0.75f);
                                pusers->m_item->StaticHndl.SetVolume(Ya);
                                pusers->m_item->StaticHndl.SetPosition(loc.x, loc.y, loc.z);
                            }
                            else {
                                if (pusers->m_item->StaticHndl.IsValid()) {
                                    pusers->m_item->StaticHndl.Stop();
                                    pusers->m_item->StaticHndl.Release();
                                }
                            }
                        }
                        else {
                            pusers->m_item->StaticHndl.Stop();
                            pusers->m_item->StaticHndl.Release();
                            BSAudio* v5 = &BSAudio::QInstance();
                            BSSoundHandle* arHandle = v5->GetSoundHandleByName(
                                &a1, "UIRadioStaticLP",
                                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_LOOP |
                                    BSGameSound::SOUND_FLAG_RADIO
                            );
                            pusers->m_item->StaticHndl.operator=(*arHandle);
                            pusers->m_item->StaticHndl.bAssumeSuccess = 1;
                            pusers->m_item->StaticHndl.SetPosition(loc.x, loc.y, loc.z);
                            float v31 = static_cast<double>(100 - isignalStrength) / 100.0;
                            float Y = std::powf(v31, 0.75f);
                            pusers->m_item->StaticHndl.SetVolume(Y);
                            pusers->m_item->StaticHndl.Play(0);
                        }
                        if (!pcurrentItem_2 ||
                            (pusers->m_item->Hndl.IsValid() ||
                             (!pStation->CurrentState.ShouldBePlaying() ||
                              (!pusers->m_item->bShouldBePlaying || !pusers->m_item->pRadio)))) {
                            if (!pusers->m_item->pRadio) {
                                pusers->m_item->bShouldBePlaying = 0;
                            }
                            goto LABEL_104;
                        }
                        if (!pcurrentItem_2 || !pcurrentItem_2->GetCurrentResponse()) {
                            if (pcurrentItem_2) {
                                if (!pcurrentItem_2->NextResponse() && !pcurrentItem_2->FirstResponse()) {
                                    pStation->CurrentState.iCurrentItemStartTime =
                                        (&BSAudio::QInstance())->GetSynchTime() + 50;
                                    pStation->CurrentState.iCurrentItemDuration = 1;
                                }
                            }
                            goto LABEL_104;
                        }
                        if (pcurrentItem_2->GetCurrentResponse()->pVoiceSound) {
                            bsong = 1;
                        }
                        if (pcurrentItem_2->GetCurrentResponse()) {
                            if ((&pcurrentItem_2->GetCurrentResponse()->cText)->pString) {
                                uint32_t ivoiceTextLength =
                                    BSstrlen((&pcurrentItem_2->GetCurrentResponse()->cText)->pString);
                                bool v29 =
                                    ivoiceTextLength > 1 && pcurrentItem_2->GetCurrentResponse()->GetVoice()->pString;
                                v61 = v29;
                            }
                        }
                        if (bsong) {
                            const char* Filename = pcurrentItem_2->GetCurrentResponse()->pVoiceSound->GetFilename();
                            BSstrcpy(Str, 260, Filename);
                            if (!std::strstr(Str, "_mono.")) {
                                char* pnamePt = std::strchr(Str, 46);
                                if (pnamePt) {
                                    char Dst[4];
                                    BSstrcpy(Dst, 4, pnamePt + 1);
                                    *pnamePt = 0;
                                    BSstrcat(Str, 260, "_mono.");
                                    BSstrcat(Str, 260, Dst);
                                }
                            }
                            pStation->CurrentState.SetPlayingSong(1);
                            goto LABEL_154;
                        }
                        if (v61) {
                            BSstrcpy(Str, 260, pcurrentItem_2->GetCurrentResponse()->GetVoice()->pString);
                            pStation->CurrentState.SetPlayingSong(0);
                        LABEL_154:;
                            bool bthisActorIsSpeaking = 0;
                            bool bcreatureRadio = 0;
                            Actor* pact_2 = 0;
                            if (pusers->m_item->pRadio) {
                                TESObjectREFR* pRadio = pusers->m_item->pRadio;
                                if (pRadio->IsActor()) {
                                    pact_2 = static_cast<Actor*>(pusers->m_item->pRadio);
                                    if (pcurrentItem_2->pInfo) {
                                        TESActorBase* ActorBase = pact_2->GetActorBase();
                                        if (ActorBase == pcurrentItem_2->pInfo->GetSpeaker()) {
                                            bthisActorIsSpeaking = 1;
                                        }
                                    }
                                    if (pact_2->IsCreature()) {
                                        bcreatureRadio = 1;
                                    }
                                }
                            }
                            if (!pTemplate || bcreatureRadio) {
                                if (bcreatureRadio) {
                                    fmaxUpdateDistance = fCreatureRadioMax.Float() * 1.100000023841858;
                                }
                            }
                            else {
                                fmaxUpdateDistance = static_cast<double>(pTemplate->GetMaxDistance()) * 110.0;
                            }
                            double v9 = (listenerLocation - loc).Length();
                            if (!(fmaxUpdateDistance < v9)) {
                                uint32_t iseek = (&BSAudio::QInstance())->GetSynchTime() -
                                    pStation->CurrentState.iCurrentItemStartTime;
                                if (pact_2 && bthisActorIsSpeaking) {
                                    TESObjectREFR* v18 = pusers->m_item->pRadio;
                                    TESIdleForm* ListenerIdle = pcurrentItem_2->GetCurrentResponse()->pListenIdle;
                                    TESIdleForm* SpeakerIdle = pcurrentItem_2->GetCurrentResponse()->pSpeakerIdle;
                                    uint32_t CastingType = pcurrentItem_2->GetCurrentResponse()->uiEmotion;
                                    DIALOGUE_EMOTION FormFlags = pcurrentItem_2->GetCurrentResponse()->eEmotion;
                                    BSSoundHandle* p_Hndl = &pusers->m_item->Hndl;
                                    pact_2->SpeakSoundFunction(
                                        Str, *p_Hndl, FormFlags, CastingType, 150, SpeakerIdle, ListenerIdle, v18, 0, 0,
                                        0, 1, 1
                                    );
                                }
                                else {
                                    int32_t v41 = 0;
                                    if (bcreatureRadio) {
                                        uint32_t X = v41 |
                                            (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                             BSGameSound::SOUND_FLAG_RADIO | BSGameSound::SOUND_FLAG_UNKBIT22);
                                        BSAudio* v10 = &BSAudio::QInstance();
                                        BSSoundHandle* SoundHandleByFilename =
                                            v10->GetSoundHandleByFilename(&out, Str, X, 0);
                                        pusers->m_item->Hndl.operator=(*SoundHandleByFilename);
                                        float Yb = fCreatureRadioMax.Float();
                                        float* v11 = &fCreatureRadioMin.Float();
                                        pusers->m_item->Hndl.SetMinMax(*v11, Yb);
                                    }
                                    else {
                                        uint32_t Xa = v41 |
                                            (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                             BSGameSound::SOUND_FLAG_RADIO | BSGameSound::SOUND_FLAG_UNKBIT22);
                                        BSAudio* v12 = &BSAudio::QInstance();
                                        BSSoundHandle* v26 = v12->GetSoundHandleByFilename(&th_s, Str, Xa, pTemplate);
                                        pusers->m_item->Hndl.operator=(*v26);
                                    }
                                    pusers->m_item->Hndl.SetCompletionCallback(
                                        &FalloutRadio::SoundCompletionCallback, pStation
                                    );
                                    pusers->m_item->Hndl.SetSynchStartTime(
                                        pStation->CurrentState.iCurrentItemStartTime
                                    );
                                    pusers->m_item->Hndl.SetPosition(loc.x, loc.y, loc.z);
                                    float v25 = static_cast<double>(isignalStrength) / 100.0;
                                    pusers->m_item->Hndl.SetVolume(v25);
                                    pusers->m_item->Hndl.Play(0);
                                }
                                if (bpipboyStation && !pusers->m_item->bJustStarted && !bpipboyUpdated) {
                                    bpipboyUpdated = 1;
                                    FalloutRadio::PipboyUpdate(1);
                                }
                                if (pusers->m_item->bJustStarted) {
                                    pusers->m_item->bJustStarted = 0;
                                }
                            }
                            goto LABEL_104;
                        }
                        pStation->CurrentState.iCurrentItemDuration = 1;
                        pusers = pusers->m_pkNext;
                        pStation->CurrentState.SetPlayingSong(0);
                    }
                    else {
                        pusers->m_item->bShouldBePlaying = 0;
                        pusers = pusers->m_pkNext;
                    }
                }
                TESObjectREFR* v33 = pusers->m_item->pRadio;
                loc = v33->GetLocationOnReference();
                goto LABEL_115;
            }
        }
    }
}

FalloutRadio::FORadioReceiver* FalloutRadio::GetActiveReceiver(TESObjectREFR* apRequestedReceiver) {
    FalloutRadio::FORadioReceiver* preturnValue = 0;
    BSSimpleList<FalloutRadio::FORadioReceiver*>* pactReceivers = &FalloutRadio::ActiveRadioList;
    bool bfound = 0;
    while (pactReceivers && (!pactReceivers->IsEmpty() && !bfound)) {
        if (apRequestedReceiver == pactReceivers->m_item->pRadio) {
            preturnValue = pactReceivers->m_item;
            bfound = 1;
        }
        pactReceivers = pactReceivers->m_pkNext;
    }
    return preturnValue;
}

void FalloutRadio::RadioSynchPaused() {
    FalloutRadio::iQueuedState &= (~2u);
    FalloutRadio::iQueuedState |= 1u;
    if (FalloutRadio::pCurrentPipboyStation) {
        FalloutRadio::UpdateStation(FalloutRadio::pCurrentPipboyStation);
    }
}

void FalloutRadio::RadioSynchUnPaused() {
    FalloutRadio::iQueuedState &= (~1u);
    FalloutRadio::iQueuedState |= 2u;
}

char FalloutRadio::GetIsDialogueOnlyRadio() {
    return (int8_t)FalloutRadio::bIsDialogueOnlyRadio;
}

void FalloutRadio::DisableNPCRadio(TESObjectREFR* apNPCRadio) {
    FORadioReceiver* v1;
    if (apNPCRadio->IsActor()) {
        v1 = FalloutRadio::GetActiveReceiver(apNPCRadio);
        if (v1) {
            (&v1->Hndl)->Stop();
            (&v1->Hndl)->Release();
            v1->bShouldBePlaying = (bool)0;
            v1->pRadio = nullptr;
        }
    }
}

// The context is the FORadioStation the sound plays for (both SetCompletionCallback callers): a failed sound ends the
// station's current item by setting CurrentState.iCurrentItemDuration to 1.
void FalloutRadio::SoundCompletionCallback(void* apUnused, bool abSucceeded) {
    if (!abSucceeded) {
        if (apUnused) {
            static_cast<FORadioStation*>(apUnused)->CurrentState.iCurrentItemDuration = 1;
        }
    }
}

FalloutRadio::FORadioStation* FalloutRadio::InitStation(TESObjectREFR* pWorldRadio, TESObjectREFR* apTargetStation) {
    FalloutRadio::FORadioStation* presult;
    BGSTalkingActivator* ptunedStation_2;
    if (!pWorldRadio) {
        return 0;
    }
    bool breceiver = 0;
    bool btransmitter = 0;
    TESObjectREFR* pstation = 0;
    FalloutRadio::FORadioStation* apMemory =
        static_cast<FalloutRadio::FORadioStation*>(ourMalloc(sizeof(FalloutRadio::FORadioStation)));
    if (apMemory) {
        presult = ::new (apMemory) FalloutRadio::FORadioStation;
    }
    else {
        presult = 0;
    }
    FalloutRadio::FORadioStation* presult_2 = presult;
    if (pWorldRadio->GetObjectReference()->GetFormType() == ACTI_ID ||
        (pWorldRadio->GetObjectReference()->GetFormType() == NPC__ID ||
         (pWorldRadio->GetObjectReference()->GetFormType() == CREA_ID ||
          (pWorldRadio->GetObjectReference()->GetFormType() == LVLC_ID ||
           pWorldRadio->GetObjectReference()->GetFormType() == LVLN_ID)))) {
        breceiver = 1;
    }
    if (pWorldRadio->GetObjectReference()->GetFormType() == TACT_ID) {
        btransmitter = 1;
    }
    if (breceiver && !pWorldRadio->IsActor()) {
        // Reads the base object's radio station without checking the base object's type, as the game does.
        BGSTalkingActivator* ptunedStation =
            static_cast<TESObjectACTI*>(pWorldRadio->GetObjectReference())->pRadioStation;
        if (!ptunedStation) {
            if (presult_2) {
                delete presult_2;
            }
            return 0;
        }
        pstation = FalloutRadio::GetRadioRefrForStation(ptunedStation, pWorldRadio);
        if (!pstation) {
            if (presult_2) {
                delete presult_2;
            }
            return 0;
        }
        goto LABEL_39;
    }
    if (!breceiver || (!pWorldRadio->IsActor() || !apTargetStation)) {
        if (btransmitter) {
            pstation = pWorldRadio;
        }
        goto LABEL_39;
    }
    ptunedStation_2 = static_cast<BGSTalkingActivator*>(apTargetStation->GetObjectReference());
    if (!ptunedStation_2) {
        if (presult_2) {
            delete presult_2;
        }
        return 0;
    }
    pstation = FalloutRadio::GetRadioRefrForStation(ptunedStation_2, pWorldRadio);
    if (pstation) {
    LABEL_39:;
        if (pstation) {
            presult_2->pStation = pstation;
            presult_2->CurrentState.pConvo = 0;
            presult_2->CurrentState.pNextConvo = 0;
            uint32_t SynchTime = (&BSAudio::QInstance())->GetSynchTime();
            presult_2->CurrentState.iCurrentItemStartTime = SynchTime - RandomInt(0, 0x7530);
            presult_2->CurrentState.iCurrentItemDuration = 1;
            TESBoundObject* ObjectReference = presult_2->pStation->GetObjectReference();
            bool v4 = ObjectReference->GetContinuousBroadcast();
            presult_2->CurrentState.SetContBroadcast(v4);
            bool IsContBroadcast = presult_2->CurrentState.IsContBroadcast();
            presult_2->CurrentState.SetTransmitting(IsContBroadcast);
            presult_2->CurrentState.SetJustTurnedOn(1);
            presult_2->CurrentState.cStationPower = 100;
            presult_2->CurrentState.cTargetPower = 100;
            FalloutRadio::ActiveStationList.AddHead(presult_2);
            return presult_2;
        }
        else {
            if (presult_2) {
                delete presult_2;
            }
            return 0;
        }
    }
    if (presult_2) {
        delete presult_2;
    }
    return 0;
}

TESObjectREFR*
FalloutRadio::GetRadioRefrForStation(BGSTalkingActivator* apRequestedStation, TESObjectREFR* pWorldRadio) {
    TESObjectREFR* presult = 0;
    BSSimpleList<TESObjectREFR*> stationlist;
    if (pWorldRadio) {
        BGSTalkingActivator::GetRadioStationsInRangeOfRef(pWorldRadio, &stationlist, 0);
    }
    else {
        BGSTalkingActivator::GetRadioStationsInRangeOfRef(pPlayer, &stationlist, 0);
    }
    BSSimpleList<TESObjectREFR*>* stations = &stationlist;
    bool bdone = 0;
    while (!bdone && (stations && (stations->m_item && stations->m_item->GetObjectReference()))) {
        if (apRequestedStation == stations->m_item->GetObjectReference()) {
            presult = stations->m_item;
            bdone = 1;
        }
        stations = stations->m_pkNext;
    }
    return presult;
}

void FalloutRadio::Shutdown() {
    FalloutRadio::bShutdown = 1;
    FalloutRadio::ClearInactiveReceivers(1);
    for (BSSimpleList<FalloutRadio::FORadioStation*>* pactStations = &FalloutRadio::ActiveStationList;
         pactStations && !pactStations->IsEmpty(); pactStations = pactStations->m_pkNext) {
        if (pactStations->m_item->CurrentState.pConvo) {
            Conversation* apThis = pactStations->m_item->CurrentState.pConvo;
            if (apThis) {
                delete apThis;
            }
            pactStations->m_item->CurrentState.pConvo = 0;
        }
        RadioEntry* m_item = pactStations->m_item;
        if (m_item) {
            delete m_item;
        }
    }
    FalloutRadio::ActiveStationList.RemoveAll();
    for (BSSimpleList<FalloutRadio::FORadioReceiver*>* pactReceivers = &FalloutRadio::ActiveRadioList;
         pactReceivers && !pactReceivers->IsEmpty(); pactReceivers = pactReceivers->m_pkNext) {
        FalloutRadio::FORadioReceiver* v0 = pactReceivers->m_item;
        if (v0) {
            delete v0;
        }
    }
    FalloutRadio::ActiveRadioList.RemoveAll();
    FalloutRadio::DiscoveredStationList.RemoveAll();
}

bool FalloutRadio::TunePipboyToStation(TESObjectREFR* apTargetStation, bool abForceStationUpdate) {
    if (!FalloutRadio::bPipboyRadioEnabled || FalloutRadio::bShutdown) {
        return 0;
    }
    ourMemset(FalloutRadio::cLastPipboyFilename, 0, 260u);
    int32_t iseekTime = 0;
    if (apTargetStation) {
        if (abForceStationUpdate) {
            FalloutRadio::pLostPipboyStation = 0;
            FalloutRadio::UpdateStations(1);
        }
    }
    else {
        BSSimpleList<TESObjectREFR*> stationlist;
        BGSTalkingActivator::GetRadioStationsInRangeOfRef(pPlayer, &stationlist, 0);
        BSSimpleList<TESObjectREFR*>* stations = &stationlist;
        if (&stationlist) {
            if (stations->m_item) {
                if (stations->m_item->GetObjectReference()) {
                    apTargetStation = stations->m_item;
                }
            }
        }
    }
    FalloutRadio::pCurrentPipboyStation =
        FalloutRadio::GetActiveStation(static_cast<BGSTalkingActivator*>(apTargetStation->GetObjectReference()));
    if (FalloutRadio::pCurrentPipboyStation) {
        TESBoundObject* ObjectReference = FalloutRadio::pCurrentPipboyStation->pStation->GetObjectReference();
        if (ObjectReference == apTargetStation->GetObjectReference()) {
            FalloutRadio::pCurrentPipboyStation->pStation = apTargetStation;
        }
    }
    else {
        FalloutRadio::pCurrentPipboyStation = FalloutRadio::InitStation(apTargetStation, 0);
        if (!FalloutRadio::pCurrentPipboyStation) {
            FalloutRadio::PipboyRadioEnable(0);
        }
        if (!abForceStationUpdate) {
            FalloutRadio::UpdateStations(1);
        }
    }
    if (FalloutRadio::pCurrentPipboyStation->CurrentState.ShouldStartPaused()) {
        uint32_t v4 = (&BSAudio::QInstance())->GetSynchTime() + 50;
        FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemStartTime = v4;
        FalloutRadio::pCurrentPipboyStation->CurrentState.SetStartPaused(0);
    }
    FalloutRadio::pCurrentPipboyStation->CurrentState.SetShouldBePlaying(1);
    FalloutRadio::pLostPipboyStation = 0;
    return 1;
}

void FalloutRadio::PipboyUpdate(bool abForceUpdate) {
    FNVD_INLINED_LOCAL(BSSoundHandle, subtitleHandle, std::nullopt, std::nullopt);
    double v33;
    FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, a4, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, out, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, alertSound, std::nullopt, std::nullopt);
    if (FalloutRadio::pCurrentPipboyStation) {
        int32_t StationPower = FalloutRadio::pCurrentPipboyStation->CurrentState.GetStationPower();
        v33 = static_cast<double>(StationPower) / 100.0;
    }
    else {
        v33 = 0.0;
    }
    float fsignalVolumeModifier = v33;
    bool bvolumeAlreadySet = 0;
    if (FalloutRadio::pCurrentPipboyStation && FalloutRadio::pCurrentPipboyStation->CurrentState.OutOfPBRange()) {
        BSStringT<char> strtemp;
        BSAudio* v1 = &BSAudio::QInstance();
        v1->GetSoundHandleByName(
            &alertSound, "UIRadioSignalLost",
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT |
                BSGameSound::SOUND_FLAG_RADIO
        );
        alertSound.Play(0);
        const char* pstationName = FalloutRadio::pCurrentPipboyStation->pStation->GetFullName();
        strtemp.SPrintF(sRadioSignalLost.String(), pstationName);
        FalloutRadio::pLostPipboyStation = FalloutRadio::pCurrentPipboyStation;
        FalloutRadio::PipboyRadioEnable(0);
        FalloutRadio::PipboyStaticHndl.Stop();
        FalloutRadio::PipboyStaticHndl.Release();
        HUDMainMenu::pme->ShowMessage(
            strtemp.String(), HUDMainMenu::HME_NEUTRAL, "Interface\\Icons\\Message Icons\\glow_message_radio_tower.dds",
            0, 2.0f, 0
        );
        return;
    }
    if (FalloutRadio::pCurrentPipboyStation &&
        FalloutRadio::pCurrentPipboyStation->CurrentState.GetStationPower() < 100u) {
        if (!FalloutRadio::PipboyStaticHndl.IsValid()) {
            FalloutRadio::PipboyStaticHndl.Stop();
            FalloutRadio::PipboyStaticHndl.Release();
            BSAudio* v2 = &BSAudio::QInstance();
            BSSoundHandle* SoundHandleByName = v2->GetSoundHandleByName(
                &arHandle, "UIRadioStaticLP",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                    BSGameSound::SOUND_FLAG_RADIO
            );
            FalloutRadio::PipboyStaticHndl.operator=(*SoundHandleByName);
            FalloutRadio::PipboyStaticHndl.Play(0);
        }
        int32_t v30 = 100 - FalloutRadio::pCurrentPipboyStation->CurrentState.GetStationPower();
        float v29 = static_cast<double>(v30) / 100.0;
        float fstaticVolume = std::powf(v29, 0.75f);
        FalloutRadio::PipboyStaticHndl.SetVolume(fstaticVolume);
    }
    else {
        if (FalloutRadio::pCurrentPipboyStation) {
            if (FalloutRadio::pCurrentPipboyStation->CurrentState.GetStationPower() == 100 &&
                FalloutRadio::PipboyStaticHndl.IsValid()) {
                FalloutRadio::PipboyStaticHndl.Stop();
                FalloutRadio::PipboyStaticHndl.Release();
            }
        }
    }
    if (FalloutRadio::pCurrentPipboyStation) {
        if (FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemDuration <= 0 &&
            FalloutRadio::PipboyVoiceHndl.IsValid()) {
            if (FalloutRadio::PipboyVoiceHndl.GetDuration()) {
                FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemDuration =
                    FalloutRadio::PipboyVoiceHndl.GetDuration();
            }
        }
    }
    if (FalloutRadio::pCurrentPipboyStation &&
        (FalloutRadio::pCurrentPipboyStation->CurrentState.pConvo &&
         ((!FalloutAudioMedia::MediaIsPlaying(MEDIA_RADIO, 1) && !FalloutRadio::PipboyVoiceHndl.IsValid()) ||
          abForceUpdate))) {
        DialogueItem* pcurrentItem = FalloutRadio::pCurrentPipboyStation->CurrentState.pConvo->GetCurrentItem();
        if (!pcurrentItem) {
            return;
        }
        bool bsong = 0;
        char v46 = 0;
        if (!pcurrentItem->GetCurrentResponse()) {
            pcurrentItem->FirstResponse();
        }
        if (pcurrentItem->GetCurrentResponse() &&
            (pcurrentItem->GetCurrentResponse()->pVoiceSound && !FalloutRadio::GetIsDialogueOnlyRadio())) {
            bsong = 1;
            if (FalloutRadio::pCurrentPipboyStation) {
                FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingSong(1);
            }
        }
        else {
            if (FalloutRadio::pCurrentPipboyStation) {
                FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingSong(0);
            }
        }
        if (pcurrentItem->GetCurrentResponse()) {
            if ((&pcurrentItem->GetCurrentResponse()->cText)->pString) {
                uint32_t ivoiceTextLength = BSstrlen((&pcurrentItem->GetCurrentResponse()->cText)->pString);
                int32_t v28 = ivoiceTextLength > 2 &&
                    pcurrentItem->GetCurrentResponse()->GetVoice()->operator!=(static_cast<const char*>(0));
                v46 = v28;
                if (!v28) {
                    if (pcurrentItem->GetCurrentResponse()->pVoiceSound) {
                        const char* Filename = pcurrentItem->GetCurrentResponse()->pVoiceSound->GetFilename();
                        if (std::strstr(Filename, ".wav")) {
                            v46 = 1;
                        }
                    }
                }
            }
        }
        uint32_t v45 = (&BSAudio::QInstance())->GetSynchTime() -
            FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemStartTime;
        if (v46) {
            bool v42 = pcurrentItem->GetCurrentResponse()->pVoiceSound != 0;
            FalloutRadio::PipboyVoiceHndl.Stop();
            FalloutRadio::PipboyVoiceHndl.Release();
            if (v42) {
                // Passes the voice sound as the settings context.
                TESSound* pvoiceSound = pcurrentItem->GetCurrentResponse()->pVoiceSound;
                const char* pfilename = pcurrentItem->GetCurrentResponse()->pVoiceSound->GetFilename();
                BSAudio* v4 = &BSAudio::QInstance();
                BSSoundHandle* SoundHandleByFilename = v4->GetSoundHandleByFilename(
                    &out, pfilename,
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_RADIO,
                    pvoiceSound
                );
                FalloutRadio::PipboyVoiceHndl.operator=(*SoundHandleByFilename);
            }
            else {
                const char* pvoiceFilename = pcurrentItem->GetCurrentResponse()->GetVoice()->String();
                BSAudio* v5 = &BSAudio::QInstance();
                BSSoundHandle* v25 = v5->GetSoundHandleByFilename(
                    &a4, pvoiceFilename,
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_RADIO, 0
                );
                FalloutRadio::PipboyVoiceHndl.operator=(*v25);
            }
            FalloutRadio::PipboyVoiceHndl.SetCompletionCallback(
                &FalloutRadio::SoundCompletionCallback, FalloutRadio::pCurrentPipboyStation
            );
            FalloutRadio::PipboyVoiceHndl.SetSynchStartTime(
                FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemStartTime
            );
            if (FalloutAudio::IsRadioMute()) {
                float v23 = fsignalVolumeModifier * 0.10000000149011612;
                FalloutRadio::PipboyVoiceHndl.SetVolume(v23);
            }
            else {
                BSWin32Audio* v6 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
                double RadioVolume = v6->GetRadioVolume();
                float v22 = RadioVolume * fsignalVolumeModifier;
                FalloutRadio::PipboyVoiceHndl.SetVolume(v22);
            }
            bvolumeAlreadySet = 1;
            FalloutRadio::PipboyVoiceHndl.Play(0);
            if (!FalloutRadio::PipboyVoiceHndl.IsValid()) {
                NiPoint3 speakerLoc = pPlayer->GetLocationOnReference();
                subtitleHandle.operator=(FalloutRadio::PipboyVoiceHndl);
                char* v11 = (&pcurrentItem->GetCurrentResponse()->cText)->pString;
                Interface::ShowSubtitle(v11, subtitleHandle, speakerLoc, pPlayer, false);
            }
        }
        if (!bsong || FalloutRadio::GetIsDialogueOnlyRadio()) {
            FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingSong(0);
        }
        else {
            char Src[268];
            const char* psongFilename = pcurrentItem->GetCurrentResponse()->pVoiceSound->GetFilename();
            BSsprintf(Src, 260, "%s%s", "data\\sound\\", psongFilename);
            BSStringT<char> lipString(static_cast<const char*>(Src));
            FalloutAudioMedia::MediaOpen(
                MEDIA_RADIO, Src, 500u, 0, 1, 0.0f,
                FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemStartTime
            );
            BSstrcpy(FalloutRadio::cLastPipboyFilename, 260, Src);
            FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingSong(1);
            if (FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemDuration <= 0) {
                if (FalloutAudioMedia::MediaGetDuration(MEDIA_RADIO)) {
                    FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemDuration =
                        FalloutAudioMedia::MediaGetDuration(MEDIA_RADIO);
                }
            }
        }
        if (!bsong && !v46) {
            FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemDuration = 1;
        }
    }
    else {
        if (!FalloutRadio::pCurrentPipboyStation->CurrentState.IsContBroadcast() &&
            !FalloutRadio::pCurrentPipboyStation->CurrentState.IsPlayingBreakIn() &&
            !FalloutRadio::pCurrentPipboyStation->CurrentState.IsTransmitting() &&
            !FalloutRadio::PipboyVoiceHndl.IsPlaying()) {
            FalloutRadio::PipboyVoiceHndl.Stop();
            FalloutRadio::PipboyVoiceHndl.Release();
            BSAudio* v8 = &BSAudio::QInstance();
            BSSoundHandle* v19 = v8->GetSoundHandleByName(
                &th_s, "UIRadioStaticLP",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                    BSGameSound::SOUND_FLAG_RADIO
            );
            FalloutRadio::PipboyVoiceHndl.operator=(*v19);
            FalloutRadio::PipboyVoiceHndl.bAssumeSuccess = 1;
            FalloutRadio::PipboyVoiceHndl.Play(0);
            FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingStatic(1);
            FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingSong(0);
        }
    }
    if (!bvolumeAlreadySet) {
        if (FalloutAudio::IsRadioMute()) {
            float v17 = fsignalVolumeModifier * 0.10000000149011612;
            FalloutRadio::PipboyVoiceHndl.SetVolume(v17);
        }
        else {
            BSWin32Audio* v9 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            double v10 = v9->GetRadioVolume();
            float v16 = v10 * fsignalVolumeModifier;
            FalloutRadio::PipboyVoiceHndl.SetVolume(v16);
        }
    }
}

void FalloutRadio::EnableNPCRadio(TESObjectREFR* apNPCRadio, TESObjectREFR* apTargetStation) {
    int32_t prequestedStation = 0;
    FalloutRadio::FORadioReceiver* pemitter = FalloutRadio::GetActiveReceiver(apNPCRadio);
    uint32_t iseekTime = 0;
    bool baddToList = 0;
    if (!FalloutRadio::bShutdown) {
        FalloutRadio::FORadioStation* pstation =
            FalloutRadio::GetActiveStation(static_cast<BGSTalkingActivator*>(apTargetStation->GetObjectReference()));
        if (pstation) {
            iseekTime = (&BSAudio::QInstance())->GetSynchTime() - pstation->CurrentState.iCurrentItemStartTime;
        LABEL_8:;
            if (!pemitter) {
                FalloutRadio::FORadioReceiver* pemitter_2;
                FalloutRadio::FORadioReceiver* apMemory =
                    static_cast<FalloutRadio::FORadioReceiver*>(ourMalloc(sizeof(FalloutRadio::FORadioReceiver)));
                if (apMemory) {
                    pemitter_2 = ::new (apMemory) FalloutRadio::FORadioReceiver;
                }
                else {
                    pemitter_2 = 0;
                }
                pemitter = pemitter_2;
                baddToList = 1;
            }
            pemitter->pRadio = apNPCRadio;
            pemitter->bShouldBePlaying = 1;
            pemitter->bJustStarted = 1;
            pemitter->bIsActor = 1;
            if (baddToList) {
                FalloutRadio::ActiveRadioList.AddHead(pemitter);
            }
            pstation->CurrentState.StationUsers.AddHead(pemitter);
            pstation->CurrentState.SetShouldBePlaying(1);
            return;
        }
        pstation = FalloutRadio::InitStation(apNPCRadio, apTargetStation);
        if (pstation) {
            if (!pstation->pStation) {
                platform::DebuggerBreak();
            }
        }
        if (pstation) {
            goto LABEL_8;
        }
    }
}

void FalloutRadio::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&FalloutRadio::iAvailableStationCount, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&FalloutRadio::bPipboyRadioEnabled), 1u, 0);
    apSaveGameBuffer->SaveString(FalloutRadio::cLastPipboyFilename, 0);
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (FalloutRadio::FORadioReceiver* preceiver : FalloutRadio::ActiveRadioList) {
        if (preceiver) {
            preceiver->SaveGame(apSaveGameBuffer);
            ++isaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    uint32_t isaveGameCount_2 = 0;
    uint32_t isaveGameBufferPos_2 = apSaveGameBuffer->StartVariableSizedValue();
    for (FalloutRadio::FORadioStation* pstation : FalloutRadio::ActiveStationList) {
        if (pstation) {
            pstation->SaveGame(apSaveGameBuffer);
            ++isaveGameCount_2;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_2, isaveGameBufferPos_2);
    TESBoundObject* pcurrentPipboyStationObject = 0;
    if (FalloutRadio::pCurrentPipboyStation) {
        pcurrentPipboyStationObject = FalloutRadio::pCurrentPipboyStation->pStation->GetObjectReference();
    }
    apSaveGameBuffer->SaveFormID(pcurrentPipboyStationObject, 0);
    TESBoundObject* plostPipboyStationObject = 0;
    if (FalloutRadio::pLostPipboyStation) {
        plostPipboyStationObject = FalloutRadio::pLostPipboyStation->pStation->GetObjectReference();
    }
    apSaveGameBuffer->SaveFormID(plostPipboyStationObject, 0);
    uint32_t isaveGameCount_3 = 0;
    uint32_t isaveGameBufferPos_3 = apSaveGameBuffer->StartVariableSizedValue();
    for (TESObjectREFR* pForm : FalloutRadio::DiscoveredStationList) {
        if (pForm) {
            apSaveGameBuffer->SaveFormID(pForm, 0);
            ++isaveGameCount_3;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_3, isaveGameBufferPos_3);
}

void FalloutRadio::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESObjectREFR* pForm;
    apLoadGameBuffer->LoadData(&FalloutRadio::iAvailableStationCount, 4u);
    apLoadGameBuffer->LoadData(&FalloutRadio::bPipboyRadioEnabled, 1u);
    apLoadGameBuffer->LoadString(FalloutRadio::cLastPipboyFilename);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        FalloutRadio::FORadioReceiver* preceiver;
        FalloutRadio::FORadioReceiver* apMemory =
            static_cast<FalloutRadio::FORadioReceiver*>(ourMalloc(sizeof(FalloutRadio::FORadioReceiver)));
        if (apMemory) {
            preceiver = ::new (apMemory) FalloutRadio::FORadioReceiver;
        }
        else {
            preceiver = 0;
        }
        FalloutRadio::FORadioReceiver* preceiver_2 = preceiver;
        preceiver->LoadGame(apLoadGameBuffer);
        FalloutRadio::ActiveRadioList.AddHead(preceiver_2);
    }
    uint32_t isaveGameCount_2 = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop_2 = 0; iloop_2 < isaveGameCount_2; ++iloop_2) {
        FalloutRadio::FORadioStation* pstation;
        void* v7 = ourMalloc(sizeof(FalloutRadio::FORadioStation));
        if (v7) {
            pstation = ::new (v7) FalloutRadio::FORadioStation;
        }
        else {
            pstation = 0;
        }
        FalloutRadio::FORadioStation* pstation_2 = pstation;
        pstation->LoadGame(apLoadGameBuffer);
        FalloutRadio::ActiveStationList.AddHead(pstation_2);
    }
    // Each station's TESBoundObject is kept in the FORadioStation* slot until InitLoadGame resolves it.
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    FalloutRadio::pCurrentPipboyStation = static_cast<FalloutRadio::FORadioStation*>(
        FNVD_DYNAMIC_CAST_VOID(TESBoundObject)(FormByNumericID, RTTI_TESForm, RTTI_TESBoundObject)
    );
    uint32_t v3 = apLoadGameBuffer->LoadFormID();
    TESForm* v4 = TESForm::GetFormByNumericID(v3);
    FalloutRadio::pLostPipboyStation = static_cast<FalloutRadio::FORadioStation*>(
        FNVD_DYNAMIC_CAST_VOID(TESBoundObject)(v4, RTTI_TESForm, RTTI_TESBoundObject)
    );
    if (apLoadGameBuffer->GetVersion() >= 10u) {
        uint32_t isaveGameCount_3 = apLoadGameBuffer->LoadVariableSizedValue();
        for (uint32_t iloop_3 = 0; iloop_3 < isaveGameCount_3; ++iloop_3) {
            pForm = 0;
            // The form ID is kept in the pointer slot, untyped storage until InitLoadGame resolves it.
            apLoadGameBuffer->LoadFormID(pForm);
            FalloutRadio::DiscoveredStationList.AddHead(pForm);
        }
    }
}

bool FalloutRadio::GetNextConversation(FalloutRadio::FORadioStation* apStation, TESTopic* apOverrideTopic) {
    TESTopic* Topic;
    if (apOverrideTopic) {
        Topic = apOverrideTopic;
    }
    else {
        Topic = TESTopic::GetTopic(DT_RADIO, 0);
    }
    if (apStation->CurrentState.pNextConvo) {
        RadioConvTask* v4;
        if (apStation->CurrentState.pConvo) {
            Conversation* apThis = apStation->CurrentState.pConvo;
            if (apThis) {
                delete apThis;
            }
            apStation->CurrentState.pConvo = 0;
        }
        apStation->CurrentState.pConvo = apStation->CurrentState.pNextConvo;
        Conversation* pNextConvo = apStation->CurrentState.pNextConvo;
        if (pNextConvo) {
            delete pNextConvo;
        }
        apStation->CurrentState.pNextConvo = 0;
        RadioConvTask* apMemory = static_cast<RadioConvTask*>(ourMalloc(sizeof(RadioConvTask)));
        if (apMemory) {
            v4 = ::new (apMemory) RadioConvTask(apStation);
        }
        else {
            v4 = 0;
        }
        pIOManager->AddTask(v4);
        return 1;
    }
    else {
        Conversation* v3;
        if (apStation->CurrentState.pConvo) {
            Conversation* pConvo = apStation->CurrentState.pConvo;
            if (pConvo) {
                delete pConvo;
            }
            apStation->CurrentState.pConvo = 0;
        }
        Conversation* v6 = static_cast<Conversation*>(ourMalloc(sizeof(Conversation)));
        if (v6) {
            v3 = ::new (v6) Conversation(apStation->pStation, apStation->pStation, Topic);
        }
        else {
            v3 = 0;
        }
        apStation->CurrentState.pConvo = v3;
        return 0;
    }
}

RadioConvTask::RadioConvTask(FalloutRadio::FORadioStation* apStation)
  : QueuedFileEntry(IO_TASK_PRIORITY_CRITICAL), pStation(apStation) {}

RadioConvTask::~RadioConvTask() {}

void RadioConvTask::Run() {
    TESTopic* topic;
    Conversation* v2;
    void* v4;
    if (pStation) {
        if (!pStation->CurrentState.pNextConvo) {
            v4 = ourMalloc(sizeof(Conversation));
            if (v4) {
                topic = TESTopic::GetTopic(DT_RADIO, 0);
                v2 = ::new (v4) Conversation(pStation->pStation, pStation->pStation, topic);
            }
            else {
                v2 = nullptr;
            }
            pStation->CurrentState.pNextConvo = v2;
        }
    }
}

bool RadioConvTask::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    BSsprintf(apDescription, aiBufferSize, "Radio conversation task");
    return true;
}

uint8_t RadioConvTask::GetFileIndex() {
    return IO_FILE_INDEX_MISC;
}

void RadioConvTask::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void FalloutAudioMedia::MediaSetRadio(bool val) {
    bRadioOn = val;
}
