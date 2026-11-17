#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/hkMeshShape.h"
#include "Reconstruction.hpp"

#include <stdint.h>

void finishLoadedObjecthkxVertexVectorDataChannel(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxVertexVectorDataChannel();
void finishLoadedObjecthkxVertexSelectionChannel(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxVertexSelectionChannel();
void finishLoadedObjecthkxVertexIntDataChannel(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxVertexIntDataChannel();
void finishLoadedObjecthkxVertexFloatDataChannel(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxVertexFloatDataChannel();
void finishLoadedObjecthkxTriangleSelectionChannel(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxTriangleSelectionChannel();
// Havok SDK signature (hkSceneData.lib, tied by bytes,reloc).
const void* getVtablehkxTextureInplace();
// Havok SDK signature (hkSceneData.lib, tied by bytes,reloc).
const void* getVtablehkxTextureFile();
void finishLoadedObjecthkxTextureInplace(void* p, int32_t finishing);
void finishLoadedObjecthkxTextureFile(void* p, int32_t finishing);
void finishLoadedObjecthkxSparselyAnimatedString(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxSparselyAnimatedString();
void finishLoadedObjecthkxSparselyAnimatedInt(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxSparselyAnimatedInt();
void finishLoadedObjecthkxSparselyAnimatedBool(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxSparselyAnimatedBool();
void finishLoadedObjecthkxSkinBinding(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxSkinBinding();
void finishLoadedObjecthkxNodeSelectionSet(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by bytes,reloc).
const void* getVtablehkxNodeSelectionSet();
void cleanupLoadedObjecthkxMesh(void* p);
const void* getVtablehkxMesh();
void finishLoadedObjecthkxMesh(void* p, int32_t finishing);
void finishLoadedObjecthkxMeshUserChannelInfo(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by bytes,reloc).
const void* getVtablehkxMeshUserChannelInfo();
void finishLoadedObjecthkxMeshSection(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxMeshSection();
// Havok SDK signature (hkSceneData.lib, tied by reloc).
const void* getVtablehkxMaterialShaderSet();
void finishLoadedObjecthkxMaterialShaderSet(void* p, int32_t finishing);
// The standalone build's hkxMaterialShaderTypeInfo holds it (hkMemoryMeshShape.cpp, 0xE02140).
void finishLoadedObjecthkxMaterialShader(void* p, int32_t finishing);
void cleanupLoadedObjecthkxVertexVectorDataChannel(void* p);
void cleanupLoadedObjecthkxVertexSelectionChannel(void* p);
void cleanupLoadedObjecthkxVertexIntDataChannel(void* p);
void cleanupLoadedObjecthkxVertexFloatDataChannel(void* p);
void cleanupLoadedObjecthkxTriangleSelectionChannel(void* p);
void cleanupLoadedObjecthkxSparselyAnimatedBool(void* p);
void cleanupLoadedObjecthkxMaterialShaderSet(void* p);
// Havok SDK signature (hkGeometryUtilities.lib, tied by bytes,reloc).
const void* getVtablehkMemoryMeshShape();
// The number of primitives numIndices indices of primitiveType make; 0 for an unknown type.
int32_t calculateNumPrimitives(int32_t primitiveType, int32_t numIndices);

// A mesh shape whose sections and indices are stored in the object.
class hkMemoryMeshShape : public hkMeshShape {
public:
	hkMemoryMeshShape(const hkMeshSectionCinfo* sections, int32_t numSections);

	// The SDK's finish constructor of a loaded object; assumes hkMeshShape's (the SDK's).
	hkMemoryMeshShape(hkFinishLoadedObjectFlag aFlag)
		: hkMeshShape(aFlag), m_sections(aFlag), m_indices16(aFlag), m_indices32(aFlag), m_name(aFlag) {}

	~hkMemoryMeshShape() override;

	const hkClass* getClassType() const override;
	void calcContentStatistics(hkStatisticsCollector* apCollector, const hkClass* cls) const override;
	// The number of sections, m_sections' size.
	int32_t getNumSections() const override;
	void lockSection(int32_t sectionIndex, uint8_t accessFlags, hkMeshSection& sectionOut) const override;
	void unlockSection(const hkMeshSection& section) const override;
	// The name without the string pointer's ownership flag.
	const char* getName() const override;

	static const hkClass& staticClass();

	hkArray<hkMeshSectionCinfo> m_sections;
	hkArray<uint16_t> m_indices16;
	hkArray<uint32_t> m_indices32;
	hkStringPtr m_name;
};

extern hkClass hkMemoryMeshShapeClass;
extern hkTypeInfo hkMemoryMeshShapeTypeInfo;
extern hkClass hkxSparselyAnimatedStringClass;
extern hkTypeInfo hkxSparselyAnimatedStringTypeInfo;
extern hkClass hkxSparselyAnimatedIntClass;
extern hkTypeInfo hkxSparselyAnimatedIntTypeInfo;
extern hkClass hkxSparselyAnimatedBoolClass;
extern hkTypeInfo hkxSparselyAnimatedBoolTypeInfo;
extern hkClass hkxSkinBindingClass;
extern hkTypeInfo hkxSkinBindingTypeInfo;
extern hkClass hkxMeshSectionClass;
extern hkTypeInfo hkxMeshSectionTypeInfo;
extern hkClass hkxMaterialShaderSetClass;
extern hkTypeInfo hkxMaterialShaderSetTypeInfo;
inline constexpr fnvd::Vtable hkMemoryMeshShape_vftable{};
