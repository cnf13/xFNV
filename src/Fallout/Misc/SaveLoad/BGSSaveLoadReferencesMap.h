#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

class BGSReconstructFormsInAllFilesMap;
class TESObjectCELL;

// The saved references of each cell of a worldspace (or of the interior cells), by cell form ID.
class BGSCellNumericIDArrayMap : public NiTMap<uint32_t, BSSimpleArray<uint32_t>*> {
public:
	BGSCellNumericIDArrayMap();
	~BGSCellNumericIDArrayMap() override;

	// Adds aiReferenceID to the references of cell aiCellID.
	void AddReference(uint32_t aiCellID, uint32_t aiReferenceID);
	void Clear();
	void RemoveReference(uint32_t aiCellID, uint32_t aiReferenceID);

	static void operator delete(void* apMemory) { ourFree(apMemory); }
};

// The references a save keeps by place: those moved from their cell, those in interior cells and those in each
// worldspace.
class BGSSaveLoadReferencesMap {
public:
	BGSSaveLoadReferencesMap();
	~BGSSaveLoadReferencesMap();

	// Loads the saved references of apCell, collecting the forms to rebuild in
	// apReconstructForms.
	void LoadReferencesForCell(TESObjectCELL* apCell, BGSReconstructFormsInAllFilesMap* apReconstructForms);
	// Records that reference aiFormID moved out of its cell.
	void AddMovedReference(uint32_t aiFormID);
	// Records reference aiReferenceID as saved in interior cell aiCellID.
	void AddReference(uint32_t aiCellID, uint32_t aiReferenceID);
	// Records reference aiReferenceID at arReferenceLocation as saved in world space
	// aiWorldSpaceID.
	void AddReference(uint32_t aiWorldSpaceID, uint32_t aiReferenceID, const NiPoint3& arReferenceLocation);
	bool GetMovedReference(uint32_t aiFormID);
	// Removes reference aiFormID from the moved references; returns whether it was there.
	bool RemoveMovedReference(uint32_t aiFormID);
	// Empties the three maps.
	void Clear();
	void RemoveReference(uint32_t aiWorldSpaceID, uint32_t aiReferenceID, const NiPoint3& arReferenceLocation);
	void LoadChangedReference(uint32_t aiRefID, BGSReconstructFormsInAllFilesMap* apReconstructForms);
	void RemoveReference_0(uint32_t aiCellID, uint32_t aiReferenceID);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	NiTPointerMap<uint32_t, uint32_t> MovedReferencesMap;
	BGSCellNumericIDArrayMap InteriorReferencesMap;
	NiTPointerMap<uint32_t, BGSCellNumericIDArrayMap*> WorldspaceReferencesMap;
};

using NiTMapBasePtrUIntUInt = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, uint32_t>;
using NiTMapBaseUIntCellNumericIDArrayMap =
	NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, BGSCellNumericIDArrayMap*>;
using NiTPointerMapUIntCellNumericIDArrayMap = NiTPointerMap<uint32_t, BGSCellNumericIDArrayMap*>;

// The references-of-a-cell map's base and the map itself. Defined in BGSSaveLoadReferencesMap.cpp.
using NiTMapBaseUIntBSSimpleArray =
	NiTMapBase<DFALL<NiTMapItem<uint32_t, BSSimpleArray<uint32_t>*>>, uint32_t, BSSimpleArray<uint32_t>*>;
using NiTMapUIntBSSimpleArray = NiTMap<uint32_t, BSSimpleArray<uint32_t>*>;
