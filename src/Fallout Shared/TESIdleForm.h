#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/NiFormArray.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModelAnim.h"

struct IDLE_REPLAY_DELAY;
class TESObjectREFR;

enum ANIM_GROUP_SECTION : int32_t;

// An idle's DATA chunk: its anim group section and flags, loop counts, replay delay and extra flags.
struct IDLE_DATA {
    enum IDLE_FLAGS_EX : uint8_t {
        FLAG_NO_ATTACKING = 0x1,
    };

    // Byte-swaps the record read from a big-endian file: only the replay delay is wider than a byte.
    void Endian();

    uint8_t cAnimData;
    uint8_t cLoopMin;
    uint8_t cLoopMax;
    uint16_t sReplayDelay;
    uint8_t cFlagsEx;
};

// An idle's ANAM chunk: the form IDs of its parent and previous idles.
struct IDLE_PARENT_IDS {
    // Byte-swaps the record read from a big-endian file.
    void Endian();

    uint32_t iparentID;
    uint32_t iprevID;
};

// An idle animation form (IDLE).
class TESIdleForm : public TESForm, public TESModelAnim {
public:
    TESIdleForm();
    ~TESIdleForm() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    TESForm* CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    const char* GetFormEditorID() override;
    bool SetFormEditorID(const char* apEditorID) override;

    using TESForm::Save;

    // Whether the idle's conditions hold for the actor and target.
    bool CheckConditions(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef, bool abRecurseUp);
    // Takes the idle out of the idle manager's tree.
    void RemoveFromManager();
    // Removes the child idle at aiIndex.
    void RemoveChildAtIndex(uint32_t aiIndex);
    // Links the idle into its parent's child array (or its root or loose array) after its previous idle.
    bool InitParents();
    // Moves the idle whose previous idle is this one to just after this one in apArray.
    void FixNextIdles(NiFormArray* apArray);
    // Returns &Conditions.
    TESCondition* GetConditions();
    void SetParentIdle(TESIdleForm* apIdle);
    TESIdleForm* GetParentIdle();
    TESIdleForm* GetPrevIdle();
    void SetPrevIdle(TESIdleForm* apIdle);
    // The child array, created on first use.
    NiFormArray* CreateChildArray();
    // The skeleton section the idle plays in, the low six bits of its group flags.
    ANIM_GROUP_SECTION GetIdleAnimGroupSection();
    // Sets the low six bits of the group flags.
    void SetIdleAnimGroupSection(uint8_t aeSection);
    // The number of child idles; 0 without a child array.
    uint32_t GetChildCount();
    TESIdleForm* GetChildByIndex(uint32_t aiIndex);
    uint32_t GetIndexForChild(TESIdleForm* apChild);
    // Whether the idle is a loose idle (group flag 0x40).
    bool GetLooseIdle();
    // Whether the animation file is a .kf file.
    bool CheckIdleFile();
    uint16_t GetReplayDelay();
    uint8_t GetLoops();
    bool GetAllowNoFile();
    void SetLooseIdle(bool abVal);
    bool GetNoAttacking();
    void RemoveChild(TESIdleForm* apChild);
    TESIdleForm* GetIdleToPlay(
        TESObjectREFR* apActionRef,
        TESObjectREFR* apTargetRef,
        BSSimpleList<IDLE_REPLAY_DELAY*>* apReplayDelayList
    );

    TESCondition Conditions;
    IDLE_DATA data;
    NiFormArray* pChildIdles;
    // the parent idle; holds its form ID from Load until InitItem
    TESIdleForm* pParentIdle;
    // the previous idle; holds its form ID from Load until InitItem
    TESIdleForm* pPrevIdle;
    BSStringT<char> strEditorID;
};
