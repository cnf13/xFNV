#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// A light of a scene (the SDK's members; the exe's hkClass hkxLightClass has them at 0x8, 0x10, 0x20, 0x30 and 0x34
// and an object size of 0x40).
class hkxLight : public hkReferencedObject {
public:
	// Values from the Xbox PDB.
	enum LightType : int32_t {
		POINT_LIGHT = 0,
		DIRECTIONAL_LIGHT = 1,
		SPOT_LIGHT = 2,
	};

	hkxLight() = default;
	hkxLight(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}
	// The exe's slot 0 is hkBaseObject's shared deleting destructor body (0xDE10B0).
	~hkxLight() override;

	// The reflection class hkxLightClass.
	static const hkClass& staticClass();

	hkEnum<LightType, int8_t> m_type;
	hkVector4 m_position;
	hkVector4 m_direction;
	uint32_t m_color;
	float m_angle;
};
inline constexpr fnvd::Vtable hkxLight_vftable{};

extern hkClass hkxLightClass;

extern hkTypeInfo hkxLightTypeInfo;
