#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

class hkAabb;
class hkBitField;

// A shape made of a list of child shapes, each with its own collision filter info; up to 256 children can be
// disabled through the m_enabledChildren bit field.
class hkpListShape : public hkpShapeCollection {
public:
    // One child of the list: its shape, filter info and the number of shapes it stands for.
    struct ChildInfo {
        // Havok SDK signature (hkpCollide.lib, tied by target).
        static const hkClass& staticClass();

        const hkpShape* m_shape;
        uint32_t m_collisionFilterInfo;
        // The child's SPU size: calcSizeForSpu and setShapes write it as one 32-bit value.
        int32_t m_shapeSize;
        int32_t m_numChildShapes;
    };

    // The list's flags (the SDK's values).
    enum ListShapeFlags : int32_t {
        ALL_FLAGS_CLEAR = 0,
        // The SPU build's: do not cache the child infos.
        DISABLE_SPU_CACHE_FOR_LIST_CHILD_INFO = 1 << 0,
    };

    // The SDK's bound of the children m_enabledChildren can disable; isChildEnabled takes every child from it on as
    // enabled.
    enum { MAX_DISABLED_CHILDREN = 256 };

    // Builds a list over aiNumShapes child shapes, taking a reference on each only when the
    // policy asks.
    hkpListShape(const hkpShape* const* apShapes, int32_t aiNumShapes, hkpShapeContainer::ReferencePolicy aePolicy);
    hkpListShape(hkFinishLoadedObjectFlag flag);
    ~hkpListShape() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput&, int32_t) const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
    void castRayWithCollectorImpl(
        const hkpShapeRayCastInput& input,
        const hkpCdBody& cdBody,
        hkpRayHitCollector& collector
    ) const override;
    // hkpShapeContainer's vtable at +0x10.
    int32_t getNumChildShapes() const override;
    // hkpShapeContainer's vtable at +0x10.
    uint32_t getFirstKey() const override;
    // hkpShapeContainer's vtable at +0x10.
    uint32_t getNextKey(uint32_t) const override;
    // hkpShapeContainer's vtable at +0x10.
    uint32_t getCollisionFilterInfo(uint32_t) const override;
    // hkpShapeContainer's vtable at +0x10.
    const hkpShape* getChildShape(uint32_t, char (&)[512]) const override;

    // Sets the collision filter info of child index.
    void setCollisionFilterInfo(uint32_t index, uint32_t filterInfo);
    // Writes the union of the children's boxes to t and stores its centre and half extents in the shape.
    void recalcAabbExtents(hkAabb& t);

    // Recomputes the stored centre and half extents of the children's box.
    void recalcAabbExtents();
    const hkpShape* getChildShapeInl(int32_t i) const;
    void setShapes(
        const hkpShape* const* shapeArray,
        int32_t numShapes,
        const uint32_t* filterInfo,
        hkpShapeContainer::ReferencePolicy ref
    );
    // Nonzero when child index is enabled (the SDK's hkBool32).
    int32_t isChildEnabled(uint32_t index) const {
        if (index < MAX_DISABLED_CHILDREN) {
            return m_enabledChildren[index >> 5] & (1 << (index & 0x1F));
        }
        return 1;
    }
    void disableChild(uint32_t index);
    void enableChild(uint32_t index);
    void setEnabledChildren(const hkBitField& enabledChildren);

    // The reflection class hkpListShapeClass.
    static const hkClass& staticClass();

    hkArray<ChildInfo> m_childInfo;
    uint16_t m_flags;
    uint16_t m_numDisabledChildren;
    hkVector4 m_aabbHalfExtents;
    hkVector4 m_aabbCenter;
    uint32_t m_enabledChildren[8];
};

extern hkClass hkpListShapeClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpListShapeChildInfoTypeInfo;
extern hkTypeInfo hkpListShapeTypeInfo;
