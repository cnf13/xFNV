#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

class BSFileEntry;
struct BSHashData;
enum CHUNK_ID : uint32_t;
class TESFile;
class TESForm;
class TESModelTextureSwap;

struct TESTextureList {
	TESTextureList();
	~TESTextureList();

	// Copies apSource's textures.
	void Copy(TESTextureList* apTextureList);
	// Loads the chunk's run of 24-byte texture hash records.
	void Load(TESFile* apFile, TESForm* apParentForm, const char* apModelName);
	// Reads aiCount texture hash records from apBuffer; the exe sizes the array with the
	// whole 32-bit count and keeps only its low byte as cTextureCount (thiscall, ret 0x10, 0x489670).
	void LoadFromBuffer(BSHashData* apBuffer, uint32_t aiCount, TESForm* apParentForm, const char* apModelName);
	bool ClearData();

	// The exe's deleting destructor (0x48A0A0) frees through ourFree.
	static void operator delete(void* apMemory);

	uint8_t cTextureCount;
	uint8_t padding01[3];
	BSFileEntry** pTextureOffsetArray;
};

// The model (NIF) path of a form.
class TESModel : public BaseFormComponent {
public:
	TESModel();
	virtual ~TESModel();

	void InitializeDataComponent() override;
	void ClearDataComponent() override;
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// The model path, or the shared empty string when it has none.
	virtual const char* GetModel();
	virtual void SetModel(const char* apModel);
	// This model as a texture swap model, or null when it is a plain one.
	virtual TESModelTextureSwap* GetAsModelTextureSwap() { return nullptr; }

	// The distant model name for apModelName.
	void GetDistantModelName(char* apModelName);
	// Saves the model path and texture hashes as chunks aeModelID and aeTextureID.
	void Save(CHUNK_ID aeModelID, CHUNK_ID aeTextureID);
	uint32_t GetSkinFlags() const;
	void SetFlags(char acFlags);
	char QFlags() { return cFlags; }
	// The texture list at +0xC.
	TESTextureList* GetTextureList() { return &TextureList; }
	// Saves the flags as chunk aeFlagsID when any is set.
	void SaveFlags(CHUNK_ID aeFlagsID);
	uint32_t GetModelLength();
	bool QHasFacegenData();

	// Writes apForm's distant model name into apModelName when apForm is a TESModel.
	// Inferred type: apModelName: char* from 0x4891c0 (passed to TESModel::GetDistantModelName 0)
	static void GetDistantModelName_(char* apModelName, TESForm* apForm);
	// The model chunk record codes ('MODL', 'MODD', 'MODS', 'MODT').
	static bool IsModelChunk(CHUNK_ID aChunk);
	// Reads a model chunk of apFile into apModel.
	static void LoadModelChunk(TESModel* apModel, TESFile* apFile);
	// Reads model texture data from apFile.
	static void LoadModelTextureChunk(TESModel* apModel, TESFile* apFile);
	// The model path for apObj, or the shared empty string when it has none.
	static const char* GetModel(TESForm* apObj);
	// Inferred type: apObj: TESForm* from 0x50fb07 (this of TESObject::IsMarker)
	static uint32_t GetModelLength_00489540(TESForm* apObj);

	BSStringT<char> cModel;
	TESTextureList TextureList;
	uint8_t cFlags;
	uint8_t padding15[3];
};
inline constexpr fnvd::Vtable TESModel_vftable{};

// The game settings TESModel.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bWarnOnMissingFileEntry;
extern SettingT<INISettingCollection> bUseOptimizedTextureLoading;
