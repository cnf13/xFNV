#include "BSHavok/bhkTriSampledHeightFieldBvTreeShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBvTreeShape.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkStream.h"
#include "BSHavok/hkBSHeightFieldShape.h"
#include "Fallout Shared/Havok/CellMopp.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldCollection.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpHeightFieldShape.h"

#include <bit>
#include <xmmintrin.h>

NiRTTI bhkTriSampledHeightFieldBvTreeShape::ms_RTTI("bhkTriSampledHeightFieldBvTreeShape", &bhkBvTreeShape::ms_RTTI);

bhkTriSampledHeightFieldBvTreeShape::bhkTriSampledHeightFieldBvTreeShape() {
    ++iCount;
}

bhkTriSampledHeightFieldBvTreeShape::bhkTriSampledHeightFieldBvTreeShape(
    const hkTriSampledHeightFieldBvTreeShapeCinfo& arInfo
) {
    bhkTriSampledHeightFieldBvTreeShape::Init(arInfo);
    ++iCount;
}

bhkTriSampledHeightFieldBvTreeShape::~bhkTriSampledHeightFieldBvTreeShape() {
    bhkSerializable::Kill();
    --iCount;
}

// A new shape built in the Havok heap context under the clone lock, then the members copied through the vtable.
NiObject* bhkTriSampledHeightFieldBvTreeShape::CreateClone(NiCloningProcess& arCloneData) {
    int32_t iOldMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    bhkTriSampledHeightFieldBvTreeShape* pClone = static_cast<bhkTriSampledHeightFieldBvTreeShape*>(
        NiMemObject::operator new(sizeof(bhkTriSampledHeightFieldBvTreeShape))
    );
    if (pClone) {
        ::new (pClone) bhkTriSampledHeightFieldBvTreeShape;
    }
    else {
        pClone = nullptr;
    }
    CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    GetThreadMemContext() = iOldMemContext;
    return pClone;
}

// The base's fields, then, for a big-endian source, the creation info's scale, resolutions and height range
// byte-swapped in place and its vertex array cleared. The stream's flag is compared with 1 as a byte. A cinfo built
// here is not killed. The height floats are swapped as untyped 32-bit words, hence the uint32_t views.
void bhkTriSampledHeightFieldBvTreeShape::LoadBinary(NiStream& arStream) {
    bhkBvTreeShape::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkTriSampledHeightFieldBvTreeShapeCinfo& cinfo =
            static_cast<hkTriSampledHeightFieldBvTreeShapeCinfo&>(CreateCinfo(bCreated));
        bhkStream::Endian(cinfo.m_scale);
        // EXE-INLINED none: BSEndian
        cinfo.m_xRes = static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(cinfo.m_xRes)));
        // EXE-INLINED none: BSEndian
        cinfo.m_zRes = static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(cinfo.m_zRes)));
        // EXE-INLINED none: BSEndian
        uint32_t* puiMinHeight = reinterpret_cast<uint32_t*>(&cinfo.m_minHeight);
        *puiMinHeight = __builtin_bswap32(*puiMinHeight);
        // EXE-INLINED none: BSEndian
        uint32_t* puiMaxHeight = reinterpret_cast<uint32_t*>(&cinfo.m_maxHeight);
        *puiMaxHeight = __builtin_bswap32(*puiMaxHeight);
        cinfo.pData = nullptr;
    }
}

// For a big-endian save, the creation info's scale, resolutions and height range are byte-swapped in place around the
// base's write and swapped back after it; both times the vertex array is cleared. The stream's byte is compared with 1
// and read again after the base call. Each float is swapped as an untyped 32-bit value, hence the uint32_t views.
void bhkTriSampledHeightFieldBvTreeShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkTriSampledHeightFieldBvTreeShapeCinfo& cinfo =
        static_cast<hkTriSampledHeightFieldBvTreeShapeCinfo&>(CreateCinfo(bCreated));
    uint32_t* puiMinHeight = reinterpret_cast<uint32_t*>(&cinfo.m_minHeight);
    uint32_t* puiMaxHeight = reinterpret_cast<uint32_t*>(&cinfo.m_maxHeight);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(cinfo.m_scale);
        // EXE-INLINED none: BSEndian
        cinfo.m_xRes = static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(cinfo.m_xRes)));
        // EXE-INLINED none: BSEndian
        cinfo.m_zRes = static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(cinfo.m_zRes)));
        // EXE-INLINED none: BSEndian
        *puiMinHeight = __builtin_bswap32(*puiMinHeight);
        // EXE-INLINED none: BSEndian
        *puiMaxHeight = __builtin_bswap32(*puiMaxHeight);
        cinfo.pData = nullptr;
    }
    bhkBvTreeShape::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(cinfo.m_scale);
        // EXE-INLINED none: BSEndian
        cinfo.m_xRes = static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(cinfo.m_xRes)));
        // EXE-INLINED none: BSEndian
        cinfo.m_zRes = static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(cinfo.m_zRes)));
        // EXE-INLINED none: BSEndian
        *puiMinHeight = __builtin_bswap32(*puiMinHeight);
        // EXE-INLINED none: BSEndian
        *puiMaxHeight = __builtin_bswap32(*puiMaxHeight);
        cinfo.pData = nullptr;
    }
}

void bhkTriSampledHeightFieldBvTreeShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    v2->eMemContext = MEM_CONTEXT_HAVOK;
    bhkBvTreeShape::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkTriSampledHeightFieldBvTreeShape::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v8 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v8);
    v2->eMemContext = eMemContext;
}

void bhkTriSampledHeightFieldBvTreeShape::KillCinfo(bool abCreated) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    if (abCreated) {
        hkSerializableCinfo* pInfo = this->pInfo;
        if (pInfo) {
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            // The cinfo block allocated by CreateCinfo is freed with the size of hkpSampledHeightFieldBaseCinfo, whose
            // class allocator delete the cinfo inherits; the size mismatch is deliberate, as the game does.
            Instance->m_heap->blockFree(pInfo, sizeof(hkpSampledHeightFieldBaseCinfo));
        }
        this->pInfo = nullptr;
    }
    v3->eMemContext = eMemContext;
}

// The size of hkTriSampledHeightFieldBvTreeShapeCinfo.
uint32_t bhkTriSampledHeightFieldBvTreeShape::QCinfoSize() const {
    return 80;
}

// Builds the Havok shapes from the creation info: Bethesda's height field shape over the info's height field part, the
// triangle collection over it with radius 0.5 and the tree over that, which becomes the wrapped object; the local
// references are dropped, all inside the Havok memory context.
void bhkTriSampledHeightFieldBvTreeShape::Init(const hkSerializableCinfo& arInfo) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    hkBSHeightFieldShape* pShape = static_cast<hkBSHeightFieldShape*>(
        hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkBSHeightFieldShape))
    );
    pShape->m_memSizeAndFlags = sizeof(hkBSHeightFieldShape);
    // Tests arInfo's address before adjusting it to the height field part, as the game does.
    const hkBSHeightFieldShapeCinfo* pCinfo = nullptr;
    if (&arInfo) {
        pCinfo = &static_cast<const hkTriSampledHeightFieldBvTreeShapeCinfo&>(arInfo);
    }
    ::new (pShape) hkBSHeightFieldShape(*pCinfo);

    void* pCollectionMemory =
        hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpTriSampledHeightFieldCollection));
    static_cast<hkpTriSampledHeightFieldCollection*>(pCollectionMemory)->m_memSizeAndFlags =
        sizeof(hkpTriSampledHeightFieldCollection);
    hkpTriSampledHeightFieldCollection* pcol =
        ::new (pCollectionMemory) hkpTriSampledHeightFieldCollection(pShape, 0.5f);

    void* pTreeMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpTriSampledHeightFieldBvTreeShape));
    static_cast<hkpTriSampledHeightFieldBvTreeShape*>(pTreeMemory)->m_memSizeAndFlags =
        sizeof(hkpTriSampledHeightFieldBvTreeShape);
    hkpTriSampledHeightFieldBvTreeShape* po =
        ::new (pTreeMemory) hkpTriSampledHeightFieldBvTreeShape(pcol, hkBool(false));
    SethkObject(po);
    po->removeReference();
    pcol->removeReference();
    pShape->removeReference();
    PostInit(arInfo);
    pTLSData->eMemContext = eMemContext;
}

// A new creation info from the Havok heap, filled from the Havok shape when there is one; arCreated says whether it was
// made here.
hkSerializableCinfo& bhkTriSampledHeightFieldBvTreeShape::CreateCinfo(bool& arCreated) {
    int32_t iOldMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    bool bCreate = pInfo == nullptr;
    arCreated = bCreate;
    if (bCreate) {
        void* pMemory =
            hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkTriSampledHeightFieldBvTreeShapeCinfo));
        hkTriSampledHeightFieldBvTreeShapeCinfo* pNewInfo = nullptr;
        if (pMemory) {
            pNewInfo = ::new (pMemory) hkTriSampledHeightFieldBvTreeShapeCinfo;
        }
        // The cinfo does not derive from hkSerializableCinfo (CellMopp.h); the pointer is stored as is.
        pInfo = static_cast<hkSerializableCinfo*>(static_cast<void*>(pNewInfo));
        if (phkObject) {
            CopyCinfo(*pNewInfo);
        }
    }
    hkSerializableCinfo* pResult = pInfo;
    GetThreadMemContext() = iOldMemContext;
    return *pResult;
}

void bhkTriSampledHeightFieldBvTreeShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool v4;
    CreateCinfo(v4);
    bhkBvTreeShape::CopyMembers(apDest, arCloneData);
}

// bhkBvTreeShape's part, then, when the Havok tree's collection has a height field, the field's height range
// (centre plus and minus the half extent), scale, resolutions and vertex array.
void bhkTriSampledHeightFieldBvTreeShape::CopyCinfo(hkTriSampledHeightFieldBvTreeShapeCinfo& arInfo) {
    bhkBvTreeShape::CopyCinfo(arInfo);
    const hkpTriSampledHeightFieldBvTreeShape* pTree =
        static_cast<const hkpTriSampledHeightFieldBvTreeShape*>(phkObject);
    if (pTree) {
        const hkpTriSampledHeightFieldCollection* pCollection = pTree->getShapeCollection();
        if (pCollection) {
            const hkBSHeightFieldShape* pShape =
                static_cast<const hkBSHeightFieldShape*>(pCollection->getHeightFieldShape());
            if (pShape) {
                arInfo.m_maxHeight = pShape->m_extents.m_quad[2] + pShape->m_heightCenter;
                arInfo.m_minHeight = pShape->m_heightCenter - pShape->m_extents.m_quad[2];
                arInfo.m_scale = pShape->m_intToFloatScale;
                arInfo.m_xRes = pShape->m_xRes;
                arInfo.m_zRes = pShape->m_zRes;
                arInfo.pData = pShape->pData;
            }
        }
    }
}

hkBSHeightFieldShape::hkBSHeightFieldShape(const hkBSHeightFieldShapeCinfo& arInfo)
  : hkpSampledHeightFieldShape(arInfo, HEIGHTFIELD_USER), pData(arInfo.pData),
    invZRes(static_cast<int32_t>(static_cast<double>(m_zRes) - 1.0)), fOffset(arInfo.fOffset) {}

hkBSHeightFieldShape::~hkBSHeightFieldShape() {}

// hkSampledHeightFieldShape_collideSpheres for this class, as hkpStorageSampledHeightFieldShape's: the heights and the
// flip come from the virtual getHeightAtImpl and getTriangleFlipImpl.
void hkBSHeightFieldShape::collideSpheres(
    const hkpHeightFieldShape::CollideSpheresInput& arInput,
    hkVector4* apOutputArray
) const {
    const hkSphere* spheres = arInput.m_spheres;
    hkVector4* output = apOutputArray;

    hkVector4 defNormal;
    defNormal.set(0.0f, 1.0f, 0.0f, 3.40282e+38f);

    for (int32_t i = arInput.m_numSpheres - 1; i >= 0; output++, spheres++, i--) {
        output[0] = defNormal;

        hkVector4 pos;
        pos.setMul4(spheres->m_pos, m_floatToIntScale);

        // EXE-INLINED none: hkVector4Util::convertToUint16
        // (position + offset) * scale plus the magic vector leaves each lane's 16-bit integer in bits 6 to 21.
        const hkQuadReal converted = _mm_add_ps(
            _mm_mul_ps(
                _mm_add_ps(spheres->m_pos.m_quad, m_floatToIntOffsetFloorCorrected.m_quad), m_floatToIntScale.m_quad
            ),
            hkVector4Util_floatToUint16Magic
        );
        const int32_t x = static_cast<uint16_t>(std::bit_cast<uint32_t>(converted[0]) >> 6);
        const int32_t z = static_cast<uint16_t>(std::bit_cast<uint32_t>(converted[2]) >> 6);

        if ((x >= (m_xRes - 1)) || (z >= (m_zRes - 1))) {
            continue;
        }

        const float subX = pos(0) - static_cast<float>(x);
        const float subZ = pos(2) - static_cast<float>(z);

        float height;
        float vertHeight;
        int32_t vertX;
        if (getTriangleFlip().m_bool) {
            const float h00 = getHeightAt(x + 0, z);
            vertHeight = h00;
            vertX = x;
            const float h11 = getHeightAt(x + 1, z + 1);
            if (subX > subZ) {
                const float h10 = getHeightAt(x + 1, z);

                const float dx = h10 - h00;
                const float dz = h11 - h10;

                height = h00 + subZ * dz + subX * dx;

                output[0](0) = -dx;
                output[0](2) = -dz;
            }
            else {
                const float h01 = getHeightAt(x, z + 1);

                const float dx = h11 - h01;
                const float dz = h01 - h00;

                height = h00 + subZ * dz + subX * dx;

                output[0](0) = -dx;
                output[0](2) = -dz;
            }
        }
        else {
            const float h10 = getHeightAt(x + 1, z);
            vertHeight = h10;
            vertX = x + 1;
            const float h01 = getHeightAt(x, z + 1);
            // Compares with the double 1.0 and subtracts it below.
            if (subX + subZ > 1.0) {
                const float h11 = getHeightAt(x + 1, z + 1);

                const float dx = h11 - h01;
                const float dz = h11 - h10;

                height = h10 + subZ * dz + (subX - 1.0) * dx;

                output[0](0) = -dx;
                output[0](2) = -dz;
            }
            else {
                const float h00 = getHeightAt(x + 0, z);

                const float dx = h10 - h00;
                const float dz = h01 - h00;

                height = h00 + subZ * dz + subX * dx;

                output[0](0) = -dx;
                output[0](2) = -dz;
            }
        }

        output->mul4(m_floatToIntScale);
        output->normalize3();

        float absoluteDepth;
        if (!m_useProjectionBasedHeight.m_bool) {
            absoluteDepth = (pos(1) - height) * m_intToFloatScale(1) - spheres->m_pos(3);
        }
        else {
            hkVector4 triangleVert;
            triangleVert.set(static_cast<float>(vertX), vertHeight, static_cast<float>(z), 0.0f);

            hkVector4 vectorToTri;
            vectorToTri.setSub4(pos, triangleVert);
            vectorToTri.mul4(m_intToFloatScale);

            const float distanceToTri = vectorToTri.dot3(*output);
            absoluteDepth = distanceToTri - spheres->m_pos(3);
        }
        output[0](3) = absoluteDepth;
    }
}
