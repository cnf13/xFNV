#pragma once

#include "BSCore/BSReadWriteLock.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

#include <stdint.h>

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class CombatController;
class CombatCoverLocation;
class PathingCoverLocation;
class ReservedCoverLocation;

uint8_t BSMIN(uint8_t a1, uint8_t a2);

// Where an actor stands along a cover edge: the edge's segments and the segment and width the actor takes. The byte
// fields are unsigned.
class CoverReservationInfo {
public:
	CoverReservationInfo();

	// The actor's distance along the edge: the middle of its width, from its segment, in segment lengths.
	float GetOffset();
	uint32_t GetCoverMask();
	// The segments an actor of afActorRadius takes: at least one.
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
	// The default constructor is deliberately empty and leaves the members uninitialised, as the game does;
	// BSSimpleArray runs it when constructing items.
	CoverReservation() {}
	CoverReservation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation);

	CombatController* pCombatController;
	CombatCoverLocation* pCombatCoverLocation;
};

// A cover location with the claims on its segments, as the combat cover map tracks it.
class ReservedCoverLocation {
public:
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
	// Releases apCombatCoverLocation held by apCombatController.
	void UnreserveCoverLocation(CombatController* apCombatController, CombatCoverLocation* apCombatCoverLocation);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	NiPointer<NiNode> spCoverSearchDebugGeometry;
	BSReadWriteLock Lock;
};

extern SettingT<INISettingCollection> bDebugCombatCoverSearch;
extern SettingT<INISettingCollection> bDebugCombatCoverReservations;
extern SettingT<INISettingCollection> bDebugCombatCoverSearch2;
extern SettingT<INISettingCollection> bDebugCombatCoverReservationsText;
extern SettingT<INISettingCollection> bDebugCombatCoverSearchText;
