#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/LockLevel.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiControllerManager;
class TESObjectREFR;

class LockPickMenu : public Menu {
public:
	// The menu's states (eLockState). Values from the Xbox PDB.
	enum LockState : int32_t {
		LS_INTRO_ANIM = 0,
		LS_PICK_INTRO_ANIM = 1,
		LS_IDLE = 2,
		LS_TURNING_LOCK = 3,
		LS_DAMAGING_PICK = 4,
		LS_BREAKING_PICK = 5,
		LS_RESETTING_LOCK = 6,
		LS_CLOSING_MENU = 7,
		NUM_STATES = 8,
	};

	LockPickMenu();
	~LockPickMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoEnter(int32_t, Tile*) override;
	void DoLeave(int32_t, Tile*) override;
	void DoIdle() override;
	int32_t GetClass() override;

	bool IsWellFormedMenu();
	bool GetPCInput(int32_t state);

protected:
	// Poses the lock rotation animation for the lock turned afAngleOffset past fLockAngle.
	void RotateLock(float afAngleOffset);

public:
	void Prepare3DElements();
	NiCamera* Create3DCamera();
	void RotateLockpick();
	// Inferred type: a2: float: used as one at 0x790e83
	void RotateLockpickModel(float a2);
	void SetupLockLevelSweetSpots();

	// Opens the lock pick menu for apRef (cdecl, ret 0, 0x78DB00); IDA types the second
	// argument int.
	static Tile* Create(TESObjectREFR* apTarget, int32_t aiLockLevel, bool abDebugMode);
	static void Close();
	static int32_t GetClassStatic();
	static Interface::TutorialMessageID GetTutorialMessageID();
	static void Draw3DElements();
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	LockState eLockState;
	Tile* pTiles[16];
	TESObjectREFR* pLockTarget;
	LOCK_LEVEL eLockLevel;
	int32_t iLockpickSkill;
	float fLockpickSkill;
	int32_t iPicksRemaining;
	uint32_t uiPrevIdleTick;
	float fSweetSpot;
	int32_t iTotalSweetSpotRadius;
	float fLockAngle;
	float fMaxLockAngle;
	float fPickHealth;
	bool bForceLockSuccess;
	bool bShowingTutorial;
	uint8_t padding9A[2];
	NiControllerSequence* pIntroAnim;
	NiControllerSequence* pLockRotateAnim;
	NiControllerManager* ppickAnimManager;
	NiControllerSequence* pPickIntroAnim;
	NiControllerSequence* pPickBendAnim;
	NiControllerSequence* pPickBreakAnim;
	// PC matrices have 4-byte alignment; the Xbox PDB puts origPickRotate at 0xC0.
	NiMatrix3 origPickRotate;
	NiPoint3 origPickTranslate;

	static inline LockPickMenu* pMe;
	static NiPointer<NiNode> spCameraRoot;
	static NiPointer<NiCamera> spCamera;
	static inline float PreviousAngleDelta;
	static inline int32_t iUT_Value;
	static inline int32_t iUT_X;
};

inline int32_t g_iconcentricLength;
inline int32_t g_isweetSpotLength;
inline char bCreateNewSweetSpot;
inline int32_t eprevLockState;
inline float ftotalStateSecs_1;
inline constexpr fnvd::Vtable LockPickMenu_vftable{};

// IDA name (of its first element; IDA's LockLevelMaxEasy is element 1); not in the PDB. The lockpick skill each lock
// level starts at, by lock level, and 101 past the last (LockPickMenu.cpp's initialiser 0xF90990 fills all six;
// bCreateNewSweetSpot follows at 0x11DA2A4).
using LockLevelMaxVeryEasy_t = int32_t[6];
extern LockLevelMaxVeryEasy_t LockLevelMaxVeryEasy;

// IDA name; not in the PDB. The five lock levels' iSweetSpotLengthMin settings (the exe builds them with
// SettingT<INISettingCollection>'s InitString, 0x45CFE0).
using g_iSweetSpotLengthMinArray_t = SettingT<INISettingCollection>* [5];
extern g_iSweetSpotLengthMinArray_t g_iSweetSpotLengthMinArray;

// The five lock levels' iSweetSpotLengthMax settings.
using g_iSweetSpotLengthMaxArray_t = SettingT<INISettingCollection>* [5];
extern g_iSweetSpotLengthMaxArray_t g_iSweetSpotLengthMaxArray;

// The five lock levels' iConcentricLengthMin settings.
using g_iConcentricLengthMinArray_t = SettingT<INISettingCollection>* [5];
extern g_iConcentricLengthMinArray_t g_iConcentricLengthMinArray;

// The five lock levels' iConcentricLengthMax settings.
using g_iConcentricLengthMaxArray_t = SettingT<INISettingCollection>* [5];
extern g_iConcentricLengthMaxArray_t g_iConcentricLengthMaxArray;
