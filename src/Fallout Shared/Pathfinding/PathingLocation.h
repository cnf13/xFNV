#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <stdint.h>

class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class BGSSaveGameBuffer;
class BGSWorldLocation;
struct FindTriangleForLocationFilter;
class NavMeshInfo;
class NavMeshPtr;
class TESObjectCELL;
class TESObjectREFR;
class TESWorldSpace;

// A place an actor may path to: a point, plus whatever resolves that point, either an interior cell or a worldspace
// cell key, and the navmesh triangle under it when it lies on a navmesh.
class PathingLocation {
public:
    // The bits of uiFlags.
    enum Flags {
        kFlagAllMeshesReachable = 0x01,
        kFlagUnused = 0x02,
    };

    PathingLocation();
    PathingLocation(const PathingLocation& aLoc);
    PathingLocation(const NiPoint3& akLocation, TESObjectREFR* apRef);
    PathingLocation(const NiPoint3& aLocation, const PathingLocation& aHintLoc);
    PathingLocation(const NiPoint3* arLocation, TESObjectCELL* apCell, TESWorldSpace* apWorld);
    PathingLocation(const NiPoint3* arLocation, const NavMeshInfo* apNavMeshInfo);
    PathingLocation(const NiPoint3& arLocation, const NavMeshInfo* apNavMeshInfo, uint16_t ausTriangle);
    PathingLocation(TESObjectREFR* apTargetRef);
    // From Construct_006DCE60 and Construct_006DCEA0.
    PathingLocation(const NiPoint3* a1, TESObjectCELL* a2);
    PathingLocation(const NiPoint3* a1, TESWorldSpace* a2);
    PathingLocation(BGSWorldLocation* ecx0);

    // writes the location to a save game
    virtual void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
    // reads the location back from a save game
    virtual void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
    virtual ~PathingLocation();

    // Whether an actor of radius afRadius can stand at the location.
    bool IsValidLocation(float afRadius);

    // The point itself.
    NiPoint3 GetLocation() const;

    // The cell the point is in, resolving a worldspace location through its cell key.
    TESObjectCELL* GetCell() const;

    // pWorldSpace.
    TESWorldSpace* GetWorldspaceSimple() {
        return pWorldSpace;
    }

    void SetUnused();

    // Copies every field but the first.
    PathingLocation& operator=(const PathingLocation& aLoc);

    bool QUnused() const;

    bool QAllMeshesReachable();

    void SetAllMeshesReachable(int32_t aiVal);

    // True when the point lies on a real triangle of its navmesh.
    bool HasNavMeshData();

    // The navmesh and triangle under the point; false when there is none.
    bool GetNavMeshAndTriangle(NavMeshPtr& aRetNavMesh, uint16_t& aRetTriangle) const;

    // Resolves pNavMesh and usTriangle from pNavMeshes: the loaded navmesh with a triangle under the point when the
    // cell is attached, else the navmesh whose bounds hold the point or lie nearest to it. False when nothing resolves.
    bool ResolveNavMeshInfo(FindTriangleForLocationFilter* apFilter);
    void SetTriangleIndex(uint16_t aiTriIndex);
    void ClearTriangleData();
    uint16_t GetTriangle() const;
    // pNavMesh, else the first of pNavMeshes when all of them are reachable, else null.
    const NavMeshInfo* GetBestNavMeshInfo();
    bool HasNavMesh();
    bool GetNavMesh(NavMeshPtr& aRetNavMesh) const;
    bool IsCloseTo(const PathingLocation& aOtherLoc, float afTolerance);
    // Moves the point to aLocation and clears the triangle; in a worldspace, a point that changed cell key is set
    // up again.
    void SetLocation(const NiPoint3& aLocation);
    bool ResolveNavMeshInfoOnly();

protected:
    // Whether every navmesh of pNavMeshes can be reached from its first one through adjacency; true once pNavMesh is
    // known.
    bool AreAllMeshesReachable();

public:
    // Moves the point onto the closest triangle of the candidate navmeshes.
    bool ResolveToClosestNavmeshAndTriangle();

protected:
    // Fills the location from aLocation and either apWorldSpace or apCell, and picks the navmesh when exactly one
    // covers it. SetLocation calls it.
    void SetupData(const NiPoint3& aLocation, TESObjectCELL* apCell, TESWorldSpace* apWorldSpace);

public:
    // Sets only Location from aLocation.
    void SetLocationSimple(const NiPoint3& aLocation);
    void SetNavMeshInfo(const NavMeshInfo* apNavMeshInfo);
    bool GetNavMeshInfos(BSSimpleArray<const NavMeshInfo*, 1024>& apRetInfos);
    void PrintDebugText(char* apBuffer, int32_t aiMaxSize);

    // Frees with ourFree.
    static void operator delete(void* apMemory);

    // The point the location resolves to.
    NiPoint3 Location;
    // The single navmesh the point lies on; null while only a cell or worldspace is known.
    const NavMeshInfo* pNavMesh;
    // The navmeshes covering the point, used when pNavMesh is null.
    const BSSimpleArray<NavMeshInfo*, 1024>* pNavMeshes;
    // The interior cell the point is in, if any.
    TESObjectCELL* pCell;
    // The worldspace the point is in, if it is not in an interior cell.
    TESWorldSpace* pWorldSpace;
    // The worldspace cell key of the point, used while pCell is null.
    uint32_t iCellKey;
    // The triangle inside pNavMesh the point lies on; 0xFFFF while it is not on one.
    uint16_t usTriangle;
    // Which meshes were found reachable (bit 0) and whether the location is unused (bit 1).
    uint8_t uiFlags;
    uint8_t padding27;
};
