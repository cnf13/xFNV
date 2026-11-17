#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

// A user channel of a scene mesh section: the indices of the selected edges (the SDK's member; the exe's hkClass
// hkxEdgeSelectionChannelClass has it at 0x8 and an object size of 0x14). It overrides only the destructor.
class hkxEdgeSelectionChannel : public hkReferencedObject {
public:
	hkxEdgeSelectionChannel() = default;
	hkxEdgeSelectionChannel(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_selectedEdges(aFlag) {}
	~hkxEdgeSelectionChannel() override;

	// The reflection class hkxEdgeSelectionChannelClass.
	static const hkClass& staticClass();

	hkArray<int32_t> m_selectedEdges;
};

inline constexpr fnvd::Vtable hkxEdgeSelectionChannel_vftable{};
extern hkClass hkxEdgeSelectionChannelClass;
extern hkTypeInfo hkxEdgeSelectionChannelTypeInfo;

// The member's destructor frees m_selectedEdges.
inline hkxEdgeSelectionChannel::~hkxEdgeSelectionChannel() {}
