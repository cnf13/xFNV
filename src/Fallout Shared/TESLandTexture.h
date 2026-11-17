#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/IOManager.h"
#include "Fallout Shared/TESForm.h"

class BGSTextureSet;
class QueuedFile;
class TESGrass;

enum HavokMaterialType : int32_t;

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
	// Returns &TextureGrassList.
	BSSimpleList<TESGrass*>* GetGrassList();

	BGSTextureSet* pTextureSet;
	TEXTURE_HAVOK_DATA HavokData;
	uint8_t cSpecularExponent;
	BSSimpleList<TESGrass*> TextureGrassList;
};
