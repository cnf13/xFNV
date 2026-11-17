#pragma once

#include "Fallout/Magic/MagicHitEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class ActiveEffect;
class NiNode;
class TESObjectREFR;

class MagicModelHitEffect : public MagicHitEffect {
public:
	MagicModelHitEffect();
	MagicModelHitEffect(TESObjectREFR* a1, ActiveEffect* a2);
	MagicModelHitEffect(TESObjectREFR* apTarget, const char* apFilename, float afTime);
	~MagicModelHitEffect() override;

	const NiRTTI* GetRTTI() const override;
	// Inferred type: a1: float from 0x81ccbf (passed to MagicHitEffect::Update 0)
	bool Update(float) override;
	TEMP_EFFECT_TYPE GetType() override;
	bool CheckShouldSave() override;
	uint16_t GetSaveSize() override;
	uint16_t GetSaveSize(ActiveEffect*, TESObjectREFR*) override;
	void SaveGame() override;
	void SaveGame(BGSSaveGameBuffer*) override;
	void SaveGame(ActiveEffect*, TESObjectREFR*) override;
	bool LoadGame() override;
	void LoadGame(BGSLoadGameBuffer*) override;
	void LoadGame(ActiveEffect*, TESObjectREFR*) override;
	void InitLoadGame(BGSLoadGameBuffer*) override;
	void InitLoadGame(ActiveEffect*, TESObjectREFR*) override;
	void FinishInitLoadGame(ActiveEffect*, TESObjectREFR*, void*) override;
	void FinishLoadGame(BGSLoadGameBuffer*) override;
	bool Init() override;
	void UpdatePosition() override;

	bool bOwnPath;
	bool b3rdPerson;
	uint8_t padding2A[2];
	const char* pFilename;
	NiPointer<NiNode> spObject;
	NiPointer<NiRefObject> spTarget;
	// IDA types it MagicItem* pSpell; the exe constructs it with BGSLoadGameSubBuffer's default constructor
	// (0x44DEE0), and the destructor (0x81C810) and FinishLoadGame (0x81D9F0) call DeleteBuffer (0x435B20) and
	// Create (0x864E20) on it.
	BGSLoadGameSubBuffer LoadedDataSubBuffer;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable MagicModelHitEffect_vftable{};
