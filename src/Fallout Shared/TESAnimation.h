#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

class TESFile;

// The animation (KF) file names of a form.
class TESAnimation : public BaseFormComponent {
public:
	TESAnimation();
	~TESAnimation();

	void InitializeDataComponent() override {}
	void ClearDataComponent() override;
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// Adds the KF file apFile to the list.
	void AddKFFile(const char* apFilename);
	// Reads a KFFZ chunk of apFile into apAnimation's KF file list; the exe calls it with this
	// in ecx as well (thiscall, ret 8, 0x47FAA0).
	void LoadAnimationChunk(TESAnimation* apAnimation, TESFile* apFile);
	bool HasKFFiles();
	// Same function as BaseProcess::GetActorPackageThatIsRunning (0x717E50); the exe calls that shared getter.
	BSSimpleList<char*>* GetKFFileList();
	bool IsKFFileInList(const char* apFilename);

	BSSimpleList<char*> kfFilenames;
};
inline constexpr fnvd::Vtable TESAnimation_vftable{};
