#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Reconstruction.hpp"

// A per-vertex data channel of a scene mesh: one int per vertex (the SDK's member; the exe's hkClass
// hkxVertexIntDataChannelClass has it at 0x8 and an object size of 0x14). It overrides only the destructor.
class hkxVertexIntDataChannel : public hkReferencedObject {
public:
	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkxVertexIntDataChannel(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_perVertexInts(aFlag) {}
	~hkxVertexIntDataChannel() override;

	// The reflection class hkxVertexIntDataChannelClass.
	static const hkClass& staticClass();

	hkArray<int32_t> m_perVertexInts;
};

extern hkClass hkxVertexIntDataChannelClass;
extern hkTypeInfo hkxVertexIntDataChannelTypeInfo;
inline constexpr fnvd::Vtable hkxVertexIntDataChannel_vftable{};

// The member's destructor frees m_perVertexInts.
inline hkxVertexIntDataChannel::~hkxVertexIntDataChannel() {}
