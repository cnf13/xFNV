#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

NavMeshTriangle::NavMeshTriangle()
  : Vertices{0xFFFF, 0xFFFF, 0xFFFF}, Triangles{0xFFFF, 0xFFFF, 0xFFFF}, TriangleFlags(0) {}

bool NavMeshTriangle::IsWater() const {
    return (bool)((uint8_t)GetFlag(512));
}

uint16_t NavMeshTriangle::GetVert(uint32_t aiVertIndex) const {
    return Vertices[aiVertIndex];
}

bool NavMeshTriangle::GetFlag(int32_t aFlag) const {
    return ((TriangleFlags & aFlag) != 0);
}

void NavMeshTriangle::SetFlag(int32_t aFlag) {
    TriangleFlags |= aFlag;
}

void NavMeshTriangle::ClearFlag(int32_t aFlag) {
    TriangleFlags &= (~aFlag);
}

bool NavMeshTriangle::HasEdgeExtraInfo() {
    return (TriangleFlags & 7) != 0;
}

bool NavMeshTriangle::EdgeHasExtraInfo(int32_t aiEdgeIndex) {
    return ((1 << aiEdgeIndex) & TriangleFlags) != 0;
}

uint16_t NavMeshTriangle::GetTriOrExtraInfo(int32_t aiEdgeIndex) {
    return Triangles[aiEdgeIndex];
}

void NavMeshTriangle::ClearEdgeExtraInfo(int32_t aiEdgeIndex) {
    TriangleFlags &= (~(1 << aiEdgeIndex));
    Triangles[aiEdgeIndex] = (uint16_t)(-1);
}

bool NavMeshTriangle::IsPreferred() const {
    return (TriangleFlags & 0x40) != 0;
}

bool NavMeshTriangle::IsNoLargeCreature() const {
    return GetFlag(0x10);
}

void NavMeshTriangle::ToggleNoLargeCreature() {
    TriangleFlags ^= 16;
}

bool NavMeshTriangle::HasCover() const {
    return 0 < (TriangleFlags & 0xFBE0000);
}

bool NavMeshTriangle::HasDoorPortal() const {
    return 0 < (TriangleFlags & 0x400);
}

bool NavMeshTriangle::IsEdgeConnected(int32_t aiEdgeIndex) const {
    return (bool)((uint8_t)(((uint16_t)Triangles[aiEdgeIndex]) != 65535));
}

void NavMeshTriangle::SetTri(uint32_t aiEdgeIndex, uint16_t aiTriangle) {
    ClearEdgeExtraInfo(aiEdgeIndex);
    Triangles[aiEdgeIndex] = aiTriangle;
}

void NavMeshTriangle::SetVert(uint32_t aiVertIndex, uint16_t aiVertex) {
    Vertices[aiVertIndex] = aiVertex;
}

void NavMeshTriangle::SetEdgeExtraInfo(int32_t aiEdgeIndex, uint16_t ausExtraInfoIndex) {
    TriangleFlags |= (1 << aiEdgeIndex);
    Triangles[aiEdgeIndex] = ausExtraInfoIndex;
}

// Shift counts are masked to their low five bits, matching the game's 32-bit shifts.
void NavMeshTriangle::SetEdgeCover(uint16_t aiEdge, uint16_t aiData, bool abLeftSide, bool abRightSide) {
    if (aiEdge != 2) {
        if (aiData > 15u) {
            aiData = 15;
        }
        uint32_t uiShift = 6u * aiEdge + 16u;
        TriangleFlags &= ~(63u << (uiShift & 31));
        if (aiData > 0) {
            uint32_t uiBits = static_cast<uint32_t>(aiData) << (uiShift & 31);
            if (aiData > 1) {
                uiShift += 4;
                uiBits |= static_cast<uint32_t>(static_cast<uint8_t>(abLeftSide)) << (uiShift & 31);
                uiShift += 1;
                uiBits |= static_cast<uint32_t>(static_cast<uint8_t>(abRightSide)) << (uiShift & 31);
            }
            TriangleFlags |= uiBits;
        }
    }
}

uint16_t NavMeshTriangle::GetEdgeCover(uint16_t aiEdge) const {
    return (uint16_t)((TriangleFlags >> ((6 * aiEdge) + 16)) & 15);
}

void NavMeshTriangle::GetVerts(uint16_t& i0, uint16_t& i1, uint16_t& i2) const {
    i0 = Vertices[0];
    i1 = Vertices[1];
    i2 = Vertices[2];
}

void NavMeshTriangle::GetSides(uint16_t* a2, uint16_t* a3, uint16_t* a4) {
    *a2 = Triangles[0];
    *a3 = Triangles[1];
    *a4 = Triangles[2];
}

void NavMeshTriangle::GetEdgeCoverData(uint16_t aiTriangleFlags, uint16_t& aiEdge, bool& aiData, bool& abRight) const {
    uint32_t uiFlags = TriangleFlags >> (6 * aiTriangleFlags + 16);
    aiEdge = uiFlags & 15;
    aiData = (uiFlags & 16) != 0;
    abRight = (uiFlags & 32) != 0;
}

float NavMeshTriangle::GetCoverHeight(uint32_t aiVal) {
    if (aiVal > 15) {
        aiVal = (uint32_t)15;
    }
    return (float)(((double)aiVal) * 0x1p+4f);
}

// Rounds the scaled height to the closest level (down when the fraction is under 0.25, else up), down or up.
uint32_t NavMeshTriangle::GetCoverLevel(float afHeight, bool abRoundDown, bool abRoundClosest) {
    float fLevel = afHeight * NAVMESH_COVER_HEIGHT_GRANULARITY_INVERSE;
    uint32_t uiLevel;
    if (abRoundClosest) {
        float fFloor = std::floor(fLevel);
        float fFraction = fLevel - fFloor;
        if (fFraction < 0.25) {
            uiLevel = (uint32_t)fFloor;
        }
        else {
            uiLevel = (uint32_t)std::ceil(fLevel);
        }
    }
    else if (abRoundDown) {
        uiLevel = (uint32_t)std::floor(fLevel);
    }
    else {
        uiLevel = (uint32_t)std::ceil(fLevel);
    }
    if (uiLevel > 15) {
        uiLevel = 15;
    }
    return uiLevel;
}
