#pragma once

#include "BSCore/BSReadWriteLock.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class CombatController;
class CombatCoverLocation;
class PathingCoverLocation;
class ReservedCoverLocation;

uint8_t BSMIN(uint8_t a1, uint8_t a2);

// Where an actor stands along a cover edge: the edge's segments and the segment and width the actor takes. The
// exe zero-extends the byte fields (movzx).
class CoverReservationInfo {
public:
	CoverReservationInfo();

	// The actor's distance along the edge: the middle of its width, from its segment, in segment lengths (thiscall,
	// ST0 float, 0x9A95F0).
	float GetOffset();
	uint32_t GetCoverMask();
	// The segments an actor of afActorRadius takes: at least one (thiscall, ret 4, the byte in al, 0x982750).
	char CalculateActorWidth(float afActorRadius);
	void Clear();
	void CalculateSegmentInfo(PathingCoverLocation* apCoverLocation);
	bool FindCoverReservation(PathingCoverLocation* apCoverLocation, CombatController* apCombatController,
		float afActorRadius, ReservedCoverLocation* apReservedCoverLocation, bool abHideCover, bool abLeftEdge);
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

	uint8_t cSegmentLength;
	uint8_t cSegmentCount;
	uint8_t cActorWidth;
	uint8_t cSegment;
	bool bHideCover;
	bool bLeftEdge;
};

// One combat controller's claim on a cover location.
struct CoverReservation {
	// Same function as GroupMemberData::Construct (0x4B0680), registered there: stores the two pointers
	// (thiscall, ret 8).
	// The exe's default constructor is empty (0x6815C0, run by BSSimpleArray's _ConstructItems 0x6DC3A0).
	CoverReservation() {}
	CoverReservation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation);

	CombatController* pCombatController;
	CombatCoverLocation* pCombatCoverLocation;
};

// A cover location with the claims on its segments, as the combat cover map tracks it.
class ReservedCoverLocation {
public:
	// The constructor.
	ReservedCoverLocation();
	~ReservedCoverLocation();

	// Copies the segment info from aInfo.
	void CopySegmentInfo(CoverReservationInfo& arReservationInfo);
	// Reserves the cover location for apController.
	void ReserveCoverLocation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation);
	// Tries to reserve the cover location for apController.
	bool TryReserveCoverLocation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation);
	// Whether apController holds a reservation.
	bool CanReserveCoverLocation(CombatController* apCombatController);
	void SetCoverHeight(uint8_t acCoverHeight);
	bool UnreserveCoverLocation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSSimpleArray<CoverReservation> Reservations;
	uint32_t iReservations;
	uint8_t cSegmentLength;
	uint8_t cSegmentCount;
	uint8_t cCoverHeight;
	uint8_t padding17;
	NiPointer<NiNode> spDebugGeometry;
};

// The cover locations in use, by key, with the claims on each; the read-write lock guards the map.
class CombatCoverMap : public NiTMap<uint32_t, ReservedCoverLocation*> {
public:
	// Runs the exe's constructor (thiscall, ret 0, 0x983080) on apMemory. Returns apMemory.
	CombatCoverMap();
	~CombatCoverMap() override;

	bool IsCurrentThreadWriting();
	void UnlockForWrite();
	void LockForWrite();
	// Reserves apLocation for apController.
	void ReserveCoverLocation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation);
	// Reserves apLocation for apController if it is free.
	bool TryReserveCoverLocation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation);
	// Whether apCombatController (radius afActorRadius) can reserve the side abLeftEdge of
	// apCoverLocation under key aiKey; fills apReservationInfo.
	bool CanReserveCoverLocation(uint32_t aiKey, PathingCoverLocation* apCoverLocation,
		CombatController* apCombatController, float afActorRadius, CoverReservationInfo* apReservationInfo,
		bool abHideCover, bool abLeftEdge);
	// Releases apLocation held by apController (thiscall, ret 8, 0x983520).
	// Inferred type: apController: CombatController* from 0x9a05e7 (CombatState::pCombatController (this of
	// CombatState::UnreserveCoverLocation)); apLocation: CombatCoverLocation* from 0x9a05e7 (parameter 0 of
	// CombatState::UnreserveCoverLocation), its members used at 0x98352c
	void UnreserveCoverLocation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	NiPointer<NiNode> spCoverSearchDebugGeometry;
	BSReadWriteLock Lock;
};

inline constexpr fnvd::Vtable CombatCoverMap_vftable{};

// The game settings CombatCoverMap.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bDebugCombatCoverSearch;
extern SettingT<INISettingCollection> bDebugCombatCoverReservations;
extern SettingT<INISettingCollection> bDebugCombatCoverSearch2;
extern SettingT<INISettingCollection> bDebugCombatCoverReservationsText;
extern SettingT<INISettingCollection> bDebugCombatCoverSearchText;
