#include "BSHavok/bhkTriSampledHeightFieldBvTreeShape.h"

#include "BSCore/BSCoreDefines.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkBvTreeShape.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkStream.h"
#include "BSHavok/hkBSHeightFieldShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
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
#include "xfnv/Compiler.h"

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
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::CloneCritLock();
    bhkTriSampledHeightFieldBvTreeShape* pClone = new bhkTriSampledHeightFieldBvTreeShape;
    CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

// The base's fields, then, for a big-endian source, the creation info's scale, resolutions and height range
// byte-swapped in place and its vertex array cleared. The stream's flag is compared with 1 as a byte. A cinfo built
// here is not killed.
void bhkTriSampledHeightFieldBvTreeShape::LoadBinary(NiStream& arStream) {
    bhkBvTreeShape::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkTriSampledHeightFieldBvTreeShapeCinfo& cinfo = static_cast<hkTriSampledHeightFieldBvTreeShapeCinfo&>(
            CreateCinfo(bCreated)
        );
        bhkStream::Endian(cinfo.m_scale);
        BSEndian(cinfo.m_xRes, false);
        BSEndian(cinfo.m_zRes, false);
        BSEndian(cinfo.m_minHeight, false);
        BSEndian(cinfo.m_maxHeight, false);
        cinfo.pData = nullptr;
    }
}

// For a big-endian save, the creation info's scale, resolutions and height range are byte-swapped in place around the
// base's write and swapped back after it; both times the vertex array is cleared. The stream's byte is compared with 1
// and read again after the base call.
void bhkTriSampledHeightFieldBvTreeShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkTriSampledHeightFieldBvTreeShapeCinfo& cinfo = static_cast<hkTriSampledHeightFieldBvTreeShapeCinfo&>(
        CreateCinfo(bCreated)
    );
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(cinfo.m_scale);
        BSEndian(cinfo.m_xRes, false);
        BSEndian(cinfo.m_zRes, false);
        BSEndian(cinfo.m_minHeight, false);
        BSEndian(cinfo.m_maxHeight, false);
        cinfo.pData = nullptr;
    }
    bhkBvTreeShape::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::Endian(cinfo.m_scale);
        BSEndian(cinfo.m_xRes, false);
        BSEndian(cinfo.m_zRes, false);
        BSEndian(cinfo.m_minHeight, false);
        BSEndian(cinfo.m_maxHeight, false);
        cinfo.pData = nullptr;
    }
}

void bhkTriSampledHeightFieldBvTreeShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkBvTreeShape::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkTriSampledHeightFieldBvTreeShape::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v8 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v8);
}

void bhkTriSampledHeightFieldBvTreeShape::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        delete static_cast<hkTriSampledHeightFieldBvTreeShapeCinfo*>(this->pInfo);
        this->pInfo = nullptr;
    }
}

// The size of hkTriSampledHeightFieldBvTreeShapeCinfo.
uint32_t bhkTriSampledHeightFieldBvTreeShape::QCinfoSize() const {
    return 80;
}

// Builds the Havok shapes from the creation info: Bethesda's height field shape over the info's height field part, the
// triangle collection over it with radius 0.5 and the tree over that, which becomes the wrapped object; the local
// references are dropped, all inside the Havok memory context.
void bhkTriSampledHeightFieldBvTreeShape::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    // Tests arInfo's address before adjusting it to the height field part.
    const hkBSHeightFieldShapeCinfo* pCinfo = nullptr;
    if (&arInfo) {
        pCinfo = &static_cast<const hkTriSampledHeightFieldBvTreeShapeCinfo&>(arInfo);
    }
    hkBSHeightFieldShape* pShape = new hkBSHeightFieldShape(*pCinfo);
    hkpTriSampledHeightFieldCollection* pcol = new hkpTriSampledHeightFieldCollection(pShape, 0.5f);
    hkpTriSampledHeightFieldBvTreeShape* po = new hkpTriSampledHeightFieldBvTreeShape(pcol, hkBool(false));
    SethkObject(po);
    po->removeReference();
    pcol->removeReference();
    pShape->removeReference();
    PostInit(arInfo);
}

// A new creation info from the Havok heap, filled from the Havok shape when there is one; arCreated says whether it was
// made here.
hkSerializableCinfo& bhkTriSampledHeightFieldBvTreeShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bCreate = pInfo == nullptr;
    arCreated = bCreate;
    if (bCreate) {
        hkTriSampledHeightFieldBvTreeShapeCinfo* pNewInfo = new hkTriSampledHeightFieldBvTreeShapeCinfo;
        pInfo = pNewInfo;
        if (phkObject) {
            CopyCinfo(*pNewInfo);
        }
    }
    return *pInfo;
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
    const hkpTriSampledHeightFieldBvTreeShape* pTree = static_cast<const hkpTriSampledHeightFieldBvTreeShape*>(
        phkObject
    );
    if (pTree) {
        const hkpTriSampledHeightFieldCollection* pCollection = pTree->getShapeCollection();
        if (pCollection) {
            const hkBSHeightFieldShape* pShape = static_cast<const hkBSHeightFieldShape*>(
                pCollection->getHeightFieldShape()
            );
            if (pShape) {
                arInfo.m_maxHeight = XFNV_M128_F32(pShape->m_extents.m_quad, 2) + pShape->m_heightCenter;
                arInfo.m_minHeight = pShape->m_heightCenter - XFNV_M128_F32(pShape->m_extents.m_quad, 2);
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
    defNormal.set(0.0f, 1.0f, 0.0f, HK_REAL_MAX);

    for (int32_t i = arInput.m_numSpheres - 1; i >= 0; output++, spheres++, i--) {
        output[0] = defNormal;

        hkVector4 pos;
        pos.setMul4(spheres->m_pos, m_floatToIntScale);

        hkIntUnion64 converted;
        hkVector4Util::convertToUint16(spheres->m_pos, m_floatToIntOffsetFloorCorrected, m_floatToIntScale, converted);
        const int32_t x = converted.u16[0];
        const int32_t z = converted.u16[2];

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

hkTriSampledHeightFieldBvTreeShapeCinfo::hkTriSampledHeightFieldBvTreeShapeCinfo() {}
