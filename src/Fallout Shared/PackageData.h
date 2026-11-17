#pragma once

#include <cstdint>

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class PackageLocation;
class PackageTarget;
class TESForm;
class TESObjectWEAP;
class TESTopic;

// When a dialogue package starts talking.
enum PDIAL_TRIGGER_TYPE : int32_t {
    PDTT_NONE = 0,
    PDTT_DISTANCE_CHECK = 1,
    PDTT_AT_LOCATION = 2,
};

// The type data of each package type as a plugin file stores it (TESPackage::Load reads them). Form IDs stay IDs
// until the package's InitItem resolves them.
struct PACK_DIALOGUE_DATA {
    // Byte-swaps the fields.
    void Endian();

    float fFov;
    uint32_t iTopicID;
    bool bNOHeadtracking;
    bool bDoNotControlTarget;
    bool bSpeakerMoveTalk;
    float fDistanceStartTalking;
    bool bSayTo;
    PDIAL_TRIGGER_TYPE eTriggerType;
};

struct PACK_FOLLOW_DATA {
    // Byte-swaps the radius read from a big-endian file.
    void Endian();

    float fTriggerRadius;
};

struct PACK_ESCORT_DATA {
    // Byte-swaps the radius read from a big-endian file.
    void Endian();

    int32_t iFollowPackRadius;
};

struct PACK_PATROL_DATA {
    // Does nothing: there is nothing to swap.
    void Endian();

    bool bRepeatable;
    bool bStartingLocationAtLinkedRef;
};

// The use weapon data of the older files, without bRepeatFire and the weapon.
struct PACK_USE_WEAPON_DATA_PKW2 {
    // Byte-swaps the counts and the waits.
    void Endian();

    bool bAlwaysHit;
    bool bDoNoDamage;
    bool bCrouch;
    bool bHoldFire;
    bool bVolleyFire;
    uint16_t sBurstCount;
    uint16_t sVolleyShotsMin;
    uint16_t sVolleyShotsMax;
    float fVolleyWaitMin;
    float fVolleyWaitMax;
};

struct PACK_USE_WEAPON_DATA_PKW3 {
    // Byte-swaps the PKW2 data; the weapon ID is deliberately left unswapped, as the game does.
    void Endian();

    bool bAlwaysHit;
    bool bDoNoDamage;
    bool bCrouch;
    bool bHoldFire;
    bool bVolleyFire;
    bool bRepeatFire;
    uint16_t sBurstCount;
    uint16_t sVolleyShotsMin;
    uint16_t sVolleyShotsMax;
    float fVolleyWaitMin;
    float fVolleyWaitMax;
    uint32_t iWeapon;
};

// The data only some package types have, owned by the package (TESPackage::pPackData).
class TESPackageData {
public:
    TESPackageData();
    virtual ~TESPackageData();

    virtual void Copy(TESPackageData* apData) {}
    virtual bool Compare(TESPackageData* apData);
    virtual void Save() {}
    virtual void InitItem(TESForm* apOwner) {}
    virtual void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {}
    virtual void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {}
    virtual void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {}

    // The package data classes, including the derived ones, are freed through ourFree.
    static void operator delete(void* apMemory);
};

// An ambush package's data: where the actor lies in wait.
class TESAmbushPackageData : public TESPackageData {
public:
    TESAmbushPackageData();

    // SaveGame and LoadGame are shared with the eat and use item data,
    // InitLoadGame with those and the escort and follow data.
    void Copy(TESPackageData* apCopy) override;
    bool Compare(TESPackageData* apCompare) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    // Deletes the ambush location.
    ~TESAmbushPackageData() override;

    PackageLocation* pAmbushLocation;
};

// A dialogue package's data: what to say, to whom and from where.
class TESDialoguePackageData : public TESPackageData {
public:
    TESDialoguePackageData();
    // From the file data, passed by value.
    TESDialoguePackageData(PACK_DIALOGUE_DATA eDialData);

    void Copy(TESPackageData* apCopy) override;
    bool Compare(TESPackageData* apCompare) override;
    void Save() override;
    void InitItem(TESForm* apOwner) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    // Deletes the target location.
    ~TESDialoguePackageData() override;

    float fFov;
    TESTopic* ptopic;
    PackageLocation* pTargetLocation;
    bool bNOHeadtracking;
    bool bDoNotControlTarget;
    bool bSpeakerMoveTalk;
    float fDistanceStartTalking;
    bool bSayTo;
    PDIAL_TRIGGER_TYPE eTriggerType;
};

// An eat package's data: where the actor looks for food.
class TESEatPackageData : public TESPackageData {
public:
    TESEatPackageData();

    // SaveGame, LoadGame and InitLoadGame are the ambush data's.
    void Copy(TESPackageData* apCopy) override;
    bool Compare(TESPackageData* apCompare) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    // Deletes the search location.
    ~TESEatPackageData() override;

    PackageLocation* pSearchLocation;
};

// An escort package's data: where the escorted actor is searched for and how far he may fall behind.
class TESEscortPackageData : public TESPackageData {
public:
    TESEscortPackageData();
    // From the file data.
    TESEscortPackageData(const PACK_ESCORT_DATA* apEscData);

    // SaveGame and LoadGame are shared with the follow data,
    // InitLoadGame with the ambush data.
    void Copy(TESPackageData* apCopy) override;
    bool Compare(TESPackageData* apCompare) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    // Deletes the search location.
    ~TESEscortPackageData() override;

    PackageLocation* pSearchLocation;
    uint32_t iFollowPackRadius;
};

// A follow package's data: where following starts and how close the actor stays.
class TESFollowPackageData : public TESPackageData {
public:
    TESFollowPackageData();
    // From the file data, passed by value.
    TESFollowPackageData(PACK_FOLLOW_DATA eFolData);

    // SaveGame and LoadGame are the escort data's, InitLoadGame the ambush data's.
    void Copy(TESPackageData* apCopy) override;
    bool Compare(TESPackageData* apCompare) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    // Deletes the start location.
    ~TESFollowPackageData() override;

    PackageLocation* pStartLocation;
    float fTriggerRadius;
};

// A patrol package's data.
class TESPatrolPackageData : public TESPackageData {
public:
    TESPatrolPackageData();
    // From the file data, passed by value.
    TESPatrolPackageData(PACK_PATROL_DATA aPatrolData);

    void Copy(TESPackageData* apCopy) override;
    bool Compare(TESPackageData* apCompare) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

    bool bRepeatable;
    bool bStartingLocationAtLinkedRef;
};

// A use item package's data: where the actor looks for the item.
class TESUseItemPackageData : public TESPackageData {
public:
    TESUseItemPackageData();

    // SaveGame, LoadGame and InitLoadGame are the ambush data's.
    void Copy(TESPackageData* apCopy) override;
    bool Compare(TESPackageData* apCompare) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    // Deletes the search location.
    ~TESUseItemPackageData() override;

    PackageLocation* pSearchLocation;
};

// A use weapon package's data: what to shoot at, from where, and how.
class TESUseWeaponPackageData : public TESPackageData {
public:
    TESUseWeaponPackageData();
    // From the file data of either version, passed by value.
    TESUseWeaponPackageData(PACK_USE_WEAPON_DATA_PKW2 eDialData);
    TESUseWeaponPackageData(PACK_USE_WEAPON_DATA_PKW3 eDialData);

    void Copy(TESPackageData* apCopy) override;
    bool Compare(TESPackageData* apCompare) override;
    void Save() override;
    void InitItem(TESForm* apOwner) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    // Deletes the attack target and the target location.
    ~TESUseWeaponPackageData() override;

    uint16_t GetVolleyShotsMax() const;
    bool GetDoNoDamage() const;
    bool GetAlwaysHit() const {
        return bAlwaysHit;
    }
    bool GetRepeatFire() const;
    // Reads bCrouch.
    bool IsCrouch() const;
    // Reads bHoldFire.
    char IsGetHoldFire() const;
    uint16_t GetVolleyShotsMin() const;
    void SetAttackTarget(PackageTarget* apTarg);

    PackageTarget* pAttackTarget;
    PackageLocation* pTargetLocation;
    bool bAlwaysHit;
    bool bDoNoDamage;
    bool bCrouch;
    bool bHoldFire;
    bool bVolleyFire;
    bool bRepeatFire;
    uint16_t sBurstCount;
    uint16_t sVolleyShotsMin;
    uint16_t sVolleyShotsMax;
    float fVolleyWaitMin;
    float fVolleyWaitMax;
    TESObjectWEAP* pWeapon;
};
