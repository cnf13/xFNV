#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"

class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionAgent;
class hkpCollisionInput;

// A shape phantom that caches, for each body it overlaps, the collision details of the last query, so repeated
// queries do not recompute them; m_orderDirty marks that the cache must be sorted again.
class hkpCachingShapePhantom : public hkpShapePhantom {
public:
    // The cached result of one overlapped body: the collision agent and the body's collidable.
    struct hkpCollisionDetail {
        hkpCollisionAgent* m_agent;
        hkpCollidable* m_collidable;
    };

    // ensureDeterministicOrder's sort order: the details by their collidable's broad phase id, an unsigned compare of
    // the collidables' m_broadPhaseHandle.m_id.
    class OrderByUid {
    public:
        hkBool operator()(const hkpCollisionDetail& a, const hkpCollisionDetail& b) {
            return a.m_collidable->m_broadPhaseHandle.m_id < b.m_collidable->m_broadPhaseHandle.m_id;
        }
    };

    // The reflection's member table.
    using MembersArray = const hkClassMember[2];

    hkpCachingShapePhantom(const hkpShape* a1, const hkTransform& a2, uint32_t a3);
    hkpCachingShapePhantom(hkFinishLoadedObjectFlag aFlag);
    ~hkpCachingShapePhantom() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    hkpPhantomType getType() const override;
    // Havok SDK signature (hkpDynamics.lib, tied by vtable).
    void addOverlappingCollidable(hkpCollidable* collidable) override;
    hkBool isOverlappingCollidableAdded(const hkpCollidable* apCollidable) override;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
    void removeOverlappingCollidable(hkpCollidable* collidable) override;
    void ensureDeterministicOrder() override;
    hkpPhantom* clone() const override;
    void updateShapeCollectionFilter() override;
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

    static const hkClass& staticClass();
    static const hkClass& staticClass_00DFD250();

    // The collision details cached by the last query.
    hkArray<hkpCollisionDetail> m_collisionDetails;
    hkBool m_orderDirty;
    uint8_t padding16D[3];

    static MembersArray Members;
};

// Sorts the cached collision details: the hkAlgorithm::quickSortRecursive instantiation for hkpCollisionDetail and
// OrderByUid, called with the array, the first and last index and the comparator (an empty class, OrderByUid). aFlag
// stands in for the comparator, as an empty class argument carries no bytes in C++.
void quickSortRecursive(
    hkpCachingShapePhantom::hkpCollisionDetail* apDetails,
    int32_t aFirst,
    int32_t aLast,
    uint8_t aFlag
);
