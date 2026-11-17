#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppAssembler.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppCodeGenerator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpMoppCompilerChunkInfo;
class hkpMoppMediator;
class hkpMoppNodeMgr;
class hkpMoppTreeInternalNode;
class hkpMoppTreeNode;
class hkpMoppTreeTerminal;

// Appends the jump commands of apSource to apDest, their positions moved by aiOffset (apSource and aiOffset on the
// stack, apDest in esi; ret 0). Only caller: copyCodeAndEmbedTerminals (0xE08457), with a chunk's array and the main code's.
void appendJumpCommandsWithOffset(const hkArray<hkpMoppCodeGenerator::JumpCommandInfo, hkContainerHeapAllocator>*
									  apSource,
	int32_t aiOffset, hkArray<hkpMoppCodeGenerator::JumpCommandInfo, hkContainerHeapAllocator>* apDest);
// Appends the chunk code apChunk to r_edi: the primitives of the chunk's terminals packed by apChunkInfo's compressor
// (none without one: the terminals are cleared), then apChunk's code, padded to 4 bytes, and its jump commands;
// returns the bytes added (apChunkInfo, apChunk, apTerminals on the stack, r_edi in edi; ret 0). apTerminals is the
// terminal array calcChunkPoints fills: the compressor's slots get [apTerminals] and [apTerminals+4] as the terminals
// and their count (0xE083AF).
int32_t copyCodeAndEmbedTerminals(hkpMoppCompilerChunkInfo* apChunkInfo, hkpMoppCodeGenerator* apChunk,
	hkArray<uint32_t, hkContainerHeapAllocator>* apTerminals, hkpMoppCodeGenerator* r_edi);

// What the assembler knows about one node while it assembles it: its depth, the node's extents in the code's
// integer space and the significant bits they need, the resolution, and the offsets and spreads of the node's
// terminals and properties. Members are the PDB's (hkpMoppDefaultAssembler.cpp); sizeof 0x48 from the exe's
// 0x12-dword copy in the constructor 0xE095D0 and the 72-byte locals of assemble 0xE08CC0.
class hkpMoppAssemblerNodeInfo {
	// The assembler calls both helpers itself: preCalcCutPositions and calcChunkPoints
	// call updateExtents and updateOffsetAndSpread.
	friend class hkpMoppDefaultAssembler;

public:
	// The hybrid has no zero-parameter Construct: the implicit constructor, for the stack infos the assembler
	// reconstructs from their parents.
	hkpMoppAssemblerNodeInfo() = default;
	hkpMoppAssemblerNodeInfo(const hkpMoppAssemblerNodeInfo& arParent, const hkpMoppTreeNode* apNode,
		const hkpMoppCode::CodeInfo& arCodeInfo);

protected:
	// The node's primitive id spread and its property values' offsets and spreads.
	void updateOffsetAndSpread(const hkpMoppTreeNode* node);
	// The node's extents scaled into the code's integer space by arCodeInfo's offset and scale.
	void updateExtents(const hkpMoppTreeNode* node, const hkpMoppCode::CodeInfo& codeInfo);

public:
	// The bits the largest extent needs, at most 24, into m_maxExtentsSignificantBits.
	void calcMaxExtentsSignificantBits();

	int32_t m_level;
	// The constructor 0xE095D0 clears it as a byte.
	hkBool m_ignoreBandControlCode;
	// calcMaxExtentsSignificantBits 0xE09540 stores it, clamped to 24.
	int32_t m_maxExtentsSignificantBits;
	int32_t m_extents[3][2];
	// The shift calcPossibleCutPlanes 0xE06730 rounds the cut planes with; assemble stores 16.
	int32_t m_currentBitsResolution;
	int32_t m_accumOffset[3];
	int32_t m_terminalOffset;
	// updateOffsetAndSpread 0xE066F0 stores the node's primitive id spread here and the property values below.
	int32_t m_terminalSpread;
	int32_t m_numProperties;
	int32_t m_propertiesOffset[1];
	int32_t m_propertiesSpread[1];
};

// A node's info with the cut plane positions its extents allow, rounded to the resolution. Members are the PDB's;
// calcPossibleCutPlanes 0xE06730 writes the six dwords from +0x48, and the caller 0xE07DA0 keeps one in a 96-byte
// local.
class hkpMoppAssemblerCutInfo : public hkpMoppAssemblerNodeInfo {
public:
	// Each axis's extents rounded down and up to m_currentBitsResolution's grid.
	void calcPossibleCutPlanes();

	int32_t m_lowCutPlanePosition[3];
	int32_t m_highCutPlanePosition[3];
};

// The splitting plane directions the assembler can encode: the axes and the diagonals (M: minus), each with its split
// command (hkpMoppCommands' HK_MOPP_SPLIT_X + direction). Values from Havok's release hkpinternal.pdb.
enum HK_MOPP_SPLIT_DIRECTIONS : int32_t {
	HK_MOPP_SD_X = 0x0,
	HK_MOPP_SD_Y = 0x1,
	HK_MOPP_SD_Z = 0x2,
	HK_MOPP_SD_YZ = 0x3,
	HK_MOPP_SD_YMZ = 0x4,
	HK_MOPP_SD_XZ = 0x5,
	HK_MOPP_SD_XMZ = 0x6,
	HK_MOPP_SD_XY = 0x7,
	HK_MOPP_SD_XMY = 0x8,
	HK_MOPP_SD_XYZ = 0x9,
	HK_MOPP_SD_XYMZ = 0xA,
	HK_MOPP_SD_XMYZ = 0xB,
	HK_MOPP_SD_XMYMZ = 0xC,
	HK_MOPP_SD_MAX = 0xD,
};

// Whether a node's code starts with a cut and the cut's command and two plane positions (hkpinternal.pdb, sizeof
// 0x10).
class hkpMoppAssemblerCutCommand {
public:
	hkBool m_doCut;
	int32_t m_cutCommand;
	int32_t m_cutPlanes[2];
};

// Whether a node's code rescales the integer space, with the scale command and the offset per axis that go with it
// (hkpinternal.pdb, sizeof 0x14; the PDB's initTopDown, assembleNonTerminalCommand, recalcRescale and addRescale take
// it).
class hkpMoppAssemblerRescaleCommand {
public:
	hkBool m_doRescale;
	int32_t m_scaleCommand;
	int32_t m_moppOffset[3];
};

// Assembles a fitted MOPP tree into code, tracking chunk and splitting-plane state.
class hkpMoppDefaultAssembler : public hkpMoppAssembler {
public:
	hkpMoppDefaultAssembler(const hkpMoppAssembler::hkpMoppAssemblerParams& ap, hkpMoppCodeGenerator* a2,
		hkpMoppMediator* mediator);
	~hkpMoppDefaultAssembler() override;

	int32_t getNumSplittingPlaneDirections() const override;

	const hkpMoppSplittingPlaneDirection* getSplittingPlaneDirections() const override { return m_directions.begin(); }

	// Havok SDK signature (hkpInternal.lib, tied by bytes,vtable).
	void assemble(hkpMoppTreeNode* a1, hkpMoppNodeMgr* a2, int32_t a3) override;
	// Havok SDK signature (hkpInternal.lib, tied by vtable).
	void getScaleInfo(hkpMoppTreeNode* a1, hkpMoppCode::CodeInfo* a2) override;

	void releaseNode(hkpMoppTreeNode* node);
	void fixScale(hkpMoppTreeNode* currentNode);
	void addProperty(int32_t property, int32_t value);
	int32_t addJumpChunk(int32_t chunkId);
	int32_t addDataOffset(int32_t offset, int32_t numTerminals);
	void addTermIdOffset(uint32_t offset);
	void addJump(uint32_t jumpPos);
	void updateParams(const hkpMoppAssembler::hkpMoppAssemblerParams& ap);
	// Numbers the terminals of each chunk from 0 and records each primitive's old and new key in the chunk info's
	// reindex table when it has a compressor; returns the next id.
	int32_t calcTerminalIdsForChunks(hkpMoppTreeNode* currentNode, int32_t chunkId, int32_t currentId);
	int32_t assembleJumpChunkCommand(hkpMoppTreeNode* currentNode);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	void checkAndAddProperties(hkpMoppTreeNode* a1, const hkpMoppAssemblerNodeInfo& a2, hkpMoppAssemblerNodeInfo& a3);
	// The PDB's addSplit (0xE07680, thiscall, ret 0x14).
	void addSplit(int32_t a1, int32_t a2, HK_MOPP_SPLIT_DIRECTIONS a3, int32_t a4, int32_t rpos);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	int32_t assembleNonTerminalCommand(hkpMoppTreeNode* currentNode, const hkpMoppAssemblerNodeInfo& parentInfo,
		hkpMoppAssemblerNodeInfo& currentInfo, hkpMoppAssemblerRescaleCommand& rescaleCommandInfo);
	// The PDB's addRescale. Void: its one caller (assembleNonTerminalCommand, 0xE07D37) overwrites eax.
	void addRescale(const hkpMoppAssemblerRescaleCommand& cmd);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	int32_t preCalcScale(hkpMoppTreeNode* a1, const hkpMoppAssemblerNodeInfo& a2, hkpMoppAssemblerNodeInfo& a3);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	void addCut(const hkpMoppTreeNode* a1, const hkpMoppAssemblerNodeInfo& a2);
	// The PDB's shouldAssemble (0xE06AF0, thiscall, ret 0xC; the hkBool result through a hidden pointer).
	hkBool* shouldAssemble(hkBool* apResult, hkpMoppTreeNode* a2, const hkpMoppAssemblerNodeInfo& a3);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	int32_t addTerminals(hkpMoppTreeTerminal* a1, const hkpMoppAssemblerNodeInfo& a2,
		const hkpMoppAssemblerNodeInfo& a3);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	void calcTermCut(hkpMoppTreeNode* a1, hkpMoppAssemblerCutInfo& a2);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	void initTopDown(hkpMoppTreeNode* currentNode, const hkpMoppAssemblerNodeInfo& parentInfo,
		hkpMoppAssemblerNodeInfo& currentInfo, hkpMoppAssemblerRescaleCommand& rescaleCommandInfo);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	int32_t assembleCutAndTerminalCommand(hkpMoppTreeNode* a1, const hkpMoppAssemblerNodeInfo& a2,
		hkpMoppAssemblerNodeInfo& a3);
	// With the node not yet assembled:
	// a terminal's cut planes and command, else the parent's and the node's scale and cut positions along the
	// plane (thiscall, ret 0xC, 0xE07DA0). Only caller: assemble 0xE08CC0.
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	void preCalcCutPositions(hkpMoppTreeNode* currentNode, const hkpMoppAssemblerCutInfo& parentInfo,
		hkpMoppAssemblerCutInfo& currentInfo);
	// The PDB's assemblesubNode, matched by its locals (rescaleCommandInfo, childInfo):
	// assembles the node and its two children (+0x70, +0x74) recursively, 3 for a node already assembled, -1 on
	// failure; returns the code size (thiscall, ret 0xC, 0xE08040).
	int32_t assemblesubNode(hkpMoppTreeNode* currentNode, const hkpMoppAssemblerNodeInfo& parentInfo,
		hkpMoppAssemblerNodeInfo& currentInfo);
	// The PDB's calcChunkPoints, matched by its locals (three terminal arrays and
	// rescaleCommandInfo): splits the subtree into chunks, appending to apTerminals (thiscall, ret 0x10, 0xE084F0).
	int32_t calcChunkPoints(hkpMoppTreeNode* currentNode, const hkpMoppAssemblerNodeInfo& parentInfo,
		hkpMoppAssemblerNodeInfo& currentInfo, hkArray<uint32_t, hkContainerHeapAllocator>& terminalsOut);
	// The PDB's assembleSubNodeIntoChunks, matched by its locals (rescaleCommandInfo, a
	// chunkCode hkpMoppCodeGenerator, chunkTerminals): assembles the subtree, each chunk into its own code generator
	// (thiscall, ret 0x14, 0xE08A60). Guessed type: apChunkTerminals as calcChunkPoints's array, the same local.
	void assembleSubNodeIntoChunks(hkpMoppTreeNode* currentNode, const hkpMoppAssemblerNodeInfo& parentInfo,
		hkpMoppAssemblerNodeInfo& currentInfo, int32_t chunkId,
		hkArray<uint32_t, hkContainerHeapAllocator>& terminalsOut);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	void calcReoffset(hkpMoppTreeNode* a1, const hkpMoppAssemblerNodeInfo& a2, hkpMoppAssemblerNodeInfo& a3);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	int32_t calcRescaleBits(const hkpMoppAssemblerNodeInfo& a1, hkpMoppAssemblerNodeInfo& a2);
	// Havok SDK signature (hkpInternal.lib, tied by reloc).
	void findPlanes(const hkpMoppTreeInternalNode* a1, const hkpMoppAssemblerNodeInfo& a2, int32_t& a3, int32_t& a4);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	void recalcRescale(const hkpMoppAssemblerNodeInfo& parent, hkpMoppAssemblerNodeInfo& info,
		hkpMoppAssemblerRescaleCommand& cmd);

	// The PDB's getSplitCode: the split code of the splitting direction a1 (its x, y, z against +-0.1). The exe has it
	// as a static taking the direction on the stack (stdcall, ret 4, 0xE06F20); the PDB's form takes no argument.
	// It only reads the direction (assembleNonTerminalCommand passes the node's const plane, 0xE07CBE).
	static int32_t getSplitCode(const float* a1);

	hkpMoppMediator* m_mediator;
	hkpMoppNodeMgr* m_nodeMgr;
	hkpMoppCodeGenerator* m_code;
	hkpMoppCompilerChunkInfo* m_chunkInfo;
	int32_t m_minDepthToBuild;
	int32_t m_halfTreeDepth;
	int32_t m_depthTopDownScale;
	int32_t m_minAssembledElems;
	int32_t m_numNodesAssembled;
	// m_codeInfo starts with an hkVector4 and m_params holds one, so both are 16-byte aligned: the compiler inserts
	// the 4-byte gaps at 0x2C and 0x4C itself, where the PDB names no member.
	hkpMoppCode::CodeInfo m_codeInfo;
	hkArray<hkpMoppSplittingPlaneDirection, hkContainerHeapAllocator> m_directions;
	hkpMoppAssemblerParams m_params;
};
inline constexpr fnvd::Vtable hkpMoppDefaultAssembler_vftable{};
