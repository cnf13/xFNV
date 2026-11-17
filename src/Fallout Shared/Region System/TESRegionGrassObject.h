#pragma once

#include "Fallout Shared/Region System/TESRegionObjectBase.h"
#include "Fallout Shared/TESGrass.h"
#include "Reconstruction.hpp"

class TESLandTexture;
class TESWorldSpace;

// A grass a region places, optionally only on the ground of one land texture.
class TESRegionGrassObject : public TESRegionObjectBase {
public:
	// The object as a region's data subrecord stores it: the grass's and the land texture's form IDs, before the
	// file's compile index is applied (the second constructor, 0x4F5620, reads both).
	struct REGION_GRASS_OBJECT_FILE_DATA {
		// Swaps both dwords.
		void Endian();

		uint32_t iFormID;
		uint32_t iParentFormID;
	};

	// A grass object GetGrassForLocation may place, with its density as a fraction.
	struct GenGrass {
		TESRegionGrassObject* pForm;
		float fdensity;
	};

	// Constructs the object for apGrass, on apLandTexture only when given, in apMemory.
	TESRegionGrassObject(TESGrass* apGrass, TESLandTexture* apLandTexture);
	TESRegionGrassObject(TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA* apObjectData);

	int32_t GetID() override;
	TESForm* GetObject() override { return pObject; }
	virtual ~TESRegionGrassObject();
	virtual TESLandTexture* GetParentTexture() { return pParent; }
	virtual void SetParentTexture(TESLandTexture* apLandTexture) { pParent = apLandTexture; }
	// A copy of this object, or apCopy when it is not null.
	virtual TESRegionGrassObject* Copy(TESRegionGrassObject* apCopyInto);
	// The chance that the grass grows at the position.
	virtual float GetGenerateChance(const NiPoint3& arWorldLoc, TESWorldSpace* apWorldSpace, bool abRegionTest);

	static void operator delete(void* apMemory);

	TESGrass* pObject;
	TESLandTexture* pParent;
};
inline constexpr fnvd::Vtable TESRegionGrassObject_vftable{};
