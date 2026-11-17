#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

void finishLoadedObjecthkpGenericConstraintData(void* p, int32_t finishing);
void finishLoadedObjecthkpTriangleShape(void* p, int32_t finishing);
void finishLoadedObjecthkpLinearParametricCurve(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpLinearParametricCurve();
void finishLoadedObjecthkpFixedRigidMotion(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpFixedRigidMotion();
void finishLoadedObjecthkpDefaultWorldMemoryWatchDog(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpDefaultWorldMemoryWatchDog();
// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
const void* getVtablehkpConstraintChainInstanceAction();
// The finish-loading constructor of hkpConstraintChainInstanceAction.
void finishLoadedObjecthkpConstraintChainInstanceAction(void* p, int32_t finishing);
// The finish-loading constructor of hkpConstraintChainInstance.
void finishLoadedObjecthkpConstraintChainInstance(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpTriangleShape();
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpTriSampledHeightFieldCollection();
// The class registry's finish callback for hkpTriSampledHeightFieldCollection.
void finishLoadedObjecthkpTriSampledHeightFieldCollection(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpTriSampledHeightFieldBvTreeShape();
void finishLoadedObjecthkpTransformShape(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpTransformShape();
void finishLoadedObjecthkpSphereShape(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpSphereShape();
void finishLoadedObjecthkpShapeInfo(void* p, int32_t finishing);
void finishLoadedObjecthkpRemoveTerminalsMoppModifier(void* p, int32_t finishing);
void finishLoadedObjecthkpFastMeshShape(void* p, int32_t finishing);
void finishLoadedObjecthkpPlaneShape(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpPlaneShape();
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpNullCollisionFilter();
// The type info's finish-loaded-object callback.
void finishLoadedObjecthkpNullCollisionFilter(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpMultiSphereShape();
// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
const void* getVtablehkpGroupFilter();
// The finish-constructor wrapper hkpGroupFilterTypeInfo stores.
void finishLoadedObjecthkpGroupFilter(void* p, int32_t finishing);
void cleanupLoadedObjecthkpLinearParametricCurve(void* p);
void cleanupLoadedObjecthkpFixedRigidMotion(void* p);
void cleanupLoadedObjecthkpDefaultWorldMemoryWatchDog(void* p);
void cleanupLoadedObjecthkpTriSampledHeightFieldCollection(void* p);
void cleanupLoadedObjecthkpTriSampledHeightFieldBvTreeShape(void* p);
// The type info's finish callback of hkpTriSampledHeightFieldBvTreeShape.
void finishLoadedObjecthkpTriSampledHeightFieldBvTreeShape(void* p, int32_t finishing);
void cleanupLoadedObjecthkpShapeInfo(void* p);
void cleanupLoadedObjecthkpRemoveTerminalsMoppModifier(void* p);
void cleanupLoadedObjecthkpPlaneShape(void* p);
void cleanupLoadedObjecthkpNullCollisionFilter(void* p);
void cleanupLoadedObjecthkpMultiSphereShape(void* p);
const void* getVtablehkpMalleableConstraintData();
const void* getVtablehkpGenericConstraintData();
const void* getVtablehkpAabbPhantom();
// The type info's finish-loaded-object callback for hkpAabbPhantom.
void finishLoadedObjecthkpAabbPhantom(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpRemoveTerminalsMoppModifier();

extern hkClass hkpLinearParametricCurveClass;
extern hkTypeInfo hkpLinearParametricCurveTypeInfo;
extern hkClass hkpGenericConstraintDataClass;
extern hkTypeInfo hkpGenericConstraintDataTypeInfo;
extern hkClass hkpFixedRigidMotionClass;
extern hkTypeInfo hkpFixedRigidMotionTypeInfo;
extern hkClass hkpDefaultWorldMemoryWatchDogClass;
extern hkTypeInfo hkpDefaultWorldMemoryWatchDogTypeInfo;
extern hkClass hkpTriSampledHeightFieldCollectionClass;
extern hkTypeInfo hkpTriSampledHeightFieldCollectionTypeInfo;
extern hkClass hkpTriSampledHeightFieldBvTreeShapeClass;
extern hkTypeInfo hkpTriSampledHeightFieldBvTreeShapeTypeInfo;
extern hkClass hkpShapeInfoClass;
extern hkTypeInfo hkpShapeInfoTypeInfo;
extern hkClass hkpRemoveTerminalsMoppModifierClass;
extern hkTypeInfo hkpRemoveTerminalsMoppModifierTypeInfo;
extern hkClass hkpPlaneShapeClass;
extern hkTypeInfo hkpPlaneShapeTypeInfo;
extern hkClass hkpNullCollisionFilterClass;
extern hkTypeInfo hkpNullCollisionFilterTypeInfo;
extern hkClass hkpMultiSphereShapeClass;
extern hkTypeInfo hkpMultiSphereShapeTypeInfo;
