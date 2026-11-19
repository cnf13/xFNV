#include "Gamebryo2.2/CoreLibs/NiCollision/NiUnionBV.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

NiUnionBV::NiUnionBV() : m_kBoundingVolume(2, 1), m_uiWhichObjectIntersect(0) {}

// The copy constructor deliberately leaves m_uiWhichObjectIntersect unset, as the game does.
NiUnionBV::NiUnionBV(const NiUnionBV& kABV) : NiBoundingVolume(), m_kBoundingVolume(0, 1) {
    m_kBoundingVolume.SetSize(kABV.m_kBoundingVolume.m_usSize);
    for (uint32_t i = 0; i < kABV.m_kBoundingVolume.m_usSize; ++i) {
        NiBoundingVolume* pkChildABV = kABV.m_kBoundingVolume.m_pBase[i];
        if (pkChildABV) {
            m_kBoundingVolume.SetAt(i, pkChildABV->Clone());
        }
    }
}

// KEEP: niread: the stream read fills the local count nothing initialised: a short read (malformed data) leaves it
// unwritten, and the exe loops on it (0xE9F05B).
void NiUnionBV::LoadBinary(NiStream& stream) {
    NiBoundingVolume::LoadBinary(stream);
    uint32_t uiCount;
    NiStreamLoadBinary(stream, uiCount);
    for (uint32_t i = 0; i < uiCount; i++) {
        AddBoundingVolume(NiBoundingVolume::CreateFromStream(stream));
    }
}

void NiUnionBV::SaveBinary(NiStream& stream) {
    NiBoundingVolume::SaveBinary(stream);
    uint32_t uiCount = m_kBoundingVolume.GetSize();
    NiStreamSaveBinary(stream, uiCount);
    for (uint32_t i = 0; i < m_kBoundingVolume.GetSize(); i++) {
        NiBoundingVolume* pkABV = m_kBoundingVolume.GetAt(i);
        if (pkABV) {
            pkABV->SaveBinary(stream);
        }
    }
}

// The base frees the array's buffer.
NiUnionBV::~NiUnionBV() {
    RemoveAllBoundingVolumes();
}

// Each member follows the matching member of the model union.
void NiUnionBV::UpdateWorldData(const NiBoundingVolume& kModelABV, const NiTransform& kWorld) {
    const NiUnionBV& kModel = static_cast<const NiUnionBV&>(kModelABV);
    for (uint32_t i = 0; i < m_kBoundingVolume.GetSize(); i++) {
        m_kBoundingVolume.GetAt(i)->UpdateWorldData(*kModel.m_kBoundingVolume.GetAt(i), kWorld);
    }
}

NiBoundingVolume* NiUnionBV::Create() const {
    NiUnionBV* pUnion;
    NiUnionBV* v8;
    NiUnionBV* v2 = static_cast<NiUnionBV*>(MemoryManager_Singleton.Allocate(sizeof(NiUnionBV)));
    uint32_t v3 = 0;
    if (v2) {
        pUnion = ::new (v2) NiUnionBV;
        v8 = pUnion;
    }
    else {
        v8 = nullptr;
        pUnion = nullptr;
    }
    pUnion->m_kBoundingVolume.SetSize(m_kBoundingVolume.m_usSize);
    if (!m_kBoundingVolume.m_usSize) {
        return pUnion;
    }
    do {
        NiBoundingVolume* pBvSrc = m_kBoundingVolume.m_pBase[v3];
        NiBoundingVolume* pBvDst = pBvSrc->Create();
        if (v3 < pUnion->m_kBoundingVolume.m_usSize) {
            if (pBvDst) {
                if (!pUnion->m_kBoundingVolume.m_pBase[v3]) {
                    ++pUnion->m_kBoundingVolume.m_usESize;
                }
            }
            else {
                if (pUnion->m_kBoundingVolume.m_pBase[v3]) {
                    --pUnion->m_kBoundingVolume.m_usESize;
                }
            }
        }
        else {
            pUnion->m_kBoundingVolume.m_usSize = v3 + 1;
            if (pBvDst) {
                ++pUnion->m_kBoundingVolume.m_usESize;
            }
        }
        pUnion->m_kBoundingVolume.m_pBase[v3++] = pBvDst;
    }
    while (v3 < m_kBoundingVolume.m_usSize);
    return v8;
}

NiBoundingVolume* NiUnionBV::Clone() const {
    NiUnionBV* v2 = static_cast<NiUnionBV*>(MemoryManager_Singleton.Allocate(sizeof(NiUnionBV)));
    if (v2) {
        return ::new (v2) NiUnionBV(*this);
    }

    return nullptr;
}

// Replaces the members with fresh copies of the other union's; anything but a union is ignored.
void NiUnionBV::Copy(const NiBoundingVolume& kABV) {
    if (kABV.Type() != UNION_BV) {
        return;
    }
    const NiUnionBV& kOther = static_cast<const NiUnionBV&>(kABV);
    RemoveAllBoundingVolumes();
    m_kBoundingVolume.SetSize(kOther.m_kBoundingVolume.GetSize());
    for (uint32_t i = 0; i < kOther.m_kBoundingVolume.GetSize(); i++) {
        const NiBoundingVolume* pkSource = kOther.m_kBoundingVolume.GetAt(i);
        NiBoundingVolume* pkCopy = pkSource->Create();
        pkCopy->Copy(*pkSource);
        m_kBoundingVolume.SetAt(i, pkCopy);
    }
}

// Same members in the same order.
bool NiUnionBV::operator==(const NiBoundingVolume& kABV) const {
    if (kABV.Type() != UNION_BV) {
        return false;
    }
    const NiUnionBV& kOther = static_cast<const NiUnionBV&>(kABV);
    if (m_kBoundingVolume.GetSize() != kOther.m_kBoundingVolume.GetSize()) {
        return false;
    }
    for (uint32_t i = 0; i < m_kBoundingVolume.GetSize(); i++) {
        if (*m_kBoundingVolume.GetAt(i) != *kOther.m_kBoundingVolume.GetAt(i)) {
            return false;
        }
    }
    return true;
}

bool NiUnionBV::operator!=(const NiBoundingVolume& kABV) const {
    return !(*this == kABV);
}

// The type, then each member's strings under a "   child <i>" prefix (the prefix block is never freed).
void NiUnionBV::AddViewerStrings(const char* pcPrefix, NiTPrimitiveArray<char*>* pkStrings) const {
    size_t stLen = strlen(pcPrefix) + 12;
    char* pcString = static_cast<char*>(_NiMalloc(stLen));
    NiSprintf(pcString, stLen, "%s = UNION_BV", pcPrefix);
    pkStrings->Add(pcString);

    for (uint32_t i = 0; i < m_kBoundingVolume.GetSize(); i++) {
        char* pcChild = static_cast<char*>(_NiMalloc(13));
        NiSprintf(pcChild, 13, "   child %i", i);
        m_kBoundingVolume.GetAt(i)->AddViewerStrings(pcChild, pkStrings);
    }
}

void NiUnionBV::AddBoundingVolume(NiBoundingVolume* pkABV) {
    if (!pkABV) {
        return;
    }
    for (uint32_t i = 0; i < m_kBoundingVolume.GetSize(); i++) {
        if (m_kBoundingVolume.GetAt(i) == pkABV) {
            return;
        }
    }
    m_kBoundingVolume.AddFirstEmpty(pkABV);
}

void NiUnionBV::RemoveAllBoundingVolumes() {
    for (uint32_t i = 0; i < m_kBoundingVolume.GetSize(); i++) {
        delete m_kBoundingVolume.GetAt(i);
    }
    m_kBoundingVolume.RemoveAll();
}

// LoadBinary is called without a null test: on a failed allocation it is called through a null object's vtable, as the
// game does.
NiBoundingVolume* NiUnionBV::CreateFromStream(NiStream& stream) {
    NiUnionBV* pkMemory = static_cast<NiUnionBV*>(MemoryManager_Singleton.Allocate(sizeof(NiUnionBV)));
    NiUnionBV* pkABV = nullptr;
    if (pkMemory) {
        pkABV = ::new (pkMemory) NiUnionBV;
    }
    pkABV->LoadBinary(stream);
    return pkABV;
}

// kABV0 is a union (the find table sends only UNION_BV here); the cast also drops const, since its
// m_uiWhichObjectIntersect is written.
bool NiUnionBV::UnionOtherFindIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    int32_t bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    NiUnionBV& kUnionABV0 = const_cast<NiUnionBV&>(static_cast<const NiUnionBV&>(kABV0));
    for (uint32_t i = 0; i < kUnionABV0.m_kBoundingVolume.m_usSize; ++i) {
        if (FindIntersect(
                fTime, *kUnionABV0.m_kBoundingVolume.m_pBase[i], kV0, kABV1, kV1, fIntrTime, kIntrPt, bCalcNormals,
                kNormal0, kNormal1
            )) {
            kUnionABV0.m_uiWhichObjectIntersect = i;
            return true;
        }
    }
    return false;
}

// kABV0 is a union (the tri find table sends only UNION_BV here); the cast also drops const, since its
// m_uiWhichObjectIntersect is written.
bool NiUnionBV::UnionTriFindIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiPoint3& kVert0,
    const NiPoint3& kVert1,
    const NiPoint3& kVert2,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    int32_t bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    NiUnionBV& kUnionABV0 = const_cast<NiUnionBV&>(static_cast<const NiUnionBV&>(kABV0));
    for (uint32_t i = 0; i < kUnionABV0.m_kBoundingVolume.m_usSize; i++) {
        NiBoundingVolume* pkABV = kUnionABV0.m_kBoundingVolume.m_pBase[i];
        if (FindTriIntersect(
                fTime, *pkABV, kV0, kVert0, kVert1, kVert2, kV1, fIntrTime, kIntrPt, bCalcNormals, kNormal0, kNormal1
            )) {
            kUnionABV0.m_uiWhichObjectIntersect = i;
            return true;
        }
    }
    return false;
}

// The first of kABV0's volumes that intersects kABV1 becomes the union's intersecting object. kABV0 is a union; the
// cast also drops const, since its m_uiWhichObjectIntersect is written.
bool NiUnionBV::UnionOtherTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1
) {
    NiUnionBV& kUnionABV0 = const_cast<NiUnionBV&>(static_cast<const NiUnionBV&>(kABV0));
    for (uint32_t i = 0; i < kUnionABV0.m_kBoundingVolume.GetSize(); i++) {
        if (TestIntersect(fTime, *kUnionABV0.m_kBoundingVolume.GetAt(i), kV0, kABV1, kV1)) {
            kUnionABV0.m_uiWhichObjectIntersect = i;
            return true;
        }
    }
    return false;
}

// The first pair of volumes, one of each union, that intersects becomes both unions' intersecting objects. Both are
// unions: the source's casts, which also drop const.
bool NiUnionBV::UnionUnionTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1
) {
    NiUnionBV& kUnionABV0 = const_cast<NiUnionBV&>(static_cast<const NiUnionBV&>(kABV0));
    NiUnionBV& kUnionABV1 = const_cast<NiUnionBV&>(static_cast<const NiUnionBV&>(kABV1));
    for (uint32_t i = 0; i < kUnionABV0.m_kBoundingVolume.GetSize(); i++) {
        NiBoundingVolume* pkABV0 = kUnionABV0.m_kBoundingVolume.GetAt(i);
        for (uint32_t j = 0; j < kUnionABV1.m_kBoundingVolume.GetSize(); j++) {
            if (TestIntersect(fTime, *pkABV0, kV0, *kUnionABV1.m_kBoundingVolume.GetAt(j), kV1)) {
                kUnionABV0.m_uiWhichObjectIntersect = i;
                kUnionABV1.m_uiWhichObjectIntersect = j;
                return true;
            }
        }
    }
    return false;
}

// The first of kABV0's volumes that intersects the triangle becomes the union's intersecting object. kABV0 is a
// union: the source's cast, which also drops const.
bool NiUnionBV::UnionTriTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiPoint3& kVert0,
    const NiPoint3& kVert1,
    const NiPoint3& kVert2,
    const NiPoint3& kV1
) {
    NiUnionBV& kUnionABV0 = const_cast<NiUnionBV&>(static_cast<const NiUnionBV&>(kABV0));
    for (uint32_t i = 0; i < kUnionABV0.m_kBoundingVolume.GetSize(); i++) {
        if (TestTriIntersect(fTime, *kUnionABV0.m_kBoundingVolume.GetAt(i), kV0, kVert0, kVert1, kVert2, kV1)) {
            kUnionABV0.m_uiWhichObjectIntersect = i;
            return true;
        }
    }
    return false;
}

// Both volumes are unions (the find table sends only UNION_BV pairs here). The casts also drop const, since each
// union's m_uiWhichObjectIntersect is written.
bool NiUnionBV::UnionUnionFindIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    int32_t bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    NiUnionBV& kUnionABV0 = const_cast<NiUnionBV&>(static_cast<const NiUnionBV&>(kABV0));
    NiUnionBV& kUnionABV1 = const_cast<NiUnionBV&>(static_cast<const NiUnionBV&>(kABV1));
    for (uint32_t i = 0; i < kUnionABV0.m_kBoundingVolume.m_usSize; i++) {
        NiBoundingVolume* pkABV0 = kUnionABV0.m_kBoundingVolume.m_pBase[i];
        for (uint32_t j = 0; j < kUnionABV1.m_kBoundingVolume.m_usSize; j++) {
            NiBoundingVolume* pkABV1 = kUnionABV1.m_kBoundingVolume.m_pBase[j];
            if (FindIntersect(
                    fTime, *pkABV0, kV0, *pkABV1, kV1, fIntrTime, kIntrPt, bCalcNormals, kNormal0, kNormal1
                )) {
                kUnionABV0.m_uiWhichObjectIntersect = i;
                kUnionABV1.m_uiWhichObjectIntersect = j;
                return true;
            }
        }
    }
    return false;
}

void NiUnionBV::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
