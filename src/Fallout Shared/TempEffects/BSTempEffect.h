#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class NiAVObject;
class TESObjectCELL;

// What kind of temp effect an object is.
enum TEMP_EFFECT_TYPE : int32_t {
	TEMP_EFFECT_DECAL = 0,
	TEMP_EFFECT_GEOMETRY_DECAL = 1,
	TEMP_EFFECT_PARTICLE = 2,
	TEMP_EFFECT_DEFAULT = 3,
	MAGIC_HIT_EFFECT_DEFAULT = 4,
	MAGIC_HIT_EFFECT_MODEL = 5,
	MAGIC_HIT_EFFECT_SHADER = 6,
};

// A temporary world effect: a decal, debris, or shader hit, kept by the effect list until it dies.
class BSTempEffect : public NiObject {
public:
	using DecalList = BSSimpleList<NiPointer<NiRefObject>>;

	BSTempEffect(TESObjectCELL* apCell, float afLifetime);
	~BSTempEffect() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	virtual void Initialize() { bInitialized = true; }
	virtual void Detach() {}
	// Advances the effect by afTimeDelta; false once it has finished.
	virtual bool Update(float afTimeDelta);
	virtual NiAVObject* Get3D() { return nullptr; }
	virtual TEMP_EFFECT_TYPE GetType() { return TEMP_EFFECT_DEFAULT; }
	virtual bool CheckShouldSave() { return false; }
	virtual uint16_t GetSaveSize();
	// MSVC lays out a class's overloads in reverse declaration order: the buffer overloads come first in the vtable
	virtual void SaveGame();
	virtual void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	virtual bool LoadGame();
	virtual void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	virtual void InitLoadGame(BGSLoadGameBuffer*) {}
	virtual void FinishLoadGame(BGSLoadGameBuffer*) {}
	// PC-only slot with no Xbox PDB name; the name is IDA's
	virtual bool IsFirstPerson() { return false; }

	// fLifetime, through the exe's shared float getter of +0x8 (thiscall, ret 0, ST0,
	// 0x488D50).
	float GetLifetime();
	// fAge, through the exe's shared float getter of +0x10 (thiscall, ret 0, ST0, 0x621B00).
	float GetAge();
	// pCell, through the exe's shared one-field getter of +0xC (thiscall, ret 0, 0x84E3A0).
	TESObjectCELL* GetCell();

	// Empties the decal list and resets the decal count (defined in ProcessLists.cpp, as in the PDB).
	static void RemoveAllDecals();
	// Removes apDecal from the shared decal list and counts it out; not a member, the decal is a stack argument
	// (cdecl, ret 0, 0x4EE2E0, defined in BGSDecalNode.cpp as in the PDB).
	static void RemoveDecalFromList(BSTempEffect* apDecal);

	float fLifetime;
	TESObjectCELL* pCell;
	float fAge;
	bool bInitialized;

	static NiRTTI ms_RTTI;
	static inline int32_t iNumDecals;
	static DecalList kDecals;
};
inline constexpr fnvd::Vtable BSTempEffect_vftable{};
