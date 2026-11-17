#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"

// Animated float attribute values and their interpretation hint (the PDB's and the SDK's members: m_floats at 0x8,
// m_hint at 0x14). The exe's vtable overrides only the deleting destructor.
class hkxAnimatedFloat : public hkReferencedObject {
public:
	hkxAnimatedFloat() = default;
	hkxAnimatedFloat(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_floats(aFlag) {}
	~hkxAnimatedFloat() override;

	// Havok SDK signature (hkSceneData.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<float> m_floats;
	hkEnum<hkxAttribute::Hint, uint8_t> m_hint;
};
inline constexpr fnvd::Vtable hkxAnimatedFloat_vftable{};

extern hkClass hkxAnimatedFloatClass;

extern hkTypeInfo hkxAnimatedFloatTypeInfo;
