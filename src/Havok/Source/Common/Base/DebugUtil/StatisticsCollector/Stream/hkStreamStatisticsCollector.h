#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamAnalyzer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkOstream;
class hkVtableClassRegistry;

// A statistics collector that records what it is told as a command stream (push, pop, chunk) in a byte array, for the
// visual debugger to read back.
class hkStreamStatisticsCollector : public hkStatisticsCollector {
public:
	// One command of the stream.
	struct Command {
		char m_cmd;
		uint8_t m_slen;
		uint16_t m_memSize;
		uint16_t m_allocSize;
	};

	hkStreamStatisticsCollector(hkVtableClassRegistry* vtblReg, int32_t memSize);
	~hkStreamStatisticsCollector() override;

	void addReferencedObject(const char* fieldName, const hkReferencedObject* obj, int32_t flags) override;
	void addObject(const hkClass& cls, const char* fieldName, const void* obj, int32_t flags) override;
	void addChunk(MemoryType type, const char* name, const void* chunkAddress, int32_t usedSize,
		int32_t allocSize) override;
	void pushDir(const char* dirName) override;
	void popDir() override;

	// Builds the timer tree of the recorded stream into node.
	void makeTree(hkArray<hkMonitorStreamAnalyzer::Node*>& m_nodesOut, hkArray<hkStringPtr>* stringTable);
	void beginSnapshot(int32_t statisticClasses);
	void endSnapshot();
	void reset();
	void setDataByCopying(const char* buffer, int32_t length, hkBool endianSwap);
	void writeStatistics(hkOstream& outstream, int32_t reportLevel);
	const char* getDataStream() { return m_stream.getSize() > 0 ? m_stream.begin() : nullptr; }
	int32_t getDataStreamSize() { return m_stream.getSize(); }
	// Appends a push command for the object obj of size bytes named name.
	void _startObject(const char* name, const void* object, int32_t size);
	// Ends the object _startObject began: a popDir through the vtable.
	void _endObject();

	hkArray<char> m_stream;
	hkPointerMap<const void*, int32_t> m_knownObjects;
};

// Not named in the PDB or IDA; name chosen here. Grows stream by one Command and a name of a1 bytes, and returns
// where the Command goes: the stream's end before the growth (the callers test hasSpace first, so the buffer does
// not move). The stream comes in esi, a1 on the stack (cdecl, ret 0).
hkStreamStatisticsCollector::Command* _newCommand(int32_t a1, hkArray<char, hkContainerHeapAllocator>* r_esi);
// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
void _sumNodes(hkMonitorStreamAnalyzer::Node* a1, hkArray<hkStringPtr, hkContainerHeapAllocator>* a2);

inline constexpr fnvd::Vtable hkStreamStatisticsCollector_vftable{};
