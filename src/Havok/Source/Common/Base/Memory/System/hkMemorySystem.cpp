#include "Havok/Source/Common/Base/Memory/System/hkMemorySystem.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/Thread/hkThreadMemory.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"

void printDetailedStatistics(hkOstream& aOstr, const hkMemoryAllocator::MemoryStatistics& arStats, uint32_t aiSoftLimit,
	const char* apName) {
	uint32_t uiInUse = arStats.m_inUse;
	uint32_t uiTotal = arStats.m_allocated;
	if (arStats.m_allocated <= uiInUse) {
		uiTotal = arStats.m_inUse;
	}
	uint32_t uiAvailable = arStats.m_available;
	int32_t iOverhead = uiTotal - uiInUse - uiAvailable;
	float fOverhead = static_cast<float>(iOverhead);
	float fOverheadRatio = fOverhead / static_cast<double>(uiAvailable + uiInUse);
	// 0x7FFFFFFF: the largest signed limit, which stands for "no limit" as 0 does.
	if (!aiSoftLimit || aiSoftLimit == 0x7FFFFFFF) {
		aiSoftLimit = uiTotal;
	}
	float fSoftLimit = static_cast<double>(aiSoftLimit);
	uint32_t uiUsable = static_cast<int64_t>((1.0 - fOverheadRatio) * fSoftLimit);
	uint32_t uiPercentBase = uiUsable;
	if (aiSoftLimit < uiUsable) {
		uiPercentBase = aiSoftLimit;
	}
	float fPercentScale = 100.0 / static_cast<double>(uiPercentBase);
	aOstr.printf("\t%s:\n", apName);
	aOstr.printf("            totalSize:   %10i\n", uiTotal);
	if (aiSoftLimit < uiTotal) {
		aOstr.printf("\n            - reserved:  %10i (%4.1f%%)\n", uiTotal - aiSoftLimit,
			static_cast<double>(uiTotal - aiSoftLimit) * 100.0 / static_cast<double>(uiUsable));
		aOstr.printf("            = softlimit: %10i\n", aiSoftLimit);
	}
	aOstr.printf("            - overhead:  %10i (%4.1f%%)\n", iOverhead, fOverhead * 100.0 / fSoftLimit);
	aOstr.printf("            = available: %10i\n", uiUsable);
	aOstr.printf("              used:      %10i (%4.1f%%)\n", arStats.m_inUse,
		static_cast<double>(arStats.m_inUse) * fPercentScale);
	aOstr.printf("              free:      %10i (%4.1f%%)\n", arStats.m_available,
		static_cast<double>(arStats.m_available) * fPercentScale);
	aOstr.printf("              peak:      %10i (%4.1f%%)\n\n", arStats.m_peakInUse,
		static_cast<double>(arStats.m_peakInUse) * fPercentScale);
}

// The exe's constructor: the frame the main thread's router is sized for, with the solver buffer it allocates.
hkMemorySystem::FrameInfo::FrameInfo() : m_stackSize(0x32000), m_solverBufferSize(0x600000) {}

// The arrays go with their own destructors. The exe frees them through the debug allocator, which their type
// (hkArray's default heap allocator) does not name.
hkMemorySystem::_PrintStatisticsInput::~_PrintStatisticsInput() {}

void hkMemorySystem::garbageCollect() {
	;
}

// No debug memory: no block is found.
const void* hkMemorySystem::debugFindBaseAddress(const void* p, int32_t nbytes) {
	return nullptr;
}

// No debug memory: no snapshot is taken.
hkResult hkMemorySystem::getMemorySnapshot(hkDebugMemorySnapshot& snapshot) {
	return HK_FAILURE;
}

// No call stacks are kept: the exe returns 1 without reading its arguments.
hkResult hkMemorySystem::getAllocationCallStack(const void* ptr, hkMemoryPointerInfo& info) {
	return HK_FAILURE;
}

// The heap's figures, each thread memory's cached bytes, each stack's size and use, then the solver's and the debug
// allocator's figures. The debug allocator counts as unused when its in-use figure equals the heap's.
void hkMemorySystem::_printStatistics(hkOstream& ostr, const _PrintStatisticsInput& input) {
	hkMemoryAllocator::MemoryStatistics stats = {0, 0, 0, 0, 0, 0};
	input.m_heapAllocator->getMemoryStatistics(stats);
	uint32_t heapInUse = stats.m_inUse;
	printDetailedStatistics(ostr, stats, input.m_softLimit, "Heap");
	ostr.printf("\tThreadMemory (=CachedHeapMemory)\n");

	int32_t threadTotal = 0;
	for (int32_t i = 0; i < input.m_threadMemories.m_size; ++i) {
		uint32_t cachedSize = input.m_threadMemories.m_data[i]->getCachedSizeUnchecked();
		if (cachedSize != 0) {
			ostr.printf("            Thread %i  size %10i\n", i, cachedSize);
			threadTotal += cachedSize;
		}
	}
	ostr.printf("            = total %i\n\n", threadTotal);
	ostr.printf("\tStackMemory\n");

	int32_t stackTotal = 0;
	for (int32_t i = 0; i < input.m_stacks.m_size; ++i) {
		hkStackMemory* pStack = input.m_stacks.m_data[i];
		int32_t size = static_cast<int32_t>(static_cast<char*>(pStack->m_end.m_storage) -
			static_cast<char*>(pStack->m_start.m_storage));
		// The exe computes the used bytes as (next - end) + size.
		int32_t used =
			static_cast<int32_t>(pStack->m_next.m_storage - static_cast<char*>(pStack->m_end.m_storage)) + size;
		stackTotal += size;
		ostr.printf("            Thread %i  size %10i      used:    %6i (%4.1f%%)\n", i, size, used,
			used * 100.0 / size);
	}
	if (input.m_stacks.m_size == 0) {
		stackTotal = input.m_sumAllocatedStackMemory;
	}
	ostr.printf("            = total %i\n\n", stackTotal);

	hkMemoryRouter::getInstance().m_solver->getMemoryStatistics(stats);
	ostr.printf("\t%s:\n", "Solver");
	ostr.printf("            totalSize:   %10i\n", stats.m_allocated);
	ostr.printf("              peak:      %10i (%4.1f%%)\n\n", stats.m_peakInUse,
		static_cast<double>(stats.m_peakInUse) * 100.0 / static_cast<double>(stats.m_allocated));

	hkMemoryRouter::getInstance().m_debug->getMemoryStatistics(stats);
	if (heapInUse == stats.m_inUse) {
		ostr.printf("\tDebugMem: Not used.\n\n");
	} else {
		printDetailedStatistics(ostr, stats, 0x7FFFFFFF, "DebugMem");
	}
}

void hkMemorySystem::replaceInstance(hkMemorySystem* a1) {
	hkMemorySystem::s_instance = a1;
}

hkMemorySystem* hkMemorySystem::getInstancePtr() {
	return hkMemorySystem::s_instance;
}

hkMemorySystem& hkMemorySystem::getInstance() {
	return *hkMemorySystem::s_instance;
}
