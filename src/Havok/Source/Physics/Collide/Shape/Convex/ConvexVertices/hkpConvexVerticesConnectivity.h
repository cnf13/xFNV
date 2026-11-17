#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

// The faces of a convex vertices shape: each face's vertex count, and the vertex indices of all faces in order. Only
// part of the class is declared.
class hkpConvexVerticesConnectivity : public hkReferencedObject {
public:
	// The SDK's finish constructor: a loaded object keeps the arrays the load wrote.
	hkpConvexVerticesConnectivity(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_vertexIndices(aFlag), m_numVerticesPerFace(aFlag) {}

	// Empties both arrays, keeping their storage. Returns 0.
	virtual int32_t clear();
	~hkpConvexVerticesConnectivity() override;

	void addFace(int32_t* indices, int32_t numIndices);
	hkBool* isClosed(hkBool* apResult);

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<uint16_t> m_vertexIndices;
	hkArray<uint8_t> m_numVerticesPerFace;
};

extern hkClass hkpConvexVerticesConnectivityClass;

extern hkTypeInfo hkpConvexVerticesConnectivityTypeInfo;
