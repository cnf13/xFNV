#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

// Stores sampled quaternion values for an animated scene attribute.
class hkxAnimatedQuaternion : public hkReferencedObject {
public:
	hkxAnimatedQuaternion() = default;
	hkxAnimatedQuaternion(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_quaternions(aFlag) {}
	~hkxAnimatedQuaternion() override;

	// The reflection class hkxAnimatedQuaternionClass.
	static const hkClass& staticClass();

	hkArray<hkQuaternion, hkContainerHeapAllocator> m_quaternions;
};

extern hkClass hkxAnimatedQuaternionClass;

extern hkTypeInfo hkxAnimatedQuaternionTypeInfo;
