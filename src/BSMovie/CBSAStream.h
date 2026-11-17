#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSMovie/CAsyncStream.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

class BSFile;

// An async stream over a file the game opened through its archives (a movie in a BSA), guarded by a critical
// section. IDA-only (the PDB has no class of this name).
class CBSAStream : public CAsyncStream {
public:
	CBSAStream(char* a1);
	~CBSAStream() override;

	int32_t Seek(int64_t aiPosition) override;
	int32_t Read(uint8_t* apBuffer, uint32_t auiLength, int32_t abAligned, uint32_t* apBytesRead) override;
	int64_t GetFileSize(int64_t* apSizeAvailable) override;
	uint32_t Alignment() override;
	void Lock() override;
	void Unlock() override;

	// True while pFile is open (not null).
	bool IsOpen();

	static CBSAStream* Construct(void* apMemory, char* a1);

	char cPath[260];
	BSFile* pFile;
	// Construct and Destruct run BSCriticalSection's constructor
	// and destructor on it, and the 0x18 bytes before the 8-aligned ullFileSize hold no more.
	// Lock and Unlock call 0x82F1B0 and 0x82F1F0, copies of the bare Enter/LeaveCriticalSection shared with
	// NiCriticalSection's (0x80 bytes in src, which does not fit here).
	BSCriticalSection kLock;
	// IDA: uint32_t unk124. No function of the class reads or writes it.
	uint8_t padding124[4];
	// IDA's name; not in the PDB. The file's size: GetFileSize (0x82F570) stores pFile->GetFileSize() into it and
	// Seek (0x82F310) compares the position against it as a 64-bit value.
	_LARGE_INTEGER ullFileSize;
	// IDA's name; not in the PDB. The bytes read so far: Construct (0x82F010) clears it, Seek stores the position in
	// it and Read (0x82F3C0) advances it by the length it clamps.
	_LARGE_INTEGER ullBytesRead;
};

// A one-pointer lock guard on the caller's stack: CBSAStream::Read (0x82F3F4) builds one on its kLock. Construct
// (0x82F520, thiscall, ret 4) stores apLock and enters it through 0x82F1B0; Destruct (0x82F550, thiscall, ret 0)
// leaves it through 0x82F1F0.
struct BSCriticalSectionGuard {
	BSCriticalSectionGuard(BSCriticalSection* apLock);
	~BSCriticalSectionGuard();

	BSCriticalSection* pLock;
};

inline constexpr fnvd::Vtable CBSAStream_vftable{};
