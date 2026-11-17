#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESFile.h"

class TESModelList;
struct TESTextureList;

// The alternate texture lists of a model list, one per model.
struct TESTextureListArray {
	// Frees the texture lists.
	void ClearData();
	void Load(TESModelList* apModelList, TESFile* apFile);
	TESTextureList* GetAt(uint32_t aiIndex);
	void Copy(TESTextureListArray* apTextureListArray);

	uint32_t iTextureListCount;
	TESTextureList** pTextureLists;
};

// A list of model (NIF) file names, with their alternate textures.
class TESModelList : public BaseFormComponent {
public:
	TESModelList();
	~TESModelList();

	void InitializeDataComponent() override {}
	void ClearDataComponent() override;
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// Adds the model file apFilename to the list.
	void AddModel(const char* apFilename);
	// Reads a NIFT or NIFZ chunk of apFile into apModelList's texture lists or model names.
	void LoadModelListChunk(TESModelList* apModelList, TESFile* apFile);

	BSSimpleList<const char*>* GetNIFFileList() { return &nifFilenames; }

	void Save(CHUNK_ID aeModelListID, CHUNK_ID aeTextureListID);
	bool IsNIFFileInList(const char* apFilename);

	BSSimpleList<const char*> nifFilenames;
	TESTextureListArray TextureListArray;
};
