#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BGSLoadFormBuffer;
class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class BGSSaveGameBuffer;
enum QUEUED_SUB_BUFFER_TYPE : uint32_t;
class TESForm;

// A view over a block of saved data (an animation or Havok state) that is loaded into a reference later.
class BGSLoadGameSubBuffer {
public:
	BGSLoadGameSubBuffer();
	BGSLoadGameSubBuffer(char* apBuffer);

	// Loads the saved animation state into the reference apForm.
	void LoadAnimation(TESForm* apForm);
	// Loads the saved Havok state into the reference apForm.
	void LoadHavokData(TESForm* apForm);
	// Saves apForm's animation state into a new buffer.
	void SaveAnimation(TESForm* apForm);
	// Saves apForm's Havok state into a new buffer.
	void SaveHavokData(TESForm* apForm);
	// Saves the sub-buffer's data into apSaveGameBuffer.
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	// The buffer; the exe's shared one-field getter (thiscall, ret 0, 0x559450).
	operator char*();
	void DeleteBuffer();
	// Builds the load buffer for apForm from the saved data and takes the data over (thiscall,
	// ret 4, 0x864E20; the PDB names it BGSLoadFormBuffer::Create).
	BGSLoadFormBuffer* Create(TESForm* apForm);
	void LoadCharController(TESForm* apForm);
	void CopyBuffer(BGSSaveFormBuffer* apSaveGameBuffer);
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

	char* pBuffer;
};

// The sub-buffers queued while a save loads, in three maps of form to sub-buffer, loaded into the forms later.
class BGSSaveLoadQueuedSubBufferMap {
public:
	BGSSaveLoadQueuedSubBufferMap();
	~BGSSaveLoadQueuedSubBufferMap();

	// Loads the queued sub buffers of type aeType.
	void LoadQueuedSubBuffers(QUEUED_SUB_BUFFER_TYPE aeType);
	// Queues apLoadFormBuffer of type aeType for apParentForm.
	void QueueSubBuffer(QUEUED_SUB_BUFFER_TYPE aeType, BGSLoadFormBuffer* apLoadFormBuffer, TESForm* apParentForm);
	void LoadAllQueuedSubBuffers();

	static void operator delete(void* apMemory);

	NiTMap<TESForm*, BGSLoadGameSubBuffer> QueuedSubBuffers[3];
};
