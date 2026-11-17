#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkaSkeleton;
class hkxMesh;

// Binds a scene mesh to the skeleton that deforms it: per mesh section, the bone each skin palette entry stands for.
class hkaMeshBinding : public hkReferencedObject {
public:
	// For one mesh section: the skeleton bone of each palette index.
	struct Mapping {
		static const hkClass& staticClass();
		static const hkClass& staticClass_00DFC760();

		hkArray<int16_t> m_mapping;
	};

	hkaMeshBinding();
	hkaMeshBinding(hkFinishLoadedObjectFlag aFlag);
	~hkaMeshBinding() override;

	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	static const hkClass& staticClass();
	static const hkClass& staticClass_00DFC7C0();

	hkRefPtr<hkxMesh> m_mesh;
	hkStringPtr m_originalSkeletonName;
	hkRefPtr<hkaSkeleton> m_skeleton;
	hkArray<Mapping> m_mappings;
	hkArray<hkTransform> m_boneFromSkinMeshTransforms;
};

extern hkTypeInfo hkMemoryResourceHandleTypeInfo;
extern hkTypeInfo hkMemoryResourceContainerTypeInfo;
extern hkClass hkaMeshBindingMappingClass;
extern hkClass hkaMeshBindingClass;
extern hkTypeInfo hkaMeshBindingTypeInfo;
extern hkClass hkpStiffSpringChainDataConstraintInfoClass;
extern hkClass hkpStiffSpringChainDataClass;
extern hkTypeInfo hkpStiffSpringChainDataTypeInfo;
inline constexpr fnvd::Vtable hkaMeshBinding_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaMeshBindingMappingTypeInfo;
