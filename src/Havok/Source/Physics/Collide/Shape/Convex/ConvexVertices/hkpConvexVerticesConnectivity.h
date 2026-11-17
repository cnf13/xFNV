#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

// The faces of a convex vertices shape: each face's vertex count, and the vertex indices of all faces in order.
// Only what the reconstructed code uses is declared.
class hkpConvexVerticesConnectivity : public hkReferencedObject {
public:
	// The SDK's finish constructor: a loaded object keeps the arrays the load wrote.
	hkpConvexVerticesConnectivity(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_vertexIndices(aFlag), m_numVerticesPerFace(aFlag) {}

	// Empties both arrays, keeping their storage (vtable slot 3). The PDB returns void; the exe returns 0 in eax
	// (the zero it stores), and this class introduces the slot.
	virtual int32_t clear();
	~hkpConvexVerticesConnectivity() override;

	void addFace(int32_t* indices, int32_t numIndices);
	hkBool* isClosed(hkBool* apResult);

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<uint16_t> m_vertexIndices;
	hkArray<uint8_t> m_numVerticesPerFace;
};
inline constexpr fnvd::Vtable hkpConvexVerticesConnectivity_vftable{};

extern hkClass hkpConvexVerticesConnectivityClass;

extern hkTypeInfo hkpConvexVerticesConnectivityTypeInfo;
