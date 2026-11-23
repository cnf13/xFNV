#include "Havok/Source/Physics/Collide/Shape/HeightField/CompressedSampledHeightField/hkpCompressedSampledHeightFieldShape.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Filter/List/hkpCollisionFilterList.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldBaseCinfo.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <bit>
#include <xmmintrin.h>

// The base's fields copied from hf, the heights' range taken as m_offset and m_scale, then each height quantized to
// 16 bits.
hkpCompressedSampledHeightFieldShape::hkpCompressedSampledHeightFieldShape(const hkpSampledHeightFieldShape* hf)
  : hkpSampledHeightFieldShape() {
    m_heightfieldType.m_storage = HEIGHTFIELD_COMPRESSED;
    m_xRes = hf->m_xRes;
    m_zRes = hf->m_zRes;
    hkBool triangleFlip = hf->getTriangleFlipImpl();
    m_triangleFlip = triangleFlip;
    m_heightCenter = hf->m_heightCenter;
    m_intToFloatScale = hf->m_intToFloatScale;
    m_floatToIntScale = hf->m_floatToIntScale;
    m_floatToIntOffsetFloorCorrected = hf->m_floatToIntOffsetFloorCorrected;
    m_extents = hf->m_extents;
    m_storage.setSize(m_xRes * m_zRes);
    float fMin = HK_REAL_MAX;
    float fMax = -HK_REAL_MAX;
    for (int32_t z = 0; z < m_zRes; z++) {
        for (int32_t x = 0; x < m_xRes; x++) {
            const float fHeight = hf->getHeightAtImpl(x, z);
            if (fMin > fHeight) {
                fMin = fHeight;
            }
            if (fMax < fHeight) {
                fMax = fHeight;
            }
        }
    }
    m_offset = fMin;
    m_scale = (fMax - fMin) / 65535.0;
    for (int32_t z = 0; z < m_zRes; z++) {
        for (int32_t x = 0; x < m_xRes; x++) {
            const float fHeight = hf->getHeightAtImpl(x, z);
            m_storage.m_data[z * m_xRes + x] = static_cast<uint16_t>(
                static_cast<int32_t>((fHeight - m_offset) / m_scale)
            );
        }
    }
}

hkpCompressedSampledHeightFieldShape::hkpCompressedSampledHeightFieldShape(
    const hkpSampledHeightFieldBaseCinfo& ci,
    hkArray<uint16_t>& samples,
    float quantizationOffset,
    float quantizationScale
)
  : hkpSampledHeightFieldShape(ci, HEIGHTFIELD_COMPRESSED) {
    m_triangleFlip.m_bool = 0;
    m_storage = samples;
    m_scale = quantizationScale;
    m_offset = quantizationOffset;
}

hkpCompressedSampledHeightFieldShape::~hkpCompressedSampledHeightFieldShape() {}

int32_t hkpCompressedSampledHeightFieldShape::calcSizeForSpu(
    const CalcSizeForSpuInput& input,
    int32_t spuBufferSizeLeft
) const {
    return (spuBufferSizeLeft >= 128) ? 128 : (-1);
}

// The SDK's hkSampledHeightFieldShape_collideSpheres for this class: per sphere, the default normal (up, depth
// HK_REAL_MAX); for a sphere over the field, the normal of the triangle under it from the four corner heights (read
// through the vtable, as the triangle flip), scaled and normalised, and its depth below the surface: measured
// vertically, or along the normal from the triangle's corner when m_useProjectionBasedHeight is set.
void hkpCompressedSampledHeightFieldShape::collideSpheres(
    const hkpHeightFieldShape::CollideSpheresInput& input,
    hkVector4* outputArray
) const {
    const hkSphere* spheres = input.m_spheres;
    hkVector4* output = outputArray;
    hkVector4 defNormal;
    defNormal.set(0.0f, 1.0f, 0.0f, HK_REAL_MAX);
    for (int32_t i = input.m_numSpheres - 1; i >= 0; ++output, ++spheres, --i) {
        *output = defNormal;
        hkVector4 pos;
        pos.setMul4(spheres->m_pos, m_floatToIntScale);
        hkIntUnion64 converted;
        hkVector4Util::convertToUint16(spheres->m_pos, m_floatToIntOffsetFloorCorrected, m_floatToIntScale, converted);
        const int32_t x = converted.u16[0];
        const int32_t z = converted.u16[2];
        if (x >= m_xRes - 1 || z >= m_zRes - 1) {
            continue;
        }
        float subX = pos(0) - static_cast<float>(x);
        float subZ = pos(2) - static_cast<float>(z);
        float height;
        float vertHeight;
        int32_t vertX;
        float dx;
        float dz;
        hkBool flip = getTriangleFlipImpl();
        if (flip.m_bool) {
            float h00 = getHeightAtImpl(x, z);
            vertHeight = h00;
            vertX = x;
            float h11 = getHeightAtImpl(x + 1, z + 1);
            if (subX > subZ) {
                float h10 = getHeightAtImpl(x + 1, z);
                dx = h10 - h00;
                dz = h11 - h10;
            }
            else {
                float h01 = getHeightAtImpl(x, z + 1);
                dx = h11 - h01;
                dz = h01 - h00;
            }
            height = h00 + subZ * dz + subX * dx;
        }
        else {
            float h10 = getHeightAtImpl(x + 1, z);
            vertHeight = h10;
            vertX = x + 1;
            float h01 = getHeightAtImpl(x, z + 1);
            if (subZ + subX > 1.0) {
                float h11 = getHeightAtImpl(x + 1, z + 1);
                dx = h11 - h01;
                dz = h11 - h10;
                height = h10 + subZ * dz + (subX - 1.0) * dx;
            }
            else {
                float h00 = getHeightAtImpl(x, z);
                dx = h10 - h00;
                dz = h01 - h00;
                height = h00 + subZ * dz + subX * dx;
            }
        }
        (*output)(0) = -dx;
        (*output)(2) = -dz;
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
        (*output)(3) = absoluteDepth;
    }
}

hkBool hkpCompressedSampledHeightFieldShape::getTriangleFlipImpl() const {
    return m_triangleFlip.m_bool != 0;
}

const hkClass& hkpCompressedSampledHeightFieldShape::staticClass() {
    return hkpCompressedSampledHeightFieldShapeClass;
}

// The class registry's finish-loaded-object callback for hkpCollisionFilterList: the null check, then the class's
// finish constructor.
void finishLoadedObjecthkpCollisionFilterList(void* p, int32_t finishing) {
    if (!p) {
        return;
    }
    hkFinishLoadedObjectFlag flag;
    flag.m_finishing = finishing;
    ::new (p) hkpCollisionFilterList(flag);
}
