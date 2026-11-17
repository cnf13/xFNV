#pragma once

#include "SDK/FaceGen/binaryFile.h"
#include <string>

class LoadedFile;
class NiFile;

// FaceGen's binary file reader backed by the game's file and archive interfaces.
class BSFaceGenBinaryFile : public FutBinaryFileC {
public:
	~BSFaceGenBinaryFile() override;
	bool read(void* data, uint32_t size, uint32_t num) override;
	virtual bool openForRead(std::string apFileName, bool abCTLFile);
	void close();

	using FutBinaryFileC::openForRead;
	// The hybrid's implicit default constructor, for the INLINED_LOCAL sites that reconstruct the file: the
	// reconstruct runs the destructor (close) first, so pFile and pLoadedFile start null.
	BSFaceGenBinaryFile() : pFile(nullptr), pLoadedFile(nullptr) {}
	BSFaceGenBinaryFile(std::string apString);
	// Reads num items of size bytes of the file's header into data; whether it read them.
	bool readHeader(void* data, uint32_t size, uint32_t num);
	// Moves pFile to offsetFromBeginning through its Seek (vtable +0x14) from
	// NiFile::ms_iSeekSet; false without a pFile (thiscall, ret 4, 0xAC5E90). It hides FutBinaryFileC::seek, as in
	// the PDB.
	bool seek(int32_t offsetFromBeginning);

	NiFile* pFile;
	LoadedFile* pLoadedFile;
public:
	// Reads a2 two-byte items into a1.
	bool readShorts(void* a1, int32_t a2);
};
