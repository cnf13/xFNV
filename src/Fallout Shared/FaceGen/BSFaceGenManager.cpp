#include "Fallout Shared/FaceGen/BSFaceGenManager.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
#include "Fallout Shared/FaceGen/FR2MatrixCTC.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout Shared/FaceGen/FanLinCtlGrpS.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSemaphore.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/FanCtlFile.h"
#include "SDK/FaceGen/FutDensity.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/string.h"

#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

BSCriticalSection BSFaceGenManager::FGAccessCrit;
SettingT<INISettingCollection> BSFaceGenManager::bUseFaceGenHeads("bUseFaceGenHeads:General", true);
SettingT<INISettingCollection> TESRace::bFixFaceNormals("bFixFaceNormals:General", false);
SettingT<INISettingCollection> BSFaceGenManager::bUseFaceGenLOD("bUseFaceGenLOD:LOD", false);
SettingT<INISettingCollection> BSFaceGenManager::fLodDistance("fLodDistance:LOD", 500.0f);
SettingT<INISettingCollection> BSFaceGenManager::fTalkingDistance("fTalkingDistance:LOD", 2000.0f);
SettingT<INISettingCollection>
    BSFaceGenManager::bUseMultiThreadedFaceGen("bUseMultiThreadedFaceGen:BackgroundLoad", true);
SettingT<INISettingCollection> BSFaceGenManager::bUseEyeEnvMapping("bUseEyeEnvMapping:General", true);
SettingT<INISettingCollection> gs_uiFaceGenMaxEGMDataSize_General("uiFaceGenMaxEGMDataSize:General", 67108864u);
SettingT<INISettingCollection> gs_uiFaceGenMaxEGTDataSize_General("uiFaceGenMaxEGTDataSize:General", 67108864u);
SettingT<INISettingCollection> BSFaceGenManager::uiEGTClampSize("uiEGTClampSize:General", 30u);
static SettingT<INISettingCollection> bDebugFaceGenMultithreading("bDebugFaceGenMultithreading:Debug", false);
static SettingT<INISettingCollection> bDebugFaceGenCriticalSection("bDebugFaceGenCriticalSection:Debug", false);
SettingT<INISettingCollection> BSFaceGenManager::bLoadBackgroundFaceGen("bLoadBackgroundFaceGen:BackgroundLoad", false);
SettingT<INISettingCollection> BSFaceGenManager::bLoadFaceGenHeadEGTFiles("bLoadFaceGenHeadEGTFiles:General", false);
NiSemaphore BSFaceGenManager::LoadingSema(1, 1);
BSCriticalSection BSFaceGenManager::QueueCritical;

BSFaceGenModelMap::Entry::Entry() {}

BSFaceGenModelMap::Entry::~Entry() {}

BSFaceGenModelMap::BSFaceGenModelMap() : Map(37u, true), bHasFaceGenData(false) {
    for (uint32_t cnt = 0; cnt < 2; ++cnt) {
        uiMaxDataSize[cnt] = 0x10000000;
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
                if (aspNewEntry->spModel.m_pObject->GetEGTDataSize()) {
                    continue;
                }
                size = 10;
            }
            else {
                if (aspNewEntry->spModel.m_pObject->GetEGMDataSize()) {
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
    bool bres = false;
    NiPointer<BSFaceGenModelMap::Entry> spentry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    BSFaceGenManager::LockFaceGenAccess();
    if (!Map.GetAt(apKey, spentry)) {
        char Dst[268];
        BSstrcpy(Dst, 260, apKey);
        char* pext = std::strrchr(Dst, 46);
        if (pext) {
            *pext = 0;
            BSstrcat(Dst, 260, ".egm");
            Map.GetAt(Dst, spentry);
        }
    }
    if (spentry.m_pObject) {
        spentry.m_pObject->spModel.m_pObject->KillTextureData();
        bres = true;
    }
    BSFaceGenManager::UnlockFaceGenAccess();
    return bres;
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
    NiTMapItem<const char*, NiPointer<BSFaceGenModelMap::Entry>>* iterator;
    BSFaceGenManager::LockFaceGenAccess();
    iterator = Map.GetFirstPos();
    while (iterator) {
        const char* pkey = nullptr;
        NiPointer<BSFaceGenModelMap::Entry> spentry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
        Map.GetNext(iterator, pkey, spentry);
        if (spentry.m_pObject) {
            BSFaceGenModel* pm = spentry.m_pObject->spModel.m_pObject;
            if (pm) {
                pm->UnloadEGTData();
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
    const char* v2;
    NiPointer<BSFaceGenModelMap::Entry> spentry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    if (apModel && (apModel->GetEGTFilename() && (v2 = apModel->GetEGTFilename(), BSstrlen(v2)))) {
        const char* EGMFilename;
        bool bres = false;
        BSFaceGenManager::LockFaceGenAccess();
        const char* EGTFilename = apModel->GetEGTFilename();
        if (Map.GetAt(EGTFilename, spentry) ||
            (apModel->GetEGMFilename() && (EGMFilename = apModel->GetEGMFilename(), Map.GetAt(EGMFilename, spentry)))) {
            uint32_t v6 = BSGetTickCount();
            spentry.m_pObject->uiTime = v6;
            VerifyData(spentry.m_pObject);
            spentry.m_pObject->spModel.m_pObject->LoadEGTData();
            bHasFaceGenData = true;
            bres = true;
        }
        BSFaceGenManager::UnlockFaceGenAccess();
        return bres;
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
            else if ((spEntry.operator->()) && (spEntry->spModel.operator->())) {
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
                "FACEGEN:\t%3d\t%s\t%iKB\t%iKB\r\n", spEntry->spModel->m_uiRefCount - 1, pFilename, uiEGMSize, uiEGTSize
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
        void* pMemory = NiMemObject::operator new(sizeof(BSFaceGenModelMap::Entry));
        BSFaceGenModelMap::Entry* pEntry;
        if (pMemory) {
            pEntry = ::new (pMemory) BSFaceGenModelMap::Entry;
        }
        else {
            pEntry = nullptr;
        }
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
    const char* pkey;
    NiTMapItem<const char*, NiPointer<BSFaceGenModelMap::Entry>>* iter;
    pkey = nullptr;
    const char* pLRUkey = nullptr;
    NiPointer<BSFaceGenModelMap::Entry> spentry(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    NiPointer<BSFaceGenModelMap::Entry> spLRU(static_cast<BSFaceGenModelMap::Entry*>(nullptr));
    uint32_t curTime = BSGetTickCount();
    BSFaceGenManager::LockFaceGenAccess();
    iter = Map.GetFirstPos();
    while (iter) {
        Map.GetNext(iter, pkey, spentry);
        if (!spentry.operator==(aspNewEntry)) {
            uint32_t dataSize;
            if (aDataType) {
                if (aDataType == BSFaceGenModelMap::DATA_TYPE_EGT) {
                    dataSize = spentry.m_pObject->spModel.m_pObject->GetEGTDataSize();
                }
                else {
                    dataSize = 0;
                }
            }
            else {
                dataSize = spentry.m_pObject->spModel.m_pObject->GetEGMDataSize();
            }
            if (dataSize) {
                if (spLRU.m_pObject) {
                    BSFaceGenModelMap::Entry* v3 = spentry.m_pObject;
                    uint32_t v4 = curTime - v3->uiTime;
                    BSFaceGenModelMap::Entry* v5 = spLRU.m_pObject;
                    if (v4 > curTime - v5->uiTime) {
                        spLRU.operator=(spentry);
                        pLRUkey = pkey;
                    }
                }
                else {
                    spLRU.operator=(spentry);
                    pLRUkey = pkey;
                }
            }
        }
    }
    if (spLRU.m_pObject) {
        if (aDataType) {
            if (aDataType == BSFaceGenModelMap::DATA_TYPE_EGT) {
                spLRU.m_pObject->spModel.m_pObject->UnloadEGTData();
            }
        }
        else {
            spLRU.m_pObject->spModel.m_pObject->UnloadEGMData();
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

void FanLinCtlS::operator delete(void* apMemory) {
    ourFree(apMemory);
}

FanFgResidualS::~FanFgResidualS() {}

void FanFgResidualS::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void DestroyRange(FanFgResidualS* _First, FanFgResidualS* _Last, std::allocator<FanFgResidualS>& _Al, uint8_t _Cat) {
    while (_First != _Last) {
        Destroy(_First++);
    }
}

float fsqr(float a1) {
    return a1 * a1;
}

void DestroyRange(
    FanLinCtlS* _First,
    FanLinCtlS* _Last,
    [[maybe_unused]] std::allocator<FanLinCtlS>& _Al,
    [[maybe_unused]] uint8_t _Cat
) {
    while (_First != _Last) {
        Destroy(_First);
        ++_First;
    }
}

FanLinCtlS* Destroy(FanLinCtlS* a1) {
    return DestroyFanLinCtl(a1);
}

void* Destroy(void* a1) {
    return ::ExceptionPtrDestroy_0(static_cast<FanFgResidualS*>(a1));
}

FanLinCtlS* DestroyFanLinCtl(FanLinCtlS* apThis) {
    apThis->~FanLinCtlS();
    return apThis;
}

FanFgResidualS* ExceptionPtrDestroy_0(FanFgResidualS* apThis) {
    apThis->~FanFgResidualS();
    return apThis;
}

FR2MatrixVTC<float>* ConcatRows(FR2MatrixVTC<float>* a1, FR2MatrixVTC<float>* hProcess, FR2MatrixVTC<float>* arg8) {
    FR2MatrixVTC<float>::RowT result;
    FR2MatrixVTC<float> apMemory;
    bool v5 = !hProcess->nrows || !hProcess->ncols;
    bool v4 = !arg8->nrows || !arg8->ncols;
    if (v5 && v4) {
        a1->data.~vector<float, std::allocator<float>>();
        ::new (&a1->data) std::vector<float, std::allocator<float>>;
        a1->nrows = 0;
        a1->ncols = 0;
        a1->data.resize(a1->nrows * a1->ncols, 0.0f);
        return a1;
    }

    if (v5) {
        ::new (a1) FR2MatrixVTC<float>(*arg8);
        return a1;
    }

    if (v4) {
        ::new (a1) FR2MatrixVTC<float>(*hProcess);
        return a1;
    }

    if (hProcess->ncols != arg8->ncols) {
        fr2Assert("d:\\_fallout3\\platforms\\common\\code\\sdk\\facegen\\matrixVT.hpp", 1278);
    }
    uint32_t ncols = hProcess->ncols;
    uint32_t nrows = hProcess->nrows;
    uint32_t v14 = arg8->nrows;
    apMemory.data.~vector<float, std::allocator<float>>();
    ::new (&apMemory.data) std::vector<float, std::allocator<float>>;
    apMemory.nrows = v14 + nrows;
    apMemory.ncols = ncols;
    apMemory.data.resize((v14 + nrows) * ncols, 0.0f);
    uint32_t v13 = hProcess->nrows;
    uint32_t v12 = hProcess->ncols;
    float* v21 = &hProcess->data[0];
    uint32_t v11 = apMemory.ncols;
    float* v19 = &apMemory.data[0];
    rt::memcpy(v19, v21, sizeof(float) * (v12 * v13));
    uint32_t v10 = arg8->nrows;
    uint32_t v9 = arg8->ncols;
    FR2MatrixVTC<float>::RowT* v8 = &(result = arg8->operator[](0));
    v8->p;
    float* p = v8->p;
    rt::memcpy(apMemory.operator[](hProcess->nrows).p, p, sizeof(float) * (v9 * v10));
    ::new (a1) FR2MatrixVTC<float>(apMemory);
    return a1;
}

// The end of the copy is computed first, from the byte span divided as signed, as the game does: a plain pointer
// difference would let clang assume an exact multiple.
FanLinCtlS* CopyOpt(
    FanLinCtlS* _First,
    FanLinCtlS* _Last,
    FanLinCtlS* _Dest,
    [[maybe_unused]] uint8_t _IterCat,
    [[maybe_unused]] uint8_t _PtrCat,
    [[maybe_unused]] uint8_t _Checked
) {
    ptrdiff_t iBytes = static_cast<char*>(static_cast<void*>(_Last)) - static_cast<char*>(static_cast<void*>(_First));
    FanLinCtlS* result = _Dest + iBytes / static_cast<ptrdiff_t>(sizeof(FanLinCtlS));
    for (; _First != _Last; ++_Dest, ++_First) {
        _Dest->ctl.operator=(_First->ctl);
        _Dest->label.assign(_First->label, 0, std::string::npos);
    }
    return result;
}

FanRaceDistS::FanRaceDistS() {}

FanRaceDistS::~FanRaceDistS() {}

FanOffLinCtlS::FanOffLinCtlS() {}

FanOffLinCtlS::~FanOffLinCtlS() {}

FanPrtOffLinCtlS::FanPrtOffLinCtlS() {}

FanPrtOffLinCtlS::~FanPrtOffLinCtlS() {}

FanGenCtlsGrpC::FanGenCtlsGrpC() : initialized(false) {}

FanGenCtlsGrpC::~FanGenCtlsGrpC() {}

float FanGenCtlsGrpC::getAttribute(
    FanRaceE race,
    FanAttribE att,
    FanTypeE type,
    const FR2MatrixVTC<float> (*coord)[2]
) {
    FR2MatrixVTC<float> apResult;
    if (race >= FANRACE_SIZE) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 198);
    }
    if (att >= FANATT_SIZE) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 199);
    }
    if (type >= FANTYPE_SIZE) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 200);
    }
    if (initialized) {
        float v11 = 0.0f;
        FR2MatrixVTC<float>* v7 = attr[race][att][type].ctl.Mul(&apResult, (*coord)[2 * type]);
        v11 = *v7->operator[](0).p;
        return v11 + attr[race][att][type].off;
    }

    return 0.0f;
}

void FanGenCtlsGrpC::setAttributes(FanRaceE race, FanTypeE type, const float* val, FR2MatrixVTC<float> (*coord)[2]) {
    FR2MatrixVTC<float> v9;
    FR2MatrixVTC<float> apResult;
    if (race >= FANRACE_SIZE) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 266);
    }
    if (initialized) {
        float attBoundHi[2];
        FR2Vect2TC<float> delta;
        float attBoundLo[2];
        FR2Vect2TC<float> valDiff;
        uint32_t aa;
        attBoundLo[0] = 15.0f;
        attBoundLo[1] = -4.0f;
        attBoundHi[0] = 65.0f;
        attBoundHi[1] = 4.0f;
        valDiff.x1 = 0.0f;
        valDiff.x2 = 0.0f;
        for (aa = 0; aa < 2; ++aa) {
            float newVal = val[aa];
            if (attBoundLo[aa] > static_cast<double>(newVal)) {
                newVal = attBoundLo[aa];
            }
            if (attBoundHi[aa] < static_cast<double>(newVal)) {
                newVal = attBoundHi[aa];
            }
            // The source counts the attributes with an unsigned index and passes it as the attribute.
            double Attribute = getAttribute(race, static_cast<FanAttribE>(aa), type, coord);
            *(&valDiff.x1 + aa) = newVal - Attribute;
        }
        // The 2 x 2 inverse goes to the matrix-vector product as its four floats, row by row.
        MultiplyMatrixVector(&delta.x1, &invAttCov[race][type].data[0][0], &valDiff.x1);
        for (aa = 0; aa < 2; ++aa) {
            FR2MatrixVTC<float>* v7 = attr[race][aa][type].ctl.transpose(&apResult);
            FR2MatrixVTC<float>* m = v7->MulScalar(&v9, *(&delta.x1 + aa));
            (*coord)[2 * type].operator+=(*m);
        }
    }
}

double
FanGenCtlsGrpC::getCaricature(FanRaceE race, FanTypeE type, FanSymmE symm, const FR2MatrixVTC<float> (*coord)[2]) {
    FR2MatrixVTC<float> v19;
    FR2MatrixVTC<float> v20;
    FR2MatrixVTC<float>::RowT a2a;
    FR2MatrixVTC<float> apResult;
    FR2MatrixVTC<float> apThis;
    FR2MatrixVTC<float> v26;
    if (race >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 308);
    }
    if (type >= 2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 309);
    }
    if (symm >= 2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 310);
    }
    if (!initialized) {
        return 0.0;
    }
    if (symm == 1) {
        float v17 = coord[type][symm].lengthSqr();
        float v14 = std::sqrt(static_cast<double>(v17));
        float v13 = std::sqrt(static_cast<double>(coord[type][symm].nrows));
        return (v14 / v13);
    }

    if (symm) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 344);
        return 0.0;
    }

    coord[type][symm].Sub(&v26, distrib[race].mean[type]);
    for (uint32_t i = 0; i < 2; ++i) {
        FR2MatrixVTC<float>* v11 = basis[4 * race + 2 * type + i].Mul(&apResult, v26);
        float v23 = *(::new (&a2a) FR2MatrixVTC<float>::RowT(v11->operator[](0u)))->p;
        FR2MatrixVTC<float>* v10 = basis[4 * race + 2 * type + i].transpose(&v20);
        FR2MatrixVTC<float>* v9 = v10->MulScalar(&v19, v23);
        v26.operator-=(*v9);
    }
    distrib[race].invEigBasis[type].Mul(&apThis, v26);
    float v16 = apThis.lengthSqr();
    float v8 = std::sqrt(static_cast<double>(v16));
    float v7 = static_cast<double>(apThis.nrows) - 2.0;
    float v6 = std::sqrt(static_cast<double>(v7));
    float v18 = v8 / v6;
    return v18;
}

void FanGenCtlsGrpC::setCaricature(
    FanRaceE race,
    FanTypeE type,
    FanSymmE symm,
    float val,
    FR2MatrixVTC<float> (*coord)[2]
) {
    FR2MatrixVTC<float> v19;
    FR2MatrixVTC<float> v20;
    FR2MatrixVTC<float> v21;
    FR2MatrixVTC<float> v22;
    FR2MatrixVTC<float>::RowT result;
    FR2MatrixVTC<float> apResult;
    FR2MatrixVTC<float> m;
    FR2MatrixVTC<float> apMemory;
    if (race >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 370);
    }
    if (type >= 2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 371);
    }
    if (symm >= 2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 372);
    }
    if (initialized) {
        if (val < 0.009999999776482582) {
            val = 0.01f;
        }
        if (symm == 1) {
            float v18 = coord[type][symm].lengthSqr();
            float v14 = std::sqrt(static_cast<double>(v18));
            float v13 = std::sqrt(static_cast<double>(coord[type][symm].nrows));
            float v30 = v14 / v13;
            if (v30 > 0.0) {
                float v12 = val / v30;
                coord[type][symm].operator*=(v12);
            }
        }
        else {
            if (!symm) {
                coord[type][symm].Sub(&m, distrib[race].mean[type]);
                uint32_t v17 = coord[type][symm].ncols;
                uint32_t v16 = coord[type][symm].nrows;
                apMemory.data.~vector<float, std::allocator<float>>();
                ::new (&apMemory.data) std::vector<float, std::allocator<float>>;
                apMemory.nrows = v16;
                apMemory.ncols = v17;
                apMemory.data.resize(v16 * v17, 0.0f);
                apMemory.setZero();
                for (uint32_t i = 0; i < 2; ++i) {
                    FR2MatrixVTC<float>* v11 = basis[4 * race + 2 * type + i].Mul(&apResult, m);
                    float v25 = *(::new (&result) FR2MatrixVTC<float>::RowT(v11->operator[](0u)))->p;
                    FR2MatrixVTC<float>* v10 = basis[4 * race + 2 * type + i].transpose(&v22);
                    FR2MatrixVTC<float>* v9 = v10->MulScalar(&v21, v25);
                    apMemory.operator+=(*v9);
                }
                m.operator-=(apMemory);
                float v27 = getCaricature(race, type, symm, coord);
                if (!(v27 > 0.0)) {}
                else {
                    float v8 = val / v27;
                    m.operator*=(v8);
                    FR2MatrixVTC<float>* v7 = m.Add(&v20, apMemory);
                    FR2MatrixVTC<float>* hProcess = v7->Add(&v19, distrib[race].mean[type]);
                    coord[type][symm].operator=(*hProcess);
                }
            }
        }
    }
}

double FanGenCtlsGrpC::getRace(int32_t race1, int32_t race2, const FR2MatrixVTC<float> (*coord)[2]) {
    FR2MatrixVTC<float>::RowT a2a;
    FR2MatrixVTC<float> apResult;
    if (race1 >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 436);
    }
    if (race2 >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 437);
    }
    if (race1 == race2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 438);
    }
    if (!initialized) {
        return 0.0;
    }
    float v11 = 0.0f;
    for (uint32_t i = 0; i < 2; ++i) {
        FR2MatrixVTC<float>* v6 = race[race1][race2].ctl[i].Mul(&apResult, coord[i][0]);
        float** v5 = &(::new (&a2a) FR2MatrixVTC<float>::RowT(v6->operator[](0u)))->p;
        v11 = **v5 + v11;
    }
    return (v11 + race[race1][race2].off);
}

void FanGenCtlsGrpC::setRace(int32_t race1, int32_t race2, float val, FR2MatrixVTC<float> (*coord)[2]) {
    FR2MatrixVTC<float> v8;
    FR2MatrixVTC<float> apResult;
    if (race1 >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 468);
    }
    if (race2 >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 469);
    }
    if (race1 == race2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 470);
    }
    if (initialized) {
        float s = (val - getRace(race1, race2, coord)) * race[race1][race2].scale;
        for (uint32_t i = 0; i < 2; ++i) {
            FR2MatrixVTC<float>* v6 = race[race1][race2].ctl[i].transpose(&apResult);
            FR2MatrixVTC<float>* m = v6->MulScalar(&v8, s);
            coord[i][0].operator+=(*m);
        }
    }
}

void FanGenCtlsGrpC::random(int32_t a1, FR2MatrixVTC<float>* a2) {
    FR2MatrixVTC<float> v11;
    FR2MatrixVTC<float> v12;
    FR2MatrixVTC<float> v13;
    FR2MatrixVTC<float> v14;
    FR2MatrixVTC<float> apResult;
    FR2MatrixVTC<float>::RowT a2a;
    FR2MatrixVTC<float> v20;
    if (a1 >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 498);
    }
    if (initialized) {
        for (uint32_t j = 0; j < 2; ++j) {
            for (uint32_t k = 0; k < 2; ++k) {
                for (uint32_t i = 0; i < a2[2 * j + k].nrows; ++i) {
                    FR2MatrixVTC<float>::RowT* v10 = &(a2a = a2[2 * j + k].operator[](i));
                    v10->p;
                    float* p = v10->p;
                    *p = RngStdNormal();
                }
            }
        }
        ConcatRows(&v20, a2, a2 + 2);
        FR2MatrixVTC<float>* hProcess = distrib[a1].eigBasis.Mul(&apResult, v20);
        v20.operator=(*hProcess);
        uint32_t nr = a2->nrows;
        uint32_t nrows = a2[2].nrows;
        FR2MatrixVTC<float>* v6 = v20.subMatrix(&v14, 0, 0, nr, 1u);
        FR2MatrixVTC<float>* v5 = v6->Add(&v13, *distrib[a1].mean);
        a2->operator=(*v5);
        FR2MatrixVTC<float>* v4 = v20.subMatrix(&v12, nr, 0, nrows, 1u);
        FR2MatrixVTC<float>* v3 = v4->Add(&v11, distrib[a1].mean[1]);
        (a2 + 2)->operator=(*v3);
    }
}

void FanGenCtlsGrpC::geneticRandom(
    float afGeneticVariation,
    FR2MatrixVTC<float> (*apSourceCoord)[2][2],
    FR2MatrixVTC<float> (*apOutCoord)[2][2]
) {
    FR2MatrixVTC<float>::RowT result;
    for (int32_t j = 0; j < 2; ++j) {
        for (int32_t k = 0; k < 2; ++k) {
            uint32_t ncols = (*apSourceCoord)[j][k].ncols;
            uint32_t nrows = (*apSourceCoord)[j][k].nrows;
            FR2MatrixVTC<float>* v9 = &(*apOutCoord)[j][k];
            v9->nrows = nrows;
            v9->ncols = ncols;
            v9->data.resize(ncols * nrows, 0.0f);
            for (uint32_t i = 0; i < (*apSourceCoord)[j][k].nrows; ++i) {
                for (uint32_t m = 0; m < (*apSourceCoord)[j][k].ncols; ++m) {
                    if (k) {
                        (*apOutCoord)[j][k].operator[](i).p[m] = 0.0f;
                    }
                    else {
                        double v15 = RngStdNormal() * afGeneticVariation * 1.5;
                        float* v7 = &(*apSourceCoord)[j][k].operator[](i).p[m];
                        FR2MatrixVTC<float>::RowT* v6 = &(result = (*apOutCoord)[j][k].operator[](i));
                        float v4 = *v7 + v15;
                        v6->p[m] = v4;
                    }
                }
            }
        }
    }
}

void FanGenCtlsGrpC::reflect(FanRaceE race, FR2MatrixVTC<float> (*coord)[2]) {
    FR2MatrixVTC<float> v7;
    FR2MatrixVTC<float> v8;
    FR2MatrixVTC<float> apResult;
    if (race >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 629);
    }
    if (initialized) {
        for (uint32_t i = 0; i < 2; ++i) {
            FR2MatrixVTC<float>* hProcess = coord[i][1].MulScalar(&apResult, -1.0f);
            coord[i][1].operator=(*hProcess);
            FR2MatrixVTC<float>* v4 = distrib[race].mean[i].MulScalar(&v8, 2.0f);
            FR2MatrixVTC<float>* v3 = v4->Sub(&v7, coord[i][0]);
            coord[i][0].operator=(*v3);
        }
    }
}

void FanGenCtlsGrpC::tween(
    FanTypeE type,
    FanSymmE symm,
    float ratio,
    const FR2MatrixVTC<float> (*parent1)[2],
    const FR2MatrixVTC<float> (*parent2)[2],
    FR2MatrixVTC<float> (*child)[2]
) {
    FR2MatrixVTC<float> v12;
    FR2MatrixVTC<float> apResult;
    FR2MatrixVTC<float> v14;
    float v15;
    float v16 = getCaricature(FANRACE_ALL, type, symm, parent1);
    float v17 = getCaricature(FANRACE_ALL, type, symm, parent2);
    if (!(ratio < 0.0)) {
        if (!(ratio > 1.0)) {
            v15 = v16 * ratio + (1.0 - ratio) * v17;
        }
        else {
            v15 = v16;
        }
    }
    else {
        v15 = v17;
    }
    float s = 1.0 - ratio;
    FR2MatrixVTC<float>* v9 = parent2[type][symm].MulScalar(&apResult, s);
    FR2MatrixVTC<float>* v8 = parent1[type][symm].MulScalar(&v14, ratio);
    FR2MatrixVTC<float>* hProcess = v8->Add(&v12, *v9);
    child[type][symm].operator=(*hProcess);
    setCaricature(FANRACE_ALL, type, symm, v15, child);
}

FanFgFileDataS::FanFgFileDataS() {}

FanFgFileDataS::~FanFgFileDataS() {}

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
                FanLinCtlS* pctl = &LinCtrls.grp[i][j][0];
                if (pctl) {
                    FgData.coord[i][j].resize(pctl->ctl.ncols, pctl->ctl.nrows);
                }
            }
            else {
                FgData.coord[i][j].resize(0, 0);
            }
        }
    }
    spHairTeethAlphaProp = new NiAlphaProperty();
    spHairTeethAlphaProp.m_pObject->SetAlphaTesting(true);
    spHairTeethAlphaProp.m_pObject->SetTestMode(NiAlphaProperty::TEST_GREATER);
    spHairTeethAlphaProp.m_pObject->SetTestRef(100);
    NiPixelData* pbasepixeldata = new NiPixelData(32u, 32u, NiPixelFormat_RGBA32, 1u, 1u);
    // 32 x 32 RGBA32 pixels.
    ourMemset(pbasepixeldata->GetPixels(0u, 0u), 128, 0x1000u);
    pbasepixeldata->MarkAsChanged();
    pbasepixeldata->SetNoConvert(true);
    {
        NiFixedString kName("DefaultBaseModFaceGenTexture");
        spDefaultBaseMod =
            NiSourceTexture::Create(pbasepixeldata, kName.GetHandleString(), NiSourceTexture::ms_kDefaultPrefs);
    }
    NiPixelData* pdetailpixeldata = new NiPixelData(32u, 32u, NiPixelFormat_RGBA32, 1u, 1u);
    uint8_t* pdecalpixels = pdetailpixeldata->GetPixels(0u, 0u);
    for (int32_t i = 0; i < 1024; ++i) {
        pdecalpixels[4 * i] = 62;
        pdecalpixels[4 * i + 1] = 65;
        pdecalpixels[4 * i + 2] = 62;
        pdecalpixels[4 * i + 3] = 64;
    }
    pdetailpixeldata->MarkAsChanged();
    pdetailpixeldata->SetNoConvert(true);
    {
        NiFixedString kName("DefaultDetailModFaceGenTexture");
        spDefaultDetailMod =
            NiSourceTexture::Create(pdetailpixeldata, kName.GetHandleString(), NiSourceTexture::ms_kDefaultPrefs);
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
    return (bool)((int8_t)BSFaceGenManager::bEmotions);
}

// The age is the floored coordinate attribute rounded to float and truncated to a signed byte. The accessory loop
// passes the face part counter, 8 after its loop, as the head part index, as the game does.
void BSFaceGenManager::QueueHeadParts(TESNPC* apNPC, IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    alignas(AutoMemContext) char acMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (acMemContext) AutoMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp", 3640
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
    XFNV_INLINED_LOCAL(NiFixedString, v36, std::nullopt, std::nullopt);
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp", 3114
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
    uint32_t ishaderindex = 0;
    BSFaceGenManager::LockFaceGenAccess();
    if (apNode && &arNpcData) {
        char Dst[268];
        float X = BSFaceGenManager::GetFaceCoordAttribute(arNpcData.FaceCoord, FANATT_AGE, FANTYPE_GEO);
        char cage = static_cast<int32_t>(std::floorf(X));
        BSstrcpy(Dst, 260, "Textures\\");
        for (uint32_t ipart = 0; ipart < 8; ++ipart) {
            fileDetail.Set("", 0);
            if (ipart != 6 && ipart != 7) {
                if (ipart > 1) {
                    ishaderindex = 1;
                }
                else {
                    ishaderindex = 14;
                }
                if ((arNpcData.pModel.operator[](ipart)) && (arNpcData.pTexture.operator[](ipart))) {
                    NiFixedString* v33 =
                        ::new (static_cast<void*>(&v36)) NiFixedString(arNpcData.pHeadPartNodeName.operator[](ipart));
                    NiAVObject* pobj = apNode->GetObjectByName(*v33);
                    v36.~NiFixedString();
                    if (pobj) {
                        NiTriBasedGeom* pgeom = pobj->IsTriBasedGeom();
                        if (pgeom) {
                            BSFaceGenModel* pmodelFG = BSFaceGenManager::GetFaceGenModel(pgeom);
                            if (pmodelFG) {
                                int32_t v32;
                                BSShaderManager::PrepareObject(pobj, 0, 0);
                                NiShadeProperty* pshaderprop = static_cast<NiShadeProperty*>(pobj->GetProperty(3));
                                if (pshaderprop) {
                                    int32_t v31 =
                                        pshaderprop->iShaderPropertyType >= 8 && pshaderprop->iShaderPropertyType <= 12;
                                    v32 = v31;
                                }
                                else {
                                    v32 = 0;
                                }
                                BSShaderPPLightingProperty* pshaderpropLighting =
                                    v32 ? static_cast<BSShaderPPLightingProperty*>(pshaderprop) : nullptr;
                                if (pshaderpropLighting) {
                                    const char* Xa;
                                    if ((arNpcData.spTexBaseMod.m_usSize) && (arNpcData.bFaceGenTexturing)) {
                                        sptexBaseMod.operator=(arNpcData.spTexBaseMod.operator[](ipart));
                                    }

                                    if (ipart ||
                                        (cage < 0 ||
                                         (Xa = arNpcData.pTexture.operator[](ipart)->GetTextureName(),
                                          !BSFaceGenManager::GetAsNearestDetailDDSFile(
                                              fileDetail, arNpcData.eSex, static_cast<uint8_t>(cage), Xa
                                          )))) {
                                        if (!arNpcData.spTexBaseMod.m_usSize ||
                                            !(&arNpcData.spTexBaseMod.operator[](ipart))->m_pObject) {
                                            uint32_t Xb = BSFaceGenManager::GetEGTClampSize();
                                            pmodelFG->ApplyCoordinateTexturingToMesh(
                                                &(&arNpcData)->FaceCoord, sptexBaseMod, Xb
                                            );
                                        }
                                        BSStringT<char> tempstr;
                                        TESTexture* pTexture = arNpcData.pTexture.operator[](ipart);
                                        pTexture->GetAsNormalFile(tempstr);
                                        if (tempstr.pString) {
                                            fileSpecular.operator=("Textures\\");
                                            fileSpecular.operator+=(static_cast<const char*>(tempstr.pString));
                                        }
                                    }
                                    else {
                                        if (!arNpcData.spTexBaseMod.m_usSize ||
                                            !(&arNpcData.spTexBaseMod.operator[](ipart))->m_pObject) {
                                            uint32_t EGTClampSize = BSFaceGenManager::GetEGTClampSize();
                                            pmodelFG->ApplyCoordinateTexturingToMesh(
                                                &(&arNpcData)->FaceCoord, sptexBaseMod, EGTClampSize
                                            );
                                        }
                                        char* fR = fileDetail.pString;
                                        TES::pInstance->CreateTextureImage(fR, sptexDetailMod, true, true);
                                        BSUtils::CreateNormalMapString(fileSpecular, fileDetail.pString);
                                    }
                                    if (fileSpecular.pString) {
                                        char* fRa = fileSpecular.pString;
                                        TES::pInstance->CreateTextureImage(fRa, sptexNormal, true, true);
                                    }
                                    if (sptexNormal.m_pObject) {
                                        NiTexture* Xc = sptexNormal.m_pObject;
                                        pshaderpropLighting->SetNormalTexture(0, Xc);
                                        const NiPixelFormat* PixelFormat =
                                            sptexNormal.m_pObject->NiTexture::GetPixelFormat();
                                        uint32_t eFormat = PixelFormat->m_eFormat;
                                        bool bspecular = eFormat == 5 || (eFormat == 6 || eFormat == 1);
                                        pshaderpropLighting->SetFlag(0, bspecular);
                                    }
                                    else {
                                        if (!pshaderpropLighting->GetNormalTexture(0)) {
                                            NiSourceTexture* DefNormalMap = BSShader::GetDefNormalMap();
                                            pshaderpropLighting->SetNormalTexture(0, DefNormalMap);
                                        }
                                    }
                                    if (arNpcData.bFaceGenTexturing && ishaderindex == 14) {
                                        NiTexture* pDetailTexture;
                                        NiTexture* Xd = sptexBaseMod.m_pObject;
                                        pshaderpropLighting->SetDiffuseTexture(1u, Xd);
                                        if (sptexDetailMod.m_pObject) {
                                            pDetailTexture = sptexDetailMod.m_pObject;
                                        }
                                        else {
                                            pDetailTexture = BSFaceGenManager::GetDefaultDetailModulationTexture();
                                        }
                                        pshaderpropLighting->SetNormalTexture(1, pDetailTexture);
                                        pshaderpropLighting->SetFlag(10u, 1);
                                        NiMaterialProperty* pmatprop =
                                            static_cast<NiMaterialProperty*>(pobj->GetProperty(2));
                                        if ((pmatprop) && (pmatprop->m_fShine < 1.0)) {
                                            pmatprop->SetShineness(4.0f);
                                        }

                                        if (fileSpecular.pString) {
                                            char Src[264];
                                            char DstBuf[268];
                                            BSsprintf(Src, 260, "%s", fileSpecular.String());
                                            BSShaderManager::GetModifiedTextureFilename(DstBuf, Src, "_sk", true);
                                            if (DstBuf[0]) {
                                                NiPointer<NiSourceTexture> spTempSourceTexture(
                                                    static_cast<NiSourceTexture*>(nullptr)
                                                );
                                                TES::pInstance->CreateTextureImage(
                                                    DstBuf, spTempSourceTexture, true, true
                                                );
                                                NiTexture* Xe = spTempSourceTexture.m_pObject;
                                                pshaderpropLighting->SetTexture(BSShaderProperty::TEXTYPE_GLOW, 0, Xe);
                                            }
                                        }
                                    }
                                    sptexBaseMod = (sptexDetailMod = nullptr);
                                    sptexNormal = nullptr;
                                    BSFaceGenManager::PrecacheFaceGeometry(pobj, ishaderindex, 1u);
                                    pshaderpropLighting->PrecacheTextures();
                                }
                            }
                        }
                    }
                }
            }
        }
        for (uint32_t i = 0;; ++i) {
            uint32_t ChildCount = apNode->GetChildCount();
            if (i >= ChildCount) {
                break;
            }
            NiAVObject* pchild = apNode->GetAt(i);
            if (pchild) {
                uint32_t icompress_2 = 1;
                if (NiFixedString::HandlesEqual(pchild->m_kName, leftEye) ||
                    NiFixedString::HandlesEqual(pchild->m_kName, rightEye)) {
                    icompress_2 = 0;
                    BSShaderManager::PrepareObject(pchild, 0, 0);
                }
                else {
                    const char* HandleString = hair.GetHandleString();
                    size_t Xf = std::strlen(HandleString);
                    const char* fB = hair.GetHandleString();
                    const char* v7 = (&pchild->m_kName)->GetHandleString();
                    if (!_strnicmp(v7, fB, Xf) || NiFixedString::HandlesEqual(pchild->m_kName, headAccessory)) {
                        int32_t v27;
                        BSShaderManager::PrepareObject(pchild, 0, 0);
                        NiShadeProperty* pshadeprop = static_cast<NiShadeProperty*>(pchild->GetProperty(3));
                        if (pshadeprop) {
                            v27 = pshadeprop->iShaderPropertyType == 8;
                        }
                        else {
                            v27 = 0;
                        }
                        BSShaderPPLightingProperty* phairprop =
                            v27 ? static_cast<BSShaderPPLightingProperty*>(pshadeprop) : nullptr;
                        if (phairprop) {
                            uint32_t icol = arNpcData.iHairColor;
                            float v26 = static_cast<double>(static_cast<uint8_t>(icol >> 16)) / 255.0;
                            float v25 = static_cast<double>(static_cast<uint8_t>(icol >> 8)) / 255.0;
                            float v24 = static_cast<double>(static_cast<uint8_t>(icol)) / 255.0;
                            NiColorA v35(v24, v25, v26, 1.0f);
                            NiColorA* v8 = &v35;
                            phairprop->pHairTintColor = *v8;
                            phairprop->PrecacheTextures();
                        }
                        else {
                            int32_t v23;
                            if (pshadeprop) {
                                v23 = pshadeprop->iShaderPropertyType == 12;
                            }
                            else {
                                v23 = 0;
                            }
                            Lighting30ShaderProperty* psm3prop =
                                v23 ? static_cast<Lighting30ShaderProperty*>(pshadeprop) : nullptr;
                            if (psm3prop) {
                                uint32_t icol_2 = arNpcData.iHairColor;
                                float v22 = static_cast<double>(static_cast<uint8_t>(icol_2 >> 16)) / 255.0;
                                float v21 = static_cast<double>(static_cast<uint8_t>(icol_2 >> 8)) / 255.0;
                                float v20 = static_cast<double>(static_cast<uint8_t>(icol_2)) / 255.0;
                                NiColorA v34(v20, v21, v22, 1.0f);
                                NiColorA* v9 = &v34;
                                psm3prop->pHairTintColor = *v9;
                                psm3prop->PrecacheTextures();
                            }
                        }
                    }
                }
                BSFaceGenManager::PrecacheFaceGeometry(pchild, ishaderindex, icompress_2);
            }
        }
        apNode->UpdateProperties();
        NiUpdateData UpdateData(0.0f, false, false);
        apNode->Update(UpdateData);
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
    int32_t iage = acAge;
    char cgender = 77;
    if (!apModel || (iage > 100 || aeSex >= 2)) {
        return nullptr;
    }
    BSstrcpy(cHeadName, 260, apModel);
    char* pextension = std::strrchr(cHeadName, 46);
    if (!pextension) {
        return nullptr;
    }
    *pextension = 0;
    if (aeSex == 1) {
        cgender = 70;
    }
    aOutFilename.SPrintF("Textures\\%s%c%d.ddx", cHeadName, cgender, iage);
    BSHash::MakeDirAndFile(aOutFilename.pString, rDirectoryHash, rFileNameHash);
    BSFileEntry* pFileEntry = ArchiveManager::GetFileEntryForFileFromAllArchives(
        ARCHIVE_TYPE_INDEX_TEXTURES, rDirectoryHash, rFileNameHash, aOutFilename.pString
    );
    if (!pFileEntry) {
        iage = 10 * (iage / 10);
        while (true) {
            aOutFilename.SPrintF("Textures\\%s%c%d.dds", cHeadName, cgender, iage);
            pFileEntry = ArchiveManager::GetFileEntryForFileFromAllArchives(
                ARCHIVE_TYPE_INDEX_TEXTURES, rDirectoryHash, rFileNameHash, aOutFilename.pString
            );
            if (pFileEntry) {
                break;
            }
            iage -= 10;
            if (iage < 0) {
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

// Deleting pInstance tests the copy for null again, as the game does.
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
                        float fMean =
                            static_cast<float>((*apCopyToCoord)[i][j].lengthSqr() / static_cast<double>(uiRows));
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
                        float fMean =
                            static_cast<float>((*apCopyToCoord)[i][j].lengthSqr() / static_cast<double>(uiRows));
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
    char Dst[264];
    char apDest[264];
    int32_t v5 = 0;
    if (!apModel) {
        return nullptr;
    }
    BSstrcpy(Dst, 260, apModel);
    char* pextension = std::strrchr(Dst, 46);
    if (!pextension) {
        return nullptr;
    }
    *pextension = 0;
    BSsprintf(apDest, 260u, "%s.egt", Dst);
    aOutFilename.Set(apDest, 0u);
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
    XFNV_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    BSHash arDirectoryHash;
    BSHash arFileNameHash;
    XFNV_INLINED_LOCAL(NiPointer<NiTriBasedGeom>, spshape, std::nullopt, std::nullopt);
    NiMatrix3 matrot;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp", 2875
    );
    BSStringT<char> filename;
    BSStringT<char> fileEGM;
    BSStringT<char> fileTRI;
    BSStringT<char> fileEGT;
    BSStringT<char> fileFIM;
    NiPointer<NiSourceTexture> sptexBase(static_cast<NiSourceTexture*>(nullptr));
    ::new (&spshape) NiPointer<ActorPathingMessageQueue>(static_cast<ActorPathingMessageQueue*>(nullptr));
    matrot.MakeYRotation(-1.5707964f);
    if (aNpcData.pHair) {
        for (HAIR_PART ehairPart = HAIR_PART_NORMAL; ehairPart < HAIR_PART_COUNT;
             ehairPart = static_cast<HAIR_PART>(ehairPart + 1)) {
            const char* v4 = aNpcData.pHair->GetModel();
            filename.SPrintF("Meshes\\%s", v4);
            const char* pegm = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, ehairPart);
            const char* ptri = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, ehairPart);
            BSFaceGenModel* pmodelFG = nullptr;
            BSHash::MakeDirAndFile(pegm, arDirectoryHash, arFileNameHash);
            if (ArchiveManager::GetFileEntryForFileFromAllArchives(
                    ARCHIVE_TYPE_INDEX_MESHES, arDirectoryHash, arFileNameHash, pegm
                )) {
                HAIR_PART v13 = ehairPart;
                const char* v10 = ptri;
                char* v8 = filename.pString;
                pmodelFG = BSFaceGenManager::GetFaceGenModel(pegm, v8, v10, nullptr, true, v13, false);
            }
            else {
                if (ehairPart == HAIR_PART_NORMAL) {
                    pegm = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
                    ptri = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
                    HAIR_PART v12 = ehairPart;
                    const char* v9 = ptri;
                    char* v7 = filename.pString;
                    pmodelFG = BSFaceGenManager::GetFaceGenModel(pegm, v7, v9, nullptr, true, v12, false);
                    ehairPart = HAIR_PART_COUNT;
                }
            }
            if (pmodelFG) {
                pmodelFG->ApplyCoordinateToNewMesh(&(&aNpcData)->FaceCoord, spshape);
            }
            if (spshape.m_pObject) {
                char tempHairName[32];
                if (ehairPart == HAIR_PART_HAT) {
                    spshape.m_pObject->SetAppCulled(true);
                }
                char* HairName = BSFaceGenManager::GetHairName(tempHairName, ehairPart);
                NiFixedString* a2 =
                    ::new (static_cast<void*>(&apString)) NiFixedString(static_cast<const char*>(HairName));
                spshape.m_pObject->SetName(*a2);
                apString.~NiFixedString();
                BSFaceGenManager::ApplyHairMorph(spshape.m_pObject, aNpcData.fHairLength);
                filename.SPrintF("Textures\\%s", aNpcData.pHair->GetTextureName());
                char* v11 = filename.pString;
                TES::pInstance->CreateTextureImage(v11, sptexBase, false, false);
                if (sptexBase.m_pObject) {
                    int32_t v16;
                    NiShadeProperty* pprop = static_cast<NiShadeProperty*>(spshape.m_pObject->GetProperty(3));
                    if (pprop) {
                        int32_t v15 = pprop->iShaderPropertyType >= 8 && pprop->iShaderPropertyType <= 12;
                        v16 = v15;
                    }
                    else {
                        v16 = 0;
                    }
                    BSShaderPPLightingProperty* pshaderprop =
                        v16 ? static_cast<BSShaderPPLightingProperty*>(pprop) : nullptr;
                    if (pshaderprop) {
                        NiSourceTexture* afAngle = sptexBase.m_pObject;
                        pshaderprop->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, afAngle);
                    }
                }
                if (!spshape.m_pObject->GetProperty(0)) {
                    NiAlphaProperty* HairTeethAlphaProperty = BSFaceGenManager::GetHairTeethAlphaProperty();
                    spshape.m_pObject->AttachProperty(HairTeethAlphaProperty);
                }
                if (spshape.m_pObject->GetSkinInstance()) {
                    if (abForceNoSkinning) {
                        spshape.m_pObject->SetSkinInstance(nullptr);
                        spshape.m_pObject->SetTranslate(NiPoint3_ZERO);
                        spshape.m_pObject->SetRotate(matrot);
                        apOutNodeBiped->AttachChild(spshape.m_pObject, 1);
                    }
                    else {
                        apOutNodeSkinned->AttachChild(spshape.m_pObject, 1);
                    }
                }
                else {
                    spshape.m_pObject->SetRotate(matrot);
                    apOutNodeBiped->AttachChild(spshape.m_pObject, 1);
                }
            }
            pmodelFG = nullptr;
            spshape.operator=(static_cast<NiTriBasedGeom*>(nullptr));
            sptexBase.operator=(static_cast<NiSourceTexture*>(nullptr));
        }
    }
    spshape.~NiPointer<NiTriBasedGeom>();
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
        apQuantities[i] = 3.4028235e+38f;
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
        MEM_CONTEXT_FACEGEN_SYSTEM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp", 997
    );
    NiPointer<BSFaceGenModel> spnewModel(static_cast<BSFaceGenModel*>(nullptr));
    if (!apEGMFilename && !apEGTFilename) {
        return nullptr;
    }
    if (!BSFaceGenManager::GetInstance()->pModelMap) {
        BSFaceGenModelMap* pModelMap;
        BSFaceGenModelMap* v23 = static_cast<BSFaceGenModelMap*>(ourMalloc(sizeof(BSFaceGenModelMap)));
        if (v23) {
            pModelMap = ::new (v23) BSFaceGenModelMap;
        }
        else {
            pModelMap = nullptr;
        }
        BSFaceGenManager::GetInstance()->pModelMap = pModelMap;
        uint32_t v13 = gs_uiFaceGenMaxEGMDataSize_General.operator uint32_t();
        BSFaceGenManager* Instance = BSFaceGenManager::GetInstance();
        Instance->pModelMap->SetMaxDataSize(BSFaceGenModelMap::DATA_TYPE_EGM, v13);
        uint32_t v14 = gs_uiFaceGenMaxEGTDataSize_General.operator uint32_t();
        BSFaceGenManager* v9 = BSFaceGenManager::GetInstance();
        v9->pModelMap->SetMaxDataSize(BSFaceGenModelMap::DATA_TYPE_EGT, v14);
    }
    if (apEGMFilename) {
        BSFaceGenManager* v10 = BSFaceGenManager::GetInstance();
        if (v10->pModelMap->GetAt(apEGMFilename, spnewModel)) {
            if ((apEGTFilename) && (!spnewModel.m_pObject->GetEGTFilename())) {
                spnewModel.m_pObject->LoadModelTexture(apEGTFilename);
            }

            BSFaceGenModel* v22 = spnewModel.m_pObject;
            return v22;
        }
    }
    else {
        if (apEGTFilename) {
            BSFaceGenManager* v11 = BSFaceGenManager::GetInstance();
            if (v11->pModelMap->GetAt(apEGTFilename, spnewModel)) {
                BSFaceGenModel* v21 = spnewModel.m_pObject;
                return v21;
            }
        }
    }
    if (abAllowCreate) {
        BSFaceGenModel* v17;
        BSFaceGenModel* v20 = static_cast<BSFaceGenModel*>(NiMemObject::operator new(sizeof(BSFaceGenModel)));
        if (v20) {
            v17 = ::new (v20) BSFaceGenModel;
        }
        else {
            v17 = nullptr;
        }
        spnewModel.operator=(v17);
        if (spnewModel.m_pObject->LoadModel(
                apEGMFilename, apNIFFilename, apTRIFilename, apEGTFilename, aeHairPart, abIsAccessory
            )) {
            const char* apcKey;
            if (apEGMFilename) {
                apcKey = apEGMFilename;
            }
            else {
                apcKey = apEGTFilename;
            }
            BSFaceGenModel* v15 = spnewModel.m_pObject;
            BSFaceGenManager* v12 = BSFaceGenManager::GetInstance();
            v12->pModelMap->SetAt(apcKey, v15);
        }
        else {
            spnewModel.operator=(static_cast<BSFaceGenModel*>(nullptr));
        }
    }
    BSFaceGenModel* v19 = spnewModel.m_pObject;
    return v19;
}

// Without a manager, builds a null temporary smart pointer, reads it through operator-> and releases it under a
// construction flag, as the game does.
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
    NiAlphaProperty* pProperty = pspProperty->operator->();
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
        pGeom, 0, 0,
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
    AutoMemContext apMemory(
        MEM_CONTEXT_FACEGEN_SYSTEM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp", 790
    );
    if (BSFaceGenManager::pInstance) {}
    else {
        BSFaceGenManager* v0;
        BSFaceGenManager* v1 = static_cast<BSFaceGenManager*>(ourMalloc(sizeof(BSFaceGenManager)));
        if (v1) {
            v0 = ::new (v1) BSFaceGenManager;
        }
        else {
            v0 = nullptr;
        }
        BSFaceGenManager::pInstance = v0;
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
        MEM_CONTEXT_FACEGEN_EGM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp", 1100
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
        MEM_CONTEXT_FACEGEN_EGT, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp", 1114
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
        default: eExpression = static_cast<BS_FACEGEN_EXPRESSION>(-1); break;
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
    BSFaceGenBaseMorphExtraData* FaceGenExtra = static_cast<BSFaceGenBaseMorphExtraData*>(
        BSFaceGenUtils::GetFaceGenExtraData<BSFaceGenBaseMorphExtraData>(apGeom)
    );
    if ((FaceGenExtra) && (arIterator.m_pvCurrentPosition)) {
        NiGeometryDataIteratorNiPoint3 v5 = arIterator;
        uint32_t i = 0;
        NiPoint3* pVerts = FaceGenExtra->pVerts;
        while (i < apGeom->GetVertexCount()) {
            v5.SetData(*pVerts);
            v5.Next();
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
    NiNode* proot;
    XFNV_INLINED_LOCAL(std::type_identity_t<FR2MatrixVTC<float>[2][2]>, newcoord, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(std::type_identity_t<FR2MatrixVTC<float>[2][2]>, oldcoord, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(std::type_identity_t<FR2MatrixVTC<float>[2][2]>, diffcoord, std::nullopt, std::nullopt);
    TESNPC* pnpc = static_cast<TESNPC*>(apActor->GetObjectReference());
    NiAVObject* pobj = apActor->Get3D();
    if (pobj) {
        proot = pobj->IsNode();
    }
    else {
        proot = nullptr;
    }
    rt::eh_vector_constructor_iterator(
        oldcoord, sizeof(FR2MatrixVTC<float>), 4, &xfnv::constructElement<FR2MatrixVTC<float>>,
        &xfnv::destructElement<FR2MatrixVTC<float>>
    );
    rt::eh_vector_constructor_iterator(
        newcoord, sizeof(FR2MatrixVTC<float>), 4, &xfnv::constructElement<FR2MatrixVTC<float>>,
        &xfnv::destructElement<FR2MatrixVTC<float>>
    );
    rt::eh_vector_constructor_iterator(
        diffcoord, sizeof(FR2MatrixVTC<float>), 4, &xfnv::constructElement<FR2MatrixVTC<float>>,
        &xfnv::destructElement<FR2MatrixVTC<float>>
    );
    pnpc->GetFaceCoord(oldcoord);
    pnpc->GetFaceCoord(newcoord);
    float val =
        BSFaceGenManager::GetFaceCoordValue(&oldcoord, FANTYPE_GEO, FANSYMM_SYMM, BSFaceGenManager::iCycleCtrlID);
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
    BSFaceGenNiNode* pfacenode = apActor->GetFaceNodeBiped(nullptr);
    for (int32_t a = 0; a < 2; ++a) {
        uint32_t isizeArray = pfacenode->GetArrayCount();
        for (uint32_t ichild = 0; ichild < isizeArray; ++ichild) {
            NiTriBasedGeom* ptri;
            NiAVObject* pfaceobj = pfacenode->GetAt(ichild);
            if (pfaceobj) {
                ptri = pfaceobj->IsTriBasedGeom();
            }
            else {
                ptri = nullptr;
            }
            if (ptri) {
                BSFaceGenModel* pfgmodel;
                BSFaceGenModelExtraData* pmodelExtraData = BSFaceGenManager::GetFaceGenModelExtraData(ptri);
                if (pmodelExtraData) {
                    pfgmodel = pmodelExtraData->GetModel();
                }
                else {
                    pfgmodel = nullptr;
                }
                if (pfgmodel) {
                    pfgmodel->ApplyCoordinateToExistingMesh(&diffcoord, ptri, false);
                }
            }
        }
        pfacenode = apActor->GetFaceNodeSkinned(nullptr);
    }
    FR2MatrixVTC<float>(*OffsetFaceCoord)[2][2] = pnpc->GetOffsetFaceCoord();
    TESActorBase::SEX Sex = pnpc->GetSex();
    FR2MatrixVTC<float>(*MeanFaceCoord)[2][2] = pnpc->GetFormRace()->GetMeanFaceCoord(Sex);
    BSFaceGenManager::OffsetFaceGenCoord(MeanFaceCoord, &newcoord, OffsetFaceCoord, false);
    rt::eh_vector_destructor_iterator(
        diffcoord, sizeof(FR2MatrixVTC<float>), 4, &xfnv::destructElement<FR2MatrixVTC<float>>
    );
    rt::eh_vector_destructor_iterator(
        newcoord, sizeof(FR2MatrixVTC<float>), 4, &xfnv::destructElement<FR2MatrixVTC<float>>
    );
    rt::eh_vector_destructor_iterator(
        oldcoord, sizeof(FR2MatrixVTC<float>), 4, &xfnv::destructElement<FR2MatrixVTC<float>>
    );
}

void BSFaceGenManager::SetUseProceduralEyeBlinking(bool abUse) {
    BSFaceGenManager::bUseProceduralEyeBlinking = (bool)((uint8_t)abUse);
}

void BSFaceGenManager::SetToZero(float* apQuantities, uint32_t aiNumQuantities) {
    if ((apQuantities) && (aiNumQuantities)) {
        ourMemset(apQuantities, 0, sizeof(float) * ((int32_t)aiNumQuantities));
    }
}

uint32_t BSFaceGenManager::GetEGTClampSize() {
    return (&BSFaceGenManager::uiEGTClampSize)->Setting::Int();
}

void BSFaceGenManager::SetUseEmotions(bool abUseEmotions) {
    int8_t result = (int8_t)abUseEmotions;
    BSFaceGenManager::bEmotions = (bool)((int8_t)abUseEmotions);
}

bool BSFaceGenManager::GetFixNormals() {
    return (bool)((uint8_t)(&TESRace::bFixFaceNormals)->Bool());
}

BSFaceGenMorphData* BSFaceGenManager::GetFaceGenMorphData(NiTriBasedGeom* apGeom) {
    BSFaceGenModelExtraData* faceGenExtra = BSFaceGenManager::GetFaceGenModelExtraData(apGeom);
    if ((!faceGenExtra) || (!faceGenExtra->GetModel())) {
        return nullptr;
    }
    BSFaceGenModel* v1 = faceGenExtra->GetModel();
    return v1->GetMorph();
}

// The source's NiDynamicCast: BSFaceGenMorphData's own static DynamicCast, then the downcast to the head data.
BSFaceGenMorphDataHead* BSFaceGenManager::GetFaceGenMorphDataHead(NiTriBasedGeom* apGeom) {
    BSFaceGenMorphData* pmorph = BSFaceGenManager::GetFaceGenMorphData(apGeom);
    if (!pmorph) {
        return nullptr;
    }
    return static_cast<BSFaceGenMorphDataHead*>(
        BSFaceGenMorphData::DynamicCast(&BSFaceGenMorphDataHead::ms_RTTI, pmorph)
    );
}

BSFaceGenMorphDataHair* BSFaceGenManager::GetFaceGenMorphDataHair(NiTriBasedGeom* apGeom) {
    BSFaceGenMorphData* pmorph = BSFaceGenManager::GetFaceGenMorphData(apGeom);
    if (pmorph) {
        return static_cast<BSFaceGenMorphDataHair*>(
            BSFaceGenMorphDataHair::DynamicCast(&BSFaceGenMorphDataHair::ms_RTTI, pmorph)
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
    XFNV_RECONSTRUCT(NiTConvertingStrideIterator<NiPoint3>)(&kIterator);
    apGeom->GetModelData()->GetVerticesIterator(kIterator);
    BSFaceGenManager::RefreshMeshFromBaseMorphExtraData(apGeom, kIterator);
    if (kIterator.GetCurrentPosition()) {
        pMorphDataHair->ApplyMorph(apGeom, nullptr, afModifierValue);
    }
    apGeom->GetModelData()->UnlockPackedVertexData();
}

void BSFaceGenManager::ReplaceFaceMeshLOD(BSFaceGenNiNode* apNode, int32_t aDetail) {
    XFNV_INLINED_LOCAL(NiFixedString, v14, std::nullopt, std::nullopt);
    NiGeometryDataIteratorNiPoint3 spNewFaceGeo;
    XFNV_INLINED_LOCAL(NiPointer<BSFaceGenModel>, pmodelFG, std::nullopt, std::nullopt);
    NiCloningProcess v20;
    BSHash kDirHash;
    BSHash kFileHash;
    for (int32_t i = 0;; ++i) {
        uint32_t ArrayCount = apNode->GetArrayCount();
        if (i >= ArrayCount) {
            break;
        }
        NiAVObject* pchild = apNode->GetAt(static_cast<uint32_t>(i));
        if (pchild) {
            const char* name = (&pchild->m_kName)->GetHandleString();
            if (!BSstrcmp(name, "FaceGenFace")) {
                XFNV_INLINED_LOCAL(
                    std::type_identity_t<FR2MatrixVTC<float>[2][2]>, facecoord, std::nullopt, std::nullopt
                );
                NiTriBasedGeom* kCloneProc_20 = pchild->IsTriBasedGeom();
                BSFaceGenModel* poldmodelFG = BSFaceGenManager::GetFaceGenModel(kCloneProc_20);
                NiPointer<NiTriBasedGeom> spshape(static_cast<NiTriBasedGeom*>(nullptr));
                NiTriBasedGeom* pshapeHead = nullptr;
                BSStringT<char> filename;
                BSStringT<char> lodfname;
                BSStringT<char> fileEGM;
                BSStringT<char> fileTRI;
                BSStringT<char> fileEGT;
                BSStringT<char> kCloneProc_24;
                rt::eh_vector_constructor_iterator(
                    facecoord, sizeof(FR2MatrixVTC<float>), 4, &xfnv::constructElement<FR2MatrixVTC<float>>,
                    &xfnv::destructElement<FR2MatrixVTC<float>>
                );
                FR2MatrixVTC<float>(*pcoord)[2][2] = BSFaceGenManager::GetFaceGenCoord();
                BSFaceGenManager::CopyFaceGenCoord(pcoord, &facecoord, false, false);
                const char* EGMFilename = poldmodelFG->GetEGMFilename();
                BSFaceGenManager::GetAsNIFFile(filename, EGMFilename);
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
                        ARCHIVE_TYPE_INDEX_MESHES, kDirHash, kFileHash, lodfname.pString
                    );
                }
                if (!pFileEntry) {
                    rt::eh_vector_destructor_iterator(
                        facecoord, sizeof(FR2MatrixVTC<float>), 4, &xfnv::destructElement<FR2MatrixVTC<float>>
                    );
                    return;
                }
                const char* AsEGTFile = BSFaceGenManager::GetAsEGTFile(fileEGT, lodfname.pString);
                const char* AsTRIFile = BSFaceGenManager::GetAsTRIFile(fileTRI, lodfname.pString, HAIR_PART_NONE);
                char* v7 = lodfname.pString;
                const char* AsEGMFile = BSFaceGenManager::GetAsEGMFile(fileEGM, lodfname.pString, HAIR_PART_NONE);
                ::new (&pmodelFG) NiPointer<BSFaceGenModel>(
                    BSFaceGenManager::GetFaceGenModel(AsEGMFile, v7, AsTRIFile, AsEGTFile, true, HAIR_PART_NONE, false)
                );
                if (!pmodelFG.m_pObject->ApplyCoordinateToNewMesh(&facecoord, spshape) || !spshape.m_pObject) {
                    pmodelFG.~NiPointer<BSFaceGenModel>();
                    rt::eh_vector_destructor_iterator(
                        facecoord, sizeof(FR2MatrixVTC<float>), 4, &xfnv::destructElement<FR2MatrixVTC<float>>
                    );
                    return;
                }
                NiFixedString* v13 = ::new (static_cast<void*>(&v14)) NiFixedString("FaceGenFace");
                spshape.m_pObject->SetName(*v13);
                v14.~NiFixedString();
                pshapeHead = spshape.m_pObject;
                const NiPoint3* Translate = &pchild->GetTranslate();
                pshapeHead->SetTranslate(*Translate);
                NiTransform* LocalTransform = &pchild->m_kLocal;
                pshapeHead->SetRotate(LocalTransform->m_Rotate);
                float LocalScale = pchild->m_kLocal.m_fScale;
                pshapeHead->SetScale(LocalScale);
                BSShaderManager::PrepareObject(pshapeHead, 0, 0);
                // The shader property of the face mesh is a BSShaderPPLightingProperty; its tangent space data is read
                // without a check.
                BSShaderPPLightingProperty* pshaderProp =
                    static_cast<BSShaderPPLightingProperty*>(pshapeHead->GetProperty(3));
                NiProperty* kCloneProc_16 = pchild->GetProperty(3);
                v20.~NiCloningProcess();
                ::new (&v20) NiCloningProcess(257u);
                BSShaderPPLightingProperty* pshaderPropLighting =
                    static_cast<BSShaderPPLightingProperty*>(kCloneProc_16->Clone(v20));
                BSShaderPPLightingProperty::TangentSpaceData* pTangentSpaceData = pshaderProp->GetExtraDataPointers();
                pshaderPropLighting->StoreExtraDataPointers(pTangentSpaceData);
                pshapeHead->DetachProperty(pshaderProp);
                pshapeHead->AttachProperty(pshaderPropLighting);
                // The child named "FaceGenFace" is the face geometry: its shader is read from pchild itself, with no
                // cast or check, as the game does.
                BSShader* Shader = static_cast<NiGeometry*>(pchild)->GetShader();
                pshapeHead->SetShader(Shader);
                uint16_t usVertices = pshapeHead->GetActiveVertexCount();
                if (pshapeHead->GetModelData()->LockPackedVertexData(true)) {
                    XFNV_RECONSTRUCT(NiTConvertingStrideIterator<NiPoint3>)(&spNewFaceGeo);
                    pshapeHead->GetModelData()->GetVerticesIterator(spNewFaceGeo);
                    if (spNewFaceGeo.m_pvCurrentPosition) {
                        BSFaceGenManager::ComputeModelBound(
                            pshapeHead->GetModelData()->m_kBound, spNewFaceGeo, usVertices
                        );
                    }
                }
                NiMaterialProperty* pmatprop = static_cast<NiMaterialProperty*>(pshapeHead->GetProperty(2));
                if ((pmatprop) && (pmatprop->m_fShine < 1.0)) {
                    pmatprop->SetShineness(4.0f);
                }

                apNode->SetAt(static_cast<uint32_t>(i), pshapeHead);
                pshapeHead->UpdateProperties();
                apNode->FixSkinInstances(apNode->m_pkParent, false);
                if (aDetail) {
                    if (aDetail == 1) {
                        apNode->bUsingLoResHead = false;
                    }
                }
                else {
                    apNode->bUsingLoResHead = true;
                }
                pmodelFG.~NiPointer<BSFaceGenModel>();
                rt::eh_vector_destructor_iterator(
                    facecoord, sizeof(FR2MatrixVTC<float>), 4, &xfnv::destructElement<FR2MatrixVTC<float>>
                );
            }
        }
    }
}

BS_FACEGEN_LOD BSFaceGenManager::GetForcedLod() {
    return static_cast<BS_FACEGEN_LOD>(iForcedLod);
}

void BSFaceGenManager::ClearBodyTexturesFromPalette(TESNPC* apNPC) {
    int8_t apDest[264];
    int8_t a2[268];
    TESRace* pRace = apNPC->GetRace();
    pRace->GetBodyModTextureName(apNPC, static_cast<char*>(static_cast<void*>(a2)));
    pRace = apNPC->GetRace();
    pRace->GetBodyModTextureFileName(
        apNPC, static_cast<char*>(static_cast<void*>(a2)), static_cast<char*>(static_cast<void*>(apDest))
    );
    BSTexturePalette::RemoveTexture_0(static_cast<const char*>(static_cast<const void*>(apDest)));
}

// The manager's default base texture, or null without a manager.
NiSourceTexture* BSFaceGenManager::GetBaseTexture() {
    XFNV_INLINED_LOCAL(NiPointer<NiSourceTexture>, spNull, std::nullopt, std::nullopt);
    NiPointer<NiSourceTexture>* pspBase;
    bool bConstructed = false;
    if (BSFaceGenManager::pInstance) {
        pspBase = &BSFaceGenManager::pInstance->spDefaultBaseMod;
    }
    else {
        pspBase = ::new (&spNull) NiPointer<NiSourceTexture>(static_cast<NiSourceTexture*>(nullptr));
        bConstructed = true;
    }
    NiSourceTexture* pTexture = pspBase->m_pObject;
    if (bConstructed) {
        spNull.~NiPointer<NiSourceTexture>();
    }
    return pTexture;
}

void BSFaceGenManager::OffsetFaceGenCoord(
    const FR2MatrixVTC<float> (*apCopyCoordOld)[2][2],
    const FR2MatrixVTC<float> (*apCopyCoordNew)[2][2],
    FR2MatrixVTC<float> (*apCopyToCoord)[2][2],
    bool abNoTextureOffset
) {
    int32_t a2;
    if ((apCopyCoordOld) && (apCopyCoordNew) && (apCopyToCoord)) {
        for (int32_t etype = 0; etype < 2; ++etype) {
            for (int32_t esymm = 0; esymm < 2; ++esymm) {
                uint32_t irows = (*apCopyCoordOld)[etype][esymm].nrows;
                uint32_t icols = (*apCopyCoordOld)[etype][esymm].ncols;
                if (irows && icols) {
                    (*apCopyToCoord)[etype][esymm].resize(static_cast<int32_t>(irows), static_cast<int32_t>(icols));
                    if (etype == 1 && abNoTextureOffset) {
                        (*apCopyToCoord)[etype][esymm].operator=((*apCopyCoordOld)[etype][esymm]);
                    }
                    else {
                        uint32_t isize = icols * irows;
                        (*apCopyToCoord)[etype][esymm].data.clear();
                        for (uint32_t i = 0; i < isize; ++i) {
                            // The const coordinates are read through the same non-const operator[] body.
                            float f1 = const_cast<FloatVector&>((*apCopyCoordOld)[etype][esymm].data)[i];
                            float f2 = const_cast<FloatVector&>((*apCopyCoordNew)[etype][esymm].data)[i];
                            *reinterpret_cast<float*>(&a2) = f2 - f1;
                            (*apCopyToCoord)[etype][esymm].data.push_back(*reinterpret_cast<float*>(&a2));
                        }
                    }
                }
                else {
                    (*apCopyToCoord)[etype][esymm].resize(0, 0);
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
    if ((apCoordToScale) && (!(afScaleAmount < 0.0) && !(afScaleAmount > 1.0))) {
        for (int32_t i = 0; i < 2; ++i) {
            for (int32_t j = 0; j < 2; ++j) {
                if (((*apCoordToScale)[i][j].nrows) && ((*apCoordToScale)[i][j].ncols) &&
                    (!i || ((i == 1 && abNoTextureScale) || !abNoTextureScale))) {
                    (*apCoordToScale)[i][j].operator*=(afScaleAmount);
                }
            }
        }
    }
}

const char* BSFaceGenManager::GetAsNIFFile(BSStringT<char>& aOutFilename, const char* apModel) {
    char cDest[264];
    char apDest[264];
    int32_t pDot = 0;
    if (!apModel) {
        return nullptr;
    }
    BSstrcpy(cDest, 260, apModel);
    char* pextension = std::strrchr(cDest, 46);
    if (!pextension) {
        return nullptr;
    }
    *pextension = 0;
    BSsprintf(apDest, 260u, "%s.nif", cDest);
    aOutFilename.Set(apDest, 0u);
    return aOutFilename.pString;
}

const char* BSFaceGenManager::GetAsNIFFileLowLOD(BSStringT<char>& aOutFilename, const char* apModel) {
    BSHash DirectoryHash;
    BSHash FileNameHash;
    BSStringT<char> pcheckFilename;
    char* pexists = nullptr;
    if (apModel) {
        char cDest[264];
        BSstrcpy(cDest, 260, apModel);
        pexists = std::strchr(cDest, 95);
        if (pexists) {
            aOutFilename.Set(cDest, 0);
            return apModel;
        }

        char* pextension = std::strrchr(cDest, 46);
        if (pextension) {
            char cFormattedDest[264];
            *pextension = 0;
            BSsprintf(cFormattedDest, 260, "%s_50.NIF", cDest);
            aOutFilename.Set(cFormattedDest, 0);
            pcheckFilename.SPrintF("Meshes\\%s", aOutFilename.pString);
            BSHash::MakeDirAndFile(pcheckFilename.pString, DirectoryHash, FileNameHash);
            if (ArchiveManager::GetFileEntryForFileFromAllArchives(
                    ARCHIVE_TYPE_INDEX_MESHES, DirectoryHash, FileNameHash, pcheckFilename.pString
                )) {
                char* result = aOutFilename.pString;
                return result;
            }

            return nullptr;
        }

        return apModel;
    }

    return nullptr;
}

const char* BSFaceGenManager::GetAsNIFFileHighLOD(BSStringT<char>& aOutFilename, const char* apModel) {
    char Dst[264];
    char apDest[264];
    int32_t v5 = 0;
    if (!apModel) {
        return nullptr;
    }
    BSstrcpy(Dst, 260, apModel);
    char* pextension = std::strrchr(Dst, 95);
    if (!pextension) {
        return apModel;
    }
    *pextension = 0;
    BSsprintf(apDest, 260u, "%s.nif", Dst);
    aOutFilename.Set(apDest, 0u);
    return aOutFilename.pString;
}

bool BSFaceGenManager::CreateFaceGenHead(
    BSFaceGenNiNode*& apOutNodeBiped,
    BSFaceGenNiNode*& apOutNodeSkinned,
    FaceGenNpcData& arNpcData,
    bool abForceNoSkinning
) {
    XFNV_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    BSHash v43;
    BSHash v44;
    NiMatrix3 matrot;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp", 2632
    );
    BSFaceGenModel* pmodelFG = nullptr;
    NiPointer<NiSourceTexture> sptexBase(static_cast<NiSourceTexture*>(nullptr));
    NiPointer<NiTriBasedGeom> spshape(static_cast<NiTriBasedGeom*>(nullptr));
    NiTriBasedGeom* pshapeHead = nullptr;
    NiTriBasedGeom* pshapeEars = nullptr;
    BSStringT<char> filename;
    BSStringT<char> fileEGM;
    BSStringT<char> fileTRI;
    BSStringT<char> fileEGT;
    BSStringT<char> fileFIM;
    BSStringT<char> fileLowLOD;
    matrot.MakeYRotation(-1.5707964f);
    for (uint32_t ipart = 0; ipart < 8; ++ipart) {
        if ((arNpcData.pModel.m_usSize) && (arNpcData.pModel.operator[](ipart)) &&
            (arNpcData.pModel.operator[](ipart)->GetModelLength()) && (arNpcData.pTexture.m_usSize) &&
            (arNpcData.pTexture.operator[](ipart)) && (arNpcData.pTexture.operator[](ipart)->GetTextureNameLength())) {
            TESModel* v32 = arNpcData.pModel.operator[](ipart);
            filename.SPrintF("Meshes\\%s", v32->GetModel());
            if (arNpcData.eLod == BS_FACEGEN_LOD_LOW && !ipart) {
                BSFaceGenManager::GetAsNIFFileLowLOD(fileLowLOD, filename.pString);
                BSFileEntry* FileEntryForFileFromAllArchives = nullptr;
                if (fileLowLOD.pString) {
                    BSHash::MakeDirAndFile(fileLowLOD.pString, v43, v44);
                    FileEntryForFileFromAllArchives = ArchiveManager::GetFileEntryForFileFromAllArchives(
                        ARCHIVE_TYPE_INDEX_MESHES, v43, v44, fileLowLOD.pString
                    );
                }
                if (FileEntryForFileFromAllArchives) {
                    BSFaceGenManager::GetAsNIFFileLowLOD(filename, filename.pString);
                }
            }
            const char* pegtFileName = nullptr;
            switch (ipart) {
                case 0u: {
                    if (arNpcData.bPlayer || BSFaceGenManager::bLoadFaceGenHeadEGTFiles.Bool()) {
                        pegtFileName = BSFaceGenManager::GetAsEGTFile(fileEGT, filename.pString);
                    }
                    const char* v19 = pegtFileName;
                    const char* AsTRIFile = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
                    char* v11 = filename.pString;
                    const char* AsEGMFile = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
                    pmodelFG =
                        BSFaceGenManager::GetFaceGenModel(AsEGMFile, v11, AsTRIFile, v19, true, HAIR_PART_NONE, false);
                    break;
                }
                case 1u: {
                    const char* AsEGTFile = BSFaceGenManager::GetAsEGTFile(fileEGT, filename.pString);
                    const char* v16 = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
                    char* v12 = filename.pString;
                    const char* v5 = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
                    pmodelFG = BSFaceGenManager::GetFaceGenModel(v5, v12, v16, AsEGTFile, true, HAIR_PART_NONE, false);
                    break;
                }
                case 2u:
                case 3u:
                case 4u:
                case 5u: {
                    const char* v17 = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
                    char* v13 = filename.pString;
                    const char* v6 = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
                    pmodelFG = BSFaceGenManager::GetFaceGenModel(v6, v13, v17, nullptr, true, HAIR_PART_NONE, false);
                    break;
                }
                default: {
                    break;
                }
            }
            if ((pmodelFG) && (pmodelFG->ApplyCoordinateToNewMesh(&(&arNpcData)->FaceCoord, spshape)) &&
                (spshape.m_pObject)) {
                int32_t v27;
                NiFixedString* a2 =
                    ::new (static_cast<void*>(&apString)) NiFixedString(arNpcData.pHeadPartNodeName.operator[](ipart));
                spshape.m_pObject->SetName(*a2);
                apString.~NiFixedString();
                if (!ipart) {
                    pshapeHead = spshape.m_pObject;
                }
                if (ipart == 1) {
                    pshapeEars = spshape.m_pObject;
                }
                filename.SPrintF("Textures\\%s", arNpcData.pTexture.operator[](ipart)->GetTextureName());
                char* v21 = filename.pString;
                TES::pInstance->CreateTextureImage(v21, sptexBase, false, false);
                if (sptexBase.m_pObject) {
                    int32_t v30;
                    NiShadeProperty* pprop = static_cast<NiShadeProperty*>(spshape.m_pObject->GetProperty(3));
                    if (pprop) {
                        int32_t bIsLightingShader = pprop->iShaderPropertyType >= 8 && pprop->iShaderPropertyType <= 12;
                        v30 = bIsLightingShader;
                    }
                    else {
                        v30 = 0;
                    }
                    BSShaderPPLightingProperty* pshaderprop =
                        v30 ? static_cast<BSShaderPPLightingProperty*>(pprop) : nullptr;
                    if (pshaderprop) {
                        NiSourceTexture* afAngle = sptexBase.m_pObject;
                        pshaderprop->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, afAngle);
                    }
                }
                if (ipart == 4 || ipart == 3) {
                    if (spshape.m_pObject->GetProperty(0)) {
                        spshape.m_pObject->RemoveProperty(0);
                    }
                    NiAlphaProperty* HairTeethAlphaProperty = BSFaceGenManager::GetHairTeethAlphaProperty();
                    spshape.m_pObject->AttachProperty(HairTeethAlphaProperty);
                }
                if (ipart <= 1) {
                    int32_t Type = NiMaterialProperty::GetType();
                    NiProperty* pprop_2 = spshape.m_pObject->GetProperty(Type);
                    NiMaterialProperty* pmatprop = static_cast<NiMaterialProperty*>(pprop_2);
                    if (pprop_2) {
                        pmatprop->SetName(*FixedStrings::QSkin());
                    }
                    else {
                        NiMaterialProperty* pmatprop_2;
                        NiMaterialProperty* v34 =
                            static_cast<NiMaterialProperty*>(NiMemObject::operator new(sizeof(NiMaterialProperty)));
                        if (v34) {
                            pmatprop_2 = ::new (v34) NiMaterialProperty;
                        }
                        else {
                            pmatprop_2 = nullptr;
                        }
                        NiMaterialProperty* v35 = pmatprop_2;
                        pmatprop = pmatprop_2;
                        pmatprop_2->SetName(*FixedStrings::QSkin());
                        spshape.m_pObject->AttachProperty(pmatprop);
                    }
                }
                if (spshape.m_pObject->GetSkinInstance()) {
                    if (abForceNoSkinning) {
                        spshape.m_pObject->SetSkinInstance(nullptr);
                        spshape.m_pObject->SetTranslate(NiPoint3_ZERO);
                        spshape.m_pObject->SetRotate(matrot);
                        apOutNodeBiped->AttachChild(spshape.m_pObject, 1);
                        NiProperty* pshadeprop = spshape.m_pObject->GetProperty(3);
                        BSShaderProperty* pshprop = static_cast<BSShaderProperty*>(
                            NiObject::DynamicCastStatic(&BSShaderProperty::ms_RTTI, pshadeprop)
                        );
                        if (pshprop) {
                            pshprop->SetFlag(1u, 0);
                        }
                    }
                    else {
                        apOutNodeSkinned->AttachChild(spshape.m_pObject, 1);
                    }
                }
                else {
                    spshape.m_pObject->SetRotate(matrot);
                    apOutNodeBiped->AttachChild(spshape.m_pObject, 1);
                }
                NiProperty* pshadeprop_2 = spshape.m_pObject->GetProperty(3);
                BSShaderProperty* pshprop_2 = static_cast<BSShaderProperty*>(pshadeprop_2);
                if (pshadeprop_2) {
                    int32_t v26 = pshprop_2->iShaderPropertyType >= 8 && pshprop_2->iShaderPropertyType <= 12;
                    v27 = v26;
                }
                else {
                    v27 = 0;
                }
                BSShaderPPLightingProperty* pppprop =
                    v27 ? static_cast<BSShaderPPLightingProperty*>(pshprop_2) : nullptr;
                if (pppprop) {
                    pppprop->UseDefaultNormalMap();
                }
                pmodelFG = nullptr;
                sptexBase.operator=(static_cast<NiSourceTexture*>(nullptr));
                spshape.operator=(static_cast<NiTriBasedGeom*>(nullptr));
            }
        }
    }
    if ((BSFaceGenManager::GetFixNormals()) && (pshapeHead) && (pshapeEars)) {
        FixNormals(pshapeHead, pshapeEars, 0.1f, false, false);
    }

    if (arNpcData.pHair) {
        BSFaceGenManager::AttachHairToHead(apOutNodeBiped, apOutNodeSkinned, arNpcData, abForceNoSkinning);
    }
    if ((arNpcData.pLeftEye) && (arNpcData.pRightEye)) {
        BSFaceGenManager::AttachEyesToHead(apOutNodeBiped, apOutNodeSkinned, arNpcData, abForceNoSkinning);
    }

    for (BGSHeadPart*& rpItem : arNpcData.Accessories.ItemsUntilNull()) {
        TESModelTextureSwap* v25 = static_cast<TESModelTextureSwap*>(rpItem);
        filename.SPrintF("Meshes\\%s", v25->GetModel());
        const char* v18 = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
        char* v14 = filename.pString;
        const char* v9 = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
        pmodelFG = BSFaceGenManager::GetFaceGenModel(v9, v14, v18, nullptr, true, HAIR_PART_NONE, true);
        if ((pmodelFG) && (pmodelFG->ApplyCoordinateToNewMesh(&(&arNpcData)->FaceCoord, spshape)) &&
            (spshape.m_pObject)) {
            spshape.m_pObject->SetRotate(matrot);
            apOutNodeBiped->AttachChild(spshape.m_pObject, 1);
            NiTriBasedGeom* afAnglea = spshape.m_pObject;
            rpItem->SwapTextures(afAnglea);
            NiFixedString v33("FaceGenAccessory");
            NiFixedString* kName = &v33;
            spshape.m_pObject->SetName(*kName);
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
    XFNV_INLINED_LOCAL(NiFixedString, v21, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiPointer<NiTriBasedGeom>, spshape, std::nullopt, std::nullopt);
    NiMatrix3 matrot;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\FaceGen\\BSFaceGenManager.cpp", 2983
    );
    BSStringT<char> filename;
    BSStringT<char> fileEGM;
    BSStringT<char> fileTRI;
    BSStringT<char> fileEGT;
    BSStringT<char> fileFIM;
    NiPointer<NiSourceTexture> sptexBase(static_cast<NiSourceTexture*>(nullptr));
    ::new (&spshape) NiPointer<ActorPathingMessageQueue>(static_cast<ActorPathingMessageQueue*>(nullptr));
    BSFaceGenModel* pmodelFG = nullptr;
    matrot.MakeYRotation(-1.5707964f);
    if (aNpcData.pLeftEye) {
        filename.SPrintF("Meshes\\%s", aNpcData.pLeftEye->GetModel());
        const char* AsTRIFile = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
        char* v7 = filename.pString;
        const char* AsEGMFile = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
        pmodelFG = BSFaceGenManager::GetFaceGenModel(AsEGMFile, v7, AsTRIFile, nullptr, true, HAIR_PART_NONE, false);
        if (pmodelFG) {
            pmodelFG->ApplyCoordinateToNewMesh(&(&aNpcData)->FaceCoord, spshape);
        }
        if (spshape.m_pObject) {
            NiFixedString* a2 = ::new (static_cast<void*>(&apString)) NiFixedString("FaceGenEyeLeft");
            spshape.m_pObject->SetName(*a2);
            apString.~NiFixedString();
            if (aNpcData.pEyes) {
                filename.SPrintF("Textures\\%s", aNpcData.pEyes->GetTextureName());
                char* v11 = filename.pString;
                TES::pInstance->CreateTextureImage(v11, sptexBase, false, false);
            }
            else {
                TES::pInstance->CreateTextureImage(
                    "Textures\\Characters\\Eyes\\EyeDefault.dds", sptexBase, false, false
                );
            }
            if (sptexBase.m_pObject) {
                int32_t v19;
                NiShadeProperty* pprop = static_cast<NiShadeProperty*>(spshape.m_pObject->GetProperty(3));
                if (pprop) {
                    int32_t v18 = pprop->iShaderPropertyType >= 8 && pprop->iShaderPropertyType <= 12;
                    v19 = v18;
                }
                else {
                    v19 = 0;
                }
                BSShaderPPLightingProperty* pshaderprop =
                    v19 ? static_cast<BSShaderPPLightingProperty*>(pprop) : nullptr;
                if (pshaderprop) {
                    NiSourceTexture* afAngle = sptexBase.m_pObject;
                    pshaderprop->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, afAngle);
                }
            }
            if (spshape.m_pObject->GetSkinInstance()) {
                if (abForceNoSkinning) {
                    spshape.m_pObject->SetSkinInstance(nullptr);
                    spshape.m_pObject->SetTranslate(NiPoint3_ZERO);
                    spshape.m_pObject->SetRotate(matrot);
                    apOutNodeBiped->AttachChild(spshape.m_pObject, 1);
                }
                else {
                    apOutNodeSkinned->AttachChild(spshape.m_pObject, 1);
                }
            }
            else {
                spshape.m_pObject->SetRotate(matrot);
                apOutNodeBiped->AttachChild(spshape.m_pObject, 1);
            }
        }
        pmodelFG = nullptr;
        spshape.operator=(static_cast<NiTriBasedGeom*>(nullptr));
        sptexBase.operator=(static_cast<NiSourceTexture*>(nullptr));
    }
    if (aNpcData.pRightEye) {
        const char* v5 = aNpcData.pRightEye->GetModel();
        filename.SPrintF("Meshes\\%s", v5);
        const char* v10 = BSFaceGenManager::GetAsTRIFile(fileTRI, filename.pString, HAIR_PART_NONE);
        char* v8 = filename.pString;
        const char* v6 = BSFaceGenManager::GetAsEGMFile(fileEGM, filename.pString, HAIR_PART_NONE);
        pmodelFG = BSFaceGenManager::GetFaceGenModel(v6, v8, v10, nullptr, true, HAIR_PART_NONE, false);
        if (pmodelFG) {
            pmodelFG->ApplyCoordinateToNewMesh(&(&aNpcData)->FaceCoord, spshape);
        }
        if (spshape.m_pObject) {
            NiFixedString* kName = ::new (static_cast<void*>(&v21)) NiFixedString("FaceGenEyeRight");
            spshape.m_pObject->SetName(*kName);
            v21.~NiFixedString();
            if (aNpcData.pEyes) {
                filename.SPrintF("Textures\\%s", aNpcData.pEyes->GetTextureName());
                char* v12 = filename.pString;
                TES::pInstance->CreateTextureImage(v12, sptexBase, false, false);
            }
            else {
                TES::pInstance->CreateTextureImage(
                    "Textures\\Characters\\Eyes\\EyeDefault.dds", sptexBase, false, false
                );
            }
            if (sptexBase.m_pObject) {
                int32_t v16;
                NiShadeProperty* pprop_2 = static_cast<NiShadeProperty*>(spshape.m_pObject->GetProperty(3));
                if (pprop_2) {
                    int32_t v15 = pprop_2->iShaderPropertyType >= 8 && pprop_2->iShaderPropertyType <= 12;
                    v16 = v15;
                }
                else {
                    v16 = 0;
                }
                BSShaderPPLightingProperty* pshaderprop_2 =
                    v16 ? static_cast<BSShaderPPLightingProperty*>(pprop_2) : nullptr;
                if (pshaderprop_2) {
                    NiSourceTexture* afAnglea = sptexBase.m_pObject;
                    pshaderprop_2->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, afAnglea);
                }
            }
            if (spshape.m_pObject->GetSkinInstance()) {
                if (abForceNoSkinning) {
                    spshape.m_pObject->SetSkinInstance(nullptr);
                    spshape.m_pObject->SetTranslate(NiPoint3_ZERO);
                    spshape.m_pObject->SetRotate(matrot);
                    apOutNodeBiped->AttachChild(spshape.m_pObject, 1);
                }
                else {
                    apOutNodeSkinned->AttachChild(spshape.m_pObject, 1);
                }
            }
            else {
                spshape.m_pObject->SetRotate(matrot);
                apOutNodeBiped->AttachChild(spshape.m_pObject, 1);
            }
        }
        pmodelFG = nullptr;
        spshape.operator=(static_cast<NiTriBasedGeom*>(nullptr));
        sptexBase.operator=(static_cast<NiSourceTexture*>(nullptr));
    }
    spshape.~NiPointer<NiTriBasedGeom>();
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
        NiGeometryDataIteratorNiPoint3 v13 = arIterator;
        int32_t i = 1;
        while (i < aiVertices) {
            v13.Get_(kCurrent);
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
            v13.Next();
        }
        kCenter = 0.5f * (kMin + kMax);
        NiGeometryDataIteratorNiPoint3 v12 = arIterator;
        i = 0;
        while (i < aiVertices) {
            v12.Get_(kCurrent);
            NiPoint3 diff = kCurrent - kCenter;
            float fLengthSqr = diff.Dot(diff);
            if (fRadiusSqr < static_cast<double>(fLengthSqr)) {
                fRadiusSqr = fLengthSqr;
            }
            ++i;
            v12.Next();
        }
        arBound.SetCenter(kCenter);
        float fRadius = NiSqrt(fRadiusSqr);
        arBound.SetRadius(fRadius);
    }
    else {
        arBound.SetCenter(NiPoint3_ZERO);
        arBound.SetRadius(0.0f);
    }
}

char* BSFaceGenManager::GetCustomMorphText(int32_t a1) {
    return (&BSFaceGenManager::pFaceGenCustomMorphText)[a1];
}

float BSFaceGenManager::GetFaceCoordValue(
    const FR2MatrixVTC<float> (*apCoord)[2][2],
    FanTypeE aeFanType,
    FanSymmE aeFanSymm,
    uint32_t aiCtrlID
) {
    FR2MatrixVTC<float> apResult;
    float fcurrpos = 0.0f;
    if (apCoord) {
        BSFaceGenManager* Instance = BSFaceGenManager::GetInstance();
        if (aiCtrlID < Instance->LinCtrls.grp[aeFanType][aeFanSymm].size()) {
            BSFaceGenManager* v5 = BSFaceGenManager::GetInstance();
            FanLinCtlS* v6 = &v5->LinCtrls.grp[aeFanType][aeFanSymm][aiCtrlID];
            FR2MatrixVTC<float>* v10 = v6->ctl.Mul(&apResult, (*apCoord)[aeFanType][aeFanSymm]);
            FR2MatrixVTC<float>::RowT row = v10->operator[](0u);
            fcurrpos = row[0u];
        }
    }
    return fcurrpos;
}

void BSFaceGenManager::SetFaceCoordValue(
    FR2MatrixVTC<float> (*apCoord)[2][2],
    FanTypeE aeFanType,
    FanSymmE aeFanSymm,
    uint32_t aiCtrlID,
    float afValue
) {
    FR2MatrixVTC<float> v11;
    FR2MatrixVTC<float> apResult;
    if (apCoord) {
        BSFaceGenManager* Instance = BSFaceGenManager::GetInstance();
        if (aiCtrlID < Instance->LinCtrls.grp[aeFanType][aeFanSymm].size()) {
            float fcurrpos = BSFaceGenManager::GetFaceCoordValue(apCoord, aeFanType, aeFanSymm, aiCtrlID);
            float fdelta = afValue - fcurrpos;
            BSFaceGenManager* v6 = BSFaceGenManager::GetInstance();
            FanLinCtlS* v7 = &v6->LinCtrls.grp[aeFanType][aeFanSymm][aiCtrlID];
            FR2MatrixVTC<float>* v10 = v7->ctl.transpose(&apResult);
            FR2MatrixVTC<float>* m = v10->MulScalar(&v11, fdelta);
            (*apCoord)[aeFanType][aeFanSymm].operator+=(*m);
        }
    }
}

int32_t BSFaceGenManager::GetMaxControlID(int32_t a1, int32_t a2) {
    BSFaceGenManager* singleton = BSFaceGenManager::GetInstance();
    return singleton->LinCtrls.grp[a1][a2].size() - 1;
}

void BSFaceGenManager::RandomizeFaceGenetic(
    float afGeneticVariation,
    FR2MatrixVTC<float> (*apSourceCoord)[2][2],
    FR2MatrixVTC<float> (*apOutCoord)[2][2]
) {
    if ((apOutCoord) && (apSourceCoord)) {
        BSFaceGenManager* Instance = BSFaceGenManager::GetInstance();
        Instance->GenCtrls.geneticRandom(afGeneticVariation, apSourceCoord, apOutCoord);
    }
}

FanLinCtlGrpS::FanLinCtlGrpS() {}

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

// Without mesh data the morph is read through a null temporary smart pointer, as the game does.
BSFaceGenMorphData* BSFaceGenModel::GetMorph() {
    if (pMeshData) {
        return pMeshData->spMorph.operator->();
    }
    NiPointer<BSFaceGenMorphData> spNull(nullptr);
    BSFaceGenMorphData* pMorph = spNull.operator->();
    return pMorph;
}

void NiAVObject::DetachProperty(NiProperty* pkProperty) {
    NiPointer<NiProperty> spProperty(pkProperty);
    { NiPointer<NiProperty> spRemoved = m_kPropertyList.Remove(spProperty); }
}

void NiGeometry::SetShader(BSShader* pkShader) {
    this->m_spShader = pkShader;
}

void NiPixelData::SetNoConvert(bool abTF) {
    bNoConvert = abTF;
}

NiSemaphore::NiSemaphore(int32_t aiCount, int32_t aiMaxCount)
  : m_iCount(aiCount), m_iMaxCount(aiMaxCount), m_hSemaphore(new BSSystemSemaphore(m_iCount, m_iMaxCount)) {}

NiSemaphore::~NiSemaphore() {
    delete m_hSemaphore;
}
