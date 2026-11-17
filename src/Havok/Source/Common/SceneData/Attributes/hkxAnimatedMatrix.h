#pragma once

// The native destructor of m_matrices needs the element type.
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"

// Animated matrix attribute values and their transform-interpretation hint.
class hkxAnimatedMatrix : public hkReferencedObject {
public:
	hkxAnimatedMatrix() = default;
	hkxAnimatedMatrix(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_matrices(aFlag) {}
	~hkxAnimatedMatrix() override;

	// The reflection class hkxAnimatedMatrixClass.
	static const hkClass& staticClass();

	hkArray<hkMatrix4> m_matrices;
	hkEnum<hkxAttribute::Hint, uint8_t> m_hint;
};
inline constexpr fnvd::Vtable hkxAnimatedMatrix_vftable{};

extern hkClass hkxAnimatedMatrixClass;

extern hkTypeInfo hkxAnimatedMatrixTypeInfo;
