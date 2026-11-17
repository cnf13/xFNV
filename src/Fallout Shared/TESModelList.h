#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESFile.h"
#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"

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
	// Reads a NIFT or NIFZ chunk of apFile into apModelList's texture lists or model names;
	// the exe calls it with this in ecx as well (thiscall, ret 8, 0x489CE0).
	void LoadModelListChunk(TESModelList* apModelList, TESFile* apFile);

	// Same function as BaseProcess::GetActorPackageThatIsRunning (0x717E50); the exe calls that shared getter.
	BSSimpleList<const char*>* GetNIFFileList() { return &nifFilenames; }

	void Save(CHUNK_ID aeModelListID, CHUNK_ID aeTextureListID);
	bool IsNIFFileInList(const char* apFilename);

	BSSimpleList<const char*> nifFilenames;
	TESTextureListArray TextureListArray;
};
inline constexpr fnvd::Vtable TESModelList_vftable{};
