#pragma once

#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/LocalFrame/hkLocalFrame.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

void finishLoadedObjecthkaSkeleton(void* p, int32_t finishing);

// An animation skeleton: its name, bone hierarchy, bones, reference pose, float slots and local frames.
class hkaSkeleton : public hkReferencedObject {
public:
	struct LocalFrameOnBone {
		static const hkClass& staticClass();

		hkRefPtr<hkLocalFrame> m_localFrame;
		int32_t m_boneIndex;
	};

	hkaSkeleton() {}

	hkaSkeleton(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_name(aFlag), m_parentIndices(aFlag), m_bones(aFlag), m_referencePose(aFlag),
		  m_floatSlots(aFlag), m_localFrames(aFlag) {}

	~hkaSkeleton() override;

	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	static const hkClass& staticClass();

	hkStringPtr m_name;
	// The parent bone of each bone, -1 for a root.
	hkArray<int16_t> m_parentIndices;
	hkArray<hkaBone> m_bones;
	hkArray<hkQsTransform> m_referencePose;
	hkArray<hkStringPtr> m_floatSlots;
	hkArray<LocalFrameOnBone> m_localFrames;
};

extern hkClass hkaSkeletonLocalFrameOnBoneClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaSkeletonLocalFrameOnBoneTypeInfo;
extern hkClass hkaSkeletonClass;
extern hkTypeInfo hkaSkeletonTypeInfo;
