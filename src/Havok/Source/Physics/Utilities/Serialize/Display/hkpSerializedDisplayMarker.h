#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

void cleanupLoadedObjecthkpSerializedDisplayMarker(void* p);
void finishLoadedObjecthkpSerializedDisplayMarker(void* p, int32_t finishing);
// Havok SDK signature (hkpUtilities.lib, tied by reloc,target).
const void* getVtablehkpSerializedDisplayMarker();

// A serialized display marker: a transform (the SDK's member; the exe's hkClass hkpSerializedDisplayMarkerClass has it
// at 0x10 and an object size of 0x50).
class hkpSerializedDisplayMarker : public hkReferencedObject {
public:
	hkpSerializedDisplayMarker(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}
	// The exe's slot 0 is hkBaseObject's shared deleting destructor body (0xDE10B0).
	~hkpSerializedDisplayMarker() override;

	// The reflection class hkpSerializedDisplayMarkerClass.
	static const hkClass& staticClass();

	hkTransform m_transform;
};

inline constexpr fnvd::Vtable hkpSerializedDisplayMarker_vftable{};
extern hkClass hkpSerializedDisplayMarkerClass;
extern hkTypeInfo hkpSerializedDisplayMarkerTypeInfo;
