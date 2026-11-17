#pragma once

#include "BSMain/IOManager.h"
#include "Fallout Shared/TESForm.h"
#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"

class BGSTextureSet;
enum HavokMaterialType : int32_t;
class QueuedFile;
class TESGrass;

// A landscape texture's havok material, friction and restitution bytes.
struct TEXTURE_HAVOK_DATA {
	uint8_t cMaterialType;
	uint8_t cFriction;
	uint8_t cRestitution;
};

// A landscape texture form (LTEX).
class TESLandTexture : public TESForm {
public:
	TESLandTexture();
	~TESLandTexture() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	HavokMaterialType GetHavokMaterialType();
	uint8_t GetSpecularExponent();
	void QueueTexture(IO_TASK_PRIORITY aePriority, QueuedFile* apParent);
	// &TextureGrassList; the exe's shared one-field getter (thiscall, ret 0, 0x891170).
	BSSimpleList<TESGrass*>* GetGrassList();

	BGSTextureSet* pTextureSet;
	TEXTURE_HAVOK_DATA HavokData;
	uint8_t cSpecularExponent;
	BSSimpleList<TESGrass*> TextureGrassList;
};
inline constexpr fnvd::Vtable TESLandTexture_vftable{};
