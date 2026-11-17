#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Reconstruction.hpp"

struct hkpShapeRayCastInput;

// Bethesda's triangle shape that also keeps the triangle's normal. Its one slot of its own, castRay, follows
// hkpTriangleShape's.
// IDA puts Normal at 0x58 (size 0x68), ignoring the 16-byte alignment of its vector; hkpTriangleShape is 0x60 here,
// so the normal sits at 0x60 as in the PDB.
class hkNormalTriangleShape : public hkpTriangleShape {
public:
	hkNormalTriangleShape(float afRadius);

	virtual hkBool castRay(const hkpShapeRayCastInput& arInput, hkpShapeRayCastOutput& arResults) const;

	hkVector4 Normal;
};
inline constexpr fnvd::Vtable hkNormalTriangleShape_vftable{};
