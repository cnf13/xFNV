#include "Havok/Source/Physics/Collide/Shape/HeightField/StorageSampledHeightField/hkpStorageSampledHeightFieldShape.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpHavokSnapshot.h"

#include <bit>
#include <xmmintrin.h>

// Havok SDK: the type info's finish callback of hkpStorageSampledHeightFieldShape.
static void finishLoadedObjecthkpStorageSampledHeightFieldShape(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by target).
static const void* getVtablehkpStorageSampledHeightFieldShape();
// Havok SDK: the type info's cleanup callback of hkpStorageSampledHeightFieldShape.
static void cleanupLoadedObjecthkpStorageSampledHeightFieldShape(void* p);

// The reflected members of hkpStorageSampledHeightFieldShape.
using hkpStorageSampledHeightFieldShapeClass_MembersArray = const hkClassMember[2];
static hkpStorageSampledHeightFieldShapeClass_MembersArray hkpStorageSampledHeightFieldShapeClass_Members{
    {"storage", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, {0}, 0x60, nullptr},
    {"triangleFlip", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x64, nullptr}
};
hkClass hkpStorageSampledHeightFieldShapeClass(
    "hkpStorageSampledHeightFieldShape",
    &hkpSampledHeightFieldShapeClass,
    0x80,
    nullptr,
    0,
    nullptr,
    0,
    hkpStorageSampledHeightFieldShapeClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpStorageSampledHeightFieldShapeTypeInfo = {
    "hkpStorageSampledHeightFieldShape",
    finishLoadedObjecthkpStorageSampledHeightFieldShape,
    cleanupLoadedObjecthkpStorageSampledHeightFieldShape,
    getVtablehkpStorageSampledHeightFieldShape(),
    &typeid(hkpStorageSampledHeightFieldShape),
    sizeof(hkpStorageSampledHeightFieldShape)
};

// The type info's cleanup callback for hkpStorageSampledHeightFieldShape: the virtual destructor through the vtable,
// without freeing.
static void cleanupLoadedObjecthkpStorageSampledHeightFieldShape(void* p) {
    static_cast<hkpStorageSampledHeightFieldShape*>(p)->~hkpStorageSampledHeightFieldShape();
}

// The type info's finish-loaded-object callback for hkpStorageSampledHeightFieldShape: runs its finish constructor.
static void finishLoadedObjecthkpStorageSampledHeightFieldShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpStorageSampledHeightFieldShape(flag);
    }
}

static const void* getVtablehkpStorageSampledHeightFieldShape() {
    return hkGetVtableOf<hkpStorageSampledHeightFieldShape>();
}

hkpStorageSampledHeightFieldShape::hkpStorageSampledHeightFieldShape(const hkpSampledHeightFieldShape* hf)
  : hkpSampledHeightFieldShape(), bowned(true) {
    m_heightfieldType.m_storage = HEIGHTFIELD_STORAGE;
    m_xRes = hf->m_xRes;
    m_zRes = hf->m_zRes;
    hkBool triangleFlip;
    hf->getTriangleFlipImpl(&triangleFlip);
    m_triangleFlip = triangleFlip;
    m_heightCenter = hf->m_heightCenter;
    m_intToFloatScale = hf->m_intToFloatScale;
    m_floatToIntScale = hf->m_floatToIntScale;
    m_floatToIntOffsetFloorCorrected = hf->m_floatToIntOffsetFloorCorrected;
    m_extents = hf->m_extents;
    // new float[xRes * zRes] through ourMalloc: a count whose size in bytes does not fit 32 bits asks for 0xFFFFFFFF.
    const uint64_t ulSize = static_cast<uint64_t>(static_cast<uint32_t>(m_xRes * m_zRes)) * sizeof(float);
    m_storage = static_cast<float*>(ourMalloc(ulSize > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(ulSize)));
    for (int32_t z = 0; z < m_zRes; z++) {
        for (int32_t x = 0; x < m_xRes; x++) {
            m_storage[z * m_xRes + x] = hf->getHeightAtImpl(x, z);
        }
    }
}

hkpStorageSampledHeightFieldShape::hkpStorageSampledHeightFieldShape(
    const hkpSampledHeightFieldBaseCinfo& ci,
    float* storage,
    float offset
)
  : hkpSampledHeightFieldShape(ci, HEIGHTFIELD_STORAGE) {
    m_triangleFlip.m_bool = 0;
    bowned = false;
    m_storage = storage;
    invZRes = static_cast<int32_t>(static_cast<double>(m_zRes) - 1.0);
    fOffset = offset;
}

hkpStorageSampledHeightFieldShape::hkpStorageSampledHeightFieldShape(
    const hkpSampledHeightFieldBaseCinfo& ci,
    hkArray<float>& samples,
    float offset
)
  : hkpSampledHeightFieldShape(ci, HEIGHTFIELD_STORAGE) {
    m_triangleFlip.m_bool = 0;
    bowned = false;
    m_storage = samples.m_data;
    invZRes = static_cast<int32_t>(static_cast<double>(m_zRes) - 1.0);
    fOffset = offset;
}

hkpStorageSampledHeightFieldShape::~hkpStorageSampledHeightFieldShape() {
    if (bowned) {
        ourFree(m_storage);
    }
}

int32_t hkpStorageSampledHeightFieldShape::calcSizeForSpu(const CalcSizeForSpuInput& a1, int32_t a2) const {
    return (a2 >= 128) ? 128 : (-1);
}

// hkSampledHeightFieldShape_collideSpheres for this class: the heights and the flip come from the virtual
// getHeightAtImpl and getTriangleFlipImpl.
void hkpStorageSampledHeightFieldShape::collideSpheres(
    const hkpHeightFieldShape::CollideSpheresInput& input,
    hkVector4* outputArray
) const {
    const hkSphere* spheres = input.m_spheres;
    hkVector4* output = outputArray;

    hkVector4 defNormal;
    defNormal.set(0.0f, 1.0f, 0.0f, 3.40282e+38f);

    for (int32_t i = input.m_numSpheres - 1; i >= 0; output++, spheres++, i--) {
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
        // EXE-INLINED 0x62AEA0
        // hkVector4::normalize3: the inverse length from rsqrtss and one Newton step, 0 for a zero length.
        const hkQuadReal scaled = output->m_quad;
        const hkQuadReal squares = _mm_mul_ps(scaled, scaled);
        const hkQuadReal len2 = _mm_add_ss(
            _mm_shuffle_ps(squares, squares, _MM_SHUFFLE(2, 2, 2, 2)),
            _mm_add_ss(_mm_shuffle_ps(squares, squares, _MM_SHUFFLE(1, 1, 1, 1)), squares)
        );
        const hkQuadReal approx = _mm_rsqrt_ss(len2);
        const hkQuadReal len2EqualsZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
        const hkQuadReal refined = _mm_mul_ss(
            _mm_mul_ss(_mm_set_ss(0.5f), approx),
            _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx))
        );
        const hkQuadReal lenInv = _mm_andnot_ps(len2EqualsZero, refined);
        output->m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, _MM_SHUFFLE(0, 0, 0, 0)), scaled);

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

            // EXE-INLINED 0x62B0A0
            // hkVector4::dot3: the products' lanes summed, y + x, then z + that.
            const hkQuadReal products = _mm_mul_ps(vectorToTri.m_quad, output->m_quad);
            const float distanceToTri = _mm_cvtss_f32(_mm_add_ss(
                _mm_shuffle_ps(products, products, _MM_SHUFFLE(2, 2, 2, 2)),
                _mm_add_ss(_mm_shuffle_ps(products, products, _MM_SHUFFLE(1, 1, 1, 1)), products)
            ));
            absoluteDepth = distanceToTri - spheres->m_pos(3);
        }
        output[0](3) = absoluteDepth;
    }
}

float hkpStorageSampledHeightFieldShape::getHeightAtImpl(int32_t x, int32_t z) const {
    return (m_storage[((3 * x) + 2) + (3 * (m_xRes * (invZRes - z)))] + fOffset);
}

// The shape never flips its triangles: the result is false whatever m_triangleFlip holds, as the game does.
hkBool* hkpStorageSampledHeightFieldShape::getTriangleFlipImpl(hkBool* arResult) const {
    arResult->m_bool = false;
    return arResult;
}

const hkClass& hkpStorageSampledHeightFieldShape::staticClass() {
    return hkpStorageSampledHeightFieldShapeClass;
}
