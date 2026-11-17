#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Magic/MagicSystem.h"

class Actor;
class BGSLoadFormBuffer;
class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class EffectItem;
class EffectSetting;
class MagicCaster;
class MagicHitEffect;
class MagicItem;
class MagicTarget;
class TESBoundObject;

// A magic effect running on a target.
class ActiveEffect {
public:
	enum ActiveEffectFlags : uint32_t {
		NO_HITEFFECT_FORCED = 0x1,
		NO_HITEFFECT_SHADER = 0x2,
		NO_HITEFFECT_NIF = 0x4,
		NO_SOUND = 0x8,
		NO_INTIAL_FLARE = 0x10,
		DEFERRED_SOUND = 0x20,
		DEFERRED_HITEFFECT = 0x40,
		HAS_CONDITIONS = 0x80,
		FROM_WORN_ENCHANTMENT = 0x100,
		WAS_DISPELED = 0x200,
		EFFECT_DEAD = 0x400,
	};

	ActiveEffect(MagicCaster* a1, MagicItem* a2, EffectItem* a3);
	virtual ~ActiveEffect();

	virtual ActiveEffect* Copy();
	virtual void Finalize();
	virtual void Update(float) {}
	virtual uint16_t GetSaveSize(Actor*);
	// The buffer overloads get the form buffers of SaveActiveEffectList and LoadActiveEffect.
	virtual void SaveGame(Actor*);
	virtual void SaveGame(BGSSaveFormBuffer*);
	virtual void LoadGame(Actor*);
	virtual void LoadGame(BGSLoadFormBuffer*);
	// Both PC InitLoadGame bodies return no value, despite the exported PC vtable's bool annotation.
	virtual void InitLoadGame(Actor*);
	virtual void InitLoadGame(BGSLoadGameBuffer*);
	virtual void FinishInitLoadGame(Actor*);
	virtual void Revert(Actor*);
	// Empty.
	virtual void Revert(BGSLoadFormBuffer*) {}
	virtual bool CheckDeletedCaster(MagicCaster*);
	virtual bool IsCausingDamage();
	virtual void FinishLoadGame(BGSLoadFormBuffer*);
	virtual void CopyData(ActiveEffect*);
	virtual bool CheckCast(MagicCaster*);
	virtual bool CheckTarget(MagicTarget*);
	virtual void Start() {}
	virtual void Continue() {}
	virtual void Finish() {}

	MagicItem* GetSpell();
	MagicSystem::SpellType GetSpellType();
	EffectItem* GetEffectItem();
	EffectSetting* GetEffectSetting();
	// The object whose enchantment cast the effect (pSource).
	TESBoundObject* GetEnchantmentSource();
	// Returns the target (pTarget).
	MagicTarget* GetTarget();
	MagicCaster* GetCaster() { return pCaster; }
	void ForceNoHitEffect();
	// Returns bDone.
	int8_t IsDone();
	// Sets the requested ActiveEffect flag bits.
	void SetFlag(ActiveEffect::ActiveEffectFlags aiFlag);
	// Returns whether the effect came from a worn enchantment.
	bool FromWornEnchantment();
	MagicItem* GetDisplacementSpell();
	void SetDisplacementSpell(MagicItem* apObject);
	bool HasConditions() { return (iFlags & 0x80) != 0; }
	bool IsFlagSet(ActiveEffect::ActiveEffectFlags aiFlag) { return (iFlags & aiFlag) != 0; }
	bool IsQueuedForRemoval() const;
	// Sets or clears the bit IsQueuedForRemoval tests. The whole byte is tested, so the parameter is uint8_t rather
	// than bool, which would assume 0/1.
	void SetQueuedForRemoval(uint8_t abQueued);
	// Whether the WAS_DISPELED bit is set.
	bool WasDispeled();
	void UpdateEffect(float afSecondsElapsed);
	void EvaluateConditions(float afTimeDelta, bool abForceUpdate);
	void SetEnchantmentSource(TESBoundObject* apSource);
	void Dispel(bool abNow);
	void ClearFlag(ActiveEffect::ActiveEffectFlags aiFlag);
	void SetMagnitude(float afMagnitude);
	// Scales the effect's duration or magnitude by afMultiplier.
	void AdjustEffectiveness(float afMultiplier);
	// Sets fDuration.
	void SetDuration(float afDuration);
	float GetDuration();
	// Returns fMagnitude.
	float GetMagnitude();
	void SetTarget(MagicTarget* apTarget);
	// Sets fElapsedSeconds.
	void SetElapsedTime(float afValue);
	bool CheckDisplacementSpellOnTarget();
	void StartSound();
	void RemoveHitEffect(MagicHitEffect* apHitEffect);

	static ActiveEffect* Activate(MagicCaster* apCaster, MagicItem* apMagicItem, EffectItem* apEffectItem,
		TESBoundObject* apObject);
	// Loads a saved effect: spell form, effect index, archetype, then Activate and LoadGameBGS.
	static ActiveEffect* LoadActiveEffect(BGSLoadFormBuffer* apLoadGameBuffer);
	// Reverts apActiveEffectList, then reads the saved count and inserts each loaded effect in sorted order.
	static void LoadActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
		BSSimpleList<ActiveEffect*>* apActiveEffectList);
	// Reads apActiveEffectList back from the old-format save (TESSaveLoadGame) for apActor.
	static void LoadActiveEffectListTES(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	// Clears apActiveEffectList for a load.
	static void RevertActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
		BSSimpleList<ActiveEffect*>* apActiveEffectList);
	// Resolves the saved effects of apActiveEffectList onto apActor.
	static void InitLoadGameActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	// Finishes the init-load of the effects of apActiveEffectList on apActor.
	static void FinishInitLoadGameActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	// Finishes the load of the effects of apActiveEffectList.
	static void FinishLoadActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
		BSSimpleList<ActiveEffect*>* apActiveEffectList);
	// Reverts the effects of apActiveEffectList on apActor; the TES revert calls it.
	static void RevertActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	// Saves apActiveEffectList into apSaveGameBuffer.
	static void SaveActiveEffectList(BGSSaveFormBuffer* apSaveGameBuffer,
		BSSimpleList<ActiveEffect*>* apActiveEffectList);
	// Other overload: saves apActiveEffectList through TESSaveLoadGame, the old format (a "KOLB" tag first when save
	// game blocks are on).
	static void SaveActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	// Saves apEffect through TESSaveLoadGame, the old format: its save size (version 42 on), the spell's form ID, the
	// effect's index in the spell, then its SaveGame.
	static void SaveActiveEffect(ActiveEffect* apEffect, Actor* apActor);
	// The old-format save size of apEffect for apActor: its GetSaveSize plus the spell id and effect index.
	static uint16_t GetActiveEffectSaveSize(ActiveEffect* apEffect, Actor* apActor);
	// The old-format save size of the effects of apActiveEffectList for apActor: the node walk adds
	// GetActiveEffectSaveSize of each item. Callers MiddleHighProcess::GetSaveSize and PlayerCharacter::GetSaveSize
	// pass pActiveEffectList and the actor.
	static uint16_t GetActiveEffectListSaveSize(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	static void operator delete(void* apMemory);
	static ActiveEffect* LoadActiveEffectTES(Actor* a1);
	static void InitLoadActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
		BSSimpleList<ActiveEffect*>* apActiveEffectList);
	static void SaveActiveEffect(BGSSaveFormBuffer* apSaveGameBuffer, ActiveEffect* apEffect);

	float fElapsedSeconds;
	MagicItem* pSpell;
	EffectItem* pEffect;
	bool bActive;
	bool bStarted;
	bool bFinished;
	bool bDone;
	// CopyData, SaveGame and LoadGame access this only as one byte.
	bool bWornEnchantment;
	uint8_t padding15[3];
	uint32_t iFlags;
	float fMagnitude;
	float fDuration;
	MagicTarget* pTarget;
	MagicCaster* pCaster;
	MagicSystem::SpellType eSpellType;
	BSSoundHandle PersistentSound;
	TESBoundObject* pSource;
	BSSimpleList<MagicHitEffect*>* pHitEffects;
	MagicItem* pDisplacementSpell;
};
