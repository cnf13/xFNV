#include "Fallout Shared/FaceGen/BSFaceGenModel.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSCloningProcess.h"
#include "BSShader/BSFadeNode.h"
#include "BSSystem/BSSystemFile.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenBaseMorphExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenImage.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModelExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenMorph.h"
#include "Fallout Shared/FaceGen/BSFaceGenMorphDataHair.h"
#include "Fallout Shared/FaceGen/BSFaceGenUtils.h"
#include "Fallout Shared/FaceGen/EGMData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Interface/FOVATSEffectManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryDataIterators.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/FaceGen/EgtFileIO_Sdk.h"
#include "SDK/FaceGen/MatrixVT.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/MSVC/exception.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/string.h"

#include <cstdint>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <vector>

NiRTTI BSFaceGenModelExtraData::ms_RTTI("BSFaceGenModelExtraData", &NiExtraData::ms_RTTI);

using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat = std::
    vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat = std::
    vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;

using std_nvector_lFR2Vect2TC_lfloat_g_cstd_nallocator_lFR2Vect2TC_lfloat = std::
    vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat = std::
    vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect2TC_lfloat_g_cstd_nallocator_lFR2Vect2TC_lfloat = std::
    vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat = std::
    vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect2TC_lfloat_g_cstd_nallocator_lFR2Vect2TC_lfloat = std::
    vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>>;
using vector_FR2Vect2TC_float = std::vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>>;

using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat = std::
    vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat = std::
    vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;

using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat = std::
    vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect2TC_lfloat_g_cstd_nallocator_lFR2Vect2TC_lfloat = std::
    vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat = std::
    vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat = std::
    vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;

// Each difference and then each static morph of the .tri file goes to the slot its label names: the first matching
// expression, else modifier, phoneme or custom morph. A slot array is allocated, cleared, on its first morph. A static
// morph replaces a difference morph in its slot, with a warning.
BSFaceGenMorphDataHead::BSFaceGenMorphDataHead(FffTriFileDataS* apFaceGenTri)
  : ppExpressionMorphs(nullptr), ppModifierMorphs(nullptr), ppPhonemeMorphs(nullptr), ppCustomMorphs(nullptr) {
    if (!apFaceGenTri) {
        return;
    }
    BSFaceGenMorph*** const apppMorphs[4] = {&ppExpressionMorphs, &ppModifierMorphs, &ppPhonemeMorphs, &ppCustomMorphs};
    const uint32_t auiCounts[4] = {15, 17, 16, 1};
    static const char* const apWarnings[4] = {
        "MODELS: Statistical and Differential FaceGen morphs found for expression \"%s\".  "
        "Only statistical will be used.",
        "MODELS: Statistical and Differential FaceGen morphs found for modifier \"%s\".  "
        "Only statistical will be used.",
        "MODELS: Statistical and Differential FaceGen morphs found for phoneme \"%s\".  "
        "Only statistical will be used.",
        "MODELS: Statistical and Differential FaceGen morphs found for Custom Morph \"%s\".  "
        "Only statistical will be used.",
    };
    // The slot of the morph labelled apLabel and its type in auiType; null when no type names it.
    auto FindSlot = [&](const char* apLabel, uint32_t& auiType) -> BSFaceGenMorph** {
        for (auiType = 0; auiType < 4; ++auiType) {
            for (uint32_t i = 0; i < auiCounts[auiType]; ++i) {
                const char* pText;
                switch (auiType) {
                    case 0:
                        pText = BSFaceGenManager::GetFaceGenExpressionText(static_cast<BS_FACEGEN_EXPRESSION>(i));
                        break;
                    case 1:
                        pText = BSFaceGenManager::GetFaceGenModifierText(static_cast<BS_FACEGEN_MODIFIER>(i));
                        break;
                    case 2: pText = BSFaceGenManager::GetFaceGenPhonemeText(static_cast<BS_FACEGEN_PHONEME>(i)); break;
                    default: pText = BSFaceGenManager::GetCustomMorphText(i); break;
                }
                if (BSstricmp(pText, apLabel)) {
                    continue;
                }
                BSFaceGenMorph**& rppMorphs = *apppMorphs[auiType];
                if (!rppMorphs) {
                    rppMorphs = static_cast<BSFaceGenMorph**>(ourMalloc(auiCounts[auiType] * sizeof(BSFaceGenMorph*)));
                    ourMemset(rppMorphs, 0, auiCounts[auiType] * sizeof(BSFaceGenMorph*));
                }
                return &rppMorphs[i];
            }
        }
        return nullptr;
    };
    for (uint32_t imorph = 0; imorph < apFaceGenTri->diffMorphList.size(); ++imorph) {
        const FffTriDiffMorphS& rMorph = apFaceGenTri->diffMorphList[imorph];
        uint32_t uiType;
        BSFaceGenMorph** ppSlot = FindSlot(rMorph.label.c_str(), uiType);
        if (!ppSlot) {
            continue;
        }
        uint32_t uiVertCount = static_cast<uint32_t>(rMorph.vtxDelta.size());
        BSFaceGenMorphDifferential* pMorph = new BSFaceGenMorphDifferential(uiVertCount);
        *ppSlot = pMorph;
        for (uint32_t ivert = 0; ivert < uiVertCount; ++ivert) {
            const FR2Vect3TC<float>& rDelta = rMorph.vtxDelta[ivert];
            pMorph->pVerts[ivert] = NiPoint3(rDelta.x1, rDelta.x2, rDelta.x3);
        }
    }
    for (uint32_t imorph = 0; imorph < apFaceGenTri->statMorphList.size(); ++imorph) {
        const FffTriStatMorphS& rMorph = apFaceGenTri->statMorphList[imorph];
        uint32_t uiType;
        BSFaceGenMorph** ppSlot = FindSlot(rMorph.label.c_str(), uiType);
        if (!ppSlot) {
            continue;
        }
        if (*ppSlot) {
            Warning(apWarnings[uiType], rMorph.label.c_str());
            delete *ppSlot;
        }
        uint32_t uiVertCount = static_cast<uint32_t>(rMorph.vtxIdxList.size());
        BSFaceGenMorphStatistical* pMorph = new BSFaceGenMorphStatistical(uiVertCount, rMorph.firstVtxOffset);
        *ppSlot = pMorph;
        for (uint32_t ivert = 0; ivert < uiVertCount; ++ivert) {
            pMorph->pWhichMeshVerts[ivert] = rMorph.vtxIdxList[ivert];
        }
    }
}

const NiRTTI* BSFaceGenMorphDataHead::GetRTTI() const {
    return &BSFaceGenMorphDataHead::ms_RTTI;
}

BSFaceGenMorphDataHead::~BSFaceGenMorphDataHead() {
    BSFaceGenMorph** const appMorphs[4] = {ppExpressionMorphs, ppModifierMorphs, ppPhonemeMorphs, ppCustomMorphs};
    const uint32_t auiCounts[4] = {BS_FG_EXPRESSION_COUNT, 17, 16, 1};
    for (uint32_t uiType = 0; uiType < 4; ++uiType) {
        if (!appMorphs[uiType]) {
            continue;
        }
        for (uint32_t i = 0; i < auiCounts[uiType]; ++i) {
            if (appMorphs[uiType][i]) {
                delete appMorphs[uiType][i];
            }
        }
        ourFree(appMorphs[uiType]);
    }
}

BSFaceGenMorph* BSFaceGenMorphDataHead::GetExpressionMorph(BS_FACEGEN_EXPRESSION aeIndex) {
    if (ppExpressionMorphs && (static_cast<uint32_t>(aeIndex) < BS_FG_EXPRESSION_COUNT)) {
        return ppExpressionMorphs[aeIndex];
    }

    return nullptr;
}

BSFaceGenMorph* BSFaceGenMorphDataHead::GetModifierMorph(BS_FACEGEN_MODIFIER aeIndex) {
    if (ppModifierMorphs && (static_cast<uint32_t>(aeIndex) < BS_FG_MODIFIER_COUNT)) {
        return ppModifierMorphs[aeIndex];
    }

    return nullptr;
}

BSFaceGenMorph* BSFaceGenMorphDataHead::GetPhonemeMorph(BS_FACEGEN_PHONEME aeIndex) {
    if (ppPhonemeMorphs && (static_cast<uint32_t>(aeIndex) < BS_FG_PHONEME_COUNT)) {
        return ppPhonemeMorphs[aeIndex];
    }

    return nullptr;
}

BSFaceGenMorph* BSFaceGenMorphDataHead::GetCustomMorph(BS_FACEGEN_CUSTOM_MORPH aeIndex) {
    if (!ppCustomMorphs || aeIndex) {
        return nullptr;
    }

    return *ppCustomMorphs;
}

BSFaceGenModel::BSFaceGenModel() : pMeshData(nullptr), pTextureData(nullptr) {}

// Deletes the mesh data and the texture data.
BSFaceGenModel::~BSFaceGenModel() {
    if (pMeshData) {
        delete pMeshData;
    }
    if (pTextureData) {
        delete pTextureData;
    }
}

// Both halves load even when the first fails; the results are or'ed into one byte.
bool BSFaceGenModel::LoadModel(
    const char* apEGMFilename,
    const char* apNIFFilename,
    const char* apTRIFilename,
    const char* apEGTFilename,
    HAIR_PART aeHairPart,
    bool abIsAccessory
) {
    bool bLoaded = false;
    if (!BSFaceGenManager::GetUseFaceGenHeads()) {
        return false;
    }
    bLoaded |= LoadModelMesh(apEGMFilename, apNIFFilename, apTRIFilename, aeHairPart, abIsAccessory);
    bLoaded |= LoadModelTexture(apEGTFilename);
    return bLoaded;
}

// Its flag word marks the std::string temporary as built before fffReadTriFile and is tested before the temporary's
// destructor, which therefore always runs. aeHairPart picks the child by name for the hat model, and for the normal one
// unless the root has a single child; HAIR_PART_NONE (the head) gets head morph data and the.tri file's vertices past
// the model's as additional vertices, any other part hair morph data.
bool BSFaceGenModel::LoadModelMesh(
    const char* apEGMFilename,
    const char* apNIFFilename,
    const char* apTRIFilename,
    HAIR_PART aeHairPart,
    bool abIsAccessory
) {
    if (pMeshData || !apEGMFilename) {
        return true;
    }
    alignas(AutoMemContext) char acMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (acMemContext) AutoMemContext(
        MEM_CONTEXT_FACEGEN_EGM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenModel.cpp",
        286
    );
    BSFaceGenModelMeshData* pData = new BSFaceGenModelMeshData;
    pMeshData = pData;
    pMeshData->EGMFilename.Set(apEGMFilename, 0);
    if (apNIFFilename) {
        NiNode* pNode = pModelLoader->LoadFile(apNIFFilename + 7, LOD_MULT_NONE, true, false, true, true);
        if (!pNode) {
            Warning("MODELS: Failed to load '%s' in BSFaceGenModel::LoadModelMesh.", apNIFFilename);
            pMemContext->~AutoMemContext();
            return false;
        }
        alignas(BSCloningProcess) char acCloneProc[sizeof(BSCloningProcess)];
        BSCloningProcess* pCloneProc = ::new (acCloneProc) BSCloningProcess(1.0f);
        NiObject* pCopy = TES::pInstance->CreateDeepCopySameTextures(pNode, *pCloneProc);
        pMeshData->spModelRoot = static_cast<NiNode*>(pCopy);
        if (pMeshData->spModelRoot) {
            uint32_t uiCount = pMeshData->spModelRoot->GetArrayCount();
            for (uint32_t i = 0; i < uiCount; ++i) {
                NiAVObject* pChild = pMeshData->spModelRoot->GetAt(i);
                if (pChild) {
                    bool bByName = aeHairPart == HAIR_PART_HAT || (aeHairPart == HAIR_PART_NORMAL && uiCount != 1);
                    if (!bByName || !BSstricmp(pChild->GetName(), pHairPartName[aeHairPart])) {
                        pMeshData->spModel = pChild->IsTriBasedGeom();
                    }
                }
                if (pMeshData->spModel) {
                    break;
                }
            }
            if (pMeshData->spModel) {
                if (!abIsAccessory) {
                    int32_t iType = NiTexturingProperty::GetType();
                    pMeshData->spModel->RemoveProperty(iType);
                }
                pMeshData->spModel->RemoveAllControllers();
                pMeshData->spModel->RemoveAllExtraData();
                if (pMeshData->spModel->GetSkinInstance()) {
                    NiAVObject* pRootParent = pMeshData->spModel;
                    pMeshData->spModel->GetSkinInstance()->SetRootParent(pRootParent);
                }
                pMeshData->spModel->SetSelectiveUpdate(true);
            }
            pNode->UpdateProperties();
            alignas(NiUpdateData) char acUpdateData[sizeof(NiUpdateData)];
            NiUpdateData* pUpdateData = ::new (acUpdateData) NiUpdateData(0.0f, false, false);
            pNode->Update(*pUpdateData);
        }
        pCloneProc->~BSCloningProcess();
    }
    if (apTRIFilename) {
        FffTriFileDataS* pTriData = new FffTriFileDataS;
        alignas(std::string) char acTriName[sizeof(std::string)];
        std::string* pTriName = ::new (acTriName) std::string(apTRIFilename);
        bool bLoaded = false;
        if (fffReadTriFile(*pTriName, *pTriData)) {
            uint32_t uiModelVertices = pMeshData->spModel->GetVertexCount();
            if (uiModelVertices == pTriData->numModelVtx()) {
                bLoaded = true;
            }
        }
        pTriName->~basic_string();
        if (bLoaded) {
            if (aeHairPart != HAIR_PART_NONE) {
                BSFaceGenMorphDataHair* pHair = new BSFaceGenMorphDataHair(pTriData);
                pMeshData->spMorph = pHair;
            }
            else {
                BSFaceGenMorphDataHead* pHead = new BSFaceGenMorphDataHead(pTriData);
                pMeshData->spMorph = pHead;
                uint32_t uiTriVertices = pTriData->vtxListSize();
                if (uiTriVertices > pMeshData->spModel->GetVertexCount()) {
                    uiTriVertices = pTriData->vtxListSize();
                    pMeshData->iAdditionalVerticesCount = uiTriVertices - pMeshData->spModel->GetVertexCount();
                    uint32_t uiCount = pMeshData->iAdditionalVerticesCount;
                    pMeshData->pAdditionalVertices = new NiPoint3[uiCount];
                    for (uint32_t i = 0; i < pMeshData->iAdditionalVerticesCount; ++i) {
                        float fZ = pTriData->vtxListAt(pTriData->numModelVtx() + i).x3;
                        float fY = pTriData->vtxListAt(pTriData->numModelVtx() + i).x2;
                        float fX = pTriData->vtxListAt(pTriData->numModelVtx() + i).x1;
                        alignas(NiPoint3) char acVertex[sizeof(NiPoint3)];
                        NiPoint3* pVertex = ::new (acVertex) NiPoint3(fX, fY, fZ);
                        pMeshData->pAdditionalVertices[i] = *pVertex;
                    }
                }
            }
        }
        if (pTriData) {
            delete pTriData;
        }
    }
    if (pMeshData && pMeshData->spModel) {
        pMeshData->spModel->SetConsistency(NiGeometryData::MUTABLE);
    }
    pMemContext->~AutoMemContext();
    return true;
}

// The memory context is built and torn down by its constructor and destructor; the result of Set is ignored.
bool BSFaceGenModel::LoadModelTexture(const char* apEGTFilename) {
    if (pTextureData || !apEGTFilename) {
        return true;
    }
    alignas(AutoMemContext) char acMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (acMemContext) AutoMemContext(
        MEM_CONTEXT_FACEGEN_EGT,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenModel.cpp",
        448
    );
    BSFaceGenModelTextureData* pData = new BSFaceGenModelTextureData;
    pTextureData = pData;
    pTextureData->EGTFilename.Set(apEGTFilename, 0);
    pMemContext->~AutoMemContext();
    return true;
}

bool BSFaceGenModel::CreateNewMesh(NiPointer<NiTriBasedGeom>& aspReturnMesh) {
    NiPointer<NiGeometryData> spdata(static_cast<NiGeometryData*>(nullptr));
    NiPointer<NiSkinPartition> spskinPartition(static_cast<NiSkinPartition*>(nullptr));
    aspReturnMesh = static_cast<NiTriBasedGeom*>(nullptr);
    if (pMeshData && pMeshData->spModel.m_pObject && pMeshData->spModel->GetModelData()) {
        BSFaceGenBaseMorphExtraData* pmorphBase;
        aspReturnMesh = static_cast<NiTriBasedGeom*>(pMeshData->spModel->Clone());
        NiPointer<NiObject> spTempObject(static_cast<NiObject*>(nullptr));
        pMeshData->spModel->GetModelData()->CreateDeepCopy(spTempObject);
        spdata = static_cast<NiGeometryData*>(spTempObject.m_pObject);
        aspReturnMesh->SetModelData(spdata.m_pObject);
        NiSkinInstance* pskin = aspReturnMesh->GetSkinInstance();
        if (pskin && pskin->GetSkinPartition()) {
            NiPointer<NiObject> spTempSkinObject(static_cast<NiObject*>(nullptr));
            pskin->GetSkinPartition()->CreateDeepCopy(spTempSkinObject);
            spskinPartition = static_cast<NiSkinPartition*>(spTempSkinObject.m_pObject);
            pskin->SetSkinPartition(spskinPartition.m_pObject);
        }

        pmorphBase = new BSFaceGenBaseMorphExtraData(
            aspReturnMesh.m_pObject,
            pMeshData->pAdditionalVertices,
            pMeshData->iAdditionalVerticesCount
        );
        NiExtraData* pkExtra = pmorphBase;
        if (pmorphBase) {
            aspReturnMesh->AddExtraData(pkExtra);
        }
        BSFaceGenModelExtraData* pmodelExtra = new BSFaceGenModelExtraData(this);
        aspReturnMesh->AddExtraData(pmodelExtra);
        return true;
    }

    return false;
}

bool BSFaceGenModel::ApplyCoordinateToNewMesh(
    const FR2MatrixVTC<float> (*apCoord)[2][2],
    NiPointer<NiTriBasedGeom>& aspReturnMesh
) {
    if (!apCoord) {
        return false;
    }
    if (!CreateNewMesh(aspReturnMesh)) {
        return false;
    }
    if (ApplyCoordinateToExistingMesh(apCoord, aspReturnMesh, true)) {
        return true;
    }
    aspReturnMesh = nullptr;
    return false;
}

// Morphs apMesh's vertices by the geometry modes of apCoord (the face's coordinate when apCoord is null), and the
// base-morph extra data's vertex copy with them when the mesh has one.
bool BSFaceGenModel::ApplyCoordinateToExistingMesh(
    const FR2MatrixVTC<float> (*apCoord)[2][2],
    NiTriBasedGeom* apMesh,
    bool abForceBaseMorph
) {
    if (!pMeshData) {
        return false;
    }
    if (!apCoord) {
        apCoord = BSFaceGenManager::GetFaceGenCoord();
    }
    if (!apCoord) {
        return false;
    }
    BSFaceGenManager::LockFaceGenAccess();
    if (!BSFaceGenManager::LoadEGMData(this)) {
        if (!pMeshData->pEGMData) {
            if (!pMeshData->EGMFilename.GetLength()) {
                BSFaceGenManager::UnlockFaceGenAccess();
                return false;
            }
            EGMData* pEGMData = new EGMData(pMeshData->EGMFilename.pString);
            pMeshData->pEGMData = pEGMData;
        }
        if (!pMeshData->pEGMData) {
            BSFaceGenManager::UnlockFaceGenAccess();
            return false;
        }
    }

    void* pvPositions = nullptr;
    BSFaceGenBaseMorphExtraData* pBaseMorph = static_cast<BSFaceGenBaseMorphExtraData*>(
        BSFaceGenUtils::GetFaceGenExtraData<BSFaceGenBaseMorphExtraData>(apMesh)
    );
    NiPoint3* pBaseVerts = pBaseMorph ? pBaseMorph->pVerts : nullptr;
    NiGeometryDataIteratorNiPoint3 kIter;
    if (apMesh) {
        NiGeometryData* pModelData = apMesh->GetModelData();
        if (pModelData && pModelData->LockPackedVertexData(true)) {
            pModelData->GetVerticesIterator(kIter);
            pvPositions = kIter.GetCurrentPosition();
        }
    }
    if (!pvPositions) {
        BSFaceGenManager::UnlockFaceGenAccess();
        return false;
    }

    // The vertices every mode has offsets for.
    uint32_t uiVerts = apMesh->GetVertexCount();
    for (const auto& rBasis : pMeshData->pEGMData->EGMBasis) {
        uint32_t uiModes = rBasis.size();
        for (uint32_t imode = 0; imode < uiModes; ++imode) {
            if (rBasis[imode].vertPos.size() < uiVerts) {
                uiVerts = rBasis[imode].vertPos.size();
            }
        }
    }

    float fWeight = 0.0f;
    for (uint32_t isymm = 0; isymm < FANSYMM_SIZE; ++isymm) {
        uint32_t uiModes = pMeshData->pEGMData->EGMBasis[isymm].size();
        for (uint32_t imode = 0; imode < uiModes; ++imode) {
            const float& fCoord = (*apCoord)[FANTYPE_GEO][isymm][imode][0];
            fWeight = fCoord * pMeshData->pEGMData->EGMBasis[isymm][imode].scale;
            if (fWeight == 0.0 && !abForceBaseMorph) {
                continue;
            }
            NiPoint3* pDest = pBaseVerts;
            FR2Vect3SVector::const_iterator kOffset = pMeshData->pEGMData->EGMBasis[isymm][imode].vertPos.begin();
            uint32_t ivert;
            for (ivert = 0; ivert + 3 < uiVerts; ivert += 4) {
                NiPoint3 kVert0;
                NiPoint3 kVert1;
                NiPoint3 kVert2;
                NiPoint3 kVert3;
                if (pDest) {
                    kVert0 = pDest[0];
                    kVert1 = pDest[1];
                    kVert2 = pDest[2];
                    kVert3 = pDest[3];
                }
                else {
                    kIter.Get(static_cast<int32_t>(ivert), kVert0);
                    kIter.Get(static_cast<int32_t>(ivert + 1), kVert1);
                    kIter.Get(static_cast<int32_t>(ivert + 2), kVert2);
                    kIter.Get(static_cast<int32_t>(ivert + 3), kVert3);
                }
                FR2Vect3TC<int16_t> kOffset0 = *kOffset++;
                FR2Vect3TC<int16_t> kOffset1 = *kOffset++;
                FR2Vect3TC<int16_t> kOffset2 = *kOffset++;
                FR2Vect3TC<int16_t> kOffset3 = *kOffset++;
                kVert0.x = kOffset0.x1 * fWeight + kVert0.x;
                kVert0.y = kOffset0.x2 * fWeight + kVert0.y;
                kVert0.z = kOffset0.x3 * fWeight + kVert0.z;
                kVert1.x = kOffset1.x1 * fWeight + kVert1.x;
                kVert1.y = kOffset1.x2 * fWeight + kVert1.y;
                kVert1.z = kOffset1.x3 * fWeight + kVert1.z;
                kVert2.x = kOffset2.x1 * fWeight + kVert2.x;
                kVert2.y = kOffset2.x2 * fWeight + kVert2.y;
                kVert2.z = kOffset2.x3 * fWeight + kVert2.z;
                kVert3.x = kOffset3.x1 * fWeight + kVert3.x;
                kVert3.y = kOffset3.x2 * fWeight + kVert3.y;
                kVert3.z = kOffset3.x3 * fWeight + kVert3.z;
                kIter.Set(static_cast<int32_t>(ivert), kVert0);
                kIter.Set(static_cast<int32_t>(ivert + 1), kVert1);
                kIter.Set(static_cast<int32_t>(ivert + 2), kVert2);
                kIter.Set(static_cast<int32_t>(ivert + 3), kVert3);
                if (pDest) {
                    pDest[0] = kVert0;
                    pDest[1] = kVert1;
                    pDest[2] = kVert2;
                    pDest[3] = kVert3;
                    pDest += 4;
                }
            }
            while (ivert < uiVerts) {
                NiPoint3 kVert;
                if (pDest) {
                    kVert = *pDest;
                }
                else {
                    kIter.Get(static_cast<int32_t>(ivert), kVert);
                }
                kVert.x = (*kOffset).x1 * fWeight + kVert.x;
                kVert.y = (*kOffset).x2 * fWeight + kVert.y;
                kVert.z = (*kOffset).x3 * fWeight + kVert.z;
                kIter.Set(static_cast<int32_t>(ivert), kVert);
                if (pDest) {
                    *pDest = kVert;
                    ++pDest;
                }
                ++ivert;
                ++kOffset;
            }
            // The extra data's bonus vertices after the mesh's get the remaining offsets.
            if (pDest) {
                uint32_t uiBaseVerts = pBaseMorph->GetVertexCount(false);
                uint32_t iextra = uiVerts;
                while (iextra < uiBaseVerts) {
                    pDest->x = (*kOffset).x1 * fWeight + pDest->x;
                    pDest->y = (*kOffset).x2 * fWeight + pDest->y;
                    pDest->z = (*kOffset).x3 * fWeight + pDest->z;
                    ++pDest;
                    ++iextra;
                    ++kOffset;
                }
            }
        }
    }
    if (apMesh->GetVertices() && (apMesh->GetModelData()->GetConsistency() != NiGeometryData::STATIC)) {
        apMesh->GetModelData()->MarkAsChanged(NiGeometryData::VERTEX_MASK);
    }

    apMesh->GetModelData()->UnlockPackedVertexData();
    BSFaceGenManager::UnlockFaceGenAccess();
    return true;
}

// The memory context is entered before pTextureData is checked.
bool BSFaceGenModel::LoadEGTData() {
    alignas(AutoMemContext) char acMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (acMemContext) AutoMemContext(
        MEM_CONTEXT_FACEGEN_EGT,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenModel.cpp",
        1336
    );
    if (!pTextureData) {
        pMemContext->~AutoMemContext();
        return false;
    }
    if (!pTextureData->pEGTData) {
        if (!pTextureData->EGTFilename.GetLength()) {
            pMemContext->~AutoMemContext();
            return false;
        }
        pTextureData->pEGTData = EGTData::Create(pTextureData->EGTFilename.String());
    }
    pMemContext->~AutoMemContext();
    return true;
}

// Re-reads pTextureData before clearing the pointer.
void BSFaceGenModel::UnloadEGTData() {
    if (pTextureData && pTextureData->pEGTData) {
        EGTData* pEGTData = pTextureData->pEGTData;
        if (pEGTData) {
            delete pEGTData;
        }
        pTextureData->pEGTData = nullptr;
    }
}

uint32_t BSFaceGenModel::GetEGMDataSize() {
    uint32_t size = 0;
    if (pMeshData && pMeshData->pEGMData) {
        size = 52;
        for (auto& rBasis : pMeshData->pEGMData->EGMBasis) {
            size += 24 * rBasis.size();
            for (uint32_t i = 0; i < rBasis.size(); ++i) {
                size += 6 * rBasis[i].vertPos.size();
            }
        }
    }

    return size;
}

// The size of the loaded .egt data: the EGTData, each basis's modes, and three image vectors' bytes per mode (the size
// of the first one, times three).
uint32_t BSFaceGenModel::GetEGTDataSize() {
    uint32_t size = 0;
    if (pTextureData && pTextureData->pEGTData) {
        size = sizeof(EGTData);
        for (const auto& rBasis : pTextureData->pEGTData->EGTBasis) {
            size += static_cast<uint32_t>(sizeof(FffModeTexS) * rBasis.size());
            for (uint32_t j = 0; j < rBasis.size(); ++j) {
                const FffModeTexS& rMode = rBasis[j];
                size += static_cast<uint32_t>(3 * rMode.img[0].size());
            }
        }
    }

    return size;
}

bool BSFaceGenModel::UnloadEGMData() {
    if (pMeshData && pMeshData->pEGMData) {
        if (pMeshData->iRefCount) {
            return false;
        }
        if (pMeshData->pEGMData) {
            delete pMeshData->pEGMData;
        }
        pMeshData->pEGMData = nullptr;
    }
    return true;
}

bool BSFaceGenModel::HasEGTData() const {
    return pTextureData && pTextureData->pEGTData;
}

NiSkinInstance* BSFaceGenModel::GetSkinInstance() const {
    if (!pMeshData || !pMeshData->spModel) {
        return nullptr;
    }
    NiTriBasedGeom* pModel = pMeshData->spModel;
    return pModel->NiGeometry::GetSkinInstance();
}

void BSFaceGenModel::ForceLoadModelTexture() {
    int8_t acPath[264];
    if (!pTextureData) {
        const char* pcEGMFilename = GetEGMFilename();
        if (pcEGMFilename) {
            BSstrcpy(static_cast<char*>(static_cast<void*>(acPath)), 0x104u, pcEGMFilename);
            char* pcExtension = std::strrchr(static_cast<char*>(static_cast<void*>(acPath)), 46);
            if (pcExtension) {
                *pcExtension = 0;
                BSstrcat(static_cast<char*>(static_cast<void*>(acPath)), 0x104u, ".egt");
                LoadModelTexture(static_cast<char*>(static_cast<void*>(acPath)));
                LoadEGTData();
            }
        }
    }
}

bool BSFaceGenModel::ApplyCoordinateTexturingToMesh(
    const FR2MatrixVTC<float> (*apCoord)[2][2],
    NiPointer<NiTexture>& aspReturnBaseMod,
    uint32_t aiForceBasisSize
) {
    BSFaceGenImage* pImage;
    BSFaceGenRGBA emtpyRGBA;
    uint32_t ibasisSize;
    NiPointer<BSFaceGenImage> spimageOut(static_cast<BSFaceGenImage*>(nullptr));
    BSFaceGenRGBAVector::iterator kPixelIter;
    aspReturnBaseMod = static_cast<NiTexture*>(BSFaceGenManager::GetBaseTexture());
    if (!pTextureData) {
        return false;
    }
    if (!apCoord) {
        apCoord = BSFaceGenManager::GetFaceGenCoord();
    }
    if (!apCoord) {
        return false;
    }
    BSFaceGenManager::LockFaceGenAccess();
    if (!BSFaceGenManager::LoadEGTData(this)) {
        if (!pTextureData->pEGTData) {
            if (!pTextureData->EGTFilename.GetLength()) {
                BSFaceGenManager::UnlockFaceGenAccess();
                return false;
            }
            pTextureData->pEGTData = EGTData::Create(pTextureData->EGTFilename.pString);
        }
        if (!pTextureData->pEGTData) {
            BSFaceGenManager::UnlockFaceGenAccess();
            return false;
        }
    }
    if (!pTextureData->pEGTData) {
        BSFaceGenManager::UnlockFaceGenAccess();
        return false;
    }
    // The image size from the first basis's first mode: its width and height.
    uint32_t ibasisHeight = 0;
    uint32_t ibasisWidth = 0;
    if (pTextureData->pEGTData->EGTBasis[0].size()) {
        ibasisHeight = pTextureData->pEGTData->EGTBasis[0][0].width;
        ibasisWidth = pTextureData->pEGTData->EGTBasis[0][0].height;
    }
    if (!ibasisHeight || !ibasisWidth) {
        BSFaceGenManager::UnlockFaceGenAccess();
        return false;
    }
    for (uint32_t isymm = 0; isymm < 2; ++isymm) {
        uint32_t uiRows = (*apCoord)[1][isymm].nrows;
        if (uiRows < pTextureData->pEGTData->EGTBasis[isymm].size()) {
            Warning("MODELS: FaceGen - Tried to apply a coordinate that did not match the EGT basis.");
            BSFaceGenManager::UnlockFaceGenAccess();
            return false;
        }
    }
    emtpyRGBA.pColor[3] = 0.0f;
    emtpyRGBA.pColor[2] = 0.0f;
    emtpyRGBA.pColor[1] = 0.0f;
    emtpyRGBA.pColor[0] = 0.0f;
    pImage = new BSFaceGenImage(static_cast<NiSourceTexture*>(nullptr));
    spimageOut = pImage;
    spimageOut->iWidth = ibasisHeight;
    spimageOut->iHeight = ibasisWidth;
    BSFaceGenRGBAVector* pvecOut = spimageOut->GetUncompressedImage();
    pvecOut->assign(ibasisWidth * ibasisHeight, emtpyRGBA);
    uint32_t symm = ibasisHeight;
    uint32_t height = ibasisWidth;
    // Allocates int32_t[3 * w * h]; a count whose byte size overflows asks for SIZE_MAX bytes.
    uint32_t uiCount = 3 * (ibasisWidth * ibasisHeight);
    int32_t* iptr = static_cast<int32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(int32_t) ? sizeof(int32_t) * uiCount : SIZE_MAX)
    );
    ourMemset(iptr, 0, 3 * (ibasisWidth * (sizeof(int32_t) * ibasisHeight)));
    uint32_t width = 0;
    if (aiForceBasisSize) {
        ibasisSize = aiForceBasisSize;
        if (ibasisSize > pTextureData->pEGTData->EGTBasis[width].size()) {
            ibasisSize = pTextureData->pEGTData->EGTBasis[width].size();
        }
    }
    else {
        ibasisSize = pTextureData->pEGTData->EGTBasis[width].size();
    }
    for (uint32_t basis = 0; basis < ibasisSize; ++basis) {
        const FffModeTexS& rMode = pTextureData->pEGTData->EGTBasis[width][basis];
        if (rMode.width != symm) {
            fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenModel.cpp", 1162);
        }
        if (rMode.height != height) {
            fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenModel.cpp", 1163);
        }
        int32_t coeff = static_cast<int32_t>((*apCoord)[1][width][basis][0] * 256.0);
        if (coeff) {
            uint32_t cRowWidth = rMode.width + rMode.rowPad;
            for (uint32_t clr = 0; clr < 3; ++clr) {
                int32_t fac = coeff * static_cast<int32_t>(rMode.scale * 256.0);
                // The image bytes are read as signed.
                const int8_t* egt = static_cast<const int8_t*>(static_cast<const void*>(rMode.img[clr].data()));
                for (uint32_t rr = 0; rr < height; ++rr) {
                    for (uint32_t cc = 0; cc < symm; ++cc) {
                        iptr[3 * (rr * symm + cc) + clr] += fac * egt[cc];
                    }
                    egt += cRowWidth;
                }
            }
        }
    }
    for (uint32_t clr = 0; clr < 3; ++clr) {
        float fscale = 1.5258789e-05f;
        kPixelIter = pvecOut->begin();
        float* fptr = VectorIteratorDeref(&kPixelIter).pColor;
        for (uint32_t rr = 0; rr < height; ++rr) {
            for (uint32_t cc = 0; cc < symm; ++cc) {
                fptr[4 * (rr * symm + cc) + clr] = static_cast<double>(iptr[3 * (rr * symm + cc) + clr]) * fscale;
            }
        }
    }
    ourFree(iptr);
    aspReturnBaseMod = static_cast<NiTexture*>(
        spimageOut->GetCompressedImage(-255.0f, 255.0f, 0.5f, NiPixelFormat::RGBA32)
    );
    BSFaceGenManager::UnlockFaceGenAccess();
    return true;
}

bool BSFaceGenModel::LoadEGMData() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_EGM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenModel.cpp",
        1264
    );
    if (!pMeshData) {
        return false;
    }
    if (!pMeshData->pEGMData) {
        EGMData* pEGMData;
        if (!pMeshData->EGMFilename.GetLength()) {
            return false;
        }
        pEGMData = new EGMData(pMeshData->EGMFilename.pString);
        pMeshData->pEGMData = pEGMData;
    }
    return true;
}

void BSFaceGenModel::KillTextureData() {
    if (pTextureData) {
        if (pTextureData->pEGTData) {
            delete pTextureData->pEGTData;
        }
        pTextureData->pEGTData = nullptr;
        if (pTextureData) {
            delete pTextureData;
        }
        pTextureData = nullptr;
    }
}

// The basis key and vectors are read from the .egt file apFilename.
EGTData::EGTData(char* apFilename) {
    std::string kFileName(static_cast<const char*>(apFilename));
    fffReadEgtFile(kFileName, lEGTBasisKey, EGTBasis[0], EGTBasis[1]);
}

EGTData::~EGTData() {
    EGTBasis[0].clear();
    EGTBasis[1].clear();
}

EGTData* EGTData::Create(const char* apEGTFilename) {
    return new EGTData(const_cast<char*>(apEGTFilename));
}

void EGTData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BSFaceGenModelTextureData::BSFaceGenModelTextureData() : pEGTData(nullptr) {}

BSFaceGenModelTextureData::~BSFaceGenModelTextureData() {
    if (pEGTData) {
        delete pEGTData;
    }
}

void BSFaceGenModelTextureData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BSFaceGenModelMeshData::BSFaceGenModelMeshData()
  : pEGMData(nullptr), spModelRoot(nullptr), spModel(nullptr), pAdditionalVertices(nullptr),
    iAdditionalVerticesCount(0), spMorph(nullptr), iRefCount(0) {}

// Releases the three pointers in this order before freeing the vertices and the .egm data.
BSFaceGenModelMeshData::~BSFaceGenModelMeshData() {
    spModel = nullptr;
    spModelRoot = nullptr;
    spMorph = nullptr;
    if (pAdditionalVertices) {
        delete[] pAdditionalVertices;
    }
    if (pEGMData) {
        delete pEGMData;
    }
}

void BSFaceGenModelMeshData::IncRefCount() {
    NiAtomicIncrement(iRefCount);
}

void BSFaceGenModelMeshData::DecRefCount() {
    NiAtomicDecrement(iRefCount);
}

void BSFaceGenModelMeshData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

int32_t return_value(int32_t aiValue, ...) {
    return aiValue;
}

FffModeTexS* CopyOpt(const FffModeTexS* _First, const FffModeTexS* _Last, FffModeTexS* _Dest) {
    while (_First != _Last) {
        _Dest->Assign(*_First);
        ++_Dest;
        ++_First;
    }
    return _Dest;
}

FffModeGS* CopyOpt(const FffModeGS* _First, const FffModeGS* _Last, FffModeGS* _Dest) {
    while (_First != _Last) {
        _Dest->Assign(*_First);
        ++_Dest;
        ++_First;
    }
    return _Dest;
}

// Copies [_First, _Last) of FR2Vect2TC<float> elements over _Dest and returns the end of the copies, computed before
// the loop.
FR2Vect2TC<float>* CopyOpt(FR2Vect2TC<float>* _First, FR2Vect2TC<float>* _Last, FR2Vect2TC<float>* _Dest) {
    FR2Vect2TC<float>* pEnd = _Dest + (_Last - _First);
    for (; _First != _Last; ++_Dest, ++_First) {
        *_Dest = *_First;
    }
    return pEnd;
}

// _Copy_opt of FR2Vect3TC<float> elements: copies [_First, _Last) over _Dest and returns the end of the copies, which
// is computed before the loop.
FR2Vect3TC<float>* CopyOpt(FR2Vect3TC<float>* _First, FR2Vect3TC<float>* _Last, FR2Vect3TC<float>* _Dest) {
    FR2Vect3TC<float>* pEnd = _Dest + (_Last - _First);
    for (; _First != _Last; ++_Dest, ++_First) {
        *_Dest = *_First;
    }
    return pEnd;
}

BSFaceGenModelExtraData::BSFaceGenModelExtraData(BSFaceGenModel* apModel) : NiExtraData(Tag()), spModel(apModel) {}

BSFaceGenModelExtraData::~BSFaceGenModelExtraData() {}

const NiRTTI* BSFaceGenModelExtraData::GetRTTI() const {
    return &BSFaceGenModelExtraData::ms_RTTI;
}

BSFaceGenModel* BSFaceGenModelExtraData::GetModel() {
    return spModel;
}

void BSFaceGenModelExtraData::InitTag() {
    BSFaceGenModelExtraData::pTag = new NiFixedString("FMD");
}

const NiFixedString& BSFaceGenModelExtraData::Tag() {
    return *pTag;
}

void BSFaceGenModelExtraData::KillSDM() {
    if (BSFaceGenModelExtraData::pTag) {
        delete BSFaceGenModelExtraData::pTag;
    }
    BSFaceGenModelExtraData::pTag = nullptr;
}

EGMData::~EGMData() {
    EGMBasis[0].clear();
    EGMBasis[1].clear();
}

void EGMData::operator delete(void* apMemory) {
    ourFree(apMemory);
}
