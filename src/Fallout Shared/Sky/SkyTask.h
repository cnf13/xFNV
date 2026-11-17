#pragma once

#include "BSMain/QueuedFiles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

// A queued load of a sky texture by archive path. The PDB's source module is SkyTextureManager.cpp.
class SkyTask : public QueuedFileEntry {
public:
	SkyTask(NiPointer<NiTexture>* apspTexture, const char* apFilename);
	~SkyTask() override;

	void Run() override;
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
	uint8_t GetFileIndex() override;

	// The exe's deleting destructor frees with ourFree (0x63FA90).
	static void operator delete(void* apMemory);

	// The caller's texture slot (IDA names it pTexture): the constructor (0x63FA10) stores the slot it is given and
	// Run (0x63FAC0) has BSShaderManager::GetTexture fill it; the destructor releases nothing.
	NiPointer<NiTexture>* pspTexture;
};
inline constexpr fnvd::Vtable SkyTask_vftable{};
