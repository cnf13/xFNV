#pragma once

#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppAssembler.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppCostFunction.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppSplitter.h"

#include <stdint.h>

class hkpMoppCompilerChunkInfo;
class hkpMoppMediator;

int32_t writeChunkOffset(uint8_t* a1, int32_t r_eax);
// Reads the big-endian 32-bit operand after the command byte at apThis, the counterpart of writeChunkOffset's store.
// compile reads each jump-chunk command's chunk id with it.
int32_t readChunkId(const uint8_t* apThis);

// Builds MOPP code from a mediator's primitives.
class hkpMoppCompiler {
public:
	// The implicit constructor, for the stack compiler that hkpMoppUtility constructs again with its mesh type.
	hkpMoppCompiler() = default;
	hkpMoppCompiler(int32_t aiMeshType);

	// Copies arSplitParams into m_splitParams.
	void setSplitParams(const hkpMoppSplitter::hkpMoppSplitParams& sp);
	// Havok SDK signature (hkpInternal.lib, tied by bytes).
	int32_t calculateRequiredBufferSize(hkpMoppMediator* mediator);
	// Copies arParams into m_splitCostParams.
	void setCostParams(const hkpMoppCostFunction::hkpMoppSplitCostParams& sp);
	// Copies arParams into m_assemblerParams.
	void setAssemblerParams(const hkpMoppAssembler::hkpMoppAssemblerParams& ap);
	// Builds the MOPP code of apMediator's primitives in the aiBufferSize bytes at apBuffer.
	hkpMoppCode* compile(hkpMoppMediator* mediator, char* buffer, int32_t bufferSize);
	void enableInterleavedBuilding(bool a1);

	hkpMoppTreeNode* m_debugRootNode;
	hkpMoppCompilerChunkInfo* m_chunkInfo;
	hkpMoppSplitter::hkpMoppSplitParams m_splitParams;
	hkpMoppCostFunction::hkpMoppSplitCostParams m_splitCostParams;
	// 16-aligned (its hkVector4), so 8 bytes of padding precede it.
	hkpMoppAssembler::hkpMoppAssemblerParams m_assemblerParams;
};
