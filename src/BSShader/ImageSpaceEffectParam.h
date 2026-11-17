#pragma once

#include "BSShader/BSTextureManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Reconstruction.hpp"

// The parameters an image-space effect is rendered with: the textures it borrows from the texture manager and the
// filter mode of each. Subclasses carry the effect's own values.
class ImageSpaceEffectParam {
public:
	ImageSpaceEffectParam();
	virtual ~ImageSpaceEffectParam();

	// The exe's deleting destructor frees the parameters with ourFree.
	static void operator delete(void* apMemory);

	NiTPrimitiveArray<BSTextureManager::eTextureType> pBorrowTextureTypes;
	NiTPrimitiveArray<NiTexturingProperty::FilterMode> pBorrowTextureFilters;
};
inline constexpr fnvd::Vtable ImageSpaceEffectParam_vftable{};
