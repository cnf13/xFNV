#pragma once

#include "Fallout Shared/DEFAULT_OBJECT_DATA.h"
#include "Fallout Shared/TESForm.h"

// The engine's default objects (stimpak, player faction, music, ...), by DEFAULT_OBJECT index. DO_SPOTTER_EFFECT = 31
// (TESDataHandler::CompileFiles).
enum DEFAULT_OBJECT : uint32_t {
    DO_STIMPAK,
    DO_SUPERSTIMPAK,
    DO_RADX,
    DO_RADAWAY,
    DO_MORPHINE,
    DO_PERK_PARALYSIS,
    DO_PLAYER_FACTION,
    DO_PERK_STRANGER_NPC,
    DO_PERK_STRANGER_FACTION,
    DO_DEFAULT_MUSIC,
    DO_BATTLE_MUSIC,
    DO_DEATH_MUSIC,
    DO_SUCCESS_MUSIC,
    DO_LEVELUP_MUSIC,
    DO_PLAYERVOICE_MALE,
    DO_PLAYERVOICE_MALE_CHILD,
    DO_PLAYERVOICE_FEMALE,
    DO_PLAYERVOICE_FEMALE_CHILD,
    DO_RANDOM_FOOD,
    DO_EVERY_ACTOR_ABILITY,
    DO_DRUG_EXPIRE_IMAGE_SPACE,
    DO_DOCTOR_BAG,
    DO_PERK_FORTUNE_NPC,
    DO_PERK_FORTUNE_FACTION,
    DO_MELTDOWN_EXPLOSION,
    DO_UNARMED_EFFECT_FORWARD,
    DO_UNARMED_EFFECT_BACKWARD,
    DO_UNARMED_EFFECT_LEFT,
    DO_UNARMED_EFFECT_RIGHT,
    DO_UNARMED_EFFECT_CROUCH,
    DO_UNARMED_EFFECT_COUNTER,
    DO_SPOTTER_EFFECT,
    DO_ITEM_DETECTION_EFFECT,
    DO_CATEYE_MOBILE_EFFECT,
    DO_COUNT,
};

// The form (DOBJ) that holds the default objects.
class BGSDefaultObjectManager : public TESForm {
public:
    BGSDefaultObjectManager();
    ~BGSDefaultObjectManager() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;

    using TESForm::Save;

    // The engine's one default object manager, created on first use.
    static BGSDefaultObjectManager* Instance();
    // The default object aeObject; null while the manager is not initialised.
    static TESForm* GetDefaultObject(DEFAULT_OBJECT aeObject);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void ReleaseInstance();

    TESForm* pObjectArray[DO_COUNT];

    static inline BGSDefaultObjectManager* pSingleton;
};

using kDefaultObjectData_t = DEFAULT_OBJECT_DATA[34];
inline kDefaultObjectData_t kDefaultObjectData{
    {const_cast<char*>("Stimpak"), 47},
    {const_cast<char*>("SuperStimpak"), 47},
    {const_cast<char*>("RadX"), 47},
    {const_cast<char*>("RadAway"), 47},
    {const_cast<char*>("Morphine"), 47},
    {const_cast<char*>("PerkParalysis"), 20},
    {const_cast<char*>("Player Faction"), 8},
    {const_cast<char*>("Mysterious Stranger NPC"), 42},
    {const_cast<char*>("Mysterious Stranger Faction"), 8},
    {const_cast<char*>("Default Music"), 102},
    {const_cast<char*>("Battle Music"), 102},
    {const_cast<char*>("Death Music"), 102},
    {const_cast<char*>("Success Music"), 102},
    {const_cast<char*>("Level Up Music"), 102},
    {const_cast<char*>("Player Voice (Male)"), 93},
    {const_cast<char*>("Player Voice (Male Child)"), 93},
    {const_cast<char*>("Player Voice (Female)"), 93},
    {const_cast<char*>("Player Voice (Female Child)"), 93},
    {const_cast<char*>("Eat Package Default Food"), 85},
    {const_cast<char*>("Every Actor Ability"), 20},
    {const_cast<char*>("Drug Wears Off Image Space"), 84},
    {const_cast<char*>("Doctor's Bag"), 47},
    {const_cast<char*>("Miss Fortune NPC"), 42},
    {const_cast<char*>("Miss Fortune Faction"), 8},
    {const_cast<char*>("Meltdown Explosion"), 81},
    {const_cast<char*>("Unarmed Forward PA"), 20},
    {const_cast<char*>("Unarmed Backward PA"), 20},
    {const_cast<char*>("Unarmed Left PA"), 20},
    {const_cast<char*>("Unarmed Right PA"), 20},
    {const_cast<char*>("Unarmed Crouch PA"), 20},
    {const_cast<char*>("Unarmed Counter PA"), 20},
    {const_cast<char*>("Spotter Effect"), 79},
    {const_cast<char*>("Item Detected Effect"), 79},
    {const_cast<char*>("Cateye Mobile Effect (NYI)"), 79}
};
