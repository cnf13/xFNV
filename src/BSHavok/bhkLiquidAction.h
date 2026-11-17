#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkUnaryAction.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

#include <stdint.h>

class bhkLiquidAction;
struct hkLiquidActionCinfo;
class hkpSimpleContactConstraintData;

int32_t getContactPointIdAt(hkpSimpleContactConstraintData* apThis, [[maybe_unused]] void* apEdx, int32_t a2);

// The actions following one target object, and the map from a target object to its list of them.
using bhkLiquidActionList = BSSimpleList<bhkLiquidAction*>;
using bhkLiquidActionListMap = NiTMap<NiAVObject*, bhkLiquidActionList*>;

// A Havok action that makes a body behave like a liquid: it holds the body to a target object, pulls it towards its
// neighbours and sticks it to a point and normal.
class bhkLiquidAction : public bhkUnaryAction {
public:
	bhkLiquidAction();
	bhkLiquidAction(const hkLiquidActionCinfo& arInfo);
	~bhkLiquidAction() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	// Reads the serialized form of the cinfo.
	int32_t LoadCInfo(NiStream& arStream) override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	// Sticks, damps and pushes the body each step, and hands the force to the Havok action.
	void Update(float afDelta) override;

	// Makes apObject the object the action follows and records the action in the object's list.
	void SetTargetObject(NiAVObject* apObject);

	// The factory the stream's type table holds: a new action in the engine's heap, built in place.
	static NiObject* CreateObject();

	NiAVObject* pTargetObject;
	float fNeighborDistance;
	float fNeighborStrength;
	float fInitialStickForce;
	float fStickStrength;
	NiPoint3 StickPoint;
	NiPoint3 StickNormal;
	bool bFirstUpdate;
	uint8_t padding3D[3];

	static NiRTTI ms_RTTI;
	// The actions following each target object, keyed by the object; the value is the list of actions on it.
	static bhkLiquidActionListMap LiquidListMap;
	static inline int32_t iCount;
};

// The creation info of a liquid action: the unary action's user data and body, then the liquid's four strengths. It has
// no vtable; it is allocated aligned to 16, with the padding length in the byte before the object.
struct hkLiquidActionCinfo : public hkUnaryActionCinfo {
	// The serialized form (bhkSerialized.h): the 32-bit layout, 0x18 bytes.
	struct Serialized : hkUnaryActionCinfo::Serialized {
		float fInitialStickForce;
		float fStickStrength;
		float fNeighborDistance;
		float fNeighborStrength;
	};

	hkLiquidActionCinfo();

	// Fills the info from its serialized form; the user data and the body are left empty.
	void ReadSerialized(const Serialized& arForm) {
		hkUnaryActionCinfo::ReadSerialized(arForm);
		fInitialStickForce = arForm.fInitialStickForce;
		fStickStrength = arForm.fStickStrength;
		fNeighborDistance = arForm.fNeighborDistance;
		fNeighborStrength = arForm.fNeighborStrength;
	}

	float fInitialStickForce;
	float fStickStrength;
	float fNeighborDistance;
	float fNeighborStrength;
};

inline float fNeighborMaxPercentage = 0.15f;
inline float fgravityfactor = 0.99f;
inline float fdampenNormal = 1.0f;
inline float fdampenStick = 1.0f;
inline float fgravity = -98.100006f;
inline bool balwaysAntiGrav;

static_assert(offsetof(hkLiquidActionCinfo::Serialized, fInitialStickForce) == 0x8);
static_assert(offsetof(hkLiquidActionCinfo::Serialized, fNeighborStrength) == 0x14);
static_assert(sizeof(hkLiquidActionCinfo::Serialized) == 0x18);
