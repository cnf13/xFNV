#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/QueuedFiles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class BGSTerrainChunkLoadTask;
class BGSTerrainNode;
class BSMultiBoundNode;
class BSShaderPPLightingProperty;
class NiCamera;
class NiCullingProcess;
class TESObjectREFR;
class TESWorldSpace;

class BGSTerrainChunk {
public:
    // The textures a chunk's shader property holds.
    enum ChunkTextureType : int32_t {
        CHUNK_TEXTURE_DIFFUSE = 0,
        CHUNK_TEXTURE_NORMAL = 1,
    };

    using LoadedChunks = BSSimpleList<BGSTerrainChunk*>;

    BGSTerrainChunk(BGSTerrainNode* apNode, const int32_t aiLevel, const int16_t asCellX, const int16_t asCellY);
    ~BGSTerrainChunk();

    // Sets the morph parameter of the chunk's meshes.
    void SetMorphParameter(float afMorphParam);
    // The texture of type aTexType of the chunk's shader property.
    NiTexture* GetTexture(ChunkTextureType aTexType);
    bool GetAttached();
    // spMesh's object.
    NiGeometry* GetMesh();
    // Adds the chunk to the tail of ChunksLoaded.
    void AddToStaticList();
    // Culls the chunk's mesh and water meshes.
    void Hide();
    // Un-culls the chunk's mesh and water meshes.
    void Show();
    void RemoveFromStaticList();
    void Prepare();
    void OnLoadFinished();
    void Attach();
    void AddWaterToSystem(bool abForce);
    void RemoveWaterFromSystem(bool abForce);
    BSShaderPPLightingProperty* GetShadeProperty();
    NiGeometry* GetWater() {
        return spWater;
    }
    NiGeometry* GetWaterAtReflectionHeight() {
        return spWaterAtReflectionHeight;
    }
    BSMultiBoundNode* GetNode() {
        return spNode;
    }

    // Turns the LOD water processing of the chunks on or off.
    static void EnableLODWaterProcessing(bool abEnable);
    // Shows only the persistent chunks.
    static void ShowMinimumDetail();
    // Shows every loaded chunk.
    static void ShowNormalDetail();
    static void operator delete(void* apMemory);
    static void UpdateMultiBoundVisibility(NiCamera* apCamera, NiCullingProcess& aCullProc);
    static void ResetMultiBoundResults();
    static void
    BuildLODObjectMeshFilename(char* apDest, TESWorldSpace* apWorldSpace, int32_t lodLevel, int32_t x, int32_t y);
    static int32_t BuildLODObjectDiffuseFilename(
        char* apDest,
        TESWorldSpace* apWorld,
        int32_t auiLODLevel,
        int16_t sPosX,
        int16_t sPoxY
    );
    static int32_t BuildLODObjectNormalFilename(
        char* apDest,
        TESWorldSpace* apWorld,
        int32_t aiLODLevel,
        int16_t asPosX,
        int16_t asPosY
    );

    BGSTerrainNode* pNode;
    NiPointer<NiGeometry> spMesh;
    NiPointer<NiGeometry> spWater;
    NiPointer<NiGeometry> spWaterAtReflectionHeight;
    NiPointer<BSMultiBoundNode> spNode;
    NiPointer<BSMultiBoundNode> spWaterNode;
    NiPointer<BSMultiBoundNode> spWaterReflNode;
    TESObjectREFR* pLODWaterRef;
    TESObjectREFR* pLODWaterAtReflectionHeightRef;
    NiPointer<BGSTerrainChunkLoadTask> spLoadTask;
    bool bDoneLoading;
    bool bAttached;
    bool bPrepared;
    NiPointer<BSShaderPPLightingProperty> spLightingProp;

    static LoadedChunks ChunksLoaded;
    static inline bool bLODWaterProcessing = true;
    static inline int32_t iChunkCount;
};

// The queued task that loads a terrain chunk's mesh and water meshes, then attaches them to the chunk.
class BGSTerrainChunkLoadTask : public QueuedFileEntry {
public:
    BGSTerrainChunkLoadTask(
        BGSTerrainChunk* apChunk,
        TESWorldSpace* apWorld,
        int32_t aiX,
        int32_t aiY,
        int32_t aiLevel
    );
    ~BGSTerrainChunkLoadTask() override;

    void Run() override;
    void Finish() override;
    void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    void QueueMe() override;
    void CheckFinished() override;
    uint8_t GetFileIndex() override;

    // Copies spMesh into aspGeometry; BGSTerrainChunk::OnLoadFinished calls it on spLoadTask.
    void GetMesh(NiPointer<NiGeometry>& aspGeometry);
    // spWater (+0x4C) into aspGeometry.
    void GetWater(NiPointer<NiGeometry>& aspGeometry);
    // spWaterAtReflectionHeight (+0x50) into aspGeometry.
    void GetWaterAtReflectionHeight(NiPointer<NiGeometry>& aspGeometry);
    // spNode (+0x54) into aspNode.
    void GetNode(NiPointer<BSMultiBoundNode>& aspNode);

    static void operator delete(void* apMemory);

    bool bFinishedQueuing;
    int32_t iX;
    int32_t iY;
    int32_t iLevel;
    BGSTerrainChunk* pChunk;
    TESWorldSpace* pWorldSpace;
    NiPointer<NiGeometry> spMesh;
    NiPointer<NiGeometry> spWater;
    NiPointer<NiGeometry> spWaterAtReflectionHeight;
    NiPointer<BSMultiBoundNode> spNode;
    bool bCanceled;
};

NiSmartPointer(BGSTerrainChunkLoadTask);
