#pragma once

#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Reconstruction.hpp"

class hkpMoppCode;
class hkpShapeCollection;

// Bethesda's MOPP tree shape over a scaled copy of a MOPP code (bhkMoppBvTreeShape::Init, 0xCCA6F0, builds the
// scaled code and allocates 0x50 bytes for this shape). It keeps a reference to the unscaled code in pOriginalCode:
// the constructor (0xCC9D00) adds one, the destructor (0xD54960) removes it. The exe's vtable overrides only the
// destructor.
class hkScaledMoppBvTreeShape : public hkpMoppBvTreeShape {
public:
	// The constructor over apCode, keeping a reference to the unscaled apOriginal.
	hkScaledMoppBvTreeShape(const hkpShapeCollection* apCollection, const hkpMoppCode* apCode,
		const hkpMoppCode* apOriginal);
	~hkScaledMoppBvTreeShape() override;

	// The exe's deleting destructor returns the block to the Havok heap, of the size in m_memSizeAndFlags.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	const hkpMoppCode* pOriginalCode;
};
inline constexpr fnvd::Vtable hkScaledMoppBvTreeShape_vftable{};
