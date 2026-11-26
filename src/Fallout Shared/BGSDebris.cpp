#include "Fallout Shared/BGSDebris.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TempEffects/BSTempEffectDebris.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

BGSDebrisData::BGSDebrisData() : cPercentage(10), cFlags(0), pFileName(nullptr), TextureList() {}

BGSDebrisData::~BGSDebrisData() {}

bool BGSDebrisData::GetHasCollisionData() {
    return cFlags & FLAG_HAS_COLLISION_DATA;
}

void BGSDebrisData::SetFileName(const char* apFileName) {
    if (pFileName) {
        ourFree(pFileName);
    }
    pFileName = nullptr;
    if (apFileName) {
        int32_t iFilenameLen = BSstrlen(apFileName) + 1;
        pFileName = static_cast<char*>(ourMalloc(iFilenameLen));
        BSstrcpy(pFileName, iFilenameLen, apFileName);
    }
}

int32_t DebrisSortFunction(BGSDebrisData* apDataA, BGSDebrisData* apDataB) {
    if (apDataA->cPercentage > apDataB->cPercentage) {
        return -1;
    }
    if (apDataA->cPercentage >= apDataB->cPercentage) {
        return BSstrcmp(apDataA->pFileName, apDataB->pFileName);
    }
    return 1;
}

BGSDebris::BGSDebris() : TESForm(), BGSPreloadable(), DataList() {
    SetFormType(0x52u);
    TESForm::InitializeDataComponent();
}

BGSDebris::~BGSDebris() {
    BGSDebris::ClearData();
}

void BGSDebris::ClearData() {
    while (!DataList.IsEmpty()) {
        BGSDebrisData* pData = DataList.m_item;
        if (pData) {
            ourFree(pData->pFileName);
            delete pData;
        }
        DataList.RemoveHead();
    }
}

// Reads the debris models (DATA) and their texture lists (MODT).
bool BGSDebris::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != DEBR_ID) {
        return false;
    }
    ClearData();
    LoadForm(apFile);
    SetInitialized(false);
    BGSDebrisData* pLastData = nullptr;
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk == OBND_ID) {
            LoadObjectBound(apFile);
        }
        else if (uiChunk == DATA_ID) {
            uint32_t uiSize = apFile->GetChunkSize();
            char* pBlock = static_cast<char*>(ourMalloc(uiSize));
            apFile->GetChunkData(pBlock, uiSize);
            char* pCursor = pBlock;
            char* pEnd = pBlock + uiSize;
            if (pCursor < pEnd) {
                char cPercentage = *pCursor++;
                char* pFileName = pCursor;
                pCursor += BSstrlen(pCursor) + 1;
                char cFlags = 0;
                if (pCursor < pEnd) {
                    cFlags = *pCursor++;
                }
                pLastData = AddDebrisData(cPercentage, pFileName, cFlags);
            }
            ourFree(pBlock);
        }
        else if (uiChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
            apFile->GetChunkData(pBuffer, 0x200);
            SetFormEditorID(pBuffer);
        }
        else if (uiChunk == MODT_ID) {
            if (pLastData) {
                pLastData->TextureList.Load(apFile, this, pLastData->pFileName);
            }
            else {
                Warning(
                    "MODELS: No last debris data when loading texture list for debris '%s' (%08X)",
                    GetFormEditorID(),
                    GetFormID()
                );
                TESTextureList kTextureList;
                kTextureList.Load(apFile, this, "UNKNOWN");
            }
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSDebris::Save() {
    StartForm();
    for (BGSDebrisData* pdebrisData : *GetData()) {
        if (pdebrisData && pdebrisData->pFileName && BSstrlen(pdebrisData->pFileName)) {
            uint32_t uiDataSize = BSstrlen(pdebrisData->pFileName) + 3;
            char* pData = static_cast<char*>(ourMalloc(uiDataSize));
            *pData = pdebrisData->cPercentage;
            BSstrcpy(pData + 1, uiDataSize, pdebrisData->pFileName);
            pData[BSstrlen(pdebrisData->pFileName) + 2] = pdebrisData->cFlags;
            TESForm::AddChunkArray(DATA_ID, pData, uiDataSize);
            ourFree(pData);
        }
    }
    CloseForm();
}

void BGSDebris::Copy(TESForm* apCopy) {
    BGSDebris* pcopy = dynamic_cast<BGSDebris*>(apCopy);
    if (pcopy) {
        ClearData();
        CopyAllComponents(pcopy);
        for (BGSDebrisData* pdebrisData : *pcopy->GetData()) {
            if (pdebrisData) {
                AddDebrisData(pdebrisData->cPercentage, pdebrisData->pFileName, pdebrisData->cFlags);
            }
        }
    }
}

bool BGSDebris::Compare(TESForm* apCompare) {
    BGSDebris* pcomp = dynamic_cast<BGSDebris*>(apCompare);
    if (!pcomp) {
        return true;
    }
    BSSimpleList<BGSDebrisData*>* pcompareList = pcomp->GetData();
    int32_t iCompareCount = pcompareList->ItemsInList();
    if (iCompareCount != DataList.ItemsInList()) {
        return true;
    }
    BSSimpleList<BGSDebrisData*>* plocalList = &DataList;
    while (pcompareList) {
        BGSDebrisData* pcompareData = pcompareList->m_item;
        BGSDebrisData* plocalData = plocalList->m_item;
        if ((!pcompareData && plocalData) || (pcompareData && !plocalData)) {
            return true;
        }
        if (pcompareData && plocalData) {
            if (pcompareData->cPercentage != plocalData->cPercentage) {
                return true;
            }
            if (BSstrcmp(pcompareData->pFileName, plocalData->pFileName)) {
                return true;
            }
            if (pcompareData->cFlags != plocalData->cFlags) {
                return true;
            }
        }
        plocalList = plocalList->m_pkNext;
        pcompareList = pcompareList->m_pkNext;
    }
    return false;
}

// Queues each debris model that has a file name, with its texture list.
void BGSDebris::QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    for (BSSimpleList<BGSDebrisData*>* pNode = GetData(); pNode; pNode = pNode->GetNext()) {
        BGSDebrisData* pData = pNode->GetItem();
        if (pData && pData->pFileName && BSstrlen(pData->pFileName)) {
            pModelLoader->QueueModel(
                pData->pFileName,
                &pData->TextureList,
                aePriority,
                apParent,
                LOD_MULT_OBJECTS,
                true,
                false,
                false
            );
        }
    }
}

BGSDebrisData* BGSDebris::AddDebrisData(int8_t acPercentage, const char* apFileName, int8_t acFlags) {
    if (!apFileName || !acPercentage) {
        return nullptr;
    }
    if (!BSstrlen(apFileName)) {
        return nullptr;
    }
    BGSDebrisData* pData = new BGSDebrisData;
    pData->cPercentage = acPercentage;
    pData->SetFileName(apFileName);
    pData->cFlags = acFlags;
    AddDebrisData(pData);
    return pData;
}

BSSimpleList<BGSDebrisData*>* BGSDebris::GetData() {
    return &DataList;
}

void BGSDebris::SpawnDebris(TESObjectCELL* apCell, const NiPoint3& arLocation, uint32_t aiNumParticles, float afScale) {
    NiPoint3 velocity;
    NiPoint3 debrisVector;
    if (apCell) {
        uint32_t uiNumParticlesSpawned = 0;
        for (BGSDebrisData* pdebrisData : *GetData()) {
            if (pdebrisData && pdebrisData->pFileName && pdebrisData->cPercentage) {
                uint32_t uiNumParticles = aiNumParticles * pdebrisData->cPercentage / 100;
                if (!uiNumParticles) {
                    uiNumParticles = uiNumParticlesSpawned < aiNumParticles;
                }
                for (uint32_t i = 0; i < uiNumParticles; ++i) {
                    debrisVector.x = RandomFloat(-1.0f, 1.0f);
                    debrisVector.y = RandomFloat(-1.0f, 1.0f);
                    debrisVector.z = RandomFloat(-1.0f, 1.0f);
                    debrisVector.Unitize();
                    if (pdebrisData->GetHasCollisionData()) {
                        velocity = NiPoint3::ZERO;
                        new BSTempEffectDebris(
                            apCell,
                            90.0f,
                            pdebrisData->pFileName,
                            nullptr,
                            arLocation,
                            debrisVector,
                            velocity,
                            velocity,
                            afScale,
                            false,
                            false
                        );
                    }
                    else {
                        float fParticleScale = afScale * 10.0;
                        BSTempEffectParticle::Spawn(
                            apCell,
                            10.0f,
                            pdebrisData->pFileName,
                            debrisVector,
                            arLocation,
                            fParticleScale,
                            7u,
                            nullptr
                        );
                    }
                    ++uiNumParticlesSpawned;
                }
            }
        }
    }
}

bool BGSDebris::AddDebrisData(BGSDebrisData* apDebrisData) {
    if (!apDebrisData || !apDebrisData->cPercentage || !apDebrisData->pFileName || !BSstrlen(apDebrisData->pFileName)) {
        return false;
    }
    DataList.Insert(apDebrisData, &DebrisSortFunction);
    return true;
}

BGSPreloadable::BGSPreloadable() {}
