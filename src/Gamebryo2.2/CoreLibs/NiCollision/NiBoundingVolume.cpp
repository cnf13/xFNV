#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"

// Every volume starts with its type tag, which CreateFromStream reads back.
void NiBoundingVolume::SaveBinary(NiStream& kStream) {
    uint32_t uiType = Type();
    NiStreamSaveBinary(kStream, uiType);
}

NiBoundingVolume::~NiBoundingVolume() {}

// The test table is indexed by the pair of types; a pair without an entry is tested the other way round.
bool NiBoundingVolume::TestIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kVel0,
    const NiBoundingVolume& kBV1,
    const NiPoint3& kVel1
) {
    int32_t iIndex = kBV0.Type() * MAXTYPE_BV;
    iIndex += kBV1.Type();
    NiBVTestIntersectFunction pfnTest = ms_pfnTestFuncTable[iIndex];
    if (pfnTest) {
        return pfnTest(fTime, kBV0, kVel0, kBV1, kVel1);
    }
    iIndex = kBV1.Type() * MAXTYPE_BV;
    iIndex += kBV0.Type();
    return ms_pfnTestFuncTable[iIndex](fTime, kBV1, kVel1, kBV0, kVel0);
}

// The find table is indexed by the pair of types; a pair without an entry is found the other way round, with the
// normals swapped.
bool NiBoundingVolume::FindIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kVel0,
    const NiBoundingVolume& kBV1,
    const NiPoint3& kVel1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    int32_t bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    int32_t iIndex = kBV0.Type() * MAXTYPE_BV;
    iIndex += kBV1.Type();
    NiBVFindIntersectFunction pfnFind = ms_pfnFindFuncTable[iIndex];
    if (pfnFind) {
        return pfnFind(fTime, kBV0, kVel0, kBV1, kVel1, fIntrTime, kIntrPt, bCalcNormals, kNormal0, kNormal1);
    }
    iIndex = kBV1.Type() * MAXTYPE_BV;
    iIndex += kBV0.Type();
    return ms_pfnFindFuncTable[iIndex](
        fTime,
        kBV1,
        kVel1,
        kBV0,
        kVel0,
        fIntrTime,
        kIntrPt,
        bCalcNormals,
        kNormal1,
        kNormal0
    );
}

bool NiBoundingVolume::TestTriIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kVel0,
    const NiPoint3& kVert0,
    const NiPoint3& kVert1,
    const NiPoint3& kVert2,
    const NiPoint3& kVel1
) {
    return ms_pfnTriTestFuncTable[kBV0.Type()](fTime, kBV0, kVel0, kVert0, kVert1, kVert2, kVel1);
}

bool NiBoundingVolume::FindTriIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kVel0,
    const NiPoint3& kVert0,
    const NiPoint3& kVert1,
    const NiPoint3& kVert2,
    const NiPoint3& kVel1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    int32_t bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    return ms_pfnTriFindFuncTable[kBV0.Type()](
        fTime,
        kBV0,
        kVel0,
        kVert0,
        kVert1,
        kVert2,
        kVel1,
        fIntrTime,
        kIntrPt,
        bCalcNormals,
        kNormal0,
        kNormal1
    );
}

NiBoundingVolume* NiBoundingVolume::CreateFromStream(NiStream& kStream) {
    int32_t iType;
    NiStreamLoadBinary(kStream, iType);
    if (iType < 0 || iType >= MAXTYPE_BV) {
        return nullptr;
    }
    return ms_apfnLoaders[iType](kStream);
}

void NiBoundingVolume::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
