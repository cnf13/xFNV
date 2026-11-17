#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Reconstruction.hpp"

#include <stdint.h>

template <class Char> class BSStringT;

uint32_t BSFile_RdNoSwap(NiBinaryStream* pThis, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes,
	uint32_t uiNumComponents);
// Inferred type: puiComponentSizes: uint32_t* from 0xaffa43 (passed to NiBinaryStream::DoByteSwap 2)
uint32_t BSFile_ReadAndSwap(NiBinaryStream* pThis, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes,
	uint32_t uiNumComponents);
// Inferred type: apuiComponentSizes: uint32_t* from 0xaffaa8 (passed to NiBinaryStream::DoByteSwap 2)
uint32_t BSFile_WriteAndSwap(NiBinaryStream* apThis, const void* apvBuffer, uint32_t auiBytes,
	uint32_t* apuiComponentSizes, uint32_t auiNumComponents);

// The engine's file stream: a buffered NiFile over a loose file or an archive entry. Only the virtuals the
// reconstructed code calls through the vtable are declared.
class BSFile : public NiFile {
public:
	// An unopened stream with every member cleared.
	BSFile();
	// A file stream for apName, opened only for writing.
	BSFile(const char* apName, NiFile::OpenMode aeMode, uint32_t aiBufferSize, bool abTextMode);
	~BSFile() override;

	// Picks the swapping or plain read and write functions.
	void SetEndianSwap(bool bDoSwap) override;
	// Moves the read position, seeking the underlying file only when it changes.
	void Seek(int32_t aiOffset, int32_t aiWhence) override;
	// Returns the address of cFileName.
	char* GetDevice() override { return pFileName; }
	// iFileSize, asking GetSize first when it is 0.
	uint32_t GetFileSize() const override;

	// Opens the file when it is not open yet; true when it is good.
	virtual bool Open(bool abWaitForOpen, bool abTextMode);
	// Takes over an already open file; true when it is good.
	virtual bool OpenByFilePointer(std::FILE* apFile);
	// The file's length in bytes.
	virtual uint32_t GetSize();
	// The two Read overloads are declared in reverse slot order: the MSVC ABI puts a group of overloads in the
	// vtable last-declared first.
	// The wide-string form of Read (thiscall, ret 8, 0xAFF640; vtable slot 12).
	virtual uint32_t Read(BSStringT<char16_t>& arString, uint32_t aiMaxLength);
	// Reads a line into arString, at most aiMaxLength characters; returns the length read
	// (thiscall, ret 8, 0xAFF7C0; vtable slot 11).
	virtual uint32_t Read(BSStringT<char>& arString, uint32_t aiMaxLength);
	// Reads a line of at most uiMaxBytes into pBuffer, up to cMark (thiscall, ret 0xC,
	// 0xAFFC90; vtable slot 13).
	virtual uint32_t GetLine(char* pBuffer, uint32_t uiMaxBytes, wchar_t cMark);
	// The two Write overloads are declared in reverse slot order, as the Read ones.
	// The wide-string form of Write (thiscall, ret 8, 0xAFFAF0; vtable slot 15).
	virtual uint32_t Write(const BSStringT<char16_t>& arString, bool abBinary);
	// Writes arString, as text or with abBinary as counted binary (thiscall, ret 8, 0xAFFBB0;
	// vtable slot 14).
	virtual uint32_t Write(const BSStringT<char>& arString, bool abBinary);
	// Whether the file named by cFileName exists (_access(name, 0) != -1). The exe calls it directly on a BSFile.
	virtual bool Exist();
	// Reads aiBytes into apBuffer; returns the bytes read (thiscall, ret 8, 0xB00650; vtable slot 17). The exe calls
	// it directly on a BSFile.
	virtual uint32_t ReadF(void* apBuffer, uint32_t aiBytes);
	// Writes aiBytes from apBuffer; returns the bytes written (thiscall, ret 8, 0xAFF950;
	// vtable slot 18).
	virtual uint32_t WriteF(const void* apBuffer, uint32_t aiBytes);

	// IDA name; not in the PDB. The exe's out-of-line copy of NiBinaryStream::Write: writes auiBytes bytes from
	// apBuffer as one-byte components (thiscall, ret 8, 0x473180). Gamebryo's Write takes an unsigned size.
	int32_t WriteHere(const void* apBuffer, uint32_t auiBytes);
	// Sets the good flag from whether the file is open and allocates the buffer.
	void CheckIsGood();
	// Reads a 16-bit value and returns the stream.
	BSFile& operator>>(int16_t& arVal);
	// Reads a 32-bit value and returns the stream.
	BSFile& operator>>(uint32_t& arVal);
	// The read position: iAuxTrueFilePos unless it is -1, else iTrueFilePos.
	uint32_t Tell();
	// The position of the underlying file, iTrueFilePos.
	uint32_t QTrueFilePos();
	// Whether the file is open and usable.
	bool FileIsGood() const;
	// Closes the file.
	void Close();
	char Flush();
	bool ChangeBufferSize(uint32_t aiNewSize);
	_SYSTEMTIME* GetLastSaveTime(_SYSTEMTIME* result);
	// The PC bodies do nothing; AddAuxBuffer returns true.
	bool AddAuxBuffer(uint32_t aiMinIndex, uint32_t aiMaxIndex);
	void RemoveAuxBuffer();

	static void operator delete(void* apMemory);
	// The unswapped write function the stream installs: WriteF on apThis. The last two arguments
	// are unused.
	static uint32_t WrNoSwap(NiBinaryStream* apThis, const void* apBuffer, uint32_t aiBytes, uint32_t* apNothing1,
		uint32_t aiNothing2);

	bool bUseAuxBuffer;
	uint8_t padding31[3];
	char* m_pAuxBuffer;
	int32_t iAuxTrueFilePos;
	uint32_t iAuxBufferMinIndex;
	uint32_t iAuxBufferMaxIndex;
	char pFileName[260];
	uint32_t iResult;
	uint32_t iIOSize;
	uint32_t iTrueFilePos;
	uint32_t iFileSize;
};

inline constexpr fnvd::Vtable BSFile_vftable{};

// IDA names; not in the PDB.
inline int32_t uiTESStandardBufferSize = 65536;
