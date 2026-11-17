#pragma once

#include "BSHavok/bhkSerializable.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <stdint.h>

class hkpConstraintData;
class hkpConstraintMotor;
class hkpEntity;
class NiCloningProcess;
class NiStream;

// A missile projectile: one that flies along a ballistic or straight path. Only some of its entry points are declared.
class MissileProjectile : public Projectile {
public:
	enum ImpactResult : int32_t {
		IR_NONE = 0x0,
		IR_DESTROY = 0x1,
		IR_BOUNCE = 0x2,
		IR_IMPALE = 0x3,
		IR_STICK = 0x4,
	};

	// Constructs the projectile, with a process when abAdd.
	explicit MissileProjectile(bool abAdd);
	MissileProjectile(BGSProjectile* a1, TESObjectREFR* a2, TESObjectWEAP* a3, NiPoint3 a4, float a5, float a6);

	// A projectile that stuck in something stays.
	bool IsPermanent() override;
	// Saves the projectile, then eImpactResult.
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	// Loads the projectile, then eImpactResult.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	using Projectile::InitLoadGame;
	// Clears eImpactResult after the projectile's revert.
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	using Projectile::Revert;
	void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	using Projectile::FinishLoadGame;
	Projectile::PROJECTILE_TYPE GetProjectileType() override;
	void Initialize() override;
	void Process3D() override;
	void UpdateProjectile(float afDelta) override;
	bool ProcessImpacts() override;
	~MissileProjectile() override;

protected:
	bool AddImpact(TESObjectREFR* apCollidee, const NiPoint3& aContactPoint, const NiPoint3& aContactNormal,
		const hkpCollidable* apTargetCollidable, uint32_t aiKey, HavokMaterialType aeForceMaterial) override;

public:
	void Initialize3D();
	// Picks eImpactResult after an impact is added.
	void DetermineImpactResult();
	ImpactResult GetImpactResult() { return eImpactResult; }
	// The bounce after an impact.
	bool ProcessBounce();
	// Impales the hit target.
	void ProcessImpale();
	// Sticks into the hit surface.
	void ProcessStick();

	ImpactResult eImpactResult;
	bool bWaitingToInitialize3D;
	float fStickTime;
	float fLocalRot;
};

// The creation info of the constraint that ties a missile to the entity it hit: the constraint data, its priority and
// the two entities.
class hkConstraintCinfo : public hkSerializableCinfo {
public:
	hkConstraintCinfo();
	virtual ~hkConstraintCinfo();

	virtual void CloneData(hkpConstraintData* apData, NiCloningProcess&);
	virtual void LoadBinary(NiStream& arStream);
	virtual void SaveBinary(NiStream& arStream);
	virtual uint32_t CreateData();
	virtual void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings);

	// Stores the entity wrapped by apEntity (null for none) as entity B.
	void SetEntityB(bhkEntity* apEntity);
	// Stores the entity wrapped by apEntity (null for none) as entity A.
	void SetEntityA(bhkEntity* apEntity);
	void SetEntities(bhkEntity* a1, bhkEntity* a2);
	void SetData(hkpConstraintData* apData);
	// Reads a motor from the stream, null when it holds none; the caller owns the reference.
	class hkpConstraintMotor* LoadMotor(NiStream& arStream);
	void SaveMotor(hkpConstraintMotor* apMotor, NiStream& arStream);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	hkpConstraintData* pData;
	hkpConstraintInstance::ConstraintPriority ePriority;
	hkpEntity* pEntityA;
	hkpEntity* pEntityB;
};
