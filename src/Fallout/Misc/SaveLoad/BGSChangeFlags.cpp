#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"

#include "BSHavok/bhkCollisionFilter.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"

BGSChangeFlags::BGSChangeFlags() : iFlags(0) {}

BGSChangeFlags::BGSChangeFlags(int32_t aiFlags) : iFlags(aiFlags) {}

void BGSChangeFlags::RemoveFlags(int32_t aiFlags) {
    iFlags &= (~aiFlags);
}

bool BGSChangeFlags::CheckFlags(int32_t aiFlags) {
    return ((aiFlags & ((uint32_t)iFlags)) != 0);
}

void BGSChangeFlags::AddFlags(int32_t aiFlags) {
    iFlags |= aiFlags;
}

bool BGSChangeFlags::Equals(int32_t aiFlags) const {
    return iFlags == aiFlags;
}

BGSChangeFlags* BGSChangeFlags::GetDifference(BGSChangeFlags* apResult, int32_t aFlags) const {
    ::new (apResult) CFilter((uint32_t)((int32_t)(((uint32_t)iFlags) & (~aFlags))));
    return apResult;
}

bool BGSChangeFlags::ReduceToForcedChanges(TESForm* apForm) {
    ENUM_FORM_ID eFormType = apForm->GetFormType();
    BGSChangeFlags inewFlags;
    bool bCreated = false;
    switch (eFormType) {
        case REFR_ID:
        case ACHR_ID:
        case ACRE_ID:
        case PMIS_ID:
        case PGRE_ID:
        case PBEA_ID:
        case PFLA_ID:
        case PCBE_ID: {
            TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
            if (pDataHandler->IsFormIDCreated(apForm->iFormID)) {
                bCreated = true;
            }
            if (pRef->IsCellLoaded()) {
                if (CheckFlags(0x10000000)) {
                    inewFlags.AddFlags(0x10000000);
                }
                if (CheckFlags(0x8000000)) {
                    inewFlags.AddFlags(0x8000000);
                }
            }
            if (eFormType >= ACHR_ID && eFormType <= ACRE_ID && CheckFlags(0x40000)) {
                inewFlags.AddFlags(0x40000);
            }
            break;
        }
        case QUST_ID: {
            if (CheckFlags(0x40000000)) {
                inewFlags.AddFlags(0x40000000);
            }
            break;
        }
        default: {
            break;
        }
    }
    iFlags = inewFlags.iFlags;
    return iFlags || bCreated;
}

BGSChangeFlags* BGSChangeFlags::GetUnrevertibleChanges(
    BGSChangeFlags* apResult,
    BGSChangeFlags aiOldChangeFlags,
    BGSChangeFlags aiNewChangeFlags,
    uint32_t aiFormType,
    bool abWillLoad
) {
    aiOldChangeFlags.RemoveFlags(aiNewChangeFlags.iFlags);
    aiOldChangeFlags.RemoveFlags(0xFFFF0000u);
    // The type is copied into an ENUM_FORM_ID local and compared signed.
    if (abWillLoad) {
        ENUM_FORM_ID eFormType = static_cast<ENUM_FORM_ID>(aiFormType);
        if (eFormType >= ACHR_ID && eFormType <= ACRE_ID) {
            aiOldChangeFlags.RemoveFlags(1024);
        }
    }
    apResult->iFlags = aiOldChangeFlags.iFlags;
    return apResult;
}

const char* BGSChangeFlags::GetFlagName(int32_t aiFlag, uint32_t aiFormType, bool abShortName) {
    // The type is copied into an ENUM_FORM_ID local and compared signed.
    ENUM_FORM_ID eFormType = static_cast<ENUM_FORM_ID>(aiFormType);
    if (aiFlag == 1) {
        if (abShortName) {
            return "Flags";
        }

        return "CHANGE_FORM_FLAGS";
    }
    switch (eFormType) {
        case CLAS_ID: {
            if (aiFlag != 2) {
                return "Bad Flag Name";
            }
            if (abShortName) {
                return "Tag Skills";
            }

            return "CHANGE_CLASS_TAG_SKILLS";
        }
        case FACT_ID: {
            switch (aiFlag) {
                case (-2147483647 - 1): {
                    if (abShortName) {
                        return "Faction Crime Counts";
                    }

                    return "CHANGE_FACTION_CRIME_COUNTS";
                }
                case 2: {
                    if (abShortName) {
                        return "Faction Flags";
                    }

                    return "CHANGE_FACTION_FLAGS";
                }
                case 4: {
                    if (abShortName) {
                        return "Faction Reactions";
                    }

                    return "CHANGE_FACTION_REACTIONS";
                }
                default: {
                    return "Bad Flag Name";
                }
            }
        }
        case NPC__ID:
        case CREA_ID: {
            const char* result;
            switch (aiFlag) {
                case 2: {
                    const char* v24;
                    if (abShortName) {
                        v24 = "Base Data";
                    }
                    else {
                        v24 = "CHANGE_ACTOR_BASE_DATA";
                    }
                    result = v24;
                    break;
                }
                case 4: {
                    const char* v23;
                    if (abShortName) {
                        v23 = "Attributes";
                    }
                    else {
                        v23 = "CHANGE_ACTOR_BASE_ATTRIBUTES";
                    }
                    result = v23;
                    break;
                }
                case 8: {
                    const char* v22;
                    if (abShortName) {
                        v22 = "AI Data";
                    }
                    else {
                        v22 = "CHANGE_ACTOR_BASE_AIDATA";
                    }
                    result = v22;
                    break;
                }
                case 16: {
                    const char* v21;
                    if (abShortName) {
                        v21 = "Spell List";
                    }
                    else {
                        v21 = "CHANGE_ACTOR_BASE_SPELLLIST";
                    }
                    result = v21;
                    break;
                }
                case 32: {
                    const char* v20;
                    if (abShortName) {
                        v20 = "Full Name";
                    }
                    else {
                        v20 = "CHANGE_ACTOR_BASE_FULLNAME";
                    }
                    result = v20;
                    break;
                }
                default: {
                    if (aiFormType == 42) {
                        if (aiFlag > 2048) {
                            if (aiFlag == CHANGE_NPC_GENDER) {
                                const char* v15;
                                if (abShortName) {
                                    v15 = "Gender";
                                }
                                else {
                                    v15 = "CHANGE_NPC_GENDER";
                                }
                                result = v15;
                            }
                            else {
                                const char* v16;
                                if (aiFlag != CHANGE_NPC_RACE) {
                                    return "Bad Flag Name";
                                }
                                if (abShortName) {
                                    v16 = "Race";
                                }
                                else {
                                    v16 = "CHANGE_NPC_RACE";
                                }
                                result = v16;
                            }
                        }
                        else {
                            switch (aiFlag) {
                                case 2048: {
                                    const char* v17;
                                    if (abShortName) {
                                        v17 = "Face";
                                    }
                                    else {
                                        v17 = "CHANGE_NPC_FACE";
                                    }
                                    result = v17;
                                    break;
                                }
                                case 512: {
                                    const char* v19;
                                    if (abShortName) {
                                        v19 = "NPC Skills";
                                    }
                                    else {
                                        v19 = "CHANGE_NPC_SKILLS";
                                    }
                                    result = v19;
                                    break;
                                }
                                case 1024: {
                                    const char* v18;
                                    if (abShortName) {
                                        v18 = "Class";
                                    }
                                    else {
                                        v18 = "CHANGE_NPC_CLASS";
                                    }
                                    result = v18;
                                    break;
                                }
                                default: {
                                    return "Bad Flag Name";
                                }
                            }
                        }
                    }
                    else {
                        const char* v14;
                        if (aiFormType != 43 || aiFlag != 512) {
                            return "Bad Flag Name";
                        }
                        if (abShortName) {
                            v14 = "Creature Skills";
                        }
                        else {
                            v14 = "CHANGE_CREATURE_SKILLS";
                        }
                        result = v14;
                    }
                    break;
                }
            }
            return result;
        }
        case LVLC_ID:
        case LVLN_ID:
        case LVLI_ID: {
            if (aiFlag != CHANGE_LEVELED_LIST_ADDED_OBJECT) {
                return "Bad Flag Name";
            }
            if (abShortName) {
                return "Added Object";
            }

            return "CHANGE_LEVELED_LIST_ADDED_OBJECT";
        }
        case NOTE_ID: {
            if (aiFlag != CHANGE_NOTE_READ) {
                return "Bad Flag Name";
            }
            if (abShortName) {
                return "Note Read";
            }

            return "CHANGE_NOTE_READ";
        }
        case CELL_ID: {
            if (aiFlag > 8) {
                switch (aiFlag) {
                    case 0x10000000: {
                        if (abShortName) {
                            return "Exterior Short";
                        }

                        return "CHANGE_CELL_EXTERIOR_SHORT";
                    }
                    case 0x20000000: {
                        if (abShortName) {
                            return "Exterior Char";
                        }

                        return "CHANGE_CELL_EXTERIOR_CHAR";
                    }
                    case 0x40000000: {
                        if (abShortName) {
                            return "Detach Time";
                        }

                        return "CHANGE_CELL_DETACHTIME";
                    }
                    default: {
                        return "Bad Flag Name";
                    }
                }
            }
            else {
                switch (aiFlag) {
                    case 8: {
                        if (abShortName) {
                            return "Ownership";
                        }

                        return "CHANGE_CELL_OWNERSHIP";
                    }
                    case (-2147483647 - 1): {
                        if (abShortName) {
                            return "Seen Data";
                        }

                        return "CHANGE_CELL_SEENDATA";
                    }
                    case 2: {
                        if (abShortName) {
                            return "Flags";
                        }

                        return "CHANGE_CELL_FLAGS";
                    }
                    case 4: {
                        if (abShortName) {
                            return "Full name";
                        }

                        return "CHANGE_CELL_FULLNAME";
                    }
                    default: {
                        return "Bad Flag Name";
                    }
                }
            }
        }
        case REFR_ID:
        case ACHR_ID:
        case ACRE_ID:
        case PMIS_ID:
        case PGRE_ID:
        case PBEA_ID:
        case PFLA_ID:
        case PCBE_ID: {
            if (aiFlag > 64) {
                if (aiFlag > 0x10000000) {
                    if (aiFlag == CHANGE_REFR_EXTRA_ENCOUNTER_ZONE) {
                        if (abShortName) {
                            return "Enc Zone Extra";
                        }

                        return "CHANGE_REFR_EXTRA_ENCOUNTER_ZONE";
                    }
                    if (aiFlag == CHANGE_REFR_EXTRA_CREATED_ONLY) {
                        if (abShortName) {
                            return "Created Only Extra";
                        }

                        return "CHANGE_REFR_EXTRA_CREATED_ONLY";
                    }
                }
                else {
                    switch (aiFlag) {
                        case 0x10000000: {
                            if (abShortName) {
                                return "Animation";
                            }

                            return "CHANGE_REFR_ANIMATION";
                        }
                        case 0x4000000: {
                            if (abShortName) {
                                return "Activating Children";
                            }

                            return "CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN";
                        }
                        case 0x8000000: {
                            if (abShortName) {
                                return "Leveled Inventory";
                            }

                            return "CHANGE_REFR_LEVELED_INVENTORY";
                        }
                    }
                }
            }
            else {
                if (aiFlag == 64) {
                    if (abShortName) {
                        return "Ownership Extra";
                    }

                    return "CHANGE_REFR_EXTRA_OWNERSHIP";
                }
                if (aiFlag > 8) {
                    if (aiFlag == 16) {
                        if (abShortName) {
                            return "Scale";
                        }

                        return "CHANGE_REFR_SCALE";
                    }
                    if (aiFlag == 32) {
                        if (abShortName) {
                            return "Inventory";
                        }

                        return "CHANGE_REFR_INVENTORY";
                    }
                }
                else {
                    switch (aiFlag) {
                        case 8: {
                            if (abShortName) {
                                return "Cell Changed";
                            }

                            return "CHANGE_REFR_CELL_CHANGED";
                        }
                        case (-2147483647 - 1): {
                            if (abShortName) {
                                return "Game Only Extra";
                            }

                            return "CHANGE_REFR_EXTRA_GAME_ONLY";
                        }
                        case 2: {
                            if (abShortName) {
                                return "Moved";
                            }

                            return "CHANGE_REFR_MOVE";
                        }
                        case 4: {
                            if (abShortName) {
                                return "Havok Moved";
                            }

                            return "CHANGE_REFR_HAVOK_MOVE";
                        }
                    }
                }
            }
            if (eFormType < ACHR_ID || eFormType > ACRE_ID) {
                if (aiFlag > 0x1000) {
                    if (aiFlag == CHANGE_OBJECT_OPEN_DEFAULT_STATE) {
                        if (abShortName) {
                            return "Open Default State";
                        }

                        return "CHANGE_OBJECT_OPEN_DEFAULT_STATE";
                    }
                    if (aiFlag == CHANGE_OBJECT_OPEN_STATE) {
                        if (abShortName) {
                            return "Open State";
                        }

                        return "CHANGE_OBJECT_OPEN_STATE";
                    }
                }
                else {
                    switch (aiFlag) {
                        case 0x1000: {
                            if (abShortName) {
                                return "Lock Extra";
                            }

                            return "CHANGE_OBJECT_EXTRA_LOCK";
                        }
                        case 1024: {
                            if (abShortName) {
                                return "Item Data Extra";
                            }

                            return "CHANGE_OBJECT_EXTRA_ITEM_DATA";
                        }
                        case 2048: {
                            if (abShortName) {
                                return "Ammo Extra";
                            }

                            return "CHANGE_OBJECT_EXTRA_AMMO";
                        }
                    }
                }
                if (aiFormType != 28 || aiFlag != CHANGE_DOOR_EXTRA_TELEPORT) {
                    return "Bad Flag Name";
                }
                if (abShortName) {
                    return "Teleport Extra";
                }

                return "CHANGE_DOOR_EXTRA_TELEPORT";
            }

            if (aiFlag > 0x80000) {
                if (aiFlag > 0x400000) {
                    if (aiFlag != CHANGE_ACTOR_PERMANENT_MODIFIERS) {
                        return "Bad Flag Name";
                    }
                    if (abShortName) {
                        return "Permanent Modifiers";
                    }

                    return "CHANGE_ACTOR_PERMANENT_MODIFIERS";
                }

                switch (aiFlag) {
                    case 0x400000: {
                        if (abShortName) {
                            return "Override Modifiers";
                        }

                        return "CHANGE_ACTOR_OVERRIDE_MODIFIERS";
                    }
                    case 0x100000: {
                        if (abShortName) {
                            return "Temp Modifiers";
                        }

                        return "CHANGE_ACTOR_TEMP_MODIFIERS";
                    }
                    case 0x200000: {
                        if (abShortName) {
                            return "Damage Modifiers";
                        }

                        return "CHANGE_ACTOR_DAMAGE_MODIFIERS";
                    }
                    default: {
                        return "Bad Flag Name";
                    }
                }
            }
            else {
                if (aiFlag == CHANGE_ACTOR_DISPOSITION_MODIFIERS) {
                    if (abShortName) {
                        return "Disp Modifiers";
                    }

                    return "CHANGE_ACTOR_DISPOSITION_MODIFIERS";
                }

                if (aiFlag > 0x1000) {
                    if (aiFlag == CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS) {
                        if (abShortName) {
                            return "Dismembered Limbs";
                        }

                        return "CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS";
                    }

                    if (aiFlag != CHANGE_ACTOR_EXTRA_LEVELED_ACTOR) {
                        return "Bad Flag Name";
                    }
                    if (abShortName) {
                        return "Leveled Actor";
                    }

                    return "CHANGE_ACTOR_LEVELED_ACTOR";
                }

                switch (aiFlag) {
                    case 0x1000: {
                        if (abShortName) {
                            return "Merchant Container";
                        }

                        return "CHANGE_ACTOR_EXTRA_MERCHANT_CONTAINER";
                    }
                    case 1024: {
                        if (abShortName) {
                            return "Life State";
                        }

                        return "CHANGE_ACTOR_LIFESTATE";
                    }
                    case 2048: {
                        if (abShortName) {
                            return "Package Data Extra";
                        }

                        return "CHANGE_ACTOR_EXTRA_PACKAGE_DATA";
                    }
                    default: {
                        return "Bad Flag Name";
                    }
                }
            }
        }
        case INFO_ID: {
            if (aiFlag != CHANGE_TOPIC_SAIDONCE) {
                return "Bad Flag Name";
            }
            if (abShortName) {
                return "Said Once";
            }

            return "CHANGE_TOPIC_SAIDONCE";
        }
        case QUST_ID: {
            if (aiFlag > 4) {
                if (aiFlag == CHANGE_QUEST_OBJECTIVES) {
                    if (abShortName) {
                        return "Quest Objectives";
                    }

                    return "CHANGE_QUEST_OBJECTIVES";
                }

                if (aiFlag != CHANGE_QUEST_SCRIPT) {
                    return "Bad Flag Name";
                }
                if (abShortName) {
                    return "Quest Script";
                }

                return "CHANGE_QUEST_SCRIPT";
            }

            switch (aiFlag) {
                case 4: {
                    if (abShortName) {
                        return "Quest Script Delay";
                    }

                    return "CHANGE_QUEST_SCRIPT_DELAY";
                }
                case (-2147483647 - 1): {
                    if (abShortName) {
                        return "Quest Stages";
                    }

                    return "CHANGE_QUEST_STAGES";
                }
                case 2: {
                    if (abShortName) {
                        return "Quest Flags";
                    }

                    return "CHANGE_QUEST_FLAGS";
                }
                default: {
                    return "Bad Flag Name";
                }
            }
        }
        case PACK_ID: {
            if (aiFlag == CHANGE_PACKAGE_NEVER_RUN) {
                if (abShortName) {
                    return "Never Run Flag";
                }

                return "CHANGE_PACKAGE_NEVER_RUN";
            }

            if (aiFlag != CHANGE_PACKAGE_WAITING) {
                return "Bad Flag Name";
            }
            if (abShortName) {
                return "Waiting Flag";
            }

            return "CHANGE_PACKAGE_WAITING";
        }
        case WATR_ID: {
            if (aiFlag != CHANGE_WATER_REMAPPED) {
                return "Bad Flag Name";
            }
            if (abShortName) {
                return "Remapped";
            }

            return "CHANGE_WATER_REMAPPED";
        }
        case FLST_ID: {
            if (aiFlag != CHANGE_FORM_LIST_ADDED_FORM) {
                return "Bad Flag Name";
            }
            if (abShortName) {
                return "Added Form";
            }

            return "CHANGE_FORM_LIST_ADDED_FORM";
        }
        case ECZN_ID: {
            if (aiFlag == CHANGE_ENCOUNTER_ZONE_GAME_DATA) {
                if (abShortName) {
                    return "Game Data";
                }

                return "CHANGE_ENCOUNTER_ZONE_GAME_DATA";
            }

            if (aiFlag != 2) {
                return "Bad Flag Name";
            }
            if (abShortName) {
                return "Zone Flags";
            }

            return "CHANGE_ENCOUNTER_ZONE_FLAGS";
        }
        default: {
            if (aiFlag == 2) {
                if (abShortName) {
                    return "Object Value";
                }

                return "CHANGE_BASE_OBJECT_VALUE";
            }

            if (aiFlag == 4) {
                if (abShortName) {
                    return "Object Full Name";
                }

                return "CHANGE_BASE_OBJECT_FULLNAME";
            }

            if (aiFormType != 22) {
                if (aiFormType == 25 && aiFlag == 32) {
                    if (abShortName) {
                        return "Teaches Skill";
                    }

                    return "CHANGE_BOOK_TEACHES_SKILL";
                }
                return "Bad Flag Name";
            }
            if (aiFlag != CHANGE_TALKING_ACTIVATOR_SPEAKER) {
                return "Bad Flag Name";
            }
            if (abShortName) {
                return "Speaker";
            }

            return "CHANGE_TALKING_ACTIVATOR_SPEAKER";
        }
    }
}
