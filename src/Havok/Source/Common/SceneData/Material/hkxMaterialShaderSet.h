#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Reconstruction.hpp"

class hkxMaterialShader;

// Holds the shaders associated with a scene material.
class hkxMaterialShaderSet : public hkReferencedObject {
public:
	// Both of the exe's default constructors (0xE3F4E0, 0xE4AE50) build the same empty set.
	hkxMaterialShaderSet();
	hkxMaterialShaderSet(hkFinishLoadedObjectFlag flag);
	~hkxMaterialShaderSet() override;

	// The reflection class hkxMaterialShaderSetClass.
	static const hkClass& staticClass();

	hkArray<hkRefPtr<hkxMaterialShader>, hkContainerHeapAllocator> m_shaders;
};
inline constexpr fnvd::Vtable hkxMaterialShaderSet_vftable{};
