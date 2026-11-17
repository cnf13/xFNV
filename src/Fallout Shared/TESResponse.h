#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Reconstruction.hpp"

enum DIALOGUE_EMOTION : int32_t;
class TESBoundObject;
class TESFile;
class TESForm;
class TESIdleForm;
class TESObjectREFR;
class TESQuest;
class TESSound;
class TESTopic;
class TESTopicInfo;

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

	// The PDB's GetAudioFullPath(apSpeakerBase, ...): writes the voice file path to aFullPath; false when the info has
	// no file or no voice type (or its editor ID) is found for the speaker, true otherwise, found or not (thiscall,
	// ret 0x14, 0x616FA0).
	bool GetAudioFullPath(TESBoundObject* apSpeakerBase, TESQuest* apQuest, TESTopic* apOwnerTopic,
		TESTopicInfo* apOwnerInfo, BSStringT<char>& aFullPath);
	// Copies apSource.
	void Copy(TESResponse* apCopy);
	// Appends apResponse after the last response of this chain.
	void AddTail(TESResponse* apResp);
	// pNext, through the exe's shared one-field getter (thiscall, ret 0, 0x45CD60).
	TESResponse* QNext();
	uint32_t GetResponseID() const;
	// The response text, or the engine's empty string without one. Same function as TESGlobal::GetFormEditorID
	// (0x616DE0), registered there.
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

	// The exe's deleting destructor (0x61CB30) frees through ourFree.
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

// GameSettingCollection: 0xf5441a calls 0x40c150.
extern SettingT<GameSettingCollection> gs_iStandardEmotionValue;

// INISettingCollection: 0xf5435d calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_sFileTypeGame_Voice;
