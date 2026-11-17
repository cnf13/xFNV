#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"

#include <stdint.h>

// Havok SDK signature (hkpCollide.lib, tied by reloc).
const void* getVtablehkpStorageMeshShapeSubpartStorage();
void cleanupLoadedObjecthkpStorageMeshShapeSubpartStorage(void* p);
// The type infos' finish callbacks of SubpartStorage and hkpStorageMeshShape.
void finishLoadedObjecthkpStorageMeshShapeSubpartStorage(void* p, int32_t a1);
void finishLoadedObjecthkpStorageMeshShape(void* p, int32_t finishing);
// The index count of numTriangles triangles given as indicesPerTriangle indices each (1: a strip, 2: pairs, else three
// per triangle); a helper of hkpStorageMeshShape::addSubpart.
int32_t sub_D4B390(int32_t numTriangles, int32_t indicesPerTriangle);

// A mesh shape that owns the memory of its subparts: each subpart added gets a storage object holding copies of its
// vertices, indices and materials.
class hkpStorageMeshShape : public hkpMeshShape {
public:
	// The copies of one subpart's data.
	class SubpartStorage : public hkReferencedObject {
	public:
		SubpartStorage();
		SubpartStorage(int32_t a1);
		// The SDK's finish constructor, which getVtablehkpStorageMeshShapeSubpartStorage builds: the one above with
		// the flag's m_finishing.
		SubpartStorage(hkFinishLoadedObjectFlag flag) : SubpartStorage(flag.m_finishing) {}
		~SubpartStorage() override;

		static const hkClass& staticClass();

		hkArray<float> m_vertices;
		hkArray<uint16_t> m_indices16;
		hkArray<uint32_t> m_indices32;
		hkArray<uint8_t> m_materialIndices;
		hkArray<uint32_t> m_materials;
		hkArray<uint16_t> m_materialIndices16;
	};

	hkpStorageMeshShape(hkpShapeType a1);

	// The SDK's finish constructor, which getVtablehkpStorageMeshShape builds: the one above, whose argument is the
	// flag's m_finishing.
	hkpStorageMeshShape(hkFinishLoadedObjectFlag flag)
		: hkpStorageMeshShape(static_cast<hkpShapeType>(flag.m_finishing)) {}

	hkpStorageMeshShape(float radius, int32_t numbits);
	hkpStorageMeshShape(const hkpMeshShape* mesh);
	~hkpStorageMeshShape() override;

	void addSubpart(const Subpart& partIn) override;

	static const hkClass& staticClass();

	hkArray<SubpartStorage*> m_storage;
};

extern hkClass hkpStorageMeshShapeSubpartStorageClass;
extern hkTypeInfo hkpStorageMeshShapeSubpartStorageTypeInfo;
extern hkClass hkpStorageMeshShapeClass;
extern hkTypeInfo hkpStorageMeshShapeTypeInfo;
