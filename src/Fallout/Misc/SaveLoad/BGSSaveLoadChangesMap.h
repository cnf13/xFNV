#pragma once

#include "BSCore/MemoryManager.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Fallout/Misc/SaveLoad/BGSUnloadedFormBuffer.h"
#include "Reconstruction.hpp"

class BGSFormChanges;

// The changes a save game records, keyed by form ID: each form's change flags and its unloaded form buffer.
class BGSSaveLoadChangesMap : public NiTPointerMap<uint32_t, BGSFormChanges*> {
public:
	BGSSaveLoadChangesMap();
	~BGSSaveLoadChangesMap() override;

	bool RemoveChanges(uint32_t aiFormID);
	// The changes recorded for aiFormID, or null.
	BGSFormChanges* GetChanges(uint32_t aiFormID);
	// aiFormID's change flags, or none when it has no changes. The exe returns them through a hidden result, so src
	// takes the result pointer apResult and returns it (thiscall, ret 8).
	BGSChangeFlags* GetChangeFlags(BGSChangeFlags* apResult, uint32_t aiFormID);
	// The saved data buffer of aiFormID, or an empty buffer when it has no changes, written to
	// apResult (the exe's hidden result; thiscall, ret 8, 0x845760).
	BGSUnloadedFormBuffer* GetChangeBuffer(BGSUnloadedFormBuffer* apResult, uint32_t aiFormID);
	// Whether aiFormID has any of aiChangeFlags recorded.
	bool CheckChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags);
	// Records aiChangeFlags for aiFormID.
	void AddChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags);
	// Clears aiChangeFlags of aiFormID; whether its entry went away.
	bool RemoveChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags);
	// Sets aiFormID's change flags to aiChangeFlags.
	void SetChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags);
	// Records aiChangeFlags and the saved data akBuffer of the
	// unloaded form aiFormID.
	void SetChanges(uint32_t aiFormID, BGSChangeFlags aiChangeFlags, BGSUnloadedFormBuffer akBuffer);
	void RemoveAllChanges();

	static void operator delete(void* apMemory) { ourFree(apMemory); }
};

// The changes map's base (the PDB's NiTMapBase<NiTPointerAllocator<unsigned int>,unsigned int,BGSFormChanges *>): its
// vtable and the exe's out-of-line constructor and destructor, which write it. Defined in BGSSaveLoadChangesMap.cpp.
using NiTMapBaseUIntBGSFormChanges = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, BGSFormChanges*>;

// What the save game records for one form: which parts changed, and the buffer of its saved data.
class BGSFormChanges {
public:
	BGSFormChanges();
	// The exe's destructor (0x8456C0): frees the unloaded buffer (0x702220).
	~BGSFormChanges();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BGSChangeFlags iChangeFlags;
	BGSUnloadedFormBuffer UnloadedFormBuffer;
};

inline constexpr fnvd::Vtable BGSSaveLoadChangesMap_vftable{};
inline constexpr fnvd::Vtable NiTMapBaseUIntBGSFormChanges_vftable{};
