#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Gjk/Penetration/hkpConvexPenetrationUtil.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "xfnv/Compiler.h"

// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
static void hkAgent1nMachine_VisitAllAgents(
    hkpAgent1nTrack& agentTrack,
    hkpAgent1nMachine_VisitorInput& vin,
    void* (*visitor)(hkpAgent1nMachine_VisitorInput&, hkpAgent1nMachineEntry*, void*)
);

// Copies every agent of the track into fresh sectors, calling the visitor on each agent entry; the visitor returns
// where the next entry goes. A sector that fills up is written out to the track, and the entries that did not fit
// wait in a 128-byte buffer until the next sector. Read sectors are freed once their last entry is copied, the
// last one being reused as the next write sector.
static void hkAgent1nMachine_VisitAllAgents(
    hkpAgent1nTrack& agentTrack,
    hkpAgent1nMachine_VisitorInput& vin,
    void* (*visitor)(hkpAgent1nMachine_VisitorInput&, hkpAgent1nMachineEntry*, void*)
) {
    alignas(16) uint8_t buffer[128];
    hkpAgent1nMachine_VisitorInput vinCopy;

    hkpAgent1nSector* readSector = agentTrack.m_sectors.m_data[0];
    int32_t outSectorIndex = 0;
    hkpAgent1nSector* sectorToFree = nullptr;
    // EXE-INLINED none: hkpAgent1nSector::operator new, hkpAgent1nSector::hkpAgent1nSector
    hkpAgent1nSector* writeSector =
        static_cast<hkpAgent1nSector*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpAgent1nSector)));
    if (writeSector) {
        writeSector->m_bytesAllocated = 0;
    }
    uint8_t* readData = readSector->m_data;
    uint8_t* writePtr = writeSector->m_data;
    int32_t nextReadSectorIndex = 1;
    uint8_t* readEnd = readSector->m_data + readSector->m_bytesAllocated;
    uint8_t* writeEnd = writeSector->m_data + hkpAgent1nSector::NET_SECTOR_SIZE;
    int32_t bytesInSector = 0;
    int32_t bytesInBuffer = 0;
    bool inBuffer = false;

    while (true) {
        // A sector's bytes are untyped: a stream of agent entries, each followed by its agent's data.
        const hkpAgentEntry* readEntry = reinterpret_cast<const hkpAgentEntry*>(readData);
        hkString::memCpy16NonEmpty(writePtr, readData, readEntry->m_size >> 4);
        readData += readEntry->m_size;
        vinCopy = vin;

        // The copied entry, in the write sector's (or the buffer's) untyped bytes.
        hkpAgent1nMachineEntry* entry = reinterpret_cast<hkpAgent1nMachineEntry*>(writePtr);
        int32_t entrySize = sizeof(hkpAgent1nMachinePaddedEntry);
        switch (entry->m_streamCommand & ~hkAgent3::TRANSFORM_FLAG) {
            case hkAgent3::STREAM_END: writePtr += sizeof(hkpAgent1nMachinePaddedEntry); break;
            case hkAgent3::STREAM_CALL_WITH_TIM:
            case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED: entrySize = sizeof(hkpAgent1nMachineTimEntry); [[fallthrough]];
            case hkAgent3::STREAM_CALL:
            case hkAgent3::STREAM_CALL_FLIPPED:
            case hkAgent3::STREAM_CALL_AGENT:
                writePtr = static_cast<uint8_t*>(visitor(vinCopy, entry, writePtr + entrySize));
                break;
            default: break;
        }

        if (readData == readEnd) {
            if (sectorToFree) {
                // EXE-INLINED none: hkpAgent1nSector::operator delete
                hkMemoryRouter::getInstance().heap().blockFree(sectorToFree, sizeof(hkpAgent1nSector));
            }
            sectorToFree = readSector;
            if (nextReadSectorIndex >= agentTrack.m_sectors.m_size) {
                break;
            }
            readSector = agentTrack.m_sectors.m_data[nextReadSectorIndex];
            readData = readSector->m_data;
            readEnd = readSector->m_data + readSector->m_bytesAllocated;
            nextReadSectorIndex++;
        }

        // Room for at least 128 more bytes: go on writing in place.
        if (writePtr + 128 <= writeEnd) {
            continue;
        }
        if (!inBuffer) {
            bytesInSector = static_cast<int32_t>(writePtr - writeSector->m_data);
            inBuffer = true;
            writePtr = buffer;
            writeEnd = buffer + sizeof(buffer);
            continue;
        }
        bytesInBuffer = static_cast<int32_t>(writePtr - buffer);
        if (bytesInBuffer <= hkpAgent1nSector::NET_SECTOR_SIZE - bytesInSector) {
            hkString::memCpy16NonEmpty(writeSector->m_data + bytesInSector, buffer, bytesInBuffer >> 4);
            bytesInSector += bytesInBuffer;
            if (bytesInSector < hkpAgent1nSector::NET_SECTOR_SIZE) {
                writePtr = buffer;
                continue;
            }
            inBuffer = false;
        }

        // The write sector is full: store it in the track, before the sectors still to read.
        writeSector->m_bytesAllocated = bytesInSector;
        if (outSectorIndex >= nextReadSectorIndex) {
            agentTrack.m_sectors.expandAt(nextReadSectorIndex, 1);
            nextReadSectorIndex++;
        }
        agentTrack.m_sectors.m_data[outSectorIndex] = writeSector;
        outSectorIndex++;
        if (sectorToFree) {
            writeSector = sectorToFree;
            sectorToFree = nullptr;
        }
        else {
            // EXE-INLINED none: hkpAgent1nSector::operator new, hkpAgent1nSector::hkpAgent1nSector
            writeSector = static_cast<hkpAgent1nSector*>(
                hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpAgent1nSector))
            );
            if (writeSector) {
                writeSector->m_bytesAllocated = 0;
            }
        }
        writePtr = writeSector->m_data;
        writeEnd = writeSector->m_data + hkpAgent1nSector::NET_SECTOR_SIZE;
        if (inBuffer) {
            hkString::memCpy16NonEmpty(writePtr, buffer, bytesInBuffer >> 4);
            writePtr += bytesInBuffer;
            inBuffer = false;
        }
    }

    if (!inBuffer) {
        bytesInSector = static_cast<int32_t>(writePtr - writeSector->m_data);
    }
    else {
        bytesInBuffer = static_cast<int32_t>(writePtr - buffer);
        hkString::memCpy16NonEmpty(writeSector->m_data + bytesInSector, buffer, bytesInBuffer >> 4);
        bytesInSector += bytesInBuffer;
    }
    writeSector->m_bytesAllocated = bytesInSector;
    agentTrack.m_sectors.setSize(outSectorIndex + 1);
    agentTrack.m_sectors.m_data[outSectorIndex] = writeSector;
    if (sectorToFree) {
        // EXE-INLINED none: hkpAgent1nSector::operator delete
        hkMemoryRouter::getInstance().heap().blockFree(sectorToFree, sizeof(hkpAgent1nSector));
    }
}

// Walks the sectors in order, with no bound: the key pair is relied on to be in the track. In each sector the entries
// run from its data to its bytes in use. An entry that ends the stream ends the search; a padded or time-caching entry
// is compared, both keys (B first, as the game does); a null entry is skipped. For the commands 7, 8, 9 and any above
// 14 the loop test is repeated without advancing, so such an entry would loop forever, as the game does. The sector's
// data is an untyped byte stream of entries of varying size (m_size bytes each), hence the casts.
hkpAgent1nMachineEntry*
hkAgent1nMachine_FindAgent(hkpAgent1nTrack& agentTrack, hkShapeKeyPair keyPair, hkpAgentData** agentDataOut) {
    hkpAgent1nSector** sectors = agentTrack.m_sectors.m_data;
    for (;;) {
        hkpAgent1nSector* sector = *sectors++;
        uint8_t* end = sector->m_data + sector->m_bytesAllocated;
        hkpAgent1nMachineEntry* entry = reinterpret_cast<hkpAgent1nMachineEntry*>(sector->m_data);
        while (reinterpret_cast<uint8_t*>(entry) < end) {
            hkpAgentData* agentData;
            switch (entry->m_streamCommand) {
                case hkAgent3::STREAM_NULL:
                    entry =
                        reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
                    continue;
                case hkAgent3::STREAM_END: return nullptr;
                case hkAgent3::STREAM_CALL:
                case hkAgent3::STREAM_CALL_FLIPPED:
                case hkAgent3::STREAM_CALL_AGENT:
                case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
                case hkAgent3::STREAM_CALL_FLIPPED_WITH_TRANSFORM:
                case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM:
                    agentData = static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 1;
                    break;
                case hkAgent3::STREAM_CALL_WITH_TIM:
                case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED:
                case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
                case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED_WITH_TRANSFORM:
                    agentData = static_cast<hkpAgent1nMachineTimEntry*>(entry) + 1;
                    break;
                default: continue;
            }
            if (entry->m_shapeKeyPair.m_shapeKeyB == keyPair.m_shapeKeyB &&
                entry->m_shapeKeyPair.m_shapeKeyA == keyPair.m_shapeKeyA) {
                *agentDataOut = agentData;
                return entry;
            }
            entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
        }
    }
}

// Welds the potential contacts the track's agents proposed. Each potential contact is tested against every
// representative contact: one that lies too far behind a representative (beyond the keep-contact tolerance, when the
// representative is a committed point) is dropped; one that sits in a crease against a new representative is a
// conflict. Every other point is added to the contact manager. The conflicting agents become zombies, and the conflicts
// are resolved in pairs, the two with the most opposed normals first, by a joint penetration depth of shape A against
// both convex children; a last unpaired conflict loses its point. Then each agent commits its new id, or removes its
// point, whose slot the last contact point of output fills. Leaving the conflict loop through a non-convex shape leaves
// the timer begun and not ended, deliberately, as the game does.
void hkAgent1nMachine_Weld(
    hkpAgent3Input& input,
    const hkpShapeContainer* shapeCollection,
    hkpProcessCollisionOutput& output
) {
    hkpProcessCollisionOutput::PotentialInfo& potentialInfo = *output.m_potentialContacts.m_storage;
    const hkpConvexShape* shapeA = static_cast<const hkpConvexShape*>(input.m_bodyA->m_shape);

    alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
    alignas(16) char shapeBuffer2[HK_SHAPE_BUFFER_SIZE];
    uint16_t conflicts[256];
    uint16_t newIds[256];
    int32_t numConflicts = 0;

    int32_t i = 0;
    for (hkpProcessCollisionOutput::ContactRef* ref = potentialInfo.m_potentialContacts;
         ref < potentialInfo.m_firstFreePotentialContact; ref++, i++) {
        hkpProcessCdPoint* point = ref->m_contactPoint;
        newIds[i] = HK_INVALID_CONTACT_POINT;
        bool isConflict = false;
        bool rejected = false;
        for (hkpProcessCdPoint** reps = potentialInfo.m_representativeContacts;
             reps < potentialInfo.m_firstFreeRepresentativeContact; reps++) {
            hkpProcessCdPoint* rep = *reps;
            if (point == rep) {
                continue;
            }
            const hkVector4& pointNormal = point->m_contact.m_separatingNormal;
            const hkVector4& repNormal = rep->m_contact.m_separatingNormal;
            const hkpCollisionQualityInfo* qualityInfo = input.m_input->m_collisionQualityInfo;
            hkVector4 diff;
            diff.m_quad = _mm_sub_ps(rep->m_contact.m_position.m_quad, point->m_contact.m_position.m_quad);
            hkVector4 dotsOut;
            hkVector4Util::dot3_3vs3(diff, pointNormal, repNormal, diff, repNormal, pointNormal, dotsOut);
            const hkQuadReal dots = dotsOut.m_quad;

            const float cosAngle = XFNV_M128_F32(dots, 2);
            const double scaledRadius = (cosAngle - 1.0) * shapeA->m_radius;
            const float distA = scaledRadius - XFNV_M128_F32(dots, 1);
            const float distB = scaledRadius + XFNV_M128_F32(dots, 0);
            const float radius = shapeA->m_radius + 0.0;
            if (radius < distA) {
                continue;
            }
            if (cosAngle > 0.99989998340606689453125 && rep->m_contactPointId.m_storage == HK_INVALID_CONTACT_POINT) {
                continue;
            }
            if (rep->m_contactPointId.m_storage == HK_INVALID_CONTACT_POINT && cosAngle < 0.800000011920928955078125 &&
                distA + distB < 0.1000000014901161193847656) {
                // The doubled radius is rounded to a float before the compare.
                const float twiceRadius = -radius - radius;
                if (twiceRadius > rep->m_contact.m_separatingNormal(3) + point->m_contact.m_separatingNormal(3)) {
                    isConflict = true;
                    continue;
                }
            }
            float tolerance = 0.0f;
            if (rep->m_contactPointId.m_storage != HK_INVALID_CONTACT_POINT) {
                tolerance = qualityInfo->m_keepContact * 0.05000000074505805969238281;
            }
            if (!(rep->m_contact.m_separatingNormal(3) - tolerance >= point->m_contact.m_separatingNormal(3))) {
                rejected = true;
                break;
            }
        }
        if (rejected) {
            continue;
        }
        if (isConflict) {
            conflicts[numConflicts++] = static_cast<uint16_t>(i);
        }

        hkpAgent1nMachineEntry* entry = static_cast<hkpAgent1nMachineEntry*>(ref->m_agentEntry);
        const hkpShape* childShape = shapeCollection->getChildShape(entry->m_shapeKeyPair.m_shapeKeyB, shapeBuffer);
        hkpCdBody bodyB;
        bodyB.m_parent = input.m_bodyB;
        bodyB.m_motion = input.m_bodyB->m_motion;
        bodyB.m_shape = childShape;
        bodyB.m_shapeKey = entry->m_shapeKeyPair.m_shapeKeyB;
        const uint16_t id = input.m_contactMgr->addContactPoint(
            *input.m_bodyA, bodyB, *input.m_input, output, nullptr, ref->m_contactPoint->m_contact
        );
        newIds[i] = id;
        if (id != HK_INVALID_CONTACT_POINT) {
            input.m_contactMgr->reserveContactPoints(-1);
        }
    }

    hkpCollisionDispatcher* dispatcher = input.m_input->m_dispatcher;
    for (int32_t k = 0; k < numConflicts; k++) {
        hkpProcessCollisionOutput::ContactRef& ref = potentialInfo.m_potentialContacts[conflicts[k]];
        dispatcher->m_agent3Func[ref.m_agentEntry->m_agentType].m_createZombieFunc(
            ref.m_agentEntry, ref.m_agentData, HK_INVALID_CONTACT_POINT
        );
    }

    while (numConflicts >= 2) {
        HK_TIMER_BEGIN("Conflicts", nullptr);
        // The pair with the smallest dot product of their normals.
        float bestDot = 2.0f;
        int32_t bestI = 0;
        int32_t bestJ = 1;
        for (int32_t a = 0; a < numConflicts; a++) {
            const hkVector4& normalA =
                potentialInfo.m_potentialContacts[conflicts[a]].m_contactPoint->m_contact.m_separatingNormal;
            for (int32_t b = a + 1; b < numConflicts; b++) {
                const hkVector4& normalB =
                    potentialInfo.m_potentialContacts[conflicts[b]].m_contactPoint->m_contact.m_separatingNormal;
                const float dot = normalB.dot3(normalA);
                if (bestDot > dot) {
                    bestDot = dot;
                    bestI = a;
                    bestJ = b;
                }
            }
        }
        uint16_t* conflictI = &conflicts[bestI];
        uint16_t* conflictJ = &conflicts[bestJ];
        hkpProcessCollisionOutput::ContactRef& refI = potentialInfo.m_potentialContacts[*conflictI];
        hkpProcessCollisionOutput::ContactRef& refJ = potentialInfo.m_potentialContacts[*conflictJ];
        const hkpConvexShape* convexA = static_cast<const hkpConvexShape*>(input.m_bodyA->m_shape);
        if (!(input.m_input->m_dispatcher->m_hasAlternateType[convexA->m_type.m_storage] & (1u << HK_SHAPE_CONVEX))) {
            break;
        }
        const hkpShape* childI = shapeCollection->getChildShape(
            static_cast<hkpAgent1nMachineEntry*>(refI.m_agentEntry)->m_shapeKeyPair.m_shapeKeyB, shapeBuffer
        );
        if (!(input.m_input->m_dispatcher->m_hasAlternateType[childI->m_type.m_storage] & (1u << HK_SHAPE_CONVEX))) {
            break;
        }
        const hkpShape* childJ = shapeCollection->getChildShape(
            static_cast<hkpAgent1nMachineEntry*>(refJ.m_agentEntry)->m_shapeKeyPair.m_shapeKeyB, shapeBuffer2
        );
        if (!(input.m_input->m_dispatcher->m_hasAlternateType[childJ->m_type.m_storage] & (1u << HK_SHAPE_CONVEX))) {
            break;
        }
        const hkpConvexShape* shapesB[2] = {
            static_cast<const hkpConvexShape*>(childI), static_cast<const hkpConvexShape*>(childJ)
        };
        hkContactPoint* points[2] = {&refI.m_contactPoint->m_contact, &refJ.m_contactPoint->m_contact};
        hkCalcMultiPenetrationDepth(*input.m_bodyA->m_motion.pMotion, convexA, shapesB, 2, input.m_aTb, points);
        // Both leave the list: the last two take their places.
        *conflictJ = conflicts[--numConflicts];
        *conflictI = conflicts[--numConflicts];
        HK_TIMER_END();
    }

    if (numConflicts != 0) {
        uint16_t& id = newIds[conflicts[0]];
        if (id != HK_INVALID_CONTACT_POINT) {
            input.m_contactMgr->removeContactPoint(id, *output.m_constraintOwner.m_storage);
            id = HK_INVALID_CONTACT_POINT;
            input.m_contactMgr->reserveContactPoints(1);
        }
    }

    hkpCollisionDispatcher* commitDispatcher = input.m_input->m_dispatcher;
    hkpProcessCollisionOutput::ContactRef* ref = potentialInfo.m_firstFreePotentialContact;
    // Divides the byte span, signed.
    for (int32_t n =
             static_cast<int32_t>(
                 (reinterpret_cast<intptr_t>(ref) - reinterpret_cast<intptr_t>(potentialInfo.m_potentialContacts)) /
                 static_cast<intptr_t>(sizeof(hkpProcessCollisionOutput::ContactRef))
             ) -
             1;
         n >= 0; n--) {
        const uint16_t id = newIds[n];
        ref--;
        if (id != HK_INVALID_CONTACT_POINT) {
            commitDispatcher->m_agent3Func[ref->m_agentEntry->m_agentType].m_commitPotentialFunc(
                ref->m_agentEntry, ref->m_agentData, id
            );
            ref->m_contactPoint->m_contactPointId.m_storage = id;
        }
        else {
            input.m_contactMgr->reserveContactPoints(-1);
            commitDispatcher->m_agent3Func[ref->m_agentEntry->m_agentType].m_removePointFunc(
                ref->m_agentEntry, ref->m_agentData, HK_INVALID_CONTACT_POINT
            );
            // The last contact point of output fills the removed one's slot: its members are copied, not the padding
            // after them.
            output.m_firstFreeContactPoint.m_storage--;
            const hkpProcessCdPoint* last = output.m_firstFreeContactPoint.m_storage;
            ref->m_contactPoint->m_contact = last->m_contact;
            ref->m_contactPoint->m_contactPointId.m_storage = last->m_contactPointId.m_storage;
        }
    }
}

// Asks the world's filter whether the pair of body A and the child B's key names may still collide. Yes: the entry's
// agent, when its type has an update-filter function, gets that child's body (under the collection's body, through
// the transform shapes above it when the entry carries the transform flag), in the entry's order, and the next entry
// is returned. No: the agent is destroyed and the entry itself is returned. The sector's data is an untyped byte
// stream of entries of m_size bytes each, hence the cast to the next one.
void* hkAgent1nMachine_UpdateShapeCollectionFilterVisitor(
    hkpAgent1nMachine_VisitorInput& vin,
    hkpAgent1nMachineEntry* entry,
    void* agentData
) {
    uint32_t key = entry->m_shapeKeyPair.m_shapeKeyB;
    if (vin.m_input->m_filter
            ->isCollisionEnabled(*vin.m_input, *vin.m_bodyA, *vin.m_collectionBodyB, *vin.m_containerShapeB, key)
            .m_bool) {
        hkAgent3::UpdateFilterFunc func =
            vin.m_input->m_dispatcher->m_agent3Func[entry->m_agentType].m_updateFilterFunc;
        if (func) {
            alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
            const hkpShape* shape = vin.m_containerShapeB->getChildShape(key, shapeBuffer);
            alignas(16) hkpCdBody modifiedBodyB;
            modifiedBodyB.m_parent = vin.m_collectionBodyB;
            modifiedBodyB.m_motion = modifiedBodyB.m_parent->m_motion;
            modifiedBodyB.m_shape = shape;
            modifiedBodyB.m_shapeKey = key;
            const hkpCdBody* bodyA = vin.m_bodyA;
            const hkpCdBody* bodyB = &modifiedBodyB;
            alignas(16) hkpCdBody newCdBodies[4];
            hkMotionState newMotionStates[4];
            if (entry->m_streamCommand & hkAgent3::TRANSFORM_FLAG) {
                uint8_t cdBodyHasTransformFlag = 0;
                bodyB = hkAgentMachine_processTransformedShapes(
                    bodyB, newCdBodies, newMotionStates, 4, cdBodyHasTransformFlag
                );
            }
            uint8_t command = entry->m_streamCommand & 7;
            if (command == hkAgent3::STREAM_CALL_FLIPPED || command == hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED) {
                func(entry, agentData, *bodyB, *bodyA, *vin.m_input, vin.m_contactMgr, *vin.m_constraintOwner);
            }
            else {
                func(entry, agentData, *bodyA, *bodyB, *vin.m_input, vin.m_contactMgr, *vin.m_constraintOwner);
            }
        }
        // EXE-INLINED none: hkAddByteOffset<hkpAgent1nMachineEntry>
        return reinterpret_cast<uint8_t*>(entry) + entry->m_size;
    }
    vin.m_input->m_dispatcher->m_agent3Func[entry->m_agentType].m_destroyFunc(
        entry, agentData, vin.m_contactMgr, *vin.m_constraintOwner, vin.m_input->m_dispatcher
    );
    return entry;
}

// The same for an Nm track, whose key pair names a child of each container: both keys go to the filter, and the
// agent gets both children's bodies (B's transform shapes walked first, then A's, with one flag between them).
void* hkAgentNmMachine_UpdateShapeCollectionFilterVisitor(
    hkpAgent1nMachine_VisitorInput& vin,
    hkpAgent1nMachineEntry* entry,
    void* agentData
) {
    uint32_t keyA = entry->m_shapeKeyPair.m_shapeKeyA;
    uint32_t keyB = entry->m_shapeKeyPair.m_shapeKeyB;
    if (vin.m_input->m_filter
            ->isCollisionEnabled(
                *vin.m_input, *vin.m_bodyA, *vin.m_collectionBodyB, *vin.m_containerShapeA, *vin.m_containerShapeB,
                keyA, keyB
            )
            .m_bool) {
        hkAgent3::UpdateFilterFunc func =
            vin.m_input->m_dispatcher->m_agent3Func[entry->m_agentType].m_updateFilterFunc;
        if (func) {
            alignas(16) char shapeBufferB[HK_SHAPE_BUFFER_SIZE];
            const hkpShape* shapeB = vin.m_containerShapeB->getChildShape(keyB, shapeBufferB);
            alignas(16) char shapeBufferA[HK_SHAPE_BUFFER_SIZE];
            const hkpShape* shapeA = vin.m_containerShapeA->getChildShape(keyA, shapeBufferA);
            hkpCdBody modifiedBodyB;
            modifiedBodyB.m_parent = vin.m_collectionBodyB;
            modifiedBodyB.m_motion = modifiedBodyB.m_parent->m_motion;
            modifiedBodyB.m_shape = shapeB;
            modifiedBodyB.m_shapeKey = keyB;
            hkpCdBody modifiedBodyA;
            modifiedBodyA.m_parent = vin.m_bodyA;
            modifiedBodyA.m_motion = modifiedBodyA.m_parent->m_motion;
            modifiedBodyA.m_shape = shapeA;
            modifiedBodyA.m_shapeKey = keyA;
            const hkpCdBody* bodyB = &modifiedBodyB;
            const hkpCdBody* bodyA = &modifiedBodyA;
            alignas(16) hkpCdBody newCdBodiesB[4];
            hkMotionState newMotionStatesB[4];
            alignas(16) hkpCdBody newCdBodiesA[4];
            hkMotionState newMotionStatesA[4];
            if (entry->m_streamCommand & hkAgent3::TRANSFORM_FLAG) {
                uint8_t cdBodyHasTransformFlag = 0;
                bodyB = hkAgentMachine_processTransformedShapes(
                    bodyB, newCdBodiesB, newMotionStatesB, 4, cdBodyHasTransformFlag
                );
                bodyA = hkAgentMachine_processTransformedShapes(
                    bodyA, newCdBodiesA, newMotionStatesA, 4, cdBodyHasTransformFlag
                );
            }
            uint8_t command = entry->m_streamCommand & 7;
            if (command == hkAgent3::STREAM_CALL_FLIPPED || command == hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED) {
                func(entry, agentData, *bodyB, *bodyA, *vin.m_input, vin.m_contactMgr, *vin.m_constraintOwner);
            }
            else {
                func(entry, agentData, *bodyA, *bodyB, *vin.m_input, vin.m_contactMgr, *vin.m_constraintOwner);
            }
        }
        // EXE-INLINED none: hkAddByteOffset<hkpAgent1nMachineEntry>
        return reinterpret_cast<uint8_t*>(entry) + entry->m_size;
    }
    vin.m_input->m_dispatcher->m_agent3Func[entry->m_agentType].m_destroyFunc(
        entry, agentData, vin.m_contactMgr, *vin.m_constraintOwner, vin.m_input->m_dispatcher
    );
    return entry;
}

// Destroys the track's agents in stream order through the dispatcher's destroy functions, freeing each sector once its
// entries are done, then the sector array. As in hkAgent1nMachine_FindAgent, the sectors are walked with no bound (the
// stream's end entry stops the walk), and an entry whose command is not handled (7, 8, 9 and any above 14) is tested
// again without advancing, as the game does. A finished sector is freed through the sector array as it was on entry;
// the next sector is read from the track's array as it is then. The sector's data is an untyped byte stream of entries
// of m_size bytes each, hence the casts.
void hkAgent1nMachine_Destroy(
    hkpAgent1nTrack& agentTrack,
    hkpCollisionDispatcher* dispatcher,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner
) {
    hkpAgent1nSector** sectors = agentTrack.m_sectors.m_data;
    hkpAgent1nSector* sector = sectors[0];
    hkpAgent1nMachineEntry* entry = reinterpret_cast<hkpAgent1nMachineEntry*>(sector->m_data);
    uint8_t* end = sector->m_data + sector->m_bytesAllocated;
    int32_t sectorIndex = 0;
    for (;;) {
        hkpAgent1nMachineEntry* current = entry;
        hkpAgentData* agentData;
        switch (current->m_streamCommand) {
            case hkAgent3::STREAM_NULL:
                entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
                break;
            case hkAgent3::STREAM_END: {
                hkpAgent1nSector* lastSector = sectors[sectorIndex];
                if (lastSector) {
                    hkMemoryRouter::getInstance().heap().blockFree(lastSector, sizeof(hkpAgent1nSector));
                }
                agentTrack.m_sectors.clearAndDeallocate();
                return;
            }
            case hkAgent3::STREAM_CALL:
            case hkAgent3::STREAM_CALL_FLIPPED:
            case hkAgent3::STREAM_CALL_AGENT:
            case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_FLIPPED_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM:
                agentData = static_cast<hkpAgent1nMachinePaddedEntry*>(current) + 1;
                entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
                dispatcher->m_agent3Func[current->m_agentType].m_destroyFunc(
                    current, agentData, mgr, constraintOwner, dispatcher
                );
                break;
            case hkAgent3::STREAM_CALL_WITH_TIM:
            case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED:
            case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED_WITH_TRANSFORM:
                agentData = static_cast<hkpAgent1nMachineTimEntry*>(current) + 1;
                entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
                dispatcher->m_agent3Func[current->m_agentType].m_destroyFunc(
                    current, agentData, mgr, constraintOwner, dispatcher
                );
                break;
            default: break;
        }
        if (reinterpret_cast<uint8_t*>(entry) >= end) {
            hkpAgent1nSector* doneSector = sectors[sectorIndex++];
            if (doneSector) {
                hkMemoryRouter::getInstance().heap().blockFree(doneSector, sizeof(hkpAgent1nSector));
            }
            sector = agentTrack.m_sectors.m_data[sectorIndex];
            entry = reinterpret_cast<hkpAgent1nMachineEntry*>(sector->m_data);
            end = sector->m_data + sector->m_bytesAllocated;
        }
    }
}

// Clears the track, reserves four sectors, appends one and fills it with a new sector
// (hkpAgent1nSector's class operator new from the heap and its constructor) whose only entry ends the stream. The
// sector's data is an untyped byte stream of entries, hence the cast.
void hkAgent1nMachine_Create(hkpAgent1nTrack& agentTrack) {
    agentTrack.m_sectors.clear();
    agentTrack.m_sectors.reserve(4);
    agentTrack.m_sectors.expandOne();
    // EXE-INLINED none: hkpAgent1nSector::operator new, hkpAgent1nSector::hkpAgent1nSector
    hkpAgent1nSector* sector =
        static_cast<hkpAgent1nSector*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpAgent1nSector)));
    if (sector) {
        sector->m_bytesAllocated = 0;
    }
    agentTrack.m_sectors.m_data[0] = sector;
    hkpAgent1nMachinePaddedEntry* endEntry = reinterpret_cast<hkpAgent1nMachinePaddedEntry*>(sector->m_data);
    endEntry->m_size = sizeof(hkpAgent1nMachinePaddedEntry);
    endEntry->m_streamCommand = hkAgent3::STREAM_END;
    endEntry->m_shapeKeyPair.m_shapeKeyB = HK_INVALID_SHAPE_KEY;
    endEntry->m_shapeKeyPair.m_shapeKeyA = HK_INVALID_SHAPE_KEY;
    sector->m_bytesAllocated = sizeof(hkpAgent1nMachinePaddedEntry);
}

// Invalidates the time of impact each agent of the track caches: a time-caching entry gets -1 as its time of
// separating normal and a zero normal, then every calling entry is handed to the dispatcher's invalidate function
// when it has one. The walk is the one of hkAgent1nMachine_WarpTime; the casts are its untyped byte stream's.
void hkAgent1nMachine_InvalidateTim(hkpAgent1nTrack& agentTrack, const hkpCollisionInput& input) {
    hkpAgent1nSector* sector = agentTrack.m_sectors.m_data[0];
    hkpAgent1nMachineEntry* entry = reinterpret_cast<hkpAgent1nMachineEntry*>(sector->m_data);
    uint8_t* end = sector->m_data + sector->m_bytesAllocated;
    int32_t nextSector = 1;
    for (;;) {
        hkpAgent1nMachineEntry* current = entry;
        hkpAgentData* agentData;
        switch (current->m_streamCommand) {
            case hkAgent3::STREAM_NULL:
                entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
                break;
            case hkAgent3::STREAM_END: return;
            case hkAgent3::STREAM_CALL:
            case hkAgent3::STREAM_CALL_FLIPPED:
            case hkAgent3::STREAM_CALL_AGENT:
            case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_FLIPPED_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM: {
                agentData = static_cast<hkpAgent1nMachinePaddedEntry*>(current) + 1;
                entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
                hkAgent3::InvalidateTimFunc invalidateTim =
                    input.m_dispatcher->m_agent3Func[current->m_agentType].m_invalidateTimFunc;
                if (invalidateTim) {
                    invalidateTim(current, agentData, input);
                }
                break;
            }
            case hkAgent3::STREAM_CALL_WITH_TIM:
            case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED:
            case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED_WITH_TRANSFORM: {
                hkpAgent1nMachineTimEntry* timEntry = static_cast<hkpAgent1nMachineTimEntry*>(current);
                timEntry->m_timeOfSeparatingNormal = -1.0f;
                agentData = timEntry + 1;
                timEntry->m_separatingNormal.setZero4();
                entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
                hkAgent3::InvalidateTimFunc invalidateTim =
                    input.m_dispatcher->m_agent3Func[current->m_agentType].m_invalidateTimFunc;
                if (invalidateTim) {
                    invalidateTim(current, agentData, input);
                }
                break;
            }
            default: break;
        }
        if (reinterpret_cast<uint8_t*>(entry) >= end) {
            sector = agentTrack.m_sectors.m_data[nextSector];
            entry = reinterpret_cast<hkpAgent1nMachineEntry*>(sector->m_data);
            end = sector->m_data + sector->m_bytesAllocated;
            ++nextSector;
        }
    }
}

// Warps the time each agent of the track caches from oldTime to newTime through the dispatcher's warp functions. An
// agent that caches its time of separating normal keeps it when it was taken at oldTime (moved to newTime), else the
// cached normal is invalidated (-1). The walk is the one of hkAgent1nMachine_Destroy, with no frees; the casts are its
// untyped byte stream's.
void hkAgent1nMachine_WarpTime(
    hkpAgent1nTrack& agentTrack,
    float oldTime,
    float newTime,
    const hkpCollisionInput& input
) {
    hkpAgent1nSector* sector = agentTrack.m_sectors.m_data[0];
    hkpAgent1nMachineEntry* entry = reinterpret_cast<hkpAgent1nMachineEntry*>(sector->m_data);
    uint8_t* end = sector->m_data + sector->m_bytesAllocated;
    int32_t nextSector = 1;
    for (;;) {
        hkpAgent1nMachineEntry* current = entry;
        hkpAgentData* agentData;
        switch (current->m_streamCommand) {
            case hkAgent3::STREAM_NULL:
                entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
                break;
            case hkAgent3::STREAM_END: return;
            case hkAgent3::STREAM_CALL:
            case hkAgent3::STREAM_CALL_FLIPPED:
            case hkAgent3::STREAM_CALL_AGENT:
            case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_FLIPPED_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM: {
                agentData = static_cast<hkpAgent1nMachinePaddedEntry*>(current) + 1;
                entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
                hkAgent3::WarpTimeFunc warpTime = input.m_dispatcher->m_agent3Func[current->m_agentType].m_warpTimeFunc;
                if (warpTime) {
                    warpTime(current, agentData, oldTime, newTime, input);
                }
                break;
            }
            case hkAgent3::STREAM_CALL_WITH_TIM:
            case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED:
            case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
            case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED_WITH_TRANSFORM: {
                hkpAgent1nMachineTimEntry* timEntry = static_cast<hkpAgent1nMachineTimEntry*>(current);
                agentData = timEntry + 1;
                if (timEntry->m_timeOfSeparatingNormal == oldTime) {
                    timEntry->m_timeOfSeparatingNormal = newTime;
                }
                else {
                    timEntry->m_timeOfSeparatingNormal = -1.0f;
                }
                entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
                hkAgent3::WarpTimeFunc warpTime = input.m_dispatcher->m_agent3Func[current->m_agentType].m_warpTimeFunc;
                if (warpTime) {
                    warpTime(current, agentData, oldTime, newTime, input);
                }
                break;
            }
            default: break;
        }
        if (reinterpret_cast<uint8_t*>(entry) >= end) {
            sector = agentTrack.m_sectors.m_data[nextSector];
            entry = reinterpret_cast<hkpAgent1nMachineEntry*>(sector->m_data);
            end = sector->m_data + sector->m_bytesAllocated;
            ++nextSector;
        }
    }
}

void hkAgent1nMachine_UpdateShapeCollectionFilter(hkpAgent1nTrack& agentTrack, hkpAgent1nMachine_VisitorInput& vin) {
    hkAgent1nMachine_VisitAllAgents(agentTrack, vin, &hkAgent1nMachine_UpdateShapeCollectionFilterVisitor);
}

void hkAgentNmMachine_UpdateShapeCollectionFilter(hkpAgent1nTrack& agentTrack, hkpAgent1nMachine_VisitorInput& vin) {
    hkAgent1nMachine_VisitAllAgents(agentTrack, vin, &hkAgentNmMachine_UpdateShapeCollectionFilterVisitor);
}

// Moves the potential contacts' agent pointers along with agent data copied from oldStart to newStart: each contact
// whose agent entry lies in the size bytes from oldStart gets the entry at the same offset from newStart, and its agent
// data moves by the same distance. The agent entries and their data are an untyped byte stream (the track's sectors),
// offset by bytes, hence the casts.
void updatePotentialContactPointers(
    const uint8_t* oldStart,
    int32_t size,
    hkpProcessCollisionOutput::PotentialInfo* info,
    uint8_t* newStart
) {
    if (info) {
        for (hkpProcessCollisionOutput::ContactRef* ref = info->m_potentialContacts;
             ref < info->m_firstFreePotentialContact; ++ref) {
            ptrdiff_t offset = reinterpret_cast<uint8_t*>(ref->m_agentEntry) - oldStart;
            if (offset >= 0 && offset < size) {
                ref->m_agentEntry = reinterpret_cast<hkpAgentEntry*>(newStart + offset);
                ref->m_agentData = static_cast<uint8_t*>(ref->m_agentData) + (newStart - oldStart);
            }
        }
    }
}

// Classifies a new contact point against a reference one: 0 when it is rejected, 2 when it lies close by and almost
// opposite, 1 otherwise. dots holds, per lane, the reference normal and the point's normal against the offset between
// the two positions, then the two normals against each other.
int32_t contactPointIsValid(
    const hkpProcessCdPoint* point,
    float a2,
    const float* a3,
    float a4,
    const hkContactPoint* reference
) {
    __m128 offset = _mm_sub_ps(point->m_contact.m_position.m_quad, reference->m_position.m_quad);
    __m128 refDotOffset = _mm_mul_ps(reference->m_separatingNormal.m_quad, offset);
    __m128 pointDotOffset = _mm_mul_ps(point->m_contact.m_separatingNormal.m_quad, offset);
    __m128 hi = _mm_unpackhi_ps(refDotOffset, pointDotOffset);
    __m128 lo = _mm_unpacklo_ps(refDotOffset, pointDotOffset);
    __m128 normals = _mm_mul_ps(point->m_contact.m_separatingNormal.m_quad, reference->m_separatingNormal.m_quad);
    __m128 lohi = _mm_movelh_ps(lo, normals);
    __m128 dots = _mm_add_ps(
        _mm_shuffle_ps(hi, normals, 0xE4), _mm_add_ps(_mm_shuffle_ps(_mm_movehl_ps(lohi, lo), normals, 0xD4), lohi)
    );
    double cosNormals = XFNV_M128_F32(dots, 2);
    float v16 = (cosNormals - 1.0) * a2 - XFNV_M128_F32(dots, 1);
    float v19 = (cosNormals - 1.0) * a2 + XFNV_M128_F32(dots, 0);
    float v18 = a2 + a4;
    if (v18 < v16) {
        return 0;
    }
    if (cosNormals > 0.9998999834060669 && point->m_contactPointId.m_storage == HK_INVALID_CONTACT_POINT) {
        return 0;
    }
    uint32_t id = point->m_contactPointId.m_storage;
    if ((id == HK_INVALID_CONTACT_POINT && cosNormals < 0.800000011920929) && (v16 + v19 < 0.10000000149011612)) {
        // Negates v18 and adds it to itself, as the game does.
        float v20 = -v18 + -v18;
        if (v20 > reference->m_separatingNormal(3) + point->m_contact.m_separatingNormal(3)) {
            return 2;
        }
    }

    float v17 = 0.0f;
    if (id != HK_INVALID_CONTACT_POINT) {
        v17 = *a3 * 0.05000000074505806;
    }
    return !(point->m_contact.m_separatingNormal(3) - v17 >= reference->m_separatingNormal(3));
}
