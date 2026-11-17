#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A binary stream over a memory buffer (owned, or the caller's when m_bUserMemory is set). MemRead and MemWrite
// (protected in the PDB) are not reconstructed yet.
class NiMemStream : public NiBinaryStream {
public:
	NiMemStream();
	NiMemStream(void* apBuffer, uint32_t auiSize);
	~NiMemStream() override;

	operator bool() const override;
	void Seek(int32_t iNumBytes) override;
	void SetEndianSwap(bool bDoSwap) override;

	void* Str();
	// Writes uiBytes bytes of pvBuffer, growing the buffer, and returns the bytes written.
	uint32_t MemWrite(const void* pBuffer, uint32_t uiBytes);

	// The transfer functions installed in m_pfnRead and m_pfnWrite (protected in the PDB).
	static uint32_t ReadNoSwap(NiBinaryStream* pkThis, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes,
		uint32_t uiNumComponents);
	static uint32_t WriteNoSwap(NiBinaryStream* pkThis, const void* pvBuffer, uint32_t uiBytes,
		uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	static uint32_t ReadAndSwap(NiBinaryStream* pkThis, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes,
		uint32_t uiNumComponents);
	static uint32_t WriteAndSwap(NiBinaryStream* pkThis, const void* pvBuffer, uint32_t uiBytes,
		uint32_t* puiComponentSizes, uint32_t uiNumComponents);

	char* m_pBuffer;
	uint32_t m_uiPos;
	uint32_t m_uiEnd;
	uint32_t m_uiAllocSize;
	bool m_bUserMemory;
	bool m_bFreeze;
};
inline constexpr fnvd::Vtable NiMemStream_vftable{};
