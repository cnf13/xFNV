#pragma once

#include "Fallout Shared/BGSWorldLocation.h"
#include "Reconstruction.hpp"

class BGSSaveFormBuffer;
class TESForm;
class TESObjectREFR;

// A location in combat: a world location, the reference it belongs to and a radius around it.
class CombatLocation {
public:
	CombatLocation();

	void SetRadius(float afRadius);
	bool GetFlag(uint32_t aiFlag);
	// The space of the location.
	TESForm* GetSpace();
	void Copy(const CombatLocation* apCopy);
	void SetRef(TESObjectREFR* apRef);
	void UpdateRefLocation();
	void SetFlag(uint32_t aiFlag, bool abValue);
	void SetPoint(const NiPoint3& arPoint, TESForm* apSpace);
	// Copies arLocation into WorldLoc.
	void SetLocation(const BGSWorldLocation& arLocation);
	// Sets the world location's space.
	void SetSpace(TESForm* apSpace);
	// Its caller, CombatState::InitLoadGame, passes its own load buffer.
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Reads the world location, the location type, the radius, the flags, then the reference's form ID (into
	// pRef, resolved later).
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Name not in the PDB (the field at +0x14); folded accessor, IDA: BSTimer::GetTime.
	int32_t GetLocation();
	// Name not in the PDB (the field at +0x10); folded accessor, IDA: TESMain::GetThread.
	TESObjectREFR* GetRef();
	const NiPoint3& GetPoint();
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);

	BGSWorldLocation WorldLoc;
	TESObjectREFR* pRef;
	int32_t iLocation;
	float fRadius;
	uint32_t iFlags;
};

// The per-location-type data the combat AI reads through the kCombatLocationTypeData array (PDB: CombatLocationInfo,
// sizeof 8; IDA's struct has the same five members). The PDB names no source file for it; it sits beside
// CombatLocation, which the three users (CombatState.cpp, CombatProcedureMove.cpp, CombatController.cpp) include.
struct CombatLocationInfo {
	bool bRecheckLocation;
	bool bUsesMovementCost;
	bool bUseCoveredMovement;
	bool bAvoidTargets;
	float fRadius;
};
