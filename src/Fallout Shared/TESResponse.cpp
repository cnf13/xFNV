#include "Fallout Shared/TESResponse.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSVoiceType.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "SDK/FaceGen/algs.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

SettingT<INISettingCollection> gs_sFileTypeGame_Voice("sFileTypeGame:Voice", "mp3");
SettingT<INISettingCollection> TESResponse::sFileTypeSource("sFileTypeSource:Voice", "wav");
SettingT<INISettingCollection> TESResponse::sFileTypeLip("sFileTypeLip:Voice", "lip");
SettingT<INISettingCollection> TESResponse::sFileTypeLTF("sFileTypeLTF:Voice", "ltf");
SettingT<GameSettingCollection> gs_iStandardEmotionValue("iStandardEmotionValue", 100);

void TESResponse::Load(TESFile* apFile) {
    if (apFile && apFile->GetTESChunk() == TRDT_ID) {
        apFile->GetChunkData(this, 0x18u);
        // The file's record is the 0x18-byte RESPONSE_DATA, with 4-byte form IDs at 8 and 0x10.
        TESFile::WidenFormIDRecords(
            &Data,
            1,
            0x18,
            sizeof(Data),
            {{8, offsetof(RESPONSE_DATA, pConversationTopic)}, {0x10, offsetof(RESPONSE_DATA, pVoiceSound)}}
        );
        if (apFile->QEndian()) {
            Data.Endian();
        }
    }
}

RESPONSE_DATA::RESPONSE_DATA()
  : eEmotion(DE_NEUTRAL), iEmotionValue(50), pConversationTopic(nullptr), ucResponseID(0), pVoiceSound(nullptr),
    bUseEmotion(true) {}

void RESPONSE_DATA::Endian() {
    ENDIAN(eEmotion, false);
    ENDIAN(iEmotionValue, false);
    ENDIAN(pConversationTopic, false);
    ENDIAN(pVoiceSound, false);
}

TESResponse::TESResponse() : Data(), cResponseText(), pSpeakerIdle(nullptr), pListenerIdle(nullptr), pNext(nullptr) {
    cResponseText.Set(static_cast<const char*>(nullptr), 0u);
}

TESResponse::~TESResponse() {}

// Returns true once it has the voice type's editor ID and the info's file, whether TestAudioFullPath finds the file or
// not.
bool TESResponse::GetAudioFullPath(
    TESBoundObject* apSpeakerBase,
    TESQuest* apQuest,
    TESTopic* apOwnerTopic,
    TESTopicInfo* apOwnerInfo,
    BSStringT<char>& aFullPath
) {
    BSStringT<char> path;
    BSStringT<char> fileName;
    GetAudioFilename(apQuest, apOwnerTopic, apOwnerInfo, fileName);
    BGSVoiceType* pVoiceType = nullptr;
    TESFile* pRootFile = apOwnerInfo->GetFile(0);
    if (!pRootFile) {
        return false;
    }
    const char* pMPFileName = nullptr;
    if (apOwnerInfo && apOwnerInfo->GetSpeaker()) {
        pVoiceType = apOwnerInfo->GetSpeaker()->GetVoiceType();
    }
    if (!pVoiceType) {
        if (apSpeakerBase->GetFormType() == NPC__ID) {
            pVoiceType = static_cast<TESActorBase*>(apSpeakerBase)->GetVoiceType();
        }
        else if (apSpeakerBase->GetFormType() == CREA_ID) {
            pVoiceType = static_cast<TESActorBase*>(apSpeakerBase)->GetVoiceType();
        }
        else if (apSpeakerBase->GetFormType() == TACT_ID) {
            pVoiceType = static_cast<BGSTalkingActivator*>(apSpeakerBase)->BGSTalkingActivator::GetVoiceType();
        }
        else {
            TESRace* pRace = TESRaceForm::GetFormRace(apSpeakerBase);
            if (!pRace) {
                return false;
            }
            pVoiceType = pRace->GetDefaultVoiceType(TESActorBase::SEX_MALE);
        }
    }
    if (!pVoiceType) {
        return false;
    }
    const char* pVoiceTypeName = pVoiceType->GetFormEditorID();
    if (!pVoiceTypeName) {
        return false;
    }
    const char* pDirPath = "Data\\Sound\\Voice";
    if (pRootFile) {
        pMPFileName = pRootFile->GetFilename();
        if (!pMPFileName) {
            return false;
        }
    }
    const char* pExtension = gs_sFileTypeGame_Voice.String();
    TestAudioFullPath(aFullPath, pDirPath, pMPFileName, pVoiceTypeName, fileName.pString, pExtension);
    return true;
}

void TESResponse::Copy(TESResponse* apCopy) {
    ourMemcpy(&Data, &apCopy->Data, sizeof(RESPONSE_DATA));
    cResponseText.Set(apCopy->GetResponseText(), 0);
    pSpeakerIdle = apCopy->pSpeakerIdle;
    pListenerIdle = apCopy->pListenerIdle;
}

void TESResponse::AddTail(TESResponse* apResp) {
    TESResponse* i;
    TESResponse* pLast = this;
    for (i = QNext(); i; i = pLast->QNext()) {
        pLast = pLast->QNext();
    }
    pLast->pNext = apResp;
}

TESResponse* TESResponse::QNext() {
    return pNext;
}

uint32_t TESResponse::GetResponseID() const {
    return Data.ucResponseID;
}

const char* TESResponse::GetResponseText() {
    if (cResponseText.String()) {
        return cResponseText.String();
    }
    return "";
}

uint32_t TESResponse::GetResponseTextLength() const {
    return cResponseText.GetLength();
}

void TESResponse::InitItem(TESForm* apOwner) {
    if (Data.pConversationTopic) {
        uint32_t formID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pConversationTopic));
        TESForm::AddCompileIndex(formID, apOwner->GetFile(-1));
        TESForm* pfileform = TESForm::GetFormByNumericID(formID);
        if (pfileform && pfileform->GetFormType() == DIAL_ID) {
            Data.pConversationTopic = static_cast<TESTopic*>(pfileform);
        }
        else {
            Data.pConversationTopic = nullptr;
        }
        if (!Data.pConversationTopic) {
            char cInfoText[264];
            TESTopicInfo* pinfo = dynamic_cast<TESTopicInfo*>(apOwner);
            TESTopic* ptopic = TESTopic::GetOwnerTopic(pinfo);
            TESQuest* pquest = nullptr;
            if (ptopic) {
                pquest = ptopic->GetOwnerQuest(pinfo);
            }
            BSStringT<char> infotext;
            pinfo->GetInfoDisplayText(infotext, true);
            int32_t iTopicID = ptopic ? ptopic->iFormID : 0;
            const char* pcTopicName = ptopic ? ptopic->GetFormEditorID() : "";
            int32_t iQuestID = pquest ? pquest->iFormID : 0;
            const char* pcQuestName = pquest ? pquest->GetEditorName() : "";
            std::sprintf(
                cInfoText,
                "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)",
                pinfo->iFormID,
                infotext.pString,
                pcQuestName,
                iQuestID,
                pcTopicName,
                iTopicID
            );
            Warning("MASTERFILE: Unable to find TESTopic (%08X) in TESResponse init for %s.", formID, cInfoText);
        }
    }
    if (Data.pVoiceSound) {
        uint32_t formID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pVoiceSound));
        TESForm::AddCompileIndex(formID, apOwner->GetFile(-1));
        TESForm* pfileform = TESForm::GetFormByNumericID(formID);
        if (pfileform && pfileform->GetFormType() == SOUN_ID) {
            Data.pVoiceSound = static_cast<TESSound*>(pfileform);
        }
        else {
            Data.pVoiceSound = nullptr;
        }
        if (!Data.pVoiceSound) {
            char cInfoText[268];
            TESTopicInfo* pinfo = dynamic_cast<TESTopicInfo*>(apOwner);
            TESTopic* ptopic = TESTopic::GetOwnerTopic(pinfo);
            TESQuest* pquest = nullptr;
            if (ptopic) {
                pquest = ptopic->GetOwnerQuest(pinfo);
            }
            BSStringT<char> infotext;
            pinfo->GetInfoDisplayText(infotext, true);
            int32_t iTopicID = ptopic ? ptopic->iFormID : 0;
            const char* pcTopicName = ptopic ? ptopic->GetFormEditorID() : "";
            int32_t iQuestID = pquest ? pquest->iFormID : 0;
            const char* pcQuestName = pquest ? pquest->GetEditorName() : "";
            std::sprintf(
                cInfoText,
                "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)",
                pinfo->iFormID,
                infotext.pString,
                pcQuestName,
                iQuestID,
                pcTopicName,
                iTopicID
            );
            Warning("MASTERFILE: Unable to find TESSound (%08X) in TESResponse init for %s.", formID, cInfoText);
        }
    }
    if (pSpeakerIdle) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSpeakerIdle));
        TESForm::AddCompileIndex(itempint, apOwner->GetFile(-1));
        TESForm* pfileform = TESForm::GetFormByNumericID(itempint);
        if (pfileform && pfileform->GetFormType() == IDLE_ID) {
            pSpeakerIdle = static_cast<TESIdleForm*>(pfileform);
        }
        else {
            pSpeakerIdle = nullptr;
        }
        if (!pSpeakerIdle) {
            char cInfoText[268];
            TESTopicInfo* pinfo = dynamic_cast<TESTopicInfo*>(apOwner);
            TESTopic* ptopic = TESTopic::GetOwnerTopic(pinfo);
            TESQuest* pquest = nullptr;
            if (ptopic) {
                pquest = ptopic->GetOwnerQuest(pinfo);
            }
            BSStringT<char> infotext;
            pinfo->GetInfoDisplayText(infotext, true);
            int32_t iTopicID = ptopic ? ptopic->iFormID : 0;
            const char* pcTopicName = ptopic ? ptopic->GetFormEditorID() : "";
            int32_t iQuestID = pquest ? pquest->iFormID : 0;
            const char* pcQuestName = pquest ? pquest->GetEditorName() : "";
            std::sprintf(
                cInfoText,
                "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)",
                pinfo->iFormID,
                infotext.pString,
                pcQuestName,
                iQuestID,
                pcTopicName,
                iTopicID
            );
            Warning("MASTERFILE: Could not find speaker idle (%08X) in TESResponse init for %s.", itempint, cInfoText);
        }
    }
    if (pListenerIdle) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pListenerIdle));
        TESForm::AddCompileIndex(itempint, apOwner->GetFile(-1));
        TESForm* pfileform = TESForm::GetFormByNumericID(itempint);
        if (pfileform && pfileform->GetFormType() == IDLE_ID) {
            pListenerIdle = static_cast<TESIdleForm*>(pfileform);
        }
        else {
            pListenerIdle = nullptr;
        }
        if (!pListenerIdle) {
            char cInfoText[268];
            TESTopicInfo* pinfo = dynamic_cast<TESTopicInfo*>(apOwner);
            TESTopic* ptopic = TESTopic::GetOwnerTopic(pinfo);
            if (ptopic) {
                ptopic->GetOwnerQuest(pinfo);
            }
            BSStringT<char> infotext;
            pinfo->GetInfoDisplayText(infotext, true);
            Warning("MASTERFILE: Could not find listener idle (%08X) in TESResponse init for %s.", itempint, cInfoText);
        }
    }
    if (Data.eEmotion) {
        Data.iEmotionValue = gs_iStandardEmotionValue.Int();
    }
}

void TESResponse::SetSpeakerIdle(TESIdleForm* apIdle) {
    pSpeakerIdle = apIdle;
}

void TESResponse::SetListenerIdle(TESIdleForm* apIdle) {
    pListenerIdle = apIdle;
}

bool TESResponse::TestAudioFullPath(
    BSStringT<char>& aFullPath,
    const char* apDirPath,
    const char* apMPFileName,
    const char* apVoiceTypeName,
    const char* apFileName,
    const char* apFileExt
) {
    char cPath[265];
    char* pcPath = &cPath[1];
    cPath[1] = 0;
    BSstrcat(&cPath[1], 260, apDirPath);
    BSstrcat(pcPath, 260, "\\");
    BSstrcat(pcPath, 260, apMPFileName);
    BSstrcat(pcPath, 260, "\\");
    BSstrcat(pcPath, 260, apVoiceTypeName);
    BSstrcat(pcPath, 260, "\\");
    BSstrcat(pcPath, 260, apFileName);
    BSstrcat(pcPath, 260, ".");
    BSstrcat(pcPath, 260, apFileExt);
    aFullPath = &cPath[1];
    return true;
}

void TESResponse::GetAudioFilename(
    TESQuest* apOwnerQuest,
    TESTopic* apOwnerTopic,
    TESTopicInfo* apOwnerInfo,
    BSStringT<char>& aFilename
) {
    if (apOwnerQuest && apOwnerTopic && apOwnerInfo) {
        uint32_t uiResponseID = GetResponseID();
        if (uiResponseID) {
            char cBuffer[268];
            TESResponse::GetAudioQuestTopicCombined(apOwnerQuest, apOwnerTopic, cBuffer, 260u);
            int32_t iFormID = apOwnerInfo->iFormID & 0xFFFFFF;
            aFilename.SPrintF("%s_%08X_%u", cBuffer, iFormID, uiResponseID);
        }
        else {
            aFilename.Set("New Response", 0u);
        }
    }
}

int32_t TESResponse::GetListCount() {
    if (!this) {
        return 0;
    }
    int32_t iCount = 1;
    for (TESResponse* i = pNext; i; i = i->pNext) {
        ++iCount;
    }
    return iCount;
}

bool TESResponse::GetAudioFullPath(
    TESObjectREFR* apSpeaker,
    TESQuest* apQuest,
    TESTopic* apOwnerTopic,
    TESTopicInfo* apOwnerInfo,
    BSStringT<char>& aFullPath
) {
    if (!apSpeaker) {
        return false;
    }
    int32_t iFormType = apSpeaker->GetObjectReference()->GetFormType();
    if (iFormType != 22 && (iFormType <= 41 || iFormType > 43)) {
        return false;
    }
    TESBoundObject* pSpeakerBase = apSpeaker->GetObjectReference();
    return GetAudioFullPath(pSpeakerBase, apQuest, apOwnerTopic, apOwnerInfo, aFullPath);
}

void TESResponse::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void TESResponse::GetAudioQuestTopicCombined(
    TESQuest* apQuest,
    TESTopic* apTopic,
    char* apOutBuffer,
    uint32_t aiBufferSize
) {
    if (apOutBuffer && aiBufferSize) {
        *apOutBuffer = 0;
        if (apQuest && apTopic) {
            char cQuestName[264];
            char cTopicName[264];
            ourStrcpy(cQuestName, apQuest->GetEditorName());
            ourStrcpy(cTopicName, apTopic->GetFormEditorID());
            uint32_t iqlen = std::strlen(cQuestName);
            if (std::strlen(cTopicName) + iqlen > 25) {
                uint32_t uiTopicLen = 15;
                if (iqlen <= 10) {
                    uiTopicLen = 10 - iqlen + 15;
                }
                else {
                    cQuestName[10] = 0;
                }
                cTopicName[uiTopicLen] = 0;
            }
            BSsprintf(apOutBuffer, aiBufferSize, "%s_%s", cQuestName, cTopicName);
        }
    }
}

TESRace* TESRaceForm::GetFormRace(TESForm* apForm) {
    auto* pRaceForm = dynamic_cast<TESRaceForm*>(apForm);
    return pRaceForm ? pRaceForm->GetFormRace() : nullptr;
}

TESActorBase* TESTopicInfo::GetSpeaker() {
    return pSpeaker;
}
