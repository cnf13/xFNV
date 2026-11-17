#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"

// Animated float attribute values (m_floats) and their interpretation hint (m_hint).
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

extern hkClass hkxAnimatedFloatClass;

extern hkTypeInfo hkxAnimatedFloatTypeInfo;
