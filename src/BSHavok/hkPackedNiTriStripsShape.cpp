#include "BSHavok/hkPackedNiTriStripsShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkPackedNiTriStripsShape.h"
#include "BSHavok/hkNormalTriangleShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <bit>
#include <xmmintrin.h>

// The file's component-size tables for the swapping stream. aswapSize1's first four entries describe a triangle's
// 16-bit fields; aswapSize2 is a single 4 (a run of 32-bit values).
using hkPackedNiTriStripsSwapSize1 = uint32_t[7];

using hkPackedNiTriStripsSwapSize2 = uint32_t[1];

static hkPackedNiTriStripsSwapSize2 aswapSize2{4};
static hkPackedNiTriStripsSwapSize1 aswapSize1{2, 2, 2, 2, 4, 4, 4};
NiRTTI hkPackedNiTriStripsData::ms_RTTI("hkPackedNiTriStripsData", &NiObject::ms_RTTI);

hkPackedNiTriStripsData::hkPackedNiTriStripsData()
  : uiKeyCount(0), uiVertCount(0), usPartCount(0), paTriData(nullptr), paVertData(nullptr),
    paCompressedVertData(nullptr), paSubPartData(nullptr) {}

hkPackedNiTriStripsData::~hkPackedNiTriStripsData() {
    if (paTriData) {
        MemoryManager_Singleton.Deallocate(paTriData);
    }
    if (paVertData) {
        MemoryManager_Singleton.Deallocate(paVertData);
    }
    if (paSubPartData) {
        MemoryManager_Singleton.Deallocate(paSubPartData);
    }
    if (paCompressedVertData) {
        MemoryManager_Singleton.Deallocate(paCompressedVertData);
    }
}

const NiRTTI* hkPackedNiTriStripsData::GetRTTI() const {
    return &hkPackedNiTriStripsData::ms_RTTI;
}

// Before stream version 33 each triangle is read into the local oldData, and the first 8 bytes of it are stored into
// paTriData; a short read leaves them unwritten. The byte that version 34 adds is read and dropped.
void hkPackedNiTriStripsData::LoadBinary(NiStream& arStream) {
    NiBinaryStream* pIstr = arStream.Istr();
    uint32_t uiSwapSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pIstr->m_pfnRead(pIstr, &uiKeyCount, 4, &uiSwapSize, 1);
    uint64_t uiTriBytes = static_cast<uint64_t>(uiKeyCount) * sizeof(TriangleData);
    void* pTriangles =
        MemoryManager_Singleton.Allocate(uiTriBytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(uiTriBytes));
    paTriData = static_cast<TriangleData*>(pTriangles);
    if (arStream.m_BSStreamHeader.uiVersion >= 33) {
        pIstr = arStream.Istr();
        pIstr->m_uiAbsoluteCurrentPos += pIstr->m_pfnRead(
            pIstr, pTriangles, static_cast<uint32_t>(uiKeyCount * sizeof(TriangleData)), aswapSize1_0, 4
        );
    }
    else {
        for (uint32_t I = 0; I < uiKeyCount; ++I) {
            OldTriangleData oldData;
            pIstr = arStream.Istr();
            pIstr->m_uiAbsoluteCurrentPos +=
                pIstr->m_pfnRead(pIstr, &oldData, sizeof(OldTriangleData), aswapSize1_0, 7);
            TriangleData* pTriangle = &paTriData[I];
            pTriangle->us1 = oldData.us1;
            pTriangle->us2 = oldData.us2;
            pTriangle->us3 = oldData.us3;
            pTriangle->unused = oldData.unused;
        }
    }
    pIstr = arStream.Istr();
    uiSwapSize = 4;
    pIstr->m_uiAbsoluteCurrentPos += pIstr->m_pfnRead(pIstr, &uiVertCount, 4, &uiSwapSize, 1);
    if (arStream.m_BSStreamHeader.uiVersion >= 34) {
        uint8_t ucCompressed;
        uint32_t uiByteSwapSize = 1;
        pIstr = arStream.Istr();
        pIstr->m_uiAbsoluteCurrentPos += pIstr->m_pfnRead(pIstr, &ucCompressed, 1, &uiByteSwapSize, 1);
    }
    uint64_t uiVertBytes = static_cast<uint64_t>(uiVertCount) * sizeof(NiPoint3);
    void* pVertices =
        MemoryManager_Singleton.Allocate(uiVertBytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(uiVertBytes));
    paVertData = static_cast<NiPoint3*>(pVertices);
    pIstr = arStream.Istr();
    pIstr->m_uiAbsoluteCurrentPos +=
        pIstr->m_pfnRead(pIstr, pVertices, static_cast<uint32_t>(uiVertCount * sizeof(NiPoint3)), &aswapSize2_0, 1);
    pIstr = arStream.Istr();
    uiSwapSize = 2;
    pIstr->m_uiAbsoluteCurrentPos += pIstr->m_pfnRead(pIstr, &usPartCount, 2, &uiSwapSize, 1);
    uint64_t uiPartBytes = static_cast<uint64_t>(usPartCount) * sizeof(SubPartData);
    void* pParts =
        MemoryManager_Singleton.Allocate(uiPartBytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(uiPartBytes));
    paSubPartData = static_cast<SubPartData*>(pParts);
    pIstr = arStream.Istr();
    pIstr->m_uiAbsoluteCurrentPos +=
        pIstr->m_pfnRead(pIstr, pParts, static_cast<uint32_t>(usPartCount * sizeof(SubPartData)), &aswapSize2_0, 1);
}

bool hkPackedNiTriStripsData::RegisterStreamables(NiStream& arStream) {
    return NiObject::RegisterStreamables(arStream);
}

// Writes the triangles, the vertices (compressed to three 16-bit values each when there are compressed ones, behind a
// flag byte) and the sub parts, each count first. No base call.
void hkPackedNiTriStripsData::SaveBinary(NiStream& arStream) {
    NiStreamSaveBinary(arStream, uiKeyCount);
    arStream.m_pkOstr->BinaryWrite(paTriData, 8 * uiKeyCount, aswapSize1, 4);
    NiStreamSaveBinary(arStream, uiVertCount);
    uint8_t bCompressed = paCompressedVertData != nullptr;
    NiStreamSaveBinary(arStream, bCompressed);
    if (bCompressed) {
        uint32_t uiSize = 2;
        arStream.m_pkOstr->BinaryWrite(paCompressedVertData, 6 * uiVertCount, &uiSize, 1);
    }
    else {
        arStream.m_pkOstr->BinaryWrite(paVertData, 12 * uiVertCount, aswapSize2, 1);
    }
    NiStreamSaveBinary(arStream, usPartCount);
    arStream.m_pkOstr->BinaryWrite(paSubPartData, 12 * usPartCount, aswapSize2, 1);
}

// Equal only to itself.
bool hkPackedNiTriStripsData::IsEqual(NiObject* apObject) {
    return apObject == this;
}

// Creates an empty mesh.
NiObject* hkPackedNiTriStripsData::CreateObject() {
    hkPackedNiTriStripsData* pObject =
        static_cast<hkPackedNiTriStripsData*>(NiMemObject::operator new(sizeof(hkPackedNiTriStripsData)));
    if (pObject == nullptr) {
        return nullptr;
    }
    return ::new (pObject) hkPackedNiTriStripsData;
}

// The mesh pointer takes its reference through NiPointer.
hkPackedNiTriStripsShape::hkPackedNiTriStripsShape(const hkPackedNiTriStripsShapeCinfo& arInfo)
  : hkpShapeCollection(HK_SHAPE_USER1, hkpShapeCollection::COLLECTION_USER), spTriStripsData(arInfo.spData),
    vScale(arInfo.vScale), fRadius(arInfo.fRadius) {
    // No world box yet: the SDK's inline set, w 0.
    WorldAabb.m_min.set(3.40282e+38f, 3.40282e+38f, 3.40282e+38f, 0.0f);
    WorldAabb.m_max.set(-3.40282e+38f, -3.40282e+38f, -3.40282e+38f, 0.0f);
    m_disableWelding.m_bool = 1;
}

// Releases the mesh and clears the pointer; NiPointer's destructor then finds it null.
hkPackedNiTriStripsShape::~hkPackedNiTriStripsShape() {
    spTriStripsData = nullptr;
}

void hkPackedNiTriStripsShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    hkVector4 v9;
    uint32_t a2;
    uint32_t v8 = 0;
    if (WorldAabb.m_min.m_quad[0] != 3.402820018375656e+38 &&
        (AabbTransform.isApproximatelyEqual(localToWorld, 0.001f).m_bool &&
         (v9.m_quad = vScale.m_quad, fAabbScale == vScale.m_quad[0]))) {
        out = WorldAabb;
    }
    else {
        __m128 v4 = _mm_undefined_ps();
        __m128 v5 = _mm_undefined_ps();
        __m128 v6 = _mm_undefined_ps();
        hkVector4 v11;
        v4[0] = 3.40282e+38f;
        v5[0] = 3.40282e+38f;
        v6[0] = 0.0f;
        __m128 v10 = _mm_unpacklo_ps(v5, v6);
        v11.m_quad = _mm_unpacklo_ps(
            _mm_unpacklo_ps(v4, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(rdata_104DA48)))), v10
        );
        out.m_min.m_quad = v11.m_quad;
        v11.m_quad[0] = -3.40282e+38f;
        v10[0] = -3.40282e+38f;
        out.m_max.m_quad = _mm_unpacklo_ps(
            _mm_unpacklo_ps(v11.m_quad, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(rdata_104E148)))),
            _mm_unpacklo_ps(v10, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
        );
        hkPackedNiTriStripsData* m_pObject = spTriStripsData.m_pObject;
        if (m_pObject->uiVertCount) {
            int32_t v17 = 0;
            do {
                hkVector4 v14;
                hkVector4 a3_1;
                hkVector4 th_s;
                NiPoint3* v13 = &m_pObject->paVertData[v17];
                v14.m_quad = vScale.m_quad;
                a3_1.m_quad[0] = v13->x;
                a3_1.m_quad[1] = v13->y;
                a3_1.m_quad[2] = v13->z;
                a3_1.m_quad[3] = 0.0f;
                a3_1.m_quad = _mm_mul_ps(v14.m_quad, a3_1.m_quad);
                th_s.setTransformedPos(localToWorld, a3_1);
                __m128 m_quad = th_s.m_quad;
                ++v17;
                out.m_min.m_quad = _mm_min_ps(out.m_min.m_quad, th_s.m_quad);
                out.m_max.m_quad = _mm_max_ps(out.m_max.m_quad, m_quad);
                m_pObject = spTriStripsData.m_pObject;
                ++v8;
            }
            while (v8 < m_pObject->uiVertCount);
        }
    }
    *reinterpret_cast<float*>(&a2) = fRadius + tolerance;
    __m128 v16 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(a2)), _mm_set_ss(std::bit_cast<float>(a2)), 0);
    out.m_min.m_quad = _mm_sub_ps(out.m_min.m_quad, v16);
    out.m_max.m_quad = _mm_add_ps(out.m_max.m_quad, v16);
}

hkpShapeType hkPackedNiTriStripsShape::getType() const {
    return HK_SHAPE_TRIANGLE_COLLECTION;
}

// The first sub part's first triangle: key 0 (hkpShapeContainer's vtable at +0x10).
uint32_t hkPackedNiTriStripsShape::getFirstKey() const {
    return 0;
}

// The key's top byte is the sub part, its low 24 bits the triangle. The next triangle belongs to the next sub part once
// its first index reaches the vertices of the sub parts up to the current one; the part index is compared signed.
uint32_t hkPackedNiTriStripsShape::getNextKey(uint32_t aOldKey) const {
    hkPackedNiTriStripsData* pData = spTriStripsData.m_pObject;
    int32_t iPart = static_cast<int32_t>(aOldKey >> 24);
    uint32_t uiTriangle = (aOldKey & 0xFFFFFF) + 1;
    if (uiTriangle >= pData->uiKeyCount) {
        return HK_INVALID_SHAPE_KEY;
    }
    uint32_t uiVertices = 0;
    for (int32_t i = 0; i <= iPart; ++i) {
        uiVertices += pData->paSubPartData[i].uiNumVerts;
    }
    if (pData->paTriData[uiTriangle].us1 >= uiVertices) {
        ++iPart;
    }
    return static_cast<uint32_t>(iPart) << 24 | (uiTriangle & 0xFFFFFF);
}

uint32_t hkPackedNiTriStripsShape::getCollisionFilterInfo(uint32_t aKey) const {
    hkPackedNiTriStripsData* m_pObject = spTriStripsData.m_pObject;
    if (static_cast<int32_t>(static_cast<uint8_t>(aKey >> 24)) < m_pObject->usPartCount) {
        return m_pObject->paSubPartData[static_cast<uint8_t>(aKey >> 24)].uiFilter;
    }

    return 0;
}

// The triangle is built in the caller's buffer (the SDK's placement new, with its null test).
const hkpShape* hkPackedNiTriStripsShape::getChildShape(uint32_t aKey, char (&arBuffer)[512]) const {
    hkNormalTriangleShape* ptriangleShape;
    hkVector4 v17;
    hkPackedNiTriStripsData* m_pObject = spTriStripsData.m_pObject;
    if ((aKey & 0xFFFFFF) >= m_pObject->uiKeyCount) {
        return nullptr;
    }
    TriangleData* v6 = &m_pObject->paTriData[aKey & 0xFFFFFF];
    if (&arBuffer) {
        ptriangleShape = ::new (&arBuffer) hkNormalTriangleShape(fRadius);
    }
    else {
        ptriangleShape = nullptr;
    }
    NiPoint3* v8 = &spTriStripsData.m_pObject->paVertData[v6->us1];
    float x = v8->x;
    float y = v8->y;
    float z = v8->z;
    ptriangleShape->m_vertexA.m_quad[0] = x;
    ptriangleShape->m_vertexA.m_quad[1] = y;
    ptriangleShape->m_vertexA.m_quad[2] = z;
    ptriangleShape->m_vertexA.m_quad[3] = 0.0f;
    NiPoint3* v11 = &spTriStripsData.m_pObject->paVertData[v6->us2];
    float v19 = v11->x;
    float v12 = v11->y;
    float v13 = v11->z;
    ptriangleShape->m_vertexB.m_quad[0] = v19;
    ptriangleShape->m_vertexB.m_quad[1] = v12;
    ptriangleShape->m_vertexB.m_quad[2] = v13;
    ptriangleShape->m_vertexB.m_quad[3] = 0.0f;
    NiPoint3* v14 = &spTriStripsData.m_pObject->paVertData[v6->us3];
    float v20 = v14->x;
    float v15 = v14->y;
    float v16 = v14->z;
    ptriangleShape->m_vertexC.m_quad[0] = v20;
    ptriangleShape->m_vertexC.m_quad[1] = v15;
    ptriangleShape->m_vertexC.m_quad[2] = v16;
    ptriangleShape->m_vertexC.m_quad[3] = 0.0f;
    v17.m_quad = vScale.m_quad;
    if (1.0f != vScale.m_quad[0]) {
        ptriangleShape->m_vertexA.m_quad = _mm_mul_ps(ptriangleShape->m_vertexA.m_quad, vScale.m_quad);
        ptriangleShape->m_vertexB.m_quad = _mm_mul_ps(vScale.m_quad, ptriangleShape->m_vertexB.m_quad);
        ptriangleShape->m_vertexC.m_quad = _mm_mul_ps(ptriangleShape->m_vertexC.m_quad, vScale.m_quad);
    }
    return (hkpShape*)ptriangleShape;
}

// The radius, the scale and the mesh copied into arInfo; the mesh through NiPointer's assignment.
void hkPackedNiTriStripsShape::CopyCinfo(hkPackedNiTriStripsShapeCinfo& arInfo) const {
    arInfo.fRadius = fRadius;
    arInfo.vScale = vScale;
    arInfo.spData = spTriStripsData;
}

void hkPackedNiTriStripsShape::CalcWorldAabb(const hkTransform& localToWorld) {
    __m128 v2 = _mm_undefined_ps();
    __m128 v3 = _mm_undefined_ps();
    __m128 v4 = _mm_undefined_ps();
    hkVector4 v9;
    AabbTransform = localToWorld;
    hkPackedNiTriStripsData* m_pObject = spTriStripsData.m_pObject;
    fAabbScale = vScale.m_quad[0];
    v2[0] = 3.40282e+38f;
    v3[0] = 3.40282e+38f;
    v4[0] = 0.0f;
    uint32_t i = 0;
    __m128 v8 = _mm_unpacklo_ps(v3, v4);
    v9.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v2, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(rdata_104DA48)))), v8
    );
    WorldAabb.m_min.m_quad = v9.m_quad;
    v9.m_quad[0] = -3.40282e+38f;
    v8[0] = -3.40282e+38f;
    WorldAabb.m_max.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v9.m_quad, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(rdata_104E148)))),
        _mm_unpacklo_ps(v8, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    if (m_pObject->uiVertCount) {
        uint32_t v13 = 0;
        do {
            hkVector4 v11;
            hkVector4 a3;
            hkVector4 v15;
            NiPoint3* v10 = &spTriStripsData.m_pObject->paVertData[v13];
            v11.m_quad = vScale.m_quad;
            a3.m_quad[0] = v10->x;
            a3.m_quad[1] = v10->y;
            a3.m_quad[2] = v10->z;
            a3.m_quad[3] = 0.0f;
            a3.m_quad = _mm_mul_ps(v11.m_quad, a3.m_quad);
            v15.setTransformedPos(localToWorld, a3);
            __m128 m_quad = v15.m_quad;
            ++v13;
            WorldAabb.m_min.m_quad = _mm_min_ps(WorldAabb.m_min.m_quad, v15.m_quad);
            WorldAabb.m_max.m_quad = _mm_max_ps(WorldAabb.m_max.m_quad, m_quad);
            ++i;
        }
        while (i < spTriStripsData.m_pObject->uiVertCount);
    }
}

// hkpTriangleShape's constructor with the SDK's default welding (none) and no extrusion.
hkNormalTriangleShape::hkNormalTriangleShape(float afRadius)
  : hkpTriangleShape(afRadius, 0, hkpWeldingUtility::WELDING_TYPE_NONE) {}

hkpShapeCollection::~hkpShapeCollection() {}
