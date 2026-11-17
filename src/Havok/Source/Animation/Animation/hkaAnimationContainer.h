#pragma once

#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnimationBinding.h"
#include "Havok/Source/Animation/Animation/Deform/Skinning/hkaMeshBinding.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBoneAttachment.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

// Groups the animation, rig and skin resources loaded from an animation packfile.
class hkaAnimationContainer : public hkReferencedObject {
public:
	hkaAnimationContainer(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_skeletons(aFlag), m_animations(aFlag), m_bindings(aFlag), m_attachments(aFlag),
		  m_skins(aFlag) {}

	~hkaAnimationContainer() override;

	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	// Havok SDK signature (hkaAnimation.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<hkRefPtr<hkaSkeleton>, hkContainerHeapAllocator> m_skeletons;
	hkArray<hkRefPtr<hkaAnimation>, hkContainerHeapAllocator> m_animations;
	hkArray<hkRefPtr<hkaAnimationBinding>, hkContainerHeapAllocator> m_bindings;
	hkArray<hkRefPtr<hkaBoneAttachment>, hkContainerHeapAllocator> m_attachments;
	hkArray<hkRefPtr<hkaMeshBinding>, hkContainerHeapAllocator> m_skins;
};

extern hkClass hkaAnimationContainerClass;

extern hkTypeInfo hkaAnimationContainerTypeInfo;
