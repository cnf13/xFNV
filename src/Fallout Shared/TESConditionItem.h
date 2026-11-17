#pragma once

#include "Reconstruction.hpp"

enum ENUM_COMPARISON_CONDITION : uint32_t;
enum SCRIPT_OUTPUT : int32_t;
class TESFile;
class TESForm;
class TESGlobal;
class TESObjectREFR;

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

	// The PDB's FUNCTION_DATA::Endian: swaps the function index and both operands (thiscall, 0x681110).
	void Endian();

	uint16_t usFunction;
	Parameter uParam[2];
};

// The data of one condition item as it is stored in a plugin or a save.
struct ConditionItemData {
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

	// The condition function's index: the word at +8 zero-extended into eax (0x5EB68A movzx eax, word ptr), which
	// callers use whole (0x5EB640 cmp eax, 0x46).
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

	// The exe's deleting destructor (0x7B3FA0) frees through ourFree.
	static void operator delete(void* apMemory);

	ConditionItemData Data;
};
