#pragma once

#include "BSMain/QueuedFiles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BGSDistantObjectBlock;
class BSMultiBoundNode;
class NiGeometry;
class TESWorldSpace;

// The queued task that loads one distant object block's mesh and multibound node (a QueuedFileEntry).
class BGSDistantObjectBlockLoadTask : public QueuedFileEntry {
public:
	BGSDistantObjectBlockLoadTask(BGSDistantObjectBlock* apBlock, TESWorldSpace* apWorld, int32_t aiX, int32_t aiY,
		int32_t aiLevel, bool abPostApocalypse, bool abHighPriority, bool abStingerVersion);
	~BGSDistantObjectBlockLoadTask() override;

	void Run() override;
	void Finish() override;
	void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
	void QueueMe() override;
	void CheckFinished() override;
	uint8_t GetFileIndex() override;

	// The loaded mesh, assigned to aspMesh (thiscall, ret 4, 0x6FAA30; BGSDistantObjectBlock::
	// LoadFromTask 0x6F6150 calls it).
	void GetMesh(NiPointer<NiGeometry>& aspMesh);
	// The loaded multibound node, assigned to aspNode.
	void GetNode(NiPointer<BSMultiBoundNode>& aspNode);

	// Inferred type: apDest: char* from 0x6f57b2 (passed to BSsprintf 0); apWorldspace: TESWorldSpace* from 0x6f70a8
	// (BGSDistantObjectBlockLoadTask::pWorldSpace (this of BGSDistantObjectBlockLoadTask::QueueMe)), its members used
	// at 0x6f5786
	static void GetBuildingsPath(char* apDest, TESWorldSpace* apWorldspace);
	// Inferred type: apDest: char* from 0x6f57f2 (passed to BSsprintf 0); a2: TESWorldSpace* from 0x6f70e6
	// (BGSDistantObjectBlockLoadTask::pWorldSpace (this of BGSDistantObjectBlockLoadTask::QueueMe)), its members used
	// at 0x6f57c6
	static void GetBuildingsNormalPath(char* apDest, TESWorldSpace* a2);
	static void operator delete(void* apMemory);

	bool bFinishedQueuing;
	// IDA: bool byte31.
	uint8_t padding31;
	// IDA: bool byte32.
	uint8_t padding32;
	// IDA: bool byte33.
	uint8_t padding33;
	int32_t iX;
	int32_t iY;
	int32_t iLevel;
	BGSDistantObjectBlock* pBlock;
	TESWorldSpace* pWorldSpace;
	NiPointer<NiGeometry> spMesh;
	NiPointer<BSMultiBoundNode> spNode;
	bool bCanceled;
	bool bStingerVersion;
	// IDA: bool byte52.
	uint8_t padding52;
	// IDA: bool byte53.
	uint8_t padding53;
	// IDA: bool unk54.
	uint8_t padding54;
	uint8_t padding55[3];
};
inline constexpr fnvd::Vtable BGSDistantObjectBlockLoadTask_vftable{};
