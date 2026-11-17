#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

enum CHUNK_ID : uint32_t;
class TESFile;
class TESForm;
class TESObjectREFR;

// IDA names; not in the PDB.
namespace BSUtils {
// Writes apFileName with its extension replaced by "_n.dds" to arOut and returns arOut's
// text, or null when apFileName is null or has no extension.
const char* CreateNormalMapString(BSStringT<char>& arOut, const char* apFileName);
} // namespace BSUtils

// A texture file name.
class TESTexture : public BaseFormComponent {
public:
	TESTexture();
	~TESTexture();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// The largest texture size the file may have; 0 for no limit.
	virtual uint32_t GetMaxAllowedSize() { return 0; }
	// Writes the normal map name of the texture to aOutFilename and returns it.
	virtual const char* GetAsNormalFile(BSStringT<char>& aOutFilename);
	// The folder the texture name is relative to.
	virtual const char* GetDefaultPath() const;

	// Saves the texture name as chunk aeTextureID.
	void Save(CHUNK_ID aeTextureID);
	// The length of the texture name (thiscall, 0x48CEE0). Const as the PDB's
	// ?GetTextureNameLength@TESTexture@@QBAIXZ.
	uint32_t GetTextureNameLength() const;
	// Same function as TESModel::GetModel (0x408DA0), registered there. Const as the PDB's
	// ?GetTextureName@TESTexture@@QBAPBDXZ.
	const char* GetTextureName() const;
	// Sets the texture name.
	void SetTextureName(const char* apTextureName);

	// The texture name of apForm's apRef.
	static const char* GetTextureName(TESForm* apForm, TESObjectREFR* apRef);
	// The record code of the icon chunk ('ICON').
	static bool IsTextureChunk(CHUNK_ID aChunk);
	// Reads a texture chunk of apFile into apTexture.
	static void LoadTextureChunk(TESTexture* apTexture, TESFile* apFile);
	// IDA's Texture::GetNormalPath_. Copies apTextureName to apNormalTextureName (260 bytes)
	// with its extension replaced by "_n.dds" (cdecl, ret 0, 0x48E660). No return value: its one caller,
	// BSFaceGenManager::QueueHeadParts 0x657569, does not read eax.
	static void GetAsNormalFile(const char* apTextureName, char* apNormalTextureName);
	// IDA's Texture::Append_sk_toPath. Copies apTextureName to apSkinTextureName (260 bytes)
	// with its extension replaced by "_sk.dds" (cdecl, ret 0, 0x48E6C0). No return value: its one caller,
	// BSFaceGenManager::QueueHeadParts 0x6575AD, does not read eax.
	static void GetAsSkinFile(const char* apTextureName, char* apSkinTextureName);

	BSStringT<char> TextureName;
};
inline constexpr fnvd::Vtable TESTexture_vftable{};
