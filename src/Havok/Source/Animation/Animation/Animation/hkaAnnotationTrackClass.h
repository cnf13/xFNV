#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

void cleanupLoadedObjecthkpVehicleDefaultSuspensionWheelSpringSuspensionParameters(void* p);
void cleanupLoadedObjecthkpCharacterProxyCinfo(void* p);
void finishLoadedObjecthkpRagdollLimitsDataAtoms(void* p, int32_t finishing);
void cleanupLoadedObjecthkaSplineCompressedAnimationTrackCompressionParams(void* p);
void cleanupLoadedObjecthkaSplineCompressedAnimationAnimationCompressionParams(void* p);
void cleanupLoadedObjecthkaSkeletonMapperData(void* p);
// The finish function of hkaSkeletonMapperData's type info.
void finishLoadedObjecthkaSkeletonMapperData(void* p, int32_t finishing);
void cleanupLoadedObjecthkpMoppCodeReindexedTerminal(void* p);
void cleanupLoadedObjecthkpMoppCodeCodeInfo(void* p);
void cleanupLoadedObjecthkpWheelConstraintDataAtoms(void* p);
void cleanupLoadedObjecthkpStiffSpringConstraintDataAtoms(void* p);
void finishLoadedObjecthkpStiffSpringConstraintDataAtoms(void* p, int32_t finishing);
void cleanupLoadedObjecthkpSimpleShapePhantomCollisionDetail(void* p);
void cleanupLoadedObjecthkpPulleyConstraintDataAtoms(void* p);
void finishLoadedObjecthkpPulleyConstraintDataAtoms(void* p, int32_t finishing);
void cleanupLoadedObjecthkpPoweredChainDataConstraintInfo(void* p);
void cleanupLoadedObjecthkpRagdollLimitsDataAtoms(void* p);
void finishLoadedObjecthkaDeltaCompressedAnimationQuantizationFormat(void* p, int32_t finishing);
void finishLoadedObjecthkpSimpleContactConstraintDataInfo(void* p, int32_t finishing);
void cleanupLoadedObjecthkaDeltaCompressedAnimationQuantizationFormat(void* p);
void cleanupLoadedObjecthkpSimpleContactConstraintDataInfo(void* p);
void cleanupLoadedObjecthkpRotationalConstraintDataAtoms(void* p);
void cleanupLoadedObjecthkpPrismaticConstraintDataAtoms(void* p);
void finishLoadedObjecthkpWheelConstraintDataAtoms(void* p, int32_t finishing);
void cleanupLoadedObjecthkpStiffSpringChainDataConstraintInfo(void* p);
// Havok SDK signature (hkSerialize.lib, tied by reloc).
const void* getVtablehkMemoryResourceHandle();
// The finish-loading functions of hkMemoryResourceHandle's and hkMemoryResourceContainer's type infos.
void finishLoadedObjecthkMemoryResourceHandle(void* p, int32_t finishing);
void finishLoadedObjecthkMemoryResourceContainer(void* p, int32_t finishing);
void finishLoadedObjecthkaSkeletonLocalFrameOnBone(void* p, int32_t finishing);
void cleanupLoadedObjecthkaSkeletonLocalFrameOnBone(void* p);
// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
const void* getVtablehkaSkeleton();
// Havok SDK signature (hkaAnimation.lib, tied by reloc).
const void* getVtablehkaMeshBinding();
void finishLoadedObjecthkaMeshBindingMapping(void* p, int32_t finishing);
void finishLoadedObjecthkaDeltaCompressedAnimation(void* p, int32_t finishing);
// Havok SDK signature (hkaAnimation.lib, tied by reloc).
const void* getVtablehkaDeltaCompressedAnimation();
// Destroys the hkStringPtr at p.
void cleanupLoadedObjecthkaBone(void* p);
// The finish constructor of hkaBone's string pointer at 0x0.
void finishLoadedObjecthkaBone(void* p, int32_t finishing);
// The finish constructor of hkaAnnotationTrack's string pointer at 0x0.
void finishLoadedObjecthkaAnnotationTrack(void* p, int32_t finishing);
// Destroys the loaded hkaAnnotationTrack in place.
void cleanupLoadedObjecthkaAnnotationTrack(void* p);
void finishLoadedObjecthkaAnnotationTrackAnnotation(void* p, int32_t finishing);
// The type info's cleanup callback for hkaAnnotationTrack::Annotation: its destructor destroys m_text.
void cleanupLoadedObjecthkaAnnotationTrackAnnotation(void* p);
void finishLoadedObjecthkpMoppCode(void* p, int32_t finishing);
// Havok SDK signature (hkpInternal.lib, tied by reloc,target).
const void* getVtablehkpMoppCode();
void finishLoadedObjecthkpWheelConstraintData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc).
const void* getVtablehkpWheelConstraintData();
void finishLoadedObjecthkpStiffSpringConstraintData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpStiffSpringConstraintData();
void finishLoadedObjecthkpRotationalConstraintDataAtoms(void* p, int32_t finishing);
void finishLoadedObjecthkpRotationalConstraintData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc).
const void* getVtablehkpRotationalConstraintData();
void finishLoadedObjecthkpRagdollLimitsData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc).
const void* getVtablehkpRagdollLimitsData();
// The finish-loading constructor of hkpStiffSpringChainData.
void finishLoadedObjecthkpStiffSpringChainData(void* p, int32_t finishing);
void cleanupLoadedObjecthkpRagdollConstraintDataAtoms(void* p);
void finishLoadedObjecthkpRagdollConstraintDataAtoms(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpRagdollConstraintData();
void finishLoadedObjecthkpRagdollConstraintData(void* p, int32_t finishing);
void finishLoadedObjecthkpPulleyConstraintData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpPulleyConstraintData();
void finishLoadedObjecthkpPrismaticConstraintDataAtoms(void* p, int32_t finishing);
void finishLoadedObjecthkpPrismaticConstraintData(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc).
const void* getVtablehkpPrismaticConstraintData();
void cleanupLoadedObjecthkMemoryResourceHandle(void* p);
void cleanupLoadedObjecthkaMeshBinding(void* p);
void finishLoadedObjecthkaMeshBinding(void* p, int32_t finishing);
void cleanupLoadedObjecthkaDeltaCompressedAnimation(void* p);
void cleanupLoadedObjecthkpMoppCode(void* p);
void cleanupLoadedObjecthkpWheelConstraintData(void* p);
void cleanupLoadedObjecthkpStiffSpringConstraintData(void* p);
// Runs ~Mapping on p: frees the m_mapping array.
void cleanupLoadedObjecthkaMeshBindingMapping(void* p);
void cleanupLoadedObjecthkpSimpleShapePhantom(void* p);
void cleanupLoadedObjecthkpRotationalConstraintData(void* p);
void cleanupLoadedObjecthkpRagdollLimitsData(void* p);
void cleanupLoadedObjecthkpRagdollConstraintData(void* p);
void cleanupLoadedObjecthkpPulleyConstraintData(void* p);
void cleanupLoadedObjecthkpPrismaticConstraintData(void* p);
const void* getVtablehkpSimpleShapePhantom();
// The class registry's finish-loaded-object callback.
void finishLoadedObjecthkpSimpleShapePhantom(void* p, int32_t finishing);

extern hkClass hkMemoryResourceHandleExternalLinkClass;
extern hkClass hkaSplineCompressedAnimationTrackCompressionParamsClass;
extern hkClass hkaDeltaCompressedAnimationQuantizationFormatClass;
extern hkClass hkaDeltaCompressedAnimationClass;
extern hkTypeInfo hkaDeltaCompressedAnimationTypeInfo;
extern hkClass hkaAnnotationTrackAnnotationClass;
extern hkClass hkaAnnotationTrackClass;
extern hkClass hkpCharacterProxyCinfoClass;
extern hkClass hkpMoppCodeReindexedTerminalClass;
extern hkClass hkpMoppCodeCodeInfoClass;
extern hkClass hkpMoppCodeClass;
extern hkTypeInfo hkpMoppCodeTypeInfo;
extern hkClass hkpSimpleContactConstraintDataInfoClass;
extern hkClass hkpWheelConstraintDataAtomsClass;
extern hkClass hkpWheelConstraintDataClass;
extern hkTypeInfo hkpWheelConstraintDataTypeInfo;
extern hkClass hkpStiffSpringConstraintDataAtomsClass;
extern hkClass hkpStiffSpringConstraintDataClass;
extern hkTypeInfo hkpStiffSpringConstraintDataTypeInfo;
extern hkClass hkpSimpleShapePhantomCollisionDetailClass;
extern hkClass hkpSimpleShapePhantomClass;
extern hkTypeInfo hkpSimpleShapePhantomTypeInfo;
extern hkClass hkpRotationalConstraintDataAtomsClass;
extern hkClass hkpRotationalConstraintDataClass;
extern hkTypeInfo hkpRotationalConstraintDataTypeInfo;
extern hkClass hkpRagdollLimitsDataAtomsClass;
extern hkClass hkpRagdollLimitsDataClass;
extern hkTypeInfo hkpRagdollLimitsDataTypeInfo;
extern hkClass hkpRagdollConstraintDataAtomsClass;
extern hkClass hkpRagdollConstraintDataClass;
extern hkTypeInfo hkpRagdollConstraintDataTypeInfo;
extern hkClass hkpPulleyConstraintDataAtomsClass;
extern hkClass hkpPulleyConstraintDataClass;
extern hkTypeInfo hkpPulleyConstraintDataTypeInfo;
extern hkClass hkpPrismaticConstraintDataAtomsClass;
extern hkClass hkpPrismaticConstraintDataClass;
extern hkTypeInfo hkpPrismaticConstraintDataTypeInfo;
extern hkClass hkpPoweredChainDataConstraintInfoClass;
