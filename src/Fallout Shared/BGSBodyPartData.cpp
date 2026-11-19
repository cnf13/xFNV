#include "Fallout Shared/BGSBodyPartData.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/MessageHandler.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

BGSBodyPartData::BGSBodyPartData() {
    SetFormType(BPTD_ID);
    BGSBodyPartData::InitializeData();
}

BGSBodyPartData::~BGSBodyPartData() {
    BGSBodyPartData::ClearData();
}

void BGSBodyPartData::InitializeData() {
    ourMemset(pPartArray, 0, sizeof(pPartArray));
    pRagdollData = nullptr;
}

void BGSBodyPartData::ClearData() {
    ClearParts();
}

// Reads the model, the ragdoll data and the body parts; a part chunk is followed by that part's own chunks.
bool BGSBodyPartData::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != BPTD_ID) {
        return false;
    }
    LoadForm(apFile);
    uint8_t ucTooFar = 0;
    for (;;) {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            return true;
        }
        ucTooFar = 0;
        if (uiChunk == PNAM_ID || uiChunk == BPNN_ID || uiChunk == BPTN_ID) {
            void* pMemory = ourMalloc(sizeof(BGSBodyPart));
            BGSBodyPart* pPart;
            if (pMemory) {
                pPart = ::new (pMemory) BGSBodyPart;
            }
            else {
                pPart = nullptr;
            }
            uint32_t uiWarnings = MessageHandler::QWarningCount();
            if (pPart->Load(apFile)) {
                uint32_t uiNextChunk = apFile->GetTESChunk();
                if (uiNextChunk == BPNN_ID || uiNextChunk == BPTN_ID || uiNextChunk == PNAM_ID ||
                    uiNextChunk == RAGA_ID) {
                    Warning(
                        "MASTERFILE: Body part data for '%s' (%08X) advanced file too far. Attempting recovery.",
                        GetFormEditorID(), GetFormID()
                    );
                    ucTooFar = 1;
                }
                if (MessageHandler::QWarningCount() != uiWarnings) {
                    Warning(
                        "MASTERFILE: Warnings encountered loading part '%s' for body part data '%s' (%08X).",
                        pPart->GetPartName(), GetFormEditorID(), GetFormID()
                    );
                }
                AddBodyPart(pPart);
            }
            else {
                Warning("MASTERFILE: Load failed for body part data '%s' (%08X).", GetFormEditorID(), GetFormID());
                if (pPart) {
                    delete pPart;
                }
                return false;
            }
        }
        else if (uiChunk == OBND_ID) {
            LoadObjectBound(apFile);
        }
        else if (uiChunk == RAGA_ID) {
            // Stores the form ID straight into the ragdoll pointer, as the game does.
            uint32_t uiRagdollID = 0;
            apFile->GetChunkData(uiRagdollID);
            pRagdollData = reinterpret_cast<BGSRagdoll*>(static_cast<uintptr_t>(uiRagdollID));
        }
        else if (uiChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
            apFile->GetChunkData(pBuffer, 0x200);
            SetFormEditorID(pBuffer);
        }
        else if (uiChunk == MODL_ID || uiChunk == MODT_ID) {
            TESModel::LoadModelChunk(this, apFile);
        }
        if (!ucTooFar && !apFile->NextChunk()) {
            return true;
        }
    }
}

void BGSBodyPartData::InitItem() {
    if (!GetInitialized()) {
        for (BGSBodyPart* ppart : pPartArray) {
            if (ppart) {
                ppart->InitItem(this);
            }
        }
        uint32_t uitemp = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRagdollData));
        if (uitemp) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(uitemp, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(uitemp);
            pRagdollData = dynamic_cast<BGSRagdoll*>(FormByNumericID);
            if (!pRagdollData) {
                int32_t CastingType = iFormID;
                const char* v3 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find ragdoll (%08X) for bodypart data '%s' (%08X).", uitemp, v3, CastingType
                );
            }
        }
        SetInitialized(true);
    }
}

// Queues each body part's files.
void BGSBodyPartData::QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    for (int32_t i = 0; i < BGSBodyPart::LIMB_COUNT; ++i) {
        BGSBodyPart* pPart = pPartArray[i];
        if (pPart) {
            pPart->QueueFiles(aePriority, apParent);
        }
    }
}

void BGSBodyPartData::ShowChildLimbs(
    NiAVObject* apParentObj,
    NiAVObject* apRootObj,
    BGSBodyPart::LIMB_ENUM aeParent,
    NiAVObject* apObj
) {
    NiNode* pnode = apObj->IsNode();
    for (int32_t n = 0; n < 15; ++n) {
        if ((n != aeParent) && (pPartArray[n])) {
            NiAVObject* ptestObj = BSUtilities::GetObjectByName(apRootObj, pPartArray[n]->GetNodeName());
            if (ptestObj) {
                while (ptestObj != apRootObj) {
                    if (ptestObj == apParentObj) {
                        uint32_t i = 0;
                        do {
                            NiSkinInstance* pskin;
                            NiGeometry* ptri;
                            if (pnode) {
                                NiAVObject* pkObject = pnode->GetAtChecked(i++);
                                ptri = static_cast<NiGeometry*>(
                                    NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, pkObject)
                                );
                            }
                            else {
                                ptri =
                                    static_cast<NiGeometry*>(NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, apObj));
                            }
                            if (ptri) {
                                pskin = ptri->GetSkinInstance();
                            }
                            else {
                                pskin = nullptr;
                            }
                            BSDismemberSkinInstance* pdSkinInstance = static_cast<BSDismemberSkinInstance*>(
                                NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pskin)
                            );
                            if (pdSkinInstance) {
                                for (int32_t j = 0; j < 15; ++j) {
                                    if (j == n) {
                                        pdSkinInstance->SetBodyPartEnabled(n, true);
                                    }
                                }
                            }
                        }
                        while (pnode && i < pnode->GetArrayCount());
                        break;
                    }
                    ptestObj = ptestObj->m_pkParent;
                }
            }
        }
    }
}

void BGSBodyPartData::ClearParts() {
    for (int32_t i = 0; i < 15; ++i) {
        if (pPartArray[i]) {
            delete pPartArray[i];
        }
        pPartArray[i] = nullptr;
    }
}

BGSRagdoll* BGSBodyPartData::QRagdollData() {
    return pRagdollData;
}

void BGSBodyPartData::BuildIKPartList(BSSimpleList<BGSBodyPart*>* apOutList) {
    if (apOutList) {
        for (int32_t i = 0; i < 15; ++i) {
            BGSBodyPart* a2 = pPartArray[i];
            if ((a2) && (a2->IsIKData())) {
                apOutList->AddHead(a2);
            }
        }
    }
}

// The limb is compared signed: -1 < aType < 15.
BGSBodyPart* BGSBodyPartData::GetBodyPart(BGSBodyPart::LIMB_ENUM aType) {
    BGSBodyPart* pPart = nullptr;
    if (aType > -1 && aType < 15) {
        pPart = pPartArray[aType];
    }
    return pPart;
}

void BGSBodyPartData::AddBodyPart(BGSBodyPart* apPart) {
    if (apPart) {
        uint32_t iindex = apPart->GetPartType();
        if (iindex > 14) {
            int32_t CastingType = iFormID;
            Warning(
                "MODELS: Invalid body part index %i in body part data %s (%08X).", iindex, GetFormEditorID(),
                CastingType
            );
        }
        else {
            if (pPartArray[iindex]) {
                int32_t v2 = iFormID;
                Warning(
                    "MODELS: Body part index %i already in use in body part data %s (%08X).", iindex, GetFormEditorID(),
                    v2
                );
                BGSBodyPart* apThis = pPartArray[iindex];
                if (apThis) {
                    delete apThis;
                }
            }
            pPartArray[iindex] = apPart;
        }
    }
}

BGSBodyPart* BGSBodyPartData::GetBodyPart_005E5130(ActorValue::Index aeIndex) {
    BGSBodyPart* v6 = nullptr;
    for (BGSBodyPart* pPart : pPartArray) {
        if (pPart && pPart->GetActorValue() == aeIndex) {
            return pPart;
        }
    }
    return v6;
}

void BGSBodyPartData::HideChildLimbs(
    NiAVObject* apParentObj,
    NiAVObject* apRootObj,
    BGSBodyPart::LIMB_ENUM aeParent,
    NiAVObject* apObj
) {
    NiNode* pnode = apObj->IsNode();
    for (int32_t n = 0; n < 15; ++n) {
        if ((n != aeParent) && (pPartArray[n])) {
            NiAVObject* ptestObj = BSUtilities::GetObjectByName(apRootObj, pPartArray[n]->GetNodeName());
            if (ptestObj) {
                while (ptestObj != apRootObj) {
                    if (ptestObj == apParentObj) {
                        uint32_t i = 0;
                        do {
                            NiSkinInstance* pskin;
                            NiGeometry* ptri;
                            if (pnode) {
                                NiAVObject* pkObject = pnode->GetAt(i++);
                                ptri = static_cast<NiGeometry*>(
                                    NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, pkObject)
                                );
                            }
                            else {
                                ptri =
                                    static_cast<NiGeometry*>(NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, apObj));
                            }
                            if (ptri) {
                                pskin = ptri->GetSkinInstance();
                            }
                            else {
                                pskin = nullptr;
                            }
                            BSDismemberSkinInstance* pdSkinInstance = static_cast<BSDismemberSkinInstance*>(
                                NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pskin)
                            );
                            if (pdSkinInstance) {
                                pdSkinInstance->SetBodyPartEnabled(n, false);
                            }
                        }
                        while (pnode && i < pnode->GetArrayCount());
                        break;
                    }
                    ptestObj = ptestObj->m_pkParent;
                }
            }
        }
    }
}

void BGSBodyPartData::GenerateDefaultBodyPartData() {
    BGSBodyPartData* v2;
    BGSBodyPartData* v3;
    void* v4;
    void* apMemory;
    TESForm* formByNumericID = TESForm::GetFormByNumericID(0x1Du);
    BGSBodyPartData::pDefaultBodyPartData = dynamic_cast<BGSBodyPartData*>(formByNumericID);
    if (!BGSBodyPartData::pDefaultBodyPartData) {
        apMemory = ourMalloc(sizeof(BGSBodyPartData));
        if (apMemory) {
            v3 = ::new (apMemory) BGSBodyPartData;
        }
        else {
            v3 = nullptr;
        }
        BGSBodyPartData::pDefaultBodyPartData = v3;
        v3->SetFormID(0x1Du, true);
        BGSBodyPartData::pDefaultBodyPartData->SetFormEditorID("DefaultBodyPartData");
        pDataHandler->AddFormToDataHandler(BGSBodyPartData::pDefaultBodyPartData);
        BGSBodyPartData::pDefaultBodyPartData->SetAltered(false);
    }
    TESForm* v1 = TESForm::GetFormByNumericID(0x1Cu);
    BGSBodyPartData::pPlayerBodyPartData = dynamic_cast<BGSBodyPartData*>(v1);
    if (!BGSBodyPartData::pPlayerBodyPartData) {
        v4 = ourMalloc(sizeof(BGSBodyPartData));
        if (v4) {
            v2 = ::new (v4) BGSBodyPartData;
        }
        else {
            v2 = nullptr;
        }
        BGSBodyPartData::pPlayerBodyPartData = v2;
        v2->SetFormID(0x1Cu, true);
        BGSBodyPartData::pPlayerBodyPartData->SetFormEditorID("PlayerBodyPartData");
        pDataHandler->AddFormToDataHandler(BGSBodyPartData::pPlayerBodyPartData);
        BGSBodyPartData::pPlayerBodyPartData->SetAltered(false);
        BGSBodyPartData::pPlayerBodyPartData = nullptr;
    }
}

const char* BGSBodyPart::GetNodeName() {
    return NodeName.String();
}
