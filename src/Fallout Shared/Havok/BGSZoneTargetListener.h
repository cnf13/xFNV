#pragma once

#include "BSHavok/bhkTTNode.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

struct hkpRootCdPoint;
class TargetEntry;

void ourMemalignFree(void* apPtr);

// A zone (a trigger or a body of water) a BGSZoneTargetListener tracks, with the references inside it.
class ZoneEntry : public bhkTTNode<TESObjectREFR> {
public:
	ZoneEntry(TESObjectREFR* apRefr, const hkpCollidable* apCollidable);
	~ZoneEntry();

	virtual TargetEntry* AddTarget(TESObjectREFR* apTarget, const hkpCollidable* apCol, uint32_t aiFlags);
	virtual void Run() {}

	// The zone entry of apCollidable.
	ZoneEntry* GetZoneEntry(const hkpCollidable* apCollidable);
	// Removes apTarget from the zone's targets and frees it.
	void RemoveTargetEntry(TargetEntry* apTarget);
	// Adds apTarget, hit through apCol, to the zone's trap targets with aiFlags.
	TargetEntry* AddTrapTarget(TESObjectREFR* apTarget, const hkpCollidable* apCol, uint32_t aiFlags);
	// The zone entry of apZone, or null.
	ZoneEntry* GetZoneEntry(TESObjectREFR* apZone);
	TargetEntry* GetTargetEntry(TESObjectREFR* apTarget);

	void RemoveTarget(TESObjectREFR* apTarget);
	// The next zone entry, or null.
	ZoneEntry* QNextEntry();
	TargetEntry* GetTargets();
	// Frees every target entry and empties the list.
	void EmptyList();

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	TargetEntry* pTargets;
};

// A reference inside a zone: the node it was found by, where and how fast it was when it hit, and what it hit. The
// hkVector4 members make the class 16-aligned.
class TargetEntry : public bhkTTNode<TESObjectREFR> {
public:
	enum TARGET_FLAGS : int32_t {
		TGE_HIT = 0x1,
	};

	TargetEntry(TESObjectREFR* apTarget, const hkpCollidable* apCollidable);

	virtual void Run(ZoneEntry* apTrap) {}

	// The entry of apTarget in the target list.
	TargetEntry* GetTargetEntry(TESObjectREFR* apTarget);
	// PositionAtHit = arPos.
	void SetPositionAtHit(const hkVector4& arPos);
	// VelocityAtHit = arVel.
	void SetVelocityAtHit(const hkVector4& arVel);
	// eMaterialAtHit = aeMat.
	void SetMaterialAtHit(const HavokMaterialType aeMat) { eMaterialAtHit = aeMat; }
	int32_t GetHit();
	void SetHit(bool abTF);

	// Frees with ourMemalignFree.
	static void operator delete(void* apMemory);

	hkVector4 PositionAtHit;
	hkVector4 VelocityAtHit;
	// The constructor sets eMaterialAtHit to BHK_MATERIAL_MAX.
	HavokMaterialType eMaterialAtHit;
};

// Tracks which references are inside which zones, for the trap and water listeners.
class BGSZoneTargetListener {
public:
	BGSZoneTargetListener();
	virtual ~BGSZoneTargetListener();

	virtual ZoneEntry* CreateZoneEntry(TESObjectREFR* apZone, const hkpCollidable* apCol);
	virtual TargetEntry* CreateTargetEntry(TESObjectREFR* apZone, const hkpCollidable* apCol);

	// Drops apTarget from every zone.
	void RemoveTarget(TESObjectREFR* apTarget);
	// Removes apEntry from the zones.
	void RemoveZoneEntry(ZoneEntry* apZone);
	// Removes the zone of apZone.
	void RemoveZone(TESObjectREFR* apZone);
	// Removes apReference from the targets of apEntry.
	void RemoveTargetFromZone(ZoneEntry* apZone, TESObjectREFR* apTarget);

	ZoneEntry* GetZoneEntry(TESObjectREFR* apZone);
	// Returns pZones.
	ZoneEntry* QZoneEntryList();
	void EmptyList();
	// The zone entry of apZone, added for apCol when it is missing.
	ZoneEntry* AddZone(TESObjectREFR* apZone, const hkpCollidable* apCol);
	// Adds the pair a contact point joins: apZone's entry and, in it, apTarget (a trap target when abTrap) with
	// aiFlags, the hit's position, velocity and material. Returns apZone's entry, or null.
	ZoneEntry* AddZoneTargetPair(const hkpRootCdPoint& arPoint, TESObjectREFR* apZone, TESObjectREFR* apTarget,
		uint32_t aiFlags, bool abTrap);
	ZoneEntry* RemoveTargetFromZone_00620680(const hkpCollidable* apZone, TESObjectREFR* apTarget);

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	ZoneEntry* pZones;
};
