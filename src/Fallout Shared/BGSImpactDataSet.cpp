#include "Fallout Shared/BGSImpactDataSet.h"

#include "BSCore/BSMap.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// Reads the impact data set's chunks. The DATA chunk holds one form ID per material, which a big-endian file stores
// swapped and which are swapped back in place.
bool BGSImpactDataSet::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != IPDS_ID) {
        return false;
    }
    this->LoadForm(apFile);
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case DATA_ID: apFile->GetChunkData(ppImpactData, 0x30u);
#if XFNV_64BIT
                // The file keeps a 4-byte form ID for each material, resolved by InitItem.
                TESFile::WidenFormIDRecords(ppImpactData, IMP_MAT_COUNT, 4, sizeof(BGSImpactData*), {{0, 0}});
#endif
                if (apFile->QEndian()) {
                    for (BGSImpactData*& pImpactData : ppImpactData) {
                        // Until InitItem resolves it, each slot holds an untyped form ID, which is swapped as a 32-bit
                        // value.
                        ENDIAN(reinterpret_cast<uint32_t&>(pImpactData), false);
                    }
                }
                break;
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200u);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void ImpactSwap::Load(TESFile* apFile) {
    ImpactSwap::IMPACT_SWAP_DATA data;
    CHUNK_ID TESChunk = apFile->GetTESChunk();
    if (TESChunk == IMPF_ID) {
        apFile->GetChunkData(pAltFootstepMaterialNames, 300u);
    }
    else {
        if (TESChunk == IMPS_ID) {
            ourMemset(&data, 0, sizeof(data));
            apFile->GetChunkData(&data, 12u);
#if XFNV_64BIT
            // The file's record is 12 bytes, with 4-byte form IDs at 4 and 8.
            TESFile::WidenFormIDRecords(
                &data, 1, 12, sizeof(data),
                {{4, offsetof(IMPACT_SWAP_DATA, pOldData)}, {8, offsetof(IMPACT_SWAP_DATA, pNewData)}}
            );
#endif
            if (apFile->QEndian()) {
                data.Endian();
            }
            // The map keeps the chunk's form IDs until InitItem resolves them.
            AddSwappedImpactData(data.eType, data.pOldData, data.pNewData);
        }
    }
}

BGSImpactDataSet::BGSImpactDataSet() {
    SetFormType(0x5Fu);
    BGSImpactDataSet::InitializeData();
}

BGSImpactDataSet::~BGSImpactDataSet() {}

void BGSImpactDataSet::InitializeData() {
    ourMemset(ppImpactData, 0, sizeof(ppImpactData));
}

// Writes the DATA chunk: the form ID of every material's impact data, 0 where none is set.
void BGSImpactDataSet::Save() {
    uint32_t aiFormIDs[IMP_MAT_COUNT];
    StartForm();
    for (int32_t i = 0; i < IMP_MAT_COUNT; ++i) {
        if (ppImpactData[i]) {
            aiFormIDs[i] = ppImpactData[i]->GetFormID();
        }
        else {
            aiFormIDs[i] = 0;
        }
    }
    AddChunkArray(DATA_ID, aiFormIDs, IMP_MAT_COUNT);
    CloseForm();
}

// Resolves the form ID each material's impact data slot holds into a BGSImpactData, in place, and warns about every
// one that does not resolve.
void BGSImpactDataSet::InitItem() {
    if (TESForm::GetInitialized()) {
        return;
    }
    for (int32_t i = 0; i < IMP_MAT_COUNT; ++i) {
        // Until it is resolved, the slot holds the form ID the file wrote.
        uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ppImpactData[i]));
        if (!uiID) {
            continue;
        }
        TESFile* pFile = TESForm::GetFile(-1);
        TESForm::AddCompileIndex(uiID, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(uiID);
        ppImpactData[i] = dynamic_cast<BGSImpactData*>(pForm);
        if (!ppImpactData[i]) {
            uint32_t uiFormID = TESForm::GetFormID();
            const char* pcEditorID = static_cast<TESForm*>(this)->GetFormEditorID();
            Warning(
                "MASTERFILE: Unable to find impact data (%08X) on impact data set %s (%08X).", uiID, pcEditorID,
                uiFormID
            );
        }
    }
    TESForm::SetInitialized(true);
}

void BGSImpactDataSet::Copy(TESForm* apCopy) {
    TESForm* v3 = dynamic_cast<BGSImpactDataSet*>(apCopy);
    if (v3) {
        CopyAllComponents(v3);
        ourMemcpy(ppImpactData, &v3[1].cFormType, sizeof(ppImpactData));
    }
}

bool BGSImpactDataSet::Compare(TESForm* apCompare) {
    TESForm* v4 = dynamic_cast<BGSImpactDataSet*>(apCompare);
    if (!v4) {
        return true;
    }
    if (CompareAllComponents(v4)) {
        return true;
    }
    return std::memcmp(ppImpactData, &v4[1].cFormType, sizeof(ppImpactData)) != 0;
}

void BGSImpactDataSet::QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    for (BGSImpactDataSet::IMPACT_MATERIAL_TYPE i = BGSImpactDataSet::IMP_MAT_STONE;
         i < BGSImpactDataSet::IMP_MAT_COUNT; i = static_cast<BGSImpactDataSet::IMPACT_MATERIAL_TYPE>(i + 1)) {
        BGSImpactData* pimpactData = GetImpactData(i);
        if (pimpactData) {
            if (pimpactData->GetModelLength()) {
                TESModel* apModel;
                if (pimpactData) {
                    apModel = static_cast<TESModel*>(pimpactData);
                }
                else {
                    apModel = nullptr;
                }
                pModelLoader->QueueModel(apModel, aePriority, apParent, LOD_MULT_OBJECTS, true, false, false);
            }
            BGSTextureSet* ptexset = pimpactData->pDecalTextureSet;
            if (ptexset) {
                ptexset->QueueTextureSet(aePriority, apParent, false);
            }
        }
    }
}

BGSImpactData* BGSImpactDataSet::GetImpactDataForMaterial(int32_t aeMaterial) {
    int32_t v2 = BGSImpactDataSet::GetImpactMaterialType(static_cast<HavokMaterialType>(aeMaterial));
    return GetImpactData(static_cast<BGSImpactDataSet::IMPACT_MATERIAL_TYPE>(v2));
}

BGSImpactData* BGSImpactDataSet::GetImpactData(BGSImpactDataSet::IMPACT_MATERIAL_TYPE aeType) {
    if (BGSImpactDataSet::pActiveImpactSwap) {
        return BGSImpactDataSet::pActiveImpactSwap->GetSwappedImpactData(aeType, ppImpactData[(int32_t)aeType]);
    }

    return ppImpactData[(int32_t)aeType];
}

void BGSImpactDataSet::ApplyImpactSwap(ImpactSwap* apSwap) {
    BGSImpactDataSet::pActiveImpactSwap = apSwap;
    for (int32_t a1 = 0; a1 < 10; ++a1) {
        if (apSwap) {
            const char* AltFootstepMaterialName =
                apSwap->GetAltFootstepMaterialName(static_cast<ImpactSwap::FST_MATERIALS>(a1));
            ImpactMixer::SetAltMaterialSoundName(a1, AltFootstepMaterialName);
        }
        else {
            ImpactMixer::SetAltMaterialSoundName(a1, nullptr);
        }
    }
    ImpactMixer::ReInitFootsteps();
}

void BGSImpactDataSet::CreateDefaultImpactDataSet() {
    BGSImpactDataSet* v1;
    void* v2;
    TESForm* formByNumericID = TESForm::GetFormByNumericID(0x276u);
    BGSImpactDataSet::pDefaultImpactDataSet = dynamic_cast<BGSImpactDataSet*>(formByNumericID);
    if (!BGSImpactDataSet::pDefaultImpactDataSet) {
        v2 = ourMalloc(sizeof(BGSImpactDataSet));
        if (v2) {
            v1 = ::new (v2) BGSImpactDataSet;
        }
        else {
            v1 = nullptr;
        }
        BGSImpactDataSet::pDefaultImpactDataSet = v1;
        v1->SetFormID(0x276u, true);
        BGSImpactDataSet::pDefaultImpactDataSet->SetFormEditorID("DefaultImpactDataSet");
        pDataHandler->AddFormToDataHandler(BGSImpactDataSet::pDefaultImpactDataSet);
    }
}

void BGSImpactDataSet::ClearDefaultImpactDataSet() {
    BGSImpactDataSet::pDefaultImpactDataSet = nullptr;
}

BGSImpactDataSet* BGSImpactDataSet::GetDefaultImpactDataSet() {
    return BGSImpactDataSet::pDefaultImpactDataSet;
}

BGSImpactDataSet::IMPACT_MATERIAL_TYPE BGSImpactDataSet::GetImpactMaterialType(HavokMaterialType aehkMatType) {
    BGSImpactDataSet::IMPACT_MATERIAL_TYPE eretval = BGSImpactDataSet::IMP_MAT_METAL;
    switch (aehkMatType) {
        case BHK_MATERIAL_STONE:
        case BHK_MATERIAL_HEAVYSTONE:
        case BHK_MATERIAL_BROKENCONCRETE: {
            eretval = BGSImpactDataSet::IMP_MAT_STONE;
            break;
        }
        case BHK_MATERIAL_CLOTH: {
            eretval = BGSImpactDataSet::IMP_MAT_CLOTH;
            break;
        }
        case BHK_MATERIAL_DIRT:
        case BHK_MATERIAL_SAND: {
            eretval = BGSImpactDataSet::IMP_MAT_DIRT;
            break;
        }
        case BHK_MATERIAL_GLASS:
        case BHK_MATERIAL_BOTTLE: {
            eretval = BGSImpactDataSet::IMP_MAT_GLASS;
            break;
        }
        case BHK_MATERIAL_GRASS: {
            eretval = BGSImpactDataSet::IMP_MAT_GRASS;
            break;
        }
        case BHK_MATERIAL_METAL:
        case BHK_MATERIAL_HEAVYMETAL:
        case BHK_MATERIAL_CHAIN:
        case BHK_MATERIAL_SNOW:
        case BHK_MATERIAL_ELEVATOR:
        case BHK_MATERIAL_VEHICLEBODY:
        case BHK_MATERIAL_VEHICLEPARTSOLID:
        case BHK_MATERIAL_PISTOL:
        case BHK_MATERIAL_RIFLE: {
            eretval = BGSImpactDataSet::IMP_MAT_METAL;
            break;
        }
        case BHK_MATERIAL_ORGANIC:
        case BHK_MATERIAL_SKIN: {
            eretval = BGSImpactDataSet::IMP_MAT_ORGANIC;
            break;
        }
        case BHK_MATERIAL_WATER: {
            eretval = BGSImpactDataSet::IMP_MAT_WATER;
            break;
        }
        case BHK_MATERIAL_WOOD:
        case BHK_MATERIAL_HEAVYWOOD:
        case BHK_MATERIAL_BABYRATTLE:
        case BHK_MATERIAL_RUBBERBALL: {
            eretval = BGSImpactDataSet::IMP_MAT_WOOD;
            break;
        }
        case BHK_MATERIAL_HOLLOWMETAL:
        case BHK_MATERIAL_SHEETMETAL:
        case BHK_MATERIAL_VEHICLEPARTHOLLOW:
        case BHK_MATERIAL_BARREL:
        case BHK_MATERIAL_SODACAN:
        case BHK_MATERIAL_SHOPPINGCART:
        case BHK_MATERIAL_LUNCHBOX: {
            eretval = BGSImpactDataSet::IMP_MAT_HOLLOW_METAL;
            break;
        }
        default: {
            return eretval;
        }
    }
    return eretval;
}

void ImpactSwap::IMPACT_SWAP_DATA::Endian() {
    ENDIAN(eType, false);
    ENDIAN(pOldData, false);
    ENDIAN(pNewData, false);
}

ImpactSwap::ImpactSwap() : pRemappedData{}, pAltFootstepMaterialNames{} {}

ImpactSwap::~ImpactSwap() {
    for (auto& pRemap : pRemappedData) {
        if (pRemap) {
            delete pRemap;
        }
        pRemap = nullptr;
    }
}

BGSImpactData*
ImpactSwap::GetSwappedImpactData(BGSImpactDataSet::IMPACT_MATERIAL_TYPE aeType, BGSImpactData* apOrigImpactData) {
    BGSImpactData* a2 = nullptr;
    if (!pRemappedData[aeType]) {
        return apOrigImpactData;
    }
    pRemappedData[aeType]->GetAt(apOrigImpactData, a2);
    return a2;
}

// Tests only the slot's first character and returns the slot itself.
const char* ImpactSwap::GetAltFootstepMaterialName(FST_MATERIALS aeType) {
    if (pAltFootstepMaterialNames[aeType][0] != 0) {
        return pAltFootstepMaterialNames[aeType];
    }
    return nullptr;
}

void ImpactSwap::InitItem(TESForm* apOwner) {
    BSMapItem<BGSImpactData*, BGSImpactData*>* iter;
    for (int32_t i = 0; i < 12; ++i) {
        if (pRemappedData[i]) {
            uint32_t iindex = 0;
            uint32_t icount = pRemappedData[i]->m_uiCount;
            ImpactSwap::IMPACT_SWAP_DATA* pdataarray = static_cast<ImpactSwap::IMPACT_SWAP_DATA*>(ourMalloc(
                icount <= SIZE_MAX / sizeof(ImpactSwap::IMPACT_SWAP_DATA)
                    ? sizeof(ImpactSwap::IMPACT_SWAP_DATA) * icount
                    : SIZE_MAX
            ));
            iter = pRemappedData[i]->GetFirstPos();
            while (iter) {
                pRemappedData[i]->GetNext(iter, pdataarray[iindex].pOldData, pdataarray[iindex].pNewData);
                ++iindex;
            }
            pRemappedData[i]->RemoveAll_1();
            for (iindex = 0; iindex < icount; ++iindex) {
                BGSImpactData* polddata = nullptr;
                if (pdataarray[iindex].pOldData) {
                    uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pdataarray[iindex].pOldData));
                    TESFile* File = apOwner->GetFile(-1);
                    TESForm::AddCompileIndex(itempint, File);
                    TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
                    polddata = dynamic_cast<BGSImpactData*>(FormByNumericID);
                    if (!polddata) {
                        int32_t iownerid = apOwner->iFormID;
                        const char* v6 = apOwner->GetFormEditorID();
                        Warning(
                            "Could not find old ImpactData (%08X) for ImpactSwap on '%s' (%08X).", itempint, v6,
                            iownerid
                        );
                    }
                }
                BGSImpactData* pnewdata = nullptr;
                if (pdataarray[iindex].pNewData) {
                    uint32_t itempint_2 =
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pdataarray[iindex].pNewData));
                    TESFile* v4 = apOwner->GetFile(-1);
                    TESForm::AddCompileIndex(itempint_2, v4);
                    TESForm* v5 = TESForm::GetFormByNumericID(itempint_2);
                    pnewdata = dynamic_cast<BGSImpactData*>(v5);
                    if (!pnewdata) {
                        int32_t v9 = apOwner->iFormID;
                        const char* v7 = apOwner->GetFormEditorID();
                        Warning(
                            "Could not find new ImpactData (%08X) for ImpactSwap on '%s' (%08X).", itempint_2, v7, v9
                        );
                    }
                }
                if (polddata || pnewdata) {
                    pRemappedData[i]->SetAt(polddata, pnewdata);
                }
            }
            ourFree(pdataarray);
            if (!pRemappedData[i]->m_uiCount) {
                BSMap<BGSImpactData*, BGSImpactData*>* v11 = pRemappedData[i];
                if (v11) {
                    delete v11;
                }
                pRemappedData[i] = nullptr;
            }
        }
    }
}

void ImpactSwap::Copy(ImpactSwap* apSwap) {
    BGSImpactData* pnewdata;
    BGSImpactData* polddata;
    BSMapItem<BGSImpactData*, BGSImpactData*>* iter;
    for (auto& pRemap : pRemappedData) {
        BSMap<BGSImpactData*, BGSImpactData*>* v3 = pRemap;
        if (v3) {
            delete v3;
        }
        pRemap = nullptr;
    }
    for (BGSImpactDataSet::IMPACT_MATERIAL_TYPE i_2 = BGSImpactDataSet::IMP_MAT_STONE;
         i_2 < BGSImpactDataSet::IMP_MAT_COUNT; i_2 = static_cast<BGSImpactDataSet::IMPACT_MATERIAL_TYPE>(i_2 + 1)) {
        if (apSwap->pRemappedData[i_2]) {
            iter = apSwap->pRemappedData[i_2]->GetFirstPos();
            while (iter) {
                polddata = nullptr;
                pnewdata = nullptr;
                apSwap->pRemappedData[i_2]->GetNext(iter, polddata, pnewdata);
                AddSwappedImpactData(i_2, polddata, pnewdata);
            }
        }
    }
    ourMemcpy(pAltFootstepMaterialNames, apSwap->pAltFootstepMaterialNames, sizeof(pAltFootstepMaterialNames));
}

void ImpactSwap::Save() {
    ImpactSwap::IMPACT_SWAP_DATA kData;
    BSMapItem<BGSImpactData*, BGSImpactData*>* iter;
    for (int32_t i = 0; i < 12; ++i) {
        if (pRemappedData[i]) {
            iter = pRemappedData[i]->GetFirstPos();
            while (iter) {
                kData.eType = static_cast<BGSImpactDataSet::IMPACT_MATERIAL_TYPE>(i);
                pRemappedData[i]->GetNext(iter, kData.pOldData, kData.pNewData);
                // The chunk holds form IDs: each pointer field is overwritten with its form's ID.
                if (kData.pOldData) {
                    // Stores a uint32_t form ID in the BGSImpactData* field.
                    kData.pOldData = reinterpret_cast<BGSImpactData*>(static_cast<uintptr_t>(kData.pOldData->iFormID));
                }
                if (kData.pNewData) {
                    // Stores a uint32_t form ID in the BGSImpactData* field.
                    kData.pNewData = reinterpret_cast<BGSImpactData*>(static_cast<uintptr_t>(kData.pNewData->iFormID));
                }
                if (TESForm::QEndianSwapOnSave()) {
                    kData.Endian();
                }
                TESForm::__AddChunkData(IMPS_ID, &kData, 12u);
                if (TESForm::QEndianSwapOnSave()) {
                    kData.Endian();
                }
            }
        }
    }
    TESForm::AddChunkArray(IMPF_ID, pAltFootstepMaterialNames[0], 300u);
}

bool ImpactSwap::Compare(ImpactSwap* apSwap) {
    BGSImpactData* pnewdata;
    BGSImpactData* polddata;
    BSMapItem<BGSImpactData*, BGSImpactData*>* iter;
    if (!apSwap) {
        return true;
    }
    for (BGSImpactDataSet::IMPACT_MATERIAL_TYPE i = BGSImpactDataSet::IMP_MAT_STONE;
         i < BGSImpactDataSet::IMP_MAT_COUNT; i = static_cast<BGSImpactDataSet::IMPACT_MATERIAL_TYPE>(i + 1)) {
        if (pRemappedData[i] && apSwap->pRemappedData[i]) {
            int32_t icount = pRemappedData[i]->m_uiCount;
            if (icount != apSwap->pRemappedData[i]->m_uiCount) {
                return true;
            }
            iter = pRemappedData[i]->GetFirstPos();
            while (iter) {
                polddata = nullptr;
                pnewdata = nullptr;
                pRemappedData[i]->GetNext(iter, polddata, pnewdata);
                BGSImpactData* pswapped = apSwap->GetSwappedImpactData(i, polddata);
                if (pswapped != pnewdata) {
                    return true;
                }
            }
        }
        else {
            if (pRemappedData[i] || apSwap->pRemappedData[i]) {
                return true;
            }
        }
    }
    return std::memcmp(
               pAltFootstepMaterialNames, apSwap->pAltFootstepMaterialNames, sizeof(pAltFootstepMaterialNames)
           ) != 0;
}

void ImpactSwap::AddSwappedImpactData(
    BGSImpactDataSet::IMPACT_MATERIAL_TYPE aeType,
    BGSImpactData* apOrigImpactData,
    BGSImpactData* apNewImpactData
) {
    if (apOrigImpactData != apNewImpactData) {
        if (!pRemappedData[aeType]) {
            BSMap<BGSImpactData*, BGSImpactData*>* pmap;
            BSMap<BGSImpactData*, BGSImpactData*>* apMemory = static_cast<BSMap<BGSImpactData*, BGSImpactData*>*>(
                ourMalloc(sizeof(BSMap<BGSImpactData*, BGSImpactData*>))
            );
            if (apMemory) {
                pmap = ::new (apMemory) BSMap<BGSImpactData*, BGSImpactData*>(37u);
            }
            else {
                pmap = nullptr;
            }
            pRemappedData[aeType] = pmap;
        }
        pRemappedData[aeType]->SetAt(apOrigImpactData, apNewImpactData);
    }
}
