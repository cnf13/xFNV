#pragma once

#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/ModifierList.h"

class Actor;
class BSPerformanceTimer;
class TESActorBase;

// the every-once-in-a-while process of an actor: adds the hour the package
// was evaluated at and the temporary modifiers of the actor values
class MiddleLowProcess : public LowProcess {
public:
	MiddleLowProcess();
	~MiddleLowProcess() override;

	void Update(MobileObject* apRef, float fTimeDelta) override;
	void Update(MobileObject* apRef) override;
	bool CheckforNewPackage(Actor* apActor, bool abFlag) override;
	void SetPackageEvaluateHour(int32_t iHour) override;
	int32_t GetPackageEvaluateHour() override;
	int32_t GetActorValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) override;
	float GetActorFloatValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) override;
	void TempModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) override;
	void TempModActorValue(Actor* apActor, ActorValue::Index aiWhich, int32_t aiValue) override;
	void DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) override;
	uint16_t GetSaveSize(uint32_t aiFlags, MobileObject* pmob) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void SaveGame(uint32_t aiFlags, MobileObject* pmob) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, MobileObject* pmob) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(uint32_t aiFlags, MobileObject* pmob) override;
	float GetTemporaryModifier(ActorValue::Index aiWhich) override;
	void ProcessCombat(Actor* apActor) override;
	// run one hour of combat while not in high
	virtual bool ProcessCombatOneHour(Actor* apActor, float afTimeDelta);
	using LowProcess::DamageModActorValue;

	void MiddleLowCopy(MiddleLowProcess* apProcess);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	// 0xB4 the hour of the day the package was evaluated at
	uint32_t iHourPackageEvaluated;
	// 0xB8 temporary modifiers of the actor values
	ModifierList pTemporaryModifiers;
};

extern BSPerformanceTimer AIProcessing_MiddleLowUpdate;
