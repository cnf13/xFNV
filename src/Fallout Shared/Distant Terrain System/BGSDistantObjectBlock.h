#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/eBool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSDistantObjectBlockLoadTask;
class BGSTerrainNode;
class BSMultiBoundNode;
class NiAVObject;
class NiGeometry;
class TESWorldSpace;

class BGSDistantObjectBlock {
public:
	BGSDistantObjectBlock(BGSTerrainNode* apNode, const int32_t aiLevel, int16_t asCellX, int16_t asCellY,
		bool abLoadLowPriority);
	~BGSDistantObjectBlock();

	// Queues the post-apocalypse version of the block's mesh.
	void TriggerApocalypse(TESWorldSpace* apWorld, const int32_t aiLevel, int16_t asCellX, int16_t asCellY);
	// Cancels the block's loads and queues its high-priority mesh.
	void SwitchToHigh(TESWorldSpace* apWorld, const int32_t aiLevel, int16_t asCellX, int16_t asCellY);
	// Whether this block is in the global block list.
	bool IsInList();
	void EnableAllStingerVisibility(NiAVObject* apGeomNode);
	// Whether the block is flagged to detach.
	bool CheckDetach();
	// Appends this block to BlocksLoaded.
	void AddToList();
	BSMultiBoundNode* GetNode(bool abStingerVersion);
	// The block's mesh: an empty pointer for the stinger version, else the loaded mesh.
	NiGeometry* GetMesh(bool abStingerVersion);
	bool GetHighPriority();
	void FlagDetach();
	void UpdateStingerVisibility(NiAVObject* apGeomNode, int32_t aiLODLevel, int16_t aiBaseCellx, int16_t aiBaseCelly);
	void RemoveFromList();
	void Prepare();
	void PrepareStingerObject(NiAVObject* apGeom);
	void Detach();
	void Attach();
	void SwitchToLow(TESWorldSpace* apWorld, const int32_t aiLevel, const int16_t asCellX, const int16_t asCellY);
	void LoadFromTask(eBool abLoadData);

	static void EnableStingerRenderAll(NiAVObject* apGeom, bool abEnable);
	// The block's mesh path under the world's LOD folder, written into the 0x104-byte apOutput.
	static void BuildBlockFileName(char* apOutput, TESWorldSpace* apWorld, int32_t aiLevel, int16_t asCellX,
		int16_t asCellY);
	static void BuildBlockFileNameHighPriority(char* apOutput, TESWorldSpace* apWorld, int32_t aiLevel, int16_t asCellX,
		int16_t asCellY);
	static void BuildBlockFileNamePA(char* apOutput, TESWorldSpace* apWorld, int32_t aiLevel, int16_t asCellX,
		int16_t asCellY);
	static void BuildStingerBlockFileName(char* apOutput, TESWorldSpace* apWorld, int32_t aiLevel, int16_t asCellX,
		int16_t asCellY);
	static void BuildStingerBlockFileNameHighPriority(char* apOutput, TESWorldSpace* apWorld, int32_t aiLevel,
		int16_t asCellX, int16_t asCellY);
	static void operator delete(void* apMemory);
	static void ResetCullResults();

	BGSTerrainNode* pNode;
	NiPointer<NiGeometry> spMesh;
	NiPointer<BSMultiBoundNode> spNode;
	NiPointer<NiGeometry> spOldMesh;
	NiPointer<BSMultiBoundNode> spNodeStinger;
	NiPointer<BSMultiBoundNode> spNodeStingerOld;
	// The PDB's NiPointer<BGSDistantObjectBlockLoadTask> (type 0x2B6E8); the exe calls the task's GetMesh (0x6FAA30)
	// and GetNode (0x6F6250) through them and assigns them through the shared 0x6F74F0.
	NiPointer<BGSDistantObjectBlockLoadTask> spLoadTask;
	NiPointer<BGSDistantObjectBlockLoadTask> spLoadTaskStinger;
	bool bDoneLoading;
	bool bAttached;
	bool bPrepared;
	bool bHighPriority;
	bool bDetachFlag;
	uint8_t padding25[3];
	int32_t nDetachCount;

	// The loaded blocks.
	static BSSimpleList<BGSDistantObjectBlock*> BlocksLoaded;
	static inline int32_t uiLowPrioBlocks;
	static inline int32_t uiHighPrioBlocks;
};
