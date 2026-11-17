#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"

class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionInput;

// The shape phantom that answers its queries directly against the bodies overlapping it, keeping the collision
// details of its last query.
class hkpSimpleShapePhantom : public hkpShapePhantom {
public:
    // The overlapped body's collidable.
    struct CollisionDetail {
        // The reflection class hkpSimpleShapePhantomCollisionDetailClass.
        static const hkClass& staticClass();

        hkpCollidable* m_collidable;
    };

    // ensureDeterministicOrder's sort order: the details by their collidable's broad phase id. The compare is unsigned.
    class OrderByUid {
    public:
        hkBool operator()(const CollisionDetail& a, const CollisionDetail& b) {
            return a.m_collidable->m_broadPhaseHandle.m_id < b.m_collidable->m_broadPhaseHandle.m_id;
        }
    };

    // The reflection's member table.
    using MembersArray = const hkClassMember[2];

    hkpSimpleShapePhantom(const hkpShape* a1, const hkTransform& a2, uint32_t a3);
    hkpSimpleShapePhantom(hkFinishLoadedObjectFlag flag);
    ~hkpSimpleShapePhantom() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    hkpPhantomType getType() const override;
    void addOverlappingCollidable(hkpCollidable* collidable) override;
    hkBool* isOverlappingCollidableAdded(hkBool* arResult, const hkpCollidable* apCollidable) override;
    void removeOverlappingCollidable(hkpCollidable* collidable) override;
    void ensureDeterministicOrder() override;
    hkpPhantom* clone() const override;
    void deallocateInternalArrays() override;
    void setPositionAndLinearCast(
        const hkVector4&,
        const hkpLinearCastInput&,
        hkpCdPointCollector&,
        hkpCdPointCollector*
    ) override;
    void setTransformAndLinearCast(
        const hkTransform&,
        const hkpLinearCastInput&,
        hkpCdPointCollector&,
        hkpCdPointCollector*
    ) override;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
    void getClosestPoints(hkpCdPointCollector& collector, const hkpCollisionInput* input) override;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
    void getPenetrations(hkpCdBodyPairCollector& collector, const hkpCollisionInput* input) override;

    hkArray<CollisionDetail>& getCollisionDetails() {
        return m_collisionDetails;
    }

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    hkArray<CollisionDetail> m_collisionDetails;
    hkBool m_orderDirty;
    uint8_t padding16D[3];

    static MembersArray Members;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpSimpleShapePhantomCollisionDetailTypeInfo;

namespace hkAlgorithm {
    // The collision details' sort by their collidable's broad-phase id; defined in hkpSimpleShapePhantom.cpp.
    int32_t quickSort(hkpSimpleShapePhantom::CollisionDetail* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);
} // namespace hkAlgorithm
