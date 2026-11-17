#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "Fallout Shared/Package.h"
#include "Fallout/AI/MobileObject.h"
#include "Reconstruction.hpp"

class Conversation;
class DialogueItem;
class TESTopic;

// The package of an actor in a dialogue.
class DialoguePackage : public TESPackage {
public:
	DialoguePackage();
	DialoguePackage(void* apConversation, Actor* apAct, MobileObject* apTarg, Actor* apTalkActivator, bool abCreated,
		bool abUseIdleManager);
	~DialoguePackage() override;

	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Copy(TESForm* apCopy) override;
	uint16_t GetSaveSize() override;
	void SaveGame() override;
	void LoadGame() override;
	void InitLoadGame() override;
	bool IsPackageOwner(MobileObject* apOwner) override;
	using TESPackage::InitLoadGame;
	using TESPackage::LoadGame;
	using TESPackage::SaveGame;

	void DeleteSoundHandle();
	bool GetContinueMoving();
	void SetContinueMoving(bool abContinueMoving);
	void SetTargetContinueMoving(bool abMove);
	void SetTargetMoveTowardStarter(bool abMove);
	bool IsCreated();
	bool GetTargetContinueMoving();
	bool GetTargetMoveTowardStarter();
	void SetReachedAmbushLocation(bool abFlag);
	bool GetReachedAmbushLocation();
	bool GetShouldHeadTrackTarget();
	// Folded one-field getter of pActor.
	MobileObject* GetActorStartedConversation();
	// Folded one-field getter of pTargetActor.
	MobileObject* GetTargetOfConversation();
	// Folded one-field getter of pConversation; the exe's shared +0xA4 getter it folds with
	// InterfaceManager::GetDebugTextRoot (thiscall, ret 0, 0x5E3FC0).
	Conversation* GetPackConversation();
	// Folded one-field getter of pCurrentItem; the exe's shared +0xA8 getter it folds with
	// TESObjectMISC::GetLoopingSound (thiscall, ret 0, 0x8CDD90).
	DialogueItem* GetCurrentDialogItem();
	// Folded one-field getter of pTalkingActivator.
	MobileObject* GetTalkingActivator();
	// Stores pConversation; the exe calls the engine's shared +0xA4 setter (thiscall, ret 4,
	// 0x994EF0, registered as CombatController::SetCombatPlannerData).
	void SetPackConversation(Conversation* apConversation);
	// Says the package's current dialogue response.
	void Speak(bool abUseSound);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSSoundHandle handle;
	TESTopic* ptopic;
	float flistentimer;
	MobileObject* pTalkingActor;
	bool bWaitingforLipFile;
	uint8_t padding99[0x3];
	TESObjectREFR* pTargetLocation;
	bool bUseIdleManager;
	uint8_t paddingA1[0x3];
	Conversation* pConversation;
	DialogueItem* pCurrentItem;
	DialogueResponse* pCurrentResponse;
	MobileObject* pActor;
	MobileObject* pTargetActor;
	MobileObject* pTalkingActivator;
	bool bActorHeadTrack;
	bool bCreatedPack;
	bool bContinueMoving;
	bool bTargetContinueMoving;
	bool bTargetMoveTowardStarter;
	bool bDoNotControlTargetMovement;
	bool bReachedAmbushLocation;
	uint8_t paddingC3[0x1];
	int32_t iEmotion;
	int32_t iEmotionValue;
	bool bSoundFile;
	uint8_t paddingCD[0x3];
};
inline constexpr fnvd::Vtable DialoguePackage_vftable{};
