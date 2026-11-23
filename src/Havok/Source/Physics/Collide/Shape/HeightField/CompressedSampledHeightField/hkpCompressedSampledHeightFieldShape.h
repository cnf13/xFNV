#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <stdint.h>

void cleanupLoadedObjecthkpCollisionFilterList(void* p);

// A sampled heightfield storing its heights as 16-bit values, scaled by m_scale and offset by m_offset.
class hkpCompressedSampledHeightFieldShape : public hkpSampledHeightFieldShape {
public:
    hkpCompressedSampledHeightFieldShape(const hkpSampledHeightFieldShape* hf);
    hkpCompressedSampledHeightFieldShape(
        const hkpSampledHeightFieldBaseCinfo& ci,
        hkArray<uint16_t>& samples,
        float quantizationOffset,
        float quantizationScale
    );

    // The SDK's finish constructor of a loaded object.
    hkpCompressedSampledHeightFieldShape(hkFinishLoadedObjectFlag aFlag)
      : hkpSampledHeightFieldShape(aFlag), m_storage(aFlag) {
        if (aFlag.m_finishing) {
            m_heightfieldType.m_storage = HEIGHTFIELD_COMPRESSED;
        }
    }

    ~hkpCompressedSampledHeightFieldShape() override;

    // Havok SDK signature (hkpCollide.lib, tied by vtable).
    int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
    void collideSpheres(const CollideSpheresInput& input, hkVector4* outputArray) const override;

    // The height sample at (x, z), decompressed: the stored value scaled by m_scale and offset by m_offset.
    float getHeightAtImpl(int32_t x, int32_t z) const override {
        const int32_t index = z * m_xRes + x;
        return _decompress(m_storage.m_data[index]);
    }

    float _decompress(uint16_t comp) const {
        return static_cast<float>(comp) * m_scale + m_offset;
    }

    hkBool getTriangleFlipImpl() const override;

    static const hkClass& staticClass();

    hkArray<uint16_t> m_storage;
    hkBool m_triangleFlip;
    float m_offset;
    float m_scale;
};

extern hkClass hkpCompressedSampledHeightFieldShapeClass;
extern hkTypeInfo hkpCompressedSampledHeightFieldShapeTypeInfo;
extern hkClass hkpCollisionFilterListClass;
extern hkTypeInfo hkpCollisionFilterListTypeInfo;
