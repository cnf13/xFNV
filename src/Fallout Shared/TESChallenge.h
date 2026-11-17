#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESScriptableForm.h"

// The challenge type enum, defined in TESChallengeManager.h.
enum CHALLENGE_TYPE : int32_t;

// A challenge's DATA chunk: its type, threshold, flags, interval and the three type-specific values.
struct CHALLENGE_DATA {
    enum CHALLENGE_FLAGS : uint16_t {
        CHALLENGE_STARTS_LOCKED = 0x1,
    };

    // Byte-swaps the record read from a big-endian file.
    void Endian();

    int32_t nChallengeType;
    int32_t iThreshold;
    // A 2-byte field: Endian swaps it with the engine's 2-byte swap.
    uint16_t nFlags;
    uint8_t padding0A[2];
    int32_t iInterval;
    uint16_t nSpecialDataOne;
    uint16_t nSpecialDataTwo;
    uint16_t nSpecialDataThree;
};

// A challenge's progress: the count so far and its flags, both unsigned.
struct CHALLENGE_PROGRESS {
    uint32_t iProgress;
    uint32_t nProgressFlags;
};

// A challenge form (CHAL).
class TESChallenge : public TESForm,
                     public TESFullName,
                     public TESDescription,
                     public TESScriptableForm,
                     public TESIcon,
                     public BGSMessageIcon {
public:
    TESChallenge();
    ~TESChallenge() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    uint16_t GetSaveSize(uint32_t aiFlags) override;
    void SaveGame(uint32_t aiFlags) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    using TESForm::Save;

    CHALLENGE_TYPE GetChallengeType();
    // Marks the challenge's progress changed (change flag 2).
    void MarkForSave();
    void SetShowZero(bool abVal);
    TESForm* GetFormTwo();
    TESForm* GetFormOne();
    void SetFormOne(TESForm* pForm) {
        pFormOne = pForm;
    }
    void SetFormTwo(TESForm* pForm) {
        pFormTwo = pForm;
    }
    CHALLENGE_DATA* GetData();
    int32_t GetUnlocked();
    // Returns a signed value; callers compare it signed.
    int32_t GetProgressValue() {
        return static_cast<int32_t>(progress.iProgress);
    }
    uint32_t GetStartsLocked();
    int32_t GetCompleted();
    void ClearProgress();
    int32_t GetRecurring();
    int32_t GetSpecialOne();
    int32_t GetSpecialThree();
    void SetCompleted(bool abVal);
    void SetCompletedRecurring(bool abVal);
    int32_t GetRemovedRecurring();
    void IncrementProgress(int32_t iValue);
    void SetUnlocked(bool abVal);
    int32_t GetSpecialTwo();
    bool GetShowZero();
    int32_t GetCompletedRecurring();
    void RemoveRecurringFromChallenge(char a1);

    CHALLENGE_DATA data;
    CHALLENGE_PROGRESS progress;
    TESForm* pFormOne;
    TESForm* pFormTwo;
};

// The game settings TESChallenge.cpp defines.
extern SettingT<GameSettingCollection> sChallengeComplete;
extern SettingT<INISettingCollection> bShowChallengeUpdates;
extern SettingT<GameSettingCollection> sChallengeProgress;
