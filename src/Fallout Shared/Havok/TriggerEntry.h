#pragma once

#include "BSCore/BSList.h"
#include "BSHavok/bhkTTNode.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A trigger volume's phantom node and the references currently inside it (the PDB lists the class under
// TESTrapListener.cpp, whose header is not in src).
class TriggerEntry : public bhkTTNode<TESObjectREFR> {
public:
	enum TriggerStatus : int32_t {
		TRIGGER_STATUS_NONE = 0x0,
		TRIGGER_STATUS_ENTER = 0x1,
		TRIGGER_STATUS_EXIT = 0x2,
	};

	// A reference entering (or leaving) the volume, queued in RefList::EventList.
	struct TriggerEvent {
		uint32_t RefID;
		TriggerStatus eStatus;
	};

	// The references inside, as a growable array, and the events pending for them.
	struct RefList {
		// One reference in the volume.
		struct RefStatus {
			uint32_t RefID;
			bool bInsideTrigger;
		};

		RefList();
		~RefList();

		// Whether apRef is in the list.
		bool IsInList(TESObjectREFR* apRef);
		void Reset();
		void ForceLeaveTrigger(TESObjectREFR* apTriggerRef);
		bool Add(TESObjectREFR* apRef);
		TriggerEntry::TriggerEvent* GetNewEvent();
		void UpdateExitList();

		RefStatus* pRefArray;
		int32_t iArraySize;
		int32_t iArrayMaxSize;
		BSList<TriggerEvent*> EventList;
	};

	TriggerEntry(TESObjectREFR* apRef, const hkpCollidable* apCol);
	~TriggerEntry();

	// Clears the references currently inside the trigger.
	void ClearCurrentRefs();
	// The next trigger entry in the list.
	TriggerEntry* QNextEntry();
	// The trigger's entry for apTriggerRef, via bhkTTNode::GetEntry (thiscall, ret 4, 0x62DF60). Not
	// GetTriggerEntry, which is 0x62DB40's name.
	TriggerEntry* GetZoneEntry(TESObjectREFR* apRef);
	bool IsInTrigger(TESObjectREFR* apRef);
	// The trigger entry in the list from this one whose collidable is apCol, or null.
	TriggerEntry* GetTriggerEntry(const hkpCollidable* apCol);
	// Name not in the PDB (the field at +0xC: pRef); folded accessor, IDA: this_C.
	TESObjectREFR* GetRef();
	// Collects the references inside the trigger and runs their scripts' trigger events; whether any fired.
	bool Update(float afDelta);

	static void operator delete(void* apMemory);

	RefList CurrentRefs;
};
