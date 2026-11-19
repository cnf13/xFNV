#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"

#include <xmmintrin.h>

// The entry's agent drops its cached time of impact: a time-caching entry also resets its own separating normal
// (time -1, normal zero) before the agent's invalidate function runs on the agent data after the entry.
void hkAgentNnMachine_InvalidateTimInAgent(hkpAgentNnEntry* entry, const hkpCollisionInput& input) {
    hkpAgentData* agentData;
    switch (entry->m_streamCommand) {
        case hkAgent3::STREAM_CALL:
        case hkAgent3::STREAM_CALL_AGENT:
        case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
        case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM:
            agentData = static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1;
            break;
        case hkAgent3::STREAM_CALL_WITH_TIM:
        case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM: {
            hkpAgentNnMachineTimEntry* timEntry = static_cast<hkpAgentNnMachineTimEntry*>(entry);
            timEntry->m_timeOfSeparatingNormal = -1.0f;
            timEntry->m_separatingNormal.setZero4();
            agentData = timEntry + 1;
            break;
        }
        default: return;
    }
    hkAgent3::InvalidateTimFunc invalidateTim =
        input.m_dispatcher->m_agent3Func[entry->m_agentType].m_invalidateTimFunc;
    if (invalidateTim) {
        invalidateTim(entry, agentData, input);
    }
}

hkpAgentNnEntry* hkAgentNnMachine_FindAgent(const hkpLinkedCollidable* collA, const hkpLinkedCollidable* collB) {
    const hkpLinkedCollidable* v2;
    const hkpLinkedCollidable* v3;
    if (collA->m_collisionEntries.m_size > collB->m_collisionEntries.m_size) {
        v2 = collB;
        v3 = collA;
    }
    else {
        v2 = collA;
        v3 = collB;
    }
    int32_t m_size = v2->m_collisionEntries.m_size;
    int32_t v5 = 0;
    if (m_size <= 0) {
        return nullptr;
    }
    hkpLinkedCollidable::CollisionEntry* m_data = v2->m_collisionEntries.m_data;
    for (hkpLinkedCollidable** i = &m_data->m_partner; *i != v3; i += 2) {
        if (++v5 >= m_size) {
            return nullptr;
        }
    }
    return m_data[v5].m_agentEntry;
}

// Hands out the next HK_AGENT3_AGENT_SIZE-byte entry of the track's last sector, or a fresh sector when that one is
// full.
hkpAgentNnEntry* hkAgentNnMachine_AllocateEntry(hkpAgentNnTrack& track) {
    if (track.m_bytesUsedInLastSector < sizeof(hkpAgentNnSector)) {
        hkpAgentNnSector* sector = track.m_sectors.m_data[track.m_sectors.m_size - 1];
        // The sector's data is untyped storage that entries are carved out of.
        hkpAgentNnEntry* entry = reinterpret_cast<hkpAgentNnEntry*>(sector->m_data + track.m_bytesUsedInLastSector);
        track.m_bytesUsedInLastSector += HK_AGENT3_AGENT_SIZE;
        return entry;
    }
    // The router is loaded twice: once for the sector's block and once for the array's allocator.
    hkpAgentNnSector* sector =
        static_cast<hkpAgentNnSector*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpAgentNnSector)));
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    if (track.m_sectors.m_size ==
        (track.m_sectors.m_capacityAndFlags & hkArrayBase<hkpAgentNnSector*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*router.m_heap, &track.m_sectors, sizeof(hkpAgentNnSector*));
    }
    track.m_sectors.m_data[track.m_sectors.m_size++] = sector;
    track.m_bytesUsedInLastSector = HK_AGENT3_AGENT_SIZE;
    // The sector's data starts at offset 0, so the first entry is that data.
    return reinterpret_cast<hkpAgentNnEntry*>(sector->m_data);
}

// Copies an entry's whole HK_AGENT3_AGENT_SIZE-byte slab into a freshly allocated one and points both collidables'
// collision entries at the copy.
hkpAgentNnEntry* hkAgentNnMachine_CopyAndRelinkAgentEntry(hkpAgentNnTrack& destTrack, hkpAgentNnEntry* entry) {
    hkpAgentNnEntry* newEntry = hkAgentNnMachine_AllocateEntry(destTrack);
    // Copies the slab as untyped words, the agent's own data included.
    uint32_t* dst = reinterpret_cast<uint32_t*>(newEntry);
    const uint32_t* src = reinterpret_cast<const uint32_t*>(entry);
    for (int32_t i = 0; i < HK_AGENT3_AGENT_SIZE / 4; ++i) {
        dst[i] = src[i];
    }
    newEntry->m_collidable[0]->m_collisionEntries.m_data[newEntry->m_agentIndexOnCollidable[0]].m_agentEntry = newEntry;
    newEntry->m_collidable[1]->m_collisionEntries.m_data[newEntry->m_agentIndexOnCollidable[1]].m_agentEntry = newEntry;
    return newEntry;
}

// The track's last entry moves into the freed slot, and both collidables learn its new address; a sector left empty
// goes back to the heap, and the sector array shrinks as hkInplaceArray::optimizeCapacity(4) does.
void hkAgentNnMachine_InternalDeallocateEntry(hkpAgentNnTrack& track, hkpAgentNnEntry* entry) {
    hkpAgentNnSector* lastSector = track.m_sectors.m_data[track.m_sectors.m_size - 1];
    // The SDK's hkAddByteOffset(lastSector->getBegin(), m_bytesUsedInLastSector - HK_AGENT3_AGENT_SIZE): the sector's
    // bytes are untyped storage holding the entries.
    hkpAgentNnEntry* lastEntry =
        reinterpret_cast<hkpAgentNnEntry*>(&lastSector->m_data[track.m_bytesUsedInLastSector - HK_AGENT3_AGENT_SIZE]);
    if (entry != lastEntry) {
        // Copies the whole HK_AGENT3_AGENT_SIZE-byte slab, the agent's data included.
        hkString::memCpy4(entry, lastEntry, HK_AGENT3_AGENT_SIZE / 4);
        // Both indices are read before the first store.
        uint16_t indexB = entry->m_agentIndexOnCollidable[1];
        entry->m_collidable[0]->m_collisionEntries.m_data[entry->m_agentIndexOnCollidable[0]].m_agentEntry = entry;
        entry->m_collidable[1]->m_collisionEntries.m_data[indexB].m_agentEntry = entry;
    }
    track.m_bytesUsedInLastSector -= HK_AGENT3_AGENT_SIZE;
    if (track.m_bytesUsedInLastSector == 0) {
        hkpAgentNnSector* sector = track.m_sectors.m_data[--track.m_sectors.m_size];
        hkMemoryRouter::getInstance().m_heap->blockFree(sector, sizeof(hkpAgentNnSector));
        track.m_bytesUsedInLastSector = sizeof(hkpAgentNnSector);
    }
    // EXE-INLINED none: hkInplaceArray<hkpAgentNnSector*, 1>::optimizeCapacity(4)
    if (track.m_sectors.m_capacityAndFlags >= 0) {
        uint32_t uiCapacity = track.m_sectors.m_size + 3;
        uiCapacity |= uiCapacity >> 16;
        uiCapacity |= uiCapacity >> 8;
        uiCapacity |= uiCapacity >> 4;
        uiCapacity |= uiCapacity >> 2;
        uiCapacity |= uiCapacity >> 1;
        int32_t newCapacity = static_cast<int32_t>(uiCapacity + 1);
        if (newCapacity < 1) {
            // The SDK's cast: _reduce takes the inplace storage as raw bytes.
            hkArrayUtil::_reduce(
                *hkMemoryRouter::getInstance().m_heap, &track.m_sectors, sizeof(hkpAgentNnSector*),
                reinterpret_cast<char*>(track.m_sectors.m_storage), 1
            );
        }
        else if (
            newCapacity <
            static_cast<int32_t>(track.m_sectors.m_capacityAndFlags & hkArrayBase<hkpAgentNnSector*>::CAPACITY_MASK)
        ) {
            hkArrayUtil::_reduce(
                *hkMemoryRouter::getInstance().m_heap, &track.m_sectors, sizeof(hkpAgentNnSector*), nullptr, newCapacity
            );
        }
    }
}

// The agent's destroy function runs for the stream commands that call an agent (agent data after a 0x20-byte padded
// entry, or a 0x30-byte time-caching one). Then the entry leaves both collidables' agent lists: the last pair moves
// into its slot, the moved pair's agent learns its new index, and the list's storage shrinks to the next power of two
// above size + 3 when that is below its capacity. Last the entry's slab goes back to the track.
void hkAgentNnMachine_DestroyAgent(
    hkpAgentNnTrack& track,
    hkpAgentNnEntry* entry,
    hkpCollisionDispatcher* dispatcher,
    hkpConstraintOwner& constraintOwner
) {
    switch (entry->m_streamCommand) {
        case hkAgent3::STREAM_CALL:
        case hkAgent3::STREAM_CALL_AGENT:
        case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
        case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM:
            dispatcher->m_agent3Func[entry->m_agentType].m_destroyFunc(
                entry, static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1, entry->m_contactMgr, constraintOwner,
                dispatcher
            );
            break;
        case hkAgent3::STREAM_CALL_WITH_TIM:
        case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
            dispatcher->m_agent3Func[entry->m_agentType].m_destroyFunc(
                entry, static_cast<hkpAgentNnMachineTimEntry*>(entry) + 1, entry->m_contactMgr, constraintOwner,
                dispatcher
            );
            break;
        default: break;
    }

    for (int32_t i = 0; i < 2; ++i) {
        hkpLinkedCollidable* collidable = entry->m_collidable[i];
        int32_t index = entry->m_agentIndexOnCollidable[i];
        hkArray<hkpLinkedCollidable::CollisionEntry>& entries = collidable->m_collisionEntries;
        entries.removeAt(index);
        if (index < entries.m_size) {
            hkpAgentNnEntry* moved = entries.m_data[index].m_agentEntry;
            moved->m_agentIndexOnCollidable[moved->m_collidable[0] != collidable] = static_cast<uint16_t>(index);
        }
        entries.optimizeCapacity(4);
    }
    hkAgentNnMachine_InternalDeallocateEntry(track, entry);
}

// Switches on the entry's stream command: a padded entry's agent data follows its header, and so does a time-caching
// entry's. Every other command leaves the entry alone.
void hkAgentNnMachine_WarpTimeInAgent(
    hkpAgentNnEntry* entry,
    float oldTime,
    float newTime,
    const hkpCollisionInput& input
) {
    switch (entry->m_streamCommand) {
        case 2:
        case 6:
        case 10:
        case 14: {
            hkAgent3::WarpTimeFunc warpTimeFunc = input.m_dispatcher->m_agent3Func[entry->m_agentType].m_warpTimeFunc;
            if (!warpTimeFunc) {
                return;
            }
            warpTimeFunc(entry, static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1, oldTime, newTime, input);
            return;
        }
        case 4:
        case 12: {
            hkpAgentNnMachineTimEntry* timEntry = static_cast<hkpAgentNnMachineTimEntry*>(entry);
            // The separating normal's time is dropped after the compare and the agent is handed oldTime.
            if (oldTime == timEntry->m_timeOfSeparatingNormal) {
                timEntry->m_timeOfSeparatingNormal = newTime;
            }
            else {
                timEntry->m_timeOfSeparatingNormal = -1.0f;
            }
            hkAgent3::WarpTimeFunc warpTimeFunc = input.m_dispatcher->m_agent3Func[entry->m_agentType].m_warpTimeFunc;
            if (warpTimeFunc) {
                warpTimeFunc(entry, timEntry + 1, oldTime, newTime, input);
            }
            return;
        }
        default: return;
    }
}

// Walks down the chain of transform shapes from body: each level gets a new motion state (the parent's with the shape's
// transform applied) and a new cd body over the child shape, and the flag says a transform was applied. Returns the
// first body that is not a transform shape. numSlots is deliberately never checked, as the game does.
const hkpCdBody* hkAgentMachine_processTransformedShapes(
    const hkpCdBody* cdBody,
    hkpCdBody* newCdBodies,
    hkMotionState* newMotionStates,
    int32_t numSlots,
    uint8_t& cdBodyHasTransformFlag
) {
    const hkpShape* shape = cdBody->m_shape;
    if (shape->m_type == HK_SHAPE_TRANSFORM) {
        do {
            const hkpTransformShape* transformShape = static_cast<const hkpTransformShape*>(shape);
            const hkMotionState* parent = cdBody->m_motion.pState;
            const hkTransform& t = transformShape->m_transform;
            hkMotionState* ms = newMotionStates;
            const hkRotation& parentRotation = parent->m_transform.m_rotation;
            __m128 c0 = _mm_load_ps(&reinterpret_cast<const float*>(&parentRotation.m_col0.m_quad)[0]);
            __m128 c1 = _mm_load_ps(&reinterpret_cast<const float*>(&parentRotation.m_col1.m_quad)[0]);
            __m128 c2 = _mm_load_ps(&reinterpret_cast<const float*>(&parentRotation.m_col2.m_quad)[0]);
            // EXE-INLINED none: hkTransform::_setMul (the columns from the translation down to column 0, each
            // y * c1 + x * c0, then + z * c2; the parent's translation is added afterwards)
            const hkVector4* srcCols[4] = {
                &t.m_rotation.m_col0, &t.m_rotation.m_col1, &t.m_rotation.m_col2, &t.m_translation
            };
            hkVector4* dstCols[4] = {
                &ms->m_transform.m_rotation.m_col0, &ms->m_transform.m_rotation.m_col1,
                &ms->m_transform.m_rotation.m_col2, &ms->m_transform.m_translation
            };
            for (int32_t i = 3; i >= 0; --i) {
                __m128 s = _mm_load_ps(&reinterpret_cast<const float*>(&srcCols[i]->m_quad)[0]);
                __m128 r =
                    _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), c1), _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), c0));
                r = _mm_add_ps(r, _mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), c2));
                _mm_store_ps(&reinterpret_cast<float*>(&dstCols[i]->m_quad)[0], r);
            }
            _mm_store_ps(
                &reinterpret_cast<float*>(&ms->m_transform.m_translation.m_quad)[0],
                _mm_add_ps(
                    _mm_load_ps(&reinterpret_cast<const float*>(&parent->m_transform.m_translation.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<float*>(&ms->m_transform.m_translation.m_quad)[0])
                )
            );
            ms->m_sweptTransform.m_centerOfMass0 = parent->m_sweptTransform.m_centerOfMass0;
            ms->m_sweptTransform.m_centerOfMass1 = parent->m_sweptTransform.m_centerOfMass1;
            ms->m_sweptTransform.m_rotation0.setMul(parent->m_sweptTransform.m_rotation0, transformShape->m_rotation);
            ms->m_sweptTransform.m_rotation1.setMul(parent->m_sweptTransform.m_rotation1, transformShape->m_rotation);
            // EXE-INLINED none: hkVector4::_setRotatedInverseDir of (centre of mass local - the translation)
            __m128 r0 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col0.m_quad)[0]);
            __m128 r1 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col1.m_quad)[0]);
            __m128 r2 = _mm_load_ps(&reinterpret_cast<const float*>(&t.m_rotation.m_col2.m_quad)[0]);
            __m128 v = _mm_sub_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&parent->m_sweptTransform.m_centerOfMassLocal.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&t.m_translation.m_quad)[0])
            );
            __m128 lo = _mm_unpacklo_ps(r0, r1);
            __m128 hi = _mm_unpackhi_ps(r0, r1);
            __m128 rowX = _mm_movelh_ps(lo, r2);
            __m128 rowY = _mm_shuffle_ps(_mm_movehl_ps(rowX, lo), r2, 0xD4);
            __m128 rowZ = _mm_shuffle_ps(hi, r2, 0xE4);
            __m128 local =
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), rowY), _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), rowX));
            local = _mm_add_ps(local, _mm_mul_ps(rowZ, _mm_shuffle_ps(v, v, 0xAA)));
            _mm_store_ps(&reinterpret_cast<float*>(&ms->m_sweptTransform.m_centerOfMassLocal.m_quad)[0], local);
            ms->m_deltaAngle = parent->m_deltaAngle;
            ms->m_objectRadius = parent->m_objectRadius;
            if (newCdBodies) {
                newCdBodies->m_parent = cdBody;
                newCdBodies->m_motion.pState = ms;
            }
            ++newMotionStates;
            newCdBodies->m_shape = transformShape->m_childShape.m_childShape;
            newCdBodies->m_shapeKey = 0;
            cdBody = newCdBodies;
            ++newCdBodies;
            cdBodyHasTransformFlag = hkAgent3::TRANSFORM_FLAG;
            shape = cdBody->m_shape;
        }
        while (shape->m_type == HK_SHAPE_TRANSFORM);
    }
    return cdBody;
}

// The dispatcher is read once up front for the predictive test and the create function, and reloaded from input for the
// quality table and the quality info. m_collisionQualityInfo is mutable, hence the cast.
hkpAgentNnEntry* hkAgentNnMachine_CreateAgent(
    hkpAgentNnTrack& track,
    hkpLinkedCollidable* collA,
    const hkpCdBody* firstNonTransformBodyA,
    hkpLinkedCollidable* collB,
    const hkpCdBody* firstNonTransformBodyB,
    uint8_t cdBodyHasTransformFlag,
    int32_t agentType,
    const hkpProcessCollisionInput& input,
    hkpContactMgr* mgr
) {
    hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
    hkpAgentNnEntry* entry = hkAgentNnMachine_AllocateEntry(track);
    entry->m_collidable[0] = collA;
    entry->m_contactMgr = mgr;
    entry->m_collidable[1] = collB;
    entry->m_agentIndexOnCollidable[0] = static_cast<uint16_t>(collA->m_collisionEntries.m_size);
    entry->m_agentIndexOnCollidable[1] = static_cast<uint16_t>(collB->m_collisionEntries.m_size);

    // EXE-INLINED none: hkArray::expandOne
    hkArray<hkpLinkedCollidable::CollisionEntry>& entriesA = collA->m_collisionEntries;
    hkMemoryRouter& routerA = hkMemoryRouter::getInstance();
    if (entriesA.m_size ==
        (entriesA.m_capacityAndFlags & hkArrayBase<hkpLinkedCollidable::CollisionEntry>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*routerA.m_heap, &entriesA, sizeof(hkpLinkedCollidable::CollisionEntry));
    }
    hkpLinkedCollidable::CollisionEntry* entryA = &entriesA.m_data[entriesA.m_size++];
    // EXE-INLINED none: hkArray::expandOne
    hkArray<hkpLinkedCollidable::CollisionEntry>& entriesB = collB->m_collisionEntries;
    hkMemoryRouter& routerB = hkMemoryRouter::getInstance();
    if (entriesB.m_size ==
        (entriesB.m_capacityAndFlags & hkArrayBase<hkpLinkedCollidable::CollisionEntry>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*routerB.m_heap, &entriesB, sizeof(hkpLinkedCollidable::CollisionEntry));
    }
    hkpLinkedCollidable::CollisionEntry* entryB = &entriesB.m_data[entriesB.m_size++];
    entryA->m_agentEntry = entry;
    entryB->m_agentEntry = entry;
    entryA->m_partner = collB;
    entryB->m_partner = collA;

    hkpAgent3Input in3;
    in3.m_bodyB = firstNonTransformBodyB;
    in3.m_contactMgr = entry->m_contactMgr;
    in3.m_overrideBodyA = nullptr;
    in3.m_bodyA = firstNonTransformBodyA;
    in3.m_input = &input;
    in3.m_aTb.setMulInverseMul(*firstNonTransformBodyA->m_motion.pMotion, *firstNonTransformBodyB->m_motion.pMotion);

    int8_t qualityIndex = input.m_dispatcher->m_collisionQualityTable[collA->m_broadPhaseHandle.m_objectQualityType]
                                                                     [collB->m_broadPhaseHandle.m_objectQualityType];
    entry->m_collisionQualityIndex = static_cast<uint8_t>(qualityIndex);
    const_cast<hkpProcessCollisionInput&>(input).m_collisionQualityInfo =
        &input.m_dispatcher->m_collisionQualityInfo[qualityIndex];
    entry->m_agentType = static_cast<uint8_t>(agentType);

    hkpAgentData* freeMemory;
    if (dispatcher->m_agent3Func[agentType].m_sepNormalFunc) {
        hkpAgentNnMachineTimEntry* timEntry = static_cast<hkpAgentNnMachineTimEntry*>(entry);
        entry->m_streamCommand = cdBodyHasTransformFlag | hkAgent3::STREAM_CALL_WITH_TIM;
        timEntry->m_timeOfSeparatingNormal = -1.0f;
        timEntry->m_separatingNormal.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
        freeMemory = timEntry + 1;
    }
    else {
        entry->m_streamCommand = cdBodyHasTransformFlag | hkAgent3::STREAM_CALL;
        freeMemory = static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1;
    }
    dispatcher->m_agent3Func[static_cast<uint8_t>(agentType)].m_createFunc(in3, entry, freeMemory);
    entry->m_size = HK_AGENT3_AGENT_SIZE;
    return entry;
}

// Walks the entries of every sector by their sizes, to the end of the sector or, in the last one, to the bytes it uses.
// The release build keeps the walk and none of its asserts. The sector's bytes are untyped storage holding the entries:
// an entry is found by its byte offset.
void hkAgentNnMachine_AssertTrackValidity(hkpAgentNnTrack& track) {
    const int32_t numSectors = track.m_sectors.m_size;
    for (int32_t i = 0; i < numSectors;) {
        hkpAgentNnSector* sector = track.m_sectors.m_data[i];
        i++;
        hkpAgentNnEntry* entry = sector->getBegin();
        hkpAgentNnEntry* end = (i == numSectors)
            ? reinterpret_cast<hkpAgentNnEntry*>(&sector->m_data[track.m_bytesUsedInLastSector])
            : sector->getEnd();
        while (entry < end) {
            entry = reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
        }
    }
}

void hkAgentNnMachine_GetAgentType(
    const hkpCdBody* cdBodyA,
    const hkpCdBody* cdBodyB,
    const hkpProcessCollisionInput& input,
    int32_t& agentTypeOut,
    int32_t& isFlippedOut
) {
    int32_t v9;
    hkpCollisionDispatcher* m_dispatcher = input.m_dispatcher;
    hkpShapeType m_type = cdBodyA->m_shape->m_type;
    hkpShapeType v7 = cdBodyB->m_shape->m_type;
    int32_t v8 = v7 + 32 * m_type;
    if (input.m_createPredictiveAgents.m_storage) {
        v9 = m_dispatcher->m_agent3TypesPred[0][v8];
    }
    else {
        v9 = m_dispatcher->m_agent3Types[0][v8];
    }
    int32_t v10 = m_dispatcher->m_agent3Func[v9].m_symmetric == hkAgent3::IS_NOT_SYMMETRIC_AND_FLIPPED;
    if (v9 == 1) {
        v10 = m_dispatcher->m_agent2Func[m_dispatcher->m_agent2Types[m_type][v7]].m_isFlipped.m_bool != 0;
    }
    isFlippedOut = v10;
    if (v10) {
        int32_t v12;
        int32_t v11 = m_type + 32 * v7;
        if (input.m_createPredictiveAgents.m_storage) {
            v12 = m_dispatcher->m_agent3TypesPred[0][v11];
        }
        else {
            v12 = m_dispatcher->m_agent3Types[0][v11];
        }
        agentTypeOut = v12;
    }
    else {
        agentTypeOut = v9;
    }
}

// A transform stream command first moves both bodies into the transform shapes' children (the flag the helper sets is
// not read here), then the agent's update-filter function runs on the agent data after the entry.
void hkAgentNnMachine_UpdateShapeCollectionFilter(
    hkpAgentNnEntry* entry,
    const hkpCollisionInput& input,
    hkpConstraintOwner& constraintOwner
) {
    hkpCdBody newCdBodiesA[4];
    hkMotionState newMotionStatesA[4];
    hkpCdBody newCdBodiesB[4];
    hkMotionState newMotionStatesB[4];
    uint32_t command = entry->m_streamCommand;
    const hkpCdBody* bodyA = entry->m_collidable[0];
    const hkpCdBody* bodyB = entry->m_collidable[1];
    hkpAgentData* agentData;
    while (true) {
        switch (command) {
            case hkAgent3::STREAM_CALL:
            case hkAgent3::STREAM_CALL_AGENT: agentData = static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1; break;
            case hkAgent3::STREAM_CALL_WITH_TIM: agentData = static_cast<hkpAgentNnMachineTimEntry*>(entry) + 1; break;
            case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM: {
                uint8_t cdBodyHasTransformFlag;
                bodyA = hkAgentMachine_processTransformedShapes(
                    bodyA, newCdBodiesA, newMotionStatesA, 4, cdBodyHasTransformFlag
                );
                bodyB = hkAgentMachine_processTransformedShapes(
                    bodyB, newCdBodiesB, newMotionStatesB, 4, cdBodyHasTransformFlag
                );
                command &= ~static_cast<uint32_t>(hkAgent3::TRANSFORM_FLAG);
                continue;
            }
            default: return;
        }
        break;
    }
    hkAgent3::UpdateFilterFunc updateFilter = input.m_dispatcher->m_agent3Func[entry->m_agentType].m_updateFilterFunc;
    if (updateFilter) {
        updateFilter(entry, agentData, *bodyA, *bodyB, input, entry->m_contactMgr, constraintOwner);
    }
}

void hkAgentNnMachine_DestroyTrack(
    hkpAgentNnTrack& track,
    hkpCollisionDispatcher* dispatcher,
    hkpConstraintOwner& constraintOwner
) {
    while (track.m_sectors.m_size) {
        // hkAddByteOffset(lastSector->getBegin(), m_bytesUsedInLastSector - HK_AGENT3_AGENT_SIZE): the last
        // HK_AGENT3_AGENT_SIZE-byte agent of the last sector, whose bytes getBegin reinterprets as entries.
        hkpAgentNnSector* sector = track.m_sectors.m_data[track.m_sectors.m_size - 1];
        hkpAgentNnEntry* entry =
            reinterpret_cast<hkpAgentNnEntry*>(&sector->m_data[track.m_bytesUsedInLastSector - HK_AGENT3_AGENT_SIZE]);
        hkAgentNnMachine_DestroyAgent(track, entry, dispatcher, constraintOwner);
    }
}

// The agent3 inputs are filled field by field, never their padding, nor, for a plain call, m_distAtT1; the agent's
// process copies the whole struct, so it reads uninitialised bytes, as the game does. A transform-shape command
// replaces the bodies by the first non-transform levels and runs again without the flag; a plain call runs the agent
// with no separating normal; a TIM call first refreshes the entry's separating normal (at the step's start time, or
// cleared when the quality has no continuous physics), and skips the agent, cleaning up its contact points, while the
// distance it predicts at the step's end is at least the tolerance; an agent call runs the hkpCollisionAgent behind the
// entry.
void hkAgentNnMachine_ProcessAgent(
    hkpAgentNnEntry* entry,
    const hkpProcessCollisionInput& input,
    hkpProcessCollisionOutput& output,
    hkpContactMgr* mgr
) {
    alignas(16) hkpCdBody newCdBodiesA[4];
    alignas(16) hkpCdBody newCdBodiesB[4];
    hkMotionState newMotionStatesA[4];
    hkMotionState newMotionStatesB[4];
    alignas(16) hkpAgent3ProcessInput in3;

    const hkpCdBody* firstObject = entry->m_collidable[0];
    const hkpCdBody* secondObject = entry->m_collidable[1];
    uint8_t command = entry->m_streamCommand;
    while (true) {
        switch (command) {
            case hkAgent3::STREAM_CALL: {
                in3.m_bodyA = firstObject;
                in3.m_bodyB = secondObject;
                in3.m_overrideBodyA = nullptr;
                in3.m_contactMgr = entry->m_contactMgr;
                in3.m_input = &input;
                const hkMotionState* msA = firstObject->m_motion.pState;
                const hkMotionState* msB = secondObject->m_motion.pState;
                // EXE-INLINED none: hkSweptTransformUtil::calcTimInfo
                const float fDeltaTime = input.m_stepInfo.m_deltaTime;
                const float fScaleA = msA->m_sweptTransform.m_centerOfMass1.m_quad[3] * fDeltaTime;
                const float fScaleB = fDeltaTime * msB->m_sweptTransform.m_centerOfMass1.m_quad[3];
                const __m128 kDiffA = _mm_mul_ps(
                    _mm_sub_ps(
                        msA->m_sweptTransform.m_centerOfMass0.m_quad, msA->m_sweptTransform.m_centerOfMass1.m_quad
                    ),
                    _mm_set1_ps(fScaleA)
                );
                in3.m_linearTimInfo.m_quad = _mm_add_ps(
                    _mm_mul_ps(
                        _mm_set1_ps(fScaleB),
                        _mm_sub_ps(
                            msB->m_sweptTransform.m_centerOfMass1.m_quad, msB->m_sweptTransform.m_centerOfMass0.m_quad
                        )
                    ),
                    kDiffA
                );
                in3.m_linearTimInfo.m_quad[3] = msB->m_deltaAngle.m_quad[3] * fScaleB * msB->m_objectRadius +
                    msA->m_deltaAngle.m_quad[3] * fScaleA * msA->m_objectRadius;
                in3.m_aTb.setMulInverseMul(msA->m_transform, msB->m_transform);
                input.m_dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(
                    in3, entry, static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1, nullptr, output
                );
                return;
            }
            case hkAgent3::STREAM_CALL_WITH_TIM: {
                hkpAgentNnMachineTimEntry* timEntry = static_cast<hkpAgentNnMachineTimEntry*>(entry);
                in3.m_bodyB = secondObject;
                in3.m_overrideBodyA = nullptr;
                in3.m_bodyA = firstObject;
                in3.m_contactMgr = entry->m_contactMgr;
                in3.m_input = &input;
                const hkMotionState* msA = firstObject->m_motion.pState;
                const hkMotionState* msB = secondObject->m_motion.pState;
                // EXE-INLINED none: hkSweptTransformUtil::calcTimInfo
                const float fDeltaTime = input.m_stepInfo.m_deltaTime;
                const float fScaleA = msA->m_sweptTransform.m_centerOfMass1.m_quad[3] * fDeltaTime;
                const float fScaleB = fDeltaTime * msB->m_sweptTransform.m_centerOfMass1.m_quad[3];
                const __m128 kDiffA = _mm_mul_ps(
                    _mm_sub_ps(
                        msA->m_sweptTransform.m_centerOfMass0.m_quad, msA->m_sweptTransform.m_centerOfMass1.m_quad
                    ),
                    _mm_set1_ps(fScaleA)
                );
                in3.m_linearTimInfo.m_quad = _mm_add_ps(
                    _mm_mul_ps(
                        _mm_set1_ps(fScaleB),
                        _mm_sub_ps(
                            msB->m_sweptTransform.m_centerOfMass1.m_quad, msB->m_sweptTransform.m_centerOfMass0.m_quad
                        )
                    ),
                    kDiffA
                );
                in3.m_linearTimInfo.m_quad[3] = msA->m_deltaAngle.m_quad[3] * fScaleA * msA->m_objectRadius +
                    msB->m_deltaAngle.m_quad[3] * fScaleB * msB->m_objectRadius;

                float distAtT1;
                if (input.m_stepInfo.m_startTime != timEntry->m_timeOfSeparatingNormal) {
                    if (!input.m_collisionQualityInfo->m_useContinuousPhysics) {
                        // No continuous physics: no separating normal, and the agent always runs.
                        timEntry->m_timeOfSeparatingNormal = input.m_stepInfo.m_endTime;
                        timEntry->m_separatingNormal.setZero4();
                        distAtT1 = -3.40282e+38f;
                        timEntry->m_separatingNormal.m_quad[3] = distAtT1;
                        goto processAgent;
                    }
                    // The separating normal at the step's start: both bodies at their start transforms.
                    alignas(16) hkAgentNnMachineBodyTemp bodyTemp;
                    alignas(16) hkpAgent3Input sepInput;
                    sepInput.m_overrideBodyA = nullptr;
                    hkSweptTransformUtil::lerp2(
                        in3.m_bodyA->m_motion.pState->m_sweptTransform, input.m_stepInfo.m_startTime, bodyTemp.m_transA
                    );
                    hkSweptTransformUtil::lerp2(
                        in3.m_bodyB->m_motion.pState->m_sweptTransform, in3.m_input->m_stepInfo.m_startTime,
                        bodyTemp.m_transB
                    );
                    sepInput.m_contactMgr = in3.m_contactMgr;
                    sepInput.m_bodyA = &bodyTemp.m_bodyA;
                    sepInput.m_input = in3.m_input;
                    sepInput.m_bodyB = &bodyTemp.m_bodyB;
                    bodyTemp.m_bodyA.m_shape = in3.m_bodyA->m_shape;
                    bodyTemp.m_bodyA.m_shapeKey = in3.m_bodyA->m_shapeKey;
                    bodyTemp.m_bodyA.m_motion.pMotion = &bodyTemp.m_transA;
                    bodyTemp.m_bodyA.m_parent = in3.m_bodyA;
                    bodyTemp.m_bodyB.m_shape = in3.m_bodyB->m_shape;
                    bodyTemp.m_bodyB.m_shapeKey = in3.m_bodyB->m_shapeKey;
                    bodyTemp.m_bodyB.m_motion.pMotion = &bodyTemp.m_transB;
                    bodyTemp.m_bodyB.m_parent = in3.m_bodyB;
                    sepInput.m_aTb.setMulInverseMul(bodyTemp.m_transA, bodyTemp.m_transB);
                    input.m_dispatcher->m_agent3Func[entry->m_agentType].m_sepNormalFunc(
                        sepInput, entry, timEntry + 1, timEntry->m_separatingNormal
                    );
                }
                {
                    timEntry->m_timeOfSeparatingNormal = input.m_stepInfo.m_endTime;
                    const float fProjected = in3.m_linearTimInfo.dot4xyz1(timEntry->m_separatingNormal);
                    distAtT1 = timEntry->m_separatingNormal.m_quad[3] - fProjected;
                    if (distAtT1 >= input.m_tolerance) {
                        // Still separated at the step's end: the agent does not run.
                        timEntry->m_separatingNormal.m_quad[3] = distAtT1;
                        if (entry->m_numContactPoints) {
                            input.m_dispatcher->m_agent3Func[entry->m_agentType].m_cleanupFunc(
                                entry, timEntry + 1, in3.m_contactMgr, *output.m_constraintOwner
                            );
                        }
                        return;
                    }
                }
            processAgent:
                in3.m_distAtT1 = distAtT1;
                in3.m_aTb.setMulInverseMul(msA->m_transform, msB->m_transform);
                input.m_dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(
                    in3, entry, timEntry + 1, &timEntry->m_separatingNormal, output
                );
                return;
            }
            case hkAgent3::STREAM_CALL_AGENT: {
                hkpAgentData* agentData = static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1;
                hkpCollisionAgent* agent = *static_cast<hkpCollisionAgent**>(agentData);
                agent->processCollision(*firstObject, *secondObject, input, output);
                return;
            }
            case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM: {
                uint8_t cdBodyHasTransformFlag;
                firstObject = hkAgentMachine_processTransformedShapes(
                    firstObject, newCdBodiesA, newMotionStatesA, 4, cdBodyHasTransformFlag
                );
                secondObject = hkAgentMachine_processTransformedShapes(
                    secondObject, newCdBodiesB, newMotionStatesB, 4, cdBodyHasTransformFlag
                );
                command = static_cast<uint8_t>(command & ~hkAgent3::TRANSFORM_FLAG);
                continue;
            }
            default: return;
        }
    }
}

// The appendee's entries move, last first, into the track's free slots until its last sector is full, each relinked to
// its collidables; a sector the appendee empties goes back to the heap and its sector array shrinks as
// hkInplaceArray::optimizeCapacity(4) does. The appendee's remaining sectors are then inserted after the track's, the
// track takes the appendee's fill of its last sector, and the appendee is left empty.
void hkAgentNnMachine_AppendTrack(hkpAgentNnTrack& track, hkpAgentNnTrack& appendee) {
    while (track.m_bytesUsedInLastSector != sizeof(hkpAgentNnSector) && appendee.m_sectors.m_size) {
        hkpAgentNnSector* lastSector = appendee.m_sectors.m_data[appendee.m_sectors.m_size - 1];
        // The SDK's hkAddByteOffset(lastSector->getBegin(), m_bytesUsedInLastSector - HK_AGENT3_AGENT_SIZE): the
        // sector's bytes are untyped storage holding the entries.
        hkpAgentNnEntry* lastEntry = reinterpret_cast<hkpAgentNnEntry*>(
            &lastSector->m_data[appendee.m_bytesUsedInLastSector - HK_AGENT3_AGENT_SIZE]
        );
        hkAgentNnMachine_CopyAndRelinkAgentEntry(track, lastEntry);
        appendee.m_bytesUsedInLastSector -= HK_AGENT3_AGENT_SIZE;
        if (appendee.m_bytesUsedInLastSector == 0) {
            hkpAgentNnSector* sector = appendee.m_sectors.m_data[--appendee.m_sectors.m_size];
            hkMemoryRouter::getInstance().m_heap->blockFree(sector, sizeof(hkpAgentNnSector));
            appendee.m_bytesUsedInLastSector = sizeof(hkpAgentNnSector);
        }
        // EXE-INLINED none: hkInplaceArray<hkpAgentNnSector*, 1>::optimizeCapacity(4)
        if (appendee.m_sectors.m_capacityAndFlags >= 0) {
            uint32_t uiCapacity = appendee.m_sectors.m_size + 3;
            uiCapacity |= uiCapacity >> 16;
            uiCapacity |= uiCapacity >> 8;
            uiCapacity |= uiCapacity >> 4;
            uiCapacity |= uiCapacity >> 2;
            uiCapacity |= uiCapacity >> 1;
            int32_t newCapacity = static_cast<int32_t>(uiCapacity + 1);
            if (newCapacity < 1) {
                // The SDK's cast: _reduce takes the inplace storage as raw bytes.
                hkArrayUtil::_reduce(
                    *hkMemoryRouter::getInstance().m_heap, &appendee.m_sectors, sizeof(hkpAgentNnSector*),
                    reinterpret_cast<char*>(appendee.m_sectors.m_storage), 1
                );
            }
            else if (
                newCapacity < static_cast<int32_t>(
                                  appendee.m_sectors.m_capacityAndFlags & hkArrayBase<hkpAgentNnSector*>::CAPACITY_MASK
                              )
            ) {
                hkArrayUtil::_reduce(
                    *hkMemoryRouter::getInstance().m_heap, &appendee.m_sectors, sizeof(hkpAgentNnSector*), nullptr,
                    newCapacity
                );
            }
        }
    }
    if (appendee.m_sectors.m_size) {
        track.m_sectors._insertAt(
            *hkMemoryRouter::getInstance().m_heap, track.m_sectors.m_size, appendee.m_sectors.m_data,
            appendee.m_sectors.m_size
        );
        track.m_bytesUsedInLastSector = appendee.m_bytesUsedInLastSector;
        appendee.m_sectors.m_size = 0;
        appendee.m_bytesUsedInLastSector = sizeof(hkpAgentNnSector);
    }
}

// Every agent of the track processes its pair: the transform commands first move both bodies into the transform shapes'
// children; an agent3 gets the bodies, the transform from A to B and the linear TIM information; a TIM entry first
// carries its separating normal to the step's end (recomputed at the step's start under continuous physics when it is
// stale) and skips the agent while the pair stays at least the tolerance apart, cleaning up its contact points. Then
// the pair's contact manager takes the points the agent wrote. Out of memory ends the walk at once; else the count of
// skipped agents goes to the "numTim" monitor. As in hkAgentNnMachine_ProcessAgent, in3's padding and, for a plain
// call, m_distAtT1 are never written, and hkBvTreeAgent3::process hands in3 to hkAgent1nMachine_Process, which copies
// all of it; this uninitialised read is deliberate, as the game does.
void hkAgentNnMachine_ProcessTrack(
    hkpConstraintOwner* owner,
    hkpAgentNnTrack& agentTrack,
    const hkpProcessCollisionInput& input
) {
    hkpCdBody newCdBodiesA[4];
    hkMotionState newMotionStatesA[4];
    hkpCdBody newCdBodiesB[4];
    hkMotionState newMotionStatesB[4];
    // The flag both transform helpers set; nothing here reads it.
    uint8_t cdBodyHasTransformFlag;
    // EXE-INLINED none: hkpProcessCollisionOutput::hkpProcessCollisionOutput
    hkpProcessCollisionOutput processOutput;
    processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
    processOutput.m_toi.m_properties.m_impulseApplied = 0.0f;
    processOutput.m_toi.m_properties.m_internalSolverData = 0.0f;
    processOutput.m_constraintOwner.m_storage = owner;
    int32_t numTim = 0;
    for (int32_t sectorIndex = 0; sectorIndex < agentTrack.m_sectors.m_size; sectorIndex++) {
        hkpAgentNnSector* sector = agentTrack.m_sectors.m_data[sectorIndex];
        // The sector is untyped bytes the entries are laid out in, one after the other; the last sector ends where its
        // used bytes do. The entry's address is compared with the end as an unsigned integer, wrap included, also
        // before the first entry, where a pointer compare would let the compiler test only that the used bytes are
        // nonzero.
        hkpAgentNnEntry* entry = reinterpret_cast<hkpAgentNnEntry*>(sector->m_data);
        const uintptr_t uiEndEntry = sectorIndex + 1 == agentTrack.m_sectors.m_size
            ? reinterpret_cast<uintptr_t>(sector->m_data) + agentTrack.m_bytesUsedInLastSector
            : reinterpret_cast<uintptr_t>(sector->m_data) + sizeof(hkpAgentNnSector);
        for (; reinterpret_cast<uintptr_t>(entry) < uiEndEntry;
             entry = reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size)) {
            // EXE-INLINED none: hkpProcessCollisionOutput::reset
            processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
            processOutput.m_potentialContacts.m_storage = nullptr;
            processOutput.m_firstFreeContactPoint.m_storage = processOutput.m_contactPoints;
            // EXE-INLINED 0xCFE370
            _mm_prefetch(reinterpret_cast<const char*>(entry) + 128, _MM_HINT_NTA);
            hkpLinkedCollidable* collA = entry->m_collidable[0];
            hkpLinkedCollidable* collB = entry->m_collidable[1];
            _mm_prefetch(reinterpret_cast<const char*>(entry->m_contactMgr), _MM_HINT_NTA);
            const hkpCdBody* bodyA = collA;
            const hkpCdBody* bodyB = collB;
            uint32_t streamCommand = entry->m_streamCommand;
            hkpAgent3ProcessInput in3;
            while (true) {
                switch (streamCommand) {
                    case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
                    case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
                    case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM:
                        bodyA = hkAgentMachine_processTransformedShapes(
                            bodyA, newCdBodiesA, newMotionStatesA, 4, cdBodyHasTransformFlag
                        );
                        bodyB = hkAgentMachine_processTransformedShapes(
                            bodyB, newCdBodiesB, newMotionStatesB, 4, cdBodyHasTransformFlag
                        );
                        streamCommand &= ~static_cast<uint32_t>(hkAgent3::TRANSFORM_FLAG);
                        continue;
                    case hkAgent3::STREAM_CALL_AGENT: {
                        // The agent data of an agent entry is the agent's pointer.
                        hkpAgentData* agentData = static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1;
                        hkpCollisionAgent* agent = *static_cast<hkpCollisionAgent**>(agentData);
                        agent->processCollision(*bodyA, *bodyB, input, processOutput);
                        break;
                    }
                    case hkAgent3::STREAM_CALL:
                    case hkAgent3::STREAM_CALL_WITH_TIM: {
                        in3.m_bodyA = bodyA;
                        in3.m_bodyB = bodyB;
                        in3.m_overrideBodyA = nullptr;
                        in3.m_input = &input;
                        in3.m_contactMgr = entry->m_contactMgr;
                        const hkMotionState* msA = bodyA->m_motion.pState;
                        const hkMotionState* msB = bodyB->m_motion.pState;
                        // EXE-INLINED none: hkSweptTransformUtil::calcTimInfo
                        const float deltaTime = input.m_stepInfo.m_deltaTime;
                        const float f0 = deltaTime * msA->m_sweptTransform.m_centerOfMass1(3);
                        const float f1 = deltaTime * msB->m_sweptTransform.m_centerOfMass1(3);
                        hkVector4 diff0;
                        diff0.setSub4(msA->m_sweptTransform.m_centerOfMass0, msA->m_sweptTransform.m_centerOfMass1);
                        hkVector4 diff1;
                        diff1.setSub4(msB->m_sweptTransform.m_centerOfMass1, msB->m_sweptTransform.m_centerOfMass0);
                        in3.m_linearTimInfo.m_quad = _mm_add_ps(
                            _mm_mul_ps(_mm_set_ps1(f1), diff1.m_quad), _mm_mul_ps(_mm_set_ps1(f0), diff0.m_quad)
                        );
                        in3.m_linearTimInfo(3) = msA->m_deltaAngle(3) * f0 * msA->m_objectRadius +
                            msB->m_deltaAngle(3) * f1 * msB->m_objectRadius;
                        if (streamCommand == hkAgent3::STREAM_CALL) {
                            in3.m_aTb.setMulInverseMul(msA->m_transform, msB->m_transform);
                            input.m_dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(
                                in3, entry, static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1, nullptr,
                                processOutput
                            );
                            break;
                        }
                        hkpAgentNnMachineTimEntry* timEntry = static_cast<hkpAgentNnMachineTimEntry*>(entry);
                        hkpAgentData* agentData = timEntry + 1;
                        if (input.m_stepInfo.m_startTime != timEntry->m_timeOfSeparatingNormal) {
                            if (!input.m_collisionQualityInfo->m_useContinuousPhysics.m_storage) {
                                // Without continuous physics the normal is dropped: the agent runs with no distance.
                                timEntry->m_timeOfSeparatingNormal = input.m_stepInfo.m_endTime;
                                timEntry->m_separatingNormal.setZero4();
                                timEntry->m_separatingNormal(3) = -3.40282e+38f;
                                in3.m_distAtT1.m_storage = -3.40282e+38f;
                                in3.m_aTb.setMulInverseMul(msA->m_transform, msB->m_transform);
                                input.m_dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(
                                    in3, entry, agentData, &timEntry->m_separatingNormal, processOutput
                                );
                                break;
                            }
                            // The separating normal at the step's start: a body over each of the pair's at its
                            // interpolated transform.
                            hkAgentNnMachineBodyTemp bodyTemp;
                            hkpAgent3Input in2;
                            in2.m_overrideBodyA = nullptr;
                            hkSweptTransformUtil::lerp2(
                                in3.m_bodyA->m_motion.pState->m_sweptTransform, in3.m_input->m_stepInfo.m_startTime,
                                bodyTemp.m_transA
                            );
                            hkSweptTransformUtil::lerp2(
                                in3.m_bodyB->m_motion.pState->m_sweptTransform, in3.m_input->m_stepInfo.m_startTime,
                                bodyTemp.m_transB
                            );
                            in2.m_contactMgr = in3.m_contactMgr;
                            in2.m_bodyA = &bodyTemp.m_bodyA;
                            in2.m_input = in3.m_input;
                            in2.m_bodyB = &bodyTemp.m_bodyB;
                            bodyTemp.m_bodyA.m_shape = in3.m_bodyA->m_shape;
                            bodyTemp.m_bodyA.m_shapeKey = in3.m_bodyA->m_shapeKey;
                            bodyTemp.m_bodyA.m_motion.pMotion = &bodyTemp.m_transA;
                            bodyTemp.m_bodyA.m_parent = in3.m_bodyA;
                            bodyTemp.m_bodyB.m_shape = in3.m_bodyB->m_shape;
                            bodyTemp.m_bodyB.m_shapeKey = in3.m_bodyB->m_shapeKey;
                            bodyTemp.m_bodyB.m_motion.pMotion = &bodyTemp.m_transB;
                            bodyTemp.m_bodyB.m_parent = in3.m_bodyB;
                            in2.m_aTb.setMulInverseMul(bodyTemp.m_transA, bodyTemp.m_transB);
                            input.m_dispatcher->m_agent3Func[entry->m_agentType].m_sepNormalFunc(
                                in2, entry, agentData, timEntry->m_separatingNormal
                            );
                        }
                        timEntry->m_timeOfSeparatingNormal = input.m_stepInfo.m_endTime;
                        // The distance at the step's end: the normal's distance less its projection on the linear TIM
                        // information, summed in the order (z + w) + (y + x) in single precision, as the game does.
                        const hkQuadReal timInfo = in3.m_linearTimInfo.m_quad;
                        const hkQuadReal prod = _mm_mul_ps(timEntry->m_separatingNormal.m_quad, timInfo);
                        const hkQuadReal zw = _mm_add_ss(
                            _mm_shuffle_ps(prod, prod, _MM_SHUFFLE(2, 2, 2, 2)),
                            _mm_shuffle_ps(timInfo, timInfo, _MM_SHUFFLE(3, 3, 3, 3))
                        );
                        const hkQuadReal yx = _mm_add_ss(_mm_shuffle_ps(prod, prod, _MM_SHUFFLE(1, 1, 1, 1)), prod);
                        const float projected = _mm_cvtss_f32(_mm_add_ps(zw, yx));
                        const float dist = timEntry->m_separatingNormal(3) - projected;
                        if (input.m_tolerance <= dist) {
                            timEntry->m_separatingNormal(3) = dist;
                            if (entry->m_numContactPoints) {
                                input.m_dispatcher->m_agent3Func[entry->m_agentType].m_cleanupFunc(
                                    entry, agentData, in3.m_contactMgr, *processOutput.m_constraintOwner.m_storage
                                );
                            }
                            numTim++;
                            break;
                        }
                        in3.m_distAtT1.m_storage = dist;
                        in3.m_aTb.setMulInverseMul(msA->m_transform, msB->m_transform);
                        input.m_dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(
                            in3, entry, agentData, &timEntry->m_separatingNormal, processOutput
                        );
                        break;
                    }
                    default: break;
                }
                break;
            }
            if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                return;
            }
            if (processOutput.m_firstFreeContactPoint.m_storage != processOutput.m_contactPoints) {
                entry->m_contactMgr->processContact(*collA, *collB, input, processOutput);
            }
        }
    }
    HK_MONITOR_ADD_VALUE("numTim", static_cast<float>(numTim), HK_MONITOR_TYPE_INT);
}

// out takes in's bodies at the step's start time. Each body of in gets a copy in bodyTemp (its shape and shape key)
// whose motion is its swept transform at the input's start time and whose parent is the body itself; out gets those
// copies, in's input and contact manager, and the transform from A to B at that time.
void hkAgentNnMachine_initInputAtTime(hkpAgent3Input& in, hkAgentNnMachineBodyTemp& temp, hkpAgent3Input& out) {
    hkSweptTransformUtil::lerp2(
        in.m_bodyA->m_motion.pState->m_sweptTransform, in.m_input->m_stepInfo.m_startTime, temp.m_transA
    );
    hkSweptTransformUtil::lerp2(
        in.m_bodyB->m_motion.pState->m_sweptTransform, in.m_input->m_stepInfo.m_startTime, temp.m_transB
    );
    out.m_bodyA = &temp.m_bodyA;
    out.m_bodyB = &temp.m_bodyB;
    out.m_contactMgr = in.m_contactMgr;
    out.m_input = in.m_input;
    temp.m_bodyA.m_shape = in.m_bodyA->m_shape;
    temp.m_bodyA.m_shapeKey = in.m_bodyA->m_shapeKey;
    temp.m_bodyB.m_shape = in.m_bodyB->m_shape;
    temp.m_bodyB.m_shapeKey = in.m_bodyB->m_shapeKey;
    temp.m_bodyA.m_parent = in.m_bodyA;
    temp.m_bodyA.m_motion.pMotion = &temp.m_transA;
    // Tests the second body's address for null before linking it (a placement construction's test), as the game does.
    hkpCdBody* bodyB = &temp.m_bodyB;
    if (bodyB) {
        bodyB->m_parent = in.m_bodyB;
        bodyB->m_motion.pMotion = &temp.m_transB;
    }
    out.m_aTb.setMulInverseMul(temp.m_transA, temp.m_transB);
}

// The track's last entry slot goes back. A last sector left empty goes back to the heap and the one before it is full;
// the sector array then shrinks as hkInplaceArray::optimizeCapacity(4) does.
void hkAgentNnMachine_DeallocateLastEntry(hkpAgentNnTrack& track) {
    track.m_bytesUsedInLastSector -= HK_AGENT3_AGENT_SIZE;
    if (track.m_bytesUsedInLastSector == 0) {
        hkpAgentNnSector* sector = track.m_sectors.m_data[track.m_sectors.m_size - 1];
        track.m_sectors.m_size--;
        hkMemoryRouter::getInstance().m_heap->blockFree(sector, sizeof(hkpAgentNnSector));
        track.m_bytesUsedInLastSector = sizeof(hkpAgentNnSector);
    }
    // EXE-INLINED none: hkInplaceArray<hkpAgentNnSector*, 1>::optimizeCapacity(4)
    if (track.m_sectors.m_capacityAndFlags >= 0) {
        uint32_t uiCapacity = track.m_sectors.m_size + 3;
        uiCapacity |= uiCapacity >> 16;
        uiCapacity |= uiCapacity >> 8;
        uiCapacity |= uiCapacity >> 4;
        uiCapacity |= uiCapacity >> 2;
        uiCapacity |= uiCapacity >> 1;
        int32_t newCapacity = static_cast<int32_t>(uiCapacity + 1);
        if (newCapacity < 1) {
            // The SDK's cast: _reduce takes the inplace storage as raw bytes.
            hkArrayUtil::_reduce(
                *hkMemoryRouter::getInstance().m_heap, &track.m_sectors, sizeof(hkpAgentNnSector*),
                reinterpret_cast<char*>(track.m_sectors.m_storage), 1
            );
        }
        else if (
            newCapacity <
            static_cast<int32_t>(track.m_sectors.m_capacityAndFlags & hkArrayBase<hkpAgentNnSector*>::CAPACITY_MASK)
        ) {
            hkArrayUtil::_reduce(
                *hkMemoryRouter::getInstance().m_heap, &track.m_sectors, sizeof(hkpAgentNnSector*), nullptr, newCapacity
            );
        }
    }
}

// Performs hkAgentNnMachine_ProcessAgent's steps, and a TIM entry whose agent is skipped counts in numTim. The contact
// manager is read from the entry. As in hkAgentNnMachine_ProcessAgent, in3's padding and, for a plain call, m_distAtT1
// are never written, and the agent's process copies the whole struct, so uninitialised data is read, as the game does.
void hkAgentNnMachine_InlineProcessAgent(
    hkpAgentNnEntry* entry,
    const hkpProcessCollisionInput& input,
    int32_t& numTim,
    hkpProcessCollisionOutput& output
) {
    alignas(16) hkpCdBody newCdBodiesA[4];
    alignas(16) hkpCdBody newCdBodiesB[4];
    hkMotionState newMotionStatesA[4];
    hkMotionState newMotionStatesB[4];
    alignas(16) hkpAgent3ProcessInput in3;

    const hkpCdBody* firstObject = entry->m_collidable[0];
    const hkpCdBody* secondObject = entry->m_collidable[1];
    uint8_t command = entry->m_streamCommand;
    while (true) {
        switch (command) {
            case hkAgent3::STREAM_CALL: {
                in3.m_bodyA = firstObject;
                in3.m_bodyB = secondObject;
                in3.m_overrideBodyA = nullptr;
                in3.m_contactMgr = entry->m_contactMgr;
                in3.m_input = &input;
                const hkMotionState* msA = firstObject->m_motion.pState;
                const hkMotionState* msB = secondObject->m_motion.pState;
                // EXE-INLINED none: hkSweptTransformUtil::calcTimInfo
                const float fDeltaTime = input.m_stepInfo.m_deltaTime;
                const float fScaleA = msA->m_sweptTransform.m_centerOfMass1.m_quad[3] * fDeltaTime;
                const float fScaleB = fDeltaTime * msB->m_sweptTransform.m_centerOfMass1.m_quad[3];
                const __m128 kDiffA = _mm_mul_ps(
                    _mm_sub_ps(
                        msA->m_sweptTransform.m_centerOfMass0.m_quad, msA->m_sweptTransform.m_centerOfMass1.m_quad
                    ),
                    _mm_set1_ps(fScaleA)
                );
                in3.m_linearTimInfo.m_quad = _mm_add_ps(
                    _mm_mul_ps(
                        _mm_set1_ps(fScaleB),
                        _mm_sub_ps(
                            msB->m_sweptTransform.m_centerOfMass1.m_quad, msB->m_sweptTransform.m_centerOfMass0.m_quad
                        )
                    ),
                    kDiffA
                );
                in3.m_linearTimInfo.m_quad[3] = msB->m_deltaAngle.m_quad[3] * fScaleB * msB->m_objectRadius +
                    msA->m_deltaAngle.m_quad[3] * fScaleA * msA->m_objectRadius;
                in3.m_aTb.setMulInverseMul(msA->m_transform, msB->m_transform);
                input.m_dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(
                    in3, entry, static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1, nullptr, output
                );
                return;
            }
            case hkAgent3::STREAM_CALL_WITH_TIM: {
                hkpAgentNnMachineTimEntry* timEntry = static_cast<hkpAgentNnMachineTimEntry*>(entry);
                in3.m_bodyB = secondObject;
                in3.m_overrideBodyA = nullptr;
                in3.m_bodyA = firstObject;
                in3.m_contactMgr = entry->m_contactMgr;
                in3.m_input = &input;
                const hkMotionState* msA = firstObject->m_motion.pState;
                const hkMotionState* msB = secondObject->m_motion.pState;
                // EXE-INLINED none: hkSweptTransformUtil::calcTimInfo
                const float fDeltaTime = input.m_stepInfo.m_deltaTime;
                const float fScaleA = msA->m_sweptTransform.m_centerOfMass1.m_quad[3] * fDeltaTime;
                const float fScaleB = fDeltaTime * msB->m_sweptTransform.m_centerOfMass1.m_quad[3];
                const __m128 kDiffA = _mm_mul_ps(
                    _mm_sub_ps(
                        msA->m_sweptTransform.m_centerOfMass0.m_quad, msA->m_sweptTransform.m_centerOfMass1.m_quad
                    ),
                    _mm_set1_ps(fScaleA)
                );
                in3.m_linearTimInfo.m_quad = _mm_add_ps(
                    _mm_mul_ps(
                        _mm_set1_ps(fScaleB),
                        _mm_sub_ps(
                            msB->m_sweptTransform.m_centerOfMass1.m_quad, msB->m_sweptTransform.m_centerOfMass0.m_quad
                        )
                    ),
                    kDiffA
                );
                in3.m_linearTimInfo.m_quad[3] = msA->m_deltaAngle.m_quad[3] * fScaleA * msA->m_objectRadius +
                    msB->m_deltaAngle.m_quad[3] * fScaleB * msB->m_objectRadius;

                float distAtT1;
                if (input.m_stepInfo.m_startTime != timEntry->m_timeOfSeparatingNormal) {
                    if (!input.m_collisionQualityInfo->m_useContinuousPhysics) {
                        // No continuous physics: no separating normal, and the agent always runs.
                        timEntry->m_timeOfSeparatingNormal = input.m_stepInfo.m_endTime;
                        timEntry->m_separatingNormal.setZero4();
                        distAtT1 = -3.40282e+38f;
                        timEntry->m_separatingNormal.m_quad[3] = distAtT1;
                        goto processAgent;
                    }
                    // The separating normal at the step's start: both bodies at their start transforms.
                    alignas(16) hkAgentNnMachineBodyTemp bodyTemp;
                    alignas(16) hkpAgent3Input sepInput;
                    sepInput.m_overrideBodyA = nullptr;
                    hkSweptTransformUtil::lerp2(
                        in3.m_bodyA->m_motion.pState->m_sweptTransform, input.m_stepInfo.m_startTime, bodyTemp.m_transA
                    );
                    hkSweptTransformUtil::lerp2(
                        in3.m_bodyB->m_motion.pState->m_sweptTransform, in3.m_input->m_stepInfo.m_startTime,
                        bodyTemp.m_transB
                    );
                    sepInput.m_contactMgr = in3.m_contactMgr;
                    sepInput.m_bodyA = &bodyTemp.m_bodyA;
                    sepInput.m_input = in3.m_input;
                    sepInput.m_bodyB = &bodyTemp.m_bodyB;
                    bodyTemp.m_bodyA.m_shape = in3.m_bodyA->m_shape;
                    bodyTemp.m_bodyA.m_shapeKey = in3.m_bodyA->m_shapeKey;
                    bodyTemp.m_bodyA.m_motion.pMotion = &bodyTemp.m_transA;
                    bodyTemp.m_bodyA.m_parent = in3.m_bodyA;
                    bodyTemp.m_bodyB.m_shape = in3.m_bodyB->m_shape;
                    bodyTemp.m_bodyB.m_shapeKey = in3.m_bodyB->m_shapeKey;
                    bodyTemp.m_bodyB.m_motion.pMotion = &bodyTemp.m_transB;
                    bodyTemp.m_bodyB.m_parent = in3.m_bodyB;
                    sepInput.m_aTb.setMulInverseMul(bodyTemp.m_transA, bodyTemp.m_transB);
                    input.m_dispatcher->m_agent3Func[entry->m_agentType].m_sepNormalFunc(
                        sepInput, entry, timEntry + 1, timEntry->m_separatingNormal
                    );
                }
                {
                    timEntry->m_timeOfSeparatingNormal = input.m_stepInfo.m_endTime;
                    const float fProjected = in3.m_linearTimInfo.dot4xyz1(timEntry->m_separatingNormal);
                    distAtT1 = timEntry->m_separatingNormal.m_quad[3] - fProjected;
                    if (distAtT1 >= input.m_tolerance) {
                        // Still separated at the step's end: the agent does not run, and counts as skipped.
                        timEntry->m_separatingNormal.m_quad[3] = distAtT1;
                        if (entry->m_numContactPoints) {
                            input.m_dispatcher->m_agent3Func[entry->m_agentType].m_cleanupFunc(
                                entry, timEntry + 1, in3.m_contactMgr, *output.m_constraintOwner
                            );
                        }
                        numTim++;
                        return;
                    }
                }
            processAgent:
                in3.m_distAtT1 = distAtT1;
                in3.m_aTb.setMulInverseMul(msA->m_transform, msB->m_transform);
                input.m_dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(
                    in3, entry, timEntry + 1, &timEntry->m_separatingNormal, output
                );
                return;
            }
            case hkAgent3::STREAM_CALL_AGENT: {
                hkpAgentData* agentData = static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1;
                hkpCollisionAgent* agent = *static_cast<hkpCollisionAgent**>(agentData);
                agent->processCollision(*firstObject, *secondObject, input, output);
                return;
            }
            case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM: {
                uint8_t cdBodyHasTransformFlag;
                firstObject = hkAgentMachine_processTransformedShapes(
                    firstObject, newCdBodiesA, newMotionStatesA, 4, cdBodyHasTransformFlag
                );
                secondObject = hkAgentMachine_processTransformedShapes(
                    secondObject, newCdBodiesB, newMotionStatesB, 4, cdBodyHasTransformFlag
                );
                command = static_cast<uint8_t>(command & ~hkAgent3::TRANSFORM_FLAG);
                continue;
            }
            default: return;
        }
    }
}

// Checks each sector from its begin to its used size, compared unsigned. The entry is compared as an address into the
// sectors' untyped storage.
bool hkAgentNnMachine_IsEntryOnTrack(hkpAgentNnTrack& track, hkpAgentEntry* entry) {
    const uint8_t* pEntry = reinterpret_cast<const uint8_t*>(entry);
    for (int32_t i = 0; i < track.m_sectors.m_size; i++) {
        hkpAgentNnSector* sector = track.m_sectors.m_data[i];
        // EXE-INLINED 0xCB7460
        uint32_t sectorSize = (i + 1 == track.m_sectors.m_size) ? track.m_bytesUsedInLastSector : HK_AGENT3_SECTOR_SIZE;
        if (&sector->m_data[0] <= pEntry && pEntry < &sector->m_data[sectorSize]) {
            return true;
        }
    }
    return false;
}

// hk4dGskCollideCalcToi's setup of the vertex sweep input, as that body writes it.
const hkSphere*
initVertexCollidePointsInput(const hkpAgent3ProcessInput* r_eax, hkp4dGskVertexCollidePointsInput* r_esi) {
    const hkpConvexShape* pShapeA = static_cast<const hkpConvexShape*>(r_eax->m_bodyA->m_shape);
    const hkpConvexShape* pShapeB = static_cast<const hkpConvexShape*>(r_eax->m_bodyB->m_shape);
    const hkMotionState* pMotionA = r_eax->m_bodyA->m_motion.pState;
    const hkMotionState* pMotionB = r_eax->m_bodyB->m_motion.pState;
    r_esi->m_motionA.m_storage = pMotionA;
    r_esi->m_motionB.m_storage = pMotionB;
    r_esi->m_radiusSum.m_storage = pShapeB->m_radius + pShapeA->m_radius;
    r_esi->m_linearTimInfo = r_eax->m_linearTimInfo;
    r_esi->m_stepInfo.m_storage = &r_eax->m_input->m_stepInfo;
    const int32_t iNumSpheres = pShapeA->getNumCollisionSpheresImpl();
    r_esi->m_allocatedNumVertices.m_storage = iNumSpheres;
    r_esi->m_numVertices.m_storage = iNumSpheres;
    void* pBlock = hkAllocateStack<hkVector4>(iNumSpheres, nullptr);
    r_esi->m_verticesA.m_storage = static_cast<hkVector4*>(pBlock);
    // The stack block is untyped memory the spheres are written to and read back as vectors.
    const hkSphere* pResult = pShapeA->getCollisionSpheresImpl(static_cast<hkSphere*>(pBlock));
    const float fAccelB =
        pMotionB->m_objectRadius * r_esi->m_deltaAngles[1].m_quad[3] * r_esi->m_deltaAngles[1].m_quad[3];
    const float fAccelA =
        pMotionA->m_objectRadius * r_esi->m_deltaAngles[0].m_quad[3] * r_esi->m_deltaAngles[0].m_quad[3];
    r_esi->m_maxAccel.m_storage = fAccelB + fAccelA;
    r_esi->m_invMaxAccel.m_storage = 1.0 / (r_esi->m_maxAccel.m_storage + 1.1920928955078125e-07);
    return pResult;
}
