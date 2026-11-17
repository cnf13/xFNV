#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

// Boolean samples and their sparse animation times.
class hkxSparselyAnimatedBool : public hkReferencedObject {
public:
	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkxSparselyAnimatedBool(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_bools(aFlag), m_times(aFlag) {}

	~hkxSparselyAnimatedBool() override;

	// Havok SDK signature (hkSceneData.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<hkBool> m_bools;
	hkArray<float> m_times;
};
inline constexpr fnvd::Vtable hkxSparselyAnimatedBool_vftable{};
