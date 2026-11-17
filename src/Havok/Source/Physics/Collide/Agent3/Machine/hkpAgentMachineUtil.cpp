#include "Havok/Source/Physics/Collide/Agent3/Machine/hkpAgentMachineUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"

// Collects each sector as a normal chunk (the last one with the track's used bytes), then each entry's agent through
// its type's calcStatisticsFunc, with the agent's data after the padded or the TIM entry, and the entry's contact
// manager.
void hkpAgentMachineUtil::calcNnStatistics(const hkpAgentNnTrack& track, const hkpCollisionInput& input,
	hkStatisticsCollector* collector) {
	for (int32_t i = 0; i < track.m_sectors.m_size; ++i) {
		hkpAgentNnSector* sector = track.m_sectors.m_data[i];
		uint32_t size = (i + 1 == track.m_sectors.m_size) ? track.m_bytesUsedInLastSector : sizeof(hkpAgentNnSector);
		collector->addNormalChunk("Sector", sector, size, sizeof(hkpAgentNnSector));
	}
	for (int32_t i = 0; i < track.m_sectors.m_size;) {
		hkpAgentNnSector* sector = track.m_sectors.m_data[i];
		++i;
		uint32_t used = (i == track.m_sectors.m_size) ? track.m_bytesUsedInLastSector : sizeof(hkpAgentNnSector);
		// EXE-INLINED none: hkpAgentNnSector::getBegin. A sector is untyped bytes that the exe reads as a run of
		// entries of varying sizes, each followed by its agent's data.
		hkpAgentNnEntry* entry = reinterpret_cast<hkpAgentNnEntry*>(sector->m_data);
		hkpAgentNnEntry* end = reinterpret_cast<hkpAgentNnEntry*>(&sector->m_data[used]);
		// EXE-INLINED none: hkAddByteOffset<hkpAgentNnEntry>, the entry's m_size in bytes
		for (; entry < end;
			entry = reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size)) {
			hkpAgentData* agentData;
			switch (entry->m_streamCommand) {
			case hkAgent3::STREAM_CALL:
			case hkAgent3::STREAM_CALL_AGENT:
			case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
			case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM:
				agentData = static_cast<hkpAgentNnMachinePaddedEntry*>(entry) + 1;
				break;
			case hkAgent3::STREAM_CALL_WITH_TIM:
			case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
				agentData = static_cast<hkpAgentNnMachineTimEntry*>(entry) + 1;
				break;
			default:
				continue;
			}
			hkAgent3::CalcStatisticsFunc func =
				input.m_dispatcher->m_agent3Func[entry->m_agentType].m_calcStatisticsFunc;
			if (func) {
				func(entry, agentData, input, collector);
			}
			collector->addReferencedObject("ContactMgr", entry->m_contactMgr, 0);
		}
	}
}

// Collects the sector array as an array chunk (storage the array does not own counts only with
// m_includeUnmanagedArrayMemory), then each entry's contact manager.
void hkpAgentMachineUtil::calcNnStatisticsContactMgrsOnly(const hkpAgentNnTrack& track,
	hkStatisticsCollector* collector) {
	if (!(track.m_sectors.m_capacityAndFlags & hkArrayBase<hkpAgentNnSector*>::DONT_DEALLOCATE_FLAG) ||
		collector->m_includeUnmanagedArrayMemory) {
		int32_t allocated = hkStatisticsCollector::getAllocatedSize(track.m_sectors.m_data,
			track.m_sectors.getCapacity() * static_cast<int32_t>(sizeof(hkpAgentNnSector*)));
		collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "Sectors", track.m_sectors.m_data,
			track.m_sectors.m_size * static_cast<int32_t>(sizeof(hkpAgentNnSector*)), allocated);
	}
	for (int32_t i = 0; i < track.m_sectors.m_size;) {
		hkpAgentNnSector* sector = track.m_sectors.m_data[i];
		++i;
		uint32_t used = (i == track.m_sectors.m_size) ? track.m_bytesUsedInLastSector : sizeof(hkpAgentNnSector);
		// EXE-INLINED none: hkpAgentNnSector::getBegin. A sector is untyped bytes that the exe reads as a run of
		// entries of varying sizes, each followed by its agent's data.
		hkpAgentNnEntry* entry = reinterpret_cast<hkpAgentNnEntry*>(sector->m_data);
		hkpAgentNnEntry* end = reinterpret_cast<hkpAgentNnEntry*>(&sector->m_data[used]);
		// EXE-INLINED none: hkAddByteOffset<hkpAgentNnEntry>, the entry's m_size in bytes
		for (; entry < end;
			entry = reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size)) {
			switch (entry->m_streamCommand) {
			case hkAgent3::STREAM_CALL:
			case hkAgent3::STREAM_CALL_WITH_TIM:
			case hkAgent3::STREAM_CALL_AGENT:
			case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
			case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
			case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM:
				collector->addReferencedObject("ContactMgr", entry->m_contactMgr, 0);
				break;
			default:
				break;
			}
		}
	}
}

// Reports the track's sector array and each sector, then hands each agent to the dispatcher's statistics function. The
// agent data is the track (hkpAgentData is void) and the client data the collision input whose dispatcher it reads and
// which it hands on, hence the two static casts. The sectors are walked as in hkAgent1nMachine_Destroy, except that a
// sector's first entry is handled before its end is tested.
void hkpAgentMachineUtil::calc1nStatistics(const hkpAgentData* agentData, const void* clientData,
	hkStatisticsCollector* collector) {
	const hkpAgent1nTrack* track = static_cast<const hkpAgent1nTrack*>(agentData);
	const hkpCollisionInput* input = static_cast<const hkpCollisionInput*>(clientData);
	if (track->m_sectors.getCapacityAndFlags() >= 0 || collector->m_includeUnmanagedArrayMemory) {
		int32_t allocatedSize = hkStatisticsCollector::getAllocatedSize(track->m_sectors.m_data,
			track->m_sectors.getCapacity() * static_cast<int32_t>(sizeof(hkpAgent1nSector*)));
		collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "SectorPtrs", track->m_sectors.m_data,
			track->m_sectors.m_size * static_cast<int32_t>(sizeof(hkpAgent1nSector*)), allocatedSize);
	}
	for (int32_t sectorIndex = 0;; ++sectorIndex) {
		hkpAgent1nSector* sector = track->m_sectors.m_data[sectorIndex];
		uint8_t* end = sector->m_data + sector->m_bytesAllocated;
		collector->addNormalChunk("Sector", sector,
			static_cast<int32_t>(offsetof(hkpAgent1nSector, m_data) + sector->m_bytesAllocated),
			hkpAgent1nSector::SECTOR_SIZE);
		hkpAgent1nMachineEntry* entry = reinterpret_cast<hkpAgent1nMachineEntry*>(sector->m_data);
		do {
			hkpAgentData* entryData;
			switch (entry->m_streamCommand) {
			case hkAgent3::STREAM_NULL:
				entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
				continue;
			case hkAgent3::STREAM_END:
				return;
			case hkAgent3::STREAM_CALL:
			case hkAgent3::STREAM_CALL_FLIPPED:
			case hkAgent3::STREAM_CALL_AGENT:
			case hkAgent3::STREAM_CALL_WITH_TRANSFORM:
			case hkAgent3::STREAM_CALL_FLIPPED_WITH_TRANSFORM:
			case hkAgent3::STREAM_CALL_AGENT_WITH_TRANSFORM:
				entryData = static_cast<hkpAgent1nMachinePaddedEntry*>(entry) + 1;
				break;
			case hkAgent3::STREAM_CALL_WITH_TIM:
			case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED:
			case hkAgent3::STREAM_CALL_WITH_TIM_WITH_TRANSFORM:
			case hkAgent3::STREAM_CALL_WITH_TIM_FLIPPED_WITH_TRANSFORM:
				entryData = static_cast<hkpAgent1nMachineTimEntry*>(entry) + 1;
				break;
			default:
				continue;
			}
			hkAgent3::CalcStatisticsFunc calcStatistics =
				input->m_dispatcher->m_agent3Func[entry->m_agentType].m_calcStatisticsFunc;
			if (calcStatistics) {
				calcStatistics(entry, entryData, *input, collector);
			}
			entry = reinterpret_cast<hkpAgent1nMachineEntry*>(reinterpret_cast<uint8_t*>(entry) + entry->m_size);
		} while (reinterpret_cast<uint8_t*>(entry) < end);
	}
}
