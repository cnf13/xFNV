#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"

class TESBoundObject;
class TESFile;
class TESForm;
class TESIdleForm;
class TESObjectREFR;
class TESQuest;
class TESSound;
class TESTopic;
class TESTopicInfo;

enum DIALOGUE_EMOTION : int32_t;

// A response's TRDT chunk: the emotion and its value, the topic, the response number, the voice sound and whether
// the emotion is used. The topic and sound are loaded as form IDs.
struct RESPONSE_DATA {
	RESPONSE_DATA();

	// Byte-swaps the record read from a big-endian file.
	void Endian();

	DIALOGUE_EMOTION eEmotion;
	int32_t iEmotionValue;
	TESTopic* pConversationTopic;
	uint8_t ucResponseID;
	TESSound* pVoiceSound;
	bool bUseEmotion;
};

class TESResponse {
public:
	TESResponse();
	~TESResponse();

	// Writes the voice file path to aFullPath; false when the info has no file or no voice type (or its editor ID) is
	// found for the speaker, true otherwise, found or not.
	bool GetAudioFullPath(TESBoundObject* apSpeakerBase, TESQuest* apQuest, TESTopic* apOwnerTopic,
		TESTopicInfo* apOwnerInfo, BSStringT<char>& aFullPath);
	// Copies apSource.
	void Copy(TESResponse* apCopy);
	// Appends apResponse after the last response of this chain.
	void AddTail(TESResponse* apResp);
	// Returns pNext.
	TESResponse* QNext();
	uint32_t GetResponseID() const;
	// The response text, or the engine's empty string without one.
	const char* GetResponseText();
	uint32_t GetResponseTextLength() const;
	void Load(TESFile* apFile);
	// Resolves the loaded form IDs (topic, voice sound, idles) for the owning info apOwner.
	void InitItem(TESForm* apOwner);
	// Copies apResponseText into the response text.
	void SetResponseText(const char* apResponseText);
	void SetSpeakerIdle(TESIdleForm* apIdle);
	void SetListenerIdle(TESIdleForm* apIdle);
	bool TestAudioFullPath(BSStringT<char>& aFullPath, const char* apDirPath, const char* apMPFileName,
		const char* apVoiceTypeName, const char* apFileName, const char* apFileExt);
	void GetAudioFilename(TESQuest* apOwnerQuest, TESTopic* apOwnerTopic, TESTopicInfo* apOwnerInfo,
		BSStringT<char>& aFilename);
	int32_t GetListCount();
	bool GetAudioFullPath(TESObjectREFR* apSpeaker, TESQuest* apQuest, TESTopic* apOwnerTopic,
		TESTopicInfo* apOwnerInfo, BSStringT<char>& aFullPath);

	// Deleting frees through ourFree.
	static void operator delete(void* apMemory);
	static void GetAudioQuestTopicCombined(TESQuest* apQuest, TESTopic* apTopic, char* apOutBuffer,
		uint32_t aiBufferSize);

	RESPONSE_DATA Data;
	BSStringT<char> cResponseText;
	TESIdleForm* pSpeakerIdle;
	TESIdleForm* pListenerIdle;
	TESResponse* pNext;

	static SettingT<INISettingCollection> sFileTypeSource;
	static SettingT<INISettingCollection> sFileTypeLip;
	static SettingT<INISettingCollection> sFileTypeLTF;
};

extern SettingT<GameSettingCollection> gs_iStandardEmotionValue;

extern SettingT<INISettingCollection> gs_sFileTypeGame_Voice;
