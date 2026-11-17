#pragma once

#include <cstdint>

class BGSEntryPointFunctionData;
class TESObjectREFR;

// Allocates and constructs the entry point function data of type a1 (1 one value, 2 two
// values, 3 leveled list, 4 activate choice; else none).
BGSEntryPointFunctionData* CreateEntryPointFunctionObject(int32_t a1);

// The functions a perk entry point can run on a value (set it, add to it, take its absolute value, add a leveled
// list, add an activate choice) and how many arguments each kind takes.
class BGSEntryPointFunction {
public:
    enum ENTRY_POINT_FUNCTION : int32_t {
        NONE = 0,
        SET_VALUE = 1,
        ADD_VALUE = 2,
        MULTIPLY_VALUE = 3,
        ADD_RANGE_TO_VALUE = 4,
        ADD_ACTOR_VALUE_MULT = 5,
        ABSOLUTE_VALUE = 6,
        NEGATIVE_ABSOLUTE_VALUE = 7,
        ADD_LEVELED_LIST_TO_TARGET = 8,
        ADD_ACTIVATE_CHOICE = 9,
        ENTRY_POINT_FUNCTION_COUNT = 10,
    };

    // What kind of data a function works on.
    enum ENTRY_POINT_FUNCTION_TYPE : int32_t {
        VALUE_FUNCTION = 0,
        LEVELED_LIST_FUNCTION = 1,
        ACTIVATE_FUNCTION = 2,
        SPECIAL_FUNCTION = 3,
        ENTRY_POINT_FUNCTION_TYPE_COUNT = 4,
    };

    // One row of the function table: the name, the kind of data and the function itself.
    struct EntryPointFunction {
        // The value functions below are void: the table's pFunction is void, and their one caller, ExecuteFunction,
        // discards the result. AddValue adds the one-value data's value to the float the first argument points at.
        static void AddValue(
            TESObjectREFR* a1,
            BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
            uint8_t arg8,
            void** a4,
            void* apData
        );
        // Multiplies the float the first argument points at by the one-value data's value.
        static void MultiplyValue(
            TESObjectREFR* a1,
            BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
            uint8_t arg8,
            void** a4,
            void* apData
        );
        // Adds a random value between the two values to the float the first argument points at. The data is two-value
        // data: CheckFunctionArguments is passed 2.
        static void AddRangeToValue(
            TESObjectREFR* a1,
            BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
            uint8_t arg8,
            void** a4,
            void* apData
        );
        // Adds the perk owner's actor value (the second value, an index) times the first
        // value to the float the first argument points at. The data is two-value data.
        static void AddActorValueMult(
            TESObjectREFR* a1,
            BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
            uint8_t arg8,
            void** a4,
            void* apData
        );
        // Makes the float the first argument points at its absolute value.
        static void AbsoluteValue(
            TESObjectREFR* a1,
            BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
            uint8_t arg8,
            void** a4,
            void* apData
        );
        // Makes the float the first argument points at its negated absolute value.
        static void NegativeAbsoluteValue(
            TESObjectREFR* a1,
            BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
            uint8_t arg8,
            void** a4,
            void* apData
        );
        // Adds the leveled list's current items to the reference the first argument
        // points at.
        static void AddLeveledList(
            TESObjectREFR* arg0,
            BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
            uint8_t a3,
            void** a4,
            void* apData
        );
        // A pFunction of the table, for ACTIVATE_FUNCTION data: the first argument is the list of choices, the second
        // the reference a choice that runs immediately runs on.
        static void AddActivateChoice(
            TESObjectREFR* apPerkOwner,
            ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
            uint8_t acArgumentCount,
            void** apFunctionArgument,
            void* apData
        );

        const char* pFunctionName;
        ENTRY_POINT_FUNCTION_TYPE eFunctionType;
        void (*pFunction)(
            TESObjectREFR* apPerkOwner,
            ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
            uint8_t acArgumentCount,
            void** apFunctionArgument,
            void* apData
        );
    };

    using EntryPointFunctionTable = EntryPointFunction[ENTRY_POINT_FUNCTION_COUNT];
    using ArgumentCountTable = uint32_t[ENTRY_POINT_FUNCTION_TYPE_COUNT];
    using kEntryPointFunction_t = BGSEntryPointFunction::EntryPointFunction[10];

    // Whether the function may run: the function type is aeExpectedFunctionType, the data is of kind
    // acExpectedFunctionData and the argument count fits.
    static bool CheckFunctionArguments(
        ENTRY_POINT_FUNCTION_TYPE aeExpectedFunctionType,
        uint8_t acExpectedFunctionData,
        TESObjectREFR* apPerkOwner,
        ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
        uint8_t acArgumentCount,
        void** apFunctionArgument,
        void* apData
    );
    // Stores the one-value data's value into the float the first argument points at.
    static void SetValueFunction(
        TESObjectREFR* apPerkOwner,
        ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
        uint8_t acArgumentCount,
        void** apFunctionArgument,
        void* apData
    );
    // Runs the activate choice's script at once on the second argument (a reference) when it runs immediately;
    // otherwise adds the choice to the list the first argument points at.
    static void AddActivateChoiceFunction(
        TESObjectREFR* apPerkOwner,
        ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
        uint8_t acArgumentCount,
        void** apFunctionArgument,
        void* apData
    );
    // Runs function aeFunction of the function table on the arguments, when the table has one.
    static void ExecuteFunction(
        ENTRY_POINT_FUNCTION aeFunction,
        TESObjectREFR* apPerkOwner,
        ENTRY_POINT_FUNCTION_TYPE aeFunctionType,
        uint8_t acArgumentCount,
        void** apFunctionArgument,
        void* apData
    );
    static char GetArgumentCount(int32_t a1);

    // The argument count of each kind of function, by ENTRY_POINT_FUNCTION_TYPE; only the low byte is read.
    static inline ArgumentCountTable EntryPointFunctionTypeArgumentCount{1, 1, 2};
    static inline kEntryPointFunction_t kEntryPointFunction{
        {"Null Function", BGSEntryPointFunction::SPECIAL_FUNCTION, nullptr},
        {"Set Value", BGSEntryPointFunction::VALUE_FUNCTION, &BGSEntryPointFunction::SetValueFunction},
        {"Add Value", BGSEntryPointFunction::VALUE_FUNCTION, &BGSEntryPointFunction::EntryPointFunction::AddValue},
        {"Multiply Value", BGSEntryPointFunction::VALUE_FUNCTION,
         &BGSEntryPointFunction::EntryPointFunction::MultiplyValue},
        {"Add Range To Value", BGSEntryPointFunction::VALUE_FUNCTION,
         &BGSEntryPointFunction::EntryPointFunction::AddRangeToValue},
        {"Add Actor Value Mult", BGSEntryPointFunction::VALUE_FUNCTION,
         &BGSEntryPointFunction::EntryPointFunction::AddActorValueMult},
        {"Absolute Value", BGSEntryPointFunction::VALUE_FUNCTION,
         &BGSEntryPointFunction::EntryPointFunction::AbsoluteValue},
        {"Negative Absolute Value", BGSEntryPointFunction::VALUE_FUNCTION,
         &BGSEntryPointFunction::EntryPointFunction::NegativeAbsoluteValue},
        {"Add Leveled List", BGSEntryPointFunction::LEVELED_LIST_FUNCTION,
         &BGSEntryPointFunction::EntryPointFunction::AddLeveledList},
        {"Add Activate Choice", BGSEntryPointFunction::ACTIVATE_FUNCTION,
         &BGSEntryPointFunction::EntryPointFunction::AddActivateChoice}
    };
};
