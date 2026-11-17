#pragma once

#include "BSHavok/bhkCollisionFilter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"

class bhkSimpleShapePhantom;
class bhkWorld;
class DecalCaster;
class hkpCdBodyPairCollector;
class NiAVObject;
class NiPoint3;

// The per-world decal caster. PC has no vtable: Add, Remove, GetWorld, Pick, GetInstance and
// the GetMaxDecalWidth getters are all plain or static members.
class DecalCaster {
public:
	// Which bodies a custom pick hits.
	enum CUSTOM_PICK {
		CUSTOM_PICK_ACTORS = 0,
		CUSTOM_PICK_BIPED = 1,
		CUSTOM_PICK_COUNT = 2,
	};

	DecalCaster(float afRadius, uint32_t aiGroup);
	~DecalCaster();

	// Sets whether the custom pick layer collides with aeLayer.
	void SetCollision(COL_LAYER aeLayer, bool abEnable);
	// Points the custom pick layer at the world for actor picks, or at bipeds.
	void SetCustomPickCollisions(CUSTOM_PICK aeCustomPick);
	// The aiIndex-th contact point the last pick collected.
	const hkpRootCdPoint& GetCollision(uint32_t aiIndex);
	// The number of collision points the last pick collected.
	uint32_t QNumCollisions();
	// Sets the radius of the pick phantom's sphere.
	void SetRadius(float afRadius);
	// Casts the pick phantom from apFrom to apTo, collecting the contacts; the collision count is returned.
	uint32_t Pick(const NiPoint3& arFrom, const NiPoint3& arTo, float afRadius);
	// Adds the caster's phantom to apWorld.
	void Add(bhkWorld* apWorld);
	// Collects into arCollector the bodies the caster's phantom, placed at arPos, penetrates.
	void GetPenetrations(const NiPoint3& arPos, hkpCdBodyPairCollector& arCollector);
	// Builds the pick phantom, a sphere of afRadius in collision group aiGroup.
	void Init(float afRadius, uint32_t aiGroup);
	bhkWorld* GetWorld();
	NiAVObject* GetAVObject(uint32_t aiIndex);
	void GetCollisionData(uint32_t aiIndex, NiPoint3& arPos, NiPoint3& arNormal);

	static float GetMaxDecalWidthSmall();
	static float GetMaxDecalWidthLarge();
	// The caster for decals of radius afDecalRadius, created when abCreate.
	static DecalCaster* GetInstance(float afDecalRadius, bool abCreate);
	// Frees with ourMemalignFree.
	static void operator delete(void* apMemory);
	static void ClearInstance();

	// Only the start of the caster is modelled.
	hkpAllCdPointCollector PointCollector;
	// The phantom Pick casts through the world.
	NiPointer<bhkSimpleShapePhantom> spShapePhantom;

	static inline DecalCaster* pSmallInstance;
	static inline DecalCaster* pLargeInstance;
};
