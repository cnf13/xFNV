#pragma once

#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxEnum.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedInt.h"

// Sparse integer samples with their enum-name mapping.
class hkxSparselyAnimatedEnum : public hkxSparselyAnimatedInt {
public:
	hkxSparselyAnimatedEnum() = default;
	hkxSparselyAnimatedEnum(hkFinishLoadedObjectFlag aFlag) : hkxSparselyAnimatedInt(aFlag), m_enum(aFlag) {}
	~hkxSparselyAnimatedEnum() override;

	static const hkClass& staticClass();

	hkRefPtr<hkxEnum> m_enum;
};
inline constexpr fnvd::Vtable hkxSparselyAnimatedEnum_vftable{};
