#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

// A user channel of a scene mesh section: the indices of the selected edges. It overrides only the destructor.
class hkxEdgeSelectionChannel : public hkReferencedObject {
public:
	hkxEdgeSelectionChannel() = default;
	hkxEdgeSelectionChannel(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_selectedEdges(aFlag) {}
	~hkxEdgeSelectionChannel() override;

	// The reflection class hkxEdgeSelectionChannelClass.
	static const hkClass& staticClass();

	hkArray<int32_t> m_selectedEdges;
};

extern hkClass hkxEdgeSelectionChannelClass;
extern hkTypeInfo hkxEdgeSelectionChannelTypeInfo;

// The member's destructor frees m_selectedEdges.
inline hkxEdgeSelectionChannel::~hkxEdgeSelectionChannel() {}
