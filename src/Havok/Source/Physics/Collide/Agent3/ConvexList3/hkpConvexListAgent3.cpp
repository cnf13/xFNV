#include "Havok/Source/Physics/Collide/Agent3/ConvexList3/hkpConvexListAgent3.h"

#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGsk.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListUtils.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/hkpAgentMachineUtil.h"
#include "Havok/Source/Physics/Collide/Agent3/PredGskAgent3/hkpPredGskAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexList/hkpConvexListShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/Continuous/hkpContinuousGsk.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"
#include "SDK/MSVC/string.h"

// The agent works in one of two modes. In stream mode the convex list's children are collided one by one through the
// agent's track of sub-agents; after the stream mode counter runs out, GSK is tried on the list's hull, and when it
// finds the bodies apart the agent switches to GSK mode (the track destroyed, the output rolled back). In GSK mode the
// predictive GSK agent runs on the hull, its points mapped to the children by a hkpMapPointsToSubShapeContactMgr; a
// point no child owns, or a deep penetration, switches back to stream mode. The agent data is the hkpGskCache, then the
// predictive GSK manifold in GSK mode or hkConvexListAgent3Data in stream mode.
hkpAgentData* hkConvexListAgent3::process(
    const hkpAgent3ProcessInput& input,
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkVector4* separatingNormal,
    hkpProcessCollisionOutput& output
) {
    HK_TIMER_BEGIN_LIST("CvxLst", "Tim");
    const hkpConvexListShape* cvxList = static_cast<const hkpConvexListShape*>(input.m_bodyB->m_shape);
    hkpProcessCollisionOutputBackup backup(output);
    hkpGskCache* gskCache = static_cast<hkpGskCache*>(agentData);
    // EXE-INLINED none: hkConvexListAgent3::getConvexListaData
    hkConvexListAgent3Data* listData = static_cast<hkConvexListAgent3Data*>(static_cast<void*>(gskCache + 1));
    hkpAgentData* result;
    // EXE-INLINED none: hkConvexListAgent3::isInGskMode
    if (gskCache->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE) {
        goto gskMode;
    }

streamMode:
    HK_TIMER_SPLIT_LIST("Stream");
    {
        int16_t counter = listData->m_inStreamModeCounter;
        listData->m_inStreamModeCounter = static_cast<int16_t>(counter - 1);
        if (counter >= 0) {
            goto processStream;
        }
    }
    listData->m_inStreamModeCounter = 25;
    {
        hkTransform aTb;
        aTb.setMulInverseMul(*input.m_bodyA->m_motion.pMotion, *input.m_bodyB->m_motion.pMotion);
        hkpGsk::GetClosesetPointInput gskInput;
        gskInput.m_shapeA.m_storage = static_cast<const hkpConvexShape*>(input.m_bodyA->m_shape);
        gskInput.m_shapeB.m_storage = static_cast<const hkpConvexShape*>(input.m_bodyB->m_shape);
        gskInput.m_aTb.m_storage = &aTb;
        gskInput.m_transformA.m_storage = input.m_bodyA->m_motion.pMotion;
        gskInput.m_collisionTolerance.m_storage = input.m_input->m_tolerance;
        hkVector4 pointOnB;
        if (hkpGsk::getClosestPoint(gskInput, *gskCache, *separatingNormal, pointOnB) == HK_SUCCESS) {
            goto processStream;
        }
    }
    // EXE-INLINED none: hkConvexListAgent3::switchToGskMode
    hkAgent1nMachine_Destroy(
        listData->m_agentTrack, input.m_input->m_dispatcher, input.m_contactMgr, *output.m_constraintOwner.m_storage
    );
    gskCache->m_gskFlags |= hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE;
    static_cast<hkPredGskAgent3Data*>(agentData)->m_gskManifold.init();
    backup.rollbackOutput(*input.m_bodyA, *input.m_bodyB, output, input.m_contactMgr);

gskMode:
    {
        hkpMapPointsToSubShapeContactMgr mgr;
        // EXE-INLINED none: hkpMapPointsToSubShapeContactMgr::hkpMapPointsToSubShapeContactMgr
        mgr.m_type = hkpContactMgr::TYPE_CONVEX_LIST_CONTACT_MGR;
        mgr.m_contactMgr = input.m_contactMgr;
        mgr.m_invalidPointHit.m_bool = 0;
        hkpAgent3ProcessInput gskInput(input);
        gskInput.m_contactMgr = &mgr;
        result = hkPredGskAgent3::process(gskInput, entry, agentData, separatingNormal, output);
        gskInput.m_contactMgr = mgr.m_contactMgr;
        if (!mgr.m_invalidPointHit.m_bool) {
            if (static_cast<hkPredGskAgent3Data*>(agentData)->m_gskManifold.m_numContactPoints == 0) {
                goto done;
            }
            if (gskCache->m_gskFlags & hkpGskCache::GSK_FLAGS_PROCESS_FUNCTION_CALLED) {
                goto done;
            }
            // Back to stream mode when the bodies penetrate deeper than twice the smaller allowed penetration.
            float penetrationB = input.m_bodyB->getRootCollidable()->getAllowedPenetrationDepth();
            float penetrationA = input.m_bodyA->getRootCollidable()->getAllowedPenetrationDepth();
            float minPenetration = hkMath::min2(penetrationA, penetrationB);
            float twicePenetration = minPenetration + minPenetration;
            if (!(-twicePenetration > separatingNormal->m_quad[3])) {
                goto done;
            }
        }
    }
    // EXE-INLINED none: hkConvexListAgent3::switchToStreamMode
    // EXE-INLINED 0xD33600
    if (backup.m_toi.m_time.m_storage != output.m_toi.m_time.m_storage) {
        input.m_contactMgr->removeToiImpl(*output.m_constraintOwner.m_storage, output.m_toi.m_properties);
        // EXE-INLINED 0xD33330
        output.m_toi.m_contactPoint = backup.m_toi.m_contactPoint;
        output.m_toi.m_time = backup.m_toi.m_time;
        output.m_toi.m_seperatingVelocity = backup.m_toi.m_seperatingVelocity;
        __builtin_memcpy(&output.m_toi.m_gskCache, &backup.m_toi.m_gskCache, sizeof(hkGskCache16));
        output.m_toi.m_properties = backup.m_toi.m_properties;
    }
    output.m_firstFreeContactPoint.m_storage = backup.m_firstPoint;
    if (output.m_potentialContacts.m_storage) {
        rt::memcpy(
            output.m_potentialContacts.m_storage, &backup.m_weldingInformation,
            sizeof(hkpProcessCollisionOutput::PotentialInfo)
        );
    }
    hkPredGskAgent3::cleanup(entry, agentData, input.m_contactMgr, *output.m_constraintOwner.m_storage);
    gskCache->m_gskFlags &= static_cast<uint8_t>(~hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE);
    // EXE-INLINED none: placement new of hkConvexListAgent3Data (hkpAgent1nTrack's constructor: an empty sector
    // array, flagged DONT_DEALLOCATE)
    if (listData != nullptr) {
        listData->m_agentTrack.m_sectors.m_data = nullptr;
        listData->m_agentTrack.m_sectors.m_size = 0;
        listData->m_agentTrack.m_sectors.m_capacityAndFlags =
            static_cast<int32_t>(hkArray<hkpAgent1nSector*>::DONT_DEALLOCATE_FLAG);
    }
    hkAgent1nMachine_Create(listData->m_agentTrack);
    listData->m_inStreamModeCounter = 25;
    goto streamMode;

processStream:
    {
        separatingNormal->m_quad[3] = -1.70141e+38f;
        int32_t numChildren = cvxList->m_childShapes.m_size;
        uint32_t* hitList = hkAllocateStack<uint32_t>(numChildren + 1, nullptr);
        for (int32_t i = 0; i < numChildren; i++) {
            hitList[i] = static_cast<uint32_t>(i);
        }
        hitList[numChildren] = HK_INVALID_SHAPE_KEY;
        hkAgent1nMachine_Process(listData->m_agentTrack, input, cvxList, hitList, output);
        hkDeallocateStack(hitList);
        result = listData + 1;
    }

done:
    gskCache->m_gskFlags |= hkpGskCache::GSK_FLAGS_PROCESS_FUNCTION_CALLED;
    HK_TIMER_END_LIST();
    // The next agent's data starts at the next 16-byte boundary of the untyped agent stream.
    return reinterpret_cast<hkpAgentData*>((reinterpret_cast<uintptr_t>(result) + 15) & ~static_cast<uintptr_t>(15));
}

hkpAgentData* hkConvexListAgent3::cleanup(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner
) {
    return hkPredGskAgent3::cleanup(entry, agentData, mgr, constraintOwner);
}

// The agent data begins with the hkpGskCache, as in the SDK's getGskFlag.
void hkConvexListAgent3::removePoint(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove) {
    // EXE-INLINED none: hkConvexListAgent3::isInGskMode
    if (static_cast<const hkpGskCache*>(agentData)->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE) {
        hkPredGskAgent3::removePoint(entry, agentData, idToRemove);
    }
}

// Deliberately keeps an uninitialised read, as the game does: registerAgent3 copies all of f, so the bytes after
// m_reusePreviousEntry, never written here, land in the dispatcher's table.
void hkConvexListAgent3::registerAgent3(hkpCollisionDispatcher* dispatcher) {
    alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
    f.m_isPredictive.m_bool = 0;
    f.m_ignoreSymmetricVersion.m_bool = 0;
    f.m_reusePreviousEntry.m_bool = 0;
    f.m_updateFilterFunc = nullptr;
    f.m_calcStatisticsFunc = nullptr;
    f.m_invalidateTimFunc = nullptr;
    f.m_warpTimeFunc = nullptr;
    f.m_sepNormalFunc = nullptr;
    hkConvexListAgent3::initAgentFunc(f);
    dispatcher->registerAgent3(f, HK_SHAPE_CONVEX, HK_SHAPE_CONVEX_LIST);
    dispatcher->registerAgent3(f, HK_SHAPE_CONVEX_LIST, HK_SHAPE_CONVEX_LIST);
}

// Only the GSK manifold of an agent in GSK mode keeps points with ids to commit.
void hkConvexListAgent3::commitPotential(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToCommit) {
    // EXE-INLINED none: hkConvexListAgent3::isInGskMode
    if (static_cast<const hkpGskCache*>(agentData)->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE) {
        hkPredGskAgent3::commitPotential(entry, agentData, idToCommit);
    }
}

// As commitPotential: only the GSK manifold keeps points that can become zombies.
void hkConvexListAgent3::createZombie(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToConvert) {
    // EXE-INLINED none: hkConvexListAgent3::isInGskMode
    if (static_cast<const hkpGskCache*>(agentData)->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE) {
        hkPredGskAgent3::createZombie(entry, agentData, idToConvert);
    }
}

// In stream mode the sub-agents' track is visited with the new filter; in GSK mode there is nothing to filter. The
// list's data follows the agent data's hkpGskCache (the SDK's getConvexListaData: a byte offset on the untyped agent
// data, hence the cast).
void hkConvexListAgent3::updateFilter(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner
) {
    // EXE-INLINED none: hkConvexListAgent3::isInGskMode
    if ((static_cast<const hkpGskCache*>(agentData)->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE) ==
        0) {
        hkpAgent1nMachine_VisitorInput vin;
        vin.m_containerShapeB = bodyB.m_shape->getContainer();
        vin.m_bodyA = &bodyA;
        vin.m_input = &input;
        vin.m_collectionBodyB = &bodyB;
        vin.m_contactMgr = mgr;
        vin.m_constraintOwner = &constraintOwner;
        // EXE-INLINED none: hkConvexListAgent3::getAgent1nTrack
        hkConvexListAgent3Data* pData =
            reinterpret_cast<hkConvexListAgent3Data*>(static_cast<hkpGskCache*>(agentData) + 1);
        hkAgent1nMachine_UpdateShapeCollectionFilter(pData->m_agentTrack, vin);
    }
}

// The agent data begins with the hkpGskCache, as in the SDK's getGskFlag; the convex list data follows it (the
// SDK's getConvexListaData), and only an agent in stream mode has a track of sub-agents.
void hkConvexListAgent3::calcStatistics(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCollisionInput& input,
    hkStatisticsCollector* collector
) {
    // EXE-INLINED none: hkConvexListAgent3::isInGskMode
    if (!(static_cast<const hkpGskCache*>(agentData)->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE)) {
        collector->pushDir("hkConvexListAgent3");
        // EXE-INLINED none: hkConvexListAgent3::getAgent1nTrack
        hkConvexListAgent3Data* listData =
            static_cast<hkConvexListAgent3Data*>(static_cast<void*>(static_cast<hkpGskCache*>(agentData) + 1));
        hkpAgentMachineUtil::calc1nStatistics(&listData->m_agentTrack, &input, collector);
        collector->popDir();
    }
}

// The agent data begins with the hkpGskCache; the convex list data follows it (the SDK's getConvexListaData).
void hkConvexListAgent3::invalidateTim(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input) {
    // EXE-INLINED none: hkConvexListAgent3::isInGskMode
    if (!(static_cast<const hkpGskCache*>(agentData)->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE)) {
        // EXE-INLINED none: hkConvexListAgent3::getAgent1nTrack
        hkConvexListAgent3Data* listData =
            static_cast<hkConvexListAgent3Data*>(static_cast<void*>(static_cast<hkpGskCache*>(agentData) + 1));
        hkAgent1nMachine_InvalidateTim(listData->m_agentTrack, input);
    }
}

// In GSK mode the predictive GSK agent's normal; in stream mode the identity's x axis with a constant distance.
void hkConvexListAgent3::sepNormal(
    const hkpAgent3Input& input,
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkVector4& separatingNormalOut
) {
    // EXE-INLINED none: hkConvexListAgent3::isInGskMode
    if (static_cast<const hkpGskCache*>(agentData)->m_gskFlags & hkpGskCache::GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE) {
        hkPredGskAgent3::sepNormal(input, entry, agentData, separatingNormalOut);
    }
    else {
        separatingNormalOut = hkTransformIdentity.m_rotation.m_col0;
        separatingNormalOut.m_quad[3] = -1.70141e+38f;
    }
}

hkpAgent1nTrack* hkConvexListAgent3::getAgent1nTrack(const hkpAgentEntry* entry, hkpAgentData* data) {
    return &static_cast<hkConvexListAgent3Data*>(static_cast<void*>(static_cast<hkpGskCache*>(data) + 1))->m_agentTrack;
}

const hkpAgent1nTrack* hkConvexListAgent3::getAgent1nTrack(const hkpAgentEntry* entry, const hkpAgentData* data) {
    return &static_cast<const hkConvexListAgent3Data*>(
                static_cast<const void*>(static_cast<const hkpGskCache*>(data) + 1)
    )
                ->m_agentTrack;
}

// m_toi default-constructed (time FLT_MAX), then the output's first free point, time of impact and, when it has
// them, potential contacts copied.
hkpProcessCollisionOutputBackup::hkpProcessCollisionOutputBackup(const hkpProcessCollisionOutput& output)
  : m_firstPoint(output.m_firstFreeContactPoint.m_storage) {
    m_toi = output.m_toi;
    if (output.m_potentialContacts.m_storage) {
        m_weldingInformation = *output.m_potentialContacts.m_storage;
    }
}

// The time of impact is copied by ToiInfo's copy assignment and the potential contacts by the CRT's memcpy.
void hkpProcessCollisionOutputBackup::rollbackOutput(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    hkpProcessCollisionOutput& output,
    hkpContactMgr* mgr
) {
    if (output.m_toi.m_time.m_storage != m_toi.m_time.m_storage) {
        mgr->removeToi(*output.m_constraintOwner.m_storage, output.m_toi.m_properties);
        output.m_toi.operator=(m_toi);
    }
    output.m_firstFreeContactPoint.m_storage = m_firstPoint;
    hkpProcessCollisionOutput::PotentialInfo* potentialContacts = output.m_potentialContacts.m_storage;
    if (potentialContacts) {
        rt::memcpy(potentialContacts, &m_weldingInformation, sizeof(m_weldingInformation));
    }
}

// Member-wise copy; the padding after m_seperatingVelocity is not copied. The hkGskCache16 is copied with all its 16
// bytes, the tail after hkpGskCache's 12 included, hence the 16-byte builtin copy.
hkpProcessCollisionData::ToiInfo& hkpProcessCollisionData::ToiInfo::operator=(const ToiInfo& __that) {
    m_contactPoint = __that.m_contactPoint;
    m_time = __that.m_time;
    m_seperatingVelocity = __that.m_seperatingVelocity;
    __builtin_memcpy(&m_gskCache, &__that.m_gskCache, sizeof(hkGskCache16));
    m_properties = __that.m_properties;
    return *this;
}
