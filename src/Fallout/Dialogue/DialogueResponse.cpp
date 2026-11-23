#include "Fallout/Dialogue/DialogueResponse.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESResponse.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"

// bUseEmotion is deliberately left unset.
DialogueResponse::DialogueResponse()
  : eEmotion(DE_NEUTRAL), uiEmotion(0), pSpeakerIdle(nullptr), pListenIdle(nullptr), pVoiceSound(nullptr),
    nResponseID(0) {}

// Without a response the fields get the neutral defaults; with one, its text without the {} parts, its emotion,
// voice path, idles, voice sound and ID.
DialogueResponse::DialogueResponse(
    TESQuest* apQuest,
    TESTopic* apTopic,
    TESTopicInfo* apTopicInfo,
    TESObjectREFR* apSpeaker,
    TESResponse* apResponse
) {
    if (apResponse) {
        BSStringT<char> sResponseText;
        sResponseText.Set(apResponse->GetResponseText(), 0);
        // RemoveCurlyContent builds its result in kCurlyFree.
        BSStringT<char> kCurlyFree;
        cText.Set(*sResponseText.RemoveCurlyContent(&kCurlyFree));
        eEmotion = apResponse->Data.eEmotion;
        uiEmotion = apResponse->Data.iEmotionValue;
        TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(apTopicInfo);
        apResponse->GetAudioFullPath(apSpeaker, apQuest, pOwnerTopic, apTopicInfo, cVoice);
        pSpeakerIdle = apResponse->pSpeakerIdle;
        pListenIdle = apResponse->pListenerIdle;
        pVoiceSound = apResponse->Data.pVoiceSound;
        bUseEmotion = apResponse->Data.bUseEmotion;
        nResponseID = apResponse->GetResponseID();
    }
    else {
        pSpeakerIdle = nullptr;
        pListenIdle = nullptr;
        eEmotion = DE_NEUTRAL;
        uiEmotion = 0;
        pVoiceSound = nullptr;
        bUseEmotion = true;
        nResponseID = 0;
    }
}

void DialogueResponse::LoadGame() {
    uint8_t cVoiceLength;
    uint8_t cTextLength;
    pSaveLoadGameOLD->LoadGameDataOLD(&cTextLength, sizeof(cTextLength));
    if (cTextLength) {
        char acText[264];
        ourMemset(acText, 0, 260u);
        pSaveLoadGameOLD->LoadGameDataOLD(acText, cTextLength);
        cText.Set(acText, 0);
    }
    else {
        cText.Set("", 0);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&cVoiceLength, sizeof(cVoiceLength));
    if (cVoiceLength) {
        char acVoice[264];
        ourMemset(acVoice, 0, 260u);
        pSaveLoadGameOLD->LoadGameDataOLD(acVoice, cVoiceLength);
        cVoice.Set(acVoice, 0);
    }
    else {
        cVoice.Set("", 0);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&eEmotion, sizeof(eEmotion));
    pSaveLoadGameOLD->LoadGameDataOLD(&uiEmotion, sizeof(uiEmotion));
}

void DialogueResponse::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveString(cText.pString, 0);
    apSaveGameBuffer->SaveString(cVoice.pString, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eEmotion), sizeof(eEmotion), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&uiEmotion), sizeof(uiEmotion), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bUseEmotion), sizeof(bUseEmotion), 0);
    apSaveGameBuffer->SaveFormID(pSpeakerIdle, 0u);
    apSaveGameBuffer->SaveFormID(pListenIdle, 0u);
    apSaveGameBuffer->SaveFormID(pVoiceSound, 0u);
}

void DialogueResponse::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    char acBuffer[1004];
    apLoadGameBuffer->LoadString(acBuffer);
    cText.Set(acBuffer, 0);
    apLoadGameBuffer->LoadString(acBuffer);
    cVoice.Set(acBuffer, 0);
    apLoadGameBuffer->LoadData(&eEmotion, sizeof(eEmotion));
    apLoadGameBuffer->LoadData(&uiEmotion, sizeof(uiEmotion));
    apLoadGameBuffer->LoadData(&bUseEmotion, sizeof(bUseEmotion));
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    pSpeakerIdle = dynamic_cast<TESIdleForm*>(pForm);
    uiFormID = apLoadGameBuffer->LoadFormID();
    pForm = TESForm::GetFormByNumericID(uiFormID);
    pListenIdle = dynamic_cast<TESIdleForm*>(pForm);
    uiFormID = apLoadGameBuffer->LoadFormID();
    pForm = TESForm::GetFormByNumericID(uiFormID);
    pVoiceSound = dynamic_cast<TESSound*>(pForm);
}

uint16_t DialogueResponse::GetSaveSize() {
    int16_t iTextSize = static_cast<uint8_t>(cText.ByteLength()) + 1;
    return iTextSize + static_cast<uint8_t>(cVoice.ByteLength()) + 9;
}

void DialogueResponse::SaveGame_0() {
    uint8_t cTextLength = cText.ByteLength();
    pSaveLoadGameOLD->SaveGameDataOLD(&cTextLength, sizeof(cTextLength));
    if (cTextLength) {
        pSaveLoadGameOLD->SaveGameDataOLD(cText.pString, cTextLength);
    }
    uint8_t cVoiceLength = cVoice.ByteLength();
    pSaveLoadGameOLD->SaveGameDataOLD(&cVoiceLength, sizeof(cVoiceLength));
    if (cVoiceLength) {
        pSaveLoadGameOLD->SaveGameDataOLD(cVoice.pString, cVoiceLength);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&eEmotion, sizeof(eEmotion));
    pSaveLoadGameOLD->SaveGameDataOLD(&uiEmotion, sizeof(uiEmotion));
}

void DialogueResponse::operator delete(void* apMemory) {
    ourFree(apMemory);
}
