#pragma once

#include <cstdint>

// Byte stream with endian-aware typed reads and writes; m_pfnRead/m_pfnWrite do the transfer (swapping or not) and
// m_uiAbsoluteCurrentPos counts the bytes moved.
class NiBinaryStream {
public:
	using NIBINARYSTREAMREADFN = uint32_t (*)(NiBinaryStream* pThis, void* pvBuffer, uint32_t uiBytes,
		uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	using NIBINARYSTREAMWRITEFN = uint32_t (*)(NiBinaryStream* pThis, const void* pvBuffer, uint32_t uiBytes,
		uint32_t* puiComponentSizes, uint32_t uiNumComponents);

	NiBinaryStream();
	virtual ~NiBinaryStream();

	// true while the stream is usable
	virtual operator bool() const = 0;
	virtual void Seek(int32_t iNumBytes) = 0;
	virtual uint32_t GetPosition() const { return m_uiAbsoluteCurrentPos; }
	virtual void SetEndianSwap(bool bDoSwap) = 0;

	// Moves uiTotalBytes made of components of the given sizes (a swapping stream swaps each component) and adds the
	// bytes moved to m_uiAbsoluteCurrentPos; returns them.
	uint32_t BinaryRead(void* pvBuffer, uint32_t uiTotalBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents) {
		uint32_t uiBytesRead = m_pfnRead(this, pvBuffer, uiTotalBytes, puiComponentSizes, uiNumComponents);
		m_uiAbsoluteCurrentPos += uiBytesRead;
		return uiBytesRead;
	}

	uint32_t BinaryWrite(const void* pvBuffer, uint32_t uiTotalBytes, uint32_t* puiComponentSizes,
		uint32_t uiNumComponents) {
		uint32_t uiBytesWritten = m_pfnWrite(this, pvBuffer, uiTotalBytes, puiComponentSizes, uiNumComponents);
		m_uiAbsoluteCurrentPos += uiBytesWritten;
		return uiBytesWritten;
	}

	// Raw bytes: one-byte components, never swapped.
	uint32_t Read(void* pvBuffer, uint32_t uiBytes);

	uint32_t Write(const void* pvBuffer, uint32_t uiBytes) {
		uint32_t uiSize = 1;
		return BinaryWrite(pvBuffer, uiBytes, &uiSize, 1);
	}

	// Reads one text line of at most uiMaxBytes into pBuffer.
	uint32_t GetLine(char* pBuffer, uint32_t uiMaxBytes);

	uint32_t PutS(const char* pBuffer);

	static bool GetEndianMatchHint();
	// Swaps the bytes of each component in pvBuffer.
	static void DoByteSwap(void* pvData, uint32_t uiTotalBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	// Frees through MemoryManager::Deallocate, as the game's deleting destructor does.
	static void operator delete(void* apMemory);

	uint32_t m_uiAbsoluteCurrentPos;
	NIBINARYSTREAMREADFN m_pfnRead;
	NIBINARYSTREAMWRITEFN m_pfnWrite;

	static inline bool ms_bEndianMatchHint;
};

// A free-function copy of Write that takes the stream as its first argument.
uint32_t WriteBytes(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes);
