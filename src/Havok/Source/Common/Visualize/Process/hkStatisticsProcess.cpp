#include "Havok/Source/Common/Visualize/Process/hkStatisticsProcess.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamAnalyzer.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamFrameInfo.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Array/hkArrayStreamWriter.h"
#include "Havok/Source/Common/Base/System/Stopwatch/hkStopwatch.h"
#include "Havok/Source/Common/Base/Types/hkTypeTraits.h"
#include "Havok/Source/Common/Visualize/Serialize/hkDisplaySerializeOStream.h"
#include "Havok/Source/Common/Visualize/Serialize/hkObjectSerialize.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"

hkStatisticsProcess::hkStatisticsProcess(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
  : hkProcess(true) {
    hkMonitorStreamFrameInfo info;
    info.m_heading = nullptr;
    info.m_absoluteTimeCounter.m_storage = 0;
    info.m_indexOfTimer0 = 0;
    info.m_indexOfTimer1 = 1;
    info.m_timerFactor0 = 1000.0 / static_cast<double>(hkStopwatch::getTicksPerSecond());
    info.m_timerFactor1 = 1.0f;
    hkArray<hkObjectSerialize::GlobalFixup, hkContainerHeapAllocator> globalFixups;
    {
        hkArrayStreamWriter writer(&m_infoBuffer, hkArrayStreamWriter::ARRAY_BORROW);
        hkStructureLayout destLayout_(hkStructureLayout::MsvcWin32LayoutRules);
        hkObjectSerialize::writeObject(&writer, destLayout_, 1u, &info, hkMonitorStreamFrameInfoClass, globalFixups);
    }
    m_contexts.copyFromArray(hkContainerHeapAllocator().get(&m_contexts), contexts, hkTypeIsPod());
}

hkStatisticsProcess::~hkStatisticsProcess() {}

int32_t hkStatisticsProcess::getProcessTag() {
    return hkStatisticsProcess::m_tag;
}

// Sends the visual debugger the monitor streams of the first context that has any (else the calling thread's stream),
// after m_infoBuffer and the map of their timer names, serialized into m_strBuffer.
void hkStatisticsProcess::step(float frameTimeInMs) {
    if (!m_outStream) {
        return;
    }
    using globalFixups_t = hkArray<hkObjectSerialize::GlobalFixup, hkContainerHeapAllocator>;
    using ptrs_t = hkArray<const char*, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(globalFixups_t, globalFixups, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(ptrs_t, startPtrs, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(ptrs_t, endPtrs, std::nullopt, std::nullopt);
    globalFixups.m_capacityAndFlags = hkArrayBase<hkObjectSerialize::GlobalFixup>::DONT_DEALLOCATE_FLAG;
    startPtrs.m_capacityAndFlags = hkArrayBase<const char*>::DONT_DEALLOCATE_FLAG;
    endPtrs.m_capacityAndFlags = hkArrayBase<const char*>::DONT_DEALLOCATE_FLAG;
    globalFixups.m_data = nullptr;
    globalFixups.m_size = 0;
    startPtrs.m_data = nullptr;
    startPtrs.m_size = 0;
    endPtrs.m_data = nullptr;
    endPtrs.m_size = 0;
    const int32_t numContexts = m_contexts.getSize();
    for (int32_t c = 0; c < numContexts; ++c) {
        if (m_contexts.m_data[c]->m_monitorStreamBegins.getSize() > 0) {
            // EXE-INLINED none: hkArray<const char*>::operator=
            startPtrs.copyFromArray(
                hkContainerHeapAllocator().get(&startPtrs), m_contexts.m_data[c]->m_monitorStreamBegins, hkTypeIsPod()
            );
            // EXE-INLINED none: hkArray<const char*>::operator=
            endPtrs.copyFromArray(
                hkContainerHeapAllocator().get(&endPtrs), m_contexts.m_data[c]->m_monitorStreamEnds, hkTypeIsPod()
            );
            break;
        }
    }
    if (startPtrs.getSize() == 0) {
        hkMonitorStream& stream = hkMonitorStream::getInstance();
        const char* start = stream.m_start;
        const char* end = stream.m_end;
        // EXE-INLINED none: hkArray<const char*>::pushBack
        hkMemoryAllocator& startAlloc = hkContainerHeapAllocator().get(&startPtrs);
        if (startPtrs.getSize() == startPtrs.getCapacity()) {
            hkArrayUtil::_reserveMore(startAlloc, &startPtrs, sizeof(const char*));
        }
        startPtrs.m_data[startPtrs.m_size++] = start;
        // EXE-INLINED none: hkArray<const char*>::pushBack
        hkMemoryAllocator& endAlloc = hkContainerHeapAllocator().get(&endPtrs);
        if (endPtrs.getSize() == endPtrs.getCapacity()) {
            hkArrayUtil::_reserveMore(endAlloc, &endPtrs, sizeof(const char*));
        }
        endPtrs.m_data[endPtrs.m_size++] = end;
    }

    // EXE-INLINED none: hkArray<char>::setSize
    m_strBuffer._reserve(hkContainerHeapAllocator().get(&m_strBuffer), 0);
    m_strBuffer.m_size = 0;
    int32_t totalMonLen = 0;
    {
        using map_t = hkPointerMap<const void*, const char*, hkContainerHeapAllocator>;
        FNVD_INLINED_LOCAL(map_t, map, std::nullopt, std::nullopt);
        map.m_map.m_elem = nullptr;
        map.m_map.m_numElems = 0;
        map.m_map.m_hashMod = -1;
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        map.m_map.hkUlongMapBase::reserve(*Instance->m_heap, 0);
        hkMonitorStreamStringMap strMap;
        for (int32_t si = 0; si < startPtrs.getSize(); ++si) {
            hkMonitorStreamAnalyzer::extractStringMap(startPtrs.m_data[si], endPtrs.m_data[si], map);
            totalMonLen += static_cast<int32_t>(endPtrs.m_data[si] - startPtrs.m_data[si]);
        }
        for (int32_t it = map.getIterator(); map.isValid(it); it = map.getNext(it)) {
            // EXE-INLINED none: hkArray<hkMonitorStreamStringMap::StringMap>::expandOne
            hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&strMap.m_map);
            if (strMap.m_map.getSize() == strMap.m_map.getCapacity()) {
                hkArrayUtil::_reserveMore(alloc, &strMap.m_map, sizeof(hkMonitorStreamStringMap::StringMap));
            }
            hkMonitorStreamStringMap::StringMap* slot = &strMap.m_map.m_data[strMap.m_map.m_size];
            if (slot) {
                ::new (slot) hkMonitorStreamStringMap::StringMap;
            }
            hkMonitorStreamStringMap::StringMap& newEntry = strMap.m_map.m_data[strMap.m_map.m_size++];
            // The key's 32 bits are sign-extended into the 64-bit id.
            newEntry.m_id = static_cast<hkUint64>(static_cast<int32_t>(map.m_map.m_elem[it].key));
            newEntry.m_string = map.getValue(it);
        }
        {
            // The writer's destructor runs where the block ends, right after writeObject.
            hkArrayStreamWriter writer(&m_strBuffer, hkArrayStreamWriter::ARRAY_BORROW);
            hkStructureLayout destLayout;
            destLayout.~hkStructureLayout();
            ::new (&destLayout) hkStructureLayout(hkStructureLayout::MsvcWin32LayoutRules);
            hkObjectSerialize::writeObject(
                &writer, destLayout, 1u, &strMap, hkMonitorStreamStringMapClass, globalFixups
            );
        }
        hkMemoryRouter* Instance_5 = &hkMemoryRouter::getInstance();
        map.m_map.clearAndDeallocate(*Instance_5->m_heap);
    }

    if (totalMonLen >= 1) {
        const int32_t strSize = m_strBuffer.getSize();
        const int32_t infoSize = m_infoBuffer.getSize();
        const int32_t numStreams = startPtrs.getSize();
        m_outStream->write32u(infoSize + strSize + 4 * numStreams + totalMonLen + 13);
        m_outStream->write8u(static_cast<uint8_t>(HK_SEND_STATISTICS_DUMP));
        m_outStream->write32(infoSize);
        if (infoSize > 0) {
            m_outStream->writeRaw(m_infoBuffer.m_data, infoSize);
        }
        m_outStream->write32(strSize);
        if (strSize > 0) {
            m_outStream->writeRaw(m_strBuffer.m_data, strSize);
        }
        m_outStream->write32(numStreams);
        for (int32_t i = 0; i < numStreams; ++i) {
            const int32_t len = static_cast<int32_t>(endPtrs.m_data[i] - startPtrs.m_data[i]);
            m_outStream->write32(len);
            if (len > 0) {
                m_outStream->writeRaw(startPtrs.m_data[i], len);
            }
        }
    }

    // The arrays are destroyed here, the last declared first.
    hkMemoryAllocator* endHeap = (&hkMemoryRouter::getInstance())->m_heap;
    endPtrs.m_size = 0;
    if ((endPtrs.m_capacityAndFlags & hkArrayBase<const char*>::DONT_DEALLOCATE_FLAG) == 0) {
        endHeap->_bufFree<const char*>(
            endPtrs.m_data, endPtrs.m_capacityAndFlags & hkArrayBase<const char*>::CAPACITY_MASK
        );
    }
    endPtrs.m_data = nullptr;
    endPtrs.m_capacityAndFlags = hkArrayBase<const char*>::DONT_DEALLOCATE_FLAG;
    hkMemoryAllocator* startHeap = (&hkMemoryRouter::getInstance())->m_heap;
    startPtrs.m_size = 0;
    if ((startPtrs.m_capacityAndFlags & hkArrayBase<const char*>::DONT_DEALLOCATE_FLAG) == 0) {
        startHeap->_bufFree<const char*>(
            startPtrs.m_data, startPtrs.m_capacityAndFlags & hkArrayBase<const char*>::CAPACITY_MASK
        );
    }
    startPtrs.m_data = nullptr;
    startPtrs.m_capacityAndFlags = hkArrayBase<const char*>::DONT_DEALLOCATE_FLAG;
    hkMemoryAllocator* fixupHeap = (&hkMemoryRouter::getInstance())->m_heap;
    globalFixups.m_size = 0;
    if ((globalFixups.m_capacityAndFlags & hkArrayBase<hkObjectSerialize::GlobalFixup>::DONT_DEALLOCATE_FLAG) == 0) {
        fixupHeap->_bufFree<hkObjectSerialize::GlobalFixup>(
            globalFixups.begin(),
            globalFixups.m_capacityAndFlags & hkArrayBase<hkObjectSerialize::GlobalFixup>::CAPACITY_MASK
        );
    }
}

void hkStatisticsProcess::registerProcess() {
    m_tag = hkProcessFactory::s_instance->registerProcess(hkStatisticsProcess_Name, &hkStatisticsProcess::create);
}

hkProcess* hkStatisticsProcess::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkStatisticsProcess* v2 =
        static_cast<hkStatisticsProcess*>(Instance->m_heap->blockAlloc(sizeof(hkStatisticsProcess)));
    v2->m_memSizeAndFlags = sizeof(hkStatisticsProcess);
    hkStatisticsProcess* v3 = ::new (v2) hkStatisticsProcess(contexts);
    if (v3) {
        return static_cast<hkProcess*>(v3);
    }
    else {
        return nullptr;
    }
}
