#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkpSerializedDisplayMarker;

// Havok SDK signature (hkpUtilities.lib, tied by reloc,target).
const void* getVtablehkpSerializedDisplayMarkerList();
void finishLoadedObjecthkpSerializedDisplayMarkerList(void* p, int32_t finishing);
void cleanupLoadedObjecthkpSerializedDisplayMarkerList(void* p);

// Reference-counted display markers saved with a physics snapshot.
class hkpSerializedDisplayMarkerList : public hkReferencedObject {
public:
    hkpSerializedDisplayMarkerList(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_markers(aFlag) {}
    ~hkpSerializedDisplayMarkerList() override;

    int32_t getNumMarkers() const;
    hkpSerializedDisplayMarker* getMarker(int32_t pi) const;
    void addMarker(hkpSerializedDisplayMarker* m);

    // The reflection class hkpSerializedDisplayMarkerListClass.
    static const hkClass& staticClass();

    hkArray<hkpSerializedDisplayMarker*> m_markers;
};

extern hkClass hkpSerializedDisplayMarkerListClass;
extern hkTypeInfo hkpSerializedDisplayMarkerListTypeInfo;
