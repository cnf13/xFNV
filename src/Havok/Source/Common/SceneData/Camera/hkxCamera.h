#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// A camera of a scene.
class hkxCamera : public hkReferencedObject {
public:
	hkxCamera() = default;
	hkxCamera(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}
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

extern hkClass hkxCameraClass;

extern hkTypeInfo hkxCameraTypeInfo;
