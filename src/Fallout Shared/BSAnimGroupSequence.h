#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class BGSSaveGameBuffer;
class TESAnimGroup;
class TESObjectREFR;

// An animation sequence that drives an animation group: the group's controllers follow the sequence's timing, the
// sequence plays the group's sounds and, when it ends, it puts back the objects the group drove.
class BSAnimGroupSequence : public NiControllerSequence {
public:
	BSAnimGroupSequence();
	BSAnimGroupSequence(TESAnimGroup* apAnimGroup, NiControllerSequence* apSeq);
	~BSAnimGroupSequence() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	// Ends the sequence; for the holster and unequip groups it first puts the objects the group's controllers drove
	// back to the sequence's last key.
	bool Deactivate(float afEaseOutTime, bool abTransition) override;

	// Restores the sequence's state from the old save at afTime.
	void LoadGame(float afTime);
	// Restores the sequence's state from the save buffer at afTime.
	void LoadGame(BGSLoadGameBuffer* apBuffer, float afTime);
	// Returns the animation group the sequence's keys belong to.
	TESAnimGroup* GetAnimGroup();
	uint16_t GetSaveSize();
	NiFixedString* GetRootNodeNamePtr();
	bool ResolveTransformInterpolators(NiAVObject* apNode, uint32_t aiTransformControllerCount);
	// Inferred type: return: double: used as one at 0x948e83
	double GetKeyLengthOverFrequency();
	// Looks up the priority of the group tag apTag; true when it is known.
	uint8_t GetPriority(const char* apBoneName, uint32_t* apReturnIndex);
	// Inferred type: a2: uint32_t: unsigned uses from 0x5f4be0
	void SetInterpolatorPriority(uint32_t a2, uint8_t a3);
	// Inferred type: a2: uint32_t: unsigned uses from 0x5f4c12
	uint8_t GetInterpolatorPriority(uint32_t a2);
	void SetAnimGroup(TESAnimGroup* apAnimGroup);
	void ClearBlendInterp();
	float GetScaledTime(float afTime);
	void ValidateTargets();
	void ValidateTargetsFromPalette();
	// Inferred type: a2: float from 0x49726e (parameter 0 of AnimIdle::SaveGame)
	void SaveGame(float a2);
	// Inferred type: a2: BGSSaveGameBuffer* from 0x49afcb (parameter 0 of Animation::SaveGameBGS); a3: float from
	// 0x49afcb (Animation::time (this of Animation::SaveGameBGS))
	void SaveGame(BGSSaveGameBuffer* a2, float a3);
	// Empties apDest's interpolators and frees its interpolator and tag arrays, then NiControllerSequence::CopyMembers
	// of this onto apDest.
	void CopyMembers(BSAnimGroupSequence* pDest, NiCloningProcess& arCloneData);

	// The save size of a sequence; the version check adds 4 bytes for saves before version 0x71. GetSaveSize calls
	// it out of line.
	static uint16_t GetSequenceSaveSize();
	static bool PlaySounds(NiControllerSequence* apSeq, TESObjectREFR* apRef);
	// a2: KFModel::InitSequence's PDB const char*, passed on to the C strrchr (0xEC6E30, const char*).
	static void* InitAnimGroup(BSAnimGroupSequence* apSequence, const char* a2);
	// Reads sequence uiIndex of kStream through NiControllerSequence's
	// CreateSequenceFromFile, wraps it in a new BSAnimGroupSequence (no anim group) and stores that in
	// aspControllerSequence.
	static void CreateSequenceFromFile(const NiStream& kStream, uint32_t uiIndex,
		NiPointer<NiControllerSequence>& aspControllerSequence);

	// The animation group the sequence's keys belong to.
	NiPointer<TESAnimGroup> spAnimGroup;

	// The class's RTTI record; filled in when the engine starts up.
	static NiRTTI ms_RTTI;
};

NiSmartPointer(BSAnimGroupSequence);

using animGroupActionNames_t = const char* [5][11];
inline animGroupActionNames_t animGroupActionNames{
	{"Start", "Start", "Start", "Start", "Start", "Start", "Start", "Start", "Start", "Start", "Start"},
	{"End", "End", "StartLoop", "Attach", "Detach", "Hit", "Hit", "Hold", "Release", "Fire", "Hit"},
	{"", "", "EndLoop", "End", "End", "Eject", "End", "Release", "Attach", "Loop", "End"},
	{"", "", "End", "", "", "a:", "", "Attach", "End", "End", ""}, {"", "", "", "", "", "End", "", "End", "", "", ""}};
using animEnumNames_t = char* [31];
inline animEnumNames_t animEnumNames{const_cast<char*>("Left"), const_cast<char*>("Right"),
	const_cast<char*>("BackLeft"), const_cast<char*>("BackRight"), const_cast<char*>("Idle"),
	const_cast<char*>("Aware"), const_cast<char*>("Attack"), const_cast<char*>("Hit"), const_cast<char*>("Death"),
	const_cast<char*>("Equip"), const_cast<char*>("Unequip"), const_cast<char*>("CameraShake"),
	const_cast<char*>("StopSounds"), const_cast<char*>("HitShader"), const_cast<char*>("RiderSoft"),
	const_cast<char*>("RiderLoud"), const_cast<char*>("Weapon1"), const_cast<char*>("FoleySneak"),
	const_cast<char*>("FoleyWalk"), const_cast<char*>("FoleyRun"), const_cast<char*>("ArmorSneak"),
	const_cast<char*>("ArmorWalk"), const_cast<char*>("ArmorRun"), const_cast<char*>("Auxiliary1"),
	const_cast<char*>("Auxiliary2"), const_cast<char*>("Auxiliary3"), const_cast<char*>("Auxiliary4"),
	const_cast<char*>("Auxiliary5"), const_cast<char*>("Auxiliary6"), const_cast<char*>("Auxiliary7"),
	const_cast<char*>("Auxiliary8")};
inline constexpr fnvd::Vtable BSAnimGroupSequence_vftable{};
