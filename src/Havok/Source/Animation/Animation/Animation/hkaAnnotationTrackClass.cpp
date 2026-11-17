#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"

#include "Havok/Source/Animation/Animation/Animation/DeltaCompressed/hkaDeltaCompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrack.h"
#include "Havok/Source/Animation/Animation/Deform/Skinning/hkaMeshBinding.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperData.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/ResourceDatabase/hkResourceHandle.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Rotational/hkpRotationalConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Wheel/hkpWheelConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/Powered/hkpPoweredChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/RagdollLimits/hkpRagdollLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/StiffSpring/hkpStiffSpringChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Pulley/hkpPulleyConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkaAnnotationTrackAnnotationTypeInfo = {"hkaAnnotationTrackAnnotation",
	finishLoadedObjecthkaAnnotationTrackAnnotation, cleanupLoadedObjecthkaAnnotationTrackAnnotation, nullptr,
	&typeid(hkaAnnotationTrack::Annotation), sizeof(hkaAnnotationTrack::Annotation)};
hkTypeInfo hkaAnnotationTrackTypeInfo = {"hkaAnnotationTrack", finishLoadedObjecthkaAnnotationTrack,
	cleanupLoadedObjecthkaAnnotationTrack, nullptr, &typeid(hkaAnnotationTrack), sizeof(hkaAnnotationTrack)};

using hkaAnnotationTrack_AnnotationClass_MembersArray = const hkClassMember[2];
static hkaAnnotationTrack_AnnotationClass_MembersArray hkaAnnotationTrack_AnnotationClass_Members{
	{"time", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
	{"text", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr}};
using hkaAnnotationTrackClass_MembersArray = const hkClassMember[2];
static hkaAnnotationTrackClass_MembersArray hkaAnnotationTrackClass_Members{
	{"trackName", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
	{"annotations", &hkaAnnotationTrackAnnotationClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x4, nullptr}};

hkClass hkaAnnotationTrackAnnotationClass("hkaAnnotationTrackAnnotation", nullptr, 8, nullptr, 0, nullptr, 0,
	hkaAnnotationTrack_AnnotationClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkaAnnotationTrackClass("hkaAnnotationTrack", nullptr, 0x10, nullptr, 0, nullptr, 0,
	hkaAnnotationTrackClass_Members, 2, nullptr, nullptr, 0, 0);

void cleanupLoadedObjecthkpVehicleDefaultSuspensionWheelSpringSuspensionParameters(void* p) {
	;
}

void cleanupLoadedObjecthkpCharacterProxyCinfo(void* p) {
	;
}

void finishLoadedObjecthkpRagdollLimitsDataAtoms(void* p, int32_t finishing) {
	;
}

void cleanupLoadedObjecthkaSplineCompressedAnimationTrackCompressionParams(void* p) {
	;
}

void cleanupLoadedObjecthkaSplineCompressedAnimationAnimationCompressionParams(void* p) {
	;
}

// Havok SDK: destroys a loaded hkaSkeletonMapperData.
void cleanupLoadedObjecthkaSkeletonMapperData(void* p) {
	static_cast<hkaSkeletonMapperData*>(p)->~hkaSkeletonMapperData();
}

// Havok SDK: finishes a loaded hkaSkeletonMapperData with its finish constructor, when there is one.
void finishLoadedObjecthkaSkeletonMapperData(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkaSkeletonMapperData(flag);
	}
}

void cleanupLoadedObjecthkpMoppCodeReindexedTerminal(void* p) {
	;
}

void cleanupLoadedObjecthkpMoppCodeCodeInfo(void* p) {
	;
}

void cleanupLoadedObjecthkpWheelConstraintDataAtoms(void* p) {
	;
}

void cleanupLoadedObjecthkpStiffSpringConstraintDataAtoms(void* p) {
	;
}

void finishLoadedObjecthkpStiffSpringConstraintDataAtoms(void* p, int32_t finishing) {
	;
}

void cleanupLoadedObjecthkpSimpleShapePhantomCollisionDetail(void* p) {
	;
}

void cleanupLoadedObjecthkpPulleyConstraintDataAtoms(void* p) {
	;
}

void finishLoadedObjecthkpPulleyConstraintDataAtoms(void* p, int32_t finishing) {
	;
}

void cleanupLoadedObjecthkpPoweredChainDataConstraintInfo(void* p) {
	;
}

void cleanupLoadedObjecthkpRagdollLimitsDataAtoms(void* p) {
	;
}

void finishLoadedObjecthkaDeltaCompressedAnimationQuantizationFormat(void* p, int32_t finishing) {
	;
}

void finishLoadedObjecthkpSimpleContactConstraintDataInfo(void* p, int32_t finishing) {
	;
}

void cleanupLoadedObjecthkaDeltaCompressedAnimationQuantizationFormat(void* p) {
	;
}

void cleanupLoadedObjecthkpSimpleContactConstraintDataInfo(void* p) {
	;
}

void cleanupLoadedObjecthkpRotationalConstraintDataAtoms(void* p) {
	;
}

void cleanupLoadedObjecthkpPrismaticConstraintDataAtoms(void* p) {
	;
}

void finishLoadedObjecthkpWheelConstraintDataAtoms(void* p, int32_t finishing) {
	;
}

void cleanupLoadedObjecthkpStiffSpringChainDataConstraintInfo(void* p) {
	;
}

// The reflection's getVtable for hkMemoryResourceHandle: returns the vtable that its finish constructor stores.
const void* getVtablehkMemoryResourceHandle() {
	return hkGetVtableOf<hkMemoryResourceHandle>();
}

// Finishes the object it is given, when there is one.
void finishLoadedObjecthkMemoryResourceHandle(void* p, int32_t finishing) {
	void* apObject = static_cast<void*>(p);
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (apObject) {
		::new (apObject) hkMemoryResourceHandle(aFlag);
	}
}

void finishLoadedObjecthkMemoryResourceContainer(void* p, int32_t finishing) {
	void* apObject = static_cast<void*>(p);
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (apObject) {
		::new (apObject) hkMemoryResourceContainer(aFlag);
	}
}

void finishLoadedObjecthkaSkeletonLocalFrameOnBone(void* p, int32_t finishing) {
	;
}

void cleanupLoadedObjecthkaSkeletonLocalFrameOnBone(void* p) {
	hkReferencedObject** a1 = static_cast<hkReferencedObject**>(p);
	if (*a1) {
		(*a1)->removeReference();
	}
	*a1 = 0;
}

const void* getVtablehkaSkeleton() {
	return hkGetVtableOf<hkaSkeleton>();
}

// The reflection's getVtable for hkaMeshBinding: returns the vtable that its finish constructor (flag 0) stores.
const void* getVtablehkaMeshBinding() {
	return hkGetVtableOf<hkaMeshBinding>();
}

void finishLoadedObjecthkaMeshBindingMapping(void* p, int32_t finishing) {
	;
}

// The reflection's finish function for hkaDeltaCompressedAnimation: its finish constructor.
void finishLoadedObjecthkaDeltaCompressedAnimation(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkaDeltaCompressedAnimation(aFlag);
	}
}

const void* getVtablehkaDeltaCompressedAnimation() {
	return hkGetVtableOf<hkaDeltaCompressedAnimation>();
}

void cleanupLoadedObjecthkaBone(void* p) {
	static_cast<hkStringPtr*>(p)->~hkStringPtr();
}

// The finish constructor of the string pointer at 0x0 of the object it is given.
void finishLoadedObjecthkaBone(void* p, int32_t finishing) {
	if (p) {
		::new (p) hkStringPtr(static_cast<hkFinishLoadedObjectFlag>(finishing));
	}
}

// The finish constructor of the string pointer at 0x0 of the object it is given.
void finishLoadedObjecthkaAnnotationTrack(void* p, int32_t finishing) {
	if (p) {
		::new (p) hkStringPtr(static_cast<hkFinishLoadedObjectFlag>(finishing));
	}
}

// The cleanup function the reflection uses for hkaAnnotationTrack: destroys the loaded track in place.
void cleanupLoadedObjecthkaAnnotationTrack(void* p) {
	static_cast<hkaAnnotationTrack*>(p)->~hkaAnnotationTrack();
}

void finishLoadedObjecthkaAnnotationTrackAnnotation(void* p, int32_t finishing) {
	if (p) {
		::new (static_cast<hkStringPtr*>(p) + 1) hkStringPtr(static_cast<hkFinishLoadedObjectFlag>(finishing));
	}
}

void cleanupLoadedObjecthkaAnnotationTrackAnnotation(void* p) {
	static_cast<hkaAnnotationTrack::Annotation*>(p)->m_text.~hkStringPtr();
}

// The type info's finish-loaded-object callback for hkpMoppCode: runs its finish constructor.
void finishLoadedObjecthkpMoppCode(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkpMoppCode(aFlag);
	}
}

const void* getVtablehkpMoppCode() {
	return hkGetVtableOf<hkpMoppCode>();
}

// The type info's finish-loaded-object callback for hkpWheelConstraintData: runs its finish constructor.
void finishLoadedObjecthkpWheelConstraintData(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkpWheelConstraintData(aFlag);
	}
}

const void* getVtablehkpWheelConstraintData() {
	return hkGetVtableOf<hkpWheelConstraintData>();
}

// The type info's finish-loaded-object callback for hkpStiffSpringConstraintData: runs its finish constructor.
void finishLoadedObjecthkpStiffSpringConstraintData(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkpStiffSpringConstraintData(aFlag);
	}
}

const void* getVtablehkpStiffSpringConstraintData() {
	return hkGetVtableOf<hkpStiffSpringConstraintData>();
}

void finishLoadedObjecthkpRotationalConstraintDataAtoms(void* p, int32_t finishing) {
	;
}

// The type info's finish-loaded-object callback for hkpRotationalConstraintData: runs its finish constructor.
void finishLoadedObjecthkpRotationalConstraintData(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkpRotationalConstraintData(aFlag);
	}
}

const void* getVtablehkpRotationalConstraintData() {
	return hkGetVtableOf<hkpRotationalConstraintData>();
}

// The reflection's finish function for hkpRagdollLimitsData: runs its finish constructor.
void finishLoadedObjecthkpRagdollLimitsData(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkpRagdollLimitsData(aFlag);
	}
}

const void* getVtablehkpRagdollLimitsData() {
	return hkGetVtableOf<hkpRagdollLimitsData>();
}

// The finish constructor of the stiff spring chain data it is given, when there is one.
void finishLoadedObjecthkpStiffSpringChainData(void* p, int32_t finishing) {
	hkpStiffSpringChainData* apData = static_cast<hkpStiffSpringChainData*>(p);
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (apData) {
		::new (apData) hkpStiffSpringChainData(aFlag);
	}
}

void cleanupLoadedObjecthkpRagdollConstraintDataAtoms(void* p) {
	;
}

void finishLoadedObjecthkpRagdollConstraintDataAtoms(void* p, int32_t finishing) {
	;
}

const void* getVtablehkpRagdollConstraintData() {
	return hkGetVtableOf<hkpRagdollConstraintData>();
}

void finishLoadedObjecthkpRagdollConstraintData(void* p, int32_t finishing) {
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (p) {
		::new (p) hkpRagdollConstraintData(aFlag);
	}
}

// The type info's finish-loaded-object callback for hkpPulleyConstraintData: its finish constructor.
void finishLoadedObjecthkpPulleyConstraintData(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkpPulleyConstraintData(aFlag);
	}
}

const void* getVtablehkpPulleyConstraintData() {
	return hkGetVtableOf<hkpPulleyConstraintData>();
}

void finishLoadedObjecthkpPrismaticConstraintDataAtoms(void* p, int32_t finishing) {
	;
}

// The type info's finish-loaded-object callback for hkpPrismaticConstraintData: runs its finish constructor.
void finishLoadedObjecthkpPrismaticConstraintData(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkpPrismaticConstraintData(aFlag);
	}
}

const void* getVtablehkpPrismaticConstraintData() {
	return hkGetVtableOf<hkpPrismaticConstraintData>();
}

void cleanupLoadedObjecthkMemoryResourceHandle(void* p) {
	static_cast<hkMemoryResourceHandle*>(p)->~hkMemoryResourceHandle();
}

void cleanupLoadedObjecthkaMeshBinding(void* p) {
	static_cast<hkaMeshBinding*>(p)->~hkaMeshBinding();
}

// The finish constructor of the mesh binding it is given, when there is one.
void finishLoadedObjecthkaMeshBinding(void* p, int32_t finishing) {
	hkaMeshBinding* apBinding = static_cast<hkaMeshBinding*>(p);
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (apBinding) {
		::new (apBinding) hkaMeshBinding(aFlag);
	}
}

void cleanupLoadedObjecthkaDeltaCompressedAnimation(void* p) {
	static_cast<hkaDeltaCompressedAnimation*>(p)->~hkaDeltaCompressedAnimation();
}

void cleanupLoadedObjecthkpMoppCode(void* p) {
	static_cast<hkpMoppCode*>(p)->~hkpMoppCode();
}

void cleanupLoadedObjecthkpWheelConstraintData(void* p) {
	static_cast<hkpWheelConstraintData*>(p)->~hkpWheelConstraintData();
}

void cleanupLoadedObjecthkpStiffSpringConstraintData(void* p) {
	static_cast<hkpStiffSpringConstraintData*>(p)->~hkpStiffSpringConstraintData();
}

// The reflection's cleanup for hkaMeshBinding::Mapping: runs its destructor, which frees the mapping array's buffer
// unless the array does not own it.
void cleanupLoadedObjecthkaMeshBindingMapping(void* p) {
	hkaMeshBinding::Mapping* pMapping = static_cast<hkaMeshBinding::Mapping*>(p);
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t capacityAndFlags = pMapping->m_mapping.m_capacityAndFlags;
	pMapping->m_mapping.m_size = 0;
	if (capacityAndFlags >= 0) {
		pHeap->bufFree(pMapping->m_mapping.m_data,
			static_cast<int32_t>(sizeof(int16_t) * (capacityAndFlags & hkArrayBase<int16_t>::CAPACITY_MASK)));
	}
	pMapping->m_mapping.m_capacityAndFlags = hkArrayBase<int16_t>::DONT_DEALLOCATE_FLAG;
	pMapping->m_mapping.m_data = 0;
}

void cleanupLoadedObjecthkpSimpleShapePhantom(void* p) {
	static_cast<hkpSimpleShapePhantom*>(p)->~hkpSimpleShapePhantom();
}

void cleanupLoadedObjecthkpRotationalConstraintData(void* p) {
	static_cast<hkpRotationalConstraintData*>(p)->~hkpRotationalConstraintData();
}

void cleanupLoadedObjecthkpRagdollLimitsData(void* p) {
	static_cast<hkpRagdollLimitsData*>(p)->~hkpRagdollLimitsData();
}

void cleanupLoadedObjecthkpRagdollConstraintData(void* p) {
	static_cast<hkpRagdollConstraintData*>(p)->~hkpRagdollConstraintData();
}

void cleanupLoadedObjecthkpPulleyConstraintData(void* p) {
	static_cast<hkpPulleyConstraintData*>(p)->~hkpPulleyConstraintData();
}

void cleanupLoadedObjecthkpPrismaticConstraintData(void* p) {
	static_cast<hkpPrismaticConstraintData*>(p)->~hkpPrismaticConstraintData();
}

// The reflection's getVtable for hkpSimpleShapePhantom: runs its finish constructor (flag 0) on a 16-aligned stack
// object and returns the object's first word, the vtable that constructor stores.
const void* getVtablehkpSimpleShapePhantom() {
	// The class has no native default constructor.
	return hkGetVtableOf<hkpSimpleShapePhantom>();
}

// Finishes the object it is given, when there is one.
void finishLoadedObjecthkpSimpleShapePhantom(void* p, int32_t finishing) {
	void* apObject = static_cast<void*>(p);
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (apObject) {
		::new (apObject) hkpSimpleShapePhantom(aFlag);
	}
}

const hkClass& hkaAnnotationTrack::Annotation::staticClass() {
	return hkaAnnotationTrackAnnotationClass;
}

hkaAnnotationTrack::hkaAnnotationTrack() : m_trackName(), m_annotations() {}

// The annotations' texts, the array's storage and the track name go with the members.
hkaAnnotationTrack::~hkaAnnotationTrack() {}

const hkClass& hkaAnnotationTrack::staticClass() {
	return hkaAnnotationTrackClass;
}

// The finish constructor of a pair of string pointers: constructs both, at 0x0 and 0x4 of the object it is given.
void finishLoadedObjecthkMemoryResourceHandleExternalLink(void* p, int32_t finishing) {
	if (p) {
		hkStringPtr* pStrings = static_cast<hkStringPtr*>(p);
		::new (pStrings) hkStringPtr(static_cast<hkFinishLoadedObjectFlag>(finishing));
		::new (pStrings + 1) hkStringPtr(static_cast<hkFinishLoadedObjectFlag>(finishing));
	}
}

const hkClass& hkMemoryResourceHandle::ExternalLink::staticClass() {
	return hkMemoryResourceHandleExternalLinkClass;
}

hkMemoryResourceHandle::hkMemoryResourceHandle(hkFinishLoadedObjectFlag aFlag)
	: hkResourceHandle(aFlag), m_variant(aFlag), m_name(aFlag), m_references(aFlag) {}

hkMemoryResourceContainer::hkMemoryResourceContainer(hkFinishLoadedObjectFlag flag)
	: hkResourceContainer(flag), m_name(flag), m_resourceHandles(flag), m_children(flag) {
	if (flag.m_finishing) {
		for (int32_t i = 0; i < m_children.m_size; ++i) {
			m_children.m_data[i]->m_parent = this;
		}
	}
}

const hkClass& hkaDeltaCompressedAnimation::QuantizationFormat::staticClass() {
	return hkaDeltaCompressedAnimationQuantizationFormatClass;
}

// hkpSimpleShapePhantom's finish constructor: hkpShapePhantom's, and the collision details left empty.
hkpSimpleShapePhantom::hkpSimpleShapePhantom(hkFinishLoadedObjectFlag flag) : hkpShapePhantom(flag) {}

// The finish function the reflection registers for the class: finishes the loaded object with its finish constructor,
// when the object is not null.
void finishLoadedObjecthkpPoweredChainData(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpPoweredChainData(flag);
	}
}

hkpPoweredChainData::hkpPoweredChainData(hkFinishLoadedObjectFlag flag) : hkpConstraintChainData(flag), m_infos(flag) {
	if (flag.m_finishing) {
		m_atoms.m_bridgeAtom.init(m_atoms.m_bridgeAtom.m_constraintData);
		m_atoms.m_bridgeAtom.init(this);
	}
}
