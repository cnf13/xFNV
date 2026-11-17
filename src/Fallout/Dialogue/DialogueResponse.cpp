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

// bUseEmotion is deliberately left unset, as the game does.
DialogueResponse::DialogueResponse()
	: eEmotion(DE_NEUTRAL), uiEmotion(0), pSpeakerIdle(nullptr), pListenIdle(nullptr), pVoiceSound(nullptr),
	  nResponseID(0) {}

// Without a response the fields get the neutral defaults; with one, its text without the {} parts, its emotion,
// voice path, idles, voice sound and ID.
DialogueResponse::DialogueResponse(TESQuest* apQuest, TESTopic* apTopic, TESTopicInfo* apTopicInfo,
	TESObjectREFR* apSpeaker, TESResponse* apResponse) {
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
	} else {
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
	uint8_t v4;
	uint8_t apData;
	pSaveLoadGameOLD->LoadGameDataOLD(&apData, 1u);
	if (apData) {
		char apDest[264];
		ourMemset(apDest, 0, 260u);
		pSaveLoadGameOLD->LoadGameDataOLD(apDest, apData);
		cText.Set(apDest, 0);
	} else {
		cText.Set("", 0);
	}
	pSaveLoadGameOLD->LoadGameDataOLD(&v4, 1u);
	if (v4) {
		char apString[264];
		ourMemset(apString, 0, 260u);
		pSaveLoadGameOLD->LoadGameDataOLD(apString, v4);
		cVoice.Set(apString, 0);
	} else {
		cVoice.Set("", 0);
	}
	pSaveLoadGameOLD->LoadGameDataOLD(&eEmotion, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&uiEmotion, 4u);
}

void DialogueResponse::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
	char* v2 = cText.pString;
	apSaveGameBuffer->SaveString(v2, 0);
	apSaveGameBuffer->SaveString(cVoice.pString, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eEmotion), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&uiEmotion), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bUseEmotion), 1u, 0);
	apSaveGameBuffer->SaveFormID(pSpeakerIdle, 0u);
	apSaveGameBuffer->SaveFormID(pListenIdle, 0u);
	apSaveGameBuffer->SaveFormID(pVoiceSound, 0u);
}

void DialogueResponse::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	char apString[1004];
	apLoadGameBuffer->LoadString(apString);
	cText.Set(apString, 0);
	apLoadGameBuffer->LoadString(apString);
	cVoice.Set(apString, 0);
	apLoadGameBuffer->LoadData(&eEmotion, 4u);
	apLoadGameBuffer->LoadData(&uiEmotion, 4u);
	apLoadGameBuffer->LoadData(&bUseEmotion, 1u);
	uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
	pSpeakerIdle = dynamic_cast<TESIdleForm*>(FormByNumericID);
	uint32_t v4 = apLoadGameBuffer->LoadFormID();
	TESForm* v5 = TESForm::GetFormByNumericID(v4);
	pListenIdle = dynamic_cast<TESIdleForm*>(v5);
	uint32_t v6 = apLoadGameBuffer->LoadFormID();
	TESForm* v7 = TESForm::GetFormByNumericID(v6);
	pVoiceSound = dynamic_cast<TESSound*>(v7);
}

uint16_t DialogueResponse::GetSaveSize() {
	int16_t v3 = static_cast<uint8_t>(cText.ByteLength()) + 1;
	return v3 + static_cast<uint8_t>(cVoice.ByteLength()) + 9;
}

void DialogueResponse::SaveGame_0() {
	uint8_t cvoiceLength;
	uint8_t ctextLength;
	ctextLength = cText.ByteLength();
	pSaveLoadGameOLD->SaveGameDataOLD(&ctextLength, 1u);
	if (ctextLength) {
		uint32_t v3 = ctextLength;
		void* v1 = cText.pString;
		pSaveLoadGameOLD->SaveGameDataOLD(v1, v3);
	}
	cvoiceLength = cVoice.ByteLength();
	pSaveLoadGameOLD->SaveGameDataOLD(&cvoiceLength, 1u);
	if (cvoiceLength) {
		uint32_t v4 = cvoiceLength;
		char* v2 = cVoice.pString;
		pSaveLoadGameOLD->SaveGameDataOLD(v2, v4);
	}
	pSaveLoadGameOLD->SaveGameDataOLD(&eEmotion, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&uiEmotion, 4u);
}

void DialogueResponse::operator delete(void* apMemory) {
	ourFree(apMemory);
}
