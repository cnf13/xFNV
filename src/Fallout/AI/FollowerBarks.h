#pragma once

#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"

class Actor;
class TESTopic;

// The lines followers say on their own (knocked out, low health, out of ammo...). Only the members src defines are
// declared.
class FollowerBarks {
public:
	// The situations a follower barks about.
	enum FollowerBarkType : int32_t {
		BARK_KNOCKED_OUT = 0,
		BARK_KNOCKED_BACK = 1,
		BARK_CRIPPLED = 2,
		BARK_DIED = 3,
		BARK_MINOR_HEALTH_WARNING = 4,
		BARK_MAJOR_HEALTH_WARNING = 5,
		BARK_CRIPPLED_REMINDER = 6,
		BARK_OUT_OF_AMMO = 7,
		BARK_WEAPON_BROKE = 8,
		BARK_WEAPON_INEFFECTIVE = 9,
		BARK_USED_DOCTORS_BAG = 10,
		BARK_CAME_TO = 11,
		BARK_STEALTHING = 12,
		BARK_REGENERATING = 13,
	};

	// One bark said: who said it, which kind, and until when it may not repeat and no other bark may follow.
	struct FollowerBarkEntry {
		// Fills the entry for apBarker saying aeBarkType now.
		void BuildBarkEntry(Actor* apBarker, FollowerBarkType aeBarkType);
		// Says the bark's topic: a speaker with a process greets with it (or, asleep, low-level or knocked out
		// essential, runs its first response's result scripts at once); a dead one only gets the say-to-done action.
		void Trigger();
		bool GetShouldRepeat();

		Actor* m_pSpeaker;
		FollowerBarkType m_eBarkType;
		uint32_t m_tNoRepeatExpiration;
		uint32_t m_tNoOtherBarksExpiration;
	};

	// Says aeBarkType for apSpeaker.
	static void TriggerFollowerBark(Actor* apSpeaker, FollowerBarkType aeBarkType);
	// The dialogue topic of aeBarkType, found by editor ID; null for BARK_DIED and unknown
	// types.
	static TESTopic* GetBarkTopicByType(FollowerBarkType aeBarkType);

protected:
	// How strongly aeBarkType wins over other pending barks.
	static int32_t GetBarkPriority(FollowerBarkType aeBarkType);
	// Milliseconds before aeBarkType may be said again.
	static int32_t GetBarkNoRepeatDuration(FollowerBarkType aeBarkType);

public:
	static void ProcessFollowerBarks();

	// IDA: FollowerBarks::kBarkHistory.
	static BSSimpleList<FollowerBarks::FollowerBarkEntry*> m_BarkHistory;
};
