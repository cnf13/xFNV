#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class BGSSaveLoadFile;
struct CreatedReferenceInitialData;
struct ExteriorCellInitialDataChar;
struct ExteriorCellInitialDataShort;
class InteriorCellInitialData;
struct MovedReferenceInitialData;
struct ReferenceInitialData;
class TESObjectCELL;
class TESObjectREFR;
class TESWorldSpace;

// The kinds of initial data a saved form can have (values from the exe's debug info, names from the PDB).
enum INITIAL_DATA_TYPE : int32_t {
	INITIAL_DATA_NONE = 0x0,
	INITIAL_DATA_EXTERIOR_CELL_CHAR = 0x1,
	INITIAL_DATA_EXTERIOR_CELL_SHORT = 0x2,
	INITIAL_DATA_INTERIOR_CELL = 0x3,
	INITIAL_DATA_REFERENCE_LOCATION = 0x4,
	INITIAL_DATA_CREATED_REFERENCE = 0x5,
	INITIAL_DATA_MOVED_REFERENCE = 0x6,
	INITIAL_DATA_TYPE_COUNT = 0x7,
};

class BGSSaveLoadInitialData {
public:
	// The initial data type of a form of type aeFormType with the changes aChangeFlags
	// (cdecl, 0x84E730). PDB: ENUM_FORM_ID; the exe compares the whole dword (cmp dword [ebp+0xC], 0x39) and
	// passes it on to TESObjectREFR::IsReferenceFormType.
	static INITIAL_DATA_TYPE GetInitialDataType(uint32_t aiFormID, int32_t aeFormType, BGSChangeFlags aChangeFlags);
	// The initial data type of apForm with the changes aiChangeFlags.
	static INITIAL_DATA_TYPE GetInitialDataType(TESForm* apForm, BGSChangeFlags aiChangeFlags);
	static uint32_t GetInitialDataTypeSize(INITIAL_DATA_TYPE);
	static bool IsInitialDataRefType(INITIAL_DATA_TYPE);
	static uint32_t GetInitialDataTypeDataSize(INITIAL_DATA_TYPE aeType);
	static bool IsInitialDataCellType(INITIAL_DATA_TYPE aeType);
	// Writes apForm's initial data for aiChangeFlags to apSaveGameBuffer.
	static INITIAL_DATA_TYPE SaveInitialData(BGSSaveFormBuffer* apSaveGameBuffer, TESForm* apForm,
		BGSChangeFlags aiChangeFlags);
	// Reads apForm's initial data for aiChangeFlags from apLoadGameBuffer and applies it to the form.
	static INITIAL_DATA_TYPE LoadInitialData(BGSLoadFormBuffer* apLoadGameBuffer, TESForm* apForm,
		BGSChangeFlags aiChangeFlags);
	// Reads aeType's initial data from apLoadGameBuffer into apData; the result of the buffer overload, 0 for
	// INITIAL_DATA_NONE.
	static uint32_t LoadInitialDataStruct(BGSLoadFormBuffer* apLoadGameBuffer, INITIAL_DATA_TYPE aeType, void* apData);
	// Reads the initial data of type aeType from apFile into apInitialDataBuffer; the
	// result of the buffer overload, 0 for INITIAL_DATA_NONE.
	static uint32_t LoadInitialDataStruct(BGSSaveLoadFile* apFile, INITIAL_DATA_TYPE aeType, void* apInitialDataBuffer);
	// Unpacks the initial data of type aeType from apBuffer into apInitialDataBuffer; the type's data size.
	static uint32_t LoadInitialDataStruct(char* apBuffer, INITIAL_DATA_TYPE aeType, void* apInitialDataBuffer);
	// Creates reference aiFormID from apReferenceInitialData, reusing apPreExistingForm when set; the reference or
	// null.
	static TESObjectREFR* CreateReference(uint32_t aiFormID, ReferenceInitialData* apReferenceInitialData,
		TESForm* apPreExistingForm);
};

// What one initial data type is: its size in bytes and whether it is a reference or a cell (IDA's struct; the PDB has
// only the array iInitialDataTypeData of it).
struct InitialDataTypeData {
	uint8_t cSize;
	bool bRefType;
	bool bCellType;
};

// Where a reference was created and how it is turned: packed to byte alignment (27 bytes).
#pragma pack(push, 1)
struct ReferenceLocationInitialData {
	ReferenceLocationInitialData();

	BGSNumericIDIndex LocationIDIndex;
	NiPoint3 Location;
	NiPoint3 Angle;
};
#pragma pack(pop)

// The initial data of a created reference: its location, bound and original cell, position and angle. Packed to byte
// alignment (45 bytes).
#pragma pack(push, 1)
struct ReferenceInitialData {
	ReferenceInitialData();

	void Initialize_LOCATION(ReferenceLocationInitialData* a2);
	// The cell or world space of iLocationID; for a world space, *arpCell is the cell at
	// Location (IDA: InitialDataBuffer::GetLocationCellOrWorld; thiscall, ret 8, 0x84E610).
	bool GetLocationCellAndWorld(TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld);
	// The cell or world space of iOriginalLocationID; for a world space, *arpCell is the cell
	// at iOriginalCellX, iOriginalCellY.
	bool GetOriginalLocationCellAndWorld(TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld);
	void Initialize_CREATED(CreatedReferenceInitialData* a2);
	void Initialize_MOVED(MovedReferenceInitialData* a2);

	uint32_t iLocationID;
	uint32_t iBoundID;
	uint32_t iOriginalLocationID;
	int32_t iOriginalCellX;
	int32_t iOriginalCellY;
	NiPoint3 Location;
	NiPoint3 Angle;
	uint8_t cCreatedReferenceFlags;
};
#pragma pack(pop)

// The initial data of a cell: its world space, grid coordinates and the time it was detached.
struct CellInitialData {
	void Initialize_INTERIOR(InteriorCellInitialData* a2);
	void Initialize_CHAR(ExteriorCellInitialDataChar* a2);
	void Initialize_SHORT(ExteriorCellInitialDataShort* a2);

	uint32_t iWorldSpaceID;
	int32_t iCellX;
	int32_t iCellY;
	int32_t iDetachTime;
};

// The initial data of an exterior cell whose grid coordinates fit a byte: its world space index, coordinates and the
// time it was detached (8 bytes).
struct ExteriorCellInitialDataChar {
	uint16_t sWorldSpaceIDIndex;
	char cCellX;
	char cCellY;
	int32_t iDetachTime;
};

// The initial data of an exterior cell whose grid coordinates are shorts, packed to byte alignment (10 bytes).
#pragma pack(push, 1)
struct ExteriorCellInitialDataShort {
	uint16_t sWorldSpaceIDIndex;
	int16_t sCellX;
	int16_t sCellY;
	int32_t iDetachTime;
};
#pragma pack(pop)

// The initial data of a reference created in the world, packed to byte alignment (31 bytes).
#pragma pack(push, 1)
struct CreatedReferenceInitialData {
	CreatedReferenceInitialData();

	ReferenceLocationInitialData RefLocationData;
	uint8_t cCreatedReferenceFlags;
	BGSNumericIDIndex BoundIDIndex;
};
#pragma pack(pop)

// The initial data of a reference moved from another cell, packed to byte alignment (34 bytes).
#pragma pack(push, 1)
struct MovedReferenceInitialData {
	MovedReferenceInitialData();

	ReferenceLocationInitialData RefLocationData;
	BGSNumericIDIndex OriginalLocationIDIndex;
	int16_t sOriginalCellX;
	int16_t sOriginalCellY;
};

// The table of every initial data type, indexed by the type (PDB: BGSSaveLoadInitialData::iInitialDataTypeData[7]).
using InitialDataTypeDataTable = InitialDataTypeData[7];
inline InitialDataTypeDataTable kInitialDataTypeData{{}, {8, false, true}, {10, false, true}, {4, false, true},
	{27, true}, {31, true}, {34, true}};

#pragma pack(pop)
