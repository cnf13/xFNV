#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"

class ActiveEffect;
class Actor;
class AlchemyItem;
class EffectItem;
class EffectSetting;
class MagicCaster;
class MagicItem;
class TESBoundObject;
class TESObjectREFR;

// Interface of things magic can be applied to (Actor, ...). The destructor is deliberately not virtual.
class MagicTarget {
public:
	MagicTarget();
	~MagicTarget();

	virtual bool AddTarget(MagicCaster* apCaster, MagicItem* apSpell, ActiveEffect* apActiveEffect, bool abAreaTarget);
	virtual TESObjectREFR* GetTargetStatsObject() { return nullptr; }
	virtual BSSimpleList<ActiveEffect*>* GetActiveEffectList() { return nullptr; }
	virtual bool MagicTargetIsActor() { return false; }
	virtual bool IsInvulnerable() { return false; }

protected:
	virtual void EffectAdded(ActiveEffect*) {}
	virtual void EffectRemoved(ActiveEffect*) {}
	virtual void EffectReflected(MagicCaster*, ActiveEffect*) {}
	virtual void EffectAbsorbed(MagicCaster*, ActiveEffect*) {}
	virtual float CheckResistance(MagicCaster*, MagicItem*, ActiveEffect*) { return 1.0f; }
	virtual bool CheckAbsorb(MagicCaster*, MagicItem*, ActiveEffect*, bool) { return false; }
	virtual bool CheckReflect(MagicCaster*, MagicItem*, ActiveEffect*) { return false; }

public:
	void RemoveEffects(bool abNow);
	// Applies the effects of apItem to the target.
	void ProcessEffectsFromItem(MagicItem* apItem);
	// Re-evaluates the conditions of the effects that have conditions or a displacement spell.
	void EvaluateSpellConditions();
	// Makes apDisplacementSpell the displacement spell of every effect of apDisplacedSpell and re-evaluates them.
	void SetDisplacementSpell(MagicItem* apDisplacementSpell, MagicItem* apDisplacedSpell);

protected:
	// Removes apEffect from the target and destroys it now with abImmediate (on the worker threads when they run), else
	// only marks it for removal.
	void RemoveEffect(ActiveEffect* apEffect, bool abImmediate);
	// Adds apActiveEffect's duration to the matching effect the target has; true when one
	// took it.
	bool StackEffectDuration(MagicItem* apSpell, MagicCaster* apCaster, ActiveEffect* apActiveEffect);
	// Restarts the elapsed time of the matching effect the target has; true when one was
	// found.
	bool ResetEffectElapsedTime(MagicItem* apSpell, MagicCaster* apCaster, ActiveEffect* apActiveEffect);

public:
	// Dispels the active effects that came from the enchantment source.
	void Dispel(TESBoundObject* apEnchantmentSource, bool abNow);
	// Dispels every active effect on the target.
	void Dispel();
	// Whether an active effect is damaging the target's health.
	bool HasDamageHealthEffect();
	// Dispels apSpell's effects, only those cast by apCaster or only apActiveEffect when
	// given.
	void Dispel(MagicItem* apSpell, MagicCaster* apCaster, ActiveEffect* apActiveEffect);
	// Dispels apSpell's active effect of apEffect, cast by apCaster or by anyone when null.
	void Dispel(EffectItem* apEffect, MagicItem* apSpell, MagicCaster* apCaster);
	// Whether apSpell has an effect on the target, counting inactive ones with
	// abIncludeInactiveSpells.
	bool IsSpellTarget(MagicItem* apSpell, bool abIncludeInactiveSpells);
	// Updates the target's active effects over afSecondsElapsed.
	void UpdateTarget(float afSecondsElapsed);
	// The actor the target is, or null.
	Actor* GetTargetAsActor();
	// Whether the target has a recover-flagged effect on the actor value. For the player, an unconditional ability
	// effect on the value does not count.
	bool HasRecoverableActorValueEffect(ActorValue::Index aeWhich);
	// The form ID of the target.
	uint32_t GetMagicTargetFormID();
	void Dispel_4(int32_t a1);
	// Dispels the effects of archetype aeID, only those cast by apCaster when given.
	void Dispel(EffectArchetypes::ArchetypeID aeID, MagicCaster* apCaster);
	// Rolls the addiction of apSpell for the target.
	bool CheckAddiction(AlchemyItem* apSpell);
	// Adds a copy of apActiveEffect cast by apCaster with apSpell, unless the target resists it or the effect
	// stacks onto, refreshes or replaces one it has; prints the outcome when magic stats are shown.
	bool CheckAddEffect(MagicCaster* apCaster, MagicItem* apSpell, ActiveEffect* apActiveEffect);
	// Whether the target is addicted to apAddictedToSpell, counting inactive addiction
	// effects with abIncludeInactiveAddictionEffects.
	bool IsAddicted(MagicItem* apAddictedToSpell, bool abIncludeInactiveAddictionEffects);
	bool IsLastEffect(MagicItem* apSpell, ActiveEffect* apLastEffect);
	char HasArchType(EffectArchetypes::ArchetypeID a1, char a2);
	bool ChecksForMagicEffect(EffectSetting* a1);
	double GetSpellUsageCount(MagicItem* a1);
	// Queues apSpell's effects from apCaster (or apActiveEffect alone) to be dispelled once the update ends.
	void QueueSpellDispel(MagicItem* apSpell, MagicCaster* apCaster, ActiveEffect* apActiveEffect);
	// Dispels and frees the queued spell dispels.
	void DispelQueuedSpells();

	static MagicTarget* GetMagicTargetByNumericID(uint32_t aiFormID);
	// The sort order of the active effect list: the effects' names, magnitudes and durations, compared as text.
	static int32_t ActiveEffectListCompareFn(ActiveEffect* apCompare1, ActiveEffect* apCompare2);

	// Whether the target shows its stats while selected.
	bool bShowTargetStats;

	// Set while the active effects of the target are being updated.
	bool bUpdating;
	uint8_t padding06[2];

	// Effects queued to be dispelled once the running update ends.
	BSSimpleList<void*> PostUpdateDispelList;
};
