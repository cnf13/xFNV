#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxMesh.h"

// Associates a scene mesh with its bone names and bind-pose transforms.
class hkxSkinBinding : public hkReferencedObject {
public:
	// The reflection's member table.
	using MembersArray = const hkClassMember[4];

	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkxSkinBinding(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_mesh(aFlag), m_nodeNames(aFlag), m_bindPose(aFlag) {}

	~hkxSkinBinding() override;

	// Havok SDK signature (hkSceneData.lib, tied by target).
	static const hkClass& staticClass();

	hkRefPtr<hkxMesh> m_mesh;
	hkArray<hkStringPtr, hkContainerHeapAllocator> m_nodeNames;
	hkArray<hkMatrix4, hkContainerHeapAllocator> m_bindPose;
	// 12 bytes of padding: m_initSkinTransform is 16-byte aligned (hkVector4), so it starts at 0x30.
	hkMatrix4 m_initSkinTransform;

	static MembersArray Members;
};
