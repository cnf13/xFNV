#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

void finishLoadedObjecthkpCompressedMeshShapeChunk(void* p, int32_t finishing);
void cleanupLoadedObjecthkpBallAndSocketConstraintDataAtoms(void* p);
void finishLoadedObjecthkpBallAndSocketConstraintDataAtoms(void* p, int32_t finishing);
void cleanupLoadedObjecthkpSimpleMeshShapeTriangle(void* p);
void finishLoadedObjecthkpAgent1nSector(void* p, int32_t finishing);
void cleanupLoadedObjecthkxVertexDescriptionElementDecl(void* p);
void finishLoadedObjecthkxMaterialTextureStage(void* p, int32_t finishing);
void cleanupLoadedObjecthkPackfileSectionHeader(void* p);
void cleanupLoadedObjecthkSphere(void* p);
void finishLoadedObjecthkpTypedBroadPhaseHandle(void* p, int32_t finishing);
void finishLoadedObjecthkpBroadPhaseHandle(void* p, int32_t finishing);
void cleanupLoadedObjecthkSweptTransform(void* p);
void cleanupLoadedObjecthkpMultiRayShapeRay(void* p);
void cleanupLoadedObjecthkpListShapeChildInfo(void* p);
void cleanupLoadedObjecthkTraceStreamTitle(void* p);
void cleanupLoadedObjecthkMotionState(void* p);
void finishLoadedObjecthkpBallAndSocketConstraintData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpBallAndSocketConstraintData();
void cleanupLoadedObjecthkpTypedBroadPhaseHandle(void* p);
// Havok SDK signature (hkpCollide.lib, tied by reloc).
const void* getVtablehkpSimpleMeshShape();
void finishLoadedObjecthkpMultiRayShape(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpMultiRayShape();
void finishLoadedObjecthkpListShape(void* p, int32_t finishing);
void cleanupLoadedObjecthkpConvexVerticesShapeFourVectors(void* p);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpConvexVerticesShape();
void cleanupLoadedObjecthkpCompressedMeshShapeBigTriangle(void* p);
const void* getCompressedMeshShapeVtable();
void finishLoadedObjecthkpCompressedMeshShapeConvexPiece(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by target).
void cleanupLoadedObjecthkpCompressedMeshShapeChunk(void* a1);
// Havok SDK signature (hkpCollide.lib, tied by target).
void cleanupLoadedObjecthkpCompressedMeshShapeConvexPiece(void* a1);
void cleanupLoadedObjecthkpCdBody(void* p);
void cleanupLoadedObjecthkpBroadPhaseHandle(void* p);
void cleanupLoadedObjecthkpAgent1nSector(void* p);
// Havok SDK signature (hkSceneData.lib, tied by bytes).
void cleanupLoadedObjecthkxVertexDescription(void* a1);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxVertexBuffer();
// The finish function the reflection registers for hkxVertexBuffer.
void finishLoadedObjecthkxVertexBuffer(void* p, int32_t finishing);
void finishLoadedObjecthkxVertexBufferVertexData(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by target).
void cleanupLoadedObjecthkxVertexBufferVertexData(void* a1);
void finishLoadedObjecthkxNodeAnnotationData(void* p, int32_t finishing);
void cleanupLoadedObjecthkxNodeAnnotationData(void* p);
void finishLoadedObjecthkxNode(void* p, int32_t finishing);
void cleanupLoadedObjecthkxMaterialTextureStage(void* p);
void finishLoadedObjecthkxMaterial(void* p, int32_t finishing);
// The reflection finish function of hkxEnvironment::Variable's type info.
void finishLoadedObjecthkxEnvironmentVariable(void* p, int32_t finishing);
// The reflection finish function of hkxEnvironment's type info.
void finishLoadedObjecthkxEnvironment(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc).
const void* getVtablehkxEnvironment();
// hkxEnvironment::Variable's cleanup function: it follows getVtablehkxEnvironment and destroys the two
// hkStringPtr members of p, m_value (+4) then m_name (+0).
void cleanupLoadedObjecthkxEnvironmentVariable(void* p);
void finishLoadedObjecthkxEnumItem(void* p, int32_t finishing);
void cleanupLoadedObjecthkxEnumItem(void* p);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxSparselyAnimatedEnum();
void finishLoadedObjecthkxEnum(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
const void* getVtablehkxEnum();
// The reflection finish function of hkxAttributeGroup's type info.
void finishLoadedObjecthkxAttributeGroup(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by bytes).
void cleanupLoadedObjecthkxAttribute(void* a1);
// Destroys the loaded hkxAttributeGroup in place.
void cleanupLoadedObjecthkxAttributeGroup(void* p);
void cleanupLoadedObjecthkPackfileHeader(void* p);
void finishLoadedObjecthkTraceStream(void* p, int32_t finishing);
// Havok SDK signature (hkBase.lib, tied by reloc,target).
const void* getVtablehkTraceStream();
void cleanupLoadedObjecthkpBallAndSocketConstraintData(void* p);
void cleanupLoadedObjecthkxVertexBuffer(void* p);
void cleanupLoadedObjecthkxEnvironment(void* p);
void finishLoadedObjecthkxSparselyAnimatedEnum(void* p, int32_t finishing);
const void* getVtablehkpListShape();
void finishLoadedObjecthkxVertexDescription(void* p, int32_t finishing);
void finishLoadedObjecthkxAttribute(void* p, int32_t finishing);
void finishLoadedObjecthkMonitorStreamStringMapStringMap(void* p, int32_t finishing);

extern hkClass hkpBallAndSocketConstraintDataAtomsClass;
extern hkClass hkpBallAndSocketConstraintDataClass;
extern hkTypeInfo hkpBallAndSocketConstraintDataTypeInfo;
extern hkClass hkpTypedBroadPhaseHandleClass;
extern hkClass hkpListShapeChildInfoClass;
extern hkClass hkpConvexVerticesShapeFourVectorsClass;
extern hkClass hkpCdBodyClass;
extern hkClass hkpBroadPhaseHandleClass;
extern hkClass hkpAgent1nSectorClass;
extern hkClass hkxVertexDescriptionElementDeclClass;
extern hkClass hkxVertexDescriptionClass;
extern hkClass hkxAttributeGroupClass;
extern hkClass hkPackfileSectionHeaderClass;
extern hkClass hkPackfileHeaderClass;
extern hkClass hkTraceStreamTitleClass;
extern hkClass hkSweptTransformClass;
extern hkClass hkSphereClass;
extern hkClass hkMotionStateClass;
