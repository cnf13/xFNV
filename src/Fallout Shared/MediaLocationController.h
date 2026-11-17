#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"

class MediaSet;
class TESFaction;
class TESObjectREFR;

enum FIGHT_REACTION : int32_t;

// A media location controller form (ALOC).
class MediaLocationController : public TESForm, public TESFullName {
public:
	// How a location's media loop plays.
	enum LOOPFLAGS : int32_t {
		LOOP_LOOP = 0,
		LOOP_RANDOM = 1,
		LOOP_RETRIGGER = 2,
		LOOP_NONE = 3,
	};

	// Which factions a dead reputation counts.
	enum DEADREPFLAGS : int32_t {
		DEADFACTION_CONDITIONAL_NEUTRAL = 0,
		DEADFACTION_CONDITIONAL_ENEMY = 1,
		DEADFACTION_CONDITIONAL_ALLY = 2,
		DEADFACTION_CONDITIONAL_FRIEND = 3,
		DEADFACTION_LOCATION = 4,
		DEADFACTION_NONE = 5,
	};

	MediaLocationController();
	~MediaLocationController() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	bool GetDefaultTime();
	void InitItemList(BSSimpleList<MediaSet*>* plist);
	bool GetDayNight();
	MediaLocationController::DEADREPFLAGS GetDeadRep();
	MediaLocationController::LOOPFLAGS GetLoop();
	bool GetDay();
	MediaSet* GetRandomListItem(BSSimpleList<MediaSet*>* list);
	bool Update(bool abInCombat);

	uint32_t uiLocationDelay;
	uint32_t uiLayerTime;
	uint32_t uiLoopTime;
	uint32_t uiMediaStartTime;
	bool bIsActive;
	bool bInTension;
	bool bInCombat;
	bool bIsDay;
	bool bIsConditional;
	float fCurrentPlayerRadius;
	uint32_t uiFoundHostileActors;
	uint32_t uiFoundFactionActors;
	FIGHT_REACTION eCurrentFactionReaction;
	int8_t cCurrentLayer;
	MediaSet* pCurrentMediaSet;
	TESFullName MediaLocationControllerName;
	TESFaction* pFaction;
	// The faction reaction that makes the location conditional (FNAM; the constructor sets FIGHT_REACTION_ENEMY).
	FIGHT_REACTION rFactionConditional;
	TESObjectREFR* pAudioMarker;
	uint32_t iFlags;
	float fLayerTwoPercent;
	float fLayerThreePercent;
	float fRetriggerDelay;
	float fLocationDelay;
	uint32_t uiDayStart;
	uint32_t uiNightStart;
	BSSimpleList<MediaSet*> lConditionalNeutralList;
	BSSimpleList<MediaSet*> lConditionalAllyList;
	BSSimpleList<MediaSet*> lConditionalFriendList;
	BSSimpleList<MediaSet*> lConditionalEnemyList;
	BSSimpleList<MediaSet*> lLocationList;
	BSSimpleList<MediaSet*> lBattleList;
};
