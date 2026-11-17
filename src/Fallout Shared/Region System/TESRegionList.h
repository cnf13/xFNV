#pragma once

#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiPoint2;
class NiPoint3;
enum REGION_DATA_ID : int32_t;
class TESRegion;
class TESRegionData;
class TESRegionPoint;
class TESWorldSpace;

// A list of regions. The engine's class is polymorphic, so its vtable pointer comes before the list.
class TESRegionList : public BSSimpleList<TESRegion*> {
public:
	// The region data and the list of one region data kind (the PDB's TESRegionList::CachedData; IDA's pData at
	// +0x0, pList at +0x4). The exe keeps nine of them, indexed by REGION_DATA_ID, at 0x11C7F80 (IDA:
	// TESRegionList::kCachedData; the PDB's TESRegionList::pCachedData; 0x4F6320, 0x4F64B0, 0x4F6850).
	struct CachedData {
		TESRegionData* pData;
		TESRegionList* pList;
	};

	using kCachedData_t = TESRegionList::CachedData[9];

	// Constructs a list in apMemory (the exe's constructor, thiscall, ret 4, 0x4F6320).
	TESRegionList(bool abOwnsRegionMemory);
	virtual ~TESRegionList();

	// The region data of kind aeFind for apPoint in apWorldSpace.
	TESRegionData* GetDerivedData(REGION_DATA_ID aeFind, TESRegionPoint* apPoint, TESWorldSpace* apWorldSpace);
	// The region data of kind aeFind at arPoint in apWorldSpace, through a TESRegionPoint copy of the point.
	TESRegionData* GetDerivedData(REGION_DATA_ID aeFind, const NiPoint2& arPoint, TESWorldSpace* apWorldSpace);
	// The PDB's NiPoint3 overload, folded into the NiPoint2 one (both read only x and y); GetMapNameForLocation
	// (0x586500) and GetGrassForLocation (0x586990) call it with a NiPoint3.
	TESRegionData* GetDerivedData(REGION_DATA_ID aeFind, const NiPoint3& arPoint, TESWorldSpace* apWorldSpace);
	// Empties the list; a list owning its regions deletes each one.
	void Clear();
	// The region of the list whose form ID is auiFormID, or null.
	TESRegion* HasRegion(uint32_t auiFormID);
	// Adds apRegion to the list.
	bool Add(TESRegion* apRegion);
	// Whether apList holds the same regions in the same order; false when either list is null.
	bool Compare(TESRegionList* apList);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	bool bOwnsRegionMemory;

	static inline int32_t iOwnLists;
	static inline kCachedData_t kCachedData{};
};

inline constexpr fnvd::Vtable TESRegionList_vftable{};
