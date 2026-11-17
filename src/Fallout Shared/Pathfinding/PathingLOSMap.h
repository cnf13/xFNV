#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

class NavMeshPtr;

char BSMAX_1(char a1, char a2);

class PathingLOSMap : public NiRefObject {
public:
    PathingLOSMap(NiPoint3 aCenter, float afRadius);
    ~PathingLOSMap() override;

    // The score of triangle aiIndex in the map's triangle scores. The index is unsigned: the callers count it up to the
    // unsigned iTotalTriangleCount, as GetNavMeshAndTriangleForIndex's. The returned char is used as signed.
    char GetTriangleScore(uint32_t aiIndex);
    float GetSeenPercent() const;
    bool GetNavMeshAndTriangleForIndex(uint32_t aiIndex, NavMeshPtr& aRetNavMesh, uint16_t& aRetTriangle);
    bool GetOffsetForNavMesh(uint32_t aNavMeshID, uint32_t& aRetOffset);
    // The navmesh's slice of pTriangleScores, or null when the map has no offset for it.
    char* GetArrayForNavMesh(uint32_t aNavMeshID);
    // The score of triangle ausTriangle of navmesh aNavMeshID, or 127 when the map has no
    // score for it.
    char GetAt(uint32_t aNavMeshID, uint16_t ausTriangle);
    NiPoint3 GetCenter();
    void SetAt(int32_t a2, char a3);
    uint32_t GetNavMeshCount() const;
    // The pointer to navmesh aiNavMeshIndex of NavMeshes.
    NavMeshPtr& GetNavMeshByIndex(uint32_t aiNavMeshIndex);
    bool Setup(const BSSimpleArray<NavMeshPtr, 1024>& aNavMeshes);
    bool GetUnseenNavMeshAndTriangle(NavMeshPtr& aRetNavMesh, uint16_t& aRetTriangle);
    NiNode* CreateDebugGeometry();
    void ClearUnseenTriangles();
    bool IsNavMeshInMap(uint32_t aiNavMeshID);

    NiTMap<uint32_t, uint32_t> NavMeshToOffsetMap;
    char* pTriangleScores;
    BSSimpleArray<NavMeshPtr, 1024> NavMeshes;
    uint32_t iTotalTriangleCount;
    uint32_t iTotalTrianglesSeen;
    uint32_t iTotalTrianglesUnseen;
    uint8_t iMaxScore;
    int8_t iMinScore;
    uint8_t padding3A[2];
    NiPoint3 Center;
    float fRadius;
    // The sight radius; the constructor leaves it uninitialised.
    float fSightRadius;
};
