#pragma once

#include "BSCore/BSSimpleList.h"

class TESConditionItem;
class TESFile;
class TESForm;
class TESObjectREFR;

// The comparison a condition item applies between its two operand values.
enum ENUM_COMPARISON_CONDITION : uint32_t {
    CC_EQUAL = 0,
    CC_NOT_EQUAL = 1,
    CC_GREATER_THAN = 2,
    CC_GREATER_THAN_EQUAL = 3,
    CC_LESS_THAN = 4,
    CC_LESS_THAN_EQUAL = 5,
    CC_COUNT = 6,
};

// A form's condition list: the items are chained with a trailing 'and' operator.
class TESCondition : public BSSimpleList<void*> {
public:
    TESCondition();
    ~TESCondition();

    // The condition item at aiIndex.
    TESConditionItem* GetConditionItem(int32_t aiIndex);
    // Copies the condition items of apCopy, owned by apOwner.
    void Copy(TESCondition* apCopy, TESForm* apOwner);

    // Appends a condition item at the list's tail.
    void AddItem(TESConditionItem* apConditionItem);

    // Runs every item's function and comparison; a null or empty list counts as true.
    bool IsTrue(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef);
    // Also reports a disposition failure through apDispositionFailure and can skip non-essential properties.
    bool IsTrueFull(
        TESObjectREFR* apActionRef,
        TESObjectREFR* apTargetRef,
        bool* apDispositionFailure,
        bool abEssentialPropertiesOnly
    );

    // Reads a condition chunk into a new item at the list's tail.
    void Load(TESFile* apFile);
    // Writes a CTDA chunk per condition item to the form buffer.
    void Save();

    // Resolves the editor-ID references of every item's parameters.
    void InitItem(TESForm* apOwner);

    // Deletes every condition item and empties the list.
    void ClearAllConditionItems();

    // The array deleting destructor
    // BGSEntryPointPerkEntry::ClearData calls on a conditions array with flags 3.
    void* DeleteArray(uint32_t auiFlags);
    // Whether the two lists differ: true at the first differing item or when their lengths differ.
    bool Compare(TESCondition* apCompare);

    // Compares the two values with the comparison's operator; an unordered (NaN) compare
    // reads as true for every operator except CC_EQUAL.
    static bool CheckValue(ENUM_COMPARISON_CONDITION aiCondition, float afVal1, float afVal2);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void operator delete[](void* apMemory) {
        ourFree(apMemory);
    }
};
