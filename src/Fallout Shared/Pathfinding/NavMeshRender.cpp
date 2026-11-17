#include "Fallout Shared/Pathfinding/NavMeshRender.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSTestObjects.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshArray.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshStaticAvoidNode.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"

// The game settings this file defines, with their IDs and values, in the game's initialization order.
NiPointer<NiNode> NavMeshRender::spRootDrawNode;
SettingT<INISettingCollection> bSelectedPathDrawAvoidNodes("bSelectedPathDrawAvoidNodes:Pathfinding", false);

// The layout the two constructors write, pinned on the int map (NiTMapBase.h has no pins; the template arguments'
// commas need __builtin_offsetof and an alias).
using NiTMapBase_Int_Int = NiTMapBase<DFALL<NiTMapItem<int32_t, int32_t>>, int32_t, int32_t>;

NavMeshRender::NavMeshRender()
  : sp3DNode(nullptr), spSelectedPathGeometry(nullptr), spLOSMapGeometry(nullptr), spAlphaProp(nullptr),
    spLocalMapMaterial(nullptr), NavMeshNodes(37u), NavMeshCellMap(37u), bDraw(false) {
    ColorValues[0] = NiColorA(1.0f, 0.0f, 0.0f, 1.0f);
    ColorValues[1] = NiColorA(1.0f, 0.0f, 0.0f, 1.0f);
    ColorValues[2] = NiColorA(0.0f, 1.0f, 0.0f, 1.0f);
    ColorValues[3] = NiColorA(0.0f, 1.0f, 0.0f, 1.0f);
    ColorValues[4] = NiColorA(0.0f, 1.0f, 0.5f, 1.0f);
    ColorValues[5] = NiColorA(0.0f, 0.5f, 1.0f, 1.0f);
    ColorValues[6] = NiColorA(1.0f, 1.0f, 0.0f, 1.0f);
    ColorValues[7] = NiColorA(0.0f, 0.0f, 1.0f, 1.0f);
    ColorValues[8] = NiColorA(0.5f, 0.5f, 1.0f, 1.0f);
    ColorValues[9] = NiColorA(1.0f, 1.0f, 1.0f, 1.0f);
    ColorValues[10] = NiColorA(0.0f, 0.0f, 0.0f, 1.0f);
    ColorValues[11] = NiColorA(1.0f, 0.0f, 1.0f, 1.0f);
    ColorValues[12] = NiColorA(0.75f, 0.25f, 1.0f, 1.0f);
    ColorValues[13] = NiColorA(1.0f, 0.5f, 0.0f, 1.0f);
    ColorValues[14] = NiColorA(1.0f, 0.5f, 0.5f, 1.0f);
    ColorValues[15] = NiColorA(1.0f, 0.0f, 1.0f, 1.0f);
    ColorValues[16] = NiColorA(0.75f, 0.0f, 0.0f, 1.0f);
    const float afEdgeAlpha[17] = {0.5f, 1.0f,  0.5f,  1.0f,  1.0f, 1.0f, 0.5f, 0.5f, 0.5f,
                                   0.5f, 0.75f, 0.25f, 0.25f, 0.5f, 0.5f, 0.5f, 0.5f};
    // Index 5 is deliberately left unwritten and read below, as the game does.
    float afTriangleAlpha[17];
    afTriangleAlpha[0] = 0.175f;
    afTriangleAlpha[1] = 0.35f;
    afTriangleAlpha[2] = 0.175f;
    afTriangleAlpha[3] = 0.35f;
    afTriangleAlpha[4] = 0.175f;
    afTriangleAlpha[6] = 0.175f;
    afTriangleAlpha[7] = 0.175f;
    afTriangleAlpha[8] = 0.175f;
    afTriangleAlpha[9] = 1.0f;
    afTriangleAlpha[10] = 0.175f;
    afTriangleAlpha[11] = 0.25f;
    afTriangleAlpha[12] = 0.25f;
    afTriangleAlpha[13] = 0.175f;
    afTriangleAlpha[14] = 0.175f;
    afTriangleAlpha[15] = 0.175f;
    afTriangleAlpha[16] = 0.175f;
    spAlphaProp = new NiAlphaProperty;
    spAlphaProp->SetAlphaBlending(true);
    for (uint32_t i = 0; i < 17; ++i) {
        spEdgeAndVertexMaterialArray[i] = new NiMaterialProperty;
        spEdgeAndVertexMaterialArray[i]->SetEmittance(NiColor_WHITE);
        spEdgeAndVertexMaterialArray[i]->SetAlpha(afEdgeAlpha[i]);
        spTriangleMaterialArray[i] = new NiMaterialProperty;
        spTriangleMaterialArray[i]->SetEmittance(NiColor_WHITE);
        spTriangleMaterialArray[i]->SetAlpha(afTriangleAlpha[i]);
        spVertPoints[i] = MakeTriPoint(fPointSize, ColorValues[i], true);
        spVertPoints[i]->AttachProperty(spEdgeAndVertexMaterialArray[i].m_pObject);
        NiPointer<BSShaderNoLightingProperty> spShaderProp = new BSShaderNoLightingProperty;
        spShaderProp->SetFlag(32u, 0);
        spVertPoints[i]->RemoveProperty(3);
        spVertPoints[i]->AttachProperty(spShaderProp.m_pObject);
    }
    spLocalMapMaterial = new NiMaterialProperty;
    spLocalMapMaterial->SetEmittance(NiColor_WHITE);
    spLocalMapMaterial->SetAlpha(0.5f);
}

// The material arrays and the vertex points are released in the game's order before the members are destroyed.
NavMeshRender::~NavMeshRender() {
    for (uint32_t i = 0; i < 17; ++i) {
        spEdgeAndVertexMaterialArray[i] = nullptr;
        spTriangleMaterialArray[i] = nullptr;
        spVertPoints[i] = nullptr;
    }
}

bool NavMeshRender::GetDraw() {
    return (bool)((uint8_t)bDraw);
}

// Builds the debug geometry of apCell's navmeshes under a new cell node (unless the cell has one already): per navmesh
// a triangle shape coloured by triangle type, the edge lines with the cover boxes, the connection lines across shared
// and portal edges and a sphere per static avoid node. The by-value NiPointer argument of the cell map's SetAt is a
// copy of spCellNode.
void NavMeshRender::AddDrawOnlyNavMeshes(TESObjectCELL* apCell, bool abUpdate) {
    using NiPoint3Array = BSSimpleArray<NiPoint3, 1024>;
    using NiColorAArray = BSSimpleArray<NiColorA, 1024>;
    using ByteArray = BSSimpleArray<uint8_t, 1024>;
    if (!apCell) {
        return;
    }
    NavMeshArray* pNavMeshes = apCell->pNavMeshes;
    if (!pNavMeshes) {
        return;
    }
    uint32_t uiNavMeshes = pNavMeshes->GetNavMeshCount();
    NiPointer<NiNode> spCellNode(static_cast<NiNode*>(nullptr));
    if (NavMeshCellMap.GetAt(apCell, spCellNode)) {
        return;
    }
    void* pNodeMemory = NiMemObject::operator new(sizeof(NiNode));
    NiNode* pNode = pNodeMemory ? ::new (pNodeMemory) NiNode(2 * uiNavMeshes) : nullptr;
    spCellNode = pNode;
    spRootDrawNode->AttachChild(spCellNode, 1);
    NavMeshCellMap.SetAt(apCell, spCellNode);
    NavMeshPtr spNavMesh;
    for (uint32_t i = 0; i < uiNavMeshes; ++i) {
        FNVD_INLINED_LOCAL(NavMeshPtr, kNavMeshAt, std::nullopt, std::nullopt);
        NavMeshPtr* pNavMeshAt = pNavMeshes->GetNavMeshByIndex(&kNavMeshAt, i);
        spNavMesh = *pNavMeshAt;
        kNavMeshAt.~NavMeshPtr();
        uint32_t uiTriSize = spNavMesh->GetTriangleCount();
        uint32_t uiVertSize = spNavMesh->GetVertexCount();
        if (!uiTriSize) {
            continue;
        }
        bool bDisabledMesh = spNavMesh->GetDisabled();
        // The triangle shape's vertices and colours: the navmesh's vertices, plus a copy of the three vertices of
        // each triangle drawn in a colour of its own.
        NiPoint3Array triVertList(uiVertSize);
        NiColorAArray triColorList(uiVertSize);
        if (bDisabledMesh) {
            for (uint32_t iVert = 0; iVert < uiVertSize; ++iVert) {
                NiPoint3& rLocation = spNavMesh->Vertices.GetAt(static_cast<int32_t>(iVert)).Location;
                triVertList.GetAt(static_cast<int32_t>(iVert)) = rLocation;
                triColorList.GetAt(static_cast<int32_t>(iVert)) = ColorValues[NAVMESH_RENDER_COLOR_TYPE_BLACK];
            }
        }
        else {
            for (uint32_t iVert = 0; iVert < uiVertSize; ++iVert) {
                NiPoint3& rLocation = spNavMesh->Vertices.GetAt(static_cast<int32_t>(iVert)).Location;
                triVertList.GetAt(static_cast<int32_t>(iVert)) = rLocation;
                triColorList.GetAt(static_cast<int32_t>(iVert)) = ColorValues[NAVMESH_RENDER_COLOR_TYPE_NORMAL];
            }
        }
        uint16_t* pusTriList = static_cast<uint16_t*>(_NiMalloc(3 * sizeof(uint16_t) * uiTriSize));
        uint32_t uiTotalSize = uiVertSize;
        uint32_t uiCopy = 0;
        uint32_t uiLineSize = 6 * uiTriSize;
        NiPoint3Array edgeVertList(uiLineSize);
        ByteArray edgeConnectionList(uiLineSize);
        NiColorAArray edgeColorList(uiLineSize);
        uint32_t iEdgeCopy = 0;
        NiPoint3Array connectVertList(uiLineSize);
        ByteArray connectConnectionList(uiLineSize);
        NiColorAArray connectColorList(uiLineSize);
        uint32_t iConnectionCopy = 0;
        for (uint32_t iTri = 0; iTri < uiTriSize; ++iTri) {
            NavMeshTriangle* pTri = spNavMesh->GetTriangle(static_cast<uint16_t>(iTri));
            if (pTri->GetFlag(NavMeshTriangle::NAV_TRI_DISABLED)) {
                continue;
            }
            uint16_t ausTriVertices[3];
            pTri->GetVerts(ausTriVertices[0], ausTriVertices[1], ausTriVertices[2]);
            NAVMESH_RENDER_COLOR_TYPE eColor;
            if (bDisabledMesh || (eColor = GetTriangleColor(pTri)) == NAVMESH_RENDER_COLOR_TYPE_NORMAL) {
                pusTriList[uiCopy++] = ausTriVertices[0];
                pusTriList[uiCopy++] = ausTriVertices[1];
                pusTriList[uiCopy++] = ausTriVertices[2];
            }
            else {
                triVertList.SetSize(uiTotalSize + 3, true);
                triColorList.SetSize(uiTotalSize + 3, true);
                for (uint32_t iCorner = 0; iCorner < 3; ++iCorner) {
                    NiPoint3& rCorner = triVertList.GetAt(ausTriVertices[iCorner]);
                    triVertList.GetAt(static_cast<int32_t>(uiTotalSize)) = rCorner;
                    triColorList.GetAt(static_cast<int32_t>(uiTotalSize)) = ColorValues[eColor];
                    pusTriList[uiCopy++] = static_cast<uint16_t>(uiTotalSize++);
                }
            }
            // 0xFFFF: no triangle on that side.
            uint16_t ausSides[3];
            pTri->GetSides(&ausSides[0], &ausSides[1], &ausSides[2]);
            for (uint16_t ii = 0; ii < 3; ++ii) {
                int32_t iNextLinePoint = static_cast<int32_t>(iEdgeCopy + 1);
                bool bPortal = pTri->EdgeHasExtraInfo(ii);
                if (bDrawConnections && ausSides[ii] != 0xFFFF) {
                    NavMeshPtr spOtherNavMesh(spNavMesh);
                    NavMeshTriangle* pOtherTri = nullptr;
                    bool bFound = false;
                    uint16_t usOtherEdge = 0xFFFF;
                    if (bPortal) {
                        EdgeExtraInfo* pExtraEdge = spNavMesh->GetEdgeExtraInfo(static_cast<uint16_t>(iTri), ii);
                        if (pExtraEdge && pExtraEdge->Portal.pOtherMesh &&
                            pExtraEdge->Portal.pOtherMesh->HasNavMesh()) {
                            pExtraEdge->Portal.pOtherMesh->GetNavMeshEvenDisabled(spOtherNavMesh);
                            uint32_t uiOtherTriangle = pExtraEdge->Portal.sTriangle;
                            pOtherTri = spOtherNavMesh->GetTriangle(static_cast<uint16_t>(uiOtherTriangle));
                            uint16_t ausOtherSides[3];
                            pOtherTri->GetSides(&ausOtherSides[0], &ausOtherSides[1], &ausOtherSides[2]);
                            // A portal back to this triangle of this navmesh.
                            for (int32_t iV = 0; iV < 3; ++iV) {
                                if (!pOtherTri->EdgeHasExtraInfo(iV)) {
                                    continue;
                                }
                                EdgeExtraInfo* pOtherExtra =
                                    spOtherNavMesh->GetEdgeExtraInfo(static_cast<uint16_t>(uiOtherTriangle), iV);
                                if (!pOtherExtra) {
                                    continue;
                                }
                                NavMeshPtr spBackNavMesh;
                                pOtherExtra->Portal.pOtherMesh->GetNavMeshEvenDisabled(spBackNavMesh);
                                if (spBackNavMesh == spNavMesh && pOtherExtra->Portal.sTriangle == iTri) {
                                    bFound = true;
                                    usOtherEdge = static_cast<uint16_t>(iV);
                                }
                            }
                        }
                    }
                    else {
                        pOtherTri = spNavMesh->GetTriangle(ausSides[ii]);
                        uint16_t ausOtherSides[3];
                        pOtherTri->GetSides(&ausOtherSides[0], &ausOtherSides[1], &ausOtherSides[2]);
                        for (int32_t iV = 0; iV < 3; ++iV) {
                            if (ausOtherSides[iV] == iTri) {
                                bFound = true;
                                usOtherEdge = static_cast<uint16_t>(iV);
                            }
                        }
                    }
                    if (bFound) {
                        // A line from near this edge's middle, inside this triangle, to the same point in the other.
                        uint32_t iConnectionNext = iConnectionCopy + 1;
                        uint16_t usRightVert = ausTriVertices[ii];
                        uint16_t usLeftVert = ausTriVertices[(ii + 1) % 3];
                        NiPoint3 kRightVert = spNavMesh->Vertices.GetAt(usRightVert).Location;
                        NiPoint3 kLeftVert = spNavMesh->Vertices.GetAt(usLeftVert).Location;
                        NiPoint3 kBackVert = spNavMesh->Vertices.GetAt(ausTriVertices[(ii + 2) % 3]).Location;
                        float fClosest = 0.37f;
                        float fSameSideClose = 0.33f;
                        float fBack = 0.3f;
                        NiPoint3 kBack = kBackVert * fBack;
                        NiPoint3 kSameSide = kRightVert * fSameSideClose;
                        NiPoint3 kStartLocation = kLeftVert * fClosest + kSameSide + kBack;
                        uint16_t ausOtherVertices[3];
                        pOtherTri->GetVerts(ausOtherVertices[0], ausOtherVertices[1], ausOtherVertices[2]);
                        NiPoint3 kOtherRightVert =
                            spOtherNavMesh->Vertices.GetAt(ausOtherVertices[usOtherEdge]).Location;
                        NiPoint3 kOtherLeftVert =
                            spOtherNavMesh->Vertices.GetAt(ausOtherVertices[(usOtherEdge + 1) % 3]).Location;
                        NiPoint3 kOtherBackVert =
                            spOtherNavMesh->Vertices.GetAt(ausOtherVertices[(usOtherEdge + 2) % 3]).Location;
                        NiPoint3 kOtherBack = kOtherBackVert * fBack;
                        NiPoint3 kOtherSameSide = kOtherLeftVert * fSameSideClose;
                        NiPoint3 kEndLocation = kOtherRightVert * fClosest + kOtherSameSide + kOtherBack;
                        connectVertList.GetAt(static_cast<int32_t>(iConnectionCopy)) = kStartLocation;
                        connectVertList.GetAt(static_cast<int32_t>(iConnectionNext)) = kEndLocation;
                        connectConnectionList.GetAt(static_cast<int32_t>(iConnectionCopy)) = 1;
                        connectConnectionList.GetAt(static_cast<int32_t>(iConnectionNext)) = 0;
                        connectColorList.GetAt(static_cast<int32_t>(iConnectionCopy)) =
                            ColorValues[NAVMESH_RENDER_COLOR_TYPE_GREEN];
                        connectColorList.GetAt(static_cast<int32_t>(iConnectionNext)) =
                            ColorValues[NAVMESH_RENDER_COLOR_TYPE_WHITE];
                        iConnectionCopy += 2;
                    }
                }
                // Each shared edge once (from the lower triangle), each portal and open edge from its triangle.
                if (ausSides[ii] > iTri || bPortal) {
                    NiPoint3& rStart = spNavMesh->Vertices.GetAt(ausTriVertices[ii]).Location;
                    edgeVertList.GetAt(static_cast<int32_t>(iEdgeCopy)) = rStart;
                    NiPoint3& rEnd = spNavMesh->Vertices.GetAt(ausTriVertices[(ii + 1) % 3]).Location;
                    edgeVertList.GetAt(iNextLinePoint) = rEnd;
                    edgeConnectionList.GetAt(static_cast<int32_t>(iEdgeCopy)) = 1;
                    edgeConnectionList.GetAt(iNextLinePoint) = 0;
                    if (ausSides[ii] == 0xFFFF) {
                        // An open edge, coloured by its cover.
                        uint16_t usCover = 0;
                        bool bLeft = false;
                        bool bRight = false;
                        if (ii < 2) {
                            pTri->GetEdgeCoverData(ii, usCover, bLeft, bRight);
                        }
                        NAVMESH_RENDER_COLOR_TYPE eEdgeColor;
                        switch (usCover) {
                            case 0: eEdgeColor = NAVMESH_RENDER_COLOR_TYPE_YELLOW; break;
                            case 1: eEdgeColor = NAVMESH_RENDER_COLOR_TYPE_BLACK; break;
                            case 2: eEdgeColor = NAVMESH_RENDER_COLOR_TYPE_DARK_BLUE_GREEN; break;
                            case 15: eEdgeColor = NAVMESH_RENDER_COLOR_TYPE_WHITE; break;
                            default: eEdgeColor = NAVMESH_RENDER_COLOR_TYPE_BLUE; break;
                        }
                        edgeColorList.GetAt(static_cast<int32_t>(iEdgeCopy)) = ColorValues[eEdgeColor];
                        edgeColorList.GetAt(iNextLinePoint) = ColorValues[eEdgeColor];
                        if (bDrawCover && usCover > 1) {
                            float fHeight = NavMeshTriangle::GetCoverHeight(usCover);
                            NiColorA* pCoverColor = &edgeColorList.GetAt(static_cast<int32_t>(iEdgeCopy));
                            NiPoint3* pRightCorner = &edgeVertList.GetAt(iNextLinePoint);
                            NiPoint3* pLeftCorner = &edgeVertList.GetAt(static_cast<int32_t>(iEdgeCopy));
                            NiAVObject* pCoverBox = BSTestObjects::CreateBox(
                                *pLeftCorner, *pRightCorner, fcoverBoxHalfWidth, fcoverBoxHalfWidth, fHeight, 0.0f,
                                pCoverColor, nullptr
                            );
                            spCellNode->AttachChild(pCoverBox, 1);
                            NiPoint3* pEdgeEnd = &edgeVertList.GetAt(iNextLinePoint);
                            NiPoint3 kEdgeDir = edgeVertList.GetAt(static_cast<int32_t>(iEdgeCopy)) - *pEdgeEnd;
                            kEdgeDir.Unitize();
                            kEdgeDir *= NavMesh::DebugScale;
                            if (bLeft) {
                                NiPoint3 kStart = edgeVertList.GetAt(iNextLinePoint) - kEdgeDir;
                                NiPoint3 kEnd = edgeVertList.GetAt(iNextLinePoint) + kEdgeDir;
                                NiAVObject* pLeftEdge = BSTestObjects::CreateBox(
                                    kStart, kEnd, fedgeHalfWidth, fedgeHalfWidth, fHeight + fedgeAdditionalHeight, 0.0f,
                                    &ColorValues[NAVMESH_RENDER_COLOR_TYPE_BLACK], nullptr
                                );
                                spCellNode->AttachChild(pLeftEdge, 1);
                            }
                            if (bRight) {
                                NiPoint3 kStart = edgeVertList.GetAt(static_cast<int32_t>(iEdgeCopy)) - kEdgeDir;
                                NiPoint3 kEnd = edgeVertList.GetAt(static_cast<int32_t>(iEdgeCopy)) + kEdgeDir;
                                NiAVObject* pRightEdge = BSTestObjects::CreateBox(
                                    kStart, kEnd, fedgeHalfWidth, fedgeHalfWidth, fHeight + fedgeAdditionalHeight, 0.0f,
                                    &ColorValues[NAVMESH_RENDER_COLOR_TYPE_BLACK], nullptr
                                );
                                spCellNode->AttachChild(pRightEdge, 1);
                            }
                        }
                    }
                    else {
                        edgeColorList.GetAt(static_cast<int32_t>(iEdgeCopy)) =
                            ColorValues[NAVMESH_RENDER_COLOR_TYPE_NORMAL];
                        edgeColorList.GetAt(iNextLinePoint) = ColorValues[NAVMESH_RENDER_COLOR_TYPE_NORMAL];
                    }
                    iEdgeCopy += 2;
                }
            }
        }
        {
            // The edge lines. The array allocations ask for 0xFFFFFFFF bytes where the size overflows 32 bits.
            uint64_t ullBytes = uint64_t(iEdgeCopy) * sizeof(NiPoint3);
            void* pEdgeVertMemory = ourMalloc(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes));
            NiPoint3* pEdgeVerts;
            if (pEdgeVertMemory) {
                rt::eh_vector_constructor_iterator(
                    pEdgeVertMemory, sizeof(NiPoint3), static_cast<int32_t>(iEdgeCopy), &emptyPointConstructor
                );
                pEdgeVerts = static_cast<NiPoint3*>(pEdgeVertMemory);
            }
            else {
                pEdgeVerts = nullptr;
            }
            ullBytes = uint64_t(iEdgeCopy) * sizeof(NiColorA);
            void* pEdgeColorMemory = ourMalloc(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes));
            NiColorA* pEdgeColors;
            if (pEdgeColorMemory) {
                rt::eh_vector_constructor_iterator(
                    pEdgeColorMemory, sizeof(NiColorA), static_cast<int32_t>(iEdgeCopy),
                    &fnvd::constructElement<NiColorA>
                );
                pEdgeColors = static_cast<NiColorA*>(pEdgeColorMemory);
            }
            else {
                pEdgeColors = nullptr;
            }
            uint8_t* pEdgeConnections = static_cast<uint8_t*>(ourMalloc(iEdgeCopy));
            ourMemcpy(pEdgeVerts, &edgeVertList.GetAt(0), sizeof(NiPoint3) * iEdgeCopy);
            ourMemcpy(pEdgeColors, &edgeColorList.GetAt(0), sizeof(NiColorA) * iEdgeCopy);
            ourMemcpy(pEdgeConnections, &edgeConnectionList.GetAt(0), iEdgeCopy);
            void* pLinesMemory = NiMemObject::operator new(sizeof(NiLines));
            NiLines* pLines = pLinesMemory ? ::new (pLinesMemory) NiLines(
                                                 static_cast<uint16_t>(iEdgeCopy), pEdgeVerts, pEdgeColors, nullptr, 0,
                                                 NiGeometryData::NBT_METHOD_NONE, pEdgeConnections
                                             )
                                           : nullptr;
            NiPointer<NiLines> spLines(pLines);
            void* pLinesShaderMemory = NiMemObject::operator new(sizeof(BSShaderNoLightingProperty));
            BSShaderNoLightingProperty* pLinesShader =
                pLinesShaderMemory ? ::new (pLinesShaderMemory) BSShaderNoLightingProperty : nullptr;
            NiPointer<BSShaderNoLightingProperty> spLinesShaderProp(pLinesShader);
            spLinesShaderProp->SetFlag(BSShaderProperty::BSSP_FLAG_ZBUFFER_WRITE, 0);
            spLines->AttachProperty(spLinesShaderProp);
            spLines->AttachProperty(spEdgeAndVertexMaterialArray[NAVMESH_RENDER_COLOR_TYPE_NORMAL]);
            spLines->AttachProperty(spAlphaProp);
            spLines->SetTranslate(NiPoint3(0.0f, 0.0f, 1.0f));
            // The connection lines, raised above the edge lines.
            NiPointer<NiLines> spConnections(static_cast<NiLines*>(nullptr));
            if (iConnectionCopy) {
                ullBytes = uint64_t(iConnectionCopy) * sizeof(NiPoint3);
                void* pConVertMemory = ourMalloc(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes));
                NiPoint3* pConVerts;
                if (pConVertMemory) {
                    rt::eh_vector_constructor_iterator(
                        pConVertMemory, sizeof(NiPoint3), static_cast<int32_t>(iConnectionCopy), &emptyPointConstructor
                    );
                    pConVerts = static_cast<NiPoint3*>(pConVertMemory);
                }
                else {
                    pConVerts = nullptr;
                }
                ullBytes = uint64_t(iConnectionCopy) * sizeof(NiColorA);
                void* pConColorMemory = ourMalloc(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes));
                NiColorA* pConColors;
                if (pConColorMemory) {
                    rt::eh_vector_constructor_iterator(
                        pConColorMemory, sizeof(NiColorA), static_cast<int32_t>(iConnectionCopy),
                        &fnvd::constructElement<NiColorA>
                    );
                    pConColors = static_cast<NiColorA*>(pConColorMemory);
                }
                else {
                    pConColors = nullptr;
                }
                uint8_t* pConConnections = static_cast<uint8_t*>(ourMalloc(iConnectionCopy));
                ourMemcpy(pConVerts, &connectVertList.GetAt(0), sizeof(NiPoint3) * iConnectionCopy);
                ourMemcpy(pConColors, &connectColorList.GetAt(0), sizeof(NiColorA) * iConnectionCopy);
                ourMemcpy(pConConnections, &connectConnectionList.GetAt(0), iConnectionCopy);
                void* pConLinesMemory = NiMemObject::operator new(sizeof(NiLines));
                NiLines* pConLines = pConLinesMemory
                    ? ::new (pConLinesMemory) NiLines(
                          static_cast<uint16_t>(iConnectionCopy), pConVerts, pConColors, nullptr, 0,
                          NiGeometryData::NBT_METHOD_NONE, pConConnections
                      )
                    : nullptr;
                spConnections = pConLines;
                void* pConShaderMemory = NiMemObject::operator new(sizeof(BSShaderNoLightingProperty));
                BSShaderNoLightingProperty* pConShader =
                    pConShaderMemory ? ::new (pConShaderMemory) BSShaderNoLightingProperty : nullptr;
                spLinesShaderProp = pConShader;
                spLinesShaderProp->SetFlag(BSShaderProperty::BSSP_FLAG_ZBUFFER_WRITE, 0);
                spConnections->AttachProperty(spLinesShaderProp);
                spConnections->AttachProperty(spAlphaProp);
                spConnections->SetTranslate(NiPoint3(0.0f, 0.0f, 3.0f));
            }
            // The triangle shape.
            ullBytes = uint64_t(uiTotalSize) * sizeof(NiPoint3);
            void* pTriVertMemory = ourMalloc(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes));
            NiPoint3* pTriVerts;
            if (pTriVertMemory) {
                rt::eh_vector_constructor_iterator(
                    pTriVertMemory, sizeof(NiPoint3), static_cast<int32_t>(uiTotalSize), &emptyPointConstructor
                );
                pTriVerts = static_cast<NiPoint3*>(pTriVertMemory);
            }
            else {
                pTriVerts = nullptr;
            }
            ullBytes = uint64_t(uiTotalSize) * sizeof(NiColorA);
            void* pTriColorMemory = ourMalloc(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes));
            NiColorA* pTriColors;
            if (pTriColorMemory) {
                rt::eh_vector_constructor_iterator(
                    pTriColorMemory, sizeof(NiColorA), static_cast<int32_t>(uiTotalSize),
                    &fnvd::constructElement<NiColorA>
                );
                pTriColors = static_cast<NiColorA*>(pTriColorMemory);
            }
            else {
                pTriColors = nullptr;
            }
            ourMemcpy(pTriVerts, &triVertList.GetAt(0), sizeof(NiPoint3) * uiTotalSize);
            ourMemcpy(pTriColors, &triColorList.GetAt(0), sizeof(NiColorA) * uiTotalSize);
            void* pShapeMemory = NiMemObject::operator new(sizeof(NiTriShape));
            NiTriShape* pShape = pShapeMemory
                ? ::new (pShapeMemory) NiTriShape(
                      static_cast<uint16_t>(uiTotalSize), pTriVerts, nullptr, pTriColors, nullptr, 0,
                      NiGeometryData::NBT_METHOD_NONE, static_cast<uint16_t>(uiTriSize), pusTriList
                  )
                : nullptr;
            NiPointer<NiTriShape> spTriangles(pShape);
            NiFixedString kName("NavMesh Triangles");
            spTriangles->SetName(kName);
            void* pShaderMemory = NiMemObject::operator new(sizeof(BSShaderNoLightingProperty));
            BSShaderNoLightingProperty* pShader =
                pShaderMemory ? ::new (pShaderMemory) BSShaderNoLightingProperty : nullptr;
            NiPointer<BSShaderNoLightingProperty> spShaderProp(pShader);
            spShaderProp->SetFlag(BSShaderProperty::BSSP_FLAG_ZBUFFER_WRITE, 0);
            spTriangles->AttachProperty(spTriangleMaterialArray[NAVMESH_RENDER_COLOR_TYPE_NORMAL]);
            spTriangles->AttachProperty(spAlphaProp);
            spTriangles->AttachProperty(spShaderProp);
            spTriangles->SetTranslate(NiPoint3(0.0f, 0.0f, 1.0f));
            // A translucent dark red sphere per static avoid node.
            for (uint32_t uiAvoid = 0; uiAvoid < spNavMesh->GetStaticAvoidNodeCount(); ++uiAvoid) {
                NavMeshStaticAvoidNode* pAvoidNode = spNavMesh->GetStaticAvoidNode(uiAvoid);
                NiColorA kLightDarkRed(0.25f, 0.0f, 0.0f, 0.5f);
                NiPointer<NiAVObject> spSphere(
                    BSTestObjects::CreateSphere(pAvoidNode->fRadius, 10, 10, &kLightDarkRed)
                );
                void* pAlphaMemory = NiMemObject::operator new(sizeof(NiAlphaProperty));
                NiAlphaProperty* pAlpha = pAlphaMemory ? ::new (pAlphaMemory) NiAlphaProperty : nullptr;
                NiPointer<NiAlphaProperty> spAlpha(pAlpha);
                spAlpha->SetAlphaBlending(true);
                spSphere->AttachProperty(spAlpha);
                spSphere->SetTranslate(pAvoidNode->Point1);
                spCellNode->AttachChild(spSphere, 1);
            }
            spCellNode->AttachChild(spLines, 1);
            spCellNode->AttachChild(spTriangles, 1);
            if (iConnectionCopy) {
                spCellNode->AttachChild(spConnections, 1);
            }
            AddNoLightingPropertyRecurse(spCellNode);
            BSShaderManager::PrepareObject(spCellNode, 0, 0);
        }
    }
    if (abUpdate) {
        NiUpdateData kUpdateData(0.0f, false, false);
        spRootDrawNode->Update(kUpdateData);
        spRootDrawNode->UpdateProperties();
    }
}

void NavMeshRender::UpdateRenderOnlyNavMeshes() {
    TESWorldSpace* worldSpace;
    NavMeshRender* singleton;
    TESObjectCELL* cellFromCellCoord;
    int32_t j;
    int32_t i;
    int32_t v7;
    SetDraw(false);
    TESObjectCELL* currentCell = TES::pInstance->GetCurrentCell();
    if (currentCell) {
        if (currentCell->GetInterior()) {
            singleton = NavMeshRender::GetInstance();
            singleton->AddDrawOnlyNavMeshes(currentCell, false);
        }
        else {
            v7 = ((int32_t)((&uGridsToLoad)->UInt() - 1)) / 2;
            for (i = currentCell->GetDataX() - v7; i <= (v7 + currentCell->GetDataX()); ++i) {
                for (j = currentCell->GetDataY() - v7; j <= (v7 + currentCell->GetDataY()); ++j) {
                    worldSpace = currentCell->GetWorldSpace();
                    cellFromCellCoord = worldSpace->GetCellFromCellCoord(i, j);
                    AddDrawOnlyNavMeshes(cellFromCellCoord, false);
                }
            }
        }
    }
    SetDraw(true);
}

void NavMeshRender::Update() {
    NiNode* v1;
    NiNode* v2;
    NiNode* v3;
    NiNode* v4;
    NiNode* v5;
    NiNode* v6;
    NiNode* v7;
    NiNode* parent;
    if ((&spSelectedPathGeometry)->m_pObject) {
        v1 = (&spSelectedPathGeometry)->m_pObject;
        if (v1->NiAVObject::GetParent()) {
            v2 = (&spSelectedPathGeometry)->m_pObject;
            parent = v2->NiAVObject::GetParent();
            v3 = (&spSelectedPathGeometry)->m_pObject;
            parent->DetachChild(v3);
        }
    }
    if ((&spLOSMapGeometry)->m_pObject) {
        v4 = (&spLOSMapGeometry)->m_pObject;
        if (v4->NiAVObject::GetParent()) {
            v5 = (&spLOSMapGeometry)->m_pObject;
            v7 = v5->NiAVObject::GetParent();
            v6 = (&spLOSMapGeometry)->m_pObject;
            v7->DetachChild(v6);
        }
    }
    (&spSelectedPathGeometry)->operator=(nullptr);
    (&spLOSMapGeometry)->operator=(nullptr);
}

// Showing updates the root draw node; hiding also detaches its children and forgets the cells drawn.
void NavMeshRender::SetDraw(bool abDraw) {
    NiUpdateData kShowData;
    NiUpdateData kHideData;
    if (abDraw) {
        spRootDrawNode->UpdateProperties();
        kShowData.~NiUpdateData();
        ::new (&kShowData) NiUpdateData(0.0f, false, false);
        spRootDrawNode->Update(kShowData);
    }
    else {
        spRootDrawNode->RemoveAllChildren();
        spRootDrawNode->UpdateProperties();
        kHideData.~NiUpdateData();
        ::new (&kHideData) NiUpdateData(0.0f, false, false);
        spRootDrawNode->Update(kHideData);
        NavMeshCellMap.RemoveAll();
    }
    bDraw = abDraw;
}

void NavMeshRender::SetTransparentOverLayMode(bool abTransparent) {
    NiMaterialProperty* v2;
    NiMaterialProperty* v3;
    NiMaterialProperty* v4;
    NiMaterialProperty* v5;
    NiMaterialProperty* v6;
    NiMaterialProperty* v7;
    NiMaterialProperty* v8;
    NiMaterialProperty* v9;
    NiMaterialProperty* v10;
    NiMaterialProperty* v11;
    NiMaterialProperty* v12;
    NiMaterialProperty* v13;
    NiMaterialProperty* v14;
    NiMaterialProperty* v15;
    NiMaterialProperty* v16;
    NiMaterialProperty* v17;
    NiMaterialProperty* v18;
    NiMaterialProperty* v19;
    NiMaterialProperty* v20;
    NiMaterialProperty* v21;
    NiMaterialProperty* v22;
    v2 = spTriangleMaterialArray->operator->();
    if (abTransparent) {
        v2->SetAlpha(0.10000000149011612f);
        v3 = (&spTriangleMaterialArray[7])->m_pObject;
        v3->SetAlpha(0.10000000149011612f);
        v4 = (&spTriangleMaterialArray[8])->m_pObject;
        v4->SetAlpha(0.10000000149011612f);
        v5 = (&spTriangleMaterialArray[4])->m_pObject;
        v5->SetAlpha(0.17499999701976776f);
        v6 = (&spTriangleMaterialArray[5])->m_pObject;
        v6->SetAlpha(0.17499999701976776f);
        v7 = spEdgeAndVertexMaterialArray->operator->();
        v7->SetAlpha(0x1p-2f);
        v8 = (&spEdgeAndVertexMaterialArray[6])->m_pObject;
        v8->SetAlpha(0x1p-2f);
        v9 = (&spEdgeAndVertexMaterialArray[2])->m_pObject;
        v9->SetAlpha(0x1p-2f);
        v10 = (&spEdgeAndVertexMaterialArray[7])->m_pObject;
        v10->SetAlpha(0x1p-2f);
        v11 = (&spEdgeAndVertexMaterialArray[9])->m_pObject;
        v11->SetAlpha(0x1p-2f);
        v12 = (&spEdgeAndVertexMaterialArray[10])->m_pObject;
        v12->SetAlpha(0.3499999940395355f);
    }
    else {
        v2->SetAlpha(0.17499999701976776f);
        v13 = (&spTriangleMaterialArray[7])->m_pObject;
        v13->SetAlpha(0.17499999701976776f);
        v14 = (&spTriangleMaterialArray[8])->m_pObject;
        v14->SetAlpha(0.17499999701976776f);
        v15 = (&spTriangleMaterialArray[4])->m_pObject;
        v15->SetAlpha(0x1p+0f);
        v16 = (&spTriangleMaterialArray[5])->m_pObject;
        v16->SetAlpha(0x1p+0f);
        v17 = spEdgeAndVertexMaterialArray->operator->();
        v17->SetAlpha(0x1p-1f);
        v18 = (&spEdgeAndVertexMaterialArray[6])->m_pObject;
        v18->SetAlpha(0x1p-1f);
        v19 = (&spEdgeAndVertexMaterialArray[2])->m_pObject;
        v19->SetAlpha(0x1p-1f);
        v20 = (&spEdgeAndVertexMaterialArray[7])->m_pObject;
        v20->SetAlpha(0x1p-1f);
        v21 = (&spEdgeAndVertexMaterialArray[9])->m_pObject;
        v21->SetAlpha(0x1p-1f);
        v22 = (&spEdgeAndVertexMaterialArray[10])->m_pObject;
        v22->SetAlpha(0.75f);
    }
    NiNode* v23 = (&NavMeshRender::spRootDrawNode)->m_pObject;
    BSShaderProperty::SetFlagRecurse(v23, BSShaderProperty::BSSP_FLAG_ZBUFFER_TEST, !abTransparent);
}

void NavMeshRender::RemoveCell(TESObjectCELL* apCell) {
    NiPointer<NiNode> spNode(static_cast<NiNode*>(nullptr));
    if (NavMeshCellMap.GetAt(apCell, spNode)) {
        NiNode* pRoot = NavMeshRender::spRootDrawNode.m_pObject;
        pRoot->DetachChild(spNode.m_pObject);
        NiUpdateData kUpdateData(0.0f, false, false);
        NavMeshRender::spRootDrawNode.m_pObject->Update(kUpdateData);
        NavMeshRender::spRootDrawNode.m_pObject->UpdateProperties();
        NavMeshCellMap.RemoveAt(apCell);
    }
}

NavMeshRender::NAVMESH_RENDER_COLOR_TYPE NavMeshRender::GetTriangleColor(const NavMeshTriangle* apTri) {
    NAVMESH_RENDER_COLOR_TYPE eret = NAVMESH_RENDER_COLOR_TYPE_NORMAL;
    if (apTri->GetObjectCount()) {
        if (apTri->GetFlag(256)) {
            return NAVMESH_RENDER_COLOR_TYPE_OBSTACLE_BOUNDS;
        }
        else {
            return NAVMESH_RENDER_COLOR_TYPE_OBSTACLE;
        }
    }
    else {
        if (apTri->IsPreferred()) {
            if (apTri->HasDoorPortal()) {
                return NAVMESH_RENDER_COLOR_TYPE_DARK_BLUE_GREEN;
            }
            else {
                if (apTri->IsWater()) {
                    return NAVMESH_RENDER_COLOR_TYPE_CYAN;
                }
                else {
                    return NAVMESH_RENDER_COLOR_TYPE_ORANGE;
                }
            }
        }
        else {
            if (apTri->IsWater()) {
                return NAVMESH_RENDER_COLOR_TYPE_BLUE;
            }
            else {
                if (apTri->IsNoLargeCreature()) {
                    return NAVMESH_RENDER_COLOR_TYPE_NOLARGECREATURE;
                }
                else {
                    if (apTri->HasDoorPortal()) {
                        return NAVMESH_RENDER_COLOR_TYPE_BLUE_GREEN;
                    }
                    else {
                        if (apTri->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
                            return NAVMESH_RENDER_COLOR_TYPE_WHITE;
                        }
                        else {
                            if (apTri->GetFlag(NavMeshTriangle::NAV_TRI_AVOID_NODE)) {
                                return NAVMESH_RENDER_COLOR_TYPE_DARK_RED;
                            }
                        }
                    }
                }
            }
        }
    }
    return eret;
}

void NavMeshRender::operator delete(void* apMemory) {
    ourFree(apMemory);
}

NavMeshRender* NavMeshRender::GetInstance() {
    return NavMeshRender::TheRender;
}

void NavMeshRender::InitSingleton() {
    NavMeshRender* v0;
    void* v1 = ourMalloc(sizeof(NavMeshRender));
    if (v1) {
        v0 = ::new (v1) NavMeshRender;
    }
    else {
        v0 = nullptr;
    }
    NavMeshRender::TheRender = v0;
}

void NavMeshRender::Destroy() {
    if (NavMeshRender::TheRender) {
        delete NavMeshRender::TheRender;
    }
    NavMeshRender::TheRender = nullptr;
}
