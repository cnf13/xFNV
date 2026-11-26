#include "Fallout Shared/Pathfinding/PathingLOSMap.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "BSDevices/Xenon/XInputManager.h"
#include "BSMain/BSTestObjects.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavMeshVertex.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

char BSMAX_1(char acA, char acB) {
    if (acA <= acB) {
        return acB;
    }

    return acA;
}

PathingLOSMap::PathingLOSMap(NiPoint3 aCenter, float afRadius)
  : NiRefObject(), NavMeshToOffsetMap(37u), pTriangleScores(nullptr), NavMeshes(), iTotalTriangleCount(0),
    iTotalTrianglesSeen(0), iTotalTrianglesUnseen(0), iMaxScore(0), iMinScore(0), Center(aCenter), fRadius(afRadius) {}

PathingLOSMap::~PathingLOSMap() {
    if (pTriangleScores) {
        ourFree(pTriangleScores);
    }
}

char PathingLOSMap::GetTriangleScore(uint32_t aiIndex) {
    return pTriangleScores[aiIndex];
}

float PathingLOSMap::GetSeenPercent() const {
    int32_t iTotal = static_cast<int32_t>(iTotalTrianglesUnseen + iTotalTrianglesSeen);
    if (!iTotal) {
        return 1.0;
    }
    return static_cast<float>(
        static_cast<double>(iTotalTrianglesSeen) / static_cast<double>(static_cast<uint32_t>(iTotal))
    );
}

bool PathingLOSMap::GetNavMeshAndTriangleForIndex(uint32_t aiIndex, NavMeshPtr& aRetNavMesh, uint16_t& aRetTriangle) {
    uint32_t uiOffset;
    int32_t i;
    if (NavMeshes.IsEmpty()) {
        return false;
    }
    uint32_t uiPrevOffset = 0;
    for (i = 0; static_cast<uint32_t>(i) < NavMeshes.iSize; ++i) {
        void* pMesh = NavMeshes.GetAt(i).m_pObject;
        uint32_t uiNavMeshID = static_cast<TESEnchantableForm*>(pMesh)->TESEnchantableForm::GetCastingType();
        if (!NavMeshToOffsetMap.GetAt(uiNavMeshID, uiOffset)) {
            return false;
        }
        if (uiOffset > aiIndex) {
            break;
        }
        uiPrevOffset = uiOffset;
    }
    aRetTriangle = static_cast<uint16_t>(aiIndex - uiPrevOffset);
    aRetNavMesh = NavMeshes.GetAt(i - 1);
    return true;
}

bool PathingLOSMap::GetOffsetForNavMesh(uint32_t aNavMeshID, uint32_t& aRetOffset) {
    return NavMeshToOffsetMap.GetAt(aNavMeshID, aRetOffset);
}

char* PathingLOSMap::GetArrayForNavMesh(uint32_t aNavMeshID) {
    uint32_t uiOffset;
    if (NavMeshToOffsetMap
            .NiTMapBase<DFALL<NiTMapItem<uint32_t, uint32_t>>, uint32_t, uint32_t>::GetAt(aNavMeshID, uiOffset)) {
        return pTriangleScores + uiOffset;
    }
    return nullptr;
}

void PathingLOSMap::SetAt(int32_t aiIndex, char acScore) {
    pTriangleScores[aiIndex] = acScore;
}

uint32_t PathingLOSMap::GetNavMeshCount() const {
    return NavMeshes.iSize;
}

bool PathingLOSMap::Setup(const BSSimpleArray<NavMeshPtr, 1024>& aNavMeshes) {
    iTotalTriangleCount = 0;
    for (const NavMeshPtr& spMesh : aNavMeshes) {
        NavMeshToOffsetMap.SetAt(spMesh->iFormID, iTotalTriangleCount);
        iTotalTriangleCount += spMesh->GetTriangleCount();
        NavMeshes.Add(spMesh);
    }
    if (pTriangleScores) {
        ourFree(pTriangleScores);
    }
    pTriangleScores = static_cast<char*>(ourMalloc(iTotalTriangleCount));
    for (uint32_t i = 0; i < iTotalTriangleCount; ++i) {
        pTriangleScores[i] = 127;
    }
    return true;
}

bool PathingLOSMap::GetUnseenNavMeshAndTriangle(NavMeshPtr& aRetNavMesh, uint16_t& aRetTriangle) {
    if (NavMeshes.IsEmpty()) {
        return false;
    }
    for (uint32_t i = 0; i < iTotalTriangleCount; ++i) {
        if (GetTriangleScore(i) <= 0 && GetNavMeshAndTriangleForIndex(i, aRetNavMesh, aRetTriangle)) {
            return true;
        }
    }
    return false;
}

NiNode* PathingLOSMap::CreateDebugGeometry() {
    NiNode* pLOSGeometry = new NiNode(0);
    void* pAlphaProp = new NiAlphaProperty;
    NiPointer<NiAlphaProperty> spAlphaProp(static_cast<NiAlphaProperty*>(pAlphaProp));
    spAlphaProp->SetAlphaBlending(true);
    void* pMaterial = new NiMaterialProperty;
    NiPointer<NiMaterialProperty> spmat(static_cast<NiMaterialProperty*>(pMaterial));
    spmat->SetEmittance(NiColor(1.0f, 1.0f, 1.0f));
    spmat->SetAlpha(0.75f);
    pLOSGeometry->AttachProperty(spAlphaProp.m_pObject);
    pLOSGeometry->AttachProperty(spmat.m_pObject);
    char cLargestVal = -127;
    char cSmallestVal = 127;
    for (uint32_t i = 0; i < iTotalTriangleCount; ++i) {
        char cScore = GetTriangleScore(i);
        if (cScore != 127) {
            cLargestVal = BSMAX_1(cLargestVal, cScore);
            cSmallestVal = BSMIN(cSmallestVal, cScore);
        }
    }
    pLOSGeometry->SetChildArraySize(iTotalTriangleCount + 10);
    for (uint32_t i = 0; i < iTotalTriangleCount; ++i) {
        uint16_t usTriangle;
        NavMeshPtr spMesh;
        if (GetNavMeshAndTriangleForIndex(i, spMesh, usTriangle)) {
            char cScore = GetTriangleScore(i);
            if (cScore != 127) {
                double dGreenScale;
                double dRedScale;
                if (cScore >= 0) {
                    dRedScale = 0.0;
                }
                else {
                    dRedScale = static_cast<double>(cScore) / static_cast<double>(cSmallestVal) * 0.5 + 0.5;
                }
                float fRed = dRedScale;
                if (cScore <= 0) {
                    dGreenScale = 0.0;
                }
                else {
                    dGreenScale = static_cast<double>(cScore) / static_cast<double>(cLargestVal) * 0.5 + 0.5;
                }
                float fGreen = dGreenScale;
                NiColorA kColor(fRed, fGreen, 0.0f, 0.75f);
                NiPoint3 verts[3];
                NavMeshTriangle* pTriangle = spMesh->GetTriangle(usTriangle);
                if (!pTriangle->GetFlag(NavMeshTriangle::NAV_TRI_DISABLED)) {
                    for (uint32_t v = 0; v < 3; ++v) {
                        uint16_t usVert = pTriangle->GetVert(v);
                        NavMeshVertex* pVertex = spMesh->GetVertex(usVert);
                        verts[v] = pVertex->Location;
                        verts[v].z = verts[v].z + 5.0;
                    }
                    NiPointer<NiTriShape> spTri(MakeTriangle(verts[0], verts[1], verts[2], kColor, true));
                    pLOSGeometry->AttachChild(spTri.m_pObject, 1);
                }
            }
        }
    }
    if (fRadius != 0.0) {
        NiNode* pRadiusDebugGeometry = new NiNode(0);
        NiColorA color(1.0f, 1.0f, 0.0f, 0.5f);
        for (uint32_t i = 0; i < 5; ++i) {
            NiAVObject* pRadiusCircle = BSTestObjects::CreateCircle(fRadius, 36, &color);
            float fZ = static_cast<double>(i) * 25.0 + Center.z;
            pRadiusCircle->SetTranslate(Center.x, Center.y, fZ);
            pRadiusDebugGeometry->AttachChild(pRadiusCircle, 1);
        }
        NiAVObject* pCenterCylinder = BSTestObjects::CreateCappedCylinder(16.0f, 16.0f, 400.0f, 10, 10, &color);
        pCenterCylinder->SetTranslate(Center);
        pRadiusDebugGeometry->AttachChild(pCenterCylinder, 1);
        AddNoLightingPropertyRecurse(pRadiusDebugGeometry);
        BSShaderManager::PrepareObject(pRadiusDebugGeometry, 0, 0);
        pLOSGeometry->AttachChild(pRadiusDebugGeometry, 1);
    }
    pLOSGeometry->UpdateProperties();
    NiUpdateData UpdateData(0.0f, false, false);
    pLOSGeometry->Update(UpdateData);
    return pLOSGeometry;
}

void PathingLOSMap::ClearUnseenTriangles() {
    for (uint32_t i = 0; i < iTotalTriangleCount; ++i) {
        if (GetTriangleScore(i) <= 0) {
            SetAt(i, 127);
        }
    }
}

bool PathingLOSMap::IsNavMeshInMap(uint32_t aiNavMeshID) {
    uint32_t uiOffset;
    return NavMeshToOffsetMap.GetAt(aiNavMeshID, uiOffset);
}
