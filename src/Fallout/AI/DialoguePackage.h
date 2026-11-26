#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/Package.h"
#include "Fallout/AI/MobileObject.h"

class Conversation;
class DialogueItem;
class TESTopic;

// The package of an actor in a dialogue.
class DialoguePackage : public TESPackage {
public:
    DialoguePackage();
    DialoguePackage(
        void* apConversation,
        Actor* apAct,
        MobileObject* apTarg,
        Actor* apTalkActivator,
        bool abCreated,
        bool abUseIdleManager
    );
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
    // Getter of pActor.
    MobileObject* GetActorStartedConversation();
    // Returns pTargetActor.
    MobileObject* GetTargetOfConversation();
    Conversation* GetPackConversation();
    DialogueItem* GetCurrentDialogItem();
    MobileObject* GetTalkingActivator();
    // Stores pConversation.
    void SetPackConversation(Conversation* apConversation);
    // Says the package's current dialogue response.
    void Speak(bool abUseSound);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BSSoundHandle handle;
    TESTopic* ptopic;
    float flistentimer;
    MobileObject* pTalkingActor;
    bool bWaitingforLipFile;
    TESObjectREFR* pTargetLocation;
    bool bUseIdleManager;
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
    int32_t iEmotion;
    int32_t iEmotionValue;
    bool bSoundFile;
};
