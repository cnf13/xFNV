#pragma once

#include <cstdint>

class TESFile;
class TESForm;
class TESGlobal;
class TESObjectREFR;

enum ENUM_COMPARISON_CONDITION : uint32_t;
enum SCRIPT_OUTPUT : int32_t;

// The reference a condition runs on (ConditionItemData::uiRunOn).
enum CONDITIONITEMOBJECT : uint32_t {
    CIO_SELF = 0x0,
    CIO_TARGET = 0x1,
    CIO_REF = 0x2,
    CIO_COMBATTARGET = 0x3,
    CIO_LINKEDREF = 0x4,
    CIO_COUNT = 0x5,
};

// One operand of a condition function: a literal, a number or a pointer.
struct FunctionData {
    union Parameter {
        float fValue;
        uint32_t uiNumber;
        void* pPointer;
    };

    // Swaps the function index and both operands.
    void Endian();

    uint16_t usFunction;
    Parameter uParam[2];
};

// The data of one condition item as it is stored in a plugin or a save.
struct ConditionItemData {
    // Bits of ucType: the flags, and the comparison operator (ENUM_COMPARISON_CONDITION) in the top three bits. The
    // two run-on flags are the old format's way of naming the reference the condition runs on; loading replaces them
    // with uiRunOn.
    enum TYPE_FLAG : uint8_t {
        TYPE_OR = 0x1,
        TYPE_RUN_ON_TARGET = 0x2,
        TYPE_GLOBAL_VALUE = 0x4,
        TYPE_RUN_ON_REFERENCE = 0x8,
        TYPE_FLAGS_MASK = 0x1F,
        TYPE_COMPARISON_MASK = 0xE0,
        TYPE_COMPARISON_POS = 5,
    };

    // The item's comparison value: a literal or a global to read the value from.
    union Comparison {
        float fValue;
        TESGlobal* pGlobal;
    };

    // Swaps the data's values into the machine's byte order.
    void Endian();

    uint8_t ucType;
    Comparison uComparisonValue;
    FunctionData kFunctionData;
    uint32_t uiRunOn;
    TESObjectREFR* pReference;
};

// One test of a condition list; a TESCondition list holds the items.
class TESConditionItem {
public:
    TESConditionItem();

    // Returns the condition function's index, a 16-bit value zero-extended, which callers use whole.
    SCRIPT_OUTPUT GetConditionFunction();
    // True when this item's result is or-ed with the next item's result.
    bool GetOr();
    // True when the comparison value is read from a global rather than stored here.
    bool GetValueIsGlobal();
    // The comparison operator, held in the top three bits of the type byte.
    ENUM_COMPARISON_CONDITION GetComparisonCondition();
    void SetComparisonValueFloat(float afValue);
    // Clears the function's parameters and stores the new function index.
    void SetConditionFunction(uint16_t aiFunc);
    void SetComparisonCondition(ENUM_COMPARISON_CONDITION aiCond);

    void Load(TESFile* apFile);

    // Fills in the new function's default parameters.
    void SetDefaultDataForFunction(SCRIPT_OUTPUT aiFunc);
    float GetComparisonValue();
    void Copy(TESConditionItem* apCopy, TESForm* apOwner);
    // Resolves the reference and the comparison global from their file-local form IDs.
    void InitItem(TESForm* apOwner);
    bool Compare(TESConditionItem* apCompare);
    // Runs the condition on the references its run-on value picks; sets abDispositionFailure for failed
    // disposition tests.
    bool IsTrue(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef, bool& abDispositionFailure);
    // Writes the item's CTDA chunk, with its forms as form IDs.
    void Save();

    // Deleting frees through ourFree.
    static void operator delete(void* apMemory);

    ConditionItemData Data;
};
