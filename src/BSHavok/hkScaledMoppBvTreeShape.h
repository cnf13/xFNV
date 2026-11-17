#pragma once

#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"

class hkpMoppCode;
class hkpShapeCollection;

// Bethesda's MOPP tree shape over a scaled copy of a MOPP code (bhkMoppBvTreeShape::Init builds the scaled code and
// this shape). It keeps a reference to the unscaled code in pOriginalCode: the constructor adds one, the destructor
// removes it. Only the destructor is overridden.
class hkScaledMoppBvTreeShape : public hkpMoppBvTreeShape {
public:
	// The constructor over apCode, keeping a reference to the unscaled apOriginal.
	hkScaledMoppBvTreeShape(const hkpShapeCollection* apCollection, const hkpMoppCode* apCode,
		const hkpMoppCode* apOriginal);
	~hkScaledMoppBvTreeShape() override;

	// Deleting returns the block to the Havok heap, of the size in m_memSizeAndFlags.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	const hkpMoppCode* pOriginalCode;
};
