#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BGSExplosion;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class MagicCaster;
class NonActorMagicCaster;
class TESObjectWEAP;
struct hkpRootCdPoint;

// Not named in the PDB or IDA; name chosen here. The SDK's hkAlgorithm::quickSort for hkpRootCdPoint. Its comparator
// (an empty hkAlgorithm::less<hkpRootCdPoint>) travels in a 4-byte stack slot: the exe reads its byte and pushes it
// zero-extended (movzx at 0x9b2eac), and hkAlgorithm::quickSort (0x9B2E00) pushes a zeroed one, so it is a byte
// here, as hkAlgorithm::quickSort's aiLess (hkSort.h). An empty class argument carries no bytes in C++.
// Inferred type: a2: int32_t: signed uses from 0x9b2e9a
void quickSortRootCdPoints(hkpRootCdPoint* a1, int32_t a2, uint8_t aiLess);

// An explosion reference. Only the entry points the reconstructed code calls are declared.
class Explosion : public MobileObject {
public:
	// One limb an explosion reached and its distance; AddLimbTarget allocates 8 bytes and stores aeLimb
	// at +0, afDistance at +4.
	struct ExplodedLimb {
		BGSBodyPart::LIMB_ENUM eLimb;
		float fDistance;
	};

	// One affected reference and its processed limb list; AddTarget allocates 0x10 bytes at 0x9B0E00.
	struct ExplosionTarget {
		ExplosionTarget();
		~ExplosionTarget();

		static void operator delete(void* apMemory) { ourFree(apMemory); }

		TESObjectREFR* pRef;
		bool bHitWeakPoint;
		bool bMobile;
		bool bProcessed;
		uint8_t padding07;
		BSSimpleList<ExplodedLimb*> limbList;
	};

	Explosion();
	Explosion(BGSExplosion* apExplosionBase, TESObjectREFR* apOwnerRef, TESObjectCELL* apCell, NiPoint3 aOrigin,
		NiMatrix3 aRotation);
	~Explosion() override;

	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void SetActorCause(ActorCause* apActorCause) override;
	ActorCause* GetActorCause() override;
	MagicCaster* GetMagicCaster() override;
	void InitHavok() override;
	bool IsExplosion() override;
	void Process(float afDelta) override {}
	bool ChangeProcessLevel() override;
	bool IsTransient() override;
	void Update(float afDelta) override;

	using MobileObject::FinishLoadGame;
	using MobileObject::InitLoadGame;
	using MobileObject::LoadGame;
	using MobileObject::Revert;
	using MobileObject::SaveGame;

	// Sets the explosion up once its 3D is loaded (vtable slot 193, 0x9ACD60).
	virtual void Initialize();

	// Whether the explosion is underwater: bUnderWater.
	bool GetUnderWater();
	// Puts the impact decals of the explosion on what it hit.
	void ApplyImpactDecals();
	// The movement update that may not run while rendering.
	void UpdateNonRenderSafeMovement(float afDelta);
	// Dims the explosion's light as it ages: full at birth, out at half its lifetime.
	void UpdateLight();
	// fAge; the exe's shared one-field getter 0x51B8E0 (thiscall, ST0).
	float GetAge();
	float GetLifetime();
	float GetRadius();
	bool IsDistant();
	// The reference that caused the explosion: pOwner. The exe shares the getter (0x9ACCE0) with other +0xC8 reads.
	TESObjectREFR* GetOwner();
	// Stops and releases both explosion sounds.
	void StopSounds();
	void SetOuterRadius(float a2);
	// The explosion's base form.
	BGSExplosion* GetExplosionBase();
	// The listed target of apTargetRef, its weak-point and mobile flags filled in, or a new target added to the head
	// of the list.
	ExplosionTarget* AddTarget(TESObjectREFR* apTargetRef, bool abWeakPoint, bool abMobile);
	// Sets pWeaponSource.
	void SetWeaponSource(TESObjectWEAP* apSource);
	// Sets fLifetime.
	void SetLifetime(float afValue);
	void ProcessMovement(float a2);
	void Kill();
	// The limbs of apTarget the explosion reached, or null.
	BSSimpleList<ExplodedLimb*>* GetAffectedLimbList(TESObjectREFR* apTarget);
	// Sets ClosestPointNormal.
	void SetClosestPointNormal(NiPoint3 aNormal);
	TESObjectWEAP* GetSourceWeapon();
	// Not named in the PDB or IDA; name chosen here. Attaches the explosion's 3D to the terrain manager's LOD root
	// (thiscall, ret 0); eax at its return is whatever the last call left: the void AttachChild's on the full path.
	void AttachDistant();
	void Init3D();
	void ApplyForces();
	void ProcessTargets();
	void PushActor(Actor* apTargetActor, float afExplosionPower);
	void StartSounds();
	void AttachLight();
	void UpdateRadiation();
	bool RunLOSPick(NiPoint3 aLocation, TESObjectREFR* apTarget);
	bool TestExplosionUnderWater();
	void FindTargets();
	void PushRigidBody(bhkRigidBody* apRigidBody);
	float GetDamage();
	void DetachLight();
	void CheckLimbTarget(Explosion::ExplosionTarget* apTarget, NiAVObject* apTargetObject, float afDistance);
	void AddLimbTarget(Explosion::ExplosionTarget* apTarget, BGSBodyPart::LIMB_ENUM aeLimb, float afDistance);
	void CheckRefDeleted(TESObjectREFR* apDeletedRef);

	// Creates an explosion of apExplosionBase at aOrigin turned by aRotation in apCell, caused by apOwnerRef and
	// centred on apExplodedRef; the explosion, else null.
	static Explosion* SpawnExplosion(BGSExplosion* apExplosionBase, TESObjectREFR* apOwnerRef,
		TESObjectREFR* apExplodedRef, TESObjectCELL* apCell, NiPoint3 aOrigin, NiMatrix3 aRotation);

	float fAge;
	float fLifetime;
	float fBuildTime;
	float fRadius;
	float fImageSpaceRadius;
	float fWeaponCondition;
	NiPointer<bhkSimpleShapePhantom> spPhantom;
	BSSimpleList<ExplosionTarget*> TargetList;
	BSSoundHandle Sound1;
	BSSoundHandle Sound2;
	NiPointer<NiPointLight> spLight;
	TESObjectREFR* pOwner;
	TESObjectREFR* pExplodedRef;
	NiPointer<ActorCause> spActorCause;
	bool bDecalsPlaced;
	bool bTargetsFound;
	bool bTargetsProcessed;
	bool bForcesApplied;
	bool bIgnoreImageSpaceSwap;
	bool bUnderWater;
	NonActorMagicCaster* pCaster;
	TESObjectWEAP* pWeaponSource;
	int32_t iFrameCount;
	NiPoint3 ClosestPoint;
	NiPoint3 ClosestPointNormal;
	float fCalculatedDamage;
};

// The vtables the exe's constructor stores: the primary one and the reference's child-cell one at +0x18.
inline constexpr fnvd::Vtable Explosion_vftable{};
inline constexpr fnvd::Vtable Explosion_TESChildCell_vftable{};
inline float f15_0 = 15.0f;
inline float f64_0 = 64.0f;
inline float f15_0_ = 15.0f;
inline float f5000_0 = 5000.0f;

// INISettingCollection: 0xf4248a calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bReflectExplosions_Water;

// IDA names it by its initial value (f15.0); not in the PDB.
inline float f15_0_11A580C = 15.0f;

// IDA names it by its initial value (f1.0); not in the PDB.
inline float f1_0_11A5810 = 1.0f;

namespace hkAlgorithm {
// The SDK's hkAlgorithm::quickSort<hkpRootCdPoint> as the exe compiled it: sorts pArr[0..iSize) with a zeroed
// hkAlgorithm::less<hkpRootCdPoint> (cdecl, 0x9B2E00).
void quickSort(hkpRootCdPoint* pArr, int32_t iSize);
} // namespace hkAlgorithm
