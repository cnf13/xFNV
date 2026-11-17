#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

// String samples and their sparse animation times.
class hkxSparselyAnimatedString : public hkReferencedObject {
public:
	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkxSparselyAnimatedString(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_strings(aFlag), m_times(aFlag) {}

	~hkxSparselyAnimatedString() override;

	// Havok SDK signature (hkSceneData.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<hkStringPtr> m_strings;
	hkArray<float> m_times;
};
