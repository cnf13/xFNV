#pragma once

#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Reconstruction.hpp"

class BSFile;

// Havok's byte source reading from a BSFile. Its seek and tell do not override the base's (the exe gives the class
// three slots of its own after the base's twelve: a seek, a tell and the file size), and the rest of the base's slots
// are the base's.
class HavokFileStreambufReader : public hkStreamReader {
public:
	// The exe's constructor (0x626000) does not read the path.
	HavokFileStreambufReader(const char* apPath);
	~HavokFileStreambufReader() override;

	hkBool* isOk(hkBool* arResult) const override;
	int32_t read(void* apBuf, int32_t aiBytes) override;
	// Different parameter types from the base's (seek) and no const (tell), which is why the exe has them in new
	// slots. The PDB's seek takes (long, hkStreamReader::SeekWhence); src keeps uint32_t for the whence because the
	// fixed-width rule spells the base's int and this long the same way on Linux, which would turn the method into an
	// override of the base's slot and drop this one.
	virtual hkResult seek(int32_t aiOffset, uint32_t aeWhence);
	virtual int32_t tell();
	// The size of the file; -1 without one.
	virtual int32_t sysGetFileSize();

	// The exe's deleting destructor (0x626030) frees through hkReferencedObject::deleteObject.
	static void operator delete(void* apMemory);

	BSFile* pFile;
	bool bOwned;
	uint8_t padding0D[3];
};
inline constexpr fnvd::Vtable HavokFileStreambufReader_vftable{};
