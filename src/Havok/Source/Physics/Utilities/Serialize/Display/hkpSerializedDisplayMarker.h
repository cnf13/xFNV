#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

void cleanupLoadedObjecthkpSerializedDisplayMarker(void* p);
void finishLoadedObjecthkpSerializedDisplayMarker(void* p, int32_t finishing);
// Havok SDK signature (hkpUtilities.lib, tied by reloc,target).
const void* getVtablehkpSerializedDisplayMarker();

// A serialized display marker: a transform.
class hkpSerializedDisplayMarker : public hkReferencedObject {
public:
	hkpSerializedDisplayMarker(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}
	~hkpSerializedDisplayMarker() override;

	// The reflection class hkpSerializedDisplayMarkerClass.
	static const hkClass& staticClass();

	hkTransform m_transform;
};

extern hkClass hkpSerializedDisplayMarkerClass;
extern hkTypeInfo hkpSerializedDisplayMarkerTypeInfo;
