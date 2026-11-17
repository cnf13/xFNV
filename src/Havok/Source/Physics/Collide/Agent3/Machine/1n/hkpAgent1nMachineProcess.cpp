#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3Input.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <xmmintrin.h>

// Writes into out the input seen from body B's side: the bodies swapped, the TIM info negated, aTb inverted. in is read
// for m_linearTimInfo, so it is a process input.
static void hkAgent1nMachine_flipInput(const hkpAgent3ProcessInput* in, hkpAgent3ProcessInput* out);
// Sets temp's bodies A and B to in's at the step's start time (each the child of in's body, placed by a transform
// lerped from its motion state) and writes into out the input over them.
static void initSeparatingInput(hkpAgent3Input* out, hkAgentNmMachineBodyTemp* temp, const hkpAgent3Input* in);

// The flipped input: bodies swapped, the TIM info's x, y and z negated (w is kept) and aTb inverted.
static void hkAgent1nMachine_flipInput(const hkpAgent3ProcessInput* in, hkpAgent3ProcessInput* out) {
	out->m_bodyA = in->m_bodyB;
	out->m_bodyB = in->m_bodyA;
	out->m_input = in->m_input;
	out->m_contactMgr = in->m_contactMgr;
	out->m_linearTimInfo.setNeg3(in->m_linearTimInfo);
	out->m_aTb.setInverse(in->m_aTb);
}

static void initSeparatingInput(hkpAgent3Input* out, hkAgentNmMachineBodyTemp* temp, const hkpAgent3Input* in) {
	float fStartTimeA = in->m_input->m_stepInfo.m_startTime;
	hkSweptTransformUtil::lerp2(in->m_bodyA->m_motion.pState->m_sweptTransform, fStartTimeA, temp->m_transA);
	float fStartTimeB = in->m_input->m_stepInfo.m_startTime;
	hkSweptTransformUtil::lerp2(in->m_bodyB->m_motion.pState->m_sweptTransform, fStartTimeB, temp->m_transB);
	out->m_bodyA = &temp->m_bodyA;
	out->m_bodyB = &temp->m_bodyB;
	out->m_contactMgr = in->m_contactMgr;
	out->m_input = in->m_input;
	const hkpCdBody* pBodyA = in->m_bodyA;
	temp->m_bodyA.m_shape = pBodyA->m_shape;
	temp->m_bodyA.m_shapeKey = pBodyA->m_shapeKey;
	temp->m_bodyA.m_parent = in->m_bodyA;
	temp->m_bodyA.m_motion.pMotion = &temp->m_transA;
	// Body B's address is tested for null (a placement new) before setting its parent and transform; its shape and
	// shape key are left as they are.
	hkpCdBody* pBodyB = &temp->m_bodyB;
	if (pBodyB) {
		pBodyB->m_parent = in->m_bodyB;
		pBodyB->m_motion.pMotion = &temp->m_transB;
	}
	out->m_aTb.setMulInverseMul(temp->m_transA, temp->m_transB);
}

// The flipped process inputs are filled by hkAgent1nMachine_flipInput, which never writes their padding nor, for a
// plain flipped call and an agent's creation, m_distAtT1; the separating-normal inputs keep their padding unwritten.
// They go to the agent's create, process and sepNormal functions, and an agent3 process such as hkBvTreeAgent3::process
// hands its input on to this function, which copies it whole, reading the uninitialised parts as the game does. The
// track's entries are read sector by sector and written anew into fresh sectors (the input's spare sector first),
// merged with the sorted hit list of shape keys: an entry whose key is still hit is kept, one whose key is gone has its
// agent destroyed, and a new key gets a new agent unless the filter rejects it. Each written entry then runs as its
// stream command says. Once a write sector has less than 128 bytes left, entries go to a stack buffer first, so that no
// agent writes past its sector; the buffer then moves into that sector, or into the next one, and the potential
// contacts pointing into it move along. Sectors and the buffer are untyped bytes (a stream of agent entries, each
// followed by its agent's data): entries and the potential contacts' entry pointers are reinterpreted from and to those
// bytes.
void hkAgent1nMachine_Process(hkpAgent1nTrack& agentTrack, const hkpAgent3ProcessInput& inputIn,
	const hkpShapeContainer* shapeContainerB, const uint32_t* newKeys, hkpProcessCollisionOutput& output) {
	hkpProcessCollisionOutput::PotentialInfo potentialInfo;
	hkpAgent3ProcessInput in3;
	hkpAgent3ProcessInput flippedIn3;
	hkpAgent3ProcessInput flippedTimIn3;
	hkpAgent3ProcessInput flippedCreateIn3;
	hkpAgent3Input sepInput;
	hkpAgent3Input flippedSepInput;
	hkAgentNmMachineBodyTemp bodyTemp;
	hkAgentNmMachineBodyTemp flippedBodyTemp;
	hkpCdBody newCdBodies[4];
	hkMotionState newMotionStates[4];
	alignas(16) char shapeBuffer[512];
	alignas(16) uint8_t buffer[128];
	hkpCdBody childBody;

	_mm_prefetch(reinterpret_cast<const char*>(agentTrack.m_sectors.m_data), _MM_HINT_NTA);
	hkpProcessCollisionOutput::PotentialInfo* savedPotentialInfo = output.m_potentialContacts.m_storage;
	if (!savedPotentialInfo && shapeContainerB->isWeldingEnabled()) {
		potentialInfo.m_firstFreePotentialContact = &potentialInfo.m_potentialContacts[0];
		potentialInfo.m_firstFreeRepresentativeContact = &potentialInfo.m_representativeContacts[0];
		output.m_potentialContacts.m_storage = &potentialInfo;
	} else {
		output.m_potentialContacts.m_storage = nullptr;
	}

	in3 = inputIn;
	hkpCollisionDispatcher* dispatcher = in3.m_input->m_dispatcher;
	// The child of body B the current entry is for: the shape and key change per entry.
	childBody.m_parent = inputIn.m_bodyB;
	childBody.m_motion = inputIn.m_bodyB->m_motion;
	in3.m_bodyB = &childBody;

	hkpAgent1nSector* readSector = agentTrack.m_sectors.m_data[0];
	_mm_prefetch(reinterpret_cast<const char*>(readSector), _MM_HINT_NTA);
	_mm_prefetch(reinterpret_cast<const char*>(readSector) + 64, _MM_HINT_NTA);
	_mm_prefetch(reinterpret_cast<const char*>(readSector) + 128, _MM_HINT_NTA);
	_mm_prefetch(reinterpret_cast<const char*>(readSector) + 192, _MM_HINT_NTA);
	int32_t numTim = 0;
	hkpAgent1nSector* writeSector = in3.m_input->m_spareAgentSector;
	if (!writeSector) {
		// EXE-INLINED none: hkpAgent1nSector::operator new, hkpAgent1nSector::hkpAgent1nSector
		writeSector =
			static_cast<hkpAgent1nSector*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpAgent1nSector)));
		if (writeSector) {
			writeSector->m_bytesAllocated = 0;
		}
	}
	in3.m_input->m_spareAgentSector = nullptr;
	sepInput.m_overrideBodyA = nullptr;
	flippedSepInput.m_overrideBodyA = nullptr;

	uint8_t* readPtr = readSector->m_data;
	uint8_t* readEnd = readSector->m_data + readSector->m_bytesAllocated;
	int32_t nextReadSectorIndex = 1;
	uint8_t* writePtr = writeSector->m_data;
	uint8_t* writeEnd = writeSector->m_data + hkpAgent1nSector::NET_SECTOR_SIZE;
	int32_t outSectorIndex = 0;
	int32_t bytesInSector = 0;
	int32_t bytesInBuffer = 0;
	bool inBuffer = false;
	// Whether the separating-normal inputs (and their bodies at the step's start) still hold for the next entry: not
	// after a transform shape.
	bool sepInputValid = false;
	bool flippedSepInputValid = false;

	while (true) {
		in3.m_bodyB = &childBody;
		in3.m_aTb = inputIn.m_aTb;
		hkpAgent1nMachineEntry* entry = reinterpret_cast<hkpAgent1nMachineEntry*>(writePtr);
		uint32_t key;
		if (!newKeys) {
			const hkpAgent1nMachineEntry* readEntry = reinterpret_cast<const hkpAgent1nMachineEntry*>(readPtr);
			const int32_t entrySize = readEntry->m_size;
			hkString::memCpy16NonEmpty(writePtr, readPtr, entrySize >> 4);
			key = readEntry->m_shapeKeyPair.m_shapeKeyB;
			readPtr += entrySize;
		} else {
			hkpAgent1nMachineEntry* readEntry = reinterpret_cast<hkpAgent1nMachineEntry*>(readPtr);
			key = *newKeys;
			if (key == readEntry->m_shapeKeyPair.m_shapeKeyB) {
				const int32_t entrySize = readEntry->m_size;
				hkString::memCpy16NonEmpty(writePtr, readPtr, entrySize >> 4);
				readPtr += entrySize;
			} else if (key > readEntry->m_shapeKeyPair.m_shapeKeyB) {
				// The entry's key is no longer hit: its agent goes, and the same hit is compared with the next entry.
				// A TIM command (flipped, with a transform or not) has its agent's data after the TIM entry.
				hkpAgentData* agentData = static_cast<hkpAgent1nMachinePaddedEntry*>(readEntry) + 1;
				if ((readEntry->m_streamCommand & 6) == hkAgent3::STREAM_CALL_WITH_TIM) {
					agentData = static_cast<hkpAgent1nMachineTimEntry*>(readEntry) + 1;
				}
				readPtr += readEntry->m_size;
				dispatcher->m_agent3Func[readEntry->m_agentType].m_destroyFunc(readEntry, agentData, in3.m_contactMgr,
					*output.m_constraintOwner.m_storage, dispatcher);
				goto entryDone;
			} else {
				// A new key: a new entry, an empty one when the filter rejects the pair.
				const hkpShapeCollectionFilter* filter = in3.m_input->m_filter;
				if (!filter->isCollisionEnabled(*in3.m_input, *inputIn.m_bodyA, *inputIn.m_bodyB, *shapeContainerB, key)
						.m_bool) {
					entry->m_streamCommand = hkAgent3::STREAM_NULL;
					entry->m_agentType = 0;
					entry->m_numContactPoints = 0;
					entry->m_size = 0;
					entry->m_shapeKeyPair.m_shapeKeyA = HK_INVALID_SHAPE_KEY;
					entry->m_shapeKeyPair.m_shapeKeyB = key;
					entry->m_size = sizeof(hkpAgent1nMachinePaddedEntry);
				} else {
					if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
						newKeys++;
						goto entryDone;
					}
					childBody.m_shape = shapeContainerB->getChildShape(key, shapeBuffer);
					childBody.m_shapeKey = key;
					const hkpShape* shapeA = in3.m_bodyA->m_shape;
					uint8_t cdBodyHasTransformFlag = 0;
					in3.m_bodyB = hkAgentMachine_processTransformedShapes(in3.m_bodyB, newCdBodies, newMotionStates, 4,
						cdBodyHasTransformFlag);
					const hkpShape* shapeB = in3.m_bodyB->m_shape;
					in3.m_aTb.setMulInverseMul(*in3.m_bodyA->m_motion.pMotion, *in3.m_bodyB->m_motion.pMotion);
					entry->m_shapeKeyPair.m_shapeKeyA = HK_INVALID_SHAPE_KEY;
					entry->m_shapeKeyPair.m_shapeKeyB = key;
					const uint8_t agentType = in3.m_input->m_createPredictiveAgents.m_storage
						? dispatcher->m_agent3TypesPred[shapeA->m_type.m_storage][shapeB->m_type.m_storage]
						: dispatcher->m_agent3Types[shapeA->m_type.m_storage][shapeB->m_type.m_storage];
					entry->m_agentType = agentType;
					entry->m_numContactPoints = 0;
					flippedCreateIn3.m_overrideBodyA = nullptr;
					const hkpAgent3ProcessInput* createInput = &in3;
					uint8_t flipped = 0;
					if (dispatcher->m_agent3Func[agentType].m_symmetric == hkAgent3::IS_NOT_SYMMETRIC_AND_FLIPPED) {
						createInput = &flippedCreateIn3;
						flipped = 1;
						hkAgent1nMachine_flipInput(&in3, &flippedCreateIn3);
					}
					const hkpCollisionDispatcher::Agent3FuncsIntern& funcs =
						dispatcher->m_agent3Func[entry->m_agentType];
					hkpAgentData* end;
					if (funcs.m_sepNormalFunc) {
						hkpAgent1nMachineTimEntry* timEntry = static_cast<hkpAgent1nMachineTimEntry*>(entry);
						timEntry->m_timeOfSeparatingNormal = -1.0f;
						entry->m_streamCommand =
							static_cast<uint8_t>((hkAgent3::STREAM_CALL_WITH_TIM + flipped) | cdBodyHasTransformFlag);
						timEntry->m_separatingNormal.setZero4();
						end = funcs.m_createFunc(*createInput, entry, timEntry + 1);
					} else {
						entry->m_streamCommand =
							static_cast<uint8_t>((hkAgent3::STREAM_CALL + flipped) | cdBodyHasTransformFlag);
						end = funcs.m_createFunc(*createInput, entry,
							static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 1);
					}
					entry->m_size = static_cast<uint8_t>(static_cast<uint8_t*>(end) - writePtr);
				}
			}
			newKeys++;
		}

		{
			uint8_t command = entry->m_streamCommand;
			bool transformed = false;
			while (true) {
				switch (command) {
				case hkAgent3::STREAM_NULL:
				case hkAgent3::STREAM_END:
					writePtr += sizeof(hkpAgent1nMachinePaddedEntry);
					break;
				case hkAgent3::STREAM_CALL: {
					childBody.m_shape = shapeContainerB->getChildShape(key, shapeBuffer);
					childBody.m_shapeKey = key;
					hkpAgentData* end = dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(in3, entry,
						static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 1, nullptr, output);
					entry->m_size = static_cast<uint8_t>(static_cast<uint8_t*>(end) - writePtr);
					writePtr = static_cast<uint8_t*>(end);
					break;
				}
				case hkAgent3::STREAM_CALL_FLIPPED: {
					flippedIn3.m_overrideBodyA = nullptr;
					hkAgent1nMachine_flipInput(&in3, &flippedIn3);
					childBody.m_shape = shapeContainerB->getChildShape(key, shapeBuffer);
					childBody.m_shapeKey = key;
					const float fTimeOfImpact = output.m_toi.m_time.m_storage;
					hkpProcessCdPoint* firstNewPoint = output.m_firstFreeContactPoint.m_storage;
					hkpAgentData* end = dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(flippedIn3, entry,
						static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 1, nullptr, output);
					entry->m_size = static_cast<uint8_t>(static_cast<uint8_t*>(end) - writePtr);
					writePtr = static_cast<uint8_t*>(end);
					// The new points are B's against A: moved by their distance along the normal, which turns round.
					for (hkpProcessCdPoint* point = firstNewPoint; point < output.m_firstFreeContactPoint.m_storage;
						++point) {
						const hkQuadReal normal = point->m_contact.m_separatingNormal.m_quad;
						point->m_contact.m_position.m_quad =
							_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normal, normal, _MM_SHUFFLE(3, 3, 3, 3)), normal),
								point->m_contact.m_position.m_quad);
						point->m_contact.m_separatingNormal.m_quad = _mm_xor_ps(normal, rdata_10C4C00);
					}
					// A time of impact the agent found is turned round the same way.
					if (output.m_toi.m_time.m_storage != fTimeOfImpact) {
						output.m_toi.m_contactPoint.m_separatingNormal.m_quad =
							_mm_xor_ps(rdata_10C4C00, output.m_toi.m_contactPoint.m_separatingNormal.m_quad);
						output.m_toi.m_gskCache.flip();
					}
					break;
				}
				case hkAgent3::STREAM_CALL_WITH_TIM: {
					hkpAgent1nMachineTimEntry* timEntry = static_cast<hkpAgent1nMachineTimEntry*>(entry);
					float distAtT1;
					if (in3.m_input->m_stepInfo.m_startTime != timEntry->m_timeOfSeparatingNormal) {
						if (!(in3.m_input->m_stepInfo.m_endTime <= timEntry->m_timeOfSeparatingNormal)) {
							const hkpCollisionQualityInfo* quality = in3.m_input->m_collisionQualityInfo;
							if (!quality->m_useContinuousPhysics.m_storage) {
								// No continuous physics: no separating normal, and the agent runs.
								timEntry->m_timeOfSeparatingNormal = in3.m_input->m_stepInfo.m_endTime;
								timEntry->m_separatingNormal.setZero4();
								distAtT1 = static_cast<float>(quality->m_minSeparation * 0.10000000149011612);
								timEntry->m_separatingNormal.m_quad[3] = distAtT1;
								goto processTim;
							}
							// The separating normal at the step's start: both bodies at their start transforms.
							if (!sepInputValid) {
								sepInputValid = !transformed;
								initSeparatingInput(&sepInput, &bodyTemp, &in3);
							}
							childBody.m_shape = shapeContainerB->getChildShape(key, shapeBuffer);
							childBody.m_shapeKey = key;
							bodyTemp.m_bodyB.m_shape = in3.m_bodyB->m_shape;
							bodyTemp.m_bodyB.m_shapeKey = in3.m_bodyB->m_shapeKey;
							dispatcher->m_agent3Func[entry->m_agentType].m_sepNormalFunc(sepInput, entry, timEntry + 1,
								timEntry->m_separatingNormal);
						}
						// Otherwise the normal is from the step's end or later. Its distance is extrapolated over that
						// time, but the result is never used.
					}
					timEntry->m_timeOfSeparatingNormal = in3.m_input->m_stepInfo.m_endTime;
					{
						const float fProjected = in3.m_linearTimInfo.dot4xyz1(timEntry->m_separatingNormal);
						distAtT1 = timEntry->m_separatingNormal.m_quad[3] - fProjected;
					}
					if (in3.m_input->m_tolerance <= distAtT1) {
						// Still separated at the step's end: the agent does not run.
						timEntry->m_separatingNormal.m_quad[3] = distAtT1;
						if (entry->m_numContactPoints) {
							hkpAgentData* end = dispatcher->m_agent3Func[entry->m_agentType].m_cleanupFunc(entry,
								timEntry + 1, in3.m_contactMgr, *output.m_constraintOwner.m_storage);
							entry->m_size = static_cast<uint8_t>(static_cast<uint8_t*>(end) - writePtr);
							writePtr = static_cast<uint8_t*>(end);
						} else {
							writePtr += entry->m_size;
						}
						numTim++;
						break;
					}
				processTim:
					in3.m_distAtT1.m_storage = distAtT1;
					childBody.m_shape = shapeContainerB->getChildShape(key, shapeBuffer);
					childBody.m_shapeKey = key;
					{
						hkpAgentData* end = dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(in3, entry,
							timEntry + 1, &timEntry->m_separatingNormal, output);
						entry->m_size = static_cast<uint8_t>(static_cast<uint8_t*>(end) - writePtr);
						writePtr = static_cast<uint8_t*>(end);
					}
					break;
				}
				case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED: {
					flippedTimIn3.m_overrideBodyA = nullptr;
					hkAgent1nMachine_flipInput(&in3, &flippedTimIn3);
					hkpAgent1nMachineTimEntry* timEntry = static_cast<hkpAgent1nMachineTimEntry*>(entry);
					float distAtT1;
					if (flippedTimIn3.m_input->m_stepInfo.m_startTime != timEntry->m_timeOfSeparatingNormal) {
						const hkpCollisionQualityInfo* quality = flippedTimIn3.m_input->m_collisionQualityInfo;
						if (!quality->m_useContinuousPhysics.m_storage) {
							// No continuous physics: no separating normal, and the agent runs.
							timEntry->m_timeOfSeparatingNormal = flippedTimIn3.m_input->m_stepInfo.m_endTime;
							timEntry->m_separatingNormal.setZero4();
							distAtT1 = static_cast<float>(quality->m_minSeparation * 0.5);
							timEntry->m_separatingNormal.m_quad[3] = distAtT1;
							goto processFlippedTim;
						}
						// The separating normal at the step's start: both bodies at their start transforms. The child
						// of body B is body A here.
						if (!flippedSepInputValid) {
							flippedSepInputValid = !transformed;
							initSeparatingInput(&flippedSepInput, &flippedBodyTemp, &flippedTimIn3);
							flippedBodyTemp.m_bodyB.m_shape = flippedTimIn3.m_bodyB->m_shape;
							flippedBodyTemp.m_bodyB.m_shapeKey = flippedTimIn3.m_bodyB->m_shapeKey;
						}
						childBody.m_shape = shapeContainerB->getChildShape(key, shapeBuffer);
						childBody.m_shapeKey = key;
						flippedBodyTemp.m_bodyA.m_shape = in3.m_bodyB->m_shape;
						flippedBodyTemp.m_bodyA.m_shapeKey = in3.m_bodyB->m_shapeKey;
						dispatcher->m_agent3Func[entry->m_agentType].m_sepNormalFunc(flippedSepInput, entry,
							timEntry + 1, timEntry->m_separatingNormal);
					}
					timEntry->m_timeOfSeparatingNormal = flippedTimIn3.m_input->m_stepInfo.m_endTime;
					{
						const float fProjected = flippedTimIn3.m_linearTimInfo.dot4xyz1(timEntry->m_separatingNormal);
						distAtT1 = timEntry->m_separatingNormal.m_quad[3] - fProjected;
					}
					if (flippedTimIn3.m_input->m_tolerance <= distAtT1) {
						// Still separated at the step's end: the agent does not run.
						timEntry->m_separatingNormal.m_quad[3] = distAtT1;
						if (entry->m_numContactPoints) {
							hkpAgentData* end = dispatcher->m_agent3Func[entry->m_agentType].m_cleanupFunc(entry,
								timEntry + 1, flippedTimIn3.m_contactMgr, *output.m_constraintOwner.m_storage);
							entry->m_size = static_cast<uint8_t>(static_cast<uint8_t*>(end) - writePtr);
							writePtr = static_cast<uint8_t*>(end);
						} else {
							writePtr += entry->m_size;
						}
						numTim++;
						break;
					}
				processFlippedTim:
					flippedTimIn3.m_distAtT1.m_storage = distAtT1;
					childBody.m_shape = shapeContainerB->getChildShape(key, shapeBuffer);
					childBody.m_shapeKey = key;
					{
						const float fTimeOfImpact = output.m_toi.m_time.m_storage;
						hkpProcessCdPoint* firstNewPoint = output.m_firstFreeContactPoint.m_storage;
						hkpAgentData* end = dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(flippedTimIn3,
							entry, timEntry + 1, &timEntry->m_separatingNormal, output);
						entry->m_size = static_cast<uint8_t>(static_cast<uint8_t*>(end) - writePtr);
						writePtr = static_cast<uint8_t*>(end);
						// The new points are B's against A: moved by their distance along the normal, which turns
						// round.
						for (hkpProcessCdPoint* point = firstNewPoint; point < output.m_firstFreeContactPoint.m_storage;
							++point) {
							const hkQuadReal normal = point->m_contact.m_separatingNormal.m_quad;
							point->m_contact.m_position.m_quad =
								_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normal, normal, _MM_SHUFFLE(3, 3, 3, 3)), normal),
									point->m_contact.m_position.m_quad);
							point->m_contact.m_separatingNormal.m_quad = _mm_xor_ps(normal, rdata_10C4C00);
						}
						// A time of impact the agent found is turned round the same way.
						if (output.m_toi.m_time.m_storage != fTimeOfImpact) {
							output.m_toi.m_contactPoint.m_separatingNormal.m_quad =
								_mm_xor_ps(rdata_10C4C00, output.m_toi.m_contactPoint.m_separatingNormal.m_quad);
							output.m_toi.m_gskCache.flip();
						}
					}
					break;
				}
				case hkAgent3::STREAM_CALL_AGENT: {
					childBody.m_shape = shapeContainerB->getChildShape(key, shapeBuffer);
					childBody.m_shapeKey = key;
					hkpAgentData* agentData = static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 1;
					hkpCollisionAgent* agent = *static_cast<hkpCollisionAgent**>(agentData);
					agent->processCollision(*in3.m_bodyA, *in3.m_bodyB, *in3.m_input, output);
					// The entry and the 16 bytes holding the agent pointer.
					writePtr += 2 * sizeof(hkpAgent1nMachinePaddedEntry);
					break;
				}
				case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
				case hkAgent3::STREAM_CALL_FLIPPED_WITH_TRANSFORM:
				case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
				case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED_WITH_TRANSFORM:
				case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM: {
					// Body B becomes the first level under the child that is not a transform shape; the command then
					// runs again without the flag.
					childBody.m_shape = shapeContainerB->getChildShape(key, shapeBuffer);
					childBody.m_shapeKey = key;
					uint8_t cdBodyHasTransformFlag = 0;
					in3.m_bodyB = hkAgentMachine_processTransformedShapes(in3.m_bodyB, newCdBodies, newMotionStates, 4,
						cdBodyHasTransformFlag);
					in3.m_aTb.setMulInverseMul(*in3.m_bodyA->m_motion.pMotion, *in3.m_bodyB->m_motion.pMotion);
					command = static_cast<uint8_t>(command & ~hkAgent3::TRANSFORM_FLAG);
					transformed = true;
					flippedSepInputValid = false;
					sepInputValid = false;
					continue;
				}
				default:
					break;
				}
				break;
			}
		}

	entryDone:
		if (readPtr == readEnd) {
			// The read sector is used up: it becomes the input's spare sector, or is freed when there is one.
			if (in3.m_input->m_spareAgentSector) {
				// EXE-INLINED none: hkpAgent1nSector::operator delete
				hkMemoryRouter::getInstance().heap().blockFree(readSector, sizeof(hkpAgent1nSector));
			} else {
				in3.m_input->m_spareAgentSector = readSector;
			}
			if (nextReadSectorIndex >= agentTrack.m_sectors.m_size) {
				break;
			}
			readSector = agentTrack.m_sectors.m_data[nextReadSectorIndex];
			readPtr = readSector->m_data;
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
			// The buffer still fits into the write sector.
			uint8_t* dst = writeSector->m_data + bytesInSector;
			hkString::memCpy16NonEmpty(dst, buffer, bytesInBuffer >> 4);
			bytesInSector += bytesInBuffer;
			hkpProcessCollisionOutput::PotentialInfo* info = output.m_potentialContacts.m_storage;
			if (info) {
				for (hkpProcessCollisionOutput::ContactRef* ref = &info->m_potentialContacts[0];
					ref < info->m_firstFreePotentialContact; ++ref) {
					const ptrdiff_t offset = reinterpret_cast<uint8_t*>(ref->m_agentEntry) - buffer;
					if (offset >= 0 && offset < bytesInBuffer) {
						ref->m_agentEntry = reinterpret_cast<hkpAgentEntry*>(dst + offset);
						ref->m_agentData = static_cast<uint8_t*>(ref->m_agentData) + (dst - buffer);
					}
				}
			}
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
		writeSector = in3.m_input->m_spareAgentSector;
		if (!writeSector) {
			// EXE-INLINED none: hkpAgent1nSector::operator new, hkpAgent1nSector::hkpAgent1nSector
			writeSector = static_cast<hkpAgent1nSector*>(hkMemoryRouter::getInstance()
					.heap()
					.blockAlloc(sizeof(hkpAgent1nSector)));
			if (writeSector) {
				writeSector->m_bytesAllocated = 0;
			}
		}
		writePtr = writeSector->m_data;
		writeEnd = writeSector->m_data + hkpAgent1nSector::NET_SECTOR_SIZE;
		in3.m_input->m_spareAgentSector = nullptr;
		if (inBuffer) {
			// The buffer did not fit: it starts the new sector.
			hkString::memCpy16NonEmpty(writePtr, buffer, bytesInBuffer >> 4);
			hkpProcessCollisionOutput::PotentialInfo* info = output.m_potentialContacts.m_storage;
			if (info) {
				for (hkpProcessCollisionOutput::ContactRef* ref = &info->m_potentialContacts[0];
					ref < info->m_firstFreePotentialContact; ++ref) {
					const ptrdiff_t offset = reinterpret_cast<uint8_t*>(ref->m_agentEntry) - buffer;
					if (offset >= 0 && offset < bytesInBuffer) {
						ref->m_agentEntry = reinterpret_cast<hkpAgentEntry*>(writePtr + offset);
						ref->m_agentData = static_cast<uint8_t*>(ref->m_agentData) + (writePtr - buffer);
					}
				}
			}
			writePtr += bytesInBuffer;
			inBuffer = false;
		}
	}

	if (!inBuffer) {
		bytesInSector = static_cast<int32_t>(writePtr - writeSector->m_data);
	} else {
		// The last buffer goes into the last sector. Only the last potential contact is looked at here.
		const int32_t bytesInLastBuffer = static_cast<int32_t>(writePtr - buffer);
		uint8_t* dst = writeSector->m_data + bytesInSector;
		hkString::memCpy16NonEmpty(dst, buffer, bytesInLastBuffer >> 4);
		hkpProcessCollisionOutput::PotentialInfo* info = output.m_potentialContacts.m_storage;
		if (info && info->m_firstFreePotentialContact > &info->m_potentialContacts[0]) {
			hkpProcessCollisionOutput::ContactRef* ref = info->m_firstFreePotentialContact - 1;
			const ptrdiff_t offset = reinterpret_cast<uint8_t*>(ref->m_agentEntry) - buffer;
			if (offset >= 0 && offset < bytesInLastBuffer) {
				ref->m_agentEntry = reinterpret_cast<hkpAgentEntry*>(dst + offset);
				ref->m_agentData = static_cast<uint8_t*>(ref->m_agentData) + (dst - buffer);
			}
		}
		bytesInSector += bytesInLastBuffer;
	}
	writeSector->m_bytesAllocated = bytesInSector;
	agentTrack.m_sectors.setSize(outSectorIndex + 1);
	agentTrack.m_sectors.m_data[outSectorIndex] = writeSector;

	HK_MONITOR_ADD_VALUE("numTim", static_cast<float>(numTim), HK_MONITOR_TYPE_INT);

	// The potential contacts of the children are welded against each other, with the input's own body B.
	in3.m_aTb = inputIn.m_aTb;
	in3.m_bodyB = inputIn.m_bodyB;
	hkpProcessCollisionOutput::PotentialInfo* info = output.m_potentialContacts.m_storage;
	if (info && info->m_firstFreePotentialContact > &info->m_potentialContacts[0]) {
		hkAgent1nMachine_Weld(in3, shapeContainerB, output);
	}
	output.m_potentialContacts.m_storage = savedPotentialInfo;
}

// The agents get inputs with bytes nothing wrote. The flipped inputs (hkAgent1nMachine_flipInput) leave the alignment
// bytes unwritten and, for a plain flipped call or a created agent, m_distAtT1; a flipped agent's process reached from
// here copies the whole struct to its own frame, as this body copies its own input. The track's entries are rewritten
// into fresh sectors as the sorted hit list of key pairs is walked against them: a pair both have keeps its entry, a
// pair only the track has loses its agent, a pair only the hit list has gets an agent (or a null entry when the filter
// rejects it). Every kept or new entry is then processed as hkAgentNnMachine_ProcessAgent does, with the child shapes
// of the pair's keys (cached across entries) as the bodies; a flipped agent runs on the flipped input and its new
// contact points and time of impact are flipped back. An entry written past the room left in a sector goes to the
// extension buffer first and then into this sector or the next. A sector's data is an untyped byte stream of entries
// and their agents' data, stepped through by the entries' byte sizes, hence the casts between bytes and entries.
void hkAgentNmMachine_Process(hkpAgent1nTrack& agentTrack, const hkpAgent3ProcessInput& inputIn,
	const hkpShapeContainer* shapeContainerA, const hkpShapeContainer* shapeContainerB, const hkShapeKeyPair* newKeys,
	hkpProcessCollisionOutput& output) {
	alignas(16) char shapeBufferA[HK_SHAPE_BUFFER_SIZE];
	alignas(16) char shapeBufferB[HK_SHAPE_BUFFER_SIZE];
	alignas(16) hkpCdBody newCdBodiesA[4];
	alignas(16) hkpCdBody newCdBodiesB[4];
	hkMotionState newMotionStatesA[4];
	hkMotionState newMotionStatesB[4];
	alignas(16) uint8_t extensionBuffer[128];
	alignas(16) hkAgentNmMachineBodyTemp bodyTemp;
	alignas(16) hkAgentNmMachineBodyTemp bodyTempFlipped;
	alignas(16) hkpAgent3Input sepInput;
	alignas(16) hkpAgent3Input sepInputFlipped;
	alignas(16) hkpAgent3ProcessInput flippedTimInput;
	alignas(16) hkpAgent3ProcessInput flippedCallInput;
	alignas(16) hkpAgent3ProcessInput flippedCreateInput;
	hkpCdBody bodyA;
	hkpCdBody bodyB;

	hkpAgent1nSector** sectors = agentTrack.m_sectors.m_data;
	_mm_prefetch(reinterpret_cast<const char*>(sectors), _MM_HINT_NTA);
	alignas(16) hkpAgent3ProcessInput in3 = inputIn;
	uint32_t lastKeyA = HK_INVALID_SHAPE_KEY;
	uint32_t lastKeyB = HK_INVALID_SHAPE_KEY;
	hkpCollisionDispatcher* dispatcher = in3.m_input->m_dispatcher;
	bodyB.m_parent = inputIn.m_bodyB;
	bodyB.m_motion = inputIn.m_bodyB->m_motion;
	in3.m_bodyB = &bodyB;
	bodyA.m_parent = inputIn.m_bodyA;
	bodyA.m_motion = inputIn.m_bodyA->m_motion;
	in3.m_bodyA = &bodyA;
	hkpAgent1nSector* readSector = sectors[0];
	hkpAgent1nSector* currentSector = in3.m_input->m_spareAgentSector;
	// The SDK's forcePrefetch<256>: one prefetch per 64-byte cache line.
	for (int32_t iLine = 0; iLine < 256; iLine += 64) {
		_mm_prefetch(reinterpret_cast<const char*>(readSector) + iLine, _MM_HINT_NTA);
	}
	int32_t numTim = 0;
	if (!currentSector) {
		// EXE-INLINED none: hkpAgent1nSector::operator new, hkpAgent1nSector::hkpAgent1nSector
		currentSector =
			static_cast<hkpAgent1nSector*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpAgent1nSector)));
		if (currentSector) {
			currentSector->m_bytesAllocated = 0;
		}
	}
	in3.m_input->m_spareAgentSector = nullptr;
	sepInputFlipped.m_overrideBodyA = nullptr;
	sepInput.m_overrideBodyA = nullptr;
	uint8_t* readData = readSector->m_data;
	uint8_t* currentData = currentSector->m_data;
	uint8_t* readEnd = readSector->m_data + readSector->m_bytesAllocated;
	uint8_t* currentEnd = reinterpret_cast<uint8_t*>(currentSector + 1);
	int32_t currentSectorIndex = 0;
	bool sepInputInitialized = false;
	bool sepInputFlippedInitialized = false;
	int32_t nextReadSectorIndex = 1;
	int32_t bytesInCurrentSector = 0;
	bool inExtensionBuffer = false;
	int32_t extensionSize;

	uint32_t keyA;
	uint32_t keyB;
	// The child shapes of the current keys, fetched again only when a key changes.
	auto extractShapes = [&]() __attribute__((always_inline)) {
		if (lastKeyB != keyB) {
			lastKeyB = keyB;
			bodyB.m_shape = shapeContainerB->getChildShape(keyB, shapeBufferB);
			bodyB.m_shapeKey = keyB;
		}
		if (lastKeyA != keyA) {
			lastKeyA = keyA;
			bodyA.m_shape = shapeContainerA->getChildShape(keyA, shapeBufferA);
			bodyA.m_shapeKey = keyA;
		}
	};

	for (;;) {
		in3.m_bodyB = &bodyB;
		in3.m_aTb = inputIn.m_aTb;
		in3.m_bodyA = &bodyA;
		hkpAgent1nMachineEntry* entry = reinterpret_cast<hkpAgent1nMachineEntry*>(currentData);
		hkpAgent1nMachineEntry* readEntry = reinterpret_cast<hkpAgent1nMachineEntry*>(readData);
		if (!newKeys) {
			// No hit list: every entry of the track is kept.
			const uint8_t size = readEntry->m_size;
			hkString::memCpy16NonEmpty(entry, readEntry, size >> 4);
			readData += size;
			keyA = readEntry->m_shapeKeyPair.m_shapeKeyA;
			keyB = readEntry->m_shapeKeyPair.m_shapeKeyB;
		} else {
			keyA = newKeys->m_shapeKeyA;
			keyB = newKeys->m_shapeKeyB;
			const uint32_t readKeyB = readEntry->m_shapeKeyPair.m_shapeKeyB;
			if (keyB == readKeyB && keyA == readEntry->m_shapeKeyPair.m_shapeKeyA) {
				const uint8_t size = readEntry->m_size;
				hkString::memCpy16NonEmpty(entry, readEntry, size >> 4);
				readData += size;
			} else if (keyB > readKeyB || (keyB == readKeyB && keyA > readEntry->m_shapeKeyPair.m_shapeKeyA)) {
				// The track's pair is no longer hit: its agent goes and the hit list stays where it is.
				hkpAgentData* agentData = ((readEntry->m_streamCommand & 6) == hkAgent3::STREAM_CALL_WITH_TIM)
					? static_cast<hkpAgentData*>(static_cast<hkpAgent1nMachineTimEntry*>(readEntry) + 1)
					: static_cast<hkpAgentData*>(static_cast<hkpAgent1nMachinePaddedEntry*>(readEntry) + 1);
				readData += readEntry->m_size;
				dispatcher->m_agent3Func[readEntry->m_agentType].m_destroyFunc(readEntry, agentData, in3.m_contactMgr,
					*output.m_constraintOwner.m_storage, dispatcher);
				goto entryDone;
			} else {
				// A new pair: an agent for it when the filter lets the children collide.
				const hkpShapeCollectionFilter* filter = in3.m_input->m_filter;
				if (filter->isCollisionEnabled(*in3.m_input, *inputIn.m_bodyA, *inputIn.m_bodyB, *shapeContainerA,
						*shapeContainerB, keyA, keyB)) {
					if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
						newKeys++;
						goto entryDone;
					}
					extractShapes();
					uint8_t cdBodyHasTransformFlag = 0;
					in3.m_bodyB = hkAgentMachine_processTransformedShapes(in3.m_bodyB, newCdBodiesB, newMotionStatesB,
						4, cdBodyHasTransformFlag);
					const hkpShape* shapeB = in3.m_bodyB->m_shape;
					in3.m_bodyA = hkAgentMachine_processTransformedShapes(in3.m_bodyA, newCdBodiesA, newMotionStatesA,
						4, cdBodyHasTransformFlag);
					const hkpShape* shapeA = in3.m_bodyA->m_shape;
					in3.m_aTb.setMulInverseMul(*in3.m_bodyA->m_motion.pMotion, *in3.m_bodyB->m_motion.pMotion);
					entry->m_shapeKeyPair.m_shapeKeyB = keyB;
					entry->m_shapeKeyPair.m_shapeKeyA = keyA;
					const uint32_t typeA = shapeA->m_type.m_storage;
					const uint32_t typeB = shapeB->m_type.m_storage;
					const uint8_t agentType = in3.m_input->m_createPredictiveAgents.m_storage
						? dispatcher->m_agent3TypesPred[typeA][typeB]
						: dispatcher->m_agent3Types[typeA][typeB];
					entry->m_agentType = agentType;
					entry->m_numContactPoints = 0;
					flippedCreateInput.m_overrideBodyA = nullptr;
					const hkpAgent3ProcessInput* createInput = &in3;
					int32_t isFlipped = 0;
					if (dispatcher->m_agent3Func[agentType].m_symmetric == hkAgent3::IS_NOT_SYMMETRIC_AND_FLIPPED) {
						createInput = &flippedCreateInput;
						isFlipped = 1;
						hkAgent1nMachine_flipInput(&in3, &flippedCreateInput);
					}
					const hkpCollisionDispatcher::Agent3FuncsIntern& funcs =
						dispatcher->m_agent3Func[entry->m_agentType];
					uint8_t* end;
					if (funcs.m_sepNormalFunc) {
						hkpAgent1nMachineTimEntry* timEntry = static_cast<hkpAgent1nMachineTimEntry*>(entry);
						timEntry->m_timeOfSeparatingNormal = -1.0f;
						entry->m_streamCommand =
							static_cast<uint8_t>((isFlipped + hkAgent3::STREAM_CALL_WITH_TIM) | cdBodyHasTransformFlag);
						timEntry->m_separatingNormal.setZero4();
						end = static_cast<uint8_t*>(funcs.m_createFunc(*createInput, entry, timEntry + 1));
					} else {
						entry->m_streamCommand =
							static_cast<uint8_t>((isFlipped + hkAgent3::STREAM_CALL) | cdBodyHasTransformFlag);
						end = static_cast<uint8_t*>(funcs.m_createFunc(*createInput, entry,
							static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 1));
					}
					entry->m_size = static_cast<uint8_t>(end - currentData);
				} else {
					// A null entry keeps the pair, so that the filter is not asked again.
					entry->m_streamCommand = hkAgent3::STREAM_NULL;
					entry->m_agentType = 0;
					entry->m_numContactPoints = 0;
					entry->m_shapeKeyPair.m_shapeKeyB = keyB;
					entry->m_shapeKeyPair.m_shapeKeyA = keyA;
					entry->m_size = sizeof(hkpAgent1nMachinePaddedEntry);
				}
			}
			newKeys++;
		}

		{
			uint32_t command = entry->m_streamCommand;
			bool bodiesTransformed = false;
			for (;;) {
				switch (command) {
				case hkAgent3::STREAM_NULL:
				case hkAgent3::STREAM_END:
					currentData += sizeof(hkpAgent1nMachinePaddedEntry);
					goto entryDone;
				case hkAgent3::STREAM_CALL: {
					extractShapes();
					currentData = static_cast<uint8_t*>(dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(in3,
						entry, static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 1, nullptr, output));
					entry->m_size = static_cast<uint8_t>(currentData - reinterpret_cast<uint8_t*>(entry));
					goto entryDone;
				}
				case hkAgent3::STREAM_CALL_FLIPPED: {
					flippedCallInput.m_overrideBodyA = nullptr;
					hkAgent1nMachine_flipInput(&in3, &flippedCallInput);
					extractShapes();
					const float oldToi = output.m_toi.m_time.m_storage;
					hkpProcessCdPoint* pp = output.m_firstFreeContactPoint.m_storage;
					currentData = static_cast<uint8_t*>(dispatcher->m_agent3Func[entry->m_agentType]
							.m_processFunc(flippedCallInput, entry,
								static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 1, nullptr, output));
					entry->m_size = static_cast<uint8_t>(currentData - reinterpret_cast<uint8_t*>(entry));
					// EXE-INLINED none: hkContactPoint::setFlipped, hkpProcessCollisionOutput::ToiInfo::flip
					for (; pp < output.m_firstFreeContactPoint.m_storage; ++pp) {
						__m128 normal = pp->m_contact.m_separatingNormal.m_quad;
						pp->m_contact.m_position.m_quad =
							_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normal, normal, 255), normal),
								pp->m_contact.m_position.m_quad);
						pp->m_contact.m_separatingNormal.m_quad = _mm_xor_ps(normal, rdata_10C4C00);
					}
					if (output.m_toi.m_time.m_storage != oldToi) {
						output.m_toi.m_contactPoint.m_separatingNormal.m_quad =
							_mm_xor_ps(rdata_10C4C00, output.m_toi.m_contactPoint.m_separatingNormal.m_quad);
						output.m_toi.m_gskCache.flip();
					}
					goto entryDone;
				}
				case hkAgent3::STREAM_CALL_WITH_TIM: {
					hkpAgent1nMachineTimEntry* timEntry = static_cast<hkpAgent1nMachineTimEntry*>(entry);
					const hkpProcessCollisionInput* pInput = in3.m_input;
					float distAtT1;
					if (pInput->m_stepInfo.m_startTime != timEntry->m_timeOfSeparatingNormal) {
						// A normal from at or after this step's end is kept as it is.
						if (!(timEntry->m_timeOfSeparatingNormal >= pInput->m_stepInfo.m_endTime)) {
							const hkpCollisionQualityInfo* quality = pInput->m_collisionQualityInfo;
							if (!quality->m_useContinuousPhysics.m_storage) {
								// No continuous physics: no separating normal, and the agent always runs.
								timEntry->m_timeOfSeparatingNormal = pInput->m_stepInfo.m_endTime;
								timEntry->m_separatingNormal.setZero4();
								distAtT1 = static_cast<float>(quality->m_minSeparation * 0.10000000149011612);
								timEntry->m_separatingNormal.m_quad[3] = distAtT1;
								goto processTim;
							}
							// The separating normal at the step's start: both bodies at their start transforms. The
							// input over them stays valid until the bodies are transformed.
							if (!sepInputInitialized) {
								sepInputInitialized = !bodiesTransformed;
								initSeparatingInput(&sepInput, &bodyTemp, &in3);
							}
							extractShapes();
							bodyTemp.m_bodyB.m_shape = in3.m_bodyB->m_shape;
							bodyTemp.m_bodyB.m_shapeKey = in3.m_bodyB->m_shapeKey;
							bodyTemp.m_bodyA.m_shape = in3.m_bodyA->m_shape;
							bodyTemp.m_bodyA.m_shapeKey = in3.m_bodyA->m_shapeKey;
							dispatcher->m_agent3Func[entry->m_agentType].m_sepNormalFunc(sepInput, entry, timEntry + 1,
								timEntry->m_separatingNormal);
							pInput = in3.m_input;
						}
					}
					{
						timEntry->m_timeOfSeparatingNormal = pInput->m_stepInfo.m_endTime;
						const float fProjected = in3.m_linearTimInfo.dot4xyz1(timEntry->m_separatingNormal);
						distAtT1 = timEntry->m_separatingNormal.m_quad[3] - fProjected;
						if (distAtT1 >= in3.m_input->m_tolerance) {
							// Still separated at the step's end: the agent does not run.
							timEntry->m_separatingNormal.m_quad[3] = distAtT1;
							if (!entry->m_numContactPoints) {
								currentData += entry->m_size;
								numTim++;
								goto entryDone;
							}
							currentData =
								static_cast<uint8_t*>(dispatcher->m_agent3Func[entry->m_agentType].m_cleanupFunc(entry,
									timEntry + 1, in3.m_contactMgr, *output.m_constraintOwner.m_storage));
							numTim++;
							entry->m_size = static_cast<uint8_t>(currentData - reinterpret_cast<uint8_t*>(entry));
							goto entryDone;
						}
					}
				processTim:
					// EXE-INLINED none: hkPadSpu<float>::operator=
					in3.m_distAtT1.m_storage = distAtT1;
					extractShapes();
					currentData = static_cast<uint8_t*>(dispatcher->m_agent3Func[entry->m_agentType].m_processFunc(in3,
						entry, timEntry + 1, &timEntry->m_separatingNormal, output));
					entry->m_size = static_cast<uint8_t>(currentData - reinterpret_cast<uint8_t*>(entry));
					goto entryDone;
				}
				case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED: {
					flippedTimInput.m_overrideBodyA = nullptr;
					hkAgent1nMachine_flipInput(&in3, &flippedTimInput);
					hkpAgent1nMachineTimEntry* timEntry = static_cast<hkpAgent1nMachineTimEntry*>(entry);
					float distAtT1;
					if (flippedTimInput.m_input->m_stepInfo.m_startTime != timEntry->m_timeOfSeparatingNormal) {
						const hkpCollisionQualityInfo* quality = flippedTimInput.m_input->m_collisionQualityInfo;
						if (!quality->m_useContinuousPhysics.m_storage) {
							timEntry->m_timeOfSeparatingNormal = flippedTimInput.m_input->m_stepInfo.m_endTime;
							timEntry->m_separatingNormal.setZero4();
							distAtT1 = static_cast<float>(quality->m_minSeparation * 0.5);
							timEntry->m_separatingNormal.m_quad[3] = distAtT1;
							goto processTimFlipped;
						}
						if (!sepInputFlippedInitialized) {
							sepInputFlippedInitialized = !bodiesTransformed;
							initSeparatingInput(&sepInputFlipped, &bodyTempFlipped, &flippedTimInput);
							bodyTempFlipped.m_bodyB.m_shape = flippedTimInput.m_bodyB->m_shape;
							bodyTempFlipped.m_bodyB.m_shapeKey = flippedTimInput.m_bodyB->m_shapeKey;
						}
						extractShapes();
						// The flipped input's body A is in3's body B.
						bodyTempFlipped.m_bodyA.m_shape = in3.m_bodyB->m_shape;
						bodyTempFlipped.m_bodyA.m_shapeKey = in3.m_bodyB->m_shapeKey;
						bodyTempFlipped.m_bodyB.m_shape = in3.m_bodyA->m_shape;
						bodyTempFlipped.m_bodyB.m_shapeKey = in3.m_bodyA->m_shapeKey;
						dispatcher->m_agent3Func[entry->m_agentType].m_sepNormalFunc(sepInputFlipped, entry,
							timEntry + 1, timEntry->m_separatingNormal);
					}
					{
						timEntry->m_timeOfSeparatingNormal = flippedTimInput.m_input->m_stepInfo.m_endTime;
						const float fProjected = flippedTimInput.m_linearTimInfo.dot4xyz1(timEntry->m_separatingNormal);
						distAtT1 = timEntry->m_separatingNormal.m_quad[3] - fProjected;
						if (distAtT1 >= flippedTimInput.m_input->m_tolerance) {
							timEntry->m_separatingNormal.m_quad[3] = distAtT1;
							if (!entry->m_numContactPoints) {
								currentData += entry->m_size;
								numTim++;
								goto entryDone;
							}
							currentData =
								static_cast<uint8_t*>(dispatcher->m_agent3Func[entry->m_agentType].m_cleanupFunc(entry,
									timEntry + 1, flippedTimInput.m_contactMgr, *output.m_constraintOwner.m_storage));
							numTim++;
							entry->m_size = static_cast<uint8_t>(currentData - reinterpret_cast<uint8_t*>(entry));
							goto entryDone;
						}
					}
				processTimFlipped:
					{
						// EXE-INLINED none: hkPadSpu<float>::operator=
						flippedTimInput.m_distAtT1.m_storage = distAtT1;
						extractShapes();
						hkpProcessCdPoint* pp = output.m_firstFreeContactPoint.m_storage;
						const float oldToi = output.m_toi.m_time.m_storage;
						currentData = static_cast<uint8_t*>(dispatcher->m_agent3Func[entry->m_agentType]
								.m_processFunc(flippedTimInput, entry, timEntry + 1, &timEntry->m_separatingNormal,
									output));
						entry->m_size = static_cast<uint8_t>(currentData - reinterpret_cast<uint8_t*>(entry));
						// EXE-INLINED none: hkContactPoint::setFlipped, hkpProcessCollisionOutput::ToiInfo::flip
						for (; pp < output.m_firstFreeContactPoint.m_storage; ++pp) {
							__m128 normal = pp->m_contact.m_separatingNormal.m_quad;
							pp->m_contact.m_position.m_quad =
								_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(normal, normal, 255), normal),
									pp->m_contact.m_position.m_quad);
							pp->m_contact.m_separatingNormal.m_quad = _mm_xor_ps(normal, rdata_10C4C00);
						}
						if (output.m_toi.m_time.m_storage != oldToi) {
							output.m_toi.m_contactPoint.m_separatingNormal.m_quad =
								_mm_xor_ps(output.m_toi.m_contactPoint.m_separatingNormal.m_quad, rdata_10C4C00);
							output.m_toi.m_gskCache.flip();
						}
						goto entryDone;
					}
				}
				case hkAgent3::STREAM_CALL_AGENT: {
					extractShapes();
					hkpAgentData* agentData = static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 1;
					hkpCollisionAgent* agent = *static_cast<hkpCollisionAgent**>(agentData);
					agent->processCollision(*in3.m_bodyA, *in3.m_bodyB, *in3.m_input, output);
					// The entry and one 16-byte slot holding the agent.
					currentData = reinterpret_cast<uint8_t*>(static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 2);
					goto entryDone;
				}
				case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
				case hkAgent3::STREAM_CALL_FLIPPED_WITH_TRANSFORM:
				case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
				case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED_WITH_TRANSFORM:
				case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM: {
					// Replace the bodies by their first non-transform levels and run the command again without
					// the flag; the separating-normal inputs over the old bodies are stale.
					extractShapes();
					uint8_t cdBodyHasTransformFlag = 0;
					in3.m_bodyB = hkAgentMachine_processTransformedShapes(in3.m_bodyB, newCdBodiesB, newMotionStatesB,
						4, cdBodyHasTransformFlag);
					in3.m_bodyA = hkAgentMachine_processTransformedShapes(in3.m_bodyA, newCdBodiesA, newMotionStatesA,
						4, cdBodyHasTransformFlag);
					in3.m_aTb.setMulInverseMul(*in3.m_bodyA->m_motion.pMotion, *in3.m_bodyB->m_motion.pMotion);
					command &= static_cast<uint8_t>(~hkAgent3::TRANSFORM_FLAG);
					bodiesTransformed = true;
					sepInputFlippedInitialized = false;
					sepInputInitialized = false;
					continue;
				}
				default:
					goto entryDone;
				}
			}
		}

	entryDone:
		if (readData == readEnd) {
			// The read sector is used up: it becomes the spare sector, or is freed when there is one.
			if (in3.m_input->m_spareAgentSector) {
				hkMemoryRouter::getInstance().heap().blockFree(readSector, sizeof(hkpAgent1nSector));
			} else {
				in3.m_input->m_spareAgentSector = readSector;
			}
			if (nextReadSectorIndex >= agentTrack.m_sectors.m_size) {
				break;
			}
			readSector = agentTrack.m_sectors.m_data[nextReadSectorIndex];
			readData = readSector->m_data;
			readEnd = readSector->m_data + readSector->m_bytesAllocated;
			nextReadSectorIndex++;
		}
		// Room for one more agent of the largest size, the extension buffer's.
		if (currentData + sizeof(extensionBuffer) <= currentEnd) {
			continue;
		}
		if (!inExtensionBuffer) {
			bytesInCurrentSector = static_cast<int32_t>(currentData - currentSector->m_data);
			inExtensionBuffer = true;
			currentData = extensionBuffer;
			currentEnd = extensionBuffer + sizeof(extensionBuffer);
			continue;
		}
		extensionSize = static_cast<int32_t>(currentData - extensionBuffer);
		if (extensionSize <= hkpAgent1nSector::NET_SECTOR_SIZE - bytesInCurrentSector) {
			hkString::memCpy16NonEmpty(currentSector->m_data + bytesInCurrentSector, extensionBuffer,
				extensionSize >> 4);
			bytesInCurrentSector += extensionSize;
			if (bytesInCurrentSector < hkpAgent1nSector::NET_SECTOR_SIZE) {
				currentData = extensionBuffer;
				continue;
			}
			inExtensionBuffer = false;
		}
		// The current sector is full: it goes into the track, before the sectors still to be read.
		currentSector->m_bytesAllocated = bytesInCurrentSector;
		if (currentSectorIndex >= nextReadSectorIndex) {
			agentTrack.m_sectors.expandAt(nextReadSectorIndex, 1);
			nextReadSectorIndex++;
		}
		agentTrack.m_sectors.m_data[currentSectorIndex] = currentSector;
		currentSectorIndex++;
		currentSector = in3.m_input->m_spareAgentSector;
		if (!currentSector) {
			// EXE-INLINED none: hkpAgent1nSector::operator new, hkpAgent1nSector::hkpAgent1nSector
			currentSector = static_cast<hkpAgent1nSector*>(hkMemoryRouter::getInstance()
					.heap()
					.blockAlloc(sizeof(hkpAgent1nSector)));
			if (currentSector) {
				currentSector->m_bytesAllocated = 0;
			}
		}
		currentData = currentSector->m_data;
		in3.m_input->m_spareAgentSector = nullptr;
		currentEnd = reinterpret_cast<uint8_t*>(currentSector + 1);
		if (inExtensionBuffer) {
			hkString::memCpy16NonEmpty(currentData, extensionBuffer, extensionSize >> 4);
			currentData += extensionSize;
			inExtensionBuffer = false;
		}
	}

	int32_t bytesAllocated;
	if (!inExtensionBuffer) {
		bytesAllocated = static_cast<int32_t>(currentData - currentSector->m_data);
	} else {
		extensionSize = static_cast<int32_t>(currentData - extensionBuffer);
		hkString::memCpy16NonEmpty(currentSector->m_data + bytesInCurrentSector, extensionBuffer, extensionSize >> 4);
		bytesAllocated = bytesInCurrentSector + extensionSize;
	}
	currentSector->m_bytesAllocated = bytesAllocated;
	agentTrack.m_sectors.setSize(currentSectorIndex + 1);
	agentTrack.m_sectors.m_data[currentSectorIndex] = currentSector;
	HK_MONITOR_ADD_VALUE("numTim", static_cast<float>(numTim), HK_MONITOR_TYPE_INT);
}
