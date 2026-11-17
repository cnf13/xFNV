#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

class BGSCameraShot;
class NiFormArray;
class TESObjectREFR;

// The parent and previous path form IDs an ANAM chunk holds. Not named in the PDB or IDA; name chosen here.
struct CAMERA_PATH_LINKS {
	// Swaps both dwords.
	void Endian();

	uint32_t uiParentPathID;
	uint32_t uiPrevPathID;
};

// The path's flags (DATA chunk): bit 0 zoom disabled, bit 1 zoom to the shot list, bit 7 no shots allowed.
struct PATH_DATA {
	uint8_t cFlags;
};

// A camera path form (CPTH).
class BGSCameraPath : public TESForm {
public:
	BGSCameraPath();
	~BGSCameraPath() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	TESForm* CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	bool GetAllowNoShots();
	bool GetZoomDisable();
	bool GetZoomShotList();
	uint32_t GetIndexForChild(BGSCameraPath* apChild);
	void RemoveChildAtIndex(uint32_t aiIndex);
	BGSCameraPath* GetChildByIndex(uint32_t aiIndex);
	// The path or child path to play for apActionRef and apTargetRef, or null.
	BGSCameraPath* GetCameraPathToPlay(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef);
	// The number of child paths.
	// Inferred type: return: uint32_t: unsigned uses from 0x58a563
	uint32_t GetChildCount();
	// Resolves the parent and previous paths by form ID.
	bool InitParents();
	void RemoveChild(BGSCameraPath* apChild);
	NiFormArray* CreateChildArray();
	bool GetZoomDefault();
	void FixNextPaths(NiFormArray* apArray);

	TESCondition Conditions;
	// the shots of the path (IDA types the items void*; the PDB name is ShotList)
	BSSimpleList<BGSCameraShot*> ShotList;
	PATH_DATA data;
	uint8_t padding29[3];
	NiFormArray* pChildPaths;
	BGSCameraPath* pParentPath;
	BGSCameraPath* pPrevPath;
};
inline constexpr fnvd::Vtable BGSCameraPath_vftable{};
