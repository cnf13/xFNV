#include "Fallout Shared/Pathfinding/PathingLOSMap.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"

char BSMAX_1(char a1, char a2) {
    if (a1 <= a2) {
        return a2;
    }
    else {
        return a1;
    }
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
    int32_t v3 = (int32_t)(iTotalTrianglesUnseen + iTotalTrianglesSeen);
    if (!v3) {
        return 1.0;
    }
    return (float)(((double)iTotalTrianglesSeen) / ((double)((uint32_t)v3)));
}

bool PathingLOSMap::GetNavMeshAndTriangleForIndex(uint32_t aiIndex, NavMeshPtr& aRetNavMesh, uint16_t& aRetTriangle) {
    uint32_t formFlags;
    NavMeshPtr* v6;
    void* v7;
    uint32_t a1;
    uint32_t a2;
    int32_t i;
    bool vCondition1;
    vCondition1 = (&NavMeshes)->IsEmpty();
    if (vCondition1) {
        return (bool)0;
    }
    uint32_t v13 = 0;
    for (i = 0;; ++i) {
        formFlags = (&NavMeshes)->iSize;
        if (((uint32_t)i) >= formFlags) {
            break;
        }
        v6 = &(&NavMeshes)->GetAt(i);
        v7 = v6->m_pObject;
        a1 = static_cast<TESEnchantableForm*>(v7)->TESEnchantableForm::GetCastingType();
        bool vCondition2;
        vCondition2 = !(&NavMeshToOffsetMap)->GetAt(a1, a2);
        if (vCondition2) {
            return (bool)0;
        }
        if (a2 > aiIndex) {
            break;
        }
        v13 = a2;
    }
    aRetTriangle = (uint16_t)(aiIndex - v13);
    NavMeshPtr* v8 = &(&NavMeshes)->GetAt(i - 1);
    aRetNavMesh = *v8;
    return (bool)1;
}

bool PathingLOSMap::GetOffsetForNavMesh(uint32_t aNavMeshID, uint32_t& aRetOffset) {
    return (bool)((uint8_t)(&NavMeshToOffsetMap)
                      ->NiTMapBase<DFALL<NiTMapItem<uint32_t, uint32_t>>, uint32_t, uint32_t>::GetAt(
                          aNavMeshID, aRetOffset
                      ));
}

char* PathingLOSMap::GetArrayForNavMesh(uint32_t aNavMeshID) {
    uint32_t uiOffset;
    if (NavMeshToOffsetMap.NiTMapBase<DFALL<NiTMapItem<uint32_t, uint32_t>>, uint32_t, uint32_t>::GetAt(
            aNavMeshID, uiOffset
        )) {
        return pTriangleScores + uiOffset;
    }
    return nullptr;
}

void PathingLOSMap::SetAt(int32_t a2, char a3) {
    pTriangleScores[a2] = a3;
}

uint32_t PathingLOSMap::GetNavMeshCount() const {
    return (&NavMeshes)->iSize;
}

bool PathingLOSMap::Setup(const BSSimpleArray<NavMeshPtr, 1024>& aNavMeshes) {
    iTotalTriangleCount = 0;
    for (uint32_t uiIndex = 0; uiIndex < aNavMeshes.iSize; ++uiIndex) {
        NavMeshToOffsetMap.SetAt(
            (&aNavMeshes.GetAt(static_cast<int32_t>(uiIndex)))->m_pObject->iFormID, iTotalTriangleCount
        );
        iTotalTriangleCount += (&aNavMeshes.GetAt(static_cast<int32_t>(uiIndex)))->m_pObject->GetTriangleCount();
        NavMeshes.Add(aNavMeshes.GetAt(static_cast<int32_t>(uiIndex)));
    }
    if (pTriangleScores) {
        ourFree(pTriangleScores);
    }
    pTriangleScores = static_cast<char*>(ourMalloc(iTotalTriangleCount));
    for (uint32_t i = 0; i < iTotalTriangleCount; ++i) {
        pTriangleScores[i] = 127;
    }
    return 1;
}

bool PathingLOSMap::GetUnseenNavMeshAndTriangle(NavMeshPtr& aRetNavMesh, uint16_t& aRetTriangle) {
    if (NavMeshes.IsEmpty()) {
        return 0;
    }
    for (uint32_t i = 0; i < iTotalTriangleCount; ++i) {
        if (GetTriangleScore(i) <= 0 && GetNavMeshAndTriangleForIndex(i, aRetNavMesh, aRetTriangle)) {
            return 1;
        }
    }
    return 0;
}

NiNode* PathingLOSMap::CreateDebugGeometry() {
    void* v15;
    void* v16;
    NiNode* pLOSGeometry;
    NiColor dst;
    void* apMemory = NiMemObject::operator new(sizeof(NiNode));
    if (apMemory) {
        pLOSGeometry = ::new (apMemory) NiNode(0);
    }
    else {
        pLOSGeometry = 0;
    }
    NiAlphaProperty* v24 = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
    if (v24) {
        v16 = (void*)::new (v24) NiAlphaProperty;
    }
    else {
        v16 = 0;
    }
    NiRefObject* v25 = static_cast<NiRefObject*>(v16);
    NiPointer<NiAlphaProperty> spAlphaProp(static_cast<NiAlphaProperty*>(v16));
    spAlphaProp.m_pObject->SetAlphaBlending(1);
    NiMaterialProperty* v22 = static_cast<NiMaterialProperty*>(NiMemObject::operator new(sizeof(NiMaterialProperty)));
    if (v22) {
        v15 = (void*)::new (v22) NiMaterialProperty;
    }
    else {
        v15 = 0;
    }
    NiRefObject* v23 = static_cast<NiRefObject*>(v15);
    NiPointer<NiMaterialProperty> spmat(static_cast<NiMaterialProperty*>(v15));
    NiColor* v1 = FNVD_RECONSTRUCT(NiColor)(&dst, 1.0f, 1.0f, 1.0f);
    spmat.m_pObject->SetEmittance(*v1);
    spmat.m_pObject->SetAlpha(0.75f);
    NiAlphaProperty* a4 = spAlphaProp.m_pObject;
    pLOSGeometry->AttachProperty(a4);
    NiMaterialProperty* a4a = spmat.m_pObject;
    pLOSGeometry->AttachProperty(a4a);
    char ilargestVal = -127;
    char ismallestVal = 127;
    for (uint32_t i = 0;; ++i) {
        uint32_t Location_ = iTotalTriangleCount;
        if (i >= Location_) {
            break;
        }
        char cscore = GetTriangleScore(i);
        if (cscore != 127) {
            ilargestVal = BSMAX_1(ilargestVal, cscore);
            ismallestVal = BSMIN(ismallestVal, cscore);
        }
    }
    uint32_t a4b = iTotalTriangleCount + 10;
    pLOSGeometry->SetChildArraySize(a4b);
    for (uint32_t i_2 = 0;; ++i_2) {
        uint16_t ustriangle;
        uint32_t v3 = iTotalTriangleCount;
        if (i_2 >= v3) {
            break;
        }
        NavMeshPtr spmesh;
        if (GetNavMeshAndTriangleForIndex(i_2, spmesh, ustriangle)) {
            char cscore_2 = GetTriangleScore(i_2);
            if (cscore_2 != 127) {
                double fgreenvalue;
                double fredvalue;
                NiPoint3 verts[3];
                if (cscore_2 >= 0) {
                    fredvalue = 0.0;
                }
                else {
                    fredvalue = static_cast<double>(cscore_2) / static_cast<double>(ismallestVal) * 0.5 + 0.5;
                }
                float fredvalue_2 = fredvalue;
                if (cscore_2 <= 0) {
                    fgreenvalue = 0.0;
                }
                else {
                    fgreenvalue = static_cast<double>(cscore_2) / static_cast<double>(ilargestVal) * 0.5 + 0.5;
                }
                float fgreenvalue_2 = fgreenvalue;
                NiColorA colora(fredvalue_2, fgreenvalue_2, 0.0f, 0.75f);
                rt::eh_vector_constructor_iterator(verts, sizeof(NiPoint3), 3, &emptyPointConstructor);
                NavMeshTriangle* ptriangle = spmesh.m_pObject->GetTriangle(ustriangle);
                if (!ptriangle->GetFlag(32)) {
                    for (uint32_t v = 0; v < 3; ++v) {
                        uint16_t Vert = ptriangle->GetVert(v);
                        NavMeshVertex* Vertex = spmesh.m_pObject->GetVertex(Vert);
                        verts[v] = Vertex->Location;
                        verts[v].z = verts[v].z + 5.0;
                    }
                    NiPointer<NiTriShape> spTri(MakeTriangle(verts[0], verts[1], verts[2], colora, 1));
                    NiTriShape* a3 = spTri.m_pObject;
                    pLOSGeometry->AttachChild(a3, 1);
                }
            }
        }
    }
    if (fRadius != 0.0) {
        NiNode* pradiusDebugGeometry;
        NiNode* v20 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
        if (v20) {
            pradiusDebugGeometry = ::new (v20) NiNode(0);
        }
        else {
            pradiusDebugGeometry = 0;
        }
        NiColorA color(1.0f, 1.0f, 0.0f, 0.5f);
        for (uint32_t i_3 = 0; i_3 < 5; ++i_3) {
            NiAVObject* pradiusCircle = BSTestObjects::CreateCircle(fRadius, 36, &color);
            float v11 = static_cast<double>(i_3) * 25.0 + Center.z;
            pradiusCircle->SetTranslate(Center.x, Center.y, v11);
            pradiusDebugGeometry->AttachChild(pradiusCircle, 1);
        }
        NiAVObject* pcenterCylinder = BSTestObjects::CreateCappedCylinder(16.0f, 16.0f, 400.0f, 10, 10, &color);
        pcenterCylinder->SetTranslate(Center);
        pradiusDebugGeometry->AttachChild(pcenterCylinder, 1);
        AddNoLightingPropertyRecurse(pradiusDebugGeometry);
        BSShaderManager::PrepareObject(pradiusDebugGeometry, 0, 0);
        pLOSGeometry->AttachChild(pradiusDebugGeometry, 1);
    }
    pLOSGeometry->UpdateProperties();
    NiUpdateData UpdateData(0.0f, 0, 0);
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
    uint32_t a2;
    return NavMeshToOffsetMap.GetAt(aiNavMeshID, a2);
}

void NiAVObjectArray_SetSize(NiTObjectArray<NiPointer<NiAVObject>>* apArray, uint32_t auiSize) {
    FNVD_INLINED_LOCAL(NiPointer<NiAVObject>, v6, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiPointer<NiAVObject>, apMemory, std::nullopt, std::nullopt);
    if (auiSize != apArray->m_usMaxSize) {
        uint16_t i;
        if (auiSize < apArray->m_usSize) {
            for (i = auiSize; i < static_cast<int32_t>(apArray->m_usSize); ++i) {
                NiPointer<NiAVObject>* ptr = ::new (&apMemory) NiPointer<NiAVObject>(nullptr);
                bool v8 = apArray->m_pBase[i] != *ptr;
                apMemory.~NiPointer<NiAVObject>();
                if (v8) {
                    NiPointer<NiAVObject>* v3 = ::new (&v6) NiPointer<NiAVObject>(nullptr);
                    apArray->m_pBase[i].operator=(*v3);
                    v6.~NiPointer<NiAVObject>();
                    --apArray->m_usESize;
                }
            }
            apArray->m_usSize = auiSize;
        }
        NiPointer<NiAVObject>* pSaveBase = apArray->m_pBase;
        apArray->m_usMaxSize = auiSize;
        if (auiSize) {
            apArray->m_pBase = NiTNewInterface<NiPointer<NiAVObject>>::Allocate(apArray->m_usMaxSize);
            for (i = 0; i < static_cast<int32_t>(apArray->m_usSize); ++i) {
                apArray->m_pBase[i].operator=(pSaveBase[i]);
            }
            for (i = apArray->m_usSize; i < static_cast<int32_t>(apArray->m_usMaxSize); ++i) {
                NiPointer<NiAVObject> v5(nullptr);
                NiPointer<NiAVObject>* v2 = &v5;
                apArray->m_pBase[i].operator=(*v2);
            }
        }
        else {
            apArray->m_pBase = 0;
        }
        NiAVObjectArray_Deallocate(pSaveBase);
    }
}

void NiNode::SetChildArraySize(uint32_t aiSize) {
    NiAVObjectArray_SetSize(&m_kChildren, aiSize);
}
