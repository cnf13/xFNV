#pragma once

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/iosfwd.h"

#include <memory>
#include <string>

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class BGSSaveLoadFile;
enum SIZE_TYPE : int32_t;

// VC9's 4-argument _Uninit_move(_First, _Last, _Dest, _Al) for
// strings: hands the work to sub_AD66E0 with the move and checked tags.
std::string* UncheckedUninitializedMove(std::string* _First, std::string* _Last, std::string* _Dest,
	std::allocator<std::string>& _Al);

// The buffer a form's saved data is read into while the form is not loaded.
class BGSUnloadedFormBuffer {
public:
	BGSUnloadedFormBuffer();

	uint8_t GetVersion();
	SIZE_TYPE GetSizeType();
	ENUM_FORM_ID GetFormType();
	// The buffer.
	operator char*() { return pBuffer; }
	// The form's data past the buffer's header, or null without a buffer (IDA name; not in the PDB; thiscall,
	// ret 0, 0x866390).
	char* GetAdvancedBuffer();
	// Writes the buffered form aiFormID with aiChangeFlags to apFile.
	void Save(BGSSaveLoadFile* apFile, uint32_t aiFormID, BGSChangeFlags aiChangeFlags);
	// Reads the form's saved data from apFile into a new buffer.
	void Load(BGSSaveLoadFile* apFile);
	uint32_t GetSize();
	uint32_t GetOverhead();
	BGSLoadFormBuffer* FillLoadFormBuffer();
	// Copies apSaveGameBuffer's form data, type and version into a new buffer.
	void Initialize(BGSSaveFormBuffer* apSaveGameBuffer);
	void Initialize(char* apBuffer, uint32_t aiBufferSize, ENUM_FORM_ID aeFormType, uint8_t acVersion);

	// A second destructor copy: clears the buffer pointer without freeing it (thiscall, ret 0; BGSSaveLoadGame's
	// LoadForm and CleanupExpiredData call it after FillLoadFormBuffer). Both builds call it: the buffer's owner
	// changes, and natively the BGSFormChanges holding it is still destroyed later. Not named in the PDB or IDA; name
	// chosen here.
	static void DestructKeepBuffer(BGSUnloadedFormBuffer* apThis);

	char* pBuffer;
};
