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
    if ((apFile) && (((int32_t)apFile->GetTESChunk()) == 1413763668)) {
        apFile->GetChunkData(this, 0x18u);
#if XFNV_64BIT
            // The file's record is the 0x18-byte RESPONSE_DATA, with 4-byte form IDs at 8 and 0x10.
            TESFile::WidenFormIDRecords(
                &Data, 1, 0x18, sizeof(Data),
                {{8, offsetof(RESPONSE_DATA, pConversationTopic)}, {0x10, offsetof(RESPONSE_DATA, pVoiceSound)}}
            );
#endif
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
    TESResponse* processLevel = this;
    for (i = QNext(); i; i = processLevel->QNext()) {
        processLevel = processLevel->QNext();
    }
    processLevel->pNext = apResp;
}

TESResponse* TESResponse::QNext() {
    return pNext;
}

uint32_t TESResponse::GetResponseID() const {
    return (uint32_t)Data.ucResponseID;
}

const char* TESResponse::GetResponseText() {
    if (cResponseText.String()) {
        return cResponseText.String();
    }
    return "";
}

uint32_t TESResponse::GetResponseTextLength() const {
    return (&cResponseText)->GetLength();
}

void TESResponse::InitItem(TESForm* apOwner) {
    if (Data.pConversationTopic) {
        uint32_t formID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pConversationTopic));
        TESFile* File = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(formID, File);
        TESForm* pfileform = TESForm::GetFormByNumericID(formID);
        if (pfileform && pfileform->GetFormType() == DIAL_ID) {
            Data.pConversationTopic = static_cast<TESTopic*>(pfileform);
        }
        else {
            Data.pConversationTopic = nullptr;
        }
        if (!Data.pConversationTopic) {
            const char* v20;
            int32_t v21;
            const char* v22;
            int32_t CastingType;
            char v46[264];
            TESTopicInfo* pinfo = dynamic_cast<TESTopicInfo*>(apOwner);
            TESTopic* ptopic = TESTopic::GetOwnerTopic(pinfo);
            TESQuest* pquest = nullptr;
            if (ptopic) {
                pquest = ptopic->GetOwnerQuest(pinfo);
            }
            BSStringT<char> infotext;
            pinfo->GetInfoDisplayText(infotext, true);
            if (ptopic) {
                CastingType = ptopic->iFormID;
            }
            else {
                CastingType = 0;
            }
            if (ptopic) {
                v22 = ptopic->GetFormEditorID();
            }
            else {
                v22 = "";
            }
            if (pquest) {
                v21 = pquest->iFormID;
            }
            else {
                v21 = 0;
            }
            if (pquest) {
                v20 = pquest->GetEditorName();
            }
            else {
                v20 = "";
            }
            char* v9 = infotext.pString;
            int32_t v3 = pinfo->iFormID;
            std::sprintf(
                v46, "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)", v3, v9, v20, v21, v22, CastingType
            );
            Warning("MASTERFILE: Unable to find TESTopic (%08X) in TESResponse init for %s.", formID, v46);
        }
    }
    if (Data.pVoiceSound) {
        uint32_t formID_2 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pVoiceSound));
        TESFile* v4 = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(formID_2, v4);
        TESForm* pfileform_2 = TESForm::GetFormByNumericID(formID_2);
        if (pfileform_2 && pfileform_2->GetFormType() == SOUN_ID) {
            Data.pVoiceSound = static_cast<TESSound*>(pfileform_2);
        }
        else {
            Data.pVoiceSound = nullptr;
        }
        if (!Data.pVoiceSound) {
            const char* v16;
            int32_t v17;
            const char* v18;
            int32_t v19;
            char v39[268];
            TESTopicInfo* pinfo_2 = dynamic_cast<TESTopicInfo*>(apOwner);
            TESTopic* ptopic_2 = TESTopic::GetOwnerTopic(pinfo_2);
            TESQuest* pquest_2 = nullptr;
            if (ptopic_2) {
                pquest_2 = ptopic_2->GetOwnerQuest(pinfo_2);
            }
            BSStringT<char> infotext_2;
            pinfo_2->GetInfoDisplayText(infotext_2, true);
            if (ptopic_2) {
                v19 = ptopic_2->iFormID;
            }
            else {
                v19 = 0;
            }
            if (ptopic_2) {
                v18 = ptopic_2->GetFormEditorID();
            }
            else {
                v18 = "";
            }
            if (pquest_2) {
                v17 = pquest_2->iFormID;
            }
            else {
                v17 = 0;
            }
            if (pquest_2) {
                v16 = pquest_2->GetEditorName();
            }
            else {
                v16 = "";
            }
            char* v10 = infotext_2.pString;
            int32_t v5 = pinfo_2->iFormID;
            std::sprintf(v39, "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)", v5, v10, v16, v17, v18, v19);
            Warning("MASTERFILE: Unable to find TESSound (%08X) in TESResponse init for %s.", formID_2, v39);
        }
    }
    if (pSpeakerIdle) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSpeakerIdle));
        TESFile* v6 = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(itempint, v6);
        TESForm* pfileform_3 = TESForm::GetFormByNumericID(itempint);
        if (pfileform_3 && pfileform_3->GetFormType() == IDLE_ID) {
            pSpeakerIdle = static_cast<TESIdleForm*>(pfileform_3);
        }
        else {
            pSpeakerIdle = nullptr;
        }
        if (!pSpeakerIdle) {
            const char* v12;
            int32_t v13;
            const char* v14;
            int32_t v15;
            char v32[268];
            TESTopicInfo* pinfo_3 = dynamic_cast<TESTopicInfo*>(apOwner);
            TESTopic* ptopic_3 = TESTopic::GetOwnerTopic(pinfo_3);
            TESQuest* pquest_3 = nullptr;
            if (ptopic_3) {
                pquest_3 = ptopic_3->GetOwnerQuest(pinfo_3);
            }
            BSStringT<char> infotext_3;
            pinfo_3->GetInfoDisplayText(infotext_3, true);
            if (ptopic_3) {
                v15 = ptopic_3->iFormID;
            }
            else {
                v15 = 0;
            }
            if (ptopic_3) {
                v14 = ptopic_3->GetFormEditorID();
            }
            else {
                v14 = "";
            }
            if (pquest_3) {
                v13 = pquest_3->iFormID;
            }
            else {
                v13 = 0;
            }
            if (pquest_3) {
                v12 = pquest_3->GetEditorName();
            }
            else {
                v12 = "";
            }
            char* v11 = infotext_3.pString;
            int32_t v7 = pinfo_3->iFormID;
            std::sprintf(v32, "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)", v7, v11, v12, v13, v14, v15);
            Warning("MASTERFILE: Could not find speaker idle (%08X) in TESResponse init for %s.", itempint, v32);
        }
    }
    if (pListenerIdle) {
        uint32_t itempint_2 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pListenerIdle));
        TESFile* v8 = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(itempint_2, v8);
        TESForm* pfileform_4 = TESForm::GetFormByNumericID(itempint_2);
        if (pfileform_4 && pfileform_4->GetFormType() == IDLE_ID) {
            pListenerIdle = static_cast<TESIdleForm*>(pfileform_4);
        }
        else {
            pListenerIdle = nullptr;
        }
        if (!pListenerIdle) {
            char v25[268];
            TESTopicInfo* pinfo_4 = dynamic_cast<TESTopicInfo*>(apOwner);
            TESTopic* ptopic_4 = TESTopic::GetOwnerTopic(pinfo_4);
            TESQuest* pquest_4 = nullptr;
            if (ptopic_4) {
                pquest_4 = ptopic_4->GetOwnerQuest(pinfo_4);
            }
            BSStringT<char> infotext_4;
            pinfo_4->GetInfoDisplayText(infotext_4, true);
            Warning("MASTERFILE: Could not find listener idle (%08X) in TESResponse init for %s.", itempint_2, v25);
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
    char result[265];
    char* pout = &result[1];
    result[1] = 0;
    BSstrcat(&result[1], 260, apDirPath);
    BSstrcat(pout, 260, "\\");
    BSstrcat(pout, 260, apMPFileName);
    BSstrcat(pout, 260, "\\");
    BSstrcat(pout, 260, apVoiceTypeName);
    BSstrcat(pout, 260, "\\");
    BSstrcat(pout, 260, apFileName);
    BSstrcat(pout, 260, ".");
    BSstrcat(pout, 260, apFileExt);
    aFullPath = &result[1];
    return true;
}

void TESResponse::GetAudioFilename(
    TESQuest* apOwnerQuest,
    TESTopic* apOwnerTopic,
    TESTopicInfo* apOwnerInfo,
    BSStringT<char>& aFilename
) {
    if ((apOwnerQuest) && (apOwnerTopic) && (apOwnerInfo)) {
        uint32_t temp = GetResponseID();
        if (temp) {
            char cBuffer[268];
            TESResponse::GetAudioQuestTopicCombined(apOwnerQuest, apOwnerTopic, cBuffer, 260u);
            int32_t uiFormID = apOwnerInfo->iFormID & 0xFFFFFF;
            aFilename.SPrintF("%s_%08X_%u", cBuffer, uiFormID, temp);
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
    int32_t v2 = 1;
    for (TESResponse* i = pNext; i; i = i->pNext) {
        ++v2;
    }
    return v2;
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
    TESBoundObject* ObjectReference = apSpeaker->GetObjectReference();
    int32_t FormType = ObjectReference->GetFormType();
    if (FormType != 22 && (FormType <= 41 || FormType > 43)) {
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
    if ((apOutBuffer) && (aiBufferSize)) {
        *apOutBuffer = 0;
        if ((apQuest) && (apTopic)) {
            char Destination[264];
            char Str[264];
            const char* v4 = apQuest->GetEditorName();
            ourStrcpy(Destination, v4);
            const char* v5 = apTopic->GetFormEditorID();
            ourStrcpy(Str, v5);
            uint32_t iqlen = std::strlen(Destination);
            if (std::strlen(Str) + iqlen > 25) {
                uint32_t v6 = 15;
                if (iqlen <= 10) {
                    v6 = 10 - iqlen + 15;
                }
                else {
                    Destination[10] = 0;
                }
                Str[v6] = 0;
            }
            BSsprintf(apOutBuffer, aiBufferSize, "%s_%s", Destination, Str);
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
