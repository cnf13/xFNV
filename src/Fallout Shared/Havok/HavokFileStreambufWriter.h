#pragma once

#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

class BSFile;

// Havok's byte sink writing to a BSFile. Its seek and tell do not override the base's (the exe gives the class two
// slots of its own after the base's nine), and its flush is the base's.
class HavokFileStreambufWriter : public hkStreamWriter {
public:
	// The constructor, writing to the file apFilename.
	HavokFileStreambufWriter(const char* apFilename);
	~HavokFileStreambufWriter() override;

	hkBool* isOk(hkBool* arResult) const override;
	int32_t write(const void* apBuf, int32_t aiBytes) override;
	// Different parameter types from the base's (seek) and no const (tell), which is why the exe has them in new
	// slots. The PDB's seek takes (long, hkStreamWriter::SeekWhence); src keeps uint32_t for the whence because the
	// fixed-width rule spells the base's int and this long the same way on Linux, which would turn the method into an
	// override of the base's slot and drop this one.
	virtual hkResult seek(int32_t aiOffset, uint32_t aeWhence);
	virtual int32_t tell();

	// The exe's deleting destructor (0x626250) frees through hkReferencedObject::deleteObject.
	static void operator delete(void* apMemory);

	BSFile* pFile;
	bool bOwned;
	uint8_t padding0D[3];
};
inline constexpr fnvd::Vtable HavokFileStreambufWriter_vftable{};
