#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// A camera of a scene (the SDK's members; the exe's hkClass hkxCameraClass has them at 0x10 to 0x4C and an object size
// of 0x50).
class hkxCamera : public hkReferencedObject {
public:
	hkxCamera() = default;
	hkxCamera(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}
	// The exe's slot 0 is hkBaseObject's shared deleting destructor body (0xDE10B0).
	~hkxCamera() override;

	// The reflection class hkxCameraClass.
	static const hkClass& staticClass();

	hkVector4 m_from;
	hkVector4 m_focus;
	hkVector4 m_up;
	float m_fov;
	float m_far;
	float m_near;
	hkBool m_leftHanded;
};
inline constexpr fnvd::Vtable hkxCamera_vftable{};

extern hkClass hkxCameraClass;

extern hkTypeInfo hkxCameraTypeInfo;
