#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

void finishLoadedObjecthkpConvexPieceStreamData(void* p, int32_t finishing);
// Havok SDK signature (hkpInternal.lib, tied by reloc,target).
const void* getVtablehkpConvexPieceStreamData();

// Holds the packed convex-piece stream and its triangle index tables.
class hkpConvexPieceStreamData : public hkReferencedObject {
public:
    // The SDK's finish constructor: the loaded arrays stay as loaded.
    hkpConvexPieceStreamData(hkFinishLoadedObjectFlag aFlag)
      : hkReferencedObject(aFlag), m_convexPieceStream(aFlag), m_convexPieceOffsets(aFlag),
        m_convexPieceSingleTriangles(aFlag) {}

    ~hkpConvexPieceStreamData() override;

    // Havok SDK signature (hkpInternal.lib, tied by target).
    static const hkClass& staticClass();

    hkArray<uint32_t, hkContainerHeapAllocator> m_convexPieceStream;
    hkArray<uint32_t, hkContainerHeapAllocator> m_convexPieceOffsets;
    hkArray<uint32_t, hkContainerHeapAllocator> m_convexPieceSingleTriangles;
};

extern hkClass hkpConvexPieceStreamDataClass;
extern hkTypeInfo hkpConvexPieceStreamDataTypeInfo;
