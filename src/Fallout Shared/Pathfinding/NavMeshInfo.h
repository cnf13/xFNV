#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NavMeshBounds;
class TESObjectCELL;
class TESWorldSpace;

// A navmesh's entry in the navmesh info map: its ids, flags, bounds and connections. Only some of the members are
// declared.
class NavMeshInfo {
public:
    // An empty info: no ids, flags, parent, navmesh or bounds; cell key 0xDEADBEAD; the location at the origin.
    NavMeshInfo();
    // Detaches the navmesh and deletes the bounds; the compiler destroys the arrays.
    ~NavMeshInfo();

    void SetDisabled(bool abDisable);
    uint32_t GetNavMeshID() const;
    // Puts the loaded navmesh into aReturnNavMeshPtr; false when disabled or not loaded.
    bool GetNavMesh(NavMeshPtr& aReturnNavMeshPtr) const;
    // Returns pParentSpace, else the form named by ParentSpaceID.
    TESForm* GetParentSpace() const;
    void SetNavMesh(NavMesh* apNavMesh);
    // Fills the info from apNavMesh.
    void CreateFromNavMesh(NavMesh* apNavMesh);
    // Sets fPreferredPercent.
    void SetPreferredPercent(float afPercent);
    // Whether NAVMESHINFO_FLAG_DISABLED (0x10) is set in uiFlags.
    bool GetDisabled() const;
    NavMeshBounds* GetBounds() const;

    // Resolves stored door ids in place and removes entries whose forms are missing.
    void InitDoorPointers();
    // The exterior cell at iCellKey in the navmesh's worldspace, else the interior cell.
    TESObjectCELL* GetCell() const;
    // Returns pParentSpace when it is a cell, else null.
    TESObjectCELL* GetInteriorCell() const;
    // Whether pNavMesh is non-null.
    bool HasNavMesh() const;
    TESWorldSpace* GetWorldSpace() const;
    // The bytes FillInfoChunk writes.
    int32_t GetInfoChunkSize();
    // The bytes FillPortalChunk writes.
    int32_t GetPortalChunkSize();
    // Writes the flags, ids, cell key, location, bounds and preferred percent to apChunkBuffer, byte swapped when the
    // save swaps.
    void FillInfoChunk(char* apChunkBuffer);
    // Reads what FillInfoChunk writes; the preferred percent only from file version 10 on.
    void SetFromInfoChunk(char* apChunkBuffer, TESFile* apFile, int32_t aiFileVersionNumber);
    // Writes the adjacent and preferred adjacent navmesh IDs and the connected door IDs, each list after its count.
    void FillPortalChunk(char* apChunkBuffer);
    // Puts the navmesh into aReturnNavMeshPtr even when disabled; false when not loaded.
    bool GetNavMeshEvenDisabled(NavMeshPtr& aReturnNavMeshPtr);
    // Reads the adjacent and preferred adjacent navmesh IDs and the connected door IDs that
    // FillPortalChunk writes.
    void SetFromPortalChunk(char* apChunkBuffer, TESFile* apFile, int32_t aiFileVersionNumber);
    // Copies this info's ids, flags, location, navmesh, arrays and bounds into apDestInfo.
    void CopyTo(NavMeshInfo* apDestInfo);
    // Empty in this build: the engine's shared empty stub.
    void SetAltered(bool abAltered);
    // False in this build.
    bool GetAltered() const;
    // Returns the raw navmesh pointer, not reference counted, null when disabled; true when there is one.
    bool GetNavMesh(NavMesh*& arpReturnNavMesh) const;

    // Deleting a NavMeshInfo frees it with ourFree.
    static void operator delete(void* apMemory);
    // The cell key map's key for a1: its interior cell's address, else its exterior cell key. The key holds a cell
    // pointer, so it is pointer-sized.
    static uintptr_t GetCellOrCoords(NavMeshInfo* a1);
    // Returns its first argument. Actor::QueueReplacementLocomotion passes the first-person skeleton's file name and 0.
    static char* GetCellOrCoords(char* apFirst, int32_t aiUnused);

    uint32_t NavMeshID;
    uint32_t ParentSpaceID;
    uint32_t uiFlags;
    int32_t iCellKey;
    NiPoint3 ApproxLocation;
    TESForm* pParentSpace;
    float fPreferredPercent;
    BSSimpleArray<NavMeshInfo*> AdjacentNavMeshes;
    BSSimpleArray<NavMeshInfo*> PreferredAdjacentNavMeshes;
    BSSimpleArray<TESObjectREFR*> ConnectedDoors;
    NavMesh* pNavMesh;
    NavMeshBounds* pBounds;
};
