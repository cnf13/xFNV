#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class FastModifierCollection;

// a value modifier of an actor value
class Modifier {
public:
	Modifier();
	Modifier(char acKey, float afValue);
	Modifier(Modifier* apSource);

	// the actor value the modifier applies to
	char GetKey();
	// the amount added to the actor value
	float GetValue();
	void SetValue(float a1);

	// 0x0 the actor value the modifier applies to
	uint8_t cKey;
	uint8_t padding01[3];
	// 0x4 the amount added to the actor value
	float fValue;
};

// the value modifiers of an actor, kept as a list and, while many are
// active, also as a fast lookup collection
class ModifierList : public BSSimpleList<Modifier*> {
public:
	// How ModifyAndClamp clamps the modified value.
	enum ClampStyle : int32_t {
		LE_ZERO = 0,
		GE_ZERO = 1,
		NO_CLAMP = 2,
	};

	ModifierList();
	~ModifierList();

	// Adds apModifier to the fast collection when its actor value is a fast one, else to the list.
	void AddModifier(Modifier* apModifier);
	// Adds apModifier to the fast collection at acKey.
	void AddFastModifier(char acKey, Modifier* apNew);
	// Removes apModifier from the fast collection when its actor value is a fast one and the collection exists,
	// else from the list, and frees it.
	void DeleteModifier(Modifier* apModifier);
	void DeleteAllModifiers();
	// The bytes SaveGame writes: a 2-byte count and 5 bytes per modifier, list and fast modifiers.
	uint16_t GetSaveSize();

	// the value of the modifier of acKey, 0.0f when none exists
	float GetModifier(char acKey);
	// the value of the modifier of acKey, 0.0f when none exists;
	// abExists reports whether a modifier exists
	float GetModifier(char acKey, bool& abExists);

	// the modifier of acKey, from the fast collection or from the list;
	// private in the engine
	Modifier* GetModifierItem(char acKey);
	// Replaces the modifiers with copies of arList's.
	void operator=(const ModifierList& arList);
	void SetZeroValuesAllowed(bool abZeroValuesAllowed);
	// Sets the modifier of acKey to afValue.
	void SetModifier(char acKey, float afValue);
	void AdjustModifier(char acKey, float afValue, ClampStyle aeClampStyle);
	void Revert();
	bool WriteModifier(Modifier* apModifier);
	void RevertBGS(BGSLoadFormBuffer* apBuffer);
	// Saves the modifier count, then the key byte and the value of each list and fast modifier.
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	// Loads the old-format modifiers from the save.
	void LoadGameTES();
	// Reads what SaveGame writes and adds each modifier.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	void SaveGame_009378D0();

	// afValue plus afModifier, clamped to at most 0 (LE_ZERO) or at least 0 (GE_ZERO).
	static float ModifyAndClamp(float afValue, float afModifier, ClampStyle aeClampStyle);

	// 0x8 modifiers with value zero may be stored
	bool bZeroValuesAllowed;
	// 0xC modifiers held for fast lookup
	FastModifierCollection* pFastModifiers;
};

// The modifiers of an actor held for fast lookup: an array of modifier pointers indexed by actor value.
class FastModifierCollection {
public:
	// Maps each actor value to the index of its modifier in the
	// modifier array (ACTOR_VALUE_COUNT where it has none).
	using uiAVMapping_t = int32_t[77];

	FastModifierCollection();
	FastModifierCollection(FastModifierCollection* a2);
	~FastModifierCollection();

	// Deletes the modifier at auiIndex.
	void DeleteModifier(uint32_t auiIndex);

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// Builds, once, the map from each actor value to its slot in the modifier array: the values flagged for fast
	// modifiers get consecutive slots, the others ACTOR_VALUE_COUNT.
	static void CheckInitializeMapping();

	Modifier** pModifiers;

	static inline int32_t uiModifierCount;
	// Set once the actor value mapping is built. Tests the whole byte; bool would assume 0/1.
	static inline uint8_t bInitialized;
	static inline uiAVMapping_t uiAVMapping{};
};
