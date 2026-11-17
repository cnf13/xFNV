#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESIcon.h"
#include "Reconstruction.hpp"

enum CHUNK_ID : uint32_t;
class TESFile;

// The icon texture shown with a message.
class BGSMessageIcon : public BaseFormComponent {
public:
	BGSMessageIcon();
	~BGSMessageIcon();

	void InitializeDataComponent() override;
	void ClearDataComponent() override;
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// The icon's texture file name.
	const char* GetMessageIconTextureName();
	// Writes the icon's texture file name as a chunk of aeChunk.
	void Save(CHUNK_ID aeTextureID);
	void SetMessageIconTextureName(const char* apTexture);
	uint32_t GetMessageIconTextureNameLength() const;

	// Reads the MICO chunk of apFile into apIcon.
	static void LoadMessageIcon(BGSMessageIcon* apIcon, TESFile* apFile);

	TESIcon Icon;
};
inline constexpr fnvd::Vtable BGSMessageIcon_vftable{};
