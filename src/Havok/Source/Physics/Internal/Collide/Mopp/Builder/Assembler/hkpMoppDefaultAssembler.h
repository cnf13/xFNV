#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppAssembler.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppCodeGenerator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

#include <stdint.h>

class hkpMoppCompilerChunkInfo;
class hkpMoppMediator;
class hkpMoppNodeMgr;
class hkpMoppTreeInternalNode;
class hkpMoppTreeNode;
class hkpMoppTreeTerminal;

// Appends the jump commands of apSource to apDest, their positions moved by aiOffset. Only caller:
// copyCodeAndEmbedTerminals, with a chunk's array and the main code's.
void appendJumpCommandsWithOffset(
    const hkArray<hkpMoppCodeGenerator::JumpCommandInfo, hkContainerHeapAllocator>* apSource,
    int32_t aiOffset,
    hkArray<hkpMoppCodeGenerator::JumpCommandInfo, hkContainerHeapAllocator>* apDest
);
// Appends the chunk code apChunk to r_edi: the primitives of the chunk's terminals packed by apChunkInfo's compressor
// (none without one: the terminals are cleared), then apChunk's code, padded to 4 bytes, and its jump commands; returns
// the bytes added. apTerminals is the terminal array calcChunkPoints fills: the compressor receives the terminals and
// their count from it.
int32_t copyCodeAndEmbedTerminals(
    hkpMoppCompilerChunkInfo* apChunkInfo,
    hkpMoppCodeGenerator* apChunk,
    hkArray<uint32_t, hkContainerHeapAllocator>* apTerminals,
    hkpMoppCodeGenerator* r_edi
);

// What the assembler knows about one node while it assembles it: its depth, the node's extents in the code's integer
// space and the significant bits they need, the resolution, and the offsets and spreads of the node's terminals and
// properties.
class hkpMoppAssemblerNodeInfo {
    // The assembler calls both helpers itself: preCalcCutPositions and calcChunkPoints
    // call updateExtents and updateOffsetAndSpread.
    friend class hkpMoppDefaultAssembler;

public:
    // The default constructor is for the stack infos the assembler rebuilds from their parents.
    hkpMoppAssemblerNodeInfo() = default;
    hkpMoppAssemblerNodeInfo(
        const hkpMoppAssemblerNodeInfo& arParent,
        const hkpMoppTreeNode* apNode,
        const hkpMoppCode::CodeInfo& arCodeInfo
    );

protected:
    // The node's primitive id spread and its property values' offsets and spreads.
    void updateOffsetAndSpread(const hkpMoppTreeNode* node);
    // The node's extents scaled into the code's integer space by arCodeInfo's offset and scale.
    void updateExtents(const hkpMoppTreeNode* node, const hkpMoppCode::CodeInfo& codeInfo);

public:
    // The bits the largest extent needs, at most 24, into m_maxExtentsSignificantBits.
    void calcMaxExtentsSignificantBits();

    int32_t m_level;
    hkBool m_ignoreBandControlCode;
    // Set by calcMaxExtentsSignificantBits, clamped to 24.
    int32_t m_maxExtentsSignificantBits;
    int32_t m_extents[3][2];
    // The shift calcPossibleCutPlanes rounds the cut planes with; assemble stores 16.
    int32_t m_currentBitsResolution;
    int32_t m_accumOffset[3];
    int32_t m_terminalOffset;
    // updateOffsetAndSpread stores the node's primitive id spread here and the property values below.
    int32_t m_terminalSpread;
    int32_t m_numProperties;
    int32_t m_propertiesOffset[1];
    int32_t m_propertiesSpread[1];
};

// A node's info with the cut plane positions its extents allow, rounded to the resolution.
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

// Whether a node's code rescales the integer space, with the scale command and the offset per axis that go with it.
class hkpMoppAssemblerRescaleCommand {
public:
    hkBool m_doRescale;
    int32_t m_scaleCommand;
    int32_t m_moppOffset[3];
};

// Assembles a fitted MOPP tree into code, tracking chunk and splitting-plane state.
class hkpMoppDefaultAssembler : public hkpMoppAssembler {
public:
    hkpMoppDefaultAssembler(
        const hkpMoppAssembler::hkpMoppAssemblerParams& ap,
        hkpMoppCodeGenerator* a2,
        hkpMoppMediator* mediator
    );
    ~hkpMoppDefaultAssembler() override;

    int32_t getNumSplittingPlaneDirections() const override;

    const hkpMoppSplittingPlaneDirection* getSplittingPlaneDirections() const override {
        return m_directions.begin();
    }

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
    void addSplit(int32_t a1, int32_t a2, HK_MOPP_SPLIT_DIRECTIONS a3, int32_t a4, int32_t rpos);
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    int32_t assembleNonTerminalCommand(
        hkpMoppTreeNode* currentNode,
        const hkpMoppAssemblerNodeInfo& parentInfo,
        hkpMoppAssemblerNodeInfo& currentInfo,
        hkpMoppAssemblerRescaleCommand& rescaleCommandInfo
    );
    void addRescale(const hkpMoppAssemblerRescaleCommand& cmd);
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    int32_t preCalcScale(hkpMoppTreeNode* a1, const hkpMoppAssemblerNodeInfo& a2, hkpMoppAssemblerNodeInfo& a3);
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    void addCut(const hkpMoppTreeNode* a1, const hkpMoppAssemblerNodeInfo& a2);
    hkBool* shouldAssemble(hkBool* apResult, hkpMoppTreeNode* a2, const hkpMoppAssemblerNodeInfo& a3);
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    int32_t
    addTerminals(hkpMoppTreeTerminal* a1, const hkpMoppAssemblerNodeInfo& a2, const hkpMoppAssemblerNodeInfo& a3);
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    void calcTermCut(hkpMoppTreeNode* a1, hkpMoppAssemblerCutInfo& a2);
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    void initTopDown(
        hkpMoppTreeNode* currentNode,
        const hkpMoppAssemblerNodeInfo& parentInfo,
        hkpMoppAssemblerNodeInfo& currentInfo,
        hkpMoppAssemblerRescaleCommand& rescaleCommandInfo
    );
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    int32_t assembleCutAndTerminalCommand(
        hkpMoppTreeNode* a1,
        const hkpMoppAssemblerNodeInfo& a2,
        hkpMoppAssemblerNodeInfo& a3
    );
    // With the node not yet assembled: a terminal's cut planes and command, else the parent's and the node's scale and
    // cut positions along the plane. Only caller: assemble.
    void preCalcCutPositions(
        hkpMoppTreeNode* currentNode,
        const hkpMoppAssemblerCutInfo& parentInfo,
        hkpMoppAssemblerCutInfo& currentInfo
    );
    // Assembles the node and its two children recursively; returns the code size, 3 for a node already assembled, -1 on
    // failure.
    int32_t assemblesubNode(
        hkpMoppTreeNode* currentNode,
        const hkpMoppAssemblerNodeInfo& parentInfo,
        hkpMoppAssemblerNodeInfo& currentInfo
    );
    // Splits the subtree into chunks, appending to terminalsOut.
    int32_t calcChunkPoints(
        hkpMoppTreeNode* currentNode,
        const hkpMoppAssemblerNodeInfo& parentInfo,
        hkpMoppAssemblerNodeInfo& currentInfo,
        hkArray<uint32_t, hkContainerHeapAllocator>& terminalsOut
    );
    // Assembles the subtree, each chunk into its own code generator.
    void assembleSubNodeIntoChunks(
        hkpMoppTreeNode* currentNode,
        const hkpMoppAssemblerNodeInfo& parentInfo,
        hkpMoppAssemblerNodeInfo& currentInfo,
        int32_t chunkId,
        hkArray<uint32_t, hkContainerHeapAllocator>& terminalsOut
    );
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    void calcReoffset(hkpMoppTreeNode* a1, const hkpMoppAssemblerNodeInfo& a2, hkpMoppAssemblerNodeInfo& a3);
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    int32_t calcRescaleBits(const hkpMoppAssemblerNodeInfo& a1, hkpMoppAssemblerNodeInfo& a2);
    // Havok SDK signature (hkpInternal.lib, tied by reloc).
    void findPlanes(const hkpMoppTreeInternalNode* a1, const hkpMoppAssemblerNodeInfo& a2, int32_t& a3, int32_t& a4);
    // Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
    void recalcRescale(
        const hkpMoppAssemblerNodeInfo& parent,
        hkpMoppAssemblerNodeInfo& info,
        hkpMoppAssemblerRescaleCommand& cmd
    );

    // Returns the split code of the splitting direction a1 (its x, y, z against +-0.1). It only reads the direction
    // (assembleNonTerminalCommand passes the node's const plane).
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
    // m_codeInfo starts with an hkVector4 and m_params holds one, so both are 16-byte aligned and the compiler inserts
    // the padding before them itself.
    hkpMoppCode::CodeInfo m_codeInfo;
    hkArray<hkpMoppSplittingPlaneDirection, hkContainerHeapAllocator> m_directions;
    hkpMoppAssemblerParams m_params;
};
