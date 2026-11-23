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
#include <limits>

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
    TESObjectREFR* pRef = nullptr;
    if (pRadio) {
        TESForm* pForm = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pRadio));
        pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pRadio = pRef;
}

void FalloutRadio::FORadioReceiver::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveFormID(pRadio, 0);
    apSaveGameBuffer
        ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bShouldBePlaying)), sizeof(bShouldBePlaying), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bJustStarted)), sizeof(bJustStarted), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bInRange)), sizeof(bInRange), 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bIsActor)), sizeof(bIsActor), 0);
}

void FalloutRadio::FORadioReceiver::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    // Loads the saved form ID into the pointer field, to be resolved after the load.
    apLoadGameBuffer->LoadFormID(pRadio);
    apLoadGameBuffer->LoadData(&bShouldBePlaying, sizeof(bShouldBePlaying));
    apLoadGameBuffer->LoadData(&bJustStarted, sizeof(bJustStarted));
    apLoadGameBuffer->LoadData(&bInRange, sizeof(bInRange));
    apLoadGameBuffer->LoadData(&bIsActor, sizeof(bIsActor));
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
    return (iStationFlags & STATION_START_PAUSED) != 0;
}

bool FalloutRadio::StationState::IsContBroadcast() {
    return (iStationFlags & STATION_CONT_BROADCAST) != 0;
}

void FalloutRadio::StationState::SetJustTurnedOn(bool abVal) {
    if (abVal) {
        iStationFlags |= STATION_JUST_TURNED_ON;
    }
    else {
        iStationFlags &= ~STATION_JUST_TURNED_ON;
    }
}

void FalloutRadio::StationState::SetTransmitting(bool abVal) {
    if (abVal) {
        iStationFlags |= STATION_TRANSMITTING;
    }
    else {
        iStationFlags &= ~STATION_TRANSMITTING;
    }
}

bool FalloutRadio::StationState::OutOfPBRange() {
    return (iStationFlags & STATION_OUT_OF_PB_RANGE) != 0;
}

bool FalloutRadio::StationState::IsPlayingBreakIn() {
    return (iStationFlags & STATION_PLAYING_BREAK_IN) != 0;
}

uint8_t FalloutRadio::StationState::GetStationPower() {
    return cStationPower;
}

void FalloutRadio::StationState::SetPlayingSong(bool abVal) {
    if (abVal) {
        iStationFlags |= STATION_PLAYING_SONG;
    }
    else {
        iStationFlags &= ~STATION_PLAYING_SONG;
    }
}

void FalloutRadio::StationState::SetPlayingStatic(bool abVal) {
    if (abVal) {
        iStationFlags |= STATION_PLAYING_STATIC;
    }
    else {
        iStationFlags &= ~STATION_PLAYING_STATIC;
    }
}

void FalloutRadio::StationState::SetOutOfPBRange(bool abVal) {
    if (abVal) {
        iStationFlags |= STATION_OUT_OF_PB_RANGE;
    }
    else {
        iStationFlags &= ~STATION_OUT_OF_PB_RANGE;
    }
}

void FalloutRadio::StationState::SetShouldBePlaying(bool abVal) {
    if (abVal) {
        iStationFlags |= STATION_SHOULD_BE_PLAYING;
    }
    else {
        iStationFlags &= ~STATION_SHOULD_BE_PLAYING;
    }
}

void FalloutRadio::StationState::SetContBroadcast(bool abVal) {
    if (abVal) {
        iStationFlags |= STATION_CONT_BROADCAST;
    }
    else {
        iStationFlags &= ~STATION_CONT_BROADCAST;
    }
}

bool FalloutRadio::StationState::ShouldBePlaying() {
    return (iStationFlags & STATION_SHOULD_BE_PLAYING) != 0;
}

bool FalloutRadio::StationState::IsPlayingSong() {
    return (iStationFlags & STATION_PLAYING_SONG) != 0;
}

void FalloutRadio::StationState::SetPlayingBreakIn(bool abVal) {
    if (abVal) {
        iStationFlags |= STATION_PLAYING_BREAK_IN;
    }
    else {
        iStationFlags &= ~STATION_PLAYING_BREAK_IN;
    }
}

bool FalloutRadio::StationState::IsTransmitting() {
    return (iStationFlags & STATION_TRANSMITTING) != 0;
}

void FalloutRadio::StationState::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCurrentItemStartTime), sizeof(iCurrentItemStartTime));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iCurrentItemDuration), sizeof(iCurrentItemDuration));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cStationPower), sizeof(cStationPower));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cTargetPower), sizeof(cTargetPower));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iStationFlags), sizeof(iStationFlags));
    uint32_t uiUserCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiUserCount; ++i) {
        TESObjectREFR* pRadio = nullptr;
        apLoadGameBuffer->LoadFormID(pRadio);
        FalloutRadio::FORadioReceiver* pReceiver = FalloutRadio::GetActiveReceiver(pRadio);
        if (pReceiver) {
            StationUsers.AddHead(pReceiver);
        }
    }
    char cHasConvo = 0;
    apLoadGameBuffer->LoadData(&cHasConvo, sizeof(cHasConvo));
    if (cHasConvo) {
        Conversation* pNewConvo;
        pNewConvo = new Conversation;
        pConvo = pNewConvo;
        pConvo->LoadGameBGS(static_cast<BGSLoadFormBuffer*>(apLoadGameBuffer));
    }
}

void FalloutRadio::StationState::SetStartPaused(bool abVal) {
    if (abVal) {
        iStationFlags |= STATION_START_PAUSED;
    }
    else {
        iStationFlags &= ~STATION_START_PAUSED;
    }
}

void FalloutRadio::StationState::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    int32_t imodifiedStartTime = BSAudio::QInstance().GetSynchTime() - iCurrentItemStartTime;
    if (iCurrentItemDuration <= 0) {
        imodifiedStartTime = 0;
    }
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&imodifiedStartTime), sizeof(imodifiedStartTime), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iCurrentItemDuration), sizeof(iCurrentItemDuration), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cStationPower), sizeof(cStationPower), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cTargetPower), sizeof(cTargetPower), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iStationFlags), sizeof(iStationFlags), 0);
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (FalloutRadio::FORadioReceiver* preceiver : StationUsers) {
        if (preceiver) {
            apSaveGameBuffer->SaveFormID(preceiver->pRadio, 0u);
            ++isaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    bool bhasConvo = pConvo != nullptr;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bhasConvo), sizeof(bhasConvo), 0);
    if (bhasConvo) {
        pConvo->SaveGameBGS(static_cast<BGSSaveFormBuffer*>(apSaveGameBuffer));
    }
}

void FalloutRadio::StationState::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    if (pConvo) {
        pConvo->InitItem(apLoadGameBuffer);
    }
    BSSimpleList<FalloutRadio::FORadioReceiver*>* plist = &StationUsers;
    BSSimpleList<FalloutRadio::FORadioReceiver*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        bool bremove;
        FalloutRadio::FORadioReceiver* preceiver = plist->m_item;
        if (preceiver) {
            bremove = preceiver->pRadio == nullptr;
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
}

void FalloutRadio::StationState::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    if (BSAudio::QInstance().GetSynchTime() >= iCurrentItemStartTime && iCurrentItemDuration > 0) {
        iCurrentItemStartTime = BSAudio::QInstance().GetSynchTime() - iCurrentItemStartTime;
    }
    else {
        iCurrentItemStartTime = 0;
        iCurrentItemDuration = 1;
        if (pConvo) {
            delete pConvo;
        }
        pConvo = nullptr;
    }
    for (BSSimpleList<FalloutRadio::FORadioReceiver*>* users = &StationUsers;
         users && !users->IsEmpty() && users->m_item;
         users = users->m_pkNext) {
        if (!users->m_item->pRadio ||
            (!users->m_item->pRadio->GetObjectReference() ||
             (users->m_item->pRadio->GetObjectReference()->GetFormType() == ACTI_ID &&
              !static_cast<TESObjectACTI*>(users->m_item->pRadio->GetObjectReference())->pRadioStation))) {
            users->m_item->Hndl.Stop();
            users->m_item->Hndl.Release();
            users->m_item->StaticHndl.Stop();
            users->m_item->StaticHndl.Release();
            users->m_item->bShouldBePlaying = false;
        }
    }
}

// The constructor deliberately leaves pStation unset.
FalloutRadio::FORadioStation::FORadioStation() {}

FalloutRadio::FORadioStation::~FORadioStation() {}

void FalloutRadio::FORadioStation::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadFormID(pStation);
    CurrentState.LoadGame(apLoadGameBuffer);
}

void FalloutRadio::FORadioStation::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveFormID(pStation, 0);
    CurrentState.SaveGame(apSaveGameBuffer);
}

void FalloutRadio::FORadioStation::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESObjectREFR* pRef = nullptr;
    if (pStation) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pStation)));
        pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pStation = pRef;
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
        return false;
    }
    bool bretValue = false;
    FalloutRadio::FORadioStation* pstation = FalloutRadio::GetActiveStation(
        static_cast<BGSTalkingActivator*>(apTargetStation->GetObjectReference())
    );
    if (pstation) {
        return pstation->CurrentState.IsContBroadcast();
    }
    return bretValue;
}

void FalloutRadio::StartRadioConversation(TESObjectREFR* apTargetStation, TESTopic* apTopic) {
    FalloutRadio::FORadioStation* pstation = nullptr;
    if (!FalloutRadio::bShutdown) {
        for (FalloutRadio::FORadioStation*& rpItem : FalloutRadio::ActiveStationList.ItemsUntilNull()) {
            if (rpItem->pStation == apTargetStation) {
                pstation = rpItem;
                break;
            }
        }
        if (!pstation) {
            pstation = FalloutRadio::InitStation(apTargetStation, nullptr);
        }
        TESTopic* ptopicToUse = apTopic ? apTopic : TESTopic::GetTopic(DT_RADIO, 0);
        if (pstation && !pstation->pStation->GetDisabled()) {
            bool bwasContBroadcast = pstation->CurrentState.IsContBroadcast();
            FalloutRadio::RadioUpdateCriticalSection.Enter("FalloutRadio::StartRadioConversation");
            if (pstation->CurrentState.pConvo) {
                while (pstation->CurrentState.pConvo->GetNext()) {
                    ;
                }
            }
            if (pstation->CurrentState.IsTransmitting()) {
                pstation->CurrentState.SetContBroadcast(false);
                pstation->CurrentState.iCurrentItemDuration = 1;
                FalloutRadio::UpdateStation(pstation);
            }
            if (FalloutRadio::pCurrentPipboyStation == pstation && FalloutRadio::bPipboyRadioEnabled) {
                FalloutRadio::PipboyVoiceHndl.Stop();
                FalloutRadio::PipboyVoiceHndl.Release();
                if (FalloutRadio::GetIsDialogueOnlyRadio()) {
                    FalloutAudioMedia::MediaClose();
                    FalloutAudioMedia::MediaSetRadio(true);
                    FalloutAudioMedia::MediaOpen(MEDIA_RADIO, nullptr, 1000u, false, false, 0.0f, 0);
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
                        FalloutAudioMedia::MediaSetRadio(true);
                        FalloutAudioMedia::MediaOpen(MEDIA_RADIO, nullptr, 1000u, false, false, 0.0f, 0);
                        FalloutAudioMedia::MediaPlay();
                    }
                }
                pstation->CurrentState.pConvo->GetCurrentItem()->FirstResponse();
            }
            else {
                Conversation* pConvo = pstation->CurrentState.pConvo;
                if (pConvo) {
                    delete pConvo;
                }
                pstation->CurrentState.pConvo = nullptr;
                pstation->CurrentState.iCurrentItemDuration = 1;
            }
            if (pstation->CurrentState.iCurrentItemDuration > 1) {
                pstation->CurrentState.iCurrentItemDuration = 0;
            }
            pstation->CurrentState.iCurrentItemStartTime = BSAudio::QInstance().GetSynchTime() + 50;
            pstation->CurrentState.SetPlayingBreakIn(true);
            pstation->CurrentState.SetContBroadcast(bwasContBroadcast);
            FalloutRadio::RadioUpdateCriticalSection.Leave();
        }
    }
}

void FalloutRadio::ClearInactiveReceivers(bool bRemoveAll) {
    BSSimpleList<FalloutRadio::FORadioReceiver*>* pactReceivers = &FalloutRadio::ActiveRadioList;
    FalloutRadio::FORadioReceiver* pemitter = nullptr;
    while (pactReceivers && !pactReceivers->IsEmpty()) {
        if (pactReceivers->m_item->bShouldBePlaying && !bRemoveAll) {
            pactReceivers = pactReceivers->m_pkNext;
        }
        else {
            pemitter = pactReceivers->m_item;
            FalloutRadio::ActiveRadioList.Remove(pactReceivers->m_item);
            pactReceivers = &FalloutRadio::ActiveRadioList;
            for (BSSimpleList<FalloutRadio::FORadioStation*>* pactStations = &FalloutRadio::ActiveStationList;
                 pactStations && !pactStations->IsEmpty();
                 pactStations = pactStations->m_pkNext) {
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
    BSSimpleList<TESObjectREFR*> stationlist;
    BSSimpleList<float> stationDistances;
    if (!FalloutRadio::bShutdown) {
        if (abUpdateSignals) {
            BGSTalkingActivator::GetRadioStationsInRangeOfRef(pPlayer, &stationlist, &stationDistances);
            for (TESObjectREFR*& rpItem : stationlist.ItemsUntilNull()) {
                if ((!rpItem->GetDisabled() || Script::IsPendingEnabledReference(rpItem)) &&
                    !FalloutRadio::DiscoveredStationList.IsInList(rpItem)) {
                    if (!FalloutRadio::bFirstUpdate) {
                        BSStringT<char> strtemp;
                        BSSoundHandle alertSound = BSAudio::QInstance().GetSoundHandleByName(
                            "UIRadioSignalFound",
                            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_RADIO
                        );
                        alertSound.Play(false);
                        const char* pStationName = rpItem->GetFullName();
                        strtemp.SPrintF(static_cast<const char*>(sRadioStationDiscovered), pStationName);
                        Interface::ShowMessage(
                            strtemp.pString,
                            0,
                            "Interface\\Icons\\Message Icons\\glow_message_radio_tower.dds",
                            nullptr,
                            2.0f,
                            false
                        );
                    }
                    FalloutRadio::DiscoveredStationList.AddHead(rpItem);
                }
            }
            FalloutRadio::bFirstUpdate = false;
        }
        bool bmatch = false;
        float fdistance = std::numeric_limits<float>::max();
        float fstaticDistance = 1.0 - fRadioStaticAtOuterRadiusPct.Float();
        for (BSSimpleList<FalloutRadio::FORadioStation*>* pactStations = &FalloutRadio::ActiveStationList;
             pactStations && !pactStations->IsEmpty();
             pactStations = pactStations->m_pkNext) {
            BSSimpleList<TESObjectREFR*>* stations = &stationlist;
            BSSimpleList<float>* pdistances = &stationDistances;
            bmatch = false;
            fdistance = std::numeric_limits<float>::max();
            FalloutRadio::FORadioStation* pstation = pactStations->m_item;
            if (pstation) {
                while (abUpdateSignals && (stations && pdistances)) {
                    if (stations->m_item == pstation->pStation) {
                        bmatch = true;
                        uint8_t isignalPower = 100;
                        pstation->CurrentState.SetOutOfPBRange(false);
                        fdistance = pdistances->m_item;
                        float fradioRadius = pstation->pStation->GetRadioStationRadius();
                        if (pstation->pStation->GetRadioStaticPct() != 0.0) {
                            fstaticDistance = 1.0 - pstation->pStation->GetRadioStaticPct() / 100.0;
                        }
                        if (pstation->pStation->GetRadioStationRangeType() == RADIO_RANGE_RADIUS &&
                            fradioRadius > 0.0 &&
                            fdistance > 0.0) {
                            float fstaticRatio = fdistance / fradioRadius;
                            if (fstaticDistance < static_cast<double>(fstaticRatio)) {
                                isignalPower = 100 -
                                    static_cast<int32_t>(
                                                   (fstaticRatio - fstaticDistance) / (1.0 - fstaticDistance) * 100.0
                                    );
                            }
                        }
                        pstation->CurrentState.cTargetPower = isignalPower;
                        stations = nullptr;
                        pdistances = nullptr;
                    }
                    else {
                        stations = stations->m_pkNext;
                    }
                }
                if (abUpdateSignals && !bmatch) {
                    pstation->CurrentState.SetOutOfPBRange(true);
                    pstation->CurrentState.cTargetPower = 0;
                }
                if (bmatch && FalloutRadio::pLostPipboyStation == pstation) {
                    BSStringT<char> strtemp;
                    BSSoundHandle alertSound = BSAudio::QInstance().GetSoundHandleByName(
                        "UIRadioSignalFound",
                        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                            BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_RADIO
                    );
                    alertSound.Play(false);
                    const char* pStationName = pstation->pStation->GetFullName();
                    strtemp.SPrintF(static_cast<const char*>(sRadioStationDiscovered), pStationName);
                    Interface::ShowMessage(
                        strtemp.pString,
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_radio_tower.dds",
                        nullptr,
                        2.0f,
                        false
                    );
                    FalloutRadio::PipboyRadioEnable(true);
                    FalloutRadio::TunePipboyToStation(pstation->pStation, false);
                    FalloutRadio::pLostPipboyStation = nullptr;
                }
                FalloutRadio::UpdateStation(pstation);
            }
        }
        stationDistances.RemoveAll();
        stationlist.RemoveAll();
    }
}

FalloutRadio::FORadioStation* FalloutRadio::GetActiveStation(BGSTalkingActivator* apRequestedStation) {
    FalloutRadio::FORadioStation* preturnValue = nullptr;
    BSSimpleList<FalloutRadio::FORadioStation*>* pactStations = &FalloutRadio::ActiveStationList;
    RADIO_RANGE_TYPE erangeType = RADIO_RANGE_RADIUS;
    if (FalloutRadio::bShutdown) {
        return nullptr;
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
    FalloutRadio::bIsDialogueOnlyRadio = abDialogueOnly;
    return abDialogueOnly;
}

bool FalloutRadio::Update(bool abForceStationUpdate) {
    bool bUpdateSignals = false;
    if (!FalloutRadio::iLastRadioUpdate) {
        FalloutRadio::iLastRadioUpdate = appTimer.GetTime();
    }
    uint32_t uiElapsed = appTimer.GetTime() - FalloutRadio::iLastRadioUpdate;
    if (uiElapsed > iRadioUpdateInterval.UInt() && !Interface::GetLoadingMenuVisible()) {
        bUpdateSignals = true;
        FalloutRadio::iLastRadioUpdate = appTimer.GetTime();
    }
    if (pPlayer && pPlayer->GetParentCell()) {
        TESObjectCELL* pParentCell = pPlayer->GetParentCell();
        if (pParentCell->GetInterior()) {
            bUpdateSignals = !FalloutRadio::bWasInInterior;
            FalloutRadio::bWasInInterior = true;
        }
    }

    if (abForceStationUpdate) {
        bUpdateSignals = true;
    }
    if ((FalloutRadio::iQueuedState & 1) != 0) {
        FalloutRadio::iRadioState &= ~2u;
        FalloutRadio::iRadioState |= 1u;
    }
    else if ((FalloutRadio::iQueuedState & 2) != 0) {
        FalloutRadio::iRadioState &= ~1u;
        FalloutRadio::iRadioState |= 2u;
    }
    FalloutRadio::iQueuedState = 0;
    FalloutRadio::ClearInactiveReceivers(false);
    if (!FalloutRadio::pRadioAttenuationModel) {
        FalloutRadio::pRadioAttenuationModel = pDataHandler->GetSound("AMLRadio");
    }
    FalloutRadio::UpdateStations(bUpdateSignals);
    FalloutRadio::ClearInactiveReceivers(false);
    FalloutRadio::iRadioState = 0;
    return true;
}

void FalloutRadio::ActivateRadio(TESObjectREFR* apRadio, bool abOn) {
    if (!FalloutRadio::bShutdown) {
        FalloutRadio::FORadioReceiver* pReceivers[2];
        BGSTalkingActivator* prequestedStation = nullptr;
        pReceivers[0] = FalloutRadio::GetActiveReceiver(apRadio);
        pReceivers[1] = nullptr;
        char cNewReceiver = 0;
        if (!pReceivers[0] ||
            ((!abOn || !pReceivers[0]->bShouldBePlaying) && (abOn || pReceivers[0]->bShouldBePlaying))) {
            if (apRadio && apRadio->GetObjectReference()->GetFormType() == ACTI_ID) {
                prequestedStation = static_cast<TESObjectACTI*>(apRadio->GetObjectReference())->pRadioStation;
            }

            if (prequestedStation) {
                FalloutRadio::FORadioStation* pstation = FalloutRadio::GetActiveStation(prequestedStation);
                if (abOn) {
                    if (pstation || (pstation = FalloutRadio::InitStation(apRadio, nullptr)) != nullptr) {
                        if (!pReceivers[0]) {
                            FalloutRadio::FORadioReceiver* pemitter = new FalloutRadio::FORadioReceiver;
                            pReceivers[0] = pemitter;
                            cNewReceiver = 1;
                        }
                        pReceivers[0]->pRadio = apRadio;
                        pReceivers[0]->bShouldBePlaying = true;
                        pReceivers[0]->bJustStarted = true;
                        pReceivers[0]->bIsActor = false;
                        if (cNewReceiver) {
                            FalloutRadio::ActiveRadioList.AddHead(*pReceivers);
                        }
                        pstation->CurrentState.StationUsers.AddHead(*pReceivers);
                        pstation->CurrentState.SetShouldBePlaying(true);
                        if (pstation->CurrentState.ShouldStartPaused()) {
                            pstation->CurrentState.iCurrentItemStartTime = BSAudio::QInstance().GetSynchTime() + 50;
                            pstation->CurrentState.SetStartPaused(false);
                        }
                    }
                }
                else if (pReceivers[0]) {
                    pReceivers[0]->Hndl.Stop();
                    pReceivers[0]->Hndl.Release();
                    pReceivers[0]->StaticHndl.Stop();
                    pReceivers[0]->StaticHndl.Release();
                    pReceivers[0]->bShouldBePlaying = false;
                    FalloutRadio::ClearInactiveReceivers(false);
                }
            }
        }
    }
}

bool FalloutRadio::IsPipboyRadioEnabled() {
    return FalloutRadio::bPipboyRadioEnabled;
}

void FalloutRadio::SetBroadcastState(TESObjectREFR* apTargetStation, bool abEnabled) {
    if (!FalloutRadio::bShutdown) {
        FalloutRadio::FORadioStation* pstation = FalloutRadio::GetActiveStation(
            static_cast<BGSTalkingActivator*>(apTargetStation->GetObjectReference())
        );
        FalloutRadio::RadioUpdateCriticalSection.Enter("FalloutRadio::SetBroadcastState");
        static_cast<TESObjectACTI*>(apTargetStation->GetObjectReference())
            ->TESObjectACTI::SetContinuousBroadcast(abEnabled);
        if (pstation) {
            if (pstation->CurrentState.IsContBroadcast() && !abEnabled) {
                for (BSSimpleList<FalloutRadio::FORadioReceiver*>* puser = &pstation->CurrentState.StationUsers;
                     puser && !puser->IsEmpty();
                     puser = puser->m_pkNext) {
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
                        FalloutAudioMedia::MediaSetRadio(true);
                        FalloutAudioMedia::MediaOpen(MEDIA_RADIO, nullptr, 1000u, false, false, 0.0f, 0);
                        FalloutAudioMedia::MediaPlay();
                    }
                }
                pstation->CurrentState.SetTransmitting(false);
                Conversation* pConvo = pstation->CurrentState.pConvo;
                if (pConvo) {
                    delete pConvo;
                }
                pstation->CurrentState.pConvo = nullptr;
            }
            pstation->CurrentState.SetContBroadcast(abEnabled);
        }
        FalloutRadio::RadioUpdateCriticalSection.Leave();
    }
}

void FalloutRadio::NewCellEntered() {
    FalloutRadio::bWasInInterior = false;
}

void FalloutRadio::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    BSSimpleList<FalloutRadio::FORadioStation*>* plist = &FalloutRadio::ActiveStationList;
    BSSimpleList<FalloutRadio::FORadioStation*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        FalloutRadio::FORadioStation* pstation = plist->m_item;
        bool bremove = false;
        if (pstation) {
            pstation->FinishLoadGame(apLoadGameBuffer);
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
    FalloutRadio::PipboyRadioEnable(FalloutRadio::bPipboyRadioEnabled);
    if (FalloutRadio::pCurrentPipboyStation) {
        FalloutRadio::PipboyUpdate(true);
    }
}

void FalloutRadio::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {
    FalloutRadio::pCurrentPipboyStation = nullptr;
    FalloutRadio::pLostPipboyStation = nullptr;
    FalloutRadio::iLastRadioUpdate = 1;
    FalloutRadio::iAvailableStationCount = 0;
    FalloutRadio::bPipboyRadioEnabled = false;
    FalloutRadio::bFirstUpdate = true;
    FalloutRadio::cLastPipboyFilename[0] = 0;
    while (!FalloutRadio::ActiveStationList.IsEmpty()) {
        RadioEntry* pEntry = FalloutRadio::ActiveStationList.m_item;
        if (pEntry) {
            delete pEntry;
        }
        FalloutRadio::ActiveStationList.RemoveHead();
    }
    while (!FalloutRadio::ActiveRadioList.IsEmpty()) {
        FalloutRadio::FORadioReceiver* preceiver = FalloutRadio::ActiveRadioList.m_item;
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
    BSSimpleList<FalloutRadio::FORadioStation*>* plist = &FalloutRadio::ActiveStationList;
    BSSimpleList<FalloutRadio::FORadioStation*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        FalloutRadio::FORadioStation* pstation = plist->m_item;
        bool bremove = false;
        if (pstation) {
            pstation->InitLoadGame(apLoadGameBuffer);
            if (!pstation->pStation) {
                delete pstation;
                pstation = nullptr;
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
    BSSimpleList<FalloutRadio::FORadioReceiver*>* preceiverList = &FalloutRadio::ActiveRadioList;
    BSSimpleList<FalloutRadio::FORadioReceiver*>* plastReceiverList = nullptr;
    while (preceiverList && !preceiverList->IsEmpty()) {
        FalloutRadio::FORadioReceiver* preceiver = preceiverList->m_item;
        bool bremove = false;
        if (preceiver) {
            if (!preceiver->pRadio) {
                delete preceiver;
                preceiver = nullptr;
                bremove = true;
            }
        }
        else {
            bremove = true;
        }
        if (bremove) {
            if (plastReceiverList) {
                plastReceiverList->Remove(preceiverList->m_item);
                preceiverList = plastReceiverList->m_pkNext;
            }
            else {
                preceiverList->RemoveHead();
            }
        }
        else {
            plastReceiverList = preceiverList;
            preceiverList = preceiverList->m_pkNext;
        }
    }
    // LoadGame left each station's base object (its BGSTalkingActivator) in these slots, which serve as untyped storage
    // until they are resolved to the active station here.
    if (FalloutRadio::pCurrentPipboyStation) {
        FalloutRadio::pCurrentPipboyStation = FalloutRadio::GetActiveStation(
            reinterpret_cast<BGSTalkingActivator*>(FalloutRadio::pCurrentPipboyStation)
        );
    }
    if (FalloutRadio::pLostPipboyStation) {
        FalloutRadio::pLostPipboyStation = FalloutRadio::GetActiveStation(
            reinterpret_cast<BGSTalkingActivator*>(FalloutRadio::pLostPipboyStation)
        );
    }
    if (apLoadGameBuffer->GetVersion() >= 10u) {
        BSSimpleList<TESObjectREFR*>* pdiscoveredList = &FalloutRadio::DiscoveredStationList;
        BSSimpleList<TESObjectREFR*>* plastDiscoveredList = nullptr;
        while (pdiscoveredList && !pdiscoveredList->IsEmpty()) {
            TESObjectREFR* pForm = pdiscoveredList->m_item;
            bool bremove = false;
            if (pForm) {
                // LoadGame left the form ID in the pointer slot, which is used as untyped storage here.
                TESForm* pFoundForm = TESForm::GetFormByNumericID(
                    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm))
                );
                pForm = dynamic_cast<TESObjectREFR*>(pFoundForm);
                if (pForm) {
                    pdiscoveredList->SetItem(pForm);
                }
                else {
                    bremove = true;
                }
            }
            else {
                bremove = true;
            }
            if (bremove) {
                if (plastDiscoveredList) {
                    plastDiscoveredList->Remove(pdiscoveredList->m_item);
                    pdiscoveredList = plastDiscoveredList->m_pkNext;
                }
                else {
                    pdiscoveredList->RemoveHead();
                }
            }
            else {
                plastDiscoveredList = pdiscoveredList;
                pdiscoveredList = pdiscoveredList->m_pkNext;
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
    bool bsong = false;
    bool bhasVoice = false;
    bool bstaticPlaying = false;
    bool bpipboyUpdated = false;
    if (pStation && pStation->pStation) {
        bool bdisabledStation = pStation->pStation->GetDisabled();
        bool bsystemPausing = (FalloutRadio::iRadioState & 1) != 0;
        bool bsystemUnpausing = (FalloutRadio::iRadioState & 2) != 0;
        bool bpipboyStation = pStation == FalloutRadio::pCurrentPipboyStation;
        float fmaxUpdateDistance = 3000.0f;
        NiPoint3 listenerLocation = BSAudio::QInstance().GetListenerPosition();
        FalloutRadio::RadioUpdateCriticalSection.Enter("FalloutRadio::UpdateStation");
        DialogueItem* pcurrentItem = pStation->CurrentState.pConvo ? pStation->CurrentState.pConvo->GetCurrentItem()
                                                                   : nullptr;
        BSSimpleList<FalloutRadio::FORadioReceiver*>* pusers = &pStation->CurrentState.StationUsers;
        if (!FalloutRadio::bShutdown) {
            if (bpipboyStation || !pStation->CurrentState.StationUsers.IsEmpty()) {
                NiPoint3 loc;
                // The station's sound template: the radio attenuation model unless the activator has its own.
                TESSound* pTemplate;
                pTemplate = static_cast<TESSound*>(FalloutRadio::pRadioAttenuationModel);
                if (pStation->pStation) {
                    TESObjectACTI* pact = dynamic_cast<TESObjectACTI*>(pStation->pStation->GetObjectReference());
                    if (pact && pact->pRadioTemplate) {
                        pTemplate = pact->pRadioTemplate;
                    }
                }
                if (bdisabledStation || (bsystemPausing || bsystemUnpausing)) {
                    for (BSSimpleList<FalloutRadio::FORadioReceiver*>* puser = &pStation->CurrentState.StationUsers;
                         puser && !puser->IsEmpty();
                         puser = puser->m_pkNext) {
                        if (puser->m_item->Hndl.IsValid()) {
                            if (bdisabledStation) {
                                puser->m_item->Hndl.Stop();
                                puser->m_item->Hndl.Release();
                                puser->m_item->StaticHndl.Stop();
                                puser->m_item->StaticHndl.Release();
                            }
                            else if (bsystemPausing) {
                                puser->m_item->Hndl.Pause();
                                puser->m_item->StaticHndl.Pause();
                            }
                            else if (bsystemUnpausing && puser->m_item->bShouldBePlaying) {
                                puser->m_item->Hndl.Play(false);
                                puser->m_item->StaticHndl.Play(false);
                            }
                        }
                    }
                    if (bpipboyStation) {
                        if (bdisabledStation) {
                            FalloutRadio::PipboyRadioEnable(false);
                        }
                        if (FalloutRadio::pCurrentPipboyStation &&
                            FalloutRadio::pCurrentPipboyStation->CurrentState.IsPlayingSong()) {
                            if (bsystemPausing) {
                                FalloutRadio::PipboyStaticHndl.Pause();
                                FalloutAudioMedia::MediaPause();
                            }
                            else if (bsystemUnpausing) {
                                FalloutRadio::PipboyStaticHndl.Play(false);
                                FalloutAudioMedia::MediaUnPause();
                            }
                        }
                        else if (bsystemPausing) {
                            FalloutRadio::PipboyStaticHndl.Pause();
                            FalloutRadio::PipboyVoiceHndl.Pause();
                        }
                        else if (bsystemUnpausing) {
                            FalloutRadio::PipboyStaticHndl.Play(false);
                            FalloutRadio::PipboyVoiceHndl.Play(false);
                        }
                    }
                }
                else {
                    if (pStation->CurrentState.iCurrentItemDuration <= 0 &&
                        pStation == FalloutRadio::pCurrentPipboyStation) {
                        if (FalloutRadio::PipboyVoiceHndl.IsValid()) {
                            pStation->CurrentState.iCurrentItemDuration = FalloutRadio::PipboyVoiceHndl.GetDuration();
                        }
                        else if (
                            pcurrentItem &&
                            pcurrentItem->GetCurrentResponse()->pVoiceSound &&
                            FalloutAudioMedia::MediaGetDuration(MEDIA_RADIO)
                        ) {
                            pStation->CurrentState.iCurrentItemDuration = FalloutAudioMedia::MediaGetDuration(
                                MEDIA_RADIO
                            );
                        }
                    }
                    uint32_t icurrentTime = BSAudio::QInstance().GetSynchTime();
                    if (pStation->CurrentState.iCurrentItemStartTime > icurrentTime + 100) {
                        pStation->CurrentState.iCurrentItemStartTime = icurrentTime;
                    }
                    if (pStation->CurrentState.iCurrentItemDuration > 0 &&
                        pStation->CurrentState.iCurrentItemDuration + pStation->CurrentState.iCurrentItemStartTime <
                            icurrentTime) {
                        if (pStation->CurrentState.pConvo && pStation->CurrentState.pConvo->GetCurrentItem()) {
                            if (pStation->CurrentState.pConvo->GetNext()) {
                                if (pStation->CurrentState.pConvo->GetCurrentItem()->pInfo &&
                                    !pStation->CurrentState.pConvo->GetCurrentItem()->pInfo->IsRunImmediately()) {
                                    pStation->CurrentState.pConvo->GetCurrentItem()->pInfo->RunResult(
                                        TIRS_BEGIN,
                                        pStation->pStation
                                    );
                                }

                                pStation->CurrentState.pConvo->GetCurrentItem()->FirstResponse();
                                pStation->CurrentState.iCurrentItemStartTime = BSAudio::QInstance().GetSynchTime() + 50;
                                pStation->CurrentState.iCurrentItemDuration = 0;
                                if (pStation->CurrentState.StationUsers.IsEmpty() &&
                                    !FalloutRadio::pCurrentPipboyStation) {
                                    pStation->CurrentState.SetStartPaused(true);
                                }
                            }
                            else {
                                Conversation* pConvo = pStation->CurrentState.pConvo;
                                if (pConvo) {
                                    delete pConvo;
                                }
                                pStation->CurrentState.pConvo = nullptr;
                                pcurrentItem = nullptr;
                                pStation->CurrentState.iCurrentItemStartTime = 0;
                                pStation->CurrentState.iCurrentItemDuration = 0;
                                if (pStation->CurrentState.IsPlayingBreakIn()) {
                                    pStation->CurrentState.SetPlayingBreakIn(false);
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
                                    FalloutAudioMedia::MediaSetRadio(true);
                                    FalloutAudioMedia::MediaOpen(MEDIA_RADIO, nullptr, 1000u, false, false, 0.0f, 0);
                                    FalloutAudioMedia::MediaPlay();
                                }
                            }
                        }

                        if (!pStation->CurrentState.pConvo || !pStation->CurrentState.pConvo->GetCurrentItem()) {
                            if (pStation->CurrentState.IsContBroadcast()) {
                                pStation->CurrentState.SetTransmitting(true);
                                FalloutRadio::GetNextConversation(pStation, nullptr);
                                pStation->CurrentState.pConvo->PrecacheSounds();
                                pStation->CurrentState.pConvo->FirstItem();
                                if (pStation->CurrentState.pConvo->GetCurrentItem()) {
                                    pcurrentItem = pStation->CurrentState.pConvo->GetCurrentItem();
                                    if (pStation->CurrentState.pConvo->GetCurrentItem()->pInfo &&
                                        !pStation->CurrentState.pConvo->GetCurrentItem()->pInfo->IsRunImmediately()) {
                                        pStation->CurrentState.pConvo->GetCurrentItem()->pInfo->RunResult(
                                            TIRS_BEGIN,
                                            pStation->pStation
                                        );
                                    }

                                    pStation->CurrentState.pConvo->GetCurrentItem()->FirstResponse();
                                    pStation->CurrentState.iCurrentItemStartTime = BSAudio::QInstance().GetSynchTime() +
                                        50;
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
                                pStation->CurrentState.SetTransmitting(false);
                            }
                        }
                    }
                    if (pStation->CurrentState.pConvo && pStation->CurrentState.pConvo->GetCurrentItem()) {
                        pcurrentItem = pStation->CurrentState.pConvo->GetCurrentItem();
                        if (!pcurrentItem->GetCurrentResponse() && !pcurrentItem->NextResponse()) {
                            pStation->CurrentState.iCurrentItemStartTime = BSAudio::QInstance().GetSynchTime() + 50;
                            pStation->CurrentState.iCurrentItemDuration = 1;
                        }
                    }

                    pStation->CurrentState.cStationPower = pStation->CurrentState.cTargetPower;
                    uint8_t isignalStrength = pStation->CurrentState.GetStationPower();
                    if (isignalStrength < 100u) {
                        bstaticPlaying = true;
                    }
                    pusers = &pStation->CurrentState.StationUsers;
                    while (pusers && !pusers->IsEmpty()) {
                        if (pusers->m_item->bShouldBePlaying) {
                            if (pStation->CurrentState.iCurrentItemDuration <= 0 &&
                                pusers->m_item->Hndl.IsValid() &&
                                pusers->m_item->Hndl.GetDuration()) {
                                pStation->CurrentState.iCurrentItemDuration = pusers->m_item->Hndl.GetDuration();
                            }

                            loc.x = std::numeric_limits<float>::max();
                            loc.y = std::numeric_limits<float>::max();
                            loc.z = std::numeric_limits<float>::max();
                            if (pusers->m_item && pusers->m_item->pRadio) {
                                loc = pusers->m_item->pRadio->GetLocationOnReference();
                            }
                            else if (pusers->m_item) {
                                pusers->m_item->bShouldBePlaying = false;
                                pusers = pusers->m_pkNext;
                                continue;
                            }
                            TESWorldSpace* pWorldSpace;
                            TESObjectCELL* pParentCell;
                            char cFilename[268];
                            if ((pPlayer->GetWorldSpace() &&
                                 (pWorldSpace = pPlayer->GetWorldSpace(),
                                  pWorldSpace != pusers->m_item->pRadio->GetWorldSpace())) ||
                                (pPlayer->GetParentCell()->GetInterior() &&
                                 (pParentCell = pusers->m_item->pRadio->GetParentCell(),
                                  pParentCell != pPlayer->GetParentCell()))) {
                                pusers->m_item->Hndl.SetVolume(0.0f);
                                pusers->m_item->bShouldBePlaying = false;
                            }
                            if (pusers->m_item && pusers->m_item->Hndl.IsValid()) {
                                pusers->m_item->Hndl.SetPosition(loc.x, loc.y, loc.z);
                            }

                            if (!bstaticPlaying || pusers->m_item->StaticHndl.IsValid()) {
                                if (bstaticPlaying) {
                                    float fstaticLevel = static_cast<double>(100 - isignalStrength) / 100.0;
                                    pusers->m_item->StaticHndl.SetVolume(std::powf(fstaticLevel, 0.75f));
                                    pusers->m_item->StaticHndl.SetPosition(loc.x, loc.y, loc.z);
                                }
                                else if (pusers->m_item->StaticHndl.IsValid()) {
                                    pusers->m_item->StaticHndl.Stop();
                                    pusers->m_item->StaticHndl.Release();
                                }
                            }
                            else {
                                pusers->m_item->StaticHndl.Stop();
                                pusers->m_item->StaticHndl.Release();
                                pusers->m_item->StaticHndl = BSAudio::QInstance().GetSoundHandleByName(
                                    "UIRadioStaticLP",
                                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_LOOP |
                                        BSGameSound::SOUND_FLAG_RADIO
                                );
                                pusers->m_item->StaticHndl.bAssumeSuccess = true;
                                pusers->m_item->StaticHndl.SetPosition(loc.x, loc.y, loc.z);
                                float fstaticLevel = static_cast<double>(100 - isignalStrength) / 100.0;
                                pusers->m_item->StaticHndl.SetVolume(std::powf(fstaticLevel, 0.75f));
                                pusers->m_item->StaticHndl.Play(false);
                            }
                            if (!pcurrentItem ||
                                (pusers->m_item->Hndl.IsValid() ||
                                 (!pStation->CurrentState.ShouldBePlaying() ||
                                  (!pusers->m_item->bShouldBePlaying || !pusers->m_item->pRadio)))) {
                                if (!pusers->m_item->pRadio) {
                                    pusers->m_item->bShouldBePlaying = false;
                                }
                            }
                            else if (!pcurrentItem || !pcurrentItem->GetCurrentResponse()) {
                                if (pcurrentItem && (!pcurrentItem->NextResponse() && !pcurrentItem->FirstResponse())) {
                                    pStation->CurrentState.iCurrentItemStartTime = BSAudio::QInstance().GetSynchTime() +
                                        50;
                                    pStation->CurrentState.iCurrentItemDuration = 1;
                                }
                            }
                            else {
                                if (pcurrentItem->GetCurrentResponse()->pVoiceSound) {
                                    bsong = true;
                                }
                                if (pcurrentItem->GetCurrentResponse() &&
                                    pcurrentItem->GetCurrentResponse()->cText.pString) {
                                    uint32_t ivoiceTextLength = BSstrlen(
                                        pcurrentItem->GetCurrentResponse()->cText.pString
                                    );
                                    bhasVoice = ivoiceTextLength > 1 &&
                                        pcurrentItem->GetCurrentResponse()->GetVoice()->pString;
                                }

                                if (bsong) {
                                    const char* pfilename = pcurrentItem->GetCurrentResponse()
                                                                ->pVoiceSound->GetFilename();
                                    BSstrcpy(cFilename, 260, pfilename);
                                    if (!std::strstr(cFilename, "_mono.")) {
                                        char* pnamePt = std::strchr(cFilename, 46);
                                        if (pnamePt) {
                                            char cExtension[4];
                                            BSstrcpy(cExtension, 4, pnamePt + 1);
                                            *pnamePt = 0;
                                            BSstrcat(cFilename, 260, "_mono.");
                                            BSstrcat(cFilename, 260, cExtension);
                                        }
                                    }
                                    pStation->CurrentState.SetPlayingSong(true);
                                }
                                else if (bhasVoice) {
                                    BSstrcpy(cFilename, 260, pcurrentItem->GetCurrentResponse()->GetVoice()->pString);
                                    pStation->CurrentState.SetPlayingSong(false);
                                }
                                else {
                                    pStation->CurrentState.iCurrentItemDuration = 1;
                                    pusers = pusers->m_pkNext;
                                    pStation->CurrentState.SetPlayingSong(false);
                                    continue;
                                }
                                bool bthisActorIsSpeaking = false;
                                bool bcreatureRadio = false;
                                Actor* pactor = nullptr;
                                if (pusers->m_item->pRadio) {
                                    TESObjectREFR* pRadio = pusers->m_item->pRadio;
                                    if (pRadio->IsActor()) {
                                        pactor = static_cast<Actor*>(pusers->m_item->pRadio);
                                        if (pcurrentItem->pInfo) {
                                            TESActorBase* pActorBase = pactor->GetActorBase();
                                            if (pActorBase == pcurrentItem->pInfo->GetSpeaker()) {
                                                bthisActorIsSpeaking = true;
                                            }
                                        }
                                        if (pactor->IsCreature()) {
                                            bcreatureRadio = true;
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
                                double dlistenerDistance = (listenerLocation - loc).Length();
                                if (!(fmaxUpdateDistance < dlistenerDistance)) {
                                    if (pactor && bthisActorIsSpeaking) {
                                        TESObjectREFR* pradioRef = pusers->m_item->pRadio;
                                        TESIdleForm* plistenerIdle = pcurrentItem->GetCurrentResponse()->pListenIdle;
                                        TESIdleForm* pspeakerIdle = pcurrentItem->GetCurrentResponse()->pSpeakerIdle;
                                        uint32_t uiEmotion = pcurrentItem->GetCurrentResponse()->uiEmotion;
                                        DIALOGUE_EMOTION eEmotion = pcurrentItem->GetCurrentResponse()->eEmotion;
                                        pactor->SpeakSoundFunction(
                                            cFilename,
                                            pusers->m_item->Hndl,
                                            eEmotion,
                                            uiEmotion,
                                            150,
                                            pspeakerIdle,
                                            plistenerIdle,
                                            pradioRef,
                                            false,
                                            false,
                                            false,
                                            true,
                                            true
                                        );
                                    }
                                    else {
                                        if (bcreatureRadio) {
                                            uint32_t uiflags = BSGameSound::SOUND_FLAG_IS_3D |
                                                BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_RADIO |
                                                BSGameSound::SOUND_FLAG_UNKBIT22;
                                            pusers->m_item->Hndl = BSAudio::QInstance().GetSoundHandleByFilename(
                                                cFilename,
                                                uiflags,
                                                nullptr
                                            );
                                            float fmaxDistance = fCreatureRadioMax.Float();
                                            pusers->m_item->Hndl.SetMinMax(fCreatureRadioMin.Float(), fmaxDistance);
                                        }
                                        else {
                                            uint32_t uiflags = BSGameSound::SOUND_FLAG_IS_3D |
                                                BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_RADIO |
                                                BSGameSound::SOUND_FLAG_UNKBIT22;
                                            pusers->m_item->Hndl = BSAudio::QInstance().GetSoundHandleByFilename(
                                                cFilename,
                                                uiflags,
                                                pTemplate
                                            );
                                        }
                                        pusers->m_item->Hndl.SetCompletionCallback(
                                            &FalloutRadio::SoundCompletionCallback,
                                            pStation
                                        );
                                        pusers->m_item->Hndl.SetSynchStartTime(
                                            pStation->CurrentState.iCurrentItemStartTime
                                        );
                                        pusers->m_item->Hndl.SetPosition(loc.x, loc.y, loc.z);
                                        float fvolume = static_cast<double>(isignalStrength) / 100.0;
                                        pusers->m_item->Hndl.SetVolume(fvolume);
                                        pusers->m_item->Hndl.Play(false);
                                    }
                                    if (bpipboyStation && !pusers->m_item->bJustStarted && !bpipboyUpdated) {
                                        bpipboyUpdated = true;
                                        FalloutRadio::PipboyUpdate(true);
                                    }
                                    if (pusers->m_item->bJustStarted) {
                                        pusers->m_item->bJustStarted = false;
                                    }
                                }
                            }
                        }
                        pusers = pusers->m_pkNext;
                    }
                    if (pStation == FalloutRadio::pCurrentPipboyStation && !bpipboyUpdated) {
                        FalloutRadio::PipboyUpdate(false);
                    }
                }
            }
            FalloutRadio::RadioUpdateCriticalSection.Leave();
        }
    }
}

FalloutRadio::FORadioReceiver* FalloutRadio::GetActiveReceiver(TESObjectREFR* apRequestedReceiver) {
    FalloutRadio::FORadioReceiver* preturnValue = nullptr;
    BSSimpleList<FalloutRadio::FORadioReceiver*>* pactReceivers = &FalloutRadio::ActiveRadioList;
    bool bfound = false;
    while (pactReceivers && (!pactReceivers->IsEmpty() && !bfound)) {
        if (apRequestedReceiver == pactReceivers->m_item->pRadio) {
            preturnValue = pactReceivers->m_item;
            bfound = true;
        }
        pactReceivers = pactReceivers->m_pkNext;
    }
    return preturnValue;
}

void FalloutRadio::RadioSynchPaused() {
    FalloutRadio::iQueuedState &= ~2u;
    FalloutRadio::iQueuedState |= 1u;
    if (FalloutRadio::pCurrentPipboyStation) {
        FalloutRadio::UpdateStation(FalloutRadio::pCurrentPipboyStation);
    }
}

void FalloutRadio::RadioSynchUnPaused() {
    FalloutRadio::iQueuedState &= ~1u;
    FalloutRadio::iQueuedState |= 2u;
}

char FalloutRadio::GetIsDialogueOnlyRadio() {
    return FalloutRadio::bIsDialogueOnlyRadio;
}

void FalloutRadio::DisableNPCRadio(TESObjectREFR* apNPCRadio) {
    if (apNPCRadio->IsActor()) {
        FORadioReceiver* preceiver = FalloutRadio::GetActiveReceiver(apNPCRadio);
        if (preceiver) {
            preceiver->Hndl.Stop();
            preceiver->Hndl.Release();
            preceiver->bShouldBePlaying = false;
            preceiver->pRadio = nullptr;
        }
    }
}

// The context is the FORadioStation the sound plays for (both SetCompletionCallback callers): a failed sound ends the
// station's current item by setting CurrentState.iCurrentItemDuration to 1.
void FalloutRadio::SoundCompletionCallback(void* apUnused, bool abSucceeded) {
    if (!abSucceeded && apUnused) {
        static_cast<FORadioStation*>(apUnused)->CurrentState.iCurrentItemDuration = 1;
    }
}

FalloutRadio::FORadioStation* FalloutRadio::InitStation(TESObjectREFR* pWorldRadio, TESObjectREFR* apTargetStation) {
    BGSTalkingActivator* ptunedStation;
    if (!pWorldRadio) {
        return nullptr;
    }
    bool breceiver = false;
    bool btransmitter = false;
    TESObjectREFR* pstation = nullptr;
    FalloutRadio::FORadioStation* pnewStation = new FalloutRadio::FORadioStation;
    if (pWorldRadio->GetObjectReference()->GetFormType() == ACTI_ID ||
        (pWorldRadio->GetObjectReference()->GetFormType() == NPC__ID ||
         (pWorldRadio->GetObjectReference()->GetFormType() == CREA_ID ||
          (pWorldRadio->GetObjectReference()->GetFormType() == LVLC_ID ||
           pWorldRadio->GetObjectReference()->GetFormType() == LVLN_ID)))) {
        breceiver = true;
    }
    if (pWorldRadio->GetObjectReference()->GetFormType() == TACT_ID) {
        btransmitter = true;
    }
    if (breceiver && !pWorldRadio->IsActor()) {
        // Reads the base object's radio station without checking the base object's type.
        ptunedStation = static_cast<TESObjectACTI*>(pWorldRadio->GetObjectReference())->pRadioStation;
        if (!ptunedStation) {
            if (pnewStation) {
                delete pnewStation;
            }
            return nullptr;
        }
        pstation = FalloutRadio::GetRadioRefrForStation(ptunedStation, pWorldRadio);
        if (!pstation) {
            if (pnewStation) {
                delete pnewStation;
            }
            return nullptr;
        }
    }
    else if (!breceiver || (!pWorldRadio->IsActor() || !apTargetStation)) {
        if (btransmitter) {
            pstation = pWorldRadio;
        }
    }
    else {
        ptunedStation = static_cast<BGSTalkingActivator*>(apTargetStation->GetObjectReference());
        if (!ptunedStation) {
            if (pnewStation) {
                delete pnewStation;
            }
            return nullptr;
        }
        pstation = FalloutRadio::GetRadioRefrForStation(ptunedStation, pWorldRadio);
    }
    if (pstation) {
        pnewStation->pStation = pstation;
        pnewStation->CurrentState.pConvo = nullptr;
        pnewStation->CurrentState.pNextConvo = nullptr;
        uint32_t icurrentTime = BSAudio::QInstance().GetSynchTime();
        pnewStation->CurrentState.iCurrentItemStartTime = icurrentTime - RandomInt(0, 0x7530);
        pnewStation->CurrentState.iCurrentItemDuration = 1;
        TESBoundObject* pbaseObject = pnewStation->pStation->GetObjectReference();
        pnewStation->CurrentState.SetContBroadcast(pbaseObject->GetContinuousBroadcast());
        pnewStation->CurrentState.SetTransmitting(pnewStation->CurrentState.IsContBroadcast());
        pnewStation->CurrentState.SetJustTurnedOn(true);
        pnewStation->CurrentState.cStationPower = 100;
        pnewStation->CurrentState.cTargetPower = 100;
        FalloutRadio::ActiveStationList.AddHead(pnewStation);
        return pnewStation;
    }
    if (pnewStation) {
        delete pnewStation;
    }
    return nullptr;
}

TESObjectREFR*
FalloutRadio::GetRadioRefrForStation(BGSTalkingActivator* apRequestedStation, TESObjectREFR* pWorldRadio) {
    TESObjectREFR* presult = nullptr;
    BSSimpleList<TESObjectREFR*> stationlist;
    if (pWorldRadio) {
        BGSTalkingActivator::GetRadioStationsInRangeOfRef(pWorldRadio, &stationlist, nullptr);
    }
    else {
        BGSTalkingActivator::GetRadioStationsInRangeOfRef(pPlayer, &stationlist, nullptr);
    }
    BSSimpleList<TESObjectREFR*>* stations = &stationlist;
    bool bdone = false;
    while (!bdone && (stations && (stations->m_item && stations->m_item->GetObjectReference()))) {
        if (apRequestedStation == stations->m_item->GetObjectReference()) {
            presult = stations->m_item;
            bdone = true;
        }
        stations = stations->m_pkNext;
    }
    return presult;
}

void FalloutRadio::Shutdown() {
    FalloutRadio::bShutdown = true;
    FalloutRadio::ClearInactiveReceivers(true);
    for (BSSimpleList<FalloutRadio::FORadioStation*>* pactStations = &FalloutRadio::ActiveStationList;
         pactStations && !pactStations->IsEmpty();
         pactStations = pactStations->m_pkNext) {
        if (pactStations->m_item->CurrentState.pConvo) {
            Conversation* pConvo = pactStations->m_item->CurrentState.pConvo;
            if (pConvo) {
                delete pConvo;
            }
            pactStations->m_item->CurrentState.pConvo = nullptr;
        }
        RadioEntry* pEntry = pactStations->m_item;
        if (pEntry) {
            delete pEntry;
        }
    }
    FalloutRadio::ActiveStationList.RemoveAll();
    for (BSSimpleList<FalloutRadio::FORadioReceiver*>* pactReceivers = &FalloutRadio::ActiveRadioList;
         pactReceivers && !pactReceivers->IsEmpty();
         pactReceivers = pactReceivers->m_pkNext) {
        FalloutRadio::FORadioReceiver* preceiver = pactReceivers->m_item;
        if (preceiver) {
            delete preceiver;
        }
    }
    FalloutRadio::ActiveRadioList.RemoveAll();
    FalloutRadio::DiscoveredStationList.RemoveAll();
}

bool FalloutRadio::TunePipboyToStation(TESObjectREFR* apTargetStation, bool abForceStationUpdate) {
    if (!FalloutRadio::bPipboyRadioEnabled || FalloutRadio::bShutdown) {
        return false;
    }
    ourMemset(FalloutRadio::cLastPipboyFilename, 0, 260u);
    if (apTargetStation) {
        if (abForceStationUpdate) {
            FalloutRadio::pLostPipboyStation = nullptr;
            FalloutRadio::UpdateStations(true);
        }
    }
    else {
        BSSimpleList<TESObjectREFR*> stationlist;
        BGSTalkingActivator::GetRadioStationsInRangeOfRef(pPlayer, &stationlist, nullptr);
        BSSimpleList<TESObjectREFR*>* stations = &stationlist;
        if (&stationlist && stations->m_item && stations->m_item->GetObjectReference()) {
            apTargetStation = stations->m_item;
        }
    }
    FalloutRadio::pCurrentPipboyStation = FalloutRadio::GetActiveStation(
        static_cast<BGSTalkingActivator*>(apTargetStation->GetObjectReference())
    );
    if (FalloutRadio::pCurrentPipboyStation) {
        TESBoundObject* pbaseObject = FalloutRadio::pCurrentPipboyStation->pStation->GetObjectReference();
        if (pbaseObject == apTargetStation->GetObjectReference()) {
            FalloutRadio::pCurrentPipboyStation->pStation = apTargetStation;
        }
    }
    else {
        FalloutRadio::pCurrentPipboyStation = FalloutRadio::InitStation(apTargetStation, nullptr);
        if (!FalloutRadio::pCurrentPipboyStation) {
            FalloutRadio::PipboyRadioEnable(false);
        }
        if (!abForceStationUpdate) {
            FalloutRadio::UpdateStations(true);
        }
    }
    if (FalloutRadio::pCurrentPipboyStation->CurrentState.ShouldStartPaused()) {
        FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemStartTime = BSAudio::QInstance().GetSynchTime() +
            50;
        FalloutRadio::pCurrentPipboyStation->CurrentState.SetStartPaused(false);
    }
    FalloutRadio::pCurrentPipboyStation->CurrentState.SetShouldBePlaying(true);
    FalloutRadio::pLostPipboyStation = nullptr;
    return true;
}

void FalloutRadio::PipboyUpdate(bool abForceUpdate) {
    double dsignalVolume = 0.0;
    if (FalloutRadio::pCurrentPipboyStation) {
        int32_t istationPower = FalloutRadio::pCurrentPipboyStation->CurrentState.GetStationPower();
        dsignalVolume = static_cast<double>(istationPower) / 100.0;
    }
    float fsignalVolumeModifier = dsignalVolume;
    bool bvolumeAlreadySet = false;
    if (FalloutRadio::pCurrentPipboyStation && FalloutRadio::pCurrentPipboyStation->CurrentState.OutOfPBRange()) {
        BSStringT<char> strtemp;
        BSSoundHandle alertSound = BSAudio::QInstance().GetSoundHandleByName(
            "UIRadioSignalLost",
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT |
                BSGameSound::SOUND_FLAG_RADIO
        );
        alertSound.Play(false);
        const char* pstationName = FalloutRadio::pCurrentPipboyStation->pStation->GetFullName();
        strtemp.SPrintF(sRadioSignalLost.String(), pstationName);
        FalloutRadio::pLostPipboyStation = FalloutRadio::pCurrentPipboyStation;
        FalloutRadio::PipboyRadioEnable(false);
        FalloutRadio::PipboyStaticHndl.Stop();
        FalloutRadio::PipboyStaticHndl.Release();
        HUDMainMenu::pme->ShowMessage(
            strtemp.String(),
            HUDMainMenu::HME_NEUTRAL,
            "Interface\\Icons\\Message Icons\\glow_message_radio_tower.dds",
            nullptr,
            2.0f,
            false
        );
        return;
    }
    if (FalloutRadio::pCurrentPipboyStation &&
        FalloutRadio::pCurrentPipboyStation->CurrentState.GetStationPower() < 100u) {
        if (!FalloutRadio::PipboyStaticHndl.IsValid()) {
            FalloutRadio::PipboyStaticHndl.Stop();
            FalloutRadio::PipboyStaticHndl.Release();
            FalloutRadio::PipboyStaticHndl = BSAudio::QInstance().GetSoundHandleByName(
                "UIRadioStaticLP",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                    BSGameSound::SOUND_FLAG_RADIO
            );
            FalloutRadio::PipboyStaticHndl.Play(false);
        }
        int32_t imissingPower = 100 - FalloutRadio::pCurrentPipboyStation->CurrentState.GetStationPower();
        float fstaticLevel = static_cast<double>(imissingPower) / 100.0;
        float fstaticVolume = std::powf(fstaticLevel, 0.75f);
        FalloutRadio::PipboyStaticHndl.SetVolume(fstaticVolume);
    }
    else if (
        FalloutRadio::pCurrentPipboyStation &&
        (FalloutRadio::pCurrentPipboyStation->CurrentState.GetStationPower() == 100 &&
         FalloutRadio::PipboyStaticHndl.IsValid())
    ) {
        FalloutRadio::PipboyStaticHndl.Stop();
        FalloutRadio::PipboyStaticHndl.Release();
    }
    if (FalloutRadio::pCurrentPipboyStation &&
        (FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemDuration <= 0 &&
         FalloutRadio::PipboyVoiceHndl.IsValid()) &&
        FalloutRadio::PipboyVoiceHndl.GetDuration()) {
        FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemDuration = FalloutRadio::PipboyVoiceHndl
                                                                                     .GetDuration();
    }

    if (FalloutRadio::pCurrentPipboyStation &&
        (FalloutRadio::pCurrentPipboyStation->CurrentState.pConvo &&
         ((!FalloutAudioMedia::MediaIsPlaying(MEDIA_RADIO, true) && !FalloutRadio::PipboyVoiceHndl.IsValid()) ||
          abForceUpdate))) {
        DialogueItem* pcurrentItem = FalloutRadio::pCurrentPipboyStation->CurrentState.pConvo->GetCurrentItem();
        if (!pcurrentItem) {
            return;
        }
        bool bsong = false;
        char cplayVoice = 0;
        if (!pcurrentItem->GetCurrentResponse()) {
            pcurrentItem->FirstResponse();
        }
        if (pcurrentItem->GetCurrentResponse() &&
            (pcurrentItem->GetCurrentResponse()->pVoiceSound && !FalloutRadio::GetIsDialogueOnlyRadio())) {
            bsong = true;
            if (FalloutRadio::pCurrentPipboyStation) {
                FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingSong(true);
            }
        }
        else if (FalloutRadio::pCurrentPipboyStation) {
            FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingSong(false);
        }
        if (pcurrentItem->GetCurrentResponse() && pcurrentItem->GetCurrentResponse()->cText.pString) {
            uint32_t ivoiceTextLength = BSstrlen(pcurrentItem->GetCurrentResponse()->cText.pString);
            int32_t ihasVoiceText = ivoiceTextLength > 2 &&
                *pcurrentItem->GetCurrentResponse()->GetVoice() != static_cast<const char*>(nullptr);
            cplayVoice = ihasVoiceText;
            if (!ihasVoiceText && pcurrentItem->GetCurrentResponse()->pVoiceSound) {
                const char* pfilename = pcurrentItem->GetCurrentResponse()->pVoiceSound->GetFilename();
                if (std::strstr(pfilename, ".wav")) {
                    cplayVoice = 1;
                }
            }
        }

        if (cplayVoice) {
            bool bhasVoiceSound = pcurrentItem->GetCurrentResponse()->pVoiceSound != nullptr;
            FalloutRadio::PipboyVoiceHndl.Stop();
            FalloutRadio::PipboyVoiceHndl.Release();
            if (bhasVoiceSound) {
                // Passes the voice sound as the settings context.
                TESSound* pvoiceSound = pcurrentItem->GetCurrentResponse()->pVoiceSound;
                const char* pfilename = pcurrentItem->GetCurrentResponse()->pVoiceSound->GetFilename();
                FalloutRadio::PipboyVoiceHndl = BSAudio::QInstance().GetSoundHandleByFilename(
                    pfilename,
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_RADIO,
                    pvoiceSound
                );
            }
            else {
                const char* pvoiceFilename = pcurrentItem->GetCurrentResponse()->GetVoice()->String();
                FalloutRadio::PipboyVoiceHndl = BSAudio::QInstance().GetSoundHandleByFilename(
                    pvoiceFilename,
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_RADIO,
                    nullptr
                );
            }
            FalloutRadio::PipboyVoiceHndl.SetCompletionCallback(
                &FalloutRadio::SoundCompletionCallback,
                FalloutRadio::pCurrentPipboyStation
            );
            FalloutRadio::PipboyVoiceHndl.SetSynchStartTime(
                FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemStartTime
            );
            if (FalloutAudio::IsRadioMute()) {
                float fvolume = fsignalVolumeModifier * 0.10000000149011612;
                FalloutRadio::PipboyVoiceHndl.SetVolume(fvolume);
            }
            else {
                BSWin32Audio* paudio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
                double dradioVolume = paudio->GetRadioVolume();
                float fvolume = dradioVolume * fsignalVolumeModifier;
                FalloutRadio::PipboyVoiceHndl.SetVolume(fvolume);
            }
            bvolumeAlreadySet = true;
            FalloutRadio::PipboyVoiceHndl.Play(false);
            if (!FalloutRadio::PipboyVoiceHndl.IsValid()) {
                NiPoint3 speakerLoc = pPlayer->GetLocationOnReference();
                char* psubtitle = pcurrentItem->GetCurrentResponse()->cText.pString;
                Interface::ShowSubtitle(psubtitle, FalloutRadio::PipboyVoiceHndl, speakerLoc, pPlayer, false);
            }
        }
        if (!bsong || FalloutRadio::GetIsDialogueOnlyRadio()) {
            FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingSong(false);
        }
        else {
            char cFilename[268];
            const char* psongFilename = pcurrentItem->GetCurrentResponse()->pVoiceSound->GetFilename();
            BSsprintf(cFilename, 260, "%s%s", "data\\sound\\", psongFilename);
            BSStringT<char> lipString(static_cast<const char*>(cFilename));
            FalloutAudioMedia::MediaOpen(
                MEDIA_RADIO,
                cFilename,
                500u,
                false,
                true,
                0.0f,
                FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemStartTime
            );
            BSstrcpy(FalloutRadio::cLastPipboyFilename, 260, cFilename);
            FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingSong(true);
            if (FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemDuration <= 0 &&
                FalloutAudioMedia::MediaGetDuration(MEDIA_RADIO)) {
                FalloutRadio::pCurrentPipboyStation->CurrentState
                    .iCurrentItemDuration = FalloutAudioMedia::MediaGetDuration(MEDIA_RADIO);
            }
        }
        if (!bsong && !cplayVoice) {
            FalloutRadio::pCurrentPipboyStation->CurrentState.iCurrentItemDuration = 1;
        }
    }
    else if (
        !FalloutRadio::pCurrentPipboyStation->CurrentState.IsContBroadcast() &&
        !FalloutRadio::pCurrentPipboyStation->CurrentState.IsPlayingBreakIn() &&
        !FalloutRadio::pCurrentPipboyStation->CurrentState.IsTransmitting() &&
        !FalloutRadio::PipboyVoiceHndl.IsPlaying()
    ) {
        FalloutRadio::PipboyVoiceHndl.Stop();
        FalloutRadio::PipboyVoiceHndl.Release();
        FalloutRadio::PipboyVoiceHndl = BSAudio::QInstance().GetSoundHandleByName(
            "UIRadioStaticLP",
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_LOOP | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                BSGameSound::SOUND_FLAG_RADIO
        );
        FalloutRadio::PipboyVoiceHndl.bAssumeSuccess = true;
        FalloutRadio::PipboyVoiceHndl.Play(false);
        FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingStatic(true);
        FalloutRadio::pCurrentPipboyStation->CurrentState.SetPlayingSong(false);
    }
    if (!bvolumeAlreadySet) {
        if (FalloutAudio::IsRadioMute()) {
            float fvolume = fsignalVolumeModifier * 0.10000000149011612;
            FalloutRadio::PipboyVoiceHndl.SetVolume(fvolume);
        }
        else {
            BSWin32Audio* paudio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            double dradioVolume = paudio->GetRadioVolume();
            float fvolume = dradioVolume * fsignalVolumeModifier;
            FalloutRadio::PipboyVoiceHndl.SetVolume(fvolume);
        }
    }
}

void FalloutRadio::EnableNPCRadio(TESObjectREFR* apNPCRadio, TESObjectREFR* apTargetStation) {
    FalloutRadio::FORadioReceiver* pemitter = FalloutRadio::GetActiveReceiver(apNPCRadio);
    bool baddToList = false;
    if (!FalloutRadio::bShutdown) {
        FalloutRadio::FORadioStation* pstation = FalloutRadio::GetActiveStation(
            static_cast<BGSTalkingActivator*>(apTargetStation->GetObjectReference())
        );
        if (!pstation) {
            pstation = FalloutRadio::InitStation(apNPCRadio, apTargetStation);
            if (pstation && !pstation->pStation) {
                platform::DebuggerBreak();
            }

            if (!pstation) {
                return;
            }
        }
        if (!pemitter) {
            FalloutRadio::FORadioReceiver* pnewEmitter = new FalloutRadio::FORadioReceiver;
            pemitter = pnewEmitter;
            baddToList = true;
        }
        pemitter->pRadio = apNPCRadio;
        pemitter->bShouldBePlaying = true;
        pemitter->bJustStarted = true;
        pemitter->bIsActor = true;
        if (baddToList) {
            FalloutRadio::ActiveRadioList.AddHead(pemitter);
        }
        pstation->CurrentState.StationUsers.AddHead(pemitter);
        pstation->CurrentState.SetShouldBePlaying(true);
    }
}

void FalloutRadio::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer
        ->SaveDataEndian(&FalloutRadio::iAvailableStationCount, sizeof(FalloutRadio::iAvailableStationCount), 0);
    apSaveGameBuffer->SaveDataEndian(
        reinterpret_cast<char*>(&FalloutRadio::bPipboyRadioEnabled),
        sizeof(FalloutRadio::bPipboyRadioEnabled),
        0
    );
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
    isaveGameCount = 0;
    isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (FalloutRadio::FORadioStation* pstation : FalloutRadio::ActiveStationList) {
        if (pstation) {
            pstation->SaveGame(apSaveGameBuffer);
            ++isaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    TESBoundObject* pcurrentPipboyStationObject = nullptr;
    if (FalloutRadio::pCurrentPipboyStation) {
        pcurrentPipboyStationObject = FalloutRadio::pCurrentPipboyStation->pStation->GetObjectReference();
    }
    apSaveGameBuffer->SaveFormID(pcurrentPipboyStationObject, 0);
    TESBoundObject* plostPipboyStationObject = nullptr;
    if (FalloutRadio::pLostPipboyStation) {
        plostPipboyStationObject = FalloutRadio::pLostPipboyStation->pStation->GetObjectReference();
    }
    apSaveGameBuffer->SaveFormID(plostPipboyStationObject, 0);
    isaveGameCount = 0;
    isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (TESObjectREFR* pForm : FalloutRadio::DiscoveredStationList) {
        if (pForm) {
            apSaveGameBuffer->SaveFormID(pForm, 0);
            ++isaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
}

void FalloutRadio::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&FalloutRadio::iAvailableStationCount, sizeof(FalloutRadio::iAvailableStationCount));
    apLoadGameBuffer->LoadData(&FalloutRadio::bPipboyRadioEnabled, sizeof(FalloutRadio::bPipboyRadioEnabled));
    apLoadGameBuffer->LoadString(FalloutRadio::cLastPipboyFilename);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        FalloutRadio::FORadioReceiver* preceiver = new FalloutRadio::FORadioReceiver;
        preceiver->LoadGame(apLoadGameBuffer);
        FalloutRadio::ActiveRadioList.AddHead(preceiver);
    }
    isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        FalloutRadio::FORadioStation* pstation;
        pstation = new FalloutRadio::FORadioStation;
        pstation->LoadGame(apLoadGameBuffer);
        FalloutRadio::ActiveStationList.AddHead(pstation);
    }
    // Each station's TESBoundObject is kept in the FORadioStation* slot until InitLoadGame resolves it.
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pstationForm = TESForm::GetFormByNumericID(uiFormID);
    FalloutRadio::pCurrentPipboyStation = reinterpret_cast<FalloutRadio::FORadioStation*>(
        dynamic_cast<TESBoundObject*>(pstationForm)
    );
    uiFormID = apLoadGameBuffer->LoadFormID();
    pstationForm = TESForm::GetFormByNumericID(uiFormID);
    FalloutRadio::pLostPipboyStation = reinterpret_cast<FalloutRadio::FORadioStation*>(
        dynamic_cast<TESBoundObject*>(pstationForm)
    );
    if (apLoadGameBuffer->GetVersion() >= 10u) {
        isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
        for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
            TESObjectREFR* pForm = nullptr;
            // The form ID is kept in the pointer slot, untyped storage until InitLoadGame resolves it.
            apLoadGameBuffer->LoadFormID(pForm);
            FalloutRadio::DiscoveredStationList.AddHead(pForm);
        }
    }
}

bool FalloutRadio::GetNextConversation(FalloutRadio::FORadioStation* apStation, TESTopic* apOverrideTopic) {
    TESTopic* ptopic = apOverrideTopic ? apOverrideTopic : TESTopic::GetTopic(DT_RADIO, 0);
    if (apStation->CurrentState.pNextConvo) {
        RadioConvTask* ptask;
        if (apStation->CurrentState.pConvo) {
            Conversation* pConvo = apStation->CurrentState.pConvo;
            if (pConvo) {
                delete pConvo;
            }
            apStation->CurrentState.pConvo = nullptr;
        }
        apStation->CurrentState.pConvo = apStation->CurrentState.pNextConvo;
        Conversation* pNextConvo = apStation->CurrentState.pNextConvo;
        if (pNextConvo) {
            delete pNextConvo;
        }
        apStation->CurrentState.pNextConvo = nullptr;
        ptask = new RadioConvTask(apStation);
        pIOManager->AddTask(ptask);
        return true;
    }

    Conversation* pnewConvo;
    if (apStation->CurrentState.pConvo) {
        Conversation* pConvo = apStation->CurrentState.pConvo;
        if (pConvo) {
            delete pConvo;
        }
        apStation->CurrentState.pConvo = nullptr;
    }
    pnewConvo = new Conversation(apStation->pStation, apStation->pStation, ptopic);
    apStation->CurrentState.pConvo = pnewConvo;
    return false;
}

RadioConvTask::RadioConvTask(FalloutRadio::FORadioStation* apStation)
  : QueuedFileEntry(IO_TASK_PRIORITY_CRITICAL), pStation(apStation) {}

RadioConvTask::~RadioConvTask() {}

void RadioConvTask::Run() {
    if (pStation && !pStation->CurrentState.pNextConvo) {
        pStation->CurrentState
            .pNextConvo = new Conversation(pStation->pStation, pStation->pStation, TESTopic::GetTopic(DT_RADIO, 0));
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
