#pragma once

#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Reconstruction.hpp"

class ActiveEffect;
class Actor;
class EffectItem;
class MagicLight;
class MagicProjectile;
class MagicTarget;
class MobileObject;
class NiNode;
class SpellItem;
class TESBoundObject;
class TESObjectCELL;
class TESObjectREFR;
class hkpRigidBody;

// Interface of things that cast magic (Actor, projectiles, ...): the light of the cast effect and the casting state.
// No virtual destructor in the exe.
class MagicCaster {
	// ExtraDataList::LoadGame sets a loaded caster's spell and target through the protected virtuals.
	friend class ExtraDataList;

public:
	enum State : uint32_t {
		NO_SPELL = 0,
		AIM = 1,
		CAST = 2,
		RELEASE = 3,
		FIND_TARGETS = 4,
		ERR_SPELL_DISABLED = 5,
		ERR_ALREADY_CASTING = 6,
		ERR_CANNOT_CAST = 7,
	};

	MagicCaster();
	// Not virtual: the exe's vtable has no destructor slot.
	~MagicCaster();

	virtual void CastAbility(SpellItem* apSpell, bool abLoadCast);
	virtual void CastAddiction(SpellItem* apSpell, bool abLoadCast);
	virtual void CastRadiation(SpellItem* apSpell, bool abLoadCast);
	virtual void CastSpellImmediate(MagicItem* apSpell, bool abLoadCast, MagicTarget* apDesiredTarget,
		float afEffectivenessMult, bool abAdjustOnlyHostileEffectiveness);
	virtual void TransferDisease(SpellItem* apSpell, MagicTarget* apDesiredTarget, bool abLoadCast);
	virtual void CastWornEnchantment(MagicItem* apSpell, TESBoundObject* apSource, bool abLoadCast);
	virtual MagicTarget* GetTouchTarget();
	virtual void StartAim();
	virtual void StartCast() { ReleaseCast(nullptr); }
	virtual void SpellCast(MagicItem*, bool) {}
	virtual bool CheckCast(MagicItem* apSpell, float* afEffectStrength, MagicItem::CannotCastReason* apReason,
		bool abUseBaseMagicka);
	virtual TESObjectREFR* GetCasterStatsObject() { return nullptr; }
	virtual NiNode* GetMagicNode() { return nullptr; }
	virtual MagicItem* GetCurrentSpell() { return nullptr; }

protected:
	virtual bool TargetSelf(ActiveEffect* apActiveEffect);
	virtual float GetMagicEffectivenessModifier(bool, float) { return 1.0f; }
	virtual void SetCurrentSpell(MagicItem*) {}
	virtual MagicTarget* GetDesiredTarget() { return nullptr; }
	virtual void SetDesiredTarget(MagicTarget*) {}
	virtual ActiveEffect* CreateActiveEffect(MagicItem* apSpell, EffectItem* apEffectItem, TESBoundObject* apSource);

public:
	// Casts apSpell at apDesiredTarget.
	bool CastSpell(MagicItem* apSpell, MagicTarget* apDesiredTarget, bool abLoadCast);
	// The caster as an Actor when its stats object is an actor, else null.
	Actor* GetCasterAsActor();
	// Applies the current spell's self and touch effects, explodes area effects and fires target effects, then
	// clears the current spell and target.
	void FindTargets(float afEffectStrength, TESBoundObject* apSource, bool abLoadCast, float afEffectivenessMult,
		bool abAdjustOnlyHostileEffectiveness);
	// Explodes apDominantEffect's area at aSourceLoc in apCell, collecting the hit actors in
	// aTargetList.
	void DoExplosion(NiPoint3 aSourceLoc, TESObjectCELL* apCell, MagicItem* apSpell, EffectItem* apDominantEffect,
		BSSimpleList<MobileObject*>* aTargetList, float afPowerScale, float afAreaScale);
	// Blows apBody apart from aSourceLoc: the impulse points away from the source and its strength comes from the
	// spell cost, the area in feet and the distance.
	void ExplodeRigidBody(hkpRigidBody* apRigidBody, NiPoint3 aSourceLoc, EffectItem* apDominantEffect,
		float afSpellCost);
	// Blows apTargetActor apart from aSourceLoc.
	void ExplodeActor(Actor* apTargetActor, NiPoint3 aSourceLoc, EffectItem* apDominantEffect, float afSpellCost);
	// Whether apSpell kills apTarget.
	bool CheckWillKill(MagicTarget* apTarget, MagicItem* apSpell);
	// Moves and fades the cast light over afDelta.
	void UpdateLight(float afDelta);
	bool ReleaseCast(TESBoundObject* apSource);
	void EndAim();
	// Stops the cast: clears the actor's magic node and the cast light, then fails the current spell and releases
	// its effect model when the magic node had children.
	void InterruptCast();
	// The caster's form ID for the save game.
	uint32_t GetMagicCasterFormID();

protected:
	// Private in the PDB.
	bool AddTarget(MagicTarget* apTarget, NiPoint3 aSourceLoc, MagicItem* apSpell, ActiveEffect* apActiveEffect,
		bool abAreaTarget);
	// Fires the current spell's target effects as a projectile.
	void FireProjectile();
	// Private in the PDB. Applies apActiveEffect to the targets in its area around
	// aLocation, skipping apTarget and those in aTargetList; sets abSkillAlreadyUsed when one of them used the skill
	// (thiscall, ret 0x2C, 0x816F10).
	void FindTargetsInArea(MagicItem* apSpell, ActiveEffect* apActiveEffect, TESObjectCELL* apCell, NiPoint3 aLocation,
		MagicTarget* apTarget, MagicProjectile* apProjectile, BSSimpleList<MobileObject*>* aTargetList,
		bool& abSkillAlreadyUsed, float afAreaScale);
	// Private in the PDB. Adds the actors in the line of sight of aSpellLocation within
	// apSpell's area of range aeRange to apActorList (thiscall, ret 0x18, 0x818CE0).
	void FindExplosionLOS(MagicItem* apSpell, MagicSystem::Range aeRange, NiPoint3 aSpellLocation,
		BSSimpleList<MobileObject*>* apActorList);
	// Private in the PDB. True when nothing static blocks the ray from aLocation to the middle of apTarget.
	bool RunExplosionLOSPick(NiPoint3 aLocation, Actor* apTarget);
	// Private in the PDB.
	void SetState(State aeState, const char* acMessage);

public:
	// Inferred type: apCell: TESObjectCELL* from 0x810d98 (TESObjectREFR::pParentCell (this of
	// MagicBallProjectile::Collide)); apProjectile: MagicProjectile* from 0x810d98 (this of
	// MagicBallProjectile::Collide), its members used at 0x817314; afPowerScale: float from 0x817501 (passed to
	// MagicCaster::DoExplosion 5); afAreaScale: float from 0x81741e (passed to MagicCaster::FindTargetsInArea 8)
	void FindTargets_0(MagicItem* apSpell, TESObjectCELL* apCell, NiPoint3 aLocation, MagicProjectile* apProjectile,
		TESObjectREFR* apCollidee, bool abLoadCast, float afPowerScale, float afAreaScale);

	static MagicCaster* GetMagicCasterByNumericID(uint32_t aiFormID);

	// The light attached to the spell while it is being cast.
	MagicLight* pLight;

	// Where the cast is in its cycle.
	State eState;
};
inline constexpr fnvd::Vtable MagicCaster_vftable{};
