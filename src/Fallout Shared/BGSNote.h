#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"

class TESActorBase;
class TESDescription;
class TESQuest;
class TESSound;
class TESTopic;

enum NOTE_TYPE : int32_t {
    NT_SOUND = 0,
    NT_TEXT = 1,
    NT_IMAGE = 2,
    NT_VOICE = 3,
    NT_COUNT = 4,
};

// A note base form (NOTE).
class BGSNote : public TESBoundObject,
                public TESModel,
                public TESFullName,
                public TESIcon,
                public BGSPickupPutdownSounds {
public:
    BGSNote();
    ~BGSNote() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    bool Activate(
        TESObjectREFR* apItemActivated,
        TESObjectREFR* apActionRef,
        bool abIdFlag,
        TESBoundObject* apObjectToGet,
        int32_t aiCount
    ) override;
    using TESForm::LoadGame;
    using TESForm::Revert;
    using TESForm::Save;

    // Whether the player has read the note.
    bool QHasBeenRead();
    // The kind of the note: sound, text, image or voice. cType is sign-extended.
    NOTE_TYPE GetNoteType();
    // Marks the note read or unread.
    void SetHasBeenRead(bool abRead);
    BSSimpleList<TESQuest*>* GetQuestList();
    TESSound* GetNoteSound();
    void SetNoteSound(TESSound* apSound);
    TESActorBase* GetNoteSpeaker();
    void SetNoteSpeaker(TESActorBase* apSpeaker);
    // The image the note shows.
    TESTexture* GetNoteImage();
    // The text the note shows.
    TESDescription* GetNoteText();
    // Sets the kind of the note.
    void SetNoteType(NOTE_TYPE aeVal);
    // Sets the topic the note names.
    void SetNoteTopic(TESTopic* apTopic);
    void SetNoteImage(TESTexture* apTex);
    void SetNoteText(TESDescription* apDesc);
    TESTopic* GetNoteTopic();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    // what the note shows: the sound, text, image or topic its type names
    union {
        TESSound* pNoteSound;
        TESDescription* pNoteText;
        TESTexture* pNoteImage;
        TESTopic* pNoteTopic;
    };

    TESActorBase* pSpeaker;
    BSSimpleList<TESQuest*> OwnerQuests;
    uint8_t cType;
    bool bHasBeenRead;
    uint8_t padding7E[2];
};
