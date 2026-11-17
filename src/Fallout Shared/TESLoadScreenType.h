#pragma once

#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

enum FONTS : int32_t;
// The data of a load screen type (its data type, then the display data of its two text windows).
struct LoadScreenType_Data;

template <class T> void ENDIAN(T& arValue, bool abUnused);

// What a load screen type shows besides its texture.
enum LOAD_SCREEN_DATA_TYPE : int32_t {
	LSDT_NONE = 0,
	LSDT_XP_PROGRESS = 1,
	LSDT_OBJECTIVE = 2,
	LSDT_TIP = 3,
	LSDT_STATS = 4,
	NUM_LSDT = 5,
};

// Where and how a load screen type draws one of its text windows.
struct DisplayData {
	// The call the load
	// screen data's Init makes on each window.
	void Init();

	uint32_t uDisplayPositionX;
	uint32_t uDisplayPositionY;
	uint32_t uDisplayWidth;
	uint32_t uDisplayHeight;
	float fDisplayRotationRads;
	FONTS eDisplayFont;
	float TextColorR;
	float TextColorG;
	float TextColorB;
	uint32_t uJustification;
};

struct LoadScreenType_Data {
	// Swaps every field read from a big-endian file.
	void Endian();
	// Initialises both windows, the data type, the second window's justification and the
	// stat count.
	LoadScreenType_Data* Init();

	LOAD_SCREEN_DATA_TYPE DataType;
	DisplayData DisplayData1;
	DisplayData DisplayData2;
	uint32_t uNumStatsToDisplay;
};

// A load screen type form (LSCT).
class TESLoadScreenType : public TESForm {
public:
	TESLoadScreenType();
	~TESLoadScreenType() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	// the exe shares this body with TESChallenge (0x5A7770)
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	// the exe shares these four bodies with TESChallenge and TESCaravanCard (0x59ADB0, 0x59ADD0, 0x59ADF0, 0x616640)
	void SaveGame(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	LoadScreenType_Data data;
};
inline constexpr fnvd::Vtable TESLoadScreenType_vftable{};
