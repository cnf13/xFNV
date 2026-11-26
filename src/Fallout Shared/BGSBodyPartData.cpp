#include "Fallout Shared/BGSBodyPartData.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
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
            BGSBodyPart* pPart = new BGSBodyPart;
            uint32_t uiWarnings = MessageHandler::QWarningCount();
            if (pPart->Load(apFile)) {
                uint32_t uiNextChunk = apFile->GetTESChunk();
                if (uiNextChunk == BPNN_ID ||
                    uiNextChunk == BPTN_ID ||
                    uiNextChunk == PNAM_ID ||
                    uiNextChunk == RAGA_ID) {
                    Warning(
                        "MASTERFILE: Body part data for '%s' (%08X) advanced file too far. Attempting recovery.",
                        GetFormEditorID(),
                        GetFormID()
                    );
                    ucTooFar = 1;
                }
                if (MessageHandler::QWarningCount() != uiWarnings) {
                    Warning(
                        "MASTERFILE: Warnings encountered loading part '%s' for body part data '%s' (%08X).",
                        pPart->GetPartName(),
                        GetFormEditorID(),
                        GetFormID()
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
            // Stores the form ID straight into the ragdoll pointer.
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
        for (BGSBodyPart* pPart : pPartArray) {
            if (pPart) {
                pPart->InitItem(this);
            }
        }
        uint32_t uiTemp = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRagdollData));
        if (uiTemp) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTemp, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTemp);
            pRagdollData = dynamic_cast<BGSRagdoll*>(pForm);
            if (!pRagdollData) {
                int32_t iID = iFormID;
                Warning(
                    "MASTERFILE: Could not find ragdoll (%08X) for bodypart data '%s' (%08X).",
                    uiTemp,
                    GetFormEditorID(),
                    iID
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
    NiNode* pNode = apObj->IsNode();
    for (int32_t n = 0; n < 15; ++n) {
        if (n != aeParent && pPartArray[n]) {
            NiAVObject* pTestObj = BSUtilities::GetObjectByName(apRootObj, pPartArray[n]->GetNodeName());
            if (pTestObj) {
                while (pTestObj != apRootObj) {
                    if (pTestObj == apParentObj) {
                        uint32_t i = 0;
                        do {
                            NiGeometry* pTri;
                            if (pNode) {
                                NiAVObject* pkObject = pNode->GetAtChecked(i++);
                                pTri = static_cast<NiGeometry*>(
                                    NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, pkObject)
                                );
                            }
                            else {
                                pTri = static_cast<NiGeometry*>(
                                    NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, apObj)
                                );
                            }
                            NiSkinInstance* pSkin = pTri ? pTri->GetSkinInstance() : nullptr;
                            BSDismemberSkinInstance* pdSkinInstance = static_cast<BSDismemberSkinInstance*>(
                                NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pSkin)
                            );
                            if (pdSkinInstance) {
                                for (int32_t j = 0; j < 15; ++j) {
                                    if (j == n) {
                                        pdSkinInstance->SetBodyPartEnabled(n, true);
                                    }
                                }
                            }
                        }
                        while (pNode && i < pNode->GetArrayCount());
                        break;
                    }
                    pTestObj = pTestObj->m_pkParent;
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
            BGSBodyPart* pPart = pPartArray[i];
            if (pPart && pPart->IsIKData()) {
                apOutList->AddHead(pPart);
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
        uint32_t uiIndex = apPart->GetPartType();
        if (uiIndex > 14) {
            int32_t iID = iFormID;
            Warning("MODELS: Invalid body part index %i in body part data %s (%08X).", uiIndex, GetFormEditorID(), iID);
        }
        else {
            if (pPartArray[uiIndex]) {
                int32_t iID = iFormID;
                Warning(
                    "MODELS: Body part index %i already in use in body part data %s (%08X).",
                    uiIndex,
                    GetFormEditorID(),
                    iID
                );
                BGSBodyPart* pOldPart = pPartArray[uiIndex];
                if (pOldPart) {
                    delete pOldPart;
                }
            }
            pPartArray[uiIndex] = apPart;
        }
    }
}

BGSBodyPart* BGSBodyPartData::GetBodyPart(ActorValue::Index aeIndex) {
    for (BGSBodyPart* pPart : pPartArray) {
        if (pPart && pPart->GetActorValue() == aeIndex) {
            return pPart;
        }
    }
    return nullptr;
}

void BGSBodyPartData::HideChildLimbs(
    NiAVObject* apParentObj,
    NiAVObject* apRootObj,
    BGSBodyPart::LIMB_ENUM aeParent,
    NiAVObject* apObj
) {
    NiNode* pNode = apObj->IsNode();
    for (int32_t n = 0; n < 15; ++n) {
        if (n != aeParent && pPartArray[n]) {
            NiAVObject* pTestObj = BSUtilities::GetObjectByName(apRootObj, pPartArray[n]->GetNodeName());
            if (pTestObj) {
                while (pTestObj != apRootObj) {
                    if (pTestObj == apParentObj) {
                        uint32_t i = 0;
                        do {
                            NiGeometry* pTri;
                            if (pNode) {
                                NiAVObject* pkObject = pNode->GetAt(i++);
                                pTri = static_cast<NiGeometry*>(
                                    NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, pkObject)
                                );
                            }
                            else {
                                pTri = static_cast<NiGeometry*>(
                                    NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, apObj)
                                );
                            }
                            NiSkinInstance* pSkin = pTri ? pTri->GetSkinInstance() : nullptr;
                            BSDismemberSkinInstance* pdSkinInstance = static_cast<BSDismemberSkinInstance*>(
                                NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pSkin)
                            );
                            if (pdSkinInstance) {
                                pdSkinInstance->SetBodyPartEnabled(n, false);
                            }
                        }
                        while (pNode && i < pNode->GetArrayCount());
                        break;
                    }
                    pTestObj = pTestObj->m_pkParent;
                }
            }
        }
    }
}

void BGSBodyPartData::GenerateDefaultBodyPartData() {
    TESForm* pForm = TESForm::GetFormByNumericID(0x1Du);
    BGSBodyPartData::pDefaultBodyPartData = dynamic_cast<BGSBodyPartData*>(pForm);
    if (!BGSBodyPartData::pDefaultBodyPartData) {
        BGSBodyPartData* pData = new BGSBodyPartData;
        BGSBodyPartData::pDefaultBodyPartData = pData;
        pData->SetFormID(0x1Du, true);
        BGSBodyPartData::pDefaultBodyPartData->SetFormEditorID("DefaultBodyPartData");
        pDataHandler->AddFormToDataHandler(BGSBodyPartData::pDefaultBodyPartData);
        BGSBodyPartData::pDefaultBodyPartData->SetAltered(false);
    }
    pForm = TESForm::GetFormByNumericID(0x1Cu);
    BGSBodyPartData::pPlayerBodyPartData = dynamic_cast<BGSBodyPartData*>(pForm);
    if (!BGSBodyPartData::pPlayerBodyPartData) {
        BGSBodyPartData* pData = new BGSBodyPartData;
        BGSBodyPartData::pPlayerBodyPartData = pData;
        pData->SetFormID(0x1Cu, true);
        BGSBodyPartData::pPlayerBodyPartData->SetFormEditorID("PlayerBodyPartData");
        pDataHandler->AddFormToDataHandler(BGSBodyPartData::pPlayerBodyPartData);
        BGSBodyPartData::pPlayerBodyPartData->SetAltered(false);
        BGSBodyPartData::pPlayerBodyPartData = nullptr;
    }
}

const char* BGSBodyPart::GetNodeName() {
    return NodeName.String();
}
