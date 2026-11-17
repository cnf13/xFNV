#pragma once

#include "BSCore/BSStringT.h"
#include "Reconstruction.hpp"

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class TESIdleForm;
class TESObjectREFR;
class TESQuest;
class TESResponse;
class TESSound;
class TESTopic;
class TESTopicInfo;

// The emotion of a dialogue response; the values the exe's switches test.
enum DIALOGUE_EMOTION : int32_t {
	DE_NEUTRAL = 0,
	DE_ANGER = 1,
	DE_DISGUST = 2,
	DE_FEAR = 3,
	DE_SAD = 4,
	DE_HAPPY = 5,
	DE_SURPRISE = 6,
	DE_PAINED = 7,
	DIALOGUE_EMOTION_COUNT = 8,
};

// One response of a dialogue line: its text and voice file, the emotion, the idles the speaker and the listener play
// and the voice sound.
class DialogueResponse {
public:
	DialogueResponse();
	DialogueResponse(TESQuest* apQuest, TESTopic* apTopic, TESTopicInfo* apTopicInfo, TESObjectREFR* apSpeaker,
		TESResponse* apResponse);
	~DialogueResponse();

	// The PDB's getters, each one of the exe's shared one-field getters (thiscall, ret 0):
	// &cText (0x6815C0), eEmotion (0x44DDC0), uiEmotion (0x84E3A0), &cVoice (0x460140), pSpeakerIdle (0x9611E0),
	// pListenIdle (0x441110), pVoiceSound (0x7AF430) and bUseEmotion (0x543C30).
	BSStringT<char>* GetText();
	DIALOGUE_EMOTION GetEmotion() { return eEmotion; }
	uint32_t GetEmotionValue() { return uiEmotion; }
	BSStringT<char>* GetVoice() { return &cVoice; }
	TESIdleForm* GetSpeakerIdle() { return pSpeakerIdle; }
	TESIdleForm* GetListenerIdle() { return pListenIdle; }
	TESSound* GetVoiceSound() { return pVoiceSound; }
	bool GetUseEmotion() { return bUseEmotion; }
	// nResponseID; the exe's shared one-field getter 0x45CD60 (thiscall, ret 0).
	uint32_t GetResponseID();
	void LoadGame();
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	uint16_t GetSaveSize();
	void SaveGame_0();

	static void operator delete(void* apMemory);

	BSStringT<char> cText;
	DIALOGUE_EMOTION eEmotion;
	uint32_t uiEmotion;
	BSStringT<char> cVoice;
	TESIdleForm* pSpeakerIdle;
	TESIdleForm* pListenIdle;
	TESSound* pVoiceSound;
	bool bUseEmotion;
	// IDA: unsigned __int8[3] gap25.
	uint8_t padding25[3];
	uint32_t nResponseID;
};
