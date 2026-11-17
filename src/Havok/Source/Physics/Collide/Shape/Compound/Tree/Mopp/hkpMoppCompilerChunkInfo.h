#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Reconstruction.hpp"

class hkpMoppCodeGenerator;

// Packs the primitives a chunked MOPP's terminals refer to into the chunk (hkpinternal.pdb, sizeof 4; the SDK ships no
// header for it). The exe has no vtable of it or of a class built on it (no RTTI); the MOPP assembler calls slot 1
// on hkpMoppCompilerChunkInfo::m_compressor with the terminals and their count (calcChunkPoints, 0xE084F0).
class hkpPrimitiveCompressor {
public:
	virtual ~hkpPrimitiveCompressor();

	// The bytes the primitives of numTerminals terminals take in a chunk.
	virtual int32_t calcPrimitivesSize(const uint32_t* terminals, int32_t numTerminals) const = 0;
	// Writes the primitives of numTerminals terminals to dataOut.
	virtual void storePrimitives(const uint32_t* terminals, int32_t numTerminals, uint8_t* dataOut) const = 0;
};

// Tracks code chunks and reindexed terminals during MOPP compilation.
class hkpMoppCompilerChunkInfo {
public:
	struct Chunk;

	~hkpMoppCompilerChunkInfo();

	int32_t m_maxChunkSize;
	hkArray<hkpMoppCodeReindexedTerminal, hkContainerHeapAllocator> m_reindexInfo;
	hkArray<Chunk, hkContainerHeapAllocator> m_chunks;
	hkpPrimitiveCompressor* m_compressor;
	int32_t m_safetySize;
};

// One chunk of the code the compiler builds: the generated code and its size.
struct hkpMoppCompilerChunkInfo::Chunk {
	hkpMoppCodeGenerator* m_code;
	int32_t m_codeSize;
};
