#include "Fallout Shared/BGSMessageIcon.h"

#include "BSCore/BSString.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESTexture.h"

BGSMessageIcon::BGSMessageIcon() {
	BGSMessageIcon::InitializeDataComponent();
}

BGSMessageIcon::~BGSMessageIcon() {
	BGSMessageIcon::ClearDataComponent();
}

void BGSMessageIcon::InitializeDataComponent() {
	// the exe calls the icon's slot through its vtable; calling through the base keeps clang from devirtualising
	static_cast<BaseFormComponent&>(Icon).InitializeDataComponent();
}

void BGSMessageIcon::ClearDataComponent() {
	(&Icon)->ClearDataComponent();
}

void BGSMessageIcon::CopyComponent(BaseFormComponent* apCopyFC) {
	const char* messageIconTextureName;
	BGSMessageIcon* v4 = dynamic_cast<BGSMessageIcon*>(apCopyFC);
	if (v4) {
		messageIconTextureName = v4->GetMessageIconTextureName();
		SetMessageIconTextureName(messageIconTextureName);
	}
}

bool BGSMessageIcon::CompareComponent(BaseFormComponent* apCompareFC) {
	BGSMessageIcon* v6 = dynamic_cast<BGSMessageIcon*>(apCompareFC);
	if (!v6) {
		return (bool)1;
	}
	const char* messageIconTextureName = v6->GetMessageIconTextureName();
	const char* v3 = GetMessageIconTextureName();
	return (BSstricmp(v3, messageIconTextureName) != 0);
}

const char* BGSMessageIcon::GetMessageIconTextureName() {
	return Icon.GetTextureName();
}

void BGSMessageIcon::Save(CHUNK_ID aeTextureID) {
	if (Icon.GetTextureNameLength()) {
		TESForm::AddChunkArray(aeTextureID, Icon.GetTextureName(), Icon.GetTextureNameLength() + 1);
	}
}

void BGSMessageIcon::SetMessageIconTextureName(const char* apTexture) {
	(&Icon)->TESTexture::SetTextureName(apTexture);
}

void BGSMessageIcon::LoadMessageIcon(BGSMessageIcon* apIcon, TESFile* apFile) {
	TESTexture* ActorPackageThatIsRunning = &apIcon->Icon;
	TESTexture::LoadTextureChunk(ActorPackageThatIsRunning, apFile);
}
