#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BSInstanceRendering;
class BSShader;
class PersistentPassList;

int32_t ComparePasses(uint16_t pass1, uint16_t pass2);

// A list of render passes that keeps its freed nodes for reuse
// (BSTPersistentList<NiTPointerAllocator<unsigned int>, BSShaderProperty::RenderPass*>).
class PersistentPassList {
public:
	// One node; freed nodes are chained through m_pkNext from pNextFreeItem.
	struct ListItem {
		ListItem* m_pkNext;
		ListItem* m_pkPrev;
		BSShaderProperty::RenderPass* m_element;
	};

	// An allocator that keeps the count in its tail: the empty pool allocator adds no size.
	struct AntiBloatAllocator : public NiTPointerAllocator<uint32_t> {
		uint32_t uiCount;
	};

	PersistentPassList();
	virtual ~PersistentPassList();

	void CleanUnusedNodes();
	// Appends *apRenderPass at the front, taking a node from the free chain or the allocator.
	void AddFront(BSShaderProperty::RenderPass** apRenderPass);
	void AddBack(BSShaderProperty::RenderPass** apRenderPass);

	static void operator delete(void* apMemory);

	ListItem* m_pkHead;
	ListItem* m_pkTail;
	ListItem* pNextFreeItem;
	AntiBloatAllocator Allocator;
};

// Collects the render passes of one batch of geometry that is drawn together.
class BSBatchRenderer : public NiObject {
public:
	// The geometry groups a pass can be registered to (pGeometryGroups' index).
	enum GEOMETRY_GROUP_ENUM : int32_t {
		GG_LOD = 0,
		GG_NO_DEPTH = 1,
		GG_STATIC_DECAL = 2,
		GG_DYNAMIC_DECAL = 3,
		GG_STATIC_DECAL_WIREFRAME = 4,
		GG_DYNAMIC_DECAL_WIREFRAME = 5,
		GG_REFRACTIVE = 6,
		GG_VELOCITY = 7,
		GG_1ST_PERSON_NONREFRACTIVE = 8,
		GG_FADING = 9,
		GG_IMMEDIATE = 10,
		GG_RENDER_DEPTH = 11,
		GG_RENDER_DEPTH_1STPERSON = 12,
		GG_COUNT = 13,
	};

	// The pass lists of one render pass, one per transparency type.
	struct PassGroup {
		PassGroup();

		// Empties every pass list; abReportNotEmpty asks to report a list that was not empty.
		void Clear(bool abReportNotEmpty);
		void AddHead(int32_t aiBucket, BSShaderProperty::RenderPass* apRenderPass);
		void AddTail(int32_t aiBucket, BSShaderProperty::RenderPass* apRenderPass);

		PersistentPassList pListA[5];
		int32_t iSize;
	};

	// Passes that were registered for the same geometry group.
	struct GeometryGroup {
		explicit GeometryGroup(uint32_t auiPassCount);
		~GeometryGroup();

		// Appends a render pass to the group's list. abAddToFront is a flag, but callers (RegisterPassGeometryGroup)
		// pass a whole 32-bit value on, so it stays uint32_t.
		void RegisterPass(BSShaderProperty::RenderPass* apRenderPass, uint32_t abAddToFront);
		void ClearPasses();
		// Renders the group's passes. abAlphaPass is an int32_t, not a bool, so the caller's whole value is passed on;
		// a bool would be zero-extended.
		void Render(int32_t abAlphaPass);

		uint8_t ucFlags;
		uint8_t padding01;
		uint16_t usCount;
		uint32_t uiPassCount;
		BSBatchRenderer* pBatchRenderer;
		PersistentPassList PassList;
		float fDepth;
	};

	BSBatchRenderer(uint32_t auiPassCount, int32_t a2);
	~BSBatchRenderer() override;

	const NiRTTI* GetRTTI() const override { return &BSBatchRenderer::ms_RTTI; }
	// auiPassEnum: BSShaderManager::RenderPassType
	virtual void RegisterPass(BSShaderProperty::RenderPass* apRenderPass, uint32_t auiPassEnum);
	// abAlphaPass is int32_t rather than bool because the caller's whole value is passed on; a bool would be
	// zero-extended.
	virtual void RenderBatches(BSShaderManager::eBSSMPassEnum aePass, BSShaderManager::eBSSMPassEnum aePassEnd,
		int32_t abAlphaPass);

	// Finds the first transparency type of the pass group that has passes queued.
	bool PrepareNextRenderPass_SetNextBucket(uint32_t auiIndex, int32_t& aiBucket);

	void ClearRenderPasses(bool abReportUnrendered);

	// Takes the next active pass in the current range off the active list and finds its first non-empty bucket.
	bool PrepareNextRenderPass_SetNextPass(uint32_t& auiPass, int32_t& aiBucket);
	// Advances to the next non-empty bucket of the pass, else to the next pass; reports the head's first-pass flag.
	bool PrepareNextRenderPass(uint32_t& auiPass, int32_t& aiBucket, bool& abFirstPass);
	// Draws the head passes of the bucket that share the head's first-pass flag, then prepares the next one.
	bool RenderNextRenderPass(uint32_t& auiPass, int32_t& aiBucket, bool& abFirstPass, int32_t abAlphaPass);
	bool QWaterRefractionPassWithinRange(BSShaderManager::eBSSMPassEnum aeStartRange,
		BSShaderManager::eBSSMPassEnum aeEndRange);

	void RegisterAlphaPass(BSShaderProperty::RenderPass* apRenderPass, uint32_t aiIndex);
	// Appends apRenderPass to geometry group aeGroup, at the front with abAddToFront. abAddToFront is an int32_t
	// because the caller's whole 32-bit value is passed on; a bool would be zero-extended.
	void RegisterPassGeometryGroup(BSShaderProperty::RenderPass* apRenderPass, GEOMETRY_GROUP_ENUM aeGroup,
		int32_t abAddToFront);
	void SortAlphaPasses(const NiCamera* apCamera, bool abGetSortByClosestPoint);
	float GetNextAlphaDepth(uint32_t aiIndex, const NiCamera* apCamera, bool abGetSortByClosestPoint);
	void StartGroupingAlphas(const NiBound& aBound, bool abZPrePass, const NiCamera* apCamera,
		bool abGetSortByClosestPoint);
	void StopGroupingAlphas();
	void ClearAlphaLists(int32_t aiSide);
	bool SkipNextRenderPass(uint32_t& auiPass, int32_t& aiBucket, bool& abFirstPass, bool abAlphaPass);
	void RenderNextAlpha(uint32_t aiIndex, const NiCamera* apCamera, bool abGetSortByClosestPoint, float afStopDepth);

	// Ends the pass being batched: restores the last shader's pass states.
	static void EndPass();
	// uicurrentPass is a BSShaderManager::RenderPassType. The three flags are int32_t rather than bool because each
	// caller's whole value is passed on to RenderPassImmediately_Skinned/_Standard.
	static void RenderPassImmediately(BSShaderProperty::RenderPass* prenderpass, uint32_t uicurrentPass,
		int32_t balphaTest, int32_t abAlphaPass, int32_t abGeometryRenderStates);
	// Renders every pass of a persistent list that has geometry, then returns its nodes to the free chain. Passes the
	// caller's whole abAlphaPass value on.
	static void RenderPersistentPassList(PersistentPassList& arPassList, int32_t abAlphaPass);
	static void RenderPassImmediately_Standard(BSShaderProperty::RenderPass* apRenderPass, int32_t abAlphaTest,
		int32_t abAlphaPass, int32_t abGeometryRenderStates);
	static void RenderPassImmediately_Skinned(BSShaderProperty::RenderPass* apRenderPass, int32_t abAlphaTest,
		int32_t abAlphaPass, int32_t abGeometryRenderStates);
	static void BeginPass(uint32_t auiPass, BSShader* apShader);

	PassGroup* pRenderPassA;
	NiTMap<uint32_t, uint32_t> RenderPassMap;
	uint32_t uiRenderPassCount;
	uint32_t uiCurrentFirstPass;
	uint32_t uiCurrentLastPass;
	BSInstanceRendering* pInstanceRenderer;
	BSSimpleList<uint16_t> kActivePassIndexList;
	int32_t iGroupingAlphas;
	bool bAlphaGroupZPrePass;
	uint8_t padding39[3];
	GeometryGroup* pGeometryGroups[GG_COUNT];
	GeometryGroup* pAlphaGroups[2];
	// The depth-sorted alpha groups of each side.
	NiTPointerList<GeometryGroup*> kAlphaGroupLists[2];
	GeometryGroup* pCurrentAlphaGroup[2];

	static NiRTTI ms_RTTI;
	static inline bool bSortByClosestPoint;
	// The camera SortAlphaPasses was given; the depth sort only reads it.
	static inline const NiCamera* pSortCamera{};
	static inline bool bFirstPass = true;
	static inline BSShader* pLastShader;
	static inline int32_t uiLastPass;
	static inline bool bPIXEventStarted;
};

NiSmartPointer(BSBatchRenderer);

// The names of the four standard pass kinds.
using pBucketTypeNameA_t = const char* [4];
inline pBucketTypeNameA_t pBucketTypeNameA{"Standard", "TwoSidedLighting", "TwoSidedLighting+AlphaTest", "AlphaTest"};
