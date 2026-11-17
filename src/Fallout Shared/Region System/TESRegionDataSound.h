#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "SDK/MSVC/ehvec.h"

class BGSMusicType;
class MediaSet;
class TESFile;
struct TESRegionSound;
class TESSound;

// One sound of a region: the sound form, its flags and how likely it plays (also the RDSD chunk's 12-byte record).
struct TESRegionSound {
	TESRegionSound();

	// Swaps the three dwords.
	void Endian();
	// The sound: the pointer uiSound holds once loaded, or on the 64-bit program the form of its ID.
	TESSound* GetSound() const;

	// The file record's form ID. On 32-bit builds the TESSound* is stored here after the load; on 64-bit builds the
	// form ID is kept, since a pointer does not fit, and GetSound looks it up. The file format is the same on both
	// widths.
	uint32_t uiSound;
	uint32_t uiFlags;
	uint32_t uiChance;
};

// A region's sound data: its music type, its sounds and the incidental and battle media sets (form IDs).
struct TESRegionSounds {
	TESRegionSounds();
	~TESRegionSounds();

	// Swaps the byte order of pType. LoadRegionDataSound calls it on its sounds after reading an RDMO form ID.
	void Endian();

	BGSMusicType* pType;
	BSSimpleList<TESRegionSound*> soundList;
	// A form ID despite the name: GetIncidentalMediaSet looks it up with GetFormByNumericID, SetIncidentalMediaSet
	// stores the form's ID.
	uint32_t pMediaSetIncidentals;
	BSSimpleList<uint32_t> MediaSetBattleList;
};

// A region's sound data.
class TESRegionDataSound : public TESRegionData {
public:
	TESRegionDataSound();
	// The copy constructor.
	TESRegionDataSound(TESRegionDataSound* arData);
	~TESRegionDataSound() override;

	void Save() override;
	REGION_DATA_ID GetID() const override;
	TESRegionData* Copy() override;
	TESRegionData* Blend(const TESRegionData*) override;
	void BlendInto(const TESRegionData*, uint32_t) override;
	bool Validate() override;
	virtual bool LoadRegionDataSound(TESFile*);
	virtual BGSMusicType* GetMusicType() const { return sounds.pType; }
	virtual bool SetMusicType(BGSMusicType*);
	virtual MediaSet* GetMediaSetIncidentals();
	virtual MediaSet* GetMediaSetBattle();
	virtual void SetMediaSetIncidentals(MediaSet*);

	bool ClearSoundList();

	TESRegionSounds sounds;
};
