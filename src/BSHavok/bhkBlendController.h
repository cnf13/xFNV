#pragma once

#include "BSHavok/bhkBlendCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiAVObject;
class NiCloningProcess;
class NiPoint3;
class NiStream;
struct NiUpdateData;

// One key of the blend: the time and the gains. The empty key is BLENDKEY(0): time -1 and the value as a percentage.
struct BLENDKEY {
	BLENDKEY(int32_t aiValue = 0) : fTime(-1.0f), fGain(static_cast<float>(aiValue / 100.0)), fVelGain(fGain) {}

	bool operator==(const BLENDKEY& arKey) const {
		return fTime == arKey.fTime && fGain == arKey.fGain && fVelGain == arKey.fVelGain;
	}

	// IDA name; the PDB's operator== (const), out of line in the exe: NiTLargeArray<BLENDKEY>::SetAt (0xC9A730) calls
	// it on its const element argument.
	int32_t IsEqual(const BLENDKEY* a2) const;
	// IDA name; the PDB's operator!= (const), out of line in the exe (called as IsEqual is).
	int32_t IsDifferent(const BLENDKEY* a2) const;

	// Not named in the PDB or IDA; name chosen here. The `default constructor closure' MSVC emits for
	// `eh vector constructor iterator': BLENDKEY(0) inlined, fTime -1 and both gains 0 / 100 (thiscall, ret 0,
	// 0xC9A6C0; IDA names it BLENDKEY::BLENDKEY). BLENDKEY_BLENDKEY below is its address as a value.
	static void DefaultConstructorClosure(BLENDKEY* apThis);

	float fTime;
	float fGain;
	float fVelGain;
};

// Blends a ragdoll between its animation and physics: the keys (time, gain, velocity gain) scale the collision
// object's gains from one key to the next, and the last key ends the blend.
class bhkBlendController : public NiTimeController {
public:
	// The bits of m_usFlags this class adds to the controller flags in the same word.
	enum BHKCONTROLLERFLAGS {
		RESTOREGAINS_POS = 3,
		CLEARKEYS_POS = 6,
		CLEARFORCE_POS = 7,
	};

	// The get-up blend time per ragdoll part; the exe indexes it with bits 8-12 of the collision filter info.
	using GetUpBlendArray = float[32];
	using pHitBlendA_t = BLENDVALUES[32];
	using pKnockDownBlendA_t = float[32];
	using pHitBlendQuadA_t = BLENDVALUES[32];

	bhkBlendController();

	void Update(NiUpdateData& UpdateData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	// NiObject slots 23 and 24.
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	virtual ~bhkBlendController();
	bool IsTransformController() const override { return true; }
	// NiTimeController's pure virtual, concrete in the exe: vtable slot 44 holds 0xA302A0 (thiscall, ret 0).
	bool TargetIsRequiredType() const override;
	const NiRTTI* GetRTTI() const override;

	// Empties the key list.
	void ClearKeys();
	// Appends arKey to the key list.
	void AddKey(const BLENDKEY& arKey);

protected:
	// The gain and the velocity gain at afTime.
	bool CalculateGain(float afTime, float& arGain, float& arVelGain);
	// Removes the target's force controller, if it has one.
	bool ClearForceController();
	// Gives apCol the gains the blend took over.
	void RestoreOriginalGains(bhkBlendCollisionObject* apCol);
	// Copies the members into apDest, which is null when its allocation failed.
	void CopyMembers(bhkBlendController* pDest, NiCloningProcess& arCloneData);

public:
	void FixStartAndEndTimes();

	// The PDB's DoHit(apObject, afTime, abQuad): starts the hit blend of apObject's ragdoll,
	// with the quadruped blend when abQuad (cdecl, 0xC9B250). The exe passes the caller's whole dword on; bool would
	// be zero-extended.
	static void DoHit(NiAVObject* apObject, float afTime, int32_t abQuad);
	// Knocks apObject's ragdoll down along arDirection (cdecl, 0xC9B670). The exe passes the caller's whole dword on
	// for abKill and abNoForce; bool would be zero-extended.
	static void DoKnockDown(NiAVObject* apObject, const NiPoint3& arDirection, int32_t abKill, float afTime,
		int32_t abNoForce);
	// The stream's factory for this class; a failed allocation gives null.
	static NiObject* CreateObject();
	// Raises the ragdolls of apObject's subtree, keyed by afTime.
	static void DoGetUp(NiAVObject* apObject, float afTime);
	// DoGetUp's walk over apObject's subtree.
	static void DoGetUpRecursive(NiAVObject* apObject, float afTime);

	int32_t iLastIndex;
	NiTLargePrimitiveArray<BLENDKEY> pBlendKeyA;
	float fOriginalGain;
	float fOriginalVelGain;
	int32_t iPriority;

	static NiRTTI ms_RTTI;
	static inline GetUpBlendArray pGetUpBlendA{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
		-1.0f};
	static inline float fPassOutForce = 2.0f;
	static inline float fPassOutTime = 1.2f;
	static inline pHitBlendA_t pHitBlendA{{0.2f, 0.9f}, {0.3f, 0.9f}, {0.2f, 0.8f}, {0.2f, 0.8f}, {0.2f, 0.8f},
		{0.4f, 0.9f}, {0.4f, 0.9f}, {0.4f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.4f, 0.9f}, {0.4f, 0.9f},
		{0.4f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {1.0f, 1.0f},
		{0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f},
		{0.2f, 0.9f}};
	static inline pKnockDownBlendA_t pKnockDownBlendA{0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f,
		0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
	static inline pHitBlendQuadA_t pHitBlendQuadA{{0.2f, 0.9f}, {0.3f, 0.9f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f},
		{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f},
		{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {1.0f, 1.0f},
		{0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f}, {0.2f, 0.9f},
		{0.2f, 0.9f}};
};
inline constexpr fnvd::Vtable bhkBlendController_vftable{};
// The key array's own vtable: Construct writes it after the class' own.
inline constexpr fnvd::Vtable NiTLargePrimitiveArray_BLENDKEY_vftable{};
