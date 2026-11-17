#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class BGSTalkingActivator;
class Conversation;
class TESObjectREFR;
class TESTopic;

// The radio stations and receivers. Only some of the members are declared.
class FalloutRadio {
public:
	class FORadioStation;

	// One receiver (a radio or an actor with a radio) tuned to a station: the sound handles it plays.
	class FORadioReceiver {
	public:
		FORadioReceiver();
		~FORadioReceiver();

		void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
		void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
		void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

		static void operator delete(void* apMemory);

		TESObjectREFR* pRadio;
		BSSoundHandle Hndl;
		BSSoundHandle StaticHndl;
		bool bShouldBePlaying;
		bool bJustStarted;
		bool bInRange;
		bool bIsActor;
	};

	// What a station is playing now: the conversation, its timing, power and receivers.
	class StationState {
	public:
		StationState();
		~StationState();

		bool ShouldStartPaused();
		bool IsContBroadcast();
		void SetJustTurnedOn(bool abVal);
		void SetTransmitting(bool abVal);
		bool OutOfPBRange();
		bool IsPlayingBreakIn();
		uint8_t GetStationPower();
		void SetPlayingSong(bool abVal);
		void SetPlayingStatic(bool abVal);
		void SetOutOfPBRange(bool abVal);
		void SetShouldBePlaying(bool abVal);
		void SetContBroadcast(bool abVal);
		bool ShouldBePlaying();
		bool IsPlayingSong();
		// Tests the whole byte; bool would assume 0/1.
		void SetPlayingBreakIn(bool abVal);
		bool IsTransmitting();
		// Reads the station state.
		void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
		void SetStartPaused(bool abVal);
		void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
		void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
		void FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

		Conversation* pConvo;
		Conversation* pNextConvo;
		uint32_t iCurrentItemStartTime;
		int32_t iCurrentItemDuration;
		uint8_t cStationPower;
		uint8_t cTargetPower;
		uint8_t padding12[2];
		uint32_t iStationFlags;
		BSSimpleList<FORadioReceiver*> StationUsers;
	};

	// A station: its reference and what it plays.
	class FORadioStation {
	public:
		FORadioStation();
		~FORadioStation();

		// Reads the station reference's form ID (into pStation, resolved later), then the state.
		void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
		void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
		void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
		void FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

		static void operator delete(void* apMemory);

		TESObjectREFR* pStation;
		StationState CurrentState;
	};

	using cLastPipboyFilename_t = char[260];

	// The broadcast state of apTargetStation.
	static bool GetBroadcastState(TESObjectREFR* apTargetStation);
	// Starts the radio conversation of apTopic on apRef.
	static void StartRadioConversation(TESObjectREFR* apTargetStation, TESTopic* apTopic);
	// Drops the receivers that are no longer in range.
	static void ClearInactiveReceivers(bool bRemoveAll);
	// Updates the stations.
	static void UpdateStations(bool abUpdateSignals);
	// The station apRef is tuned to, or null.
	static FalloutRadio::FORadioStation* GetActiveStation(BGSTalkingActivator* apRequestedStation);
	// Stores abDialogueOnly in the dialogue-only radio flag.
	static bool SetIsDialogueOnlyRadio(bool abDialogueOnly);
	// Updates the stations and the receivers; returns true.
	static bool Update(bool abForceStationUpdate);
	// Turns the radio receiver apRadio on or off.
	static void ActivateRadio(TESObjectREFR* apRadio, bool abOn);
	// bPipboyRadioEnabled.
	static bool IsPipboyRadioEnabled();
	static BSSimpleList<TESObjectREFR*>* QDiscoveredStationsList();
	// Sets the station's continuous broadcast state.
	static void SetBroadcastState(TESObjectREFR* apTargetStation, bool abEnabled);
	static void NewCellEntered();
	// Finishes loading the radio save data.
	static void FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Reverts the radio save data.
	static void Revert(BGSLoadGameBuffer* apLoadGameBuffer);
	// Initialises the radio save data after a load.
	static void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Turns the pip-boy radio on or off and returns abEnable; false, with nothing done, once the radio is shut down.
	static bool PipboyRadioEnable(bool abEnable);
	static void UpdateStation(FORadioStation* pStation);
	static FORadioReceiver* GetActiveReceiver(TESObjectREFR* apRequestedReceiver);
	static void RadioSynchPaused();
	static void RadioSynchUnPaused();
	static char GetIsDialogueOnlyRadio();
	static void DisableNPCRadio(TESObjectREFR* apNPCRadio);
	static void SoundCompletionCallback(void* apUnused, bool abSucceeded);
	static FalloutRadio::FORadioStation* InitStation(TESObjectREFR* pWorldRadio, TESObjectREFR* apTargetStation);
	static TESObjectREFR* GetRadioRefrForStation(BGSTalkingActivator* apRequestedStation, TESObjectREFR* pWorldRadio);
	static FalloutRadio::FORadioStation* GetCurrentStation();
	static void Shutdown();
	static bool TunePipboyToStation(TESObjectREFR* apTargetStation, bool abForceStationUpdate);
	static void PipboyUpdate(bool abForceUpdate);
	static void EnableNPCRadio(TESObjectREFR* apNPCRadio, TESObjectREFR* apTargetStation);
	static void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	static void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	static bool GetNextConversation(FalloutRadio::FORadioStation* apStation, TESTopic* apOverrideTopic);

	static inline int32_t iLastRadioUpdate;
	static inline int32_t iRadioState;
	static inline void* pRadioAttenuationModel;
	static inline FORadioStation* pCurrentPipboyStation;
	static inline uint32_t iQueuedState;
	static BSSoundHandle PipboyVoiceHndl;
	static BSSoundHandle PipboyStaticHndl;
	static inline bool bPipboyRadioEnabled;
	static inline bool bShutdown;
	static inline bool bIsDialogueOnlyRadio;
	static inline bool bWasInInterior;
	static inline bool bFirstUpdate = true;
	static inline FalloutRadio::FORadioStation* pLostPipboyStation;
	static inline cLastPipboyFilename_t cLastPipboyFilename{};
	static BSCriticalSection RadioUpdateCriticalSection;
	static inline uint32_t iAvailableStationCount;
	static BSSimpleList<FalloutRadio::FORadioStation*> ActiveStationList;
	static BSSimpleList<FalloutRadio::FORadioReceiver*> ActiveRadioList;
	static BSSimpleList<TESObjectREFR*> DiscoveredStationList;
};

// A queued file task of a radio station, holding its station.
class RadioConvTask : public QueuedFileEntry {
public:
	RadioConvTask(FalloutRadio::FORadioStation* apStation);
	~RadioConvTask() override;

	void Run() override;
	void Finish() override {}
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
	uint8_t GetFileIndex() override;
	virtual void Cancel() {}
	using QueuedFileEntry::Cancel;

	static void operator delete(void* apMemory);

	FalloutRadio::FORadioStation* pStation;
	uint8_t padding34[4];
};

// The game settings FalloutRadio.cpp defines.
extern SettingT<INISettingCollection> fRadioStaticAtOuterRadiusPct;
extern SettingT<INISettingCollection> fRadioDialogMute;
extern SettingT<INISettingCollection> fCreatureRadioMin;
extern SettingT<INISettingCollection> iRadioUpdateInterval;
extern SettingT<INISettingCollection> iRadioStationTimeout;
extern SettingT<INISettingCollection> fCreatureRadioMax;
