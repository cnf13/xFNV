#pragma once

#include "BSCore/BSMap.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSDistantTreeBlockLoadTask;
class BGSTerrainNode;
class BSDistantTreeShaderProperty;
class BSResizableTriShape;
class NiStencilProperty;
class NiTriShapeData;
class TESWorldSpace;

// Writes the path of apWorld's distant tree block into apPath (260
// bytes): Data\Meshes\Landscape\LOD\<world>\Trees\<world>.Level<aiLevel>.X<aiX>.Y<aiY>.DTL, the world's name read
// twice through its vtable slot 0x130.
// Inferred type: apPath: char* from 0x6f88d0 (passed to BSsprintf 0); apWorld: TESWorldSpace* from 0x6f940e (parameter
// 2 of BGSDistantTreeBlockLoadTask::Construct), its members used at 0x6f88a4
void FormatTreeDTL(char* apPath, TESWorldSpace* apWorld, int32_t aiLevel, int16_t aiX, int16_t aiY);

class BGSDistantTreeBlock {
public:
	struct TreeGroup;

	// A kind of distant tree (the PDB's BGSDistantTreeBlock::TreeType) with its instance count.
	// The PC layout differs from the Xbox PDB's: the PDB's fHeight, u0, v0, u1, v1 and spSharedModelData have no
	// matching PC field by name.
	struct TreeType {
		TreeType();
		~TreeType();

		// The PDB's TreeType::operator= (0x6F9D90), under IDA's name: copies a2's fields into this and returns this.
		BGSDistantTreeBlock::TreeType* Equal(const BGSDistantTreeBlock::TreeType* a2);

		// Fills apTreeType's per-type values.
		static void InitValues(TreeType* apTreeType);
		static void operator delete(void* apMemory);

		uint32_t uiNumber;
		NiPoint2 kSize;
		NiPoint2 kPosMin;
		NiPoint2 kPosMax;
		// The geometry data the type's instances share (PDB NiPointer<NiTriShapeData> spSharedModelData; the
		// constructor 0x6F9060 and destructor 0x6F9080 run NiPointer's on +0x1C).
		NiPointer<NiTriShapeData> spSharedModelData;
	};

	// One distant tree placed in the block (IDA's names; the PDB has none).
	struct InstanceData {
		NiPoint3 Pos;
		float fRotZ;
		float fScale;
		uint32_t id;
		uint32_t uiGroup;
		bool bHidden;
	};

	// The trees of one type in the block: their shared geometry and instances.
	struct TreeGroup {
		TreeGroup();
		~TreeGroup();

		// Releases the group's geometry and instances.
		void Free();

		static void operator delete(void* apMemory);

		TreeType* pTreeType;
		NiPointer<BSResizableTriShape> spGeometry;
		BSSimpleArray<InstanceData> Instances;
		uint32_t uiNum;
		bool bShaderPropertyUpToDate;
		uint8_t padding1D[3];
	};

	using TreeTypes_t = BSSimpleArray<BGSDistantTreeBlock::TreeType, 1024>;

	BGSDistantTreeBlock(BGSTerrainNode* apNode, uint32_t auiLevel, int16_t asCellX, int16_t asCellY);
	~BGSDistantTreeBlock();

	// Shows the LOD tree with form ID aID again.
	void ShowTree(uint32_t aID);
	// Points apShaderProp at the group's geometry data.
	void SetupShaderProperty(BSDistantTreeShaderProperty* apProp, BGSDistantTreeBlock::TreeGroup& aGroup);
	// Drops the trees queued to be added.
	void ClearQueuedTrees();
	// Hides the tree aID; abAddToUIntArray records it in the hidden list.
	void HideTree(const uint32_t aID, bool abQueue);
	// Finishes loading the block's trees.
	void FinishLoad();
	void AddTree(const uint32_t uiType, const BGSDistantTreeBlock::InstanceData& apInstanceData);
	void ClearTrees();
	NiBound* CalculateBound(NiBound* apBound, BGSDistantTreeBlock::TreeGroup* apGroup);
	void Read(NiFile* apInputFile);
	void Attach();
	void UpdateShaderProperties();
	void HideQueuedTrees();

	// Loads apWorld's tree types and the tree texture atlas and creates the shared properties.
	static void AllocateStaticData(TESWorldSpace* apWorld);
	// Reads the tree types from apInputFile (cdecl, 0x6F8F80; IDA: InitTreeTypes).
	static void ReadTypes(NiFile* apInputFile);
	static uint32_t GetBlockCount();
	static int32_t GetMaxInstanceCount();
	static bool QStaticDataLoaded();
	static void operator delete(void* apMemory);
	static void DeallocateStaticData();
	static void AddType(const BGSDistantTreeBlock::TreeType& aType);

	// The PDB types this as NiPointer<IOTask>; the exe's constructor, assignment and QueueMe call are the ones src
	// names NiPointer<QueuedFile>::Construct (0x528CB0), operator= (0x6F74F0) and slot 8.
	NiPointer<QueuedFile> spLoadTask;
	BSSimpleArray<TreeGroup*> TreeGroups;
	BSMap<uint32_t, InstanceData*> InstanceMap;
	BSMap<uint32_t, TreeGroup*> NextGroup;
	BSSimpleArray<uint32_t> TreesQueuedToHide;
	BGSTerrainNode* pNode;
	bool bDoneLoading;
	bool bAttached;
	// PC has an additional visibility byte with no matching PDB member name.
	uint8_t padding4A[2];

	static NiPointer<NiSourceTexture> spAtlasTexture;
	// The stencil property every LOD tree shares (drawn from both sides).
	static NiPointer<NiStencilProperty> spSharedStencil;
	// The alpha property every LOD tree shares (alpha tested, not blended).
	static NiPointer<NiAlphaProperty> spSharedAlpha;
	static inline bool bStaticDataLoaded;
	// IDA name; the PDB names it uiBlockCount.
	static inline int32_t iCount;
	// The LOD level of the block created last.
	static inline uint32_t uiLevel;
	static TreeTypes_t TreeTypes;
};

// The tree groups' instance array: the exe's out-of-line constructor and destructor, defined in
// BGSDistantTreeBlock.cpp, and the empty-array setup the constructor calls.
using InstanceDataArray = BSSimpleArray<BGSDistantTreeBlock::InstanceData>;

// The block's containers: the exe's out-of-line constructors, which write the container's vtable and set it up empty
// (the maps with 0x25 buckets). Not reconstructed yet: each forwards to the exe's own body.
using TreeGroupArray = BSSimpleArray<BGSDistantTreeBlock::TreeGroup*>;
using TreeGroupMap = BSMap<uint32_t, BGSDistantTreeBlock::TreeGroup*>;
using InstanceDataMap = BSMap<uint32_t, BGSDistantTreeBlock::InstanceData*>;
using UIntArray = BSSimpleArray<uint32_t>;

// The static tree-type array (the PDB's global BGSDistantTreeBlock::TreeTypes): the exe's out-of-line Clear, which
// IDA names BGSDistantTreeBlock::TreeTypes::FreeAll, and the item destructor it calls (defined in
// BGSDistantTreeBlock.cpp).
using TreeTypeArray = BSSimpleArray<BGSDistantTreeBlock::TreeType>;
// The queued task that loads one distant tree block (a QueuedFileEntry).
class BGSDistantTreeBlockLoadTask : public QueuedFileEntry {
public:
	BGSDistantTreeBlockLoadTask(BGSDistantTreeBlock* apBlock, TESWorldSpace* apWorld, int32_t aiX, int32_t aiY,
		int32_t aiLevel, bool abPostApocalypse);
	~BGSDistantTreeBlockLoadTask() override;

	void Run() override;
	void Finish() override;
	void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
	void QueueMe() override;
	void CheckFinished() override;
	uint8_t GetFileIndex() override;

	static void operator delete(void* apMemory);

	BGSDistantTreeBlock* pBlock;
	bool bFinishedQueuing;
	// IDA: _BYTE[3] gap35.
	uint8_t padding35[3];
	int32_t iX;
	int32_t iY;
	int32_t iLevel;
	bool bCanceled;
	bool bFileExists;
	uint8_t padding46[2];
};

inline constexpr fnvd::Vtable BGSDistantTreeBlockLoadTask_vftable{};

// The game settings BGSDistantTreeBlock.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> uDistantTreeBlockCacheSizePerCell;
