#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"
// The native hkRefPtr<hkxNode> destructor releases the node: the class is complete there.
#include "Havok/Source/Common/SceneData/Graph/hkxNode.h"

#include <stdint.h>

// A named set of scene graph nodes.
class hkxNodeSelectionSet : public hkxAttributeHolder {
public:
	// The reflection's member table (the PDB's hkxNodeSelectionSet::Members; .rdata).
	using MembersArray = const hkClassMember[2];

	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkxNodeSelectionSet(hkFinishLoadedObjectFlag aFlag)
		: hkxAttributeHolder(aFlag), m_selectedNodes(aFlag), m_name(aFlag) {}

	~hkxNodeSelectionSet() override;

	static const hkClass& staticClass();

	hkArray<hkRefPtr<hkxNode>> m_selectedNodes;
	hkStringPtr m_name;

	static MembersArray Members;
};

extern hkClass hkxNodeSelectionSetClass;
extern hkTypeInfo hkxNodeSelectionSetTypeInfo;
inline constexpr fnvd::Vtable hkxNodeSelectionSet_vftable{};
