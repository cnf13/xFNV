#include "Fallout Shared/FaceGen/BSFaceGenModel.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
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
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout Shared/FaceGen/FffTriFileDataS.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Interface/FOVATSEffectManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/EgtFileIO_Sdk.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/MSVC/ehvec.h"
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

using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat =
    std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat =
    std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;

// The EH cleanup of an exception pointer: it destroys the object with the non-deleting destructor.
void ExceptionPtrDestroy_3(void* apObject) {
    static_cast<FffTriDiffMorphS*>(apObject)->~FffTriDiffMorphS();
}

// Like the third, for the other object type.
void ExceptionPtrDestroy_4(void* apObject) {
    static_cast<FffTriStatMorphS*>(apObject)->~FffTriStatMorphS();
}

using std_nvector_lFR2Vect2TC_lfloat_g_cstd_nallocator_lFR2Vect2TC_lfloat =
    std::vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat =
    std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect2TC_lfloat_g_cstd_nallocator_lFR2Vect2TC_lfloat =
    std::vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat =
    std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect2TC_lfloat_g_cstd_nallocator_lFR2Vect2TC_lfloat =
    std::vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>>;
using vector_FR2Vect2TC_float = std::vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>>;

using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat =
    std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat =
    std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;

using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat =
    std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect2TC_lfloat_g_cstd_nallocator_lFR2Vect2TC_lfloat =
    std::vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat =
    std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;
using std_nvector_lFR2Vect3TC_lfloat_g_cstd_nallocator_lFR2Vect3TC_lfloat =
    std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>;

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
    if (ppExpressionMorphs && (((uint32_t)aeIndex) <= 14)) {
        return ppExpressionMorphs[(int32_t)aeIndex];
    }

    return nullptr;
}

BSFaceGenMorph* BSFaceGenMorphDataHead::GetModifierMorph(BS_FACEGEN_MODIFIER aeIndex) {
    if (ppModifierMorphs && (((uint32_t)aeIndex) <= 16)) {
        return ppModifierMorphs[(int32_t)aeIndex];
    }

    return nullptr;
}

BSFaceGenMorph* BSFaceGenMorphDataHead::GetPhonemeMorph(BS_FACEGEN_PHONEME aeIndex) {
    if (ppPhonemeMorphs && (((uint32_t)aeIndex) < 16)) {
        return ppPhonemeMorphs[(int32_t)aeIndex];
    }

    return nullptr;
}

BSFaceGenMorph* BSFaceGenMorphDataHead::GetCustomMorph(BS_FACEGEN_CUSTOM_MORPH aeIndex) {
    if ((!ppCustomMorphs) || aeIndex) {
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
        MEM_CONTEXT_FACEGEN_EGM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenModel.cpp", 286
    );
    void* pMemory = ourMalloc(sizeof(BSFaceGenModelMeshData));
    BSFaceGenModelMeshData* pData = pMemory ? ::new (pMemory) BSFaceGenModelMeshData : nullptr;
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
        if (pMeshData->spModelRoot.operator->()) {
            uint32_t uiCount = pMeshData->spModelRoot.operator->()->GetArrayCount();
            for (uint32_t i = 0; i < uiCount; ++i) {
                NiAVObject* pChild = pMeshData->spModelRoot.operator->()->GetAt(i);
                if (pChild) {
                    bool bByName = aeHairPart == HAIR_PART_HAT || (aeHairPart == HAIR_PART_NORMAL && uiCount != 1);
                    // SWEEP-INLINE 0x43B1B0
                    if (!bByName || !BSstricmp(pChild->GetName(), pHairPartName[aeHairPart])) {
                        pMeshData->spModel = pChild->IsTriBasedGeom();
                    }
                }
                if (pMeshData->spModel.operator->()) {
                    break;
                }
            }
            if (pMeshData->spModel.operator->()) {
                if (!abIsAccessory) {
                    int32_t iType = NiTexturingProperty::GetType();
                    pMeshData->spModel.operator->()->RemoveProperty(iType);
                }
                pMeshData->spModel.operator->()->RemoveAllControllers();
                pMeshData->spModel.operator->()->RemoveAllExtraData();
                if (pMeshData->spModel.operator->()->GetSkinInstance()) {
                    NiAVObject* pRootParent = pMeshData->spModel.operator->();
                    pMeshData->spModel.operator->()->GetSkinInstance()->SetRootParent(pRootParent);
                }
                pMeshData->spModel.operator->()->SetSelectiveUpdate(true);
            }
            pNode->UpdateProperties();
            alignas(NiUpdateData) char acUpdateData[sizeof(NiUpdateData)];
            NiUpdateData* pUpdateData = ::new (acUpdateData) NiUpdateData(0.0f, false, false);
            pNode->Update(*pUpdateData);
        }
        pCloneProc->~BSCloningProcess();
    }
    if (apTRIFilename) {
        void* pTriMemory = ourMalloc(sizeof(FffTriFileDataS));
        FffTriFileDataS* pTriData = pTriMemory ? ::new (pTriMemory) FffTriFileDataS : nullptr;
        alignas(std::string) char acTriName[sizeof(std::string)];
        std::string* pTriName = ::new (acTriName) std::string(apTRIFilename);
        bool bLoaded = false;
        if (fffReadTriFile(*pTriName, *pTriData)) {
            uint32_t uiModelVertices = pMeshData->spModel.operator->()->GetVertexCount();
            if (uiModelVertices == pTriData->numModelVtx()) {
                bLoaded = true;
            }
        }
        XFNV_DESTRUCT(std::string)(pTriName);
        if (bLoaded) {
            if (aeHairPart != HAIR_PART_NONE) {
                void* pHairMemory = NiMemObject::operator new(sizeof(BSFaceGenMorphDataHair));
                BSFaceGenMorphDataHair* pHair =
                    pHairMemory ? ::new (pHairMemory) BSFaceGenMorphDataHair(pTriData) : nullptr;
                pMeshData->spMorph = pHair;
            }
            else {
                void* pHeadMemory = NiMemObject::operator new(sizeof(BSFaceGenMorphDataHead));
                BSFaceGenMorphDataHead* pHead =
                    pHeadMemory ? ::new (pHeadMemory) BSFaceGenMorphDataHead(pTriData) : nullptr;
                pMeshData->spMorph = pHead;
                uint32_t uiTriVertices = pTriData->vtxListSize();
                if (uiTriVertices > pMeshData->spModel.operator->()->GetVertexCount()) {
                    uiTriVertices = pTriData->vtxListSize();
                    pMeshData->iAdditionalVerticesCount =
                        uiTriVertices - pMeshData->spModel.operator->()->GetVertexCount();
                    uint32_t uiCount = pMeshData->iAdditionalVerticesCount;
                    void* pVertexMemory =
                        ourMalloc(uiCount > SIZE_MAX / sizeof(NiPoint3) ? SIZE_MAX : sizeof(NiPoint3) * uiCount);
                    NiPoint3* pVertices = nullptr;
                    if (pVertexMemory) {
                        rt::eh_vector_constructor_iterator(
                            pVertexMemory, sizeof(NiPoint3), static_cast<int32_t>(uiCount), &emptyPointConstructor
                        );
                        pVertices = static_cast<NiPoint3*>(pVertexMemory);
                    }
                    pMeshData->pAdditionalVertices = pVertices;
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
    if (pMeshData && pMeshData->spModel.operator->()) {
        pMeshData->spModel.operator->()->SetConsistency(NiGeometryData::MUTABLE);
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
        MEM_CONTEXT_FACEGEN_EGT, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenModel.cpp", 448
    );
    void* pMemory = ourMalloc(sizeof(BSFaceGenModelTextureData));
    BSFaceGenModelTextureData* pData = pMemory ? ::new (pMemory) BSFaceGenModelTextureData : nullptr;
    pTextureData = pData;
    pTextureData->EGTFilename.Set(apEGTFilename, 0);
    pMemContext->~AutoMemContext();
    return true;
}

bool BSFaceGenModel::CreateNewMesh(NiPointer<NiTriBasedGeom>& aspReturnMesh) {
    NiPointer<NiGeometryData> spdata(static_cast<NiGeometryData*>(nullptr));
    NiPointer<NiSkinPartition> spskinPartition(static_cast<NiSkinPartition*>(nullptr));
    aspReturnMesh.operator=(static_cast<NiTriBasedGeom*>(nullptr));
    if (pMeshData && (pMeshData->spModel.m_pObject && pMeshData->spModel.m_pObject->GetModelData())) {
        BSFaceGenModelExtraData* pmorphBase_2;
        BSFaceGenBaseMorphExtraData* pmorphBase;
        aspReturnMesh.operator=(static_cast<NiTriBasedGeom*>(pMeshData->spModel.m_pObject->Clone()));
        NiPointer<NiObject> spTempObject(static_cast<NiObject*>(nullptr));
        pMeshData->spModel.m_pObject->GetModelData()->CreateDeepCopy(spTempObject);
        spdata.operator=(static_cast<NiGeometryData*>(spTempObject.m_pObject));
        NiTriBasedGeom* v6 = aspReturnMesh.m_pObject;
        v6->SetModelData(spdata.m_pObject);
        NiSkinInstance* pskin = aspReturnMesh.m_pObject->GetSkinInstance();
        if ((pskin) && (pskin->GetSkinPartition())) {
            NiPointer<NiObject> spTempSkinObject(static_cast<NiObject*>(nullptr));
            pskin->GetSkinPartition()->CreateDeepCopy(spTempSkinObject);
            spskinPartition.operator=(static_cast<NiSkinPartition*>(spTempSkinObject.m_pObject));
            NiSkinPartition* v3 = spskinPartition.m_pObject;
            pskin->SetSkinPartition(v3);
        }

        BSFaceGenBaseMorphExtraData* apMemory =
            static_cast<BSFaceGenBaseMorphExtraData*>(NiMemObject::operator new(sizeof(BSFaceGenBaseMorphExtraData)));
        if (apMemory) {
            pmorphBase = ::new (apMemory) BSFaceGenBaseMorphExtraData(
                aspReturnMesh.m_pObject, pMeshData->pAdditionalVertices, pMeshData->iAdditionalVerticesCount
            );
        }
        else {
            pmorphBase = nullptr;
        }
        NiExtraData* pkExtra = pmorphBase;
        if (pmorphBase) {
            aspReturnMesh.m_pObject->AddExtraData(pkExtra);
        }
        BSFaceGenModelExtraData* v8 =
            static_cast<BSFaceGenModelExtraData*>(NiMemObject::operator new(sizeof(BSFaceGenModelExtraData)));
        if (v8) {
            pmorphBase_2 = ::new (v8) BSFaceGenModelExtraData(this);
        }
        else {
            pmorphBase_2 = nullptr;
        }
        aspReturnMesh.m_pObject->AddExtraData(pmorphBase_2);
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
    if (ApplyCoordinateToExistingMesh(apCoord, aspReturnMesh.operator->(), true)) {
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
            void* pMemory = ourMalloc(sizeof(EGMData));
            EGMData* pEGMData;
            if (pMemory) {
                pEGMData = ::new (pMemory) EGMData(pMeshData->EGMFilename.pString);
            }
            else {
                pEGMData = nullptr;
            }
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
    for (uint32_t isymm = 0; isymm < FANSYMM_SIZE; ++isymm) {
        uint32_t uiModes = pMeshData->pEGMData->EGMBasis[isymm].size();
        for (uint32_t imode = 0; imode < uiModes; ++imode) {
            if (pMeshData->pEGMData->EGMBasis[isymm][imode].vertPos.size() < uiVerts) {
                uiVerts = pMeshData->pEGMData->EGMBasis[isymm][imode].vertPos.size();
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
    if ((apMesh->GetVertices()) && (apMesh->GetModelData()->GetConsistency() != NiGeometryData::STATIC)) {
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
        MEM_CONTEXT_FACEGEN_EGT, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenModel.cpp", 1336
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
    if ((pMeshData) && (pMeshData->pEGMData)) {
        size = 52;
        for (uint32_t i = 0; i < 2; ++i) {
            size += 24 * pMeshData->pEGMData->EGMBasis[i].size();
            for (uint32_t a2 = 0; a2 < pMeshData->pEGMData->EGMBasis[i].size(); ++a2) {
                FffModeGS* v1 = &pMeshData->pEGMData->EGMBasis[i].operator[](a2);
                size += 6 * v1->vertPos.size();
            }
        }
    }

    return size;
}

// The size of the loaded .egt data: the EGTData, each basis's modes, and three image vectors' bytes per mode (the size
// of the first one, times three).
uint32_t BSFaceGenModel::GetEGTDataSize() {
    uint32_t size = 0;
    if ((pTextureData) && (pTextureData->pEGTData)) {
        size = sizeof(EGTData);
        for (uint32_t i = 0; i < 2; ++i) {
            size += static_cast<uint32_t>(sizeof(FffModeTexS) * pTextureData->pEGTData->EGTBasis[i].size());
            for (uint32_t j = 0; j < pTextureData->pEGTData->EGTBasis[i].size(); ++j) {
                const FffModeTexS& rMode = pTextureData->pEGTData->EGTBasis[i][j];
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
    return (bool)((uint8_t)(pTextureData && pTextureData->pEGTData));
}

NiSkinInstance* BSFaceGenModel::GetSkinInstance() const {
    NiTriBasedGeom* v1;
    bool vCondition1;
    vCondition1 = (!pMeshData) || (!(&pMeshData->spModel)->operator->());
    if (vCondition1) {
        return nullptr;
    }
    v1 = (&pMeshData->spModel)->operator->();
    return v1->NiGeometry::GetSkinInstance();
}

void BSFaceGenModel::ForceLoadModelTexture() {
    char* v2;
    int8_t dst[264];
    const char* src;
    if (!pTextureData) {
        src = GetEGMFilename();
        if (src) {
            BSstrcpy(static_cast<char*>(static_cast<void*>(dst)), 0x104u, src);
            v2 = std::strrchr(static_cast<char*>(static_cast<void*>(dst)), 46);
            if (v2) {
                *v2 = 0;
                BSstrcat(static_cast<char*>(static_cast<void*>(dst)), 0x104u, ".egt");
                LoadModelTexture(static_cast<char*>(static_cast<void*>(dst)));
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
    aspReturnBaseMod.operator=(static_cast<NiTexture*>(BSFaceGenManager::GetBaseTexture()));
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
    void* pMemory = NiMemObject::operator new(sizeof(BSFaceGenImage));
    if (pMemory) {
        pImage = ::new (pMemory) BSFaceGenImage(static_cast<NiSourceTexture*>(nullptr));
    }
    else {
        pImage = nullptr;
    }
    spimageOut.operator=(pImage);
    spimageOut.m_pObject->iWidth = ibasisHeight;
    spimageOut.m_pObject->iHeight = ibasisWidth;
    BSFaceGenRGBAVector* pvecOut = spimageOut.m_pObject->GetUncompressedImage();
    pvecOut->assign(ibasisWidth * ibasisHeight, emtpyRGBA);
    uint32_t symm = ibasisHeight;
    uint32_t height = ibasisWidth;
    // Allocates int32_t[3 * w * h]; a count whose byte size overflows asks for SIZE_MAX bytes.
    uint32_t uiCount = 3 * (ibasisWidth * ibasisHeight);
    int32_t* iptr =
        static_cast<int32_t*>(ourMalloc(uiCount <= SIZE_MAX / sizeof(int32_t) ? sizeof(int32_t) * uiCount : SIZE_MAX));
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
    for (uint32_t clr_2 = 0; clr_2 < 3; ++clr_2) {
        float fac_2 = 1.5258789e-05f;
        kPixelIter = pvecOut->begin();
        float* fptr = VectorIteratorDeref(&kPixelIter).pColor;
        for (uint32_t rr_2 = 0; rr_2 < height; ++rr_2) {
            for (uint32_t cc_2 = 0; cc_2 < symm; ++cc_2) {
                fptr[4 * (rr_2 * symm + cc_2) + clr_2] =
                    static_cast<double>(iptr[3 * (rr_2 * symm + cc_2) + clr_2]) * fac_2;
            }
        }
    }
    ourFree(iptr);
    aspReturnBaseMod.operator=(
        static_cast<NiTexture*>(spimageOut.m_pObject->GetCompressedImage(-255.0f, 255.0f, 0.5f, NiPixelFormat_RGBA32))
    );
    BSFaceGenManager::UnlockFaceGenAccess();
    return true;
}

bool BSFaceGenModel::LoadEGMData() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_EGM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenModel.cpp", 1264
    );
    if (!pMeshData) {
        goto LABEL_2;
    }
    if (!pMeshData->pEGMData) {
        EGMData* v2;
        if (!pMeshData->EGMFilename.GetLength()) {
        LABEL_2:;
            return false;
        }
        EGMData* v4 = static_cast<EGMData*>(ourMalloc(sizeof(EGMData)));
        if (v4) {
            v2 = ::new (v4) EGMData(pMeshData->EGMFilename.pString);
        }
        else {
            v2 = nullptr;
        }
        pMeshData->pEGMData = v2;
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

// The basis key and vectors are read from the .egt file a1.
EGTData::EGTData(char* a1) {
    std::string kFileName(static_cast<const char*>(a1));
    fffReadEgtFile(kFileName, lEGTBasisKey, EGTBasis[0], EGTBasis[1]);
}

EGTData::~EGTData() {
    EGTBasis[0].clear();
    EGTBasis[1].clear();
}

EGTData* EGTData::Create(const char* apEGTFilename) {
    void* v3 = ourMalloc(sizeof(EGTData));
    if (v3) {
        return ::new (v3) EGTData(const_cast<char*>(apEGTFilename));
    }

    return nullptr;
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
        ourFree(pAdditionalVertices);
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

FffModeGS::FffModeGS(const FffModeGS& _Right) : scale(_Right.scale), vertPos(_Right.vertPos) {}

FffModeGS::~FffModeGS() {}

FffModeGS& FffModeGS::Assign(const FffModeGS& _Right) {
    scale = _Right.scale;
    vertPos = _Right.vertPos;
    return *this;
}

FffModeTexS::FffModeTexS() {}

FffModeTexS::FffModeTexS(const FffModeTexS& _Right)
  : scale(_Right.scale), width(_Right.width), height(_Right.height), rowPad(_Right.rowPad),
    img{_Right.img[0], _Right.img[1], _Right.img[2]} {}

FffModeTexS::~FffModeTexS() {}

FffModeTexS& FffModeTexS::Assign(const FffModeTexS& _Right) {
    scale = _Right.scale;
    width = _Right.width;
    height = _Right.height;
    rowPad = _Right.rowPad;
    for (uint32_t i = 0; i < 3; ++i) {
        img[i] = _Right.img[i];
    }
    return *this;
}

void FffModeTexS::operator delete(void* apMemory) {
    ourFree(apMemory);
}

int32_t return_value(int32_t a1, ...) {
    return a1;
}

uint32_t BSMIN(uint16_t a, uint32_t b) {
    if (((uint32_t)((int32_t)a)) < b) {
        return (uint32_t)a;
    }

    return b;
}

void Destroy(FffModeTexS* apThis) {
    apThis->~FffModeTexS();
}

FffTriVtxLabelS* DestroyTriVtxLabel(FffTriVtxLabelS* a1) {
    return ExceptionPtrDestroy_1(a1);
}

void* DestroyTriSurfLabel(void* a1) {
    return ::Destroy(static_cast<FffTriSurfLabelS*>(a1));
}

FffTriSurfLabelS* Destroy(FffTriSurfLabelS* _Ptr) {
    return XFNV_DELETE(FffTriSurfLabelS)(_Ptr, 0);
}

FffModeTexS* CopyOpt(const FffModeTexS* a1, const FffModeTexS* a2, FffModeTexS* a3) {
    while (a1 != a2) {
        a3->Assign(*a1);
        ++a3;
        ++a1;
    }
    return a3;
}

FffTriVtxLabelS* ExceptionPtrDestroy_1(FffTriVtxLabelS* a1) {
    a1->~FffTriVtxLabelS();
    return a1;
}

FffModeGS* CopyOpt(const FffModeGS* a1, const FffModeGS* a2, FffModeGS* a3) {
    while (a1 != a2) {
        a3->Assign(*a1);
        ++a3;
        ++a1;
    }
    return a3;
}

void DestroyTriDiffMorph(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1) {
    ExceptionPtrDestroy_3(a1);
}

void DestroyTriStatMorph(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1) {
    ExceptionPtrDestroy_4(a1);
}

FffModeGS* Destroy(FffModeGS* a1) {
    a1->~FffModeGS();
    return a1;
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

BSSystemFile::ErrorCode BSSystemFile::QLastError() const {
    return eLastError;
}

BSSystemFile::ErrorCode BSSystemFile::GetSizeOfFile(const char* apcPath, uint64_t& auiSize) {
    BSSystemFile apMemory;
    apMemory.~BSSystemFile();
    ::new (&apMemory) BSSystemFile(apcPath, 0, 0, false);
    BSSystemFile::ErrorCode Size = apMemory.QLastError();
    if (Size) {
        auiSize = 0;
    }
    else {
        Size = apMemory.GetSize(auiSize);
    }
    return Size;
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
    NiFixedString* v0;
    NiFixedString* v1 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v1) {
        v0 = ::new (static_cast<void*>(v1)) NiFixedString("FMD");
    }
    else {
        v0 = nullptr;
    }
    BSFaceGenModelExtraData::pTag = v0;
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

FffTriFileDataS::~FffTriFileDataS() {}

uint32_t FffTriFileDataS::numModelVtx() const {
    uint32_t numVtx = vtxListSize();
    for (uint32_t xx = 0; xx < statMorphList.size(); ++xx) {
        if (statMorphList[xx].firstVtxOffset < numVtx) {
            numVtx = statMorphList[xx].firstVtxOffset;
        }
    }
    return numVtx;
}

// The element sits m_iStride bytes apart in the untyped vertex stream. The release build's bounds check is an empty
// function.
void NiGeometryDataIteratorNiPoint3::Set(int32_t iIndex, const NiPoint3& kValue) {
    void* pvElement = static_cast<uint8_t*>(m_pvCurrentPosition) + m_iStride * static_cast<uint32_t>(iIndex);
    *static_cast<NiPoint3*>(pvElement) = kValue;
}

void NiGeometry::SetConsistency(NiGeometryData::Consistency eConsistency) {
    NiGeometryData* pModelData = this->m_spModelData.operator->();
    pModelData->SetConsistency(eConsistency);
}

uint32_t NiSkinPartition::GetPartitionCount() const {
    return m_uiPartitions;
}

NiSkinPartition::Partition* NiSkinPartition::GetPartitions() const {
    return m_pkPartitions;
}
