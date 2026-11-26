#include "Fallout Shared/FaceGen/BSFaceGenManager.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/Lighting30ShaderProperty.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSArchive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/BSHash.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSHeadPart.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenBaseMorphExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/BSFaceGenModelExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenMorphDataHair.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/FaceGen/BSFaceGenUtils.h"
#include "Fallout Shared/FaceGenNpcData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESEyes.h"
#include "Fallout Shared/TESHair.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Interface/FOVATSEffectManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryDataIterators.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSemaphore.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/FaceGen/FanControls.h"
#include "SDK/FaceGen/FanCtlFile.h"
#include "SDK/FaceGen/FutDensity.h"
#include "SDK/FaceGen/MatrixCT.h"
#include "SDK/FaceGen/MatrixVT.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/string.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

BSCriticalSection BSFaceGenManager::FGAccessCrit;
SettingT<INISettingCollection> BSFaceGenManager::bUseFaceGenHeads("bUseFaceGenHeads:General", true);
SettingT<INISettingCollection> BSFaceGenManager::bFixNormals("bFixFaceNormals:General", false);
SettingT<INISettingCollection> BSFaceGenManager::bUseFaceGenLOD("bUseFaceGenLOD:LOD", false);
SettingT<INISettingCollection> BSFaceGenManager::fLodDistance("fLodDistance:LOD", 500.0f);
SettingT<INISettingCollection> BSFaceGenManager::fTalkingDistance("fTalkingDistance:LOD", 2000.0f);
SettingT<INISettingCollection>
    BSFaceGenManager::bUseMultiThreadedFaceGen("bUseMultiThreadedFaceGen:BackgroundLoad", true);
SettingT<INISettingCollection> BSFaceGenManager::bUseEyeEnvMapping("bUseEyeEnvMapping:General", true);
SettingT<INISettingCollection> BSFaceGenManager::uiMaxEGMDataSize("uiFaceGenMaxEGMDataSize:General", 67108864u);
SettingT<INISettingCollection> BSFaceGenManager::uiMaxEGTDataSize("uiFaceGenMaxEGTDataSize:General", 67108864u);
SettingT<INISettingCollection> BSFaceGenManager::uiEGTClampSize("uiEGTClampSize:General", 30u);
static SettingT<INISettingCollection> bDebugFaceGenMultithreading("bDebugFaceGenMultithreading:Debug", false);
static SettingT<INISettingCollection> bDebugFaceGenCriticalSection("bDebugFaceGenCriticalSection:Debug", false);
SettingT<INISettingCollection> BSFaceGenManager::bLoadBackgroundFaceGen("bLoadBackgroundFaceGen:BackgroundLoad", false);
SettingT<INISettingCollection> bLoadFaceGenHeadEGTFiles("bLoadFaceGenHeadEGTFiles:General", false);
NiSemaphore BSFaceGenManager::LoadingSema(1, 1);
BSCriticalSection BSFaceGenManager::QueueCritical;

BSFaceGenModelMap::Entry::Entry() {}

BSFaceGenModelMap::Entry::~Entry() {}

BSFaceGenModelMap::BSFaceGenModelMap() : Map(37u, true), bHasFaceGenData(false) {
    for (uint32_t& uiDataSize : uiMaxDataSize) {
        uiDataSize = 0x10000000;
    }
}

BSFaceGenModelMap::~BSFaceGenModelMap() {
    Map.RemoveAll();
}

// The data types are walked with an integer counter, cast to the enum at each call.
void BSFaceGenModelMap::VerifyData(BSFaceGenModelMap::Entry* aspNewEntry) {
    uint32_t size = 0;
    BSFaceGenManager::LockFaceGenAccess();
    for (uint32_t cnt = 0; cnt < 2; ++cnt) {
        uint32_t totalSizePrev;
        if (aspNewEntry) {
            if (cnt) {
                if (aspNewEntry->spModel->GetEGTDataSize()) {
                    continue;
                }
                size = 10;
            }
            else {
                if (aspNewEntry->spModel->GetEGMDataSize()) {
                    continue;
                }
                size = 10;
            }
        }
        uint32_t totalSize = GetTotalDataSize(static_cast<BSFaceGenModelMap::DataType>(cnt));
        do {
            if (size + totalSize <= GetMaxDataSize(static_cast<BSFaceGenModelMap::DataType>(cnt))) {
                break;
            }
            totalSizePrev = totalSize;
            FreeLRUData(static_cast<BSFaceGenModelMap::DataType>(cnt), aspNewEntry);
            totalSize = GetTotalDataSize(static_cast<BSFaceGenModelMap::DataType>(cnt));
        }
        while (totalSize != totalSizePrev);
    }
    BSFaceGenManager::UnlockFaceGenAccess();
}

// Looks apcKey up; on a hit, holds the entry's model in aspModel and the model's mesh data in aspModelMeshData.
bool BSFaceGenModelMap::GetAt(
    const char* apcKey,
    NiPointer<BSFaceGenModel>& aspModel,
    NiPointer<BSFaceGenModelMeshData>& aspModelMeshData
) {
    BSFaceGenManager::LockFaceGenAccess();
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    bool bRes = false;
    if (Map.GetAt(apcKey, spEntry)) {
        aspModel = spEntry->spModel;
        aspModelMeshData = aspModel->pMeshData;
        bRes = true;
    }
    BSFaceGenManager::UnlockFaceGenAccess();
    return bRes;
}

bool BSFaceGenModelMap::KillModelTextureData(const char* apKey) {
    if (!apKey || !*apKey) {
        return false;
    }
    bool bRes = false;
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    BSFaceGenManager::LockFaceGenAccess();
    if (!Map.GetAt(apKey, spEntry)) {
        char cFileName[268];
        BSstrcpy(cFileName, 260, apKey);
        char* pExt = std::strrchr(cFileName, '.');
        if (pExt) {
            *pExt = 0;
            BSstrcat(cFileName, 260, ".egm");
            Map.GetAt(cFileName, spEntry);
        }
    }
    if (spEntry.m_pObject) {
        spEntry->spModel->KillTextureData();
        bRes = true;
    }
    BSFaceGenManager::UnlockFaceGenAccess();
    return bRes;
}

// Drops every cached model nothing but the cache holds.
void BSFaceGenModelMap::RemoveAllUnusedModels() {
    const char* pKey;
    BSFaceGenManager::LockFaceGenAccess();
    NiTMapItem<const char*, NiPointer<BSFaceGenModelMap::Entry>>* pItr = Map.GetFirstPos();
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    while (pItr) {
        Map.GetNext(pItr, pKey, spEntry);
        if (spEntry->spModel->m_uiRefCount == 1) {
            Map.RemoveAt(pKey);
        }
        spEntry = static_cast<BSFaceGenModelMap::Entry*>(nullptr);
    }
    BSFaceGenManager::UnlockFaceGenAccess();
}

void BSFaceGenModelMap::ClearEGTData() {
    BSFaceGenManager::LockFaceGenAccess();
    NiTMapItem<const char*, NiPointer<BSFaceGenModelMap::Entry>>* pItr = Map.GetFirstPos();
    while (pItr) {
        const char* pKey = nullptr;
        NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
        Map.GetNext(pItr, pKey, spEntry);
        if (spEntry.m_pObject) {
            BSFaceGenModel* pModel = spEntry->spModel.m_pObject;
            if (pModel) {
                pModel->UnloadEGTData();
            }
        }
    }
    BSFaceGenManager::UnlockFaceGenAccess();
}

// Loads the geometry morph data of apModel's cached entry, found under its EGM file name or else its EGT file name.
bool BSFaceGenModelMap::LoadEGMData(BSFaceGenModel* apModel) {
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    if (!apModel || !apModel->GetEGMFilename() || !BSstrlen(apModel->GetEGMFilename())) {
        return false;
    }
    bool bRes = false;
    BSFaceGenManager::LockFaceGenAccess();
    if (Map.GetAt(apModel->GetEGMFilename(), spEntry) ||
        (apModel->GetEGTFilename() && Map.GetAt(apModel->GetEGTFilename(), spEntry))) {
        spEntry->uiTime = BSGetTickCount();
        VerifyData(spEntry.m_pObject);
        spEntry->spModel->LoadEGMData();
        bHasFaceGenData = true;
        bRes = true;
    }
    BSFaceGenManager::UnlockFaceGenAccess();
    return bRes;
}

bool BSFaceGenModelMap::LoadEGTData(BSFaceGenModel* apModel) {
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    if (apModel && apModel->GetEGTFilename() && BSstrlen(apModel->GetEGTFilename())) {
        bool bRes = false;
        BSFaceGenManager::LockFaceGenAccess();
        if (Map.GetAt(apModel->GetEGTFilename(), spEntry) ||
            (apModel->GetEGMFilename() && Map.GetAt(apModel->GetEGMFilename(), spEntry))) {
            spEntry->uiTime = BSGetTickCount();
            VerifyData(spEntry.m_pObject);
            spEntry->spModel->LoadEGTData();
            bHasFaceGenData = true;
            bRes = true;
        }
        BSFaceGenManager::UnlockFaceGenAccess();
        return bRes;
    }

    return false;
}

uint32_t BSFaceGenModelMap::GetMaxDataSize(BSFaceGenModelMap::DataType aDataType) {
    return uiMaxDataSize[aDataType];
}

void BSFaceGenModelMap::SetMaxDataSize(BSFaceGenModelMap::DataType aDataType, uint32_t auiSize) {
    uiMaxDataSize[aDataType] = auiSize;
    VerifyData(nullptr);
}

// The total size of one kind of cached data, summed over every cached model. Walks the map holding each value in a
// NiPointer, and takes the FaceGen lock for the walk.
uint32_t BSFaceGenModelMap::GetTotalDataSize(BSFaceGenModelMap::DataType aDataType) {
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    const char* pKey = nullptr;
    uint32_t uiTotal = 0;
    BSFaceGenManager::LockFaceGenAccess();
    auto* pItem = Map.GetFirstPos();
    while (pItem) {
        Map.GetNext(pItem, pKey, spEntry);
        switch (aDataType) {
            case 0: uiTotal += spEntry->spModel->GetEGMDataSize(); break;
            case 1: uiTotal += spEntry->spModel->GetEGTDataSize(); break;
            default: break;
        }
    }
    BSFaceGenManager::UnlockFaceGenAccess();
    return uiTotal;
}

// Unloads every cached model's texture data. Takes the FaceGen lock (or tries to and skips the walk when it is held
// elsewhere), drops the entries whose model only the cache holds, and clears the data flag unless a model still holds
// data.
void BSFaceGenModelMap::UnloadAllEGMAndEGTData(bool abForce) {
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    char cLocked = abForce;
    if (abForce) {
        BSFaceGenManager::LockFaceGenAccess();
    }
    else {
        cLocked = BSFaceGenManager::TryLockFaceGenAccess();
    }
    if (cLocked) {
        bool bKeepData = false;
        const char* pKey = nullptr;
        auto* pItem = Map.GetFirstPos();
        while (pItem) {
            Map.GetNext(pItem, pKey, spEntry);
            if (spEntry->spModel->GetRefCount() == 1) {
                Map.RemoveAt(pKey);
            }
            else if (spEntry && spEntry->spModel) {
                if (!spEntry->spModel->UnloadEGMData()) {
                    bKeepData = true;
                }
                spEntry->spModel->UnloadEGTData();
            }
        }
        if (!bKeepData) {
            bHasFaceGenData = false;
        }
        BSFaceGenManager::UnlockFaceGenAccess();
    }
}

// Lists every cached model (its reference count less the cache's, file name and data sizes in KB) as warnings and, when
// apFile is given, into that file.
void BSFaceGenModelMap::OutputModelMapContents(BSFile* apFile) {
    NiTMapItem<const char*, NiPointer<BSFaceGenModelMap::Entry>>* pItr = Map.GetFirstPos();
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    while (pItr) {
        const char* pFilename;
        Map.GetNext(pItr, pFilename, spEntry);
        if (spEntry.m_pObject) {
            BSStringT<char> textline;
            uint32_t uiEGTSize = spEntry->spModel->GetEGTDataSize() >> 10;
            uint32_t uiEGMSize = spEntry->spModel->GetEGMDataSize() >> 10;
            textline.SPrintF(
                "FACEGEN:\t%3d\t%s\t%iKB\t%iKB\r\n",
                spEntry->spModel->m_uiRefCount - 1,
                pFilename,
                uiEGMSize,
                uiEGTSize
            );
            Warning(textline.pString);
            if (apFile) {
                apFile->Write(textline, true);
            }
        }
    }
}

// Caches apModel under apcKey in a new entry stamped with the current time, or drops apcKey's entry when apModel is
// null.
void BSFaceGenModelMap::SetAt(const char* apcKey, BSFaceGenModel* apModel) {
    BSFaceGenManager::LockFaceGenAccess();
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    if (apModel) {
        BSFaceGenModelMap::Entry* pEntry = new BSFaceGenModelMap::Entry;
        spEntry = pEntry;
        spEntry->spModel = apModel;
        spEntry->uiTime = BSGetTickCount();
        // The map takes its value by value.
        Map.SetAt(apcKey, spEntry);
        VerifyData(spEntry.m_pObject);
        BSFaceGenManager::UnlockFaceGenAccess();
    }
    else {
        Map.RemoveAt(apcKey);
        BSFaceGenManager::UnlockFaceGenAccess();
    }
}

// Looks apcKey up; on a hit, stamps the entry with the current time, holds its model in aspModel and loads the model's
// morph data.
bool BSFaceGenModelMap::GetAt(const char* apcKey, NiPointer<BSFaceGenModel>& aspModel) {
    BSFaceGenManager::LockFaceGenAccess();
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    bool bRes = false;
    if (Map.GetAt(apcKey, spEntry)) {
        spEntry->uiTime = BSGetTickCount();
        aspModel = spEntry->spModel;
        VerifyData(spEntry.m_pObject);
        aspModel->LoadEGMData();
        aspModel->LoadEGTData();
        bHasFaceGenData = true;
        bRes = true;
    }
    BSFaceGenManager::UnlockFaceGenAccess();
    return bRes;
}

void BSFaceGenModelMap::FreeLRUData(BSFaceGenModelMap::DataType aDataType, BSFaceGenModelMap::Entry* aspNewEntry) {
    const char* pKey = nullptr;
    NiPointer<BSFaceGenModelMap::Entry> spEntry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    NiPointer<BSFaceGenModelMap::Entry> spLRU(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    uint32_t curTime = BSGetTickCount();
    BSFaceGenManager::LockFaceGenAccess();
    NiTMapItem<const char*, NiPointer<BSFaceGenModelMap::Entry>>* pItr = Map.GetFirstPos();
    while (pItr) {
        Map.GetNext(pItr, pKey, spEntry);
        if (!(spEntry == aspNewEntry)) {
            uint32_t dataSize;
            if (aDataType) {
                if (aDataType == BSFaceGenModelMap::DATA_TYPE_EGT) {
                    dataSize = spEntry->spModel->GetEGTDataSize();
                }
                else {
                    dataSize = 0;
                }
            }
            else {
                dataSize = spEntry->spModel->GetEGMDataSize();
            }
            if (dataSize) {
                if (spLRU.m_pObject) {
                    if (curTime - spEntry->uiTime > curTime - spLRU->uiTime) {
                        spLRU = spEntry;
                    }
                }
                else {
                    spLRU = spEntry;
                }
            }
        }
    }
    if (spLRU.m_pObject) {
        if (aDataType) {
            if (aDataType == BSFaceGenModelMap::DATA_TYPE_EGT) {
                spLRU->spModel->UnloadEGTData();
            }
        }
        else {
            spLRU->spModel->UnloadEGMData();
        }
    }
    BSFaceGenManager::UnlockFaceGenAccess();
}

int32_t BSFaceGenModelMap::GetCSEnterCounter() {
    return BSFaceGenManager::iLockCount;
}

void BSFaceGenModelMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

float fsqr(float afX) {
    return afX * afX;
}

// Reads the FaceGen controls and sizes the face coordinates to them, then builds the hair and teeth alpha property and
// the default base and detail modulation textures.
BSFaceGenManager::BSFaceGenManager() : pModelMap(nullptr) {
    AutoMemContext tempMemContext(MEM_CONTEXT_FACEGEN_SYSTEM);
    bool bFailed;
    {
        std::string fname("FACEGEN\\SI.CTL");
        bFailed = !fanReadCtlFile(fname, FgData.egmKey, FgData.egtKey, iBasisSize, LinCtrls, GenCtrls);
    }
    if (bFailed) {
        std::string fname("FACEGEN\\SI.CTL");
        if (!fanReadCtlFile(fname, FgData.egmKey, FgData.egtKey, iBasisSize, LinCtrls, GenCtrls)) {
            Warning("MODELS: Unable to load CTL file");
        }
    }
    for (uint32_t i = 0; i < 2; ++i) {
        for (uint32_t j = 0; j < 2; ++j) {
            if (LinCtrls.grp[i][j].size()) {
                FanLinCtlS* pCtl = &LinCtrls.grp[i][j][0];
                if (pCtl) {
                    FgData.coord[i][j].resize(pCtl->ctl.ncols, pCtl->ctl.nrows);
                }
            }
            else {
                FgData.coord[i][j].resize(0, 0);
            }
        }
    }
    spHairTeethAlphaProp = new NiAlphaProperty();
    spHairTeethAlphaProp->SetAlphaTesting(true);
    spHairTeethAlphaProp->SetTestMode(NiAlphaProperty::TEST_GREATER);
    spHairTeethAlphaProp->SetTestRef(100);
    NiPixelData* pBasePixelData = new NiPixelData(32u, 32u, NiPixelFormat::RGBA32, 1u, 1u);
    // 32 x 32 RGBA32 pixels.
    ourMemset(pBasePixelData->GetPixels(0u, 0u), 128, 0x1000u);
    pBasePixelData->MarkAsChanged();
    pBasePixelData->SetNoConvert(true);
    {
        NiFixedString kName("DefaultBaseModFaceGenTexture");
        spDefaultBaseMod = NiSourceTexture::Create(
            pBasePixelData,
            kName.GetHandleString(),
            NiSourceTexture::ms_kDefaultPrefs
        );
    }
    NiPixelData* pDetailPixelData = new NiPixelData(32u, 32u, NiPixelFormat::RGBA32, 1u, 1u);
    uint8_t* pDecalPixels = pDetailPixelData->GetPixels(0u, 0u);
    for (int32_t i = 0; i < 1024; ++i) {
        pDecalPixels[4 * i] = 62;
        pDecalPixels[4 * i + 1] = 65;
        pDecalPixels[4 * i + 2] = 62;
        pDecalPixels[4 * i + 3] = 64;
    }
    pDetailPixelData->MarkAsChanged();
    pDetailPixelData->SetNoConvert(true);
    {
        NiFixedString kName("DefaultDetailModFaceGenTexture");
        spDefaultDetailMod = NiSourceTexture::Create(
            pDetailPixelData,
            kName.GetHandleString(),
            NiSourceTexture::ms_kDefaultPrefs
        );
    }
    BSFaceGenManager::bUseProceduralEyeBlinking = true;
}

// The textures, the alpha property and the FaceGen data are the members' own destructors.
BSFaceGenManager::~BSFaceGenManager() {
    BSFaceGenManager::ClearModelCache();
    if (pModelMap) {
        pModelMap->UnloadAllEGMAndEGTData(false);
    }
    if (pModelMap) {
        delete pModelMap;
    }
    pModelMap = nullptr;
}

BSFaceGenModelExtraData* BSFaceGenManager::GetFaceGenModelExtraData(NiTriBasedGeom* apGeom) {
    if (!apGeom) {
        return nullptr;
    }
    return static_cast<BSFaceGenModelExtraData*>(apGeom->GetExtraData(BSFaceGenModelExtraData::Tag()));
}

BSFaceGenModel* BSFaceGenManager::GetFaceGenModel(NiTriBasedGeom* apGeom) {
    BSFaceGenModelExtraData* pExtraData = GetFaceGenModelExtraData(apGeom);
    if (pExtraData) {
        return pExtraData->GetModel();
    }

    return nullptr;
}

bool BSFaceGenManager::GetUseEmotions() {
    return BSFaceGenManager::bEmotions;
}

// The age is the floored coordinate attribute rounded to float and truncated to a signed byte. The accessory loop
// passes the face part counter, 8 after its loop, as the head part index.
void BSFaceGenManager::QueueHeadParts(TESNPC* apNPC, IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    alignas(AutoMemContext) char acMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (acMemContext) AutoMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp",
        3640
    );
    alignas(FaceGenNpcData) char acNPCData[sizeof(FaceGenNpcData)];
    FaceGenNpcData* pNPCData = ::new (acNPCData) FaceGenNpcData;
    TESActorBase::SEX eSex = static_cast<TESActorBase::SEX>(apNPC->GetSex());
    TESRace* pRace = apNPC->GetRace();
    pRace->GetFaceGenData(apNPC, *pNPCData, eSex, true);

    float fAge = GetFaceCoordAttribute(pNPCData->FaceCoord, FANATT_AGE, FANTYPE_GEO);
    int8_t cAge = static_cast<int8_t>(static_cast<int32_t>(std::floorf(fAge)));

    if (pNPCData->pEyes) {
        char acEyesFile[0x104];
        BSstrcpy(acEyesFile, 0x104, "Textures\\");
        BSstrcat(acEyesFile, 0x104, pNPCData->pEyes->GetTextureName());
        pModelLoader->QueueTexture(acEyesFile, aePriority, apParent);
    }
    else {
        pModelLoader->QueueTexture("Textures\\Characters\\Eyes\\EyeDefault.dds", aePriority, apParent);
    }

    if (pNPCData->pHair) {
        pModelLoader->QueueModel(pNPCData->pHair, aePriority, apParent, LOD_MULT_NONE, false, false, true);
        char acHairFile[0x104];
        BSstrcpy(acHairFile, 0x104, "Textures\\");
        BSstrcat(acHairFile, 0x104, pNPCData->pHair->GetTextureName());
        pModelLoader->QueueTexture(acHairFile, aePriority, apParent);
        pModelLoader->QueueEGMFile(pNPCData->pHair, aePriority, apParent, 9);
    }

    char acModFile[0x104];
    if (apNPC->GetHeadPartModTextureFileName(RHP_HEAD, acModFile, 0x104)) {
        pModelLoader->QueueTexture(acModFile, aePriority, apParent);
    }

    uint32_t i;
    for (i = 0; i < 8; ++i) {
        if (i == 6 || i == 7) {
            continue;
        }
        // SWEEP-INLINE 0x877a30
        if (!pNPCData->pModel[i]) {
            continue;
        }
        // SWEEP-INLINE 0x877a30
        if (!pNPCData->pTexture[i]) {
            continue;
        }
        // 0xE for the head and the first face part, which get a skin
        // texture too.
        int32_t iPartKind = 1;
        if (i == 0 || i == 1) {
            iPartKind = 0xE;
        }
        alignas(BSStringT<char>) char acDetailFile[sizeof(BSStringT<char>)];
        BSStringT<char>* pDetailFile = ::new (acDetailFile) BSStringT<char>;
        char acFaceFile[0x104];
        BSstrcpy(acFaceFile, 0x104, "Textures\\");
        BSstrcat(acFaceFile, 0x104, pNPCData->pTexture[i]->GetTextureName());
        pModelLoader->QueueTexture(acFaceFile, aePriority, apParent);
        if (i == 0 && cAge >= 0) {
            const char* pTextureName = pNPCData->pTexture[i]->GetTextureName();
            if (GetAsNearestDetailDDSFile(*pDetailFile, pNPCData->eSex, static_cast<uint8_t>(cAge), pTextureName)) {
                BSstrcpy(acFaceFile, 0x104, pDetailFile->String());
                pModelLoader->QueueTexture(acFaceFile, aePriority, apParent);
            }
        }
        char acNormalFile[0x104];
        TESTexture::GetAsNormalFile(acFaceFile, acNormalFile);
        pModelLoader->QueueTexture(acNormalFile, aePriority, apParent);
        if (pNPCData->bFaceGenTexturing && iPartKind == 0xE) {
            char acSkinFile[0x104];
            TESTexture::GetAsSkinFile(acFaceFile, acSkinFile);
            pModelLoader->QueueTexture(acSkinFile, aePriority, apParent);
        }
        pDetailFile->~BSStringT<char>();
    }

    for (i = 0; i < 8; ++i) {
        TESModel* pModel = pNPCData->pModel[i];
        if (pModel && pModel->GetModelLength()) {
            pModelLoader->QueueModel(pModel, aePriority, apParent, LOD_MULT_NONE, false, false, true);
            pModelLoader->QueueEGMFile(pModel, aePriority, apParent, static_cast<int32_t>(i));
            pModelLoader->QueueTRIFile(pModel, aePriority, apParent, static_cast<int32_t>(i));
        }
    }

    for (BSSimpleList<BGSHeadPart*>* pNode = &pNPCData->Accessories; pNode; pNode = pNode->GetNext()) {
        BGSHeadPart* pPart = pNode->GetItem();
        if (pPart && pPart->GetModelLength()) {
            pModelLoader->QueueModel(pPart, aePriority, apParent, LOD_MULT_NONE, false, false, true);
            pModelLoader->QueueEGMFile(pPart, aePriority, apParent, static_cast<int32_t>(i));
            pModelLoader->QueueTRIFile(pPart, aePriority, apParent, static_cast<int32_t>(i));
        }
    }

    pNPCData->~FaceGenNpcData();
    pMemContext->~AutoMemContext();
}

void BSFaceGenManager::PrepareHeadForShaders(BSFaceGenNiNode* apNode, FaceGenNpcData& arNpcData) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp",
        3114
    );
    NiFixedString leftEye("FaceGenEyeLeft");
    NiFixedString rightEye("FaceGenEyeRight");
    NiFixedString headAccessory("FaceGenAccessory");
    NiFixedString hair("FaceGenHair");
    BSStringT<char> filename;
    BSStringT<char> fileEGM;
    BSStringT<char> fileDDS;
    BSStringT<char> fileDetail;
    BSStringT<char> fileSpecular;
    NiPointer<NiSourceTexture> sptexNormal(static_cast<NiSourceTexture*>(nullptr));
    NiPointer<NiTexture> sptexBaseMod(static_cast<NiTexture*>(nullptr));
    NiPointer<NiSourceTexture> sptexDetailMod(static_cast<NiSourceTexture*>(nullptr));
    uint32_t uiShaderIndex = 0;
    BSFaceGenManager::LockFaceGenAccess();
    if (apNode && &arNpcData) {
        char cTexturePath[268];
        float fAge = BSFaceGenManager::GetFaceCoordAttribute(arNpcData.FaceCoord, FANATT_AGE, FANTYPE_GEO);
        char cAge = static_cast<int32_t>(std::floorf(fAge));
        BSstrcpy(cTexturePath, 260, "Textures\\");
        for (uint32_t uiPart = 0; uiPart < 8; ++uiPart) {
            fileDetail.Set("", 0);
            if (uiPart != 6 && uiPart != 7) {
                uiShaderIndex = uiPart > 1 ? 1 : 14;
                if (arNpcData.pModel[uiPart] && arNpcData.pTexture[uiPart]) {
                    NiAVObject* pObj = apNode->GetObjectByName(arNpcData.pHeadPartNodeName[uiPart]);
                    if (pObj) {
                        NiTriBasedGeom* pGeom = pObj->IsTriBasedGeom();
                        if (pGeom) {
                            BSFaceGenModel* pModelFG = BSFaceGenManager::GetFaceGenModel(pGeom);
                            if (pModelFG) {
                                int32_t iLightingShader;
                                BSShaderManager::PrepareObject(pObj, 0, 0);
                                NiShadeProperty* pShaderProp = static_cast<NiShadeProperty*>(pObj->GetProperty(3));
                                if (pShaderProp) {
                                    iLightingShader = pShaderProp->iShaderPropertyType >= 8 &&
                                        pShaderProp->iShaderPropertyType <= 12;
                                }
                                else {
                                    iLightingShader = 0;
                                }
                                BSShaderPPLightingProperty* pShaderPropLighting = nullptr;
                                if (iLightingShader) {
                                    pShaderPropLighting = static_cast<BSShaderPPLightingProperty*>(pShaderProp);
                                }
                                if (pShaderPropLighting) {
                                    if (arNpcData.spTexBaseMod.m_usSize && arNpcData.bFaceGenTexturing) {
                                        sptexBaseMod = arNpcData.spTexBaseMod[uiPart];
                                    }

                                    if (uiPart ||
                                        cAge < 0 ||
                                        !BSFaceGenManager::GetAsNearestDetailDDSFile(
                                            fileDetail,
                                            arNpcData.eSex,
                                            static_cast<uint8_t>(cAge),
                                            arNpcData.pTexture[uiPart]->GetTextureName()
                                        )) {
                                        if (!arNpcData.spTexBaseMod.m_usSize ||
                                            !arNpcData.spTexBaseMod[uiPart].m_pObject) {
                                            pModelFG->ApplyCoordinateTexturingToMesh(
                                                &arNpcData.FaceCoord,
                                                sptexBaseMod,
                                                BSFaceGenManager::GetEGTClampSize()
                                            );
                                        }
                                        BSStringT<char> tempstr;
                                        TESTexture* pTexture = arNpcData.pTexture[uiPart];
                                        pTexture->GetAsNormalFile(tempstr);
                                        if (tempstr.pString) {
                                            fileSpecular = "Textures\\";
                                            fileSpecular += static_cast<const char*>(tempstr.pString);
                                        }
                                    }
                                    else {
                                        if (!arNpcData.spTexBaseMod.m_usSize ||
                                            !arNpcData.spTexBaseMod[uiPart].m_pObject) {
                                            pModelFG->ApplyCoordinateTexturingToMesh(
                                                &arNpcData.FaceCoord,
                                                sptexBaseMod,
                                                BSFaceGenManager::GetEGTClampSize()
                                            );
                                        }
                                        TES::pInstance
                                            ->CreateTextureImage(fileDetail.pString, sptexDetailMod, true, true);
                                        BSUtils::CreateNormalMapString(fileSpecular, fileDetail.pString);
                                    }
                                    if (fileSpecular.pString) {
                                        TES::pInstance
                                            ->CreateTextureImage(fileSpecular.pString, sptexNormal, true, true);
                                    }
                                    if (sptexNormal.m_pObject) {
                                        pShaderPropLighting->SetNormalTexture(0, sptexNormal.m_pObject);
                                        uint32_t eFormat = sptexNormal->NiTexture::GetPixelFormat()->m_eFormat;
                                        bool bSpecular = eFormat == 5 || eFormat == 6 || eFormat == 1;
                                        pShaderPropLighting->SetFlag(0, bSpecular);
                                    }
                                    else if (!pShaderPropLighting->GetNormalTexture(0)) {
                                        NiSourceTexture* pDefNormalMap = BSShader::GetDefNormalMap();
                                        pShaderPropLighting->SetNormalTexture(0, pDefNormalMap);
                                    }
                                    if (arNpcData.bFaceGenTexturing && uiShaderIndex == 14) {
                                        NiTexture* pDetailTexture;
                                        pShaderPropLighting->SetDiffuseTexture(1u, sptexBaseMod.m_pObject);
                                        if (sptexDetailMod.m_pObject) {
                                            pDetailTexture = sptexDetailMod.m_pObject;
                                        }
                                        else {
                                            pDetailTexture = BSFaceGenManager::GetDefaultDetailModulationTexture();
                                        }
                                        pShaderPropLighting->SetNormalTexture(1, pDetailTexture);
                                        pShaderPropLighting->SetFlag(BSShaderProperty::BSSP_FLAG_FACEGENBLEND, 1);
                                        NiMaterialProperty* pMatProp = static_cast<NiMaterialProperty*>(
                                            pObj->GetProperty(2)
                                        );
                                        if (pMatProp && pMatProp->m_fShine < 1.0) {
                                            pMatProp->SetShineness(4.0f);
                                        }

                                        if (fileSpecular.pString) {
                                            char cSpecularFile[264];
                                            char cSkinFile[268];
                                            BSsprintf(cSpecularFile, 260, "%s", fileSpecular.String());
                                            BSShaderManager::GetModifiedTextureFilename(
                                                cSkinFile,
                                                cSpecularFile,
                                                "_sk",
                                                true
                                            );
                                            if (cSkinFile[0]) {
                                                NiPointer<NiSourceTexture> spTempSourceTexture(
                                                    static_cast<NiSourceTexture*>(nullptr)
                                                );
                                                TES::pInstance
                                                    ->CreateTextureImage(cSkinFile, spTempSourceTexture, true, true);
                                                pShaderPropLighting->SetTexture(
                                                    BSShaderProperty::TEXTYPE_GLOW,
                                                    0,
                                                    spTempSourceTexture.m_pObject
                                                );
                                            }
                                        }
                                    }
                                    sptexBaseMod = (sptexDetailMod = nullptr);
                                    sptexNormal = nullptr;
                                    BSFaceGenManager::PrecacheFaceGeometry(pObj, uiShaderIndex, 1u);
                                    pShaderPropLighting->PrecacheTextures();
                                }
                            }
                        }
                    }
                }
            }
        }
        for (uint32_t i = 0; i < apNode->GetChildCount(); ++i) {
            NiAVObject* pChild = apNode->GetAt(i);
            if (pChild) {
                uint32_t uiCompress = 1;
                if (NiFixedString::HandlesEqual(pChild->m_kName, leftEye) ||
                    NiFixedString::HandlesEqual(pChild->m_kName, rightEye)) {
                    uiCompress = 0;
                    BSShaderManager::PrepareObject(pChild, 0, 0);
                }
                else {
                    size_t uiHairLength = std::strlen(hair.GetHandleString());
                    const char* pHairName = hair.GetHandleString();
                    const char* pChildName = pChild->m_kName.GetHandleString();
                    if (!_strnicmp(pChildName, pHairName, uiHairLength) ||
                        NiFixedString::HandlesEqual(pChild->m_kName, headAccessory)) {
                        int32_t iHairShader;
                        BSShaderManager::PrepareObject(pChild, 0, 0);
                        NiShadeProperty* pShadeProp = static_cast<NiShadeProperty*>(pChild->GetProperty(3));
                        if (pShadeProp) {
                            iHairShader = pShadeProp->iShaderPropertyType == 8;
                        }
                        else {
                            iHairShader = 0;
                        }
                        BSShaderPPLightingProperty* pHairProp = nullptr;
                        if (iHairShader) {
                            pHairProp = static_cast<BSShaderPPLightingProperty*>(pShadeProp);
                        }
                        if (pHairProp) {
                            uint32_t uiCol = arNpcData.iHairColor;
                            float fBlue = static_cast<double>(static_cast<uint8_t>(uiCol >> 16)) / 255.0;
                            float fGreen = static_cast<double>(static_cast<uint8_t>(uiCol >> 8)) / 255.0;
                            float fRed = static_cast<double>(static_cast<uint8_t>(uiCol)) / 255.0;
                            NiColorA kTint(fRed, fGreen, fBlue, 1.0f);
                            pHairProp->pHairTintColor = kTint;
                            pHairProp->PrecacheTextures();
                        }
                        else {
                            int32_t iSM3Shader;
                            if (pShadeProp) {
                                iSM3Shader = pShadeProp->iShaderPropertyType == 12;
                            }
                            else {
                                iSM3Shader = 0;
                            }
                            Lighting30ShaderProperty* pSM3Prop = nullptr;
                            if (iSM3Shader) {
                                pSM3Prop = static_cast<Lighting30ShaderProperty*>(pShadeProp);
                            }
                            if (pSM3Prop) {
                                uint32_t uiCol = arNpcData.iHairColor;
                                float fBlue = static_cast<double>(static_cast<uint8_t>(uiCol >> 16)) / 255.0;
                                float fGreen = static_cast<double>(static_cast<uint8_t>(uiCol >> 8)) / 255.0;
                                float fRed = static_cast<double>(static_cast<uint8_t>(uiCol)) / 255.0;
                                NiColorA kTint(fRed, fGreen, fBlue, 1.0f);
                                pSM3Prop->pHairTintColor = kTint;
                                pSM3Prop->PrecacheTextures();
                            }
                        }
                    }
                }
                BSFaceGenManager::PrecacheFaceGeometry(pChild, uiShaderIndex, uiCompress);
            }
        }
        apNode->UpdateProperties();
        NiUpdateData kUpdateData(0.0f, false, false);
        apNode->Update(kUpdateData);
        BSFaceGenManager::UnlockFaceGenAccess();
    }
    else {
        BSFaceGenManager::UnlockFaceGenAccess();
    }
}

// Hashes the name once, for the .ddx file, and searches every .dds age with those hashes.
const char* BSFaceGenManager::GetAsNearestDetailDDSFile(
    BSStringT<char>& aOutFilename,
    int32_t aeSex,
    uint8_t acAge,
    const char* apModel
) {
    char cHeadName[268];
    BSHash rDirectoryHash;
    BSHash rFileNameHash;
    int32_t iAge = acAge;
    char cGender = 'M';
    if (!apModel || iAge > 100 || aeSex >= 2) {
        return nullptr;
    }
    BSstrcpy(cHeadName, 260, apModel);
    char* pExtension = std::strrchr(cHeadName, '.');
    if (!pExtension) {
        return nullptr;
    }
    *pExtension = 0;
    if (aeSex == 1) {
        cGender = 'F';
    }
    aOutFilename.SPrintF("Textures\\%s%c%d.ddx", cHeadName, cGender, iAge);
    BSHash::MakeDirAndFile(aOutFilename.pString, rDirectoryHash, rFileNameHash);
    BSFileEntry* pFileEntry = ArchiveManager::GetFileEntryForFileFromAllArchives(
        ARCHIVE_TYPE_INDEX_TEXTURES,
        rDirectoryHash,
        rFileNameHash,
        aOutFilename.pString
    );
    if (!pFileEntry) {
        iAge = 10 * (iAge / 10);
        while (true) {
            aOutFilename.SPrintF("Textures\\%s%c%d.dds", cHeadName, cGender, iAge);
            pFileEntry = ArchiveManager::GetFileEntryForFileFromAllArchives(
                ARCHIVE_TYPE_INDEX_TEXTURES,
                rDirectoryHash,
                rFileNameHash,
                aOutFilename.pString
            );
            if (pFileEntry) {
                break;
            }
            iAge -= 10;
            if (iAge < 0) {
                aOutFilename.Set("", 0);
                return nullptr;
            }
        }
    }
    return aOutFilename.pString;
}

void BSFaceGenManager::SetFaceCoordAttributes(FR2MatrixVTC<float> (*apCoord)[2], FanTypeE aeFanType, float* afValues) {
    GetInstance()->GenCtrls.setAttributes(FANRACE_ALL, aeFanType, afValues, apCoord);
}

void BSFaceGenManager::SetFaceCoordAttribute(
    FR2MatrixVTC<float> (*apCoord)[2],
    FanAttribE aeFanAttribute,
    FanTypeE aeFanType,
    float afValue
) {
    float afValues[2];
    for (int32_t eFanAttribute = 0; eFanAttribute < 2; ++eFanAttribute) {
        if (eFanAttribute == aeFanAttribute) {
            afValues[eFanAttribute] = afValue;
        }
        else {
            afValues[eFanAttribute] = GetFaceCoordAttribute(apCoord, static_cast<FanAttribE>(eFanAttribute), aeFanType);
        }
    }
    SetFaceCoordAttributes(apCoord, aeFanType, afValues);
}

bool BSFaceGenManager::CompareFaceGenCoord(
    const FR2MatrixVTC<float> (*apCoord1)[2][2],
    const FR2MatrixVTC<float> (*apCoord2)[2][2]
) {
    if (!apCoord1 && !apCoord2) {
        return true;
    }
    if (!apCoord1 || !apCoord2) {
        return false;
    }
    for (int32_t i = 0; i < 2; ++i) {
        for (int32_t j = 0; j < 2; ++j) {
            if ((*apCoord1)[i][j] != (*apCoord2)[i][j]) {
                return true;
            }
        }
    }
    return false;
}

void BSFaceGenManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Deleting pInstance tests the copy for null again.
void BSFaceGenManager::DestroySystem() {
    if (pInstance) {
        BSFaceGenManager* pManager = pInstance;
        if (pManager) {
            delete pManager;
        }
        pInstance = nullptr;
    }
}

// The model name loses its extension; a HAIR_PART of NORMAL or HAT adds its headwear suffix.
const char* BSFaceGenManager::GetAsEGMFile(BSStringT<char>& aOutFilename, const char* apModel, HAIR_PART aeHairPart) {
    char* pExtension = nullptr;
    if (!apModel) {
        return nullptr;
    }
    char acModel[260];
    char acFilename[260];
    BSstrcpy(acModel, 260, apModel);
    pExtension = std::strrchr(acModel, '.');
    if (!pExtension) {
        return nullptr;
    }
    *pExtension = '\0';
    if (aeHairPart >= HAIR_PART_NORMAL && aeHairPart <= HAIR_PART_HAT) {
        BSsprintf(acFilename, 260, "%s%s.egm", acModel, pHairPartName[aeHairPart]);
    }
    else {
        BSsprintf(acFilename, 260, "%s.egm", acModel);
    }
    aOutFilename.Set(acFilename, 0);
    return aOutFilename.String();
}

// The model name loses its extension; a HAIR_PART of NORMAL or HAT adds its headwear suffix.
const char* BSFaceGenManager::GetAsTRIFile(BSStringT<char>& aOutFilename, const char* apModel, HAIR_PART aeHairPart) {
    char* pExtension = nullptr;
    if (!apModel) {
        return nullptr;
    }
    char acModel[260];
    char acFilename[260];
    BSstrcpy(acModel, 260, apModel);
    pExtension = std::strrchr(acModel, '.');
    if (!pExtension) {
        return nullptr;
    }
    *pExtension = '\0';
    if (aeHairPart >= HAIR_PART_NORMAL && aeHairPart <= HAIR_PART_HAT) {
        BSsprintf(acFilename, 260, "%s%s.tri", acModel, pHairPartName[aeHairPart]);
    }
    else {
        BSsprintf(acFilename, 260, "%s.tri", acModel);
    }
    aOutFilename.Set(acFilename, 0);
    return aOutFilename.String();
}

BSFaceGenModelMap* BSFaceGenManager::GetModelCache() {
    if (pInstance) {
        return pInstance->pModelMap;
    }

    return nullptr;
}

auto BSFaceGenManager::GetFaceGenCoord() -> FR2MatrixVTC<float> (*)[2][2] {
    return &GetInstance()->FgData.coord;
}

void BSFaceGenManager::InitFaceGenCoord(FR2MatrixVTC<float> (*apCoord)[2][2]) {
    if (!apCoord) {
        return;
    }
    (*apCoord)[0][0].resize(50, 1);
    (*apCoord)[0][1].resize(30, 1);
    (*apCoord)[1][0].resize(50, 1);
}

// An empty source matrix empties the target. The column getter is called before the row getter for resize's arguments.
void BSFaceGenManager::CopyFaceGenCoord(
    const FR2MatrixVTC<float> (*apCopyFromCoord)[2][2],
    FR2MatrixVTC<float> (*apCopyToCoord)[2][2],
    bool abGeometryOnly,
    bool abTextureOnly
) {
    if (!apCopyFromCoord || !apCopyToCoord) {
        return;
    }
    for (int32_t i = 0; i < 2; ++i) {
        for (int32_t j = 0; j < 2; ++j) {
            if ((*apCopyFromCoord)[i][j].numRows() && (*apCopyFromCoord)[i][j].numCols()) {
                uint32_t uiCols = (*apCopyFromCoord)[i][j].numCols();
                uint32_t uiRows = (*apCopyFromCoord)[i][j].numRows();
                (*apCopyToCoord)[i][j].resize(uiRows, uiCols);
                if ((abGeometryOnly && i == 0) || (abTextureOnly && i == 1) || abGeometryOnly == abTextureOnly) {
                    (*apCopyToCoord)[i][j] = (*apCopyFromCoord)[i][j];
                }
            }
            else {
                (*apCopyToCoord)[i][j].resize(0, 0);
            }
        }
    }
}

// Coord1 alone or coord2 alone is copied; both are added element by element, except the texture coordinates when
// abNoTextureMerge (coord1's are copied). A positive afClampVal scales a result whose root mean square exceeds it down
// to it. The mean is lengthSqr's float divided by the row count, and the root is the float sqrt.
void BSFaceGenManager::MergeFaceGenCoord(
    const FR2MatrixVTC<float> (*apCopyFromCoord1)[2][2],
    const FR2MatrixVTC<float> (*apCopyFromCoord2)[2][2],
    FR2MatrixVTC<float> (*apCopyToCoord)[2][2],
    bool abNoTextureMerge,
    float afClampVal
) {
    if (!apCopyToCoord || (!apCopyFromCoord1 && !apCopyFromCoord2)) {
        return;
    }
    if (!apCopyFromCoord1) {
        CopyFaceGenCoord(apCopyFromCoord2, apCopyToCoord, false, false);
        return;
    }
    if (!apCopyFromCoord2) {
        CopyFaceGenCoord(apCopyFromCoord1, apCopyToCoord, false, false);
        return;
    }
    for (int32_t i = 0; i < 2; ++i) {
        for (int32_t j = 0; j < 2; ++j) {
            uint32_t uiRows = (*apCopyFromCoord1)[i][j].numRows();
            uint32_t uiCols = (*apCopyFromCoord1)[i][j].numCols();
            if (uiRows && uiCols) {
                (*apCopyToCoord)[i][j].resize(uiRows, uiCols);
                if (i == 1 && abNoTextureMerge) {
                    (*apCopyToCoord)[i][j] = (*apCopyFromCoord1)[i][j];
                    if (static_cast<double>(afClampVal) > 0.0) {
                        float fMean = static_cast<float>(
                            (*apCopyToCoord)[i][j].lengthSqr() / static_cast<double>(uiRows)
                        );
                        float fRoot = std::sqrt(fMean);
                        if (afClampVal < fRoot) {
                            float fScale = afClampVal / fRoot;
                            (*apCopyToCoord)[i][j] *= fScale;
                        }
                    }
                }
                else {
                    uint32_t uiCount = uiRows * uiCols;
                    (*apCopyToCoord)[i][j].data.clear();
                    for (uint32_t k = 0; k < uiCount; ++k) {
                        float fValue1 = const_cast<FloatVector&>((*apCopyFromCoord1)[i][j].data)[k];
                        float fValue2 = const_cast<FloatVector&>((*apCopyFromCoord2)[i][j].data)[k];
                        float fSum = fValue1 + fValue2;
                        (*apCopyToCoord)[i][j].data.push_back(fSum);
                    }
                    if (static_cast<double>(afClampVal) > 0.0) {
                        float fMean = static_cast<float>(
                            (*apCopyToCoord)[i][j].lengthSqr() / static_cast<double>(uiRows)
                        );
                        float fRoot = std::sqrt(fMean);
                        if (afClampVal < fRoot) {
                            float fScale = afClampVal / fRoot;
                            (*apCopyToCoord)[i][j] *= fScale;
                        }
                    }
                }
            }
            else {
                (*apCopyToCoord)[i][j].resize(0, 0);
            }
        }
    }
}

const char* BSFaceGenManager::GetAsEGTFile(BSStringT<char>& aOutFilename, const char* apModel) {
    char acModel[264];
    char acFilename[264];
    if (!apModel) {
        return nullptr;
    }
    BSstrcpy(acModel, 260, apModel);
    char* pExtension = std::strrchr(acModel, '.');
    if (!pExtension) {
        return nullptr;
    }
    *pExtension = 0;
    BSsprintf(acFilename, 260u, "%s.egt", acModel);
    aOutFilename.Set(acFilename, 0u);
    return aOutFilename.pString;
}

// Without a known headwear variant the hair gets the first suffix.
char* BSFaceGenManager::GetHairName(char* apHairName, HAIR_PART aeHairPart) {
    ourStrcpy(apHairName, "FaceGenHair");
    if (aeHairPart > HAIR_PART_NONE && aeHairPart < HAIR_PART_COUNT) {
        std::strcat(apHairName, pHairPartName[aeHairPart]);
    }
    else {
        std::strcat(apHairName, pHairPartName[HAIR_PART_NORMAL]);
    }
    return apHairName;
}

void BSFaceGenManager::AttachHairToHead(
    BSFaceGenNiNode*& apOutNodeBiped,
    BSFaceGenNiNode*& apOutNodeSkinned,
    FaceGenNpcData& aNpcData,
    bool abForceNoSkinning
) {
    BSHash kDirectoryHash;
    BSHash kFileNameHash;
    NiMatrix3 matrot;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp",
        2875
    );
    BSStringT<char> filename;
    BSStringT<char> fileEGM;
    BSStringT<char> fileTRI;
    BSStringT<char> fileEGT;
    BSStringT<char> fileFIM;
    NiPointer<NiSourceTexture> sptexBase(static_cast<NiSourceTexture*>(nullptr));
    NiPointer<NiTriBasedGeom> spShape(static_cast<NiTriBasedGeom*>(nullptr));
    matrot.MakeYRotation(-1.5707964f);
    if (aNpcData.pHair) {
        for (HAIR_PART eHairPart = HAIR_PART_NORMAL; eHairPart < HAIR_PART_COUNT;
             eHairPart = static_cast<HAIR_PART>(eHairPart + 1)) {
            filename.SPrintF("Meshes\\%s", aNpcData.pHair->GetModel());
            const char* pEGM = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, eHairPart);
            const char* pTRI = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, eHairPart);
            BSFaceGenModel* pModelFG = nullptr;
            BSHash::MakeDirAndFile(pEGM, kDirectoryHash, kFileNameHash);
            if (ArchiveManager::GetFileEntryForFileFromAllArchives(
                    ARCHIVE_TYPE_INDEX_MESHES,
                    kDirectoryHash,
                    kFileNameHash,
                    pEGM
                )) {
                pModelFG = BSFaceGenManager::GetFaceGenModel(
                    pEGM,
                    filename.pString,
                    pTRI,
                    nullptr,
                    true,
                    eHairPart,
                    false
                );
            }
            else if (eHairPart == HAIR_PART_NORMAL) {
                pEGM = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
                pTRI = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
                pModelFG = BSFaceGenManager::GetFaceGenModel(
                    pEGM,
                    filename.pString,
                    pTRI,
                    nullptr,
                    true,
                    eHairPart,
                    false
                );
                eHairPart = HAIR_PART_COUNT;
            }
            if (pModelFG) {
                pModelFG->ApplyCoordinateToNewMesh(&aNpcData.FaceCoord, spShape);
            }
            if (spShape.m_pObject) {
                char tempHairName[32];
                if (eHairPart == HAIR_PART_HAT) {
                    spShape->SetAppCulled(true);
                }
                char* pHairName = BSFaceGenManager::GetHairName(tempHairName, eHairPart);
                spShape->SetName(pHairName);
                BSFaceGenManager::ApplyHairMorph(spShape.m_pObject, aNpcData.fHairLength);
                filename.SPrintF("Textures\\%s", aNpcData.pHair->GetTextureName());
                TES::pInstance->CreateTextureImage(filename.pString, sptexBase, false, false);
                if (sptexBase.m_pObject) {
                    int32_t iLightingShader;
                    NiShadeProperty* pProp = static_cast<NiShadeProperty*>(spShape->GetProperty(3));
                    if (pProp) {
                        iLightingShader = pProp->iShaderPropertyType >= 8 && pProp->iShaderPropertyType <= 12;
                    }
                    else {
                        iLightingShader = 0;
                    }
                    BSShaderPPLightingProperty* pShaderProp = nullptr;
                    if (iLightingShader) {
                        pShaderProp = static_cast<BSShaderPPLightingProperty*>(pProp);
                    }
                    if (pShaderProp) {
                        pShaderProp->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, sptexBase.m_pObject);
                    }
                }
                if (!spShape->GetProperty(0)) {
                    NiAlphaProperty* pAlphaProp = BSFaceGenManager::GetHairTeethAlphaProperty();
                    spShape->AttachProperty(pAlphaProp);
                }
                if (spShape->GetSkinInstance()) {
                    if (abForceNoSkinning) {
                        spShape->SetSkinInstance(nullptr);
                        spShape->SetTranslate(NiPoint3::ZERO);
                        spShape->SetRotate(matrot);
                        apOutNodeBiped->AttachChild(spShape.m_pObject, 1);
                    }
                    else {
                        apOutNodeSkinned->AttachChild(spShape.m_pObject, 1);
                    }
                }
                else {
                    spShape->SetRotate(matrot);
                    apOutNodeBiped->AttachChild(spShape.m_pObject, 1);
                }
            }
            pModelFG = nullptr;
            spShape = static_cast<NiTriBasedGeom*>(nullptr);
            sptexBase = static_cast<NiSourceTexture*>(nullptr);
        }
    }
}

// Clears the requested range or fills it with the largest finite float.
void BSFaceGenManager::ResetQuantityArray(float* apQuantities, uint32_t aiNumQuantities, bool abForceZero) {
    if (!apQuantities || !aiNumQuantities) {
        return;
    }
    if (abForceZero) {
        ourMemset(apQuantities, 0, aiNumQuantities << 2);
        return;
    }
    for (uint32_t i = 0; i < aiNumQuantities; ++i) {
        apQuantities[i] = std::numeric_limits<float>::max();
    }
}

BSFaceGenModel* BSFaceGenManager::GetFaceGenModel(
    const char* apEGMFilename,
    const char* apNIFFilename,
    const char* apTRIFilename,
    const char* apEGTFilename,
    bool abAllowCreate,
    HAIR_PART aeHairPart,
    bool abIsAccessory
) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp",
        997
    );
    NiPointer<BSFaceGenModel> spNewModel(static_cast<BSFaceGenModel*>(nullptr));
    if (!apEGMFilename && !apEGTFilename) {
        return nullptr;
    }
    if (!BSFaceGenManager::GetInstance()->pModelMap) {
        BSFaceGenModelMap* pModelMap;
        pModelMap = new BSFaceGenModelMap;
        BSFaceGenManager::GetInstance()->pModelMap = pModelMap;
        uint32_t uiMaxEGMSize = BSFaceGenManager::uiMaxEGMDataSize;
        BSFaceGenManager::GetInstance()->pModelMap->SetMaxDataSize(BSFaceGenModelMap::DATA_TYPE_EGM, uiMaxEGMSize);
        uint32_t uiMaxEGTSize = BSFaceGenManager::uiMaxEGTDataSize;
        BSFaceGenManager::GetInstance()->pModelMap->SetMaxDataSize(BSFaceGenModelMap::DATA_TYPE_EGT, uiMaxEGTSize);
    }
    if (apEGMFilename) {
        if (BSFaceGenManager::GetInstance()->pModelMap->GetAt(apEGMFilename, spNewModel)) {
            if (apEGTFilename && !spNewModel->GetEGTFilename()) {
                spNewModel->LoadModelTexture(apEGTFilename);
            }

            return spNewModel.m_pObject;
        }
    }
    else if (apEGTFilename) {
        if (BSFaceGenManager::GetInstance()->pModelMap->GetAt(apEGTFilename, spNewModel)) {
            return spNewModel.m_pObject;
        }
    }
    if (abAllowCreate) {
        spNewModel = new BSFaceGenModel;
        if (spNewModel
                ->LoadModel(apEGMFilename, apNIFFilename, apTRIFilename, apEGTFilename, aeHairPart, abIsAccessory)) {
            const char* pcKey = apEGMFilename ? apEGMFilename : apEGTFilename;
            BSFaceGenModel* pNewModel = spNewModel.m_pObject;
            BSFaceGenManager::GetInstance()->pModelMap->SetAt(pcKey, pNewModel);
        }
        else {
            spNewModel = static_cast<BSFaceGenModel*>(nullptr);
        }
    }
    return spNewModel.m_pObject;
}

// Without a manager, builds a null temporary smart pointer, reads its pointer and releases it under a construction
// flag.
NiAlphaProperty* BSFaceGenManager::GetHairTeethAlphaProperty() {
    alignas(NiPointer<NiAlphaProperty>) uint8_t aNullStorage[sizeof(NiPointer<NiAlphaProperty>)];
    bool bConstructed = false;
    NiPointer<NiAlphaProperty>* pspProperty;
    if (pInstance) {
        pspProperty = &pInstance->spHairTeethAlphaProp;
    }
    else {
        pspProperty = ::new (aNullStorage) NiPointer<NiAlphaProperty>(nullptr);
        bConstructed = true;
    }
    NiAlphaProperty* pProperty = *pspProperty;
    if (bConstructed) {
        pspProperty->~NiPointer<NiAlphaProperty>();
    }
    return pProperty;
}

// auiShaderIndex is unused. The renderer is always the DX9 one; its PrecacheGeometry and PerformPrecache are called,
// fetching the renderer each time.
void BSFaceGenManager::PrecacheFaceGeometry(NiAVObject* apObj, uint32_t aiShaderIndex, uint32_t aiCompressFlags) {
    if (!apObj) {
        return;
    }
    NiTriBasedGeom* pGeom = apObj->IsTriBasedGeom();
    if (!pGeom || !pGeom->GetModelData()) {
        return;
    }
    pGeom->GetModelData()->SetCompressFlags(aiCompressFlags);
    pGeom->GetModelData()->SetConsistency(NiGeometryData::MUTABLE);
    if (pGeom->GetSkinInstance()) {
        return;
    }
    BSShader* pShader = pGeom->GetShader();
    if (!pShader) {
        return;
    }
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    pRenderer->PrecacheGeometry(
        pGeom,
        0,
        0,
        pShader->GetShaderDecl(pGeom, static_cast<BSShaderProperty*>(pGeom->GetProperty(NiProperty::SHADE)))
    );
    static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->PerformPrecache();
}

BSFaceGenManager* BSFaceGenManager::GetInstance() {
    if (!pInstance) {
        InitializeSystem();
    }
    return pInstance;
}

void BSFaceGenManager::InitializeSystem() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp",
        790
    );
    if (!BSFaceGenManager::pInstance) {
        BSFaceGenManager* pManager;
        pManager = new BSFaceGenManager;
        BSFaceGenManager::pInstance = pManager;
    }
}

void BSFaceGenManager::ClearModelCache() {
    if (pInstance && pInstance->pModelMap) {
        pInstance->pModelMap->RemoveAllUnusedModels();
    }
}

void BSFaceGenManager::ClearEGTCache() {
    if (pInstance && pInstance->pModelMap) {
        pInstance->pModelMap->ClearEGTData();
    }
}

// Runs the model-map load within the file's EGM allocation context and restores it on every return path.
bool BSFaceGenManager::LoadEGMData(BSFaceGenModel* apModel) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    // Uses the stack context's address, not the constructor's result.
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_FACEGEN_EGM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp",
        1100
    );
    if (!pInstance) {
        pContext->~AutoMemContext();
        return false;
    }
    if (!pInstance->pModelMap) {
        pContext->~AutoMemContext();
        return false;
    }
    bool bLoaded = pInstance->pModelMap->LoadEGMData(apModel);
    pContext->~AutoMemContext();
    return bLoaded;
}

// Runs the model-map load within the file's EGT allocation context and restores it on every return path.
bool BSFaceGenManager::LoadEGTData(BSFaceGenModel* apModel) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    // Uses the stack context's address, not the constructor's result.
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_FACEGEN_EGT,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp",
        1114
    );
    if (!pInstance) {
        pContext->~AutoMemContext();
        return false;
    }
    if (!pInstance->pModelMap) {
        pContext->~AutoMemContext();
        return false;
    }
    bool bLoaded = pInstance->pModelMap->LoadEGTData(apModel);
    pContext->~AutoMemContext();
    return bLoaded;
}

// Compares the lock's result with 1.
bool BSFaceGenManager::TryLockFaceGenAccess() {
    bool bLocked = FGAccessCrit.TryLock();
    if (bLocked) {
        ++iLockCount;
    }
    return bLocked;
}

// Passes a null name to the lock and returns the new lock count.
int32_t BSFaceGenManager::LockFaceGenAccess() {
    FGAccessCrit.Lock(nullptr);
    return ++iLockCount;
}

void BSFaceGenManager::UnlockFaceGenAccess() {
    FGAccessCrit.Unlock();
    --iLockCount;
}

// The FaceGen expression an emotion shows; -1 for the neutral and unknown emotions. Disgust shows anger, and the values
// follow the game's mapping (4 -> 3, 5 -> 2, 6 -> 4, 7 -> 13).
BS_FACEGEN_EXPRESSION BSFaceGenManager::GetExpressionFromEmotion(DIALOGUE_EMOTION aeWhich) {
    BS_FACEGEN_EXPRESSION eExpression;
    switch (aeWhich) {
        case DE_ANGER: eExpression = BS_FG_EXPRESSION_ANGER; break;
        case DE_DISGUST: eExpression = BS_FG_EXPRESSION_ANGER; break;
        case DE_FEAR: eExpression = BS_FG_EXPRESSION_FEAR; break;
        case DE_SAD: eExpression = BS_FG_EXPRESSION_SAD; break;
        case DE_HAPPY: eExpression = BS_FG_EXPRESSION_HAPPY; break;
        case DE_SURPRISE: eExpression = BS_FG_EXPRESSION_SURPRISE; break;
        case DE_PAINED: eExpression = BS_FG_EXPRESSION_MOOD_PAINED; break;
        default: eExpression = BS_FG_EXPRESSION_NONE; break;
    }
    return eExpression;
}

float BSFaceGenManager::GetFaceCoordAttribute(
    const FR2MatrixVTC<float> (*apCoord)[2],
    FanAttribE aeFanAttribute,
    FanTypeE aeFanType
) {
    return GetInstance()->GenCtrls.getAttribute(FANRACE_ALL, aeFanAttribute, aeFanType, apCoord);
}

// Multiplies by the double constant -1.0 rather than negating; a value that is not >= 0 (NaN too) goes to afOutNeg.
void BSFaceGenManager::FixExclusiveValues(float afNeg, float afPos, float& afOutNeg, float& afOutPos) {
    float fValue = afNeg * -1.0 + afPos;
    if (fValue >= 0.0) {
        afOutNeg = 0.0f;
        afOutPos = fValue;
    }
    else {
        afOutNeg = fValue * -1.0;
        afOutPos = 0.0f;
    }
}

void BSFaceGenManager::RefreshMeshFromBaseMorphExtraData(
    NiTriBasedGeom* apGeom,
    NiGeometryDataIteratorNiPoint3& arIterator
) {
    BSFaceGenBaseMorphExtraData* pFaceGenExtra = static_cast<BSFaceGenBaseMorphExtraData*>(
        BSFaceGenUtils::GetFaceGenExtraData<BSFaceGenBaseMorphExtraData>(apGeom)
    );
    if (pFaceGenExtra && arIterator.m_pvCurrentPosition) {
        NiGeometryDataIteratorNiPoint3 kIterator = arIterator;
        uint32_t i = 0;
        NiPoint3* pVerts = pFaceGenExtra->pVerts;
        while (i < apGeom->GetVertexCount()) {
            kIterator.SetData(*pVerts);
            kIterator.Next();
            ++i;
            ++pVerts;
        }
        if (apGeom->GetVertices()) {
            apGeom->GetModelData()->MarkAsChanged(1);
        }
    }
}

// The direction flag is stored only when it turns.
void BSFaceGenManager::CycleFaceGenCoordValue(Actor* apActor) {
    TESNPC* pNPC = static_cast<TESNPC*>(apActor->GetObjectReference());
    FR2MatrixVTC<float> oldcoord[2][2];
    FR2MatrixVTC<float> newcoord[2][2];
    FR2MatrixVTC<float> diffcoord[2][2];
    pNPC->GetFaceCoord(oldcoord);
    pNPC->GetFaceCoord(newcoord);
    float
        val = BSFaceGenManager::GetFaceCoordValue(&oldcoord, FANTYPE_GEO, FANSYMM_SYMM, BSFaceGenManager::iCycleCtrlID);
    if (BSFaceGenManager::bCyclePositive) {
        val = val + 0.10000000149011612;
        if (val >= 10.0) {
            BSFaceGenManager::bCyclePositive = false;
        }
    }
    else {
        val = val - 0.10000000149011612;
        if (val <= -10.0) {
            BSFaceGenManager::bCyclePositive = true;
        }
    }
    BSFaceGenManager::SetFaceCoordValue(&newcoord, FANTYPE_GEO, FANSYMM_SYMM, BSFaceGenManager::iCycleCtrlID, val);
    BSFaceGenManager::OffsetFaceGenCoord(&oldcoord, &newcoord, &diffcoord, false);
    BSFaceGenNiNode* pFaceNode = apActor->GetFaceNodeBiped(nullptr);
    for (int32_t a = 0; a < 2; ++a) {
        uint32_t uiSizeArray = pFaceNode->GetArrayCount();
        for (uint32_t uiChild = 0; uiChild < uiSizeArray; ++uiChild) {
            NiTriBasedGeom* pTri;
            NiAVObject* pFaceObj = pFaceNode->GetAt(uiChild);
            if (pFaceObj) {
                pTri = pFaceObj->IsTriBasedGeom();
            }
            else {
                pTri = nullptr;
            }
            if (pTri) {
                BSFaceGenModel* pFGModel;
                BSFaceGenModelExtraData* pModelExtraData = BSFaceGenManager::GetFaceGenModelExtraData(pTri);
                if (pModelExtraData) {
                    pFGModel = pModelExtraData->GetModel();
                }
                else {
                    pFGModel = nullptr;
                }
                if (pFGModel) {
                    pFGModel->ApplyCoordinateToExistingMesh(&diffcoord, pTri, false);
                }
            }
        }
        pFaceNode = apActor->GetFaceNodeSkinned(nullptr);
    }
    FR2MatrixVTC<float>(*pOffsetCoord)[2][2] = pNPC->GetOffsetFaceCoord();
    TESActorBase::SEX eSex = pNPC->GetSex();
    FR2MatrixVTC<float>(*pMeanCoord)[2][2] = pNPC->GetFormRace()->GetMeanFaceCoord(eSex);
    BSFaceGenManager::OffsetFaceGenCoord(pMeanCoord, &newcoord, pOffsetCoord, false);
}

void BSFaceGenManager::SetUseProceduralEyeBlinking(bool abUse) {
    BSFaceGenManager::bUseProceduralEyeBlinking = abUse;
}

void BSFaceGenManager::SetToZero(float* apQuantities, uint32_t aiNumQuantities) {
    if (apQuantities && aiNumQuantities) {
        ourMemset(apQuantities, 0, sizeof(float) * static_cast<int32_t>(aiNumQuantities));
    }
}

uint32_t BSFaceGenManager::GetEGTClampSize() {
    return BSFaceGenManager::uiEGTClampSize.Setting::Int();
}

void BSFaceGenManager::SetUseEmotions(bool abUseEmotions) {
    BSFaceGenManager::bEmotions = abUseEmotions;
}

bool BSFaceGenManager::GetFixNormals() {
    return BSFaceGenManager::bFixNormals.Bool();
}

BSFaceGenMorphData* BSFaceGenManager::GetFaceGenMorphData(NiTriBasedGeom* apGeom) {
    BSFaceGenModelExtraData* pFaceGenExtra = BSFaceGenManager::GetFaceGenModelExtraData(apGeom);
    if (!pFaceGenExtra || !pFaceGenExtra->GetModel()) {
        return nullptr;
    }
    return pFaceGenExtra->GetModel()->GetMorph();
}

// The source's NiDynamicCast: BSFaceGenMorphData's own static DynamicCast, then the downcast to the head data.
BSFaceGenMorphDataHead* BSFaceGenManager::GetFaceGenMorphDataHead(NiTriBasedGeom* apGeom) {
    BSFaceGenMorphData* pMorph = BSFaceGenManager::GetFaceGenMorphData(apGeom);
    if (!pMorph) {
        return nullptr;
    }
    return static_cast<BSFaceGenMorphDataHead*>(
        BSFaceGenMorphData::DynamicCast(&BSFaceGenMorphDataHead::ms_RTTI, pMorph)
    );
}

BSFaceGenMorphDataHair* BSFaceGenManager::GetFaceGenMorphDataHair(NiTriBasedGeom* apGeom) {
    BSFaceGenMorphData* pMorph = BSFaceGenManager::GetFaceGenMorphData(apGeom);
    if (pMorph) {
        return static_cast<BSFaceGenMorphDataHair*>(
            BSFaceGenMorphDataHair::DynamicCast(&BSFaceGenMorphDataHair::ms_RTTI, pMorph)
        );
    }

    return nullptr;
}

// Applies apGeom's hair morph data to its base vertices, scaled by afModifierValue. The value is tested against 0.0 as
// a double, so a NaN passes it.
void BSFaceGenManager::ApplyHairMorph(NiTriBasedGeom* apGeom, float afModifierValue) {
    if (!apGeom) {
        return;
    }
    if (afModifierValue < 0.0) {
        return;
    }
    BSFaceGenMorphDataHair* pMorphDataHair = BSFaceGenManager::GetFaceGenMorphDataHair(apGeom);
    if (!pMorphDataHair) {
        return;
    }
    if (!apGeom->GetModelData()->LockPackedVertexData(true)) {
        return;
    }
    NiGeometryDataIteratorNiPoint3 kIterator;
    apGeom->GetModelData()->GetVerticesIterator(kIterator);
    BSFaceGenManager::RefreshMeshFromBaseMorphExtraData(apGeom, kIterator);
    if (kIterator.GetCurrentPosition()) {
        pMorphDataHair->ApplyMorph(apGeom, nullptr, afModifierValue);
    }
    apGeom->GetModelData()->UnlockPackedVertexData();
}

void BSFaceGenManager::ReplaceFaceMeshLOD(BSFaceGenNiNode* apNode, int32_t aDetail) {
    BSHash kDirHash;
    BSHash kFileHash;
    for (int32_t i = 0; i < apNode->GetArrayCount(); ++i) {
        NiAVObject* pChild = apNode->GetAt(static_cast<uint32_t>(i));
        if (pChild) {
            const char* name = pChild->m_kName.GetHandleString();
            if (!BSstrcmp(name, "FaceGenFace")) {
                NiTriBasedGeom* pGeom = pChild->IsTriBasedGeom();
                BSFaceGenModel* pOldModelFG = BSFaceGenManager::GetFaceGenModel(pGeom);
                NiPointer<NiTriBasedGeom> spShape(static_cast<NiTriBasedGeom*>(nullptr));
                NiTriBasedGeom* pShapeHead = nullptr;
                BSStringT<char> filename;
                BSStringT<char> lodfname;
                BSStringT<char> fileEGM;
                BSStringT<char> fileTRI;
                BSStringT<char> fileEGT;
                BSStringT<char> fileFIM;
                FR2MatrixVTC<float> facecoord[2][2];
                FR2MatrixVTC<float>(*pCoord)[2][2] = BSFaceGenManager::GetFaceGenCoord();
                BSFaceGenManager::CopyFaceGenCoord(pCoord, &facecoord, false, false);
                BSFaceGenManager::GetAsNIFFile(filename, pOldModelFG->GetEGMFilename());
                if (aDetail) {
                    if (aDetail == 1) {
                        BSFaceGenManager::GetAsNIFFileHighLOD(lodfname, filename.pString);
                    }
                }
                else {
                    BSFaceGenManager::GetAsNIFFileLowLOD(lodfname, filename.pString);
                }
                BSFileEntry* pFileEntry = nullptr;
                if (lodfname.pString) {
                    BSHash::MakeDirAndFile(lodfname.pString, kDirHash, kFileHash);
                    pFileEntry = ArchiveManager::GetFileEntryForFileFromAllArchives(
                        ARCHIVE_TYPE_INDEX_MESHES,
                        kDirHash,
                        kFileHash,
                        lodfname.pString
                    );
                }
                if (!pFileEntry) {
                    return;
                }
                const char* pEGT = BSFaceGenManager::GetAsEGTFile(fileEGT, lodfname.pString);
                const char* pTRI = BSFaceGenManager::GetAsTRIFile(fileTRI, lodfname.pString, HAIR_PART_NONE);
                char* pLODName = lodfname.pString;
                const char* pEGM = BSFaceGenManager::GetAsEGMFile(fileEGM, lodfname.pString, HAIR_PART_NONE);
                NiPointer<BSFaceGenModel> spModelFG(
                    BSFaceGenManager::GetFaceGenModel(pEGM, pLODName, pTRI, pEGT, true, HAIR_PART_NONE, false)
                );
                if (!spModelFG->ApplyCoordinateToNewMesh(&facecoord, spShape) || !spShape.m_pObject) {
                    return;
                }
                spShape->SetName("FaceGenFace");
                pShapeHead = spShape.m_pObject;
                pShapeHead->SetTranslate(pChild->GetTranslate());
                pShapeHead->SetRotate(pChild->m_kLocal.m_Rotate);
                pShapeHead->SetScale(pChild->m_kLocal.m_fScale);
                BSShaderManager::PrepareObject(pShapeHead, 0, 0);
                // The shader property of the face mesh is a BSShaderPPLightingProperty; its tangent space data is read
                // without a check.
                BSShaderPPLightingProperty* pShaderProp = static_cast<BSShaderPPLightingProperty*>(
                    pShapeHead->GetProperty(3)
                );
                NiProperty* pChildShaderProp = pChild->GetProperty(3);
                NiCloningProcess kCloneProc(257u);
                BSShaderPPLightingProperty* pShaderPropLighting = static_cast<BSShaderPPLightingProperty*>(
                    pChildShaderProp->Clone(kCloneProc)
                );
                BSShaderPPLightingProperty::TangentSpaceData* pTangentSpaceData = pShaderProp->GetExtraDataPointers();
                pShaderPropLighting->StoreExtraDataPointers(pTangentSpaceData);
                pShapeHead->DetachProperty(pShaderProp);
                pShapeHead->AttachProperty(pShaderPropLighting);
                // The child named "FaceGenFace" is the face geometry: its shader is read from pChild itself, with no
                // cast or check.
                BSShader* pShader = static_cast<NiGeometry*>(pChild)->GetShader();
                pShapeHead->SetShader(pShader);
                uint16_t usVertices = pShapeHead->GetActiveVertexCount();
                if (pShapeHead->GetModelData()->LockPackedVertexData(true)) {
                    NiGeometryDataIteratorNiPoint3 spNewFaceGeo;
                    pShapeHead->GetModelData()->GetVerticesIterator(spNewFaceGeo);
                    if (spNewFaceGeo.m_pvCurrentPosition) {
                        BSFaceGenManager::ComputeModelBound(
                            pShapeHead->GetModelData()->m_kBound,
                            spNewFaceGeo,
                            usVertices
                        );
                    }
                }
                NiMaterialProperty* pMatProp = static_cast<NiMaterialProperty*>(pShapeHead->GetProperty(2));
                if (pMatProp && pMatProp->m_fShine < 1.0) {
                    pMatProp->SetShineness(4.0f);
                }

                apNode->SetAt(static_cast<uint32_t>(i), pShapeHead);
                pShapeHead->UpdateProperties();
                apNode->FixSkinInstances(apNode->m_pkParent, false);
                if (aDetail) {
                    if (aDetail == 1) {
                        apNode->bUsingLoResHead = false;
                    }
                }
                else {
                    apNode->bUsingLoResHead = true;
                }
            }
        }
    }
}

BS_FACEGEN_LOD BSFaceGenManager::GetForcedLod() {
    return static_cast<BS_FACEGEN_LOD>(iForcedLod);
}

void BSFaceGenManager::ClearBodyTexturesFromPalette(TESNPC* apNPC) {
    int8_t acFileName[264];
    int8_t acTextureName[268];
    TESRace* pRace = apNPC->GetRace();
    pRace->GetBodyModTextureName(apNPC, static_cast<char*>(static_cast<void*>(acTextureName)));
    pRace = apNPC->GetRace();
    pRace->GetBodyModTextureFileName(
        apNPC,
        static_cast<char*>(static_cast<void*>(acTextureName)),
        static_cast<char*>(static_cast<void*>(acFileName))
    );
    BSTexturePalette::RemoveTexture_0(static_cast<const char*>(static_cast<const void*>(acFileName)));
}

// The manager's default base texture, or null without a manager.
NiSourceTexture* BSFaceGenManager::GetBaseTexture() {
    return BSFaceGenManager::pInstance ? BSFaceGenManager::pInstance->spDefaultBaseMod.m_pObject
                                       : NiPointer<NiSourceTexture>(static_cast<NiSourceTexture*>(nullptr)).m_pObject;
}

void BSFaceGenManager::OffsetFaceGenCoord(
    const FR2MatrixVTC<float> (*apCopyCoordOld)[2][2],
    const FR2MatrixVTC<float> (*apCopyCoordNew)[2][2],
    FR2MatrixVTC<float> (*apCopyToCoord)[2][2],
    bool abNoTextureOffset
) {
    int32_t iOffsetBits;
    if (apCopyCoordOld && apCopyCoordNew && apCopyToCoord) {
        for (int32_t eType = 0; eType < 2; ++eType) {
            for (int32_t eSymm = 0; eSymm < 2; ++eSymm) {
                uint32_t uiRows = (*apCopyCoordOld)[eType][eSymm].nrows;
                uint32_t uiCols = (*apCopyCoordOld)[eType][eSymm].ncols;
                if (uiRows && uiCols) {
                    (*apCopyToCoord)[eType][eSymm].resize(static_cast<int32_t>(uiRows), static_cast<int32_t>(uiCols));
                    if (eType == 1 && abNoTextureOffset) {
                        (*apCopyToCoord)[eType][eSymm] = (*apCopyCoordOld)[eType][eSymm];
                    }
                    else {
                        uint32_t uiSize = uiCols * uiRows;
                        (*apCopyToCoord)[eType][eSymm].data.clear();
                        for (uint32_t i = 0; i < uiSize; ++i) {
                            // The const coordinates are read through the same non-const operator[] body.
                            float fOld = const_cast<FloatVector&>((*apCopyCoordOld)[eType][eSymm].data)[i];
                            float fNew = const_cast<FloatVector&>((*apCopyCoordNew)[eType][eSymm].data)[i];
                            *reinterpret_cast<float*>(&iOffsetBits) = fNew - fOld;
                            (*apCopyToCoord)[eType][eSymm].data.push_back(*reinterpret_cast<float*>(&iOffsetBits));
                        }
                    }
                }
                else {
                    (*apCopyToCoord)[eType][eSymm].resize(0, 0);
                }
            }
        }
    }
}

void BSFaceGenManager::ScaleFaceCoord(
    float afScaleAmount,
    FR2MatrixVTC<float> (*apCoordToScale)[2][2],
    bool abNoTextureScale
) {
    if (apCoordToScale && !(afScaleAmount < 0.0) && !(afScaleAmount > 1.0)) {
        for (int32_t i = 0; i < 2; ++i) {
            for (int32_t j = 0; j < 2; ++j) {
                if ((*apCoordToScale)[i][j].nrows &&
                    (*apCoordToScale)[i][j].ncols &&
                    (!i || (i == 1 && abNoTextureScale) || !abNoTextureScale)) {
                    (*apCoordToScale)[i][j] *= afScaleAmount;
                }
            }
        }
    }
}

const char* BSFaceGenManager::GetAsNIFFile(BSStringT<char>& aOutFilename, const char* apModel) {
    char acModel[264];
    char acFilename[264];
    if (!apModel) {
        return nullptr;
    }
    BSstrcpy(acModel, 260, apModel);
    char* pExtension = std::strrchr(acModel, '.');
    if (!pExtension) {
        return nullptr;
    }
    *pExtension = 0;
    BSsprintf(acFilename, 260u, "%s.nif", acModel);
    aOutFilename.Set(acFilename, 0u);
    return aOutFilename.pString;
}

const char* BSFaceGenManager::GetAsNIFFileLowLOD(BSStringT<char>& aOutFilename, const char* apModel) {
    BSHash kDirectoryHash;
    BSHash kFileNameHash;
    BSStringT<char> checkFilename;
    char* pExists = nullptr;
    if (apModel) {
        char cDest[264];
        BSstrcpy(cDest, 260, apModel);
        pExists = std::strchr(cDest, '_');
        if (pExists) {
            aOutFilename.Set(cDest, 0);
            return apModel;
        }

        char* pExtension = std::strrchr(cDest, '.');
        if (pExtension) {
            char cFormattedDest[264];
            *pExtension = 0;
            BSsprintf(cFormattedDest, 260, "%s_50.NIF", cDest);
            aOutFilename.Set(cFormattedDest, 0);
            checkFilename.SPrintF("Meshes\\%s", aOutFilename.pString);
            BSHash::MakeDirAndFile(checkFilename.pString, kDirectoryHash, kFileNameHash);
            if (ArchiveManager::GetFileEntryForFileFromAllArchives(
                    ARCHIVE_TYPE_INDEX_MESHES,
                    kDirectoryHash,
                    kFileNameHash,
                    checkFilename.pString
                )) {
                return aOutFilename.pString;
            }

            return nullptr;
        }

        return apModel;
    }

    return nullptr;
}

const char* BSFaceGenManager::GetAsNIFFileHighLOD(BSStringT<char>& aOutFilename, const char* apModel) {
    char acModel[264];
    char acFilename[264];
    if (!apModel) {
        return nullptr;
    }
    BSstrcpy(acModel, 260, apModel);
    char* pExtension = std::strrchr(acModel, '_');
    if (!pExtension) {
        return apModel;
    }
    *pExtension = 0;
    BSsprintf(acFilename, 260u, "%s.nif", acModel);
    aOutFilename.Set(acFilename, 0u);
    return aOutFilename.pString;
}

bool BSFaceGenManager::CreateFaceGenHead(
    BSFaceGenNiNode*& apOutNodeBiped,
    BSFaceGenNiNode*& apOutNodeSkinned,
    FaceGenNpcData& arNpcData,
    bool abForceNoSkinning
) {
    BSHash kDirectoryHash;
    BSHash kFileNameHash;
    NiMatrix3 matrot;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp",
        2632
    );
    BSFaceGenModel* pModelFG = nullptr;
    NiPointer<NiSourceTexture> sptexBase(static_cast<NiSourceTexture*>(nullptr));
    NiPointer<NiTriBasedGeom> spShape(static_cast<NiTriBasedGeom*>(nullptr));
    NiTriBasedGeom* pShapeHead = nullptr;
    NiTriBasedGeom* pShapeEars = nullptr;
    BSStringT<char> filename;
    BSStringT<char> fileEGM;
    BSStringT<char> fileTRI;
    BSStringT<char> fileEGT;
    BSStringT<char> fileFIM;
    BSStringT<char> fileLowLOD;
    matrot.MakeYRotation(-1.5707964f);
    for (uint32_t uiPart = 0; uiPart < 8; ++uiPart) {
        if (arNpcData.pModel.m_usSize &&
            arNpcData.pModel[uiPart] &&
            arNpcData.pModel[uiPart]->GetModelLength() &&
            arNpcData.pTexture.m_usSize &&
            arNpcData.pTexture[uiPart] &&
            arNpcData.pTexture[uiPart]->GetTextureNameLength()) {
            filename.SPrintF("Meshes\\%s", arNpcData.pModel[uiPart]->GetModel());
            if (arNpcData.eLod == BS_FACEGEN_LOD_LOW && !uiPart) {
                BSFaceGenManager::GetAsNIFFileLowLOD(fileLowLOD, filename.pString);
                BSFileEntry* pFileEntry = nullptr;
                if (fileLowLOD.pString) {
                    BSHash::MakeDirAndFile(fileLowLOD.pString, kDirectoryHash, kFileNameHash);
                    pFileEntry = ArchiveManager::GetFileEntryForFileFromAllArchives(
                        ARCHIVE_TYPE_INDEX_MESHES,
                        kDirectoryHash,
                        kFileNameHash,
                        fileLowLOD.pString
                    );
                }
                if (pFileEntry) {
                    BSFaceGenManager::GetAsNIFFileLowLOD(filename, filename.pString);
                }
            }
            const char* pEGTFileName = nullptr;
            switch (uiPart) {
                case 0u: {
                    if (arNpcData.bPlayer || bLoadFaceGenHeadEGTFiles.Bool()) {
                        pEGTFileName = BSFaceGenManager::GetAsEGTFile(fileEGT, filename.pString);
                    }
                    const char* pTRI = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
                    char* pNIFName = filename.pString;
                    const char* pEGM = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
                    pModelFG = BSFaceGenManager::GetFaceGenModel(
                        pEGM,
                        pNIFName,
                        pTRI,
                        pEGTFileName,
                        true,
                        HAIR_PART_NONE,
                        false
                    );
                    break;
                }
                case 1u: {
                    const char* pEGT = BSFaceGenManager::GetAsEGTFile(fileEGT, filename.pString);
                    const char* pTRI = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
                    char* pNIFName = filename.pString;
                    const char* pEGM = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
                    pModelFG = BSFaceGenManager::GetFaceGenModel(
                        pEGM,
                        pNIFName,
                        pTRI,
                        pEGT,
                        true,
                        HAIR_PART_NONE,
                        false
                    );
                    break;
                }
                case 2u:
                case 3u:
                case 4u:
                case 5u: {
                    const char* pTRI = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
                    char* pNIFName = filename.pString;
                    const char* pEGM = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
                    pModelFG = BSFaceGenManager::GetFaceGenModel(
                        pEGM,
                        pNIFName,
                        pTRI,
                        nullptr,
                        true,
                        HAIR_PART_NONE,
                        false
                    );
                    break;
                }
                default: {
                    break;
                }
            }
            if (pModelFG && pModelFG->ApplyCoordinateToNewMesh(&arNpcData.FaceCoord, spShape) && spShape.m_pObject) {
                int32_t iPPLightingShader;
                spShape->SetName(arNpcData.pHeadPartNodeName[uiPart]);
                if (!uiPart) {
                    pShapeHead = spShape.m_pObject;
                }
                if (uiPart == 1) {
                    pShapeEars = spShape.m_pObject;
                }
                filename.SPrintF("Textures\\%s", arNpcData.pTexture[uiPart]->GetTextureName());
                TES::pInstance->CreateTextureImage(filename.pString, sptexBase, false, false);
                if (sptexBase.m_pObject) {
                    int32_t iLightingShader;
                    NiShadeProperty* pProp = static_cast<NiShadeProperty*>(spShape->GetProperty(3));
                    if (pProp) {
                        iLightingShader = pProp->iShaderPropertyType >= 8 && pProp->iShaderPropertyType <= 12;
                    }
                    else {
                        iLightingShader = 0;
                    }
                    BSShaderPPLightingProperty* pShaderProp = nullptr;
                    if (iLightingShader) {
                        pShaderProp = static_cast<BSShaderPPLightingProperty*>(pProp);
                    }
                    if (pShaderProp) {
                        pShaderProp->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, sptexBase.m_pObject);
                    }
                }
                if (uiPart == 4 || uiPart == 3) {
                    if (spShape->GetProperty(0)) {
                        spShape->RemoveProperty(0);
                    }
                    NiAlphaProperty* pAlphaProp = BSFaceGenManager::GetHairTeethAlphaProperty();
                    spShape->AttachProperty(pAlphaProp);
                }
                if (uiPart <= 1) {
                    int32_t iType = NiMaterialProperty::GetType();
                    NiProperty* pMatProperty = spShape->GetProperty(iType);
                    NiMaterialProperty* pMatProp = static_cast<NiMaterialProperty*>(pMatProperty);
                    if (pMatProperty) {
                        pMatProp->SetName(*FixedStrings::QSkin());
                    }
                    else {
                        pMatProp = new NiMaterialProperty;
                        pMatProp->SetName(*FixedStrings::QSkin());
                        spShape->AttachProperty(pMatProp);
                    }
                }
                if (spShape->GetSkinInstance()) {
                    if (abForceNoSkinning) {
                        spShape->SetSkinInstance(nullptr);
                        spShape->SetTranslate(NiPoint3::ZERO);
                        spShape->SetRotate(matrot);
                        apOutNodeBiped->AttachChild(spShape.m_pObject, 1);
                        NiProperty* pShadeProp = spShape->GetProperty(3);
                        BSShaderProperty* pShProp = static_cast<BSShaderProperty*>(
                            NiObject::DynamicCastStatic(&BSShaderProperty::ms_RTTI, pShadeProp)
                        );
                        if (pShProp) {
                            pShProp->SetFlag(BSShaderProperty::BSSP_SKINNED, 0);
                        }
                    }
                    else {
                        apOutNodeSkinned->AttachChild(spShape.m_pObject, 1);
                    }
                }
                else {
                    spShape->SetRotate(matrot);
                    apOutNodeBiped->AttachChild(spShape.m_pObject, 1);
                }
                NiProperty* pShadeProp = spShape->GetProperty(3);
                BSShaderProperty* pShProp = static_cast<BSShaderProperty*>(pShadeProp);
                if (pShadeProp) {
                    iPPLightingShader = pShProp->iShaderPropertyType >= 8 && pShProp->iShaderPropertyType <= 12;
                }
                else {
                    iPPLightingShader = 0;
                }
                BSShaderPPLightingProperty* pPPProp = nullptr;
                if (iPPLightingShader) {
                    pPPProp = static_cast<BSShaderPPLightingProperty*>(pShProp);
                }
                if (pPPProp) {
                    pPPProp->UseDefaultNormalMap();
                }
                pModelFG = nullptr;
                sptexBase = static_cast<NiSourceTexture*>(nullptr);
                spShape = static_cast<NiTriBasedGeom*>(nullptr);
            }
        }
    }
    if (BSFaceGenManager::GetFixNormals() && pShapeHead && pShapeEars) {
        FixNormals(pShapeHead, pShapeEars, 0.1f, false, false);
    }

    if (arNpcData.pHair) {
        BSFaceGenManager::AttachHairToHead(apOutNodeBiped, apOutNodeSkinned, arNpcData, abForceNoSkinning);
    }
    if (arNpcData.pLeftEye && arNpcData.pRightEye) {
        BSFaceGenManager::AttachEyesToHead(apOutNodeBiped, apOutNodeSkinned, arNpcData, abForceNoSkinning);
    }

    for (BGSHeadPart*& rpItem : arNpcData.Accessories.ItemsUntilNull()) {
        TESModelTextureSwap* pModel = static_cast<TESModelTextureSwap*>(rpItem);
        filename.SPrintF("Meshes\\%s", pModel->GetModel());
        const char* pTRI = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
        char* pNIFName = filename.pString;
        const char* pEGM = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
        pModelFG = BSFaceGenManager::GetFaceGenModel(pEGM, pNIFName, pTRI, nullptr, true, HAIR_PART_NONE, true);
        if (pModelFG && pModelFG->ApplyCoordinateToNewMesh(&arNpcData.FaceCoord, spShape) && spShape.m_pObject) {
            spShape->SetRotate(matrot);
            apOutNodeBiped->AttachChild(spShape.m_pObject, 1);
            rpItem->SwapTextures(spShape.m_pObject);
            NiFixedString kName("FaceGenAccessory");
            spShape->SetName(kName);
        }
    }
    return true;
}

void BSFaceGenManager::AttachEyesToHead(
    BSFaceGenNiNode*& apOutNodeBiped,
    BSFaceGenNiNode*& apOutNodeSkinned,
    FaceGenNpcData& aNpcData,
    bool abForceNoSkinning
) {
    NiMatrix3 matrot;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp",
        2983
    );
    BSStringT<char> filename;
    BSStringT<char> fileEGM;
    BSStringT<char> fileTRI;
    BSStringT<char> fileEGT;
    BSStringT<char> fileFIM;
    NiPointer<NiSourceTexture> sptexBase(static_cast<NiSourceTexture*>(nullptr));
    NiPointer<NiTriBasedGeom> spShape(static_cast<NiTriBasedGeom*>(nullptr));
    BSFaceGenModel* pModelFG = nullptr;
    matrot.MakeYRotation(-1.5707964f);
    if (aNpcData.pLeftEye) {
        filename.SPrintF("Meshes\\%s", aNpcData.pLeftEye->GetModel());
        const char* pTRI = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
        char* pNIFName = filename.pString;
        const char* pEGM = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
        pModelFG = BSFaceGenManager::GetFaceGenModel(pEGM, pNIFName, pTRI, nullptr, true, HAIR_PART_NONE, false);
        if (pModelFG) {
            pModelFG->ApplyCoordinateToNewMesh(&aNpcData.FaceCoord, spShape);
        }
        if (spShape.m_pObject) {
            spShape->SetName("FaceGenEyeLeft");
            if (aNpcData.pEyes) {
                filename.SPrintF("Textures\\%s", aNpcData.pEyes->GetTextureName());
                TES::pInstance->CreateTextureImage(filename.pString, sptexBase, false, false);
            }
            else {
                TES::pInstance
                    ->CreateTextureImage("Textures\\Characters\\Eyes\\EyeDefault.dds", sptexBase, false, false);
            }
            if (sptexBase.m_pObject) {
                int32_t iLightingShader;
                NiShadeProperty* pProp = static_cast<NiShadeProperty*>(spShape->GetProperty(3));
                if (pProp) {
                    iLightingShader = pProp->iShaderPropertyType >= 8 && pProp->iShaderPropertyType <= 12;
                }
                else {
                    iLightingShader = 0;
                }
                BSShaderPPLightingProperty* pShaderProp = nullptr;
                if (iLightingShader) {
                    pShaderProp = static_cast<BSShaderPPLightingProperty*>(pProp);
                }
                if (pShaderProp) {
                    pShaderProp->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, sptexBase.m_pObject);
                }
            }
            if (spShape->GetSkinInstance()) {
                if (abForceNoSkinning) {
                    spShape->SetSkinInstance(nullptr);
                    spShape->SetTranslate(NiPoint3::ZERO);
                    spShape->SetRotate(matrot);
                    apOutNodeBiped->AttachChild(spShape.m_pObject, 1);
                }
                else {
                    apOutNodeSkinned->AttachChild(spShape.m_pObject, 1);
                }
            }
            else {
                spShape->SetRotate(matrot);
                apOutNodeBiped->AttachChild(spShape.m_pObject, 1);
            }
        }
        pModelFG = nullptr;
        spShape = static_cast<NiTriBasedGeom*>(nullptr);
        sptexBase = static_cast<NiSourceTexture*>(nullptr);
    }
    if (aNpcData.pRightEye) {
        filename.SPrintF("Meshes\\%s", aNpcData.pRightEye->GetModel());
        const char* pTRI = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
        char* pNIFName = filename.pString;
        const char* pEGM = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
        pModelFG = BSFaceGenManager::GetFaceGenModel(pEGM, pNIFName, pTRI, nullptr, true, HAIR_PART_NONE, false);
        if (pModelFG) {
            pModelFG->ApplyCoordinateToNewMesh(&aNpcData.FaceCoord, spShape);
        }
        if (spShape.m_pObject) {
            spShape->SetName("FaceGenEyeRight");
            if (aNpcData.pEyes) {
                filename.SPrintF("Textures\\%s", aNpcData.pEyes->GetTextureName());
                TES::pInstance->CreateTextureImage(filename.pString, sptexBase, false, false);
            }
            else {
                TES::pInstance
                    ->CreateTextureImage("Textures\\Characters\\Eyes\\EyeDefault.dds", sptexBase, false, false);
            }
            if (sptexBase.m_pObject) {
                int32_t iLightingShader;
                NiShadeProperty* pProp = static_cast<NiShadeProperty*>(spShape->GetProperty(3));
                if (pProp) {
                    iLightingShader = pProp->iShaderPropertyType >= 8 && pProp->iShaderPropertyType <= 12;
                }
                else {
                    iLightingShader = 0;
                }
                BSShaderPPLightingProperty* pShaderProp = nullptr;
                if (iLightingShader) {
                    pShaderProp = static_cast<BSShaderPPLightingProperty*>(pProp);
                }
                if (pShaderProp) {
                    pShaderProp->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, sptexBase.m_pObject);
                }
            }
            if (spShape->GetSkinInstance()) {
                if (abForceNoSkinning) {
                    spShape->SetSkinInstance(nullptr);
                    spShape->SetTranslate(NiPoint3::ZERO);
                    spShape->SetRotate(matrot);
                    apOutNodeBiped->AttachChild(spShape.m_pObject, 1);
                }
                else {
                    apOutNodeSkinned->AttachChild(spShape.m_pObject, 1);
                }
            }
            else {
                spShape->SetRotate(matrot);
                apOutNodeBiped->AttachChild(spShape.m_pObject, 1);
            }
        }
        pModelFG = nullptr;
        spShape = static_cast<NiTriBasedGeom*>(nullptr);
        sptexBase = static_cast<NiSourceTexture*>(nullptr);
    }
}

void BSFaceGenManager::ComputeModelBound(
    NiBound& arBound,
    NiGeometryDataIteratorNiPoint3& arIterator,
    int32_t aiVertices
) {
    NiPoint3 kCenter;
    if (aiVertices > 0) {
        NiPoint3 kMin;
        NiPoint3 kCurrent;
        float fRadiusSqr = 0.0f;
        arIterator.Get(0, kMin);
        NiPoint3 kMax = kMin;
        NiGeometryDataIteratorNiPoint3 kBoxIterator = arIterator;
        int32_t i = 1;
        while (i < aiVertices) {
            kBoxIterator.Get_(kCurrent);
            if (kCurrent.x < static_cast<double>(kMin.x)) {
                kMin.x = kCurrent.x;
            }
            if (kCurrent.y < static_cast<double>(kMin.y)) {
                kMin.y = kCurrent.y;
            }
            if (kCurrent.z < static_cast<double>(kMin.z)) {
                kMin.z = kCurrent.z;
            }
            if (kCurrent.x > static_cast<double>(kMax.x)) {
                kMax.x = kCurrent.x;
            }
            if (kCurrent.y > static_cast<double>(kMax.y)) {
                kMax.y = kCurrent.y;
            }
            if (kCurrent.z > static_cast<double>(kMax.z)) {
                kMax.z = kCurrent.z;
            }
            ++i;
            kBoxIterator.Next();
        }
        kCenter = 0.5f * (kMin + kMax);
        NiGeometryDataIteratorNiPoint3 kRadiusIterator = arIterator;
        i = 0;
        while (i < aiVertices) {
            kRadiusIterator.Get_(kCurrent);
            NiPoint3 kDiff = kCurrent - kCenter;
            float fLengthSqr = kDiff.Dot(kDiff);
            if (fRadiusSqr < static_cast<double>(fLengthSqr)) {
                fRadiusSqr = fLengthSqr;
            }
            ++i;
            kRadiusIterator.Next();
        }
        arBound.SetCenter(kCenter);
        float fRadius = NiSqrt(fRadiusSqr);
        arBound.SetRadius(fRadius);
    }
    else {
        arBound.SetCenter(NiPoint3::ZERO);
        arBound.SetRadius(0.0f);
    }
}

char* BSFaceGenManager::GetCustomMorphText(int32_t aiIndex) {
    return (&BSFaceGenManager::pFaceGenCustomMorphText)[aiIndex];
}

float BSFaceGenManager::GetFaceCoordValue(
    const FR2MatrixVTC<float> (*apCoord)[2][2],
    FanTypeE aeFanType,
    FanSymmE aeFanSymm,
    uint32_t aiCtrlID
) {
    FR2MatrixVTC<float> kProduct;
    float fCurrPos = 0.0f;
    if (apCoord) {
        if (aiCtrlID < BSFaceGenManager::GetInstance()->LinCtrls.grp[aeFanType][aeFanSymm].size()) {
            FanLinCtlS* pCtl = &BSFaceGenManager::GetInstance()->LinCtrls.grp[aeFanType][aeFanSymm][aiCtrlID];
            FR2MatrixVTC<float>* pProduct = pCtl->ctl.Mul(&kProduct, (*apCoord)[aeFanType][aeFanSymm]);
            FR2MatrixVTC<float>::RowT kRow = (*pProduct)[0u];
            fCurrPos = kRow[0u];
        }
    }
    return fCurrPos;
}

void BSFaceGenManager::SetFaceCoordValue(
    FR2MatrixVTC<float> (*apCoord)[2][2],
    FanTypeE aeFanType,
    FanSymmE aeFanSymm,
    uint32_t aiCtrlID,
    float afValue
) {
    FR2MatrixVTC<float> kScaled;
    FR2MatrixVTC<float> kTransposed;
    if (apCoord) {
        if (aiCtrlID < BSFaceGenManager::GetInstance()->LinCtrls.grp[aeFanType][aeFanSymm].size()) {
            float fCurrPos = BSFaceGenManager::GetFaceCoordValue(apCoord, aeFanType, aeFanSymm, aiCtrlID);
            float fDelta = afValue - fCurrPos;
            FanLinCtlS* pCtl = &BSFaceGenManager::GetInstance()->LinCtrls.grp[aeFanType][aeFanSymm][aiCtrlID];
            FR2MatrixVTC<float>* pTransposed = pCtl->ctl.transpose(&kTransposed);
            FR2MatrixVTC<float>* pScaled = pTransposed->MulScalar(&kScaled, fDelta);
            (*apCoord)[aeFanType][aeFanSymm] += *pScaled;
        }
    }
}

int32_t BSFaceGenManager::GetMaxControlID(int32_t aiType, int32_t aiSymm) {
    return BSFaceGenManager::GetInstance()->LinCtrls.grp[aiType][aiSymm].size() - 1;
}

void BSFaceGenManager::RandomizeFaceGenetic(
    float afGeneticVariation,
    FR2MatrixVTC<float> (*apSourceCoord)[2][2],
    FR2MatrixVTC<float> (*apOutCoord)[2][2]
) {
    if (apOutCoord && apSourceCoord) {
        BSFaceGenManager::GetInstance()->GenCtrls.geneticRandom(afGeneticVariation, apSourceCoord, apOutCoord);
    }
}

NiPoint3* BSFaceGenBaseMorphExtraData::GetVertices() const {
    return pVerts;
}

uint32_t BSFaceGenBaseMorphExtraData::GetVertexCount(bool abMeshOnly) const {
    if (abMeshOnly) {
        return iVertsModelCount;
    }

    return iVertsCount;
}

const char* BSFaceGenModel::GetEGMFilename() {
    if (pMeshData) {
        return pMeshData->EGMFilename.String();
    }

    return nullptr;
}

const char* BSFaceGenModel::GetEGTFilename() {
    if (pTextureData) {
        return pTextureData->EGTFilename.String();
    }

    return nullptr;
}

// Without mesh data the morph is read through a null temporary smart pointer.
BSFaceGenMorphData* BSFaceGenModel::GetMorph() {
    if (pMeshData) {
        return pMeshData->spMorph;
    }
    NiPointer<BSFaceGenMorphData> spNull(nullptr);
    BSFaceGenMorphData* pMorph = spNull;
    return pMorph;
}
