#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamFrameInfo.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkOstream;

// The timer names of a monitor stream by the address the stream stores them under. The layout is the exe's hkClasses
// (0x126C830, built by 0xFC38C0: size 0xC, m_map at 0; StringMap 0x126C800, built by 0xFC3890: size 0x10, m_id at 0
// with the 8-byte alignment flag, m_string at 8), as the PC PDB's.
struct hkMonitorStreamStringMap {
	struct StringMap {
		StringMap();
		// A loaded entry keeps what was loaded.
		StringMap(hkFinishLoadedObjectFlag flag) : m_string(flag) {}

		static const hkClass& staticClass();

		alignas(8) hkUint64 m_id;
		hkStringPtr m_string;
	};

	hkMonitorStreamStringMap();
	// A loaded map keeps what was loaded.
	hkMonitorStreamStringMap(hkFinishLoadedObjectFlag flag) : m_map(flag) {}

	static const hkClass& staticClass();

	hkArray<StringMap, hkContainerHeapAllocator> m_map;
};

// The colours the thread drawing gives the timers, looked up by timer name; m_defaultColor for any other name. The
// layout is the exe's hkClass (0x126C8C0, built by 0xFC3970: size 0x18, members at 0x8 and 0x14), as the PC PDB's.
struct hkMonitorStreamColorTable : public hkReferencedObject {
	// One timer name and its colour (the exe's hkClass 0x126C890: size 0x8).
	struct ColorPair {
		static const hkClass& staticClass();

		hkStringPtr m_colorName;
		uint32_t m_color;
	};

	hkMonitorStreamColorTable() = default;
	// The SDK's finish constructor: the loaded table keeps its fields.
	hkMonitorStreamColorTable(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_colorPairs(aFlag) {}
	~hkMonitorStreamColorTable() override;

	// Appends a name and its colour to m_colorPairs.
	// Inferred type: apColorName: char* from 0xe0d98b (passed to hkStringPtr::Construct 1)
	void addColorPair(const char* apColorName, uint32_t aColor);
	void setupDefaultColorTable();
	uint32_t findColor(const char* color);

	static const hkClass& staticClass();

	hkArray<ColorPair, hkContainerHeapAllocator> m_colorPairs;
	uint32_t m_defaultColor;
};

// Stores captured monitor streams and controls their frame/thread presentation.
class hkMonitorStreamAnalyzer {
public:
	// Values from the Xbox PDB.
	enum : int32_t {
		REPORT_ALL = 0xf7,
		REPORT_SUMMARIES = 0x7,
		REPORT_TOTAL = 0x1,
		REPORT_PERFRAME_TIME = 0x2,
		REPORT_PERFRAME_SUMMARY = 0x4,
		REPORT_PERFRAME_PERTYPE = 0x8,
		REPORT_PERFRAME_DETAIL = 0x10,
	};

	// The arrow keys navigateMonitors moves the active node with (PC PDB: size 4).
	struct CursorKeys {
		hkBool m_upPressed;
		hkBool m_downPressed;
		hkBool m_leftPressed;
		hkBool m_rightPressed;
	};

	// One timer of the statistics tree and its children. The layout is the PC PDB's; the exe's constructor
	// (0xE0D000) stores m_children at 0x30, m_parent 0x48, m_name 0x4C, m_userFlags 0x50, m_type 0x54, clears the
	// 8 floats from 0 and the 8 counts from 0x20 and m_absoluteStartTime (a double) at 0x40. No vtable.
	struct Node {
		// Values from the Xbox PDB.
		enum NodeType : int32_t {
			NODE_TYPE_TIMER = 0x0,
			NODE_TYPE_DIRECTORY = 0x1,
			NODE_TYPE_SINGLE = 0x2,
		};

		Node(Node* apParent, const char* apcName, NodeType aeType);
		~Node();

		void setTimers(const hkMonitorStreamFrameInfo& frameInfo, const hkMonitorStream::TimerCommand& start,
			const hkMonitorStream::TimerCommand& end);

		// The deleting destructor's free: the heap allocator, by size.
		static void operator delete(void* apMemory, size_t aSize);

		// hkReal, one per timer of the frame.
		float m_value[8];
		uint16_t m_count[8];
		hkArray<Node*, hkContainerHeapAllocator> m_children;
		double m_absoluteStartTime;
		Node* m_parent;
		const char* m_name;
		uint32_t m_userFlags;
		hkEnum<NodeType, uint32_t> m_type;
	};

	struct ThreadDrawInput {
		static int32_t computePerThreadHeightToFit(int32_t textureHeight, int32_t numFrames, int32_t numThreads,
			int32_t frameGap, int32_t threadGap);

		int32_t m_frameStart;
		int32_t m_numFrames;
		int32_t m_heightPerThread;
		int32_t m_gapBetweenThreads;
		int32_t m_gapBetweenFrames;
		int32_t m_outputPixelWidth;
		float m_limitFrameTime;
		float m_limitStartTime;
		hkBool m_warnAboutMissingTimers;
		uint8_t padding21[3];
		hkMonitorStreamColorTable* m_colorTable;
	};

	// How showCombinedThreadSummary lays out the timer tree: column positions, whether only the expanded part of
	// the tree is shown (with the arrow characters and the active node marked), tabs or spaces between columns.
	struct CombinedThreadSummaryOptions {
		int32_t m_indentationToFirstTimerValue;
		int32_t m_timerColumnWidth;
		int32_t m_tabSpacingForTimerNames;
		int32_t m_tabSpacingForTimerValues;
		hkBool m_displayPartialTree;
		Node* m_activeNode;
		char m_rightArrowChar;
		char m_downArrowChar;
		hkBool m_useTabsNotSpacesForColumns;
	};

	// What the texture writer reports back: the longest sample time it drew.
	struct SampleInfo {
		float m_maxSampleTime;
	};

	hkMonitorStreamAnalyzer(int32_t memorySize, int32_t numThreads, int32_t numSpus);

	void checkAllThreadsCapturedSameNumFrames() const;
	void drawThreadsToTga(const hkMonitorStreamAnalyzer::ThreadDrawInput& input, hkOstream& outStream);
	char* getStreamBegin();
	hkBool* captureFrameDetails(hkBool* apResult, const char* monitorStreamBegin, const char* monitorStreamEnd,
		hkMonitorStreamFrameInfo& info);
	void writeRawStatistics(hkOstream& os);
	// The statistics tree of every frame of thread threadId, under a new "/" root (the PDB's QAA: a member whose
	// `this` is the first stack argument; cdecl, ret 0, 0xE0EE00).
	hkMonitorStreamAnalyzer::Node* makeStatisticsTreeForMultipleFrames(int32_t threadId, hkBool reuseNodesIfPossible);
	void writeStatistics(hkOstream& outStream, int32_t reportLevel);
	void reset();
	void resetNumThreads(int32_t numThreads, int32_t numSpus);

	// Writes node's line (the arrows and indentation, its name with its total count, then each thread's value and
	// count and the per-group averages) and recurses into its children.
	static void showCombinedThreadSummaryRec(hkOstream& os, Node* node, int32_t recursionDepth, int32_t numThreads,
		int32_t numSpus, const CombinedThreadSummaryOptions& options);
	// Scales mainTree's timer destTimerId (counts cleared) by smoothingFactor, then adds subTree's timer
	// sourceTimerId times 1 - smoothingFactor into the matching nodes, adding missing ones (cdecl, 0xE0D280). The
	// eax the merge leaves is read by no caller (0xE0FEC0).
	static void mergeTreesForCombinedThreadSummary(Node* mainTree, Node* subTree, int32_t destTimerId,
		int32_t sourceTimerId, float smoothingFactor);
	// The header line of the combined thread summary, then node's tree under it.
	static void showCombinedThreadSummaryForSingleFrame(hkMonitorStreamAnalyzer::Node* node, int32_t numThreads,
		int32_t numSpus, hkOstream& os, hkMonitorStreamAnalyzer::CombinedThreadSummaryOptions& options);
	static void applyStringMap(const char* frameStart, const char* frameEnd,
		const hkPointerMap<const void*, const char*, hkContainerHeapAllocator>& map, hkBool endianSwap);
	static hkMonitorStreamAnalyzer::Node* navigateMonitors(const hkMonitorStreamAnalyzer::CursorKeys& keys,
		hkMonitorStreamAnalyzer::Node* activeNodeIn);
	static hkMonitorStreamAnalyzer::Node* reverseLookupNodeAtTgaSample(int32_t x, int32_t y,
		const hkArrayBase<hkMonitorStreamAnalyzer::Node*>& nodeList,
		const hkMonitorStreamAnalyzer::ThreadDrawInput& input,
		hkArray<hkArray<hkMonitorStreamFrameInfo, hkContainerHeapAllocator>, hkContainerHeapAllocator>& frameInfos);
	static void writeStatisticsDetails(hkOstream& outstream, hkArrayBase<hkMonitorStreamAnalyzer::Node*>& nodes,
		int32_t numThreads, int32_t numSpus, int32_t reportLevel, const char* nodeIdForFrameOverview,
		bool showMultithreadedSummary);
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	static void extractStringMap(const char* frameStart, const char* frameEnd,
		hkPointerMap<const void*, const char*, hkContainerHeapAllocator>& map);
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	static void getTimerLimits(const hkArrayBase<hkMonitorStreamAnalyzer::Node*>& nodeList,
		const hkMonitorStreamAnalyzer::ThreadDrawInput& input,
		hkArray<hkArray<hkMonitorStreamFrameInfo, hkContainerHeapAllocator>, hkContainerHeapAllocator>& frameInfos,
		float& maxFrameTime, hkArray<float, hkContainerHeapAllocator>& startTimes);
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	static hkMonitorStreamAnalyzer::Node* makeStatisticsTreeForSingleFrame(const char* frameStart, const char* frameEnd,
		const hkMonitorStreamFrameInfo& frameInfo, const char* rootNodeName, hkBool reuseNodesIfPossible);
	// The widest timer name of node's tree, indented by spacing per recursion level
	// (0xE0C860: reads m_name +0x4C, m_userFlags +0x50, m_children +0x30).
	// The exe passes the caller's whole dword on; bool would be zero-extended.
	static int32_t findMaxTimerNameIndent(hkMonitorStreamAnalyzer::Node* node, int32_t recursionDepth, int32_t spacing,
		int32_t displayPartialTree);
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	static void writeStatisticsDetailsToTexture(const hkArrayBase<hkMonitorStreamAnalyzer::Node*>& nodeList,
		const hkMonitorStreamAnalyzer::ThreadDrawInput& input,
		hkArray<hkArray<hkMonitorStreamFrameInfo, hkContainerHeapAllocator>, hkContainerHeapAllocator>& frameInfos,
		int32_t*& texture, int32_t& height, hkMonitorStreamAnalyzer::SampleInfo* sampleInfo);
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	static void writeStatisticsDetailsToTga(const hkArrayBase<hkMonitorStreamAnalyzer::Node*>& nodeList,
		const hkMonitorStreamAnalyzer::ThreadDrawInput& input,
		hkArray<hkArray<hkMonitorStreamFrameInfo, hkContainerHeapAllocator>, hkContainerHeapAllocator>& frameInfos,
		hkOstream& outStream, hkMonitorStreamAnalyzer::SampleInfo* sampleInfo);

	hkArray<hkArray<hkMonitorStreamFrameInfo, hkContainerHeapAllocator>, hkContainerHeapAllocator> m_frameInfos;
	hkArray<char, hkContainerDebugAllocator> m_data;
	int32_t m_numThreads;
	int32_t m_numSpus;
	const char* m_nodeIdForFrameOverview;

	static inline float g_lastFrameTime = 16666.6f;
};

// One timer type of the per-type statistics: the sum of every node of that name, and the nodes it was found under
// and above. The SDK defines it in hkMonitorStreamAnalyzer.cpp (hkbase.pdb: sizeof 112, m_parents at 88,
// m_children at 100); the exe's 0xE0F880 allocates 112 bytes from the debug allocator, constructs m_this and grows
// the arrays at 0x58 and 0x64 by 88-byte nodes.
struct hkMonitorStreamAnalyzerPrintByTypeInfo {
	~hkMonitorStreamAnalyzerPrintByTypeInfo();

	hkMonitorStreamAnalyzer::Node m_this;
	hkArray<hkMonitorStreamAnalyzer::Node, hkContainerDebugAllocator> m_parents;
	hkArray<hkMonitorStreamAnalyzer::Node, hkContainerDebugAllocator> m_children;
};

// The smaller of a and b, b when they are equal (cdecl, ret 0, 0xE0C260; the result in ST(0)).
// The PDB's static (S_LPROC32) of hkMonitorStreamAnalyzer.cpp, float (float, float).
float __hkMin(float a, float b);
// Finds apNode's child called apcName, else with abRecursive its first child; the exe passes
// apNode in edi, apcName and abRecursive on the stack (caller-popped), and tests only abRecursive's byte
// (0xE0CB80).
hkMonitorStreamAnalyzer::Node* hkFindChildByName(hkMonitorStreamAnalyzer::Node* node, const char* childName,
	hkBool searchAnyChild);
// The smallest power of two not below n (IDA's mangled name ?hkRoundUpPow2@@YAII@Z, cdecl by
// its mangling; the exe takes n in ecx and pops nothing, 0xE0C2E0).
uint32_t hkRoundUpPow2(uint32_t n);
// Sums the children of apNode into the matching nodes of apSum, recursively.
void hkMakeSumRecursive(hkMonitorStreamAnalyzer::Node* sum, hkMonitorStreamAnalyzer::Node* tree);
// Not named in the PDB or IDA; name chosen here. Finds apNode's child called apcName from
// *apIndexInOut on (the hinted index first, then from 0), storing its index there; the bool is al. The exe passes
// apNode in edi (0xE0C690).
// Inferred type: apNode: hkMonitorStreamAnalyzer::Node* from 0xe0d1e0 (parameter 0 of hkMergeSubTreeTimers), its
// members used at 0xe0c69d; apcName: char* from 0xe0c6ae (passed to hkString::strCmp 1)
bool hkFindChildIndexByName(hkMonitorStreamAnalyzer::Node* apNode, const char* apcName, int32_t* apIndexInOut);
// Multiplies timer aiTimerId of apNode and its
// descendants by afFactor and clears their counts.
// Inferred type: apNode: hkMonitorStreamAnalyzer::Node* from 0xe0d294 (parameter 0 of
// hkMonitorStreamAnalyzer::mergeTreesForCombinedThreadSummary), its members used at 0xe0c716; afFactor: float from
// 0xe0d294 (parameter 4 of hkMonitorStreamAnalyzer::mergeTreesForCombinedThreadSummary)
void hkScaleTimersRecursive(hkMonitorStreamAnalyzer::Node* apNode, int32_t aiTimerId, float afFactor);
// Adds timer aiSourceTimerId of each child of apSubTree times
// 1 - afSmoothingFactor into timer aiDestTimerId (at most 6) of apMainTree's child of the same name, allocating it
// when missing, copies the count, and recurses; returns apSubTree's last child count.
// Inferred type: apMainTree: hkMonitorStreamAnalyzer::Node* from 0xe0d2b0 (parameter 0 of
// hkMonitorStreamAnalyzer::mergeTreesForCombinedThreadSummary), its members used at 0xe0d223; apSubTree:
// hkMonitorStreamAnalyzer::Node* from 0xe0d2b0 (parameter 1 of
// hkMonitorStreamAnalyzer::mergeTreesForCombinedThreadSummary), its members used at 0xe0d198; afSmoothingFactor: float
// from 0xe0d2b0 (parameter 4 of hkMonitorStreamAnalyzer::mergeTreesForCombinedThreadSummary)
int32_t mergeSubTree(hkMonitorStreamAnalyzer::Node* apMainTree, hkMonitorStreamAnalyzer::Node* apSubTree,
	int32_t aiDestTimerId, int32_t aiSourceTimerId, float afSmoothingFactor);
// Inferred type: return: float from 0xe0c2ae (parameter 1 of __hkMax); a1: float: used as one at 0xe0c290; a2: float:
// used as one at 0xe0c294
float __hkMax(float a1, float a2);
void hkWriteRec(hkOstream& outstream, hkMonitorStreamAnalyzer::Node* node, int32_t RecDepth, float factor);
hkMonitorStreamAnalyzer::Node* getNodeAtSample(hkMonitorStreamAnalyzer::Node* currentNode,
	hkMonitorStreamAnalyzer::Node* lastHit, double sampleTime, int32_t absoluteTimeIndex);
void cleanupLoadedObjecthkMonitorStreamColorTable(void* p);
// Usercall (0xE0FC50, ret 0): outstream in edi (hkMonitorStreamAnalyzer::writeStatisticsDetails sets it at 0xE10394
// and 0xE10505), rootNode and maxImportance on the stack.
void printStatisticsByType(hkOstream& outstream, hkMonitorStreamAnalyzer::Node* rootNode, float maxImportance);
// Adds node's times and counts into sum. Usercall (0xE0BF50, ret 0): node in ecx (read),
// sum in eax (written).
void hkMakeSum(const hkMonitorStreamAnalyzer::Node* node, hkMonitorStreamAnalyzer::Node* sum);
// Adds node's timers into its name's entry of mapOut, creating the entry from the debug allocator on first sight,
// then recurses into the children; each child's and the parent's timers also go into the entry's child or parent
// node of that name.
void hkBuildSizePerName(const hkMonitorStreamAnalyzer::Node* node,
	hkStringMap<hkMonitorStreamAnalyzerPrintByTypeInfo*, hkContainerHeapAllocator>& mapOut);
// The per-type statistics' sort order: true when b's second timer is above a's (cdecl, 0xE10740; the exe returns
// 0 or 1 in all of eax).
bool hkCompareInfosBySize(const hkMonitorStreamAnalyzerPrintByTypeInfo* a,
	const hkMonitorStreamAnalyzerPrintByTypeInfo* b);
// Not named in the PDB or IDA; name chosen here. Fills apSamples (a Node* array on the heap) with the node of
// apRoot's tree under each of aiNumSamples sample times, afSampleWidth apart from afStartTime on (timer aiTimer):
// null before the first child starts; sets apRoot's start time and timer to its children's span. The exe passes
// aiNumSamples in eax, apRoot in edi and apSamples in esi; the rest on the stack (caller-popped; 0xE0CEC0).
void outputStatsForFrame(float afStartTime, float afSampleWidth, int32_t aiTimer, int32_t aiNumSamples,
	hkMonitorStreamAnalyzer::Node* apRoot,
	hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator>* apSamples);
// Not named in the PDB or IDA; name chosen here. The node of frame aiFrame of a thread's frames (apFrames, the thread
// root's children) at pixel column aiX of aiWidth columns spanning afMaxFrameTime from afStartTime on, or null
// (outputStatsForFrame per column). The exe passes apInfo (the thread's frame info) in eax, the rest on the stack
// (caller-popped; 0xE0DA00).
hkMonitorStreamAnalyzer::Node* getNodeSampledAtTick(int32_t aiFrame, int32_t aiX,
	const hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator>* apFrames, int32_t aiWidth,
	float afMaxFrameTime, float afStartTime, const hkMonitorStreamFrameInfo* apInfo);
// Not named in the PDB; name chosen here (IDA's hkArray<unsigned int, hkContainerHeapAllocator>::hkArray does not
// fit its code). Sets apPrefix up as writeRawStatistics' (0xE0EEF0) line prefix: an empty char array on the debug
// allocator with 64 bytes reserved, holding the terminating NUL (thiscall, ret 0, returns apPrefix; 0xE108B0).
hkArray<char, hkContainerDebugAllocator>* hkInitIndentString(hkArray<char, hkContainerDebugAllocator>* apPrefix);
// A child of parent called name: an existing one when wantNodeReuse, else a new node of the
// given type from the heap. The exe passes parent in edi, the rest on the stack (caller-popped), and tests only
// wantNodeReuse's byte (0xE0D100).
hkMonitorStreamAnalyzer::Node* createNewNode(const char* name, hkMonitorStreamAnalyzer::Node::NodeType type,
	bool wantNodeReuse, hkMonitorStreamAnalyzer::Node* parent);
// The next child of node after oldChild called childName, else null. Usercall (0xE0CBE0,
// ret 0): oldChild in edx (only compared), childName on the stack, node in edi (its children at +0x30, +0x34).
hkMonitorStreamAnalyzer::Node* hkFindNextChildByName(const hkMonitorStreamAnalyzer::Node* oldChild,
	const char* childName, hkMonitorStreamAnalyzer::Node* node);
// Finishes a loaded hkMonitorStreamStringMap with its finish constructor (cdecl, 0xDFDEF0; hkxAttributeGroupClass.cpp).
void finishLoadedObjecthkMonitorStreamStringMap(void* p, int32_t finishing);

extern hkClass hkMonitorStreamStringMapStringMapClass;
extern hkClass hkMonitorStreamColorTableColorPairClass;
extern hkClass hkMonitorStreamColorTableClass;
extern hkTypeInfo hkMonitorStreamColorTableTypeInfo;
inline constexpr fnvd::Vtable hkMonitorStreamColorTable_vftable{};
using rdata_10EAA5C_t = const char[4];
inline rdata_10EAA5C_t rdata_10EAA5C{"\t\t"};

// Guessed bound: the database gives none; the next data item starts 0x30 bytes on.
extern hkClass hkMonitorStreamStringMapClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkMonitorStreamStringMapStringMapTypeInfo;
extern hkTypeInfo hkMonitorStreamStringMapTypeInfo;
extern hkTypeInfo hkMonitorStreamColorTableColorPairTypeInfo;
