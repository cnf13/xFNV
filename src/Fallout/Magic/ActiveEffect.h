#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Reconstruction.hpp"

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
	// MSVC places overloads in reverse declaration order. The buffer overloads get the form buffers of
	// SaveActiveEffectList and LoadActiveEffect.
	virtual void SaveGame(Actor*);
	virtual void SaveGame(BGSSaveFormBuffer*);
	virtual void LoadGame(Actor*);
	virtual void LoadGame(BGSLoadFormBuffer*);
	// Both PC InitLoadGame bodies return no value, despite the exported PC vtable's bool annotation.
	virtual void InitLoadGame(Actor*);
	virtual void InitLoadGame(BGSLoadGameBuffer*);
	virtual void FinishInitLoadGame(Actor*);
	// Returns no value on PC: the base body is the shared empty stub (0x4534F0) and overrides leave eax unset.
	virtual void Revert(Actor*);
	// Empty (thiscall, ret 4). Same function as ActorMover::UpdateTurnTimer (0x4534F0), registered there.
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

	// The spell (pSpell). The body is folded with TESForm::GetFormFlags at 0x44DDC0.
	MagicItem* GetSpell();
	// The spell type (eSpellType). The body is folded with TESPackage::GetLocation at 0x55B980.
	MagicSystem::SpellType GetSpellType();
	// The effect item (pEffect; the shared one-field getter 0x84E3A0, thiscall, ret 0).
	EffectItem* GetEffectItem();
	EffectSetting* GetEffectSetting();
	// The object whose enchantment cast the effect (pSource; the shared one-field getter
	// 0x639B40, thiscall, ret 0).
	TESBoundObject* GetEnchantmentSource();
	// The target (pTarget; the exe's shared +0x24 getter 0x59BB30, thiscall, ret 0).
	MagicTarget* GetTarget();
	// Same function as BaseProcess::GetProcessLevel (0x45CD60), the exe's shared +0x28 getter; the caster (pCaster).
	MagicCaster* GetCaster() { return pCaster; }
	void ForceNoHitEffect();
	// Returns bDone; the exe's upper EAX bits are not modeled.
	int8_t IsDone();
	// Sets the requested ActiveEffect flag bits.
	void SetFlag(ActiveEffect::ActiveEffectFlags aiFlag);
	// Returns whether the effect came from a worn enchantment.
	bool FromWornEnchantment();
	// pDisplacementSpell; the exe's shared +0x44 getter (thiscall, ret 0, 0x8041A0).
	MagicItem* GetDisplacementSpell();
	// Sets pDisplacementSpell; the exe's shared +0x44 setter (thiscall, ret 4, 0x8D8040).
	void SetDisplacementSpell(MagicItem* apObject);
	bool HasConditions() { return (iFlags & 0x80) != 0; }
	bool IsFlagSet(ActiveEffect::ActiveEffectFlags aiFlag) { return (iFlags & aiFlag) != 0; }
	bool IsQueuedForRemoval() const;
	// IDA name; not in the PDB. Sets or clears the bit IsQueuedForRemoval tests. The exe tests the whole byte;
	// bool would assume 0/1.
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
	// Sets fDuration. The body is folded with BaseProcess::SetLastTimeProcessed at 0x693D50.
	void SetDuration(float afDuration);
	// fDuration. The body is folded with BaseProcess::GetLastTimeProcessed at 0x7DF1F0 (ST0).
	float GetDuration();
	// fMagnitude. The body is folded with NiControllerSequence::GetSequenceWeight at 0x495460 (ST0).
	float GetMagnitude();
	// Sets pTarget. The body is folded with Menu::SetFadeState at 0x7037C0.
	void SetTarget(MagicTarget* apTarget);
	// Sets fElapsedSeconds. The body is folded with NiFloatKey::SetValue at 0x4F5D90.
	void SetElapsedTime(float afValue);
	bool CheckDisplacementSpellOnTarget();
	void StartSound();
	void RemoveHitEffect(MagicHitEffect* apHitEffect);

	// Inferred type: apObject: TESBoundObject* from 0x815ce7 (parameter 2 of MagicCaster::CreateActiveEffect)
	static ActiveEffect* Activate(MagicCaster* apCaster, MagicItem* apMagicItem, EffectItem* apEffectItem,
		TESBoundObject* apObject);
	// Loads a saved effect: spell form, effect index, archetype, then Activate and LoadGameBGS
	// (cdecl, ret 0, 0x806910; IDA types the argument Actor*, the exe passes LoadActiveEffectList's
	// BGSLoadFormBuffer).
	static ActiveEffect* LoadActiveEffect(BGSLoadFormBuffer* apLoadGameBuffer);
	// Reverts apActiveEffectList, then reads the saved count and inserts each loaded effect in sorted order.
	static void LoadActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
		BSSimpleList<ActiveEffect*>* apActiveEffectList);
	// IDA name LoadActiveEffectList; not this overload in the PDB (src's LoadActiveEffectList
	// is 0x806A90). Reads apActiveEffectList back from the old-format save (TESSaveLoadGame) for apActor (cdecl,
	// ret 0, 0x805D40).
	static void LoadActiveEffectListTES(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	// Clears apActiveEffectList for a load.
	static void RevertActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
		BSSimpleList<ActiveEffect*>* apActiveEffectList);
	// Resolves the saved effects of apActiveEffectList onto apActor.
	static void InitLoadGameActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	// Finishes the init-load of the effects of apActiveEffectList on apActor.
	static void FinishInitLoadGameActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	// Finishes the load of the effects of apActiveEffectList (cdecl, ret 0, 0x806B50). IDA
	// leaves it unnamed (sub_806B50); the PDB's FinishLoadActiveEffectList has these parameters, beside the revert at
	// 0x806BA0.
	static void FinishLoadActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
		BSSimpleList<ActiveEffect*>* apActiveEffectList);
	// Reverts the effects of apActiveEffectList on apActor; the TES revert calls it.
	static void RevertActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	// Saves apActiveEffectList into apSaveGameBuffer.
	static void SaveActiveEffectList(BGSSaveFormBuffer* apSaveGameBuffer,
		BSSimpleList<ActiveEffect*>* apActiveEffectList);
	// The PDB's other overload: saves apActiveEffectList through TESSaveLoadGame, the old
	// format (a "KOLB" tag first when save game blocks are on; cdecl, ret 0, 0x805B40).
	static void SaveActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor);
	// Saves apEffect through TESSaveLoadGame, the old format: its save size (version 42 on), the spell's form ID, the
	// effect's index in the spell, then its SaveGame (cdecl, ret 0, 0x805880). PDB: SaveActiveEffect(apEffect,
	// apActor); IDA leaves it unnamed.
	static void SaveActiveEffect(ActiveEffect* apEffect, Actor* apActor);
	// The old-format save size of apEffect for apActor: its GetSaveSize plus the spell id
	// and effect index (cdecl, ret 0, 0x805810). PDB: GetActiveEffectSaveSize(apEffect, apActor); IDA leaves it
	// unnamed.
	static uint16_t GetActiveEffectSaveSize(ActiveEffect* apEffect, Actor* apActor);
	// The old-format save size of the effects of apActiveEffectList for apActor (cdecl,
	// ret 0, 0x805A00): the node walk adds GetActiveEffectSaveSize of each item. PDB:
	// GetActiveEffectListSaveSize(apActiveEffectList, apActor); IDA names it a free GetSaveSize. Callers
	// MiddleHighProcess::GetSaveSize and PlayerCharacter::GetSaveSize pass pActiveEffectList and the actor.
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
	// IDA types this slot as a word, but CopyData, SaveGame and LoadGame access it only as one byte.
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
inline constexpr fnvd::Vtable ActiveEffect_vftable{};
