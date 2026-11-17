#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"
#include "Reconstruction.hpp"

// Stores sampled vector values for an animated scene attribute and how to interpret them (Havok SDK; the exe's hkClass
// hkxAnimatedVectorClass has m_hint at 0x14 and an object size of 0x18). Its slot 0 holds hkxAnimatedQuaternion's
// deleting destructor (0xE025A0), which frees one hkArray of 16-byte elements at +0x8; m_hint needs no freeing.
class hkxAnimatedVector : public hkReferencedObject {
public:
	hkxAnimatedVector() = default;
	hkxAnimatedVector(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_vectors(aFlag) {}
	~hkxAnimatedVector() override;

	// The reflection class hkxAnimatedVectorClass.
	static const hkClass& staticClass();

	hkArray<hkVector4, hkContainerHeapAllocator> m_vectors;
	hkEnum<hkxAttribute::Hint, uint8_t> m_hint;
};
inline constexpr fnvd::Vtable hkxAnimatedVector_vftable{};

extern hkClass hkxAnimatedVectorClass;

extern hkTypeInfo hkxAnimatedVectorTypeInfo;
