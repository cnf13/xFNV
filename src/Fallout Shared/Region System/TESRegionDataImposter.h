#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Region System/TESRegionData.h"

class TESFile;
class TESForm;
class TESObjectREFR;

// The imposters (distant stand-ins of references) a region shows.
class TESRegionDataImposter : public TESRegionData {
public:
	TESRegionDataImposter();
	// The copy constructor.
	TESRegionDataImposter(TESRegionDataImposter* apData);
	~TESRegionDataImposter() override;

	void Save() override;
	void InitItem(TESForm* apOwner) override;
	REGION_DATA_ID GetID() const override;
	TESRegionData* Copy() override;
	TESRegionData* Blend(const TESRegionData*) override;
	void BlendInto(const TESRegionData*, uint32_t) override;
	bool Validate() override;
	virtual bool LoadRegionDataImposter(TESFile*);

	// Appends apNewImposter unless the list holds it; false when it did.
	bool AddToImposterList(TESObjectREFR* apNewImposter);
	// Empties m_Imposters and returns true.
	bool ClearImposterList();

	// &m_Imposters, through the shared getter.
	BSSimpleList<TESObjectREFR*>* GetImposterList() { return &m_Imposters; }

	// Shows or hides every imposter of the list.
	void SetImpostersVisible(bool abVisible, bool abInstantly);

	// Shows or hides apImposter's imposter.
	static void SetImposterVisible(TESObjectREFR* apImposter, bool abVisible, bool abInstantly);
	// Whether apImposter is in the imposter list of a region the player is in.
	static bool IsImposterInActiveRegionList(TESObjectREFR* apImposter);

	// The list holds references (TESObjectREFR::SetImposterVisible is called on each item).
	BSSimpleList<TESObjectREFR*> m_Imposters;
};
