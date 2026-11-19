#include "BSHavok/bhkCapsuleShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSMain/BSCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Algorithm/Collide/LineSegment/hkLineSegmentUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCapsule.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

NiRTTI bhkCapsuleShape::ms_RTTI("bhkCapsuleShape", &bhkConvexShape::ms_RTTI);

bhkCapsuleShape::bhkCapsuleShape() {
    ++iCount;
}

bhkCapsuleShape::bhkCapsuleShape(const hkCapsuleShapeCinfo& arInfo) {
    bhkCapsuleShape::Init(arInfo);
    ++iCount;
}

// The end points and the radius scaled to Havok units. This constructor does not count its instances, as the game does.
bhkCapsuleShape::bhkCapsuleShape(const NiPoint3& arInfo, const NiPoint3& a2, float a3) {
    hkVector4 kTop;
    hkVector4 kBottom;
    kTop.m_quad[0] = (float)(arInfo.x * bhkConvert::fBS2HkScaleSC);
    kTop.m_quad[1] = (float)(arInfo.y * bhkConvert::fBS2HkScaleSC);
    kTop.m_quad[2] = (float)(arInfo.z * bhkConvert::fBS2HkScaleSC);
    kTop.m_quad[3] = 0.0f;
    kBottom.m_quad[0] = (float)(a2.x * bhkConvert::fBS2HkScaleSC);
    kBottom.m_quad[1] = (float)(a2.y * bhkConvert::fBS2HkScaleSC);
    kBottom.m_quad[2] = (float)(a2.z * bhkConvert::fBS2HkScaleSC);
    kBottom.m_quad[3] = 0.0f;
    Init(kTop, kBottom, bhkConvert::fBS2HkScaleSC * a3);
}

bhkCapsuleShape::bhkCapsuleShape(const hkVector4& arTop, const hkVector4& arBottom, float afRad) {
    ++iCount;
    Init(arTop, arBottom, afRad);
}

// The clone is built on the Havok heap's context; the clone lock keeps two clones' cinfo copying apart.
NiObject* bhkCapsuleShape::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkCapsuleShape* pClone = nullptr;
    bhkSerializable::CloneCritLock();
    void* pMemory = NiMemObject::operator new(sizeof(bhkCapsuleShape));
    if (pMemory != nullptr) {
        pClone = ::new (pMemory) bhkCapsuleShape;
    }
    // CopyMembers is called through a bhkShape pointer so the call stays virtual; bhkShape declares the virtual, while
    // bhkSerializable's CopyMembers is a different, non-virtual function.
    bhkShape* pShape = this;
    pShape->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// Reads the shape, then puts its cinfo's end points in the machine's byte order when the stream is big-endian. The
// stream's byte, read from the file, is compared with 1: any other byte swaps.
void bhkCapsuleShape::LoadBinary(NiStream& arStream) {
    bhkConvexShape::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkCapsuleShapeCinfo& rCinfo = static_cast<hkCapsuleShapeCinfo&>(CreateCinfo(bCreated));
        // The end points' floats are byte-swapped as untyped 32-bit integers, not as floats.
        uint32_t* puiVertexA = reinterpret_cast<uint32_t*>(&rCinfo.m_vertexA);
        uint32_t* puiVertexB = reinterpret_cast<uint32_t*>(&rCinfo.m_vertexB);
        puiVertexA[0] = __builtin_bswap32(puiVertexA[0]);
        puiVertexA[1] = __builtin_bswap32(puiVertexA[1]);
        puiVertexA[2] = __builtin_bswap32(puiVertexA[2]);
        puiVertexA[3] = __builtin_bswap32(puiVertexA[3]);
        puiVertexB[0] = __builtin_bswap32(puiVertexB[0]);
        puiVertexB[1] = __builtin_bswap32(puiVertexB[1]);
        puiVertexB[2] = __builtin_bswap32(puiVertexB[2]);
        puiVertexB[3] = __builtin_bswap32(puiVertexB[3]);
    }
}

// Writes the shape with its cinfo's end points in the stream's byte order: swapped for a big-endian stream before the
// base writes them, and swapped back after. The stream's byte is compared with 1 both times: any other byte swaps, so
// it is read as a byte rather than a bool tested.
void bhkCapsuleShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkCapsuleShapeCinfo& rCinfo = static_cast<hkCapsuleShapeCinfo&>(CreateCinfo(bCreated));
    // The end points' floats are byte-swapped as untyped 32-bit integers, not as floats.
    uint32_t* puiVertexA = reinterpret_cast<uint32_t*>(&rCinfo.m_vertexA);
    uint32_t* puiVertexB = reinterpret_cast<uint32_t*>(&rCinfo.m_vertexB);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        puiVertexA[0] = __builtin_bswap32(puiVertexA[0]);
        puiVertexA[1] = __builtin_bswap32(puiVertexA[1]);
        puiVertexA[2] = __builtin_bswap32(puiVertexA[2]);
        puiVertexA[3] = __builtin_bswap32(puiVertexA[3]);
        puiVertexB[0] = __builtin_bswap32(puiVertexB[0]);
        puiVertexB[1] = __builtin_bswap32(puiVertexB[1]);
        puiVertexB[2] = __builtin_bswap32(puiVertexB[2]);
        puiVertexB[3] = __builtin_bswap32(puiVertexB[3]);
    }
    bhkConvexShape::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        puiVertexA[0] = __builtin_bswap32(puiVertexA[0]);
        puiVertexA[1] = __builtin_bswap32(puiVertexA[1]);
        puiVertexA[2] = __builtin_bswap32(puiVertexA[2]);
        puiVertexA[3] = __builtin_bswap32(puiVertexA[3]);
        puiVertexB[0] = __builtin_bswap32(puiVertexB[0]);
        puiVertexB[1] = __builtin_bswap32(puiVertexB[1]);
        puiVertexB[2] = __builtin_bswap32(puiVertexB[2]);
        puiVertexB[3] = __builtin_bswap32(puiVertexB[3]);
    }
}

// The base's strings first, then this class's own line; the array grows by its grow-by past its allocated size.
void bhkCapsuleShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkConvexShape::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(bhkCapsuleShape::ms_RTTI.m_pcName);
    apStrings->Add(pcString);
}

// Builds the capsule from the creation info: its two axis end points and the radius, then drops the info.
void bhkCapsuleShape::Init(const hkSerializableCinfo& arInfo) {
    const hkCapsuleShapeCinfo& kInfo = static_cast<const hkCapsuleShapeCinfo&>(arInfo);
    hkVector4 kTop;
    kTop.m_quad = hkQuadReal{kInfo.m_vertexA.fX, kInfo.m_vertexA.fY, kInfo.m_vertexA.fZ, kInfo.m_vertexA.fW};
    hkVector4 kBottom;
    kBottom.m_quad = hkQuadReal{kInfo.m_vertexB.fX, kInfo.m_vertexB.fY, kInfo.m_vertexB.fZ, kInfo.m_vertexB.fW};
    Init(kTop, kBottom, kInfo.m_radius);
    bhkSerializable* pSerializable = this;
    pSerializable->PostInit(arInfo);
}

// Builds the capsule's creation info from the wrapped shape when it has none yet, and answers in arCreated whether it
// made one. The cinfo is allocated from the game heap inside the Havok heap context and default-constructed: no user
// data, radius 0.1, the first end point at the origin and the second at (1, 0, 0, 0). The wrapped object is tested
// after the allocation, and GetCinfo is called even for a cinfo the allocation failed to make, as the game does.
hkSerializableCinfo& bhkCapsuleShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const bool bCreated = pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        hkCapsuleShapeCinfo* pCinfo =
            static_cast<hkCapsuleShapeCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkCapsuleShapeCinfo)));
        if (pCinfo != nullptr) {
            pCinfo->m_userData = 0;
            pCinfo->m_radius = 0.1f;
            pCinfo->m_vertexA.fX = 0.0f;
            pCinfo->m_vertexA.fY = 0.0f;
            pCinfo->m_vertexA.fZ = 0.0f;
            pCinfo->m_vertexA.fW = 0.0f;
            pCinfo->m_vertexB.fX = 1.0f;
            pCinfo->m_vertexB.fY = 0.0f;
            pCinfo->m_vertexB.fZ = 0.0f;
            pCinfo->m_vertexB.fW = 0.0f;
        }
        const bool bHasObject = phkObject != nullptr;
        pInfo = pCinfo;
        if (bHasObject) {
            GetCinfo(pCinfo);
        }
    }
    return *pInfo;
}

// Scales the capsule by the cloning pass's scale (its x: the pass is always a BSCloningProcess) where it is not 1:
// both end points and the radius of the shape's own cinfo, before the base copies the members.
void bhkCapsuleShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkCapsuleShapeCinfo& rCinfo = static_cast<hkCapsuleShapeCinfo&>(CreateCinfo(bCreated));
    const float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        rCinfo.m_vertexA.fX = rCinfo.m_vertexA.fX * fScale;
        rCinfo.m_vertexA.fY = fScale * rCinfo.m_vertexA.fY;
        rCinfo.m_vertexA.fZ = rCinfo.m_vertexA.fZ * fScale;
        rCinfo.m_vertexA.fW = fScale * rCinfo.m_vertexA.fW;
        rCinfo.m_vertexB.fX = rCinfo.m_vertexB.fX * fScale;
        rCinfo.m_vertexB.fY = fScale * rCinfo.m_vertexB.fY;
        rCinfo.m_vertexB.fZ = rCinfo.m_vertexB.fZ * fScale;
        rCinfo.m_vertexB.fW = fScale * rCinfo.m_vertexB.fW;
        rCinfo.m_radius = fScale * rCinfo.m_radius;
    }
    bhkShape::CopyMembers(apDest, arCloneData);
}

// Displays the capsule: a cinfo from hkCapsuleShapeCinfo's constructor, filled from the shape, then the end
// points of the Havok capsule (the zero vector without one) handed to a 6-sided hkDisplayCapsule, whose geometry goes
// to the debug node, all under the Havok memory context. The display capsule is never freed.
void bhkCapsuleShape::BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    hkCapsuleShapeCinfo kCinfo;
    CopyCinfo(kCinfo);
    // EXE-INLINED 0x4AE750
    hkpCapsuleShape* pCapsule = static_cast<hkpCapsuleShape*>(this ? phkObject : nullptr);
    kCinfo.m_vertexA = pCapsule ? pCapsule->m_vertexA : hkVector4::getZero();
    // Tests this and reads phkObject again after the first copy, as the game does.
    // EXE-INLINED 0x4AE750
    pCapsule = static_cast<hkpCapsuleShape*>(this ? phkObject : nullptr);
    kCinfo.m_vertexB = pCapsule ? pCapsule->m_vertexB : hkVector4::getZero();
    hkDisplayCapsule* pMemory =
        static_cast<hkDisplayCapsule*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkDisplayCapsule)));
    pMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkDisplayCapsule));
    // The end points as Havok vectors, each component copied as its bits.
    // EXE-INLINED 0x4B4D10
    hkVector4 kBottom;
    kBottom.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(kCinfo.m_vertexB.fX))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(kCinfo.m_vertexB.fZ)))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(kCinfo.m_vertexB.fY))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(kCinfo.m_vertexB.fW)))
        )
    );
    // EXE-INLINED 0x4B4D10
    hkVector4 kTop;
    kTop.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(kCinfo.m_vertexA.fX))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(kCinfo.m_vertexA.fZ)))
        ),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(kCinfo.m_vertexA.fY))),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(kCinfo.m_vertexA.fW)))
        )
    );
    hkDisplayCapsule* pDisplay = ::new (pMemory) hkDisplayCapsule(kTop, kBottom, kCinfo.m_radius, 6, 1);
    pDisplay->buildGeometry();
    hkGeometry* pGeometry = pDisplay->m_geometry;
    if (pGeometry) {
        BuildDisplayFromhkGeometry(apNode, pGeometry, arDefColor, "bhkCapsuleShape");
    }
    pTLSData->eMemContext = eMemContext;
}

bhkCapsuleShape::~bhkCapsuleShape() {
    bhkSerializable::Kill();
    --iCount;
}

// The (top, bottom, radius) overload: the capsule's Havok object is built on the thread's heap allocator and handed to
// the shape, which drops the reference it was given.
void bhkCapsuleShape::Init(const hkVector4& arTop, const hkVector4& arBottom, float afRad) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
    hkpCapsuleShape* pShape = static_cast<hkpCapsuleShape*>(kHeap.blockAlloc(sizeof(hkpCapsuleShape)));
    // Writes the allocated size into the block before the constructor runs.
    pShape->m_memSizeAndFlags = sizeof(hkpCapsuleShape);
    pShape = ::new (pShape) hkpCapsuleShape(arTop, arBottom, afRad);
    bhkSerializable* pSerializable = this;
    pSerializable->SethkObject(pShape);
    pShape->removeReference();
}

// The closest points of the capsule, placed by arTrans, and arSphere: false when they are apart (the squared
// distance of the sphere's centre from the capsule's axis at least the squared sum of the radii), else true with
// arCPoint the point on the capsule's surface, the normal from the axis to the sphere's centre (a perpendicular of
// the axis when the centre is on it) and the distance between the surfaces in the normal's w.
bool bhkCapsuleShape::GetClosestPoints(const hkSphere& arSphere, const hkTransform& arTrans, hkContactPoint& arCPoint) {
    // EXE-INLINED 0x4AE750
    const hkpCapsuleShape* pCapsule = static_cast<const hkpCapsuleShape*>(this ? phkObject : nullptr);

    // The capsule's two end points placed by arTrans, the last first: (y * col1 + x * col0) + (z * col2 +
    // translation). The SDK's getVertices() is &m_vertexA, m_vertexB right after it.
    // EXE-INLINED none: hkVector4Util::transformPoints
    hkVector4 aVertices[2];
    {
        const __m128 col0 = arTrans.m_rotation.m_col0.m_quad;
        const __m128 col1 = arTrans.m_rotation.m_col1.m_quad;
        const __m128 col2 = arTrans.m_rotation.m_col2.m_quad;
        const __m128 translation = arTrans.m_translation.m_quad;
        const hkVector4* pVertices = &pCapsule->m_vertexA;
        int32_t i = 1;
        do {
            const __m128 v = pVertices[i].m_quad;
            aVertices[i].m_quad = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), col1), _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), col0)),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), col2), translation)
            );
        }
        while (--i >= 0);
    }

    hkLineSegmentUtil::ClosestPointLineSegResult kLineResult;
    hkLineSegmentUtil::closestPointLineSeg(arSphere.m_pos, aVertices[0], aVertices[1], kLineResult);

    // The sphere's radius is its w.
    const float fRadiusSum = arSphere.m_pos(3) + pCapsule->m_radius;
    hkVector4 kDiff;
    kDiff.m_quad = _mm_sub_ps(arSphere.m_pos.m_quad, kLineResult.m_pointOnEdge.m_quad);
    // lengthSquared3: z + (y + x), in SSE.
    const __m128 squares = _mm_mul_ps(kDiff.m_quad, kDiff.m_quad);
    const float fDistSquared = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(squares, squares, 0xAA), _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares))
    );
    // The squared radius sum is rounded to a float before the compare.
    const float fRadiusSumSquared = fRadiusSum * fRadiusSum;
    if (fRadiusSumSquared <= fDistSquared) {
        return false;
    }
    hkVector4 kNormal;
    float fDist;
    if (fDistSquared > 0.0f) {
        fDist = std::sqrt(fDistSquared);
        kNormal = kDiff;
    }
    else {
        fDist = 0.0f;
        hkVector4 kEdge;
        kEdge.m_quad = _mm_sub_ps(aVertices[1].m_quad, aVertices[0].m_quad);
        // The SDK's perpendicular of the axis: the smallest component's lane zero, the other two swapped, one
        // negated.
        // EXE-INLINED none: hkVector4Util::calculatePerpendicularVector
        int32_t min = 0;
        int32_t ok1 = 1;
        int32_t ok2 = 2;
        float a0 = std::fabs(kEdge(0));
        const float a1 = std::fabs(kEdge(1));
        const float a2 = std::fabs(kEdge(2));
        if (a1 < a0) {
            ok1 = 0;
            min = 1;
            a0 = a1;
        }
        if (a2 < a0) {
            ok2 = min;
        }
        kNormal.m_quad = _mm_setzero_ps();
        kNormal(ok1) = kEdge(ok2);
        kNormal(ok2) = -kEdge(ok1);
    }
    // normalize3: the broadcast inverse length (reciprocal square root estimate, one Newton step, 0 for a zero length)
    // times the vector.
    const __m128 unnormalised = kNormal.m_quad;
    const __m128 normalSquares = _mm_mul_ps(unnormalised, unnormalised);
    const __m128 len2 = _mm_add_ss(
        _mm_shuffle_ps(normalSquares, normalSquares, 0xAA),
        _mm_add_ss(_mm_shuffle_ps(normalSquares, normalSquares, 0x55), normalSquares)
    );
    const __m128 approx = _mm_rsqrt_ss(len2);
    const __m128 len2IsZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
    const __m128 refined = _mm_mul_ss(
        _mm_mul_ss(_mm_set_ss(0.5f), approx), _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx))
    );
    const __m128 lenInv = _mm_andnot_ps(len2IsZero, refined);
    const __m128 normal = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), unnormalised);
    // setNormal: x, y and z of the normal, the contact's w kept.
    // EXE-INLINED 0xCAAD60
    arCPoint.m_separatingNormal.m_quad =
        _mm_shuffle_ps(normal, _mm_unpackhi_ps(normal, arCPoint.m_separatingNormal.m_quad), _MM_SHUFFLE(3, 0, 1, 0));
    // The point on the capsule's surface: the sphere's centre plus the normal times (capsule radius - distance); the
    // distance between the surfaces in w.
    const float fFromCentre = pCapsule->m_radius - fDist;
    const __m128 scale = _mm_set_ss(fFromCentre);
    arCPoint.m_position.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), arCPoint.m_separatingNormal.m_quad), arSphere.m_pos.m_quad
    );
    arCPoint.m_separatingNormal(3) = fDist - fRadiusSum;
    return true;
}

NiObject* bhkCapsuleShape::CreateObject() {
    void* pMemory = NiMemObject::operator new(sizeof(bhkCapsuleShape));
    if (pMemory != nullptr) {
        return ::new (pMemory) bhkCapsuleShape;
    }
    return nullptr;
}
