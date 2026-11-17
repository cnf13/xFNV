#pragma once

#include "Havok/Source/Common/Base/System/Io/IArchive/hkIArchive.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Reconstruction.hpp"

class hkDisplayGeometry;
class hkStreamReader;
class hkTransform;

// The archive of the visual debugger's stream: hkIArchive with the display objects' readers on top. The exe's class
// adds no data (sizeof 0x10, the hkIArchive base) and has its own vtable 0x10D7DC8, whose slots are the base's.
class hkDisplaySerializeIStream : public hkIArchive {
public:
	hkDisplaySerializeIStream(hkStreamReader* apStream);
	~hkDisplaySerializeIStream() override;

	// x, y and z read as three reals, w cleared.
	void readQuadVector4(hkVector4& v);
	// The rotation and the translation of a transform.
	void readTransform(hkTransform& t);
	// The three 4-byte vertex indices of a triangle.
	void readTriangle(hkGeometry::Triangle& ti);
	// The vertices and triangles of a geometry.
	void readGeometry(hkGeometry& g);
	// A type-tagged display geometry, allocated from the memory router's heap.
	void readDisplayGeometry(hkDisplayGeometry*& dg);
};

inline constexpr fnvd::Vtable hkDisplaySerializeIStream_vftable{};
