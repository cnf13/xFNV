#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BaseFormComponent.h"

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
	// Reads a KFFZ chunk of apFile into apAnimation's KF file list.
	void LoadAnimationChunk(TESAnimation* apAnimation, TESFile* apFile);
	bool HasKFFiles();
	BSSimpleList<char*>* GetKFFileList();
	bool IsKFFileInList(const char* apFilename);

	BSSimpleList<char*> kfFilenames;
};
