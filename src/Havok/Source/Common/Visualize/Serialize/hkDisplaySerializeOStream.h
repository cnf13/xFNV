#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

#include <stdint.h>

class hkDisplayGeometry;
class hkStreamWriter;
class hkTransform;

// The archive the visual debugger writes its display data to: an hkOArchive with writers for vectors, transforms and
// geometry and no members of its own.
class hkDisplaySerializeOStream : public hkOArchive {
public:
	// An archive with no writer (hkOArchive()), safe to destroy: the sites declare the local, then construct it
	// again in place.
	hkDisplaySerializeOStream() = default;
	hkDisplaySerializeOStream(hkArray<char, hkContainerHeapAllocator>& a1);
	hkDisplaySerializeOStream(hkStreamWriter* a1);
	~hkDisplaySerializeOStream() override;

	// Writes v's x, y and z through writeArrayFloat32.
	void writeQuadVector4(const hkVector4& v);
	// Writes the triangle's three vertex indices with write32.
	void writeTriangle(const hkGeometry::Triangle& ti);
	// Havok SDK signature (hkVisualize.lib, tied by reloc).
	void writeTransform(const hkTransform& t);
	// Havok SDK signature (hkVisualize.lib, tied by bytes).
	void writeGeometry(const hkGeometry& g);
	// Havok SDK signature (hkVisualize.lib, tied by reloc).
	void writeDisplayGeometry(hkDisplayGeometry* dg);

	// Havok SDK signature (hkVisualize.lib, tied by bytes,reloc).
	static uint32_t computeBytesRequired(const hkGeometry& a1);
	// Havok SDK signature (hkVisualize.lib, tied by bytes,reloc).
	static uint32_t computeBytesRequired(hkDisplayGeometry* a1);
};
