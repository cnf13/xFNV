#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

class IDLE_ANIM_ROOT;
class NiFormArray;
class TESForm;
class TESIdleForm;
class TESObject;
class TESObjectREFR;

// Owns the idle animation forms and their trees.
class TESIdleManager {
public:
	TESIdleManager();
	~TESIdleManager();

	// Links the idles loaded from the data files into their trees.
	void InitLoadedIdles();
	// The .kf names of apIdleDir's idle directory for weapon group aeWeaponGroup; null when there are none.
	BSSimpleList<const char*>* GetRootFilenameList(const char* apIdleDir, ANIM_GROUP_WEAPON_ENUM aeWeaponGroup);
	// Builds the idle forms of the .kf names in apList under apIdleDir and returns their array.
	NiFormArray* AddRootIdleArray(const char* apIdleDir, BSSimpleList<const char*>* apList, bool abForceList);
	// Whether the idles are initialized (thiscall, ret 0; same code as InterfaceManager::GetMagicPageNumber,
	// 0x843150).
	bool GetInitialized();
	// The idle apActionRef plays at apTargetRef (its furniture), or null.
	TESIdleForm* GetIdleToPlay(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef);
	// Records apIdle among the idles loaded from the data files.
	void AddLoadedIdle(TESIdleForm* apIdle);
	// The loose idle array of apIdleDir's idle directory, or null.
	NiFormArray* GetLooseIdleArray(const char* apIdleDir);
	// The loose idle array of apForm's model's idle directory, or null.
	NiFormArray* GetLooseIdleArray(TESForm* apForm);
	// The root idle array of apForm's model's idle directory, created when missing (IDA's AddIdleAnimRoot_0; the
	// PDB overload by its use, thiscall, ret 4, 0x6004F0).
	NiFormArray* GetRootIdleArray(TESForm* apForm);
	// The idle root of apIdleDir's idle directory, or null.
	IDLE_ANIM_ROOT* GetIdleAnimRoot(const char* apIdleDir);
	// The idle root of apForm's model's idle directory, or null.
	// Inferred type: apForm: TESForm* from 0x5fe20e (parameter 0 of TESIdleForm::Destruct)
	IDLE_ANIM_ROOT* GetIdleAnimRoot(TESForm* apForm);
	void DestroyRootIdleArrays();
	void DestroyRootIdleArrayForms();
	void InitRootIdleArrays();

	// Writes apInDir's idle directory name into apOutDir.
	static void GetIdleDirName(const char* apInDir, char* apOutDir);
	static int32_t GetUsedItemLevel();
	static void SetUsedItemLevel(int32_t aiLevel);
	static bool GetUsedItemActivate();
	static TESObject* GetUsedItem();
	static void SetUsedItemActivate(bool abactive);
	// Records the object the next idle uses.
	static void SetUsedItem(TESObject* apObject);
	// The exe's deleting destructor (0x45DF40) frees through ourFree.
	static void operator delete(void* apMemory);

	BSTCaseInsensitiveStringMap<IDLE_ANIM_ROOT*> IdleArrayMap;
	bool bInitialized;
	uint8_t padding15[3];
	NiTPrimitiveArray<TESIdleForm*> LoadedIdleArray;

	static inline TESObject* pUsedItem;
	static inline int32_t iUsedItemLevel = -1;
	static inline bool bActivate;
};

// The engine's idle manager.
inline TESIdleManager* pIdleManager;
inline bool bCreatedByThisFunction;
