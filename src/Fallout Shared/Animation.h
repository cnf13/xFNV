#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/KFModel.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

class Actor;
class Animation;
class AnimIdle;
class AnimSequenceBase;
class BGSLoadFormBuffer;
class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class BGSSaveGameBuffer;
class NiControllerManager;
class PlayerCharacter;
class TESAnimGroup;
class TESForm;
class TESIdleForm;
class TESObjectANIO;
class TESObjectREFR;

enum ANIM_GROUP_ACTION : int32_t;

// An idle that may not replay yet and the seconds left: SpecialIdlePlay allocates 8 bytes (idle at +0, its replay
// delay at +4); Update counts fDelay down and frees the entry at zero.
struct IDLE_REPLAY_DELAY {
	TESIdleForm* pIdleForm;
	float fDelay;
};

// The sections of the skeleton an animation group plays in.
enum ANIM_GROUP_SECTION : int32_t {
	AGS_NONE = -1,
	AGS_IDLE = 0,
	AGS_MOVEMENT = 1,
	AGS_LEFT_ARM = 2,
	AGS_TALKING = 3,
	AGS_WEAPON = 4,
	AGS_WEAPON_UP = 5,
	AGS_WEAPON_DOWN = 6,
	AGS_SPECIAL_IDLE = 7,
	ANIM_GROUP_SECTION_COUNT = 8,
	// Aliases the getters map to AGS_MOVEMENT and AGS_WEAPON.
	AGS_WHOLE_BODY = 20,
	AGS_UPPER_BODY = 21,
	ANIM_GROUP_SECTION_TOTAL_COUNT = 22,
	AGS_SKIP_ALL_BUT_SPECIAL_IDLE = 23,
};

// How an animation group plays.
enum ANIM_GROUP_ACTION_TYPE : int32_t {
	AGAT_START_STOP = 0,
	AGAT_LOOP = 1,
	AGAT_SPECIAL_IDLE_LOOP = 2,
	AGAT_EQUIP = 3,
	AGAT_UNEQUIP = 4,
	AGAT_ATTACK = 5,
	AGAT_ATTACK_POWER = 6,
	AGAT_ATTACK_THROW = 7,
	AGAT_ATTACK_MINE = 8,
	AGAT_ATTACK_SPIN = 9,
	AGAT_LOOPING_RELOAD = 10,
	ANIM_GROUP_ACTION_TYPE_COUNT = 11,
};

// How Animation::PlayGroup starts a group: queued behind the playing one, or at once.
enum ACTION_FLAGS : int32_t {
	ACTION_NORMAL = 0,
	ACTION_START = 1,
};

// The static description of one animation group type: its name, section and action keys.
struct ANIM_GROUP_INFO {
	const char* pName;
	bool bMultipleSequences;
	ANIM_GROUP_SECTION eSectionLayer;
	ANIM_GROUP_ACTION_TYPE eAction;
	ANIM_GROUP_ACTION eActions[5];
};

// An idle animation queued or playing on an actor: its idle form, KF model and section. Only some of its members are
// declared.
class AnimIdle : public NiRefObject {
public:
	// eFlags: the idle's load and play state.
	enum ANIM_IDLE_ENUM : int32_t {
		ANIM_IDLE_QUEUED = 0,
		ANIM_IDLE_LOADED = 1,
		ANIM_IDLE_PLAYING = 2,
		ANIM_IDLE_DONE_PLAYING = 3,
	};

	// eType: how the idle was asked to play.
	enum PLAY_TYPE_ENUM : int32_t {
		PLAY_TYPE_REPLACE = 0,
		PLAY_TYPE_QUEUE = 1,
		PLAY_TYPE_AUTO = 2,
		PLAY_TYPE_AUTO_EXCLUSIVE = 3,
	};

	AnimIdle(TESIdleForm* apIdleForm, ANIM_GROUP_SECTION aeSection, PLAY_TYPE_ENUM aeType, Actor* apActor,
		bool abForceLoad, Animation* apAnimation);
	~AnimIdle() override;

	BSAnimGroupSequence* GetSequence();
	// The size of the saved idle data.
	uint16_t GetSaveSize();
	// Returns the load state, eFlags.
	ANIM_IDLE_ENUM GetFlag();
	// Returns the KF model, spKFModel.
	KFModel* GetKFModel();
	// The section it plays in, eSection.
	ANIM_GROUP_SECTION GetSection() { return eSection; }
	void Loaded(KFModel* apKFModel);
	TESIdleForm* GetIdleForm();
	// Holds apSeq as the idle's sequence, pSeq.
	void SetSequence(BSAnimGroupSequence* apSeq);
	// Restores the idle's state from the save buffer for apAnimation.
	void LoadGame(BGSLoadFormBuffer* apBuffer, Animation* apAnimation);
	// Writes the idle's data to the old-format save.
	void SaveGame(float afTime, Animation* apAnimation);
	// Writes the idle's load state, play type and section to apSaveGameBuffer.
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer, Animation* apAnimation);
	// Starts a loaded idle on apSeq; false unless the idle is loaded.
	bool PlayAnim(BSAnimGroupSequence* apSeq);
	// Marks a playing idle done once its sequence stops, releasing the actor's special idle state.
	void Update(Animation* apAnimation);
	void LoadGameTES(float a1, Animation* a2);
	TESObjectANIO* GetAnimObject(int32_t aiIndex);

	// The save size of apAnimIdle's data, block header included.
	static uint16_t GetAnimIdleSaveSize(Actor* apActor, AnimIdle* apAnimIdle);
	// Writes apAnimIdle's idle form ID and data to the old-format save, inside a KBLK block when blocks are used.
	static void SaveAnimIdle(Actor* apActor, AnimIdle* apAnimIdle, Animation* apAnimation, float afTime);
	static void FreeAnimIdleData(AnimIdle* apFreeIdle);
	static AnimIdle* LoadAnimIdle(Actor* apActor, Animation* apAnimation, float afTime);

	// the load state, the play type, the KF model, the section it plays in, and its sequence
	ANIM_IDLE_ENUM eFlags;
	PLAY_TYPE_ENUM eType;
	NiPointer<KFModel> spKFModel;
	ANIM_GROUP_SECTION eSection;
	NiPointer<BSAnimGroupSequence> pSeq;
	TESObjectANIO* pAnimObj[2];
	NiPointer<NiNode> spAddOnObj[2];
	TESIdleForm* pIdleForm;
	Animation* pAnimation;
	Actor* pActor;
};

NiSmartPointer(AnimIdle);

// The animation state of an actor: the groups playing in each section of its skeleton and the scene graph they
// drive.
class Animation {
public:
	Animation();
	~Animation();

	// Queues apKFModel as a replacement animation.
	void QueueReplacementAnimation(KFModel* apKFModel);
	// Clears the blend interpolators of the animation's sequences.
	void ClearBlendInterps();
	// Loads the animation's state from the save buffer.
	void LoadGameBGS(BGSLoadFormBuffer* apBuffer);
	// The group to play in place of ausAnimGroup when it has no sequence.
	uint16_t PickBestAnimation(uint16_t asAnimGroup, bool abRecursive);
	// Whether the special idle is playing.
	bool SpecialIdlePlaying();
	// The group-to-sequence map, pAnimSequences.
	NiTPointerMap<uint16_t, AnimSequenceBase*>* GetMapAnimSequenceBase();
	// The animation group of the sequence stored for group
	// asAnimGroup; null when there is none.
	TESAnimGroup* GetTESAnimGroup(uint16_t asGroup);
	// The speed of the animation group asAnimGroup; 0 when it has no single sequence.
	int16_t GetGroupSpeed(uint16_t asAnimGroup);
	// The idle form of the animation's current idle; null when it
	// has none.
	TESIdleForm* GetSpecialIdleForm();
	// Plays the idle queued for this animation; false when there is none to play.
	bool SpecialIdlePlay();
	// Queues an automatic idle of type aeType on apActor and plays it; the idle, movement and whole-body sections
	// force the auto-exclusive type.
	void SpecialIdleAuto(TESIdleForm* apIdleForm, Actor* apActor, ANIM_GROUP_SECTION aeSection,
		AnimIdle::PLAY_TYPE_ENUM aeType);
	// Whether apIdle's animation is not loaded yet.
	bool IsIdleNotLoaded(TESIdleForm* apIdle);
	// The key character of the next switch key of the weapon sequence, its time in pTime when not null.
	char FindSwitchKey(float* pTime);
	// Whether no special idle is queued or playing any more.
	bool SpecialIdleDonePlaying();
	// Starts on this animation the sequence matching apObject in the multi-sequence of group aeGroup; null when
	// there is none.
	BSAnimGroupSequence* SyncSequences(BSAnimGroupSequence* apSeq, uint16_t asGroup);
	void ZeroGlobalTransform();
	// Stores uVal shifted by uPos into the m_uFlags bits of uMask.
	void SetField(uint8_t uVal, uint8_t uMask, uint8_t uPos);
	// Sets (bVal) or clears the m_uFlags bits of uMask.
	void SetBit(bool bVal, uint8_t uMask);
	// True when any m_uFlags bit of uMask is set.
	bool GetBit(uint8_t uMask);
	// True while KF models are still loading.
	bool GetAnimsLoading();
	// The number of KF models still loading.
	uint32_t GetAnimsLoadingCount();
	NiControllerManager* GetControllerManager();

	// Returns the animated root node, pAnimRoot.
	NiNode* GetSceneRoot() { return pAnimRoot.operator->(); }

	// The multiplier of the time the animations advance by.
	void SetGlobalTimeMultiplier(float fValue);
	// True when a reload group is queued.
	bool ReloadAnimQueued();
	void ReloadTargets(bool abReload);
	// The sequence playing in the section (AGS_WHOLE_BODY reads AGS_MOVEMENT, AGS_UPPER_BODY AGS_WEAPON).
	BSAnimGroupSequence* GetCurrentSequence(ANIM_GROUP_SECTION aeSection);
	// Writes the movement delta of the animations for apRef to delta, rotated with applyRot
	// and without its Z with zeroZ.
	bool GetDelta(NiPoint3* delta, TESObjectREFR* apRef, bool applyRot, bool zeroZ);
	// The action of the group playing in the section.
	ANIM_GROUP_ACTION GetGroupAction(ANIM_GROUP_SECTION aeSection);
	// The group playing in the section (AGS_WHOLE_BODY reads AGS_MOVEMENT, AGS_UPPER_BODY AGS_WEAPON).
	uint16_t GetGroup(ANIM_GROUP_SECTION aeSection);
	// Stops the group playing in the section, easing it out over afEaseOutTime.
	void ClearGroup(ANIM_GROUP_SECTION aeSection, float afEaseOutTime);
	// The time the animations have advanced to.
	float GetTime();
	// The time apSequence has reached: its offset plus the animation's time.
	float GetTime(NiControllerSequence* apSequence);
	// Whether the special idle's KF model has loaded.
	bool SpecialIdleLoaded();
	// Whether apSeq is the sequence of the special idle, the queued one or one freed when inactive.
	bool SpecialIdleWorking(BSAnimGroupSequence* apSeq);
	// Whether the special idle or the queued one is still loading or easing in or playing.
	bool SpecialIdleWorking();
	// Moves the lip sync sequence to afPercent of its key range. afPercent is the result of
	// LipSynchAnim::GetPuppetValue.
	void SetLipTimeMember(float afPercent);
	// Plays asNewGroup in aeSection (AGS_NONE: the group's own section): queued for ACTION_NORMAL, at once for
	// ACTION_START and for equip, attack and reload groups; returns the sequence started, else null.
	BSAnimGroupSequence* PlayGroup(uint16_t asNewGroup, ACTION_FLAGS aeFlags, int32_t aiLoops,
		ANIM_GROUP_SECTION aeSection);
	// Starts asNewGroup in aeSection now (AGS_NONE: the group's own section); the sequence started, else null.
	BSAnimGroupSequence* StartGroup(uint16_t asNewGroup, ANIM_GROUP_SECTION aeSection);
	// Starts apSeq as group asNewGroup in aeSection now; returns the sequence started, else null.
	BSAnimGroupSequence* StartGroup(BSAnimGroupSequence* apSeq, uint16_t asNewGroup, ANIM_GROUP_SECTION aeSection);
	// Blends the section's group out.
	void BlendOut(ANIM_GROUP_SECTION aeSection, bool abIronSightsToggle);
	// Queues the reload group to play (sQueuedReloadGroup).
	void QueueReloadAnim(uint16_t asGroup);
	// The time scale of the animations.
	float GetGlobalTimeMultiplier();
	void Update(TESObjectREFR* ownerObject, float fSecondsPassed, float forceTime);
	// Whether the group asTestGroup is loaded for this animation.
	bool GroupLoaded(uint16_t asTestGroup);
	// Loads the .kf files of apFileList into the animation.
	void AddAnimationsFromList(BSSimpleList<const char*>* apFileList);
	// Plays asNewGroup in aeSection at once, at afNewTime.
	void ForceSection(ANIM_GROUP_SECTION aeSection, uint16_t asNewGroup, ANIM_GROUP_ACTION aeNewAction, float afNewTime,
		char aiSequenceIndex);
	void UpdateMovement(TESObjectREFR* apRef);
	// Updates the bip nodes at afTime with the accumulation root held at its stored translate.
	void UpdateBipOnly(float afTime, NiPoint3* apTranslate, bool abControllers);
	void UpdateSceneGraphNoController();
	// Sets the attack animation speed multiplier.
	void SetAttackSpeed(float attackSpeed);
	// Stores aeSection's low byte in cSkipUpdate.
	void SkipUpdate(ANIM_GROUP_SECTION aeSection);
	// Plays apIdleForm on apActor in section aeSection in place of its special idle.
	void SpecialIdleReplace(TESIdleForm* apIdleForm, Actor* apActor, ANIM_GROUP_SECTION aeSection);
	// Runs the scene graph update queued for the animation; true when it ran.
	bool UpdateQueuedScenegraph();
	void SetTime(float newTime);
	// Sets the reload animation speed multiplier.
	void SetReloadModifier(float fValue);
	// Sets the equip animation speed multiplier.
	void SetEquipSpeed(float afSpeed);
	// Loads the KF files onto the root for apRef; false without an idle animation.
	bool Init(BSSimpleList<const char*>* apFileList, NiNode* apRoot, TESObjectREFR* apRef, bool abUpdate3D);
	// Adds the special animations of the files under apPath.
	void AddSpecialAnimations(BSSimpleList<const char*>* apFileList, const char* apPath);
	// Whether the sequences of the KF models loaded now are cloned later instead of at once.
	bool ShouldQueueSequenceCloning();
	// Adds the KF model's sequences; abDelaySequenceClone queues their cloning.
	bool AddAnimation(KFModel* apKFModel, bool abDelaySequenceClone);
	// Sets or clears the moved bit.
	void SetMoved(bool abFlag);
	// Whether the moved bit is set.
	bool GetMoved();
	// Updates the bip-less scene graph of apRef's animation and its visibility mask without a world update.
	void UpdateMovementNoWorldUpdate(TESObjectREFR* apRef);
	// The smallest blend value of the blend collision objects from apNode up to the accumulation root.
	float GetSmallestHavokBlendValue(NiNode* apNode);
	// Adds the queued idle's KF model once it has loaded; true when it did.
	bool SpecialIdleReplace();
	void InitGroupSpeed(TESAnimGroup* apAnimGroup);
	void SpecialIdleFree(bool abFreeQueued, bool abForceDelete);
	float GetReloadSpeed() const;
	float GetMoveSpeed() { return m_fMoveSpeed; }
	void ClearAccumRootTranslate();
	// pAccumRoot.
	NiNode* GetAccumRoot() { return pAccumRoot; }
	void SkipNextBlend();
	BSAnimGroupSequence* SpecialIdleGetSequence();
	void SetMoveSpeed(float moveSpeed);
	void ClearDelta();
	BSAnimGroupSequence* GetSpecialIdleSequence();
	void SetLoopCount(ANIM_GROUP_SECTION aeSection, int32_t aiCount);
	void AddGroup(TESAnimGroup* pAnimGroup);
	void RemoveObject(NiAVObject* apObject);

protected:
	bool GetDelayUpdateScenegraph();
	void SetDelayUpdateScenegraph(bool abFlag);

public:
	// Reverts the animation of apRef for a TES-format load.
	void RevertTES(TESObjectREFR* apRef);
	// Reverts the animation for a BGS-format load.
	void Revert(BGSLoadGameBuffer* apLoadGameBuffer);
	// Saves the animation into apSaveGameBuffer.
	void SaveGameBGS(BGSSaveGameBuffer* apSaveGameBuffer);
	void SpecialIdleQueue(TESIdleForm* apIdleForm, ANIM_GROUP_SECTION aeSection);
	// The bytes SaveAnimation writes for this animation of apRef.
	uint16_t GetAnimationSaveSize(TESObjectREFR* apRef);
	void ShutdownAllAnimIdles();
	bool ShouldBeMoving();
	char FindNextNoteKey(ANIM_GROUP_SECTION aeSection, const char* apSearchKey, float* apKeyTime);
	bool SpecialIdlePlay(Actor* apActor);
	void AnimIdleFree(NiPointer<AnimIdle>& aspAnimIdle);
	void SaveGameTES(TESForm* apForm);
	void LoadGameTES(TESObjectREFR* apref);

	// Clears the interpolators of the controllers on apObj's sub-tree.
	static void ClearControllersInterpolators(NiAVObject* apObj);
	// Clears the count of animations cloned.
	static void ResetCloneCount();
	// Whether apAVObject or a node under it has skinned geometry.
	static bool FindSkinnedNode(NiAVObject* apAVObject);
	// The length of apSeq's key range; 0 without a sequence.
	static float GetDuration(NiControllerSequence* apSeq);
	// Loads apRef's animation from an old-format save: queues it when its saved size is not 0.
	static void LoadAnimation(TESObjectREFR* apRef, Animation* apAnimation);
	// Moves apSeq on apManager to afPercent of its key range, starting it first when inactive.
	static void SetLipTime(float afPercent, NiControllerManager* apManager, NiControllerSequence* apSeq);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void SaveAnimation(TESObjectREFR* apRef, Animation* apAnimation);

	// Bits of the animation state (SetField, SetBit and GetFlag work on them).
	uint8_t m_uFlags;
	uint8_t padding01[0x3];
	// Any reference: StartGroup tests it with IsActor before using it as an actor.
	TESObjectREFR* pActorRef;
	NiPointer<NiNode> pAnimRoot;
	NiNode* pAccumRoot;
	NiPoint3 movementDelta;
	NiPoint3 AccumRootTranslate;
	NiAVObject* pSoundPriorityBone[8];
	float m_fLooking;
	// Per section: the group playing, its action and loop count, and the group queued next.
	uint16_t group[8];
	ANIM_GROUP_ACTION action[8];
	int32_t loopCount[8];
	uint16_t nextGroup[8];
	int32_t nextLoops[8];
	char cSkipUpdate;
	bool bShutDown;
	uint8_t padding0CE[0x2];
	float time;
	float fLipTime;
	NiPointer<NiControllerManager> spManager;
	NiTPointerMap<uint16_t, AnimSequenceBase*>* pAnimSequenceMap;
	BSAnimGroupSequence* pCurrentSequence[8];
	BSAnimGroupSequence* pLastMovementSequence;
	// The KF models still loading.
	BSSimpleList<KFModel*> kfModelList;
	float m_fMoveSpeed;
	float m_fAttackSpeed;
	float m_fGlobalTimeMultiplier;
	float m_fReloadModifier;
	float m_fEquipModifier;
	char cSkipNextBlend;
	uint8_t padding121;
	// 0xFF when no reload group is queued.
	uint16_t sQueuedReloadGroup;
	NiPointer<AnimIdle> spAnimIdle;
	NiPointer<AnimIdle> spAnimIdleQueued;
	NiPointer<AnimIdle> spAnimIdleFreeWhenInactiveA[2];
	BSSimpleList<IDLE_REPLAY_DELAY*> replayDelayList;

	// The number of animations cloned since the last reset.
	static inline int32_t iAnimationsCloned;
};

// Returns FixedStrings::SpecialIdle_Cast.
NiFixedString* get_g_SpecialIdle_Cast();
int16_t ComputeAnimationSaveSize(PlayerCharacter* a1, Animation* a2);

// One entry per group type (ANIM_GROUP_COUNT, 245), indexed by TESAnimGroup::GetType.
using AnimGroupInfoArray = ANIM_GROUP_INFO[245];
extern AnimGroupInfoArray animGroupInfo;

// The game settings Animation.cpp defines.
extern SettingT<INISettingCollection> fAnimationDefaultBlend;
extern SettingT<INISettingCollection> fAnimationMult;
extern SettingT<INISettingCollection> bClampWarning;
extern SettingT<INISettingCollection> fMenuModeAnimBlend;
extern SettingT<INISettingCollection> bPriorityWarning;
extern SettingT<INISettingCollection> iAnimationClonePerLoop;
using animMoveInfo_t = char* [4];
inline animMoveInfo_t animMoveInfo{const_cast<char*>(""), const_cast<char*>("Sneak"), const_cast<char*>("Swim"),
	const_cast<char*>("Fly")};
