#pragma once

#include "BSMain/QueuedFiles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BGSDistantObjectBlock;
class BSMultiBoundNode;
class NiGeometry;
class TESWorldSpace;

// The queued task that loads one distant object block's mesh and multibound node (a QueuedFileEntry).
class BGSDistantObjectBlockLoadTask : public QueuedFileEntry {
public:
    BGSDistantObjectBlockLoadTask(
        BGSDistantObjectBlock* apBlock,
        TESWorldSpace* apWorld,
        int32_t aiX,
        int32_t aiY,
        int32_t aiLevel,
        bool abPostApocalypse,
        bool abHighPriority,
        bool abStingerVersion
    );
    ~BGSDistantObjectBlockLoadTask() override;

    void Run() override;
    void Finish() override;
    void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    void QueueMe() override;
    void CheckFinished() override;
    uint8_t GetFileIndex() override;

    // The loaded mesh, assigned to aspMesh; BGSDistantObjectBlock::LoadFromTask calls it.
    void GetMesh(NiPointer<NiGeometry>& aspMesh);
    // The loaded multibound node, assigned to aspNode.
    void GetNode(NiPointer<BSMultiBoundNode>& aspNode);

    static void GetBuildingsPath(char* apDest, TESWorldSpace* apWorldspace);
    static void GetBuildingsNormalPath(char* apDest, TESWorldSpace* a2);
    static void operator delete(void* apMemory);

    bool bFinishedQueuing;
    uint8_t padding31;
    uint8_t padding32;
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
    uint8_t padding52;
    uint8_t padding53;
    uint8_t padding54;
    uint8_t padding55[3];
};
