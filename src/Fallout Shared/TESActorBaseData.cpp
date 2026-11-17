#include "Fallout Shared/TESActorBaseData.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/BGSVoiceType.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLevCharacter.h"
#include "Fallout Shared/TESLevCreature.h"
#include "Fallout Shared/TESLevItem.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "SDK/MSVC/rtti.h"

TESActorBaseData::KarmicTitleTable TESActorBaseData::pKarmicTitles{
    {&sKarmicTitleGood01, &sKarmicTitleGood02, &sKarmicTitleGood03, &sKarmicTitleGood04, &sKarmicTitleGood05,
     &sKarmicTitleGood06, &sKarmicTitleGood07, &sKarmicTitleGood08, &sKarmicTitleGood09, &sKarmicTitleGood10,
     &sKarmicTitleGood11, &sKarmicTitleGood12, &sKarmicTitleGood13, &sKarmicTitleGood14, &sKarmicTitleGood15,
     &sKarmicTitleGood16, &sKarmicTitleGood17, &sKarmicTitleGood18, &sKarmicTitleGood19, &sKarmicTitleGood20,
     &sKarmicTitleGood21, &sKarmicTitleGood22, &sKarmicTitleGood23, &sKarmicTitleGood24, &sKarmicTitleGood25,
     &sKarmicTitleGood26, &sKarmicTitleGood27, &sKarmicTitleGood28, &sKarmicTitleGood29, &sKarmicTitleGood30,
     &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30,
     &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30,
     &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30,
     &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30, &sKarmicTitleGood30},
    {&sKarmicTitleNeutral01, &sKarmicTitleNeutral02, &sKarmicTitleNeutral03, &sKarmicTitleNeutral04,
     &sKarmicTitleNeutral05, &sKarmicTitleNeutral06, &sKarmicTitleNeutral07, &sKarmicTitleNeutral08,
     &sKarmicTitleNeutral09, &sKarmicTitleNeutral10, &sKarmicTitleNeutral11, &sKarmicTitleNeutral12,
     &sKarmicTitleNeutral13, &sKarmicTitleNeutral14, &sKarmicTitleNeutral15, &sKarmicTitleNeutral16,
     &sKarmicTitleNeutral17, &sKarmicTitleNeutral18, &sKarmicTitleNeutral19, &sKarmicTitleNeutral20,
     &sKarmicTitleNeutral21, &sKarmicTitleNeutral22, &sKarmicTitleNeutral23, &sKarmicTitleNeutral24,
     &sKarmicTitleNeutral25, &sKarmicTitleNeutral26, &sKarmicTitleNeutral27, &sKarmicTitleNeutral28,
     &sKarmicTitleNeutral29, &sKarmicTitleNeutral30, &sKarmicTitleNeutral30, &sKarmicTitleNeutral30,
     &sKarmicTitleNeutral30, &sKarmicTitleNeutral30, &sKarmicTitleNeutral30, &sKarmicTitleNeutral30,
     &sKarmicTitleNeutral30, &sKarmicTitleNeutral30, &sKarmicTitleNeutral30, &sKarmicTitleNeutral30,
     &sKarmicTitleNeutral30, &sKarmicTitleNeutral30, &sKarmicTitleNeutral30, &sKarmicTitleNeutral30,
     &sKarmicTitleNeutral30, &sKarmicTitleNeutral30, &sKarmicTitleNeutral30, &sKarmicTitleNeutral30,
     &sKarmicTitleNeutral30, &sKarmicTitleNeutral30},
    {&sKarmicTitleEvil01, &sKarmicTitleEvil02, &sKarmicTitleEvil03, &sKarmicTitleEvil04, &sKarmicTitleEvil05,
     &sKarmicTitleEvil06, &sKarmicTitleEvil07, &sKarmicTitleEvil08, &sKarmicTitleEvil09, &sKarmicTitleEvil10,
     &sKarmicTitleEvil11, &sKarmicTitleEvil12, &sKarmicTitleEvil13, &sKarmicTitleEvil14, &sKarmicTitleEvil15,
     &sKarmicTitleEvil16, &sKarmicTitleEvil17, &sKarmicTitleEvil18, &sKarmicTitleEvil19, &sKarmicTitleEvil20,
     &sKarmicTitleEvil21, &sKarmicTitleEvil22, &sKarmicTitleEvil23, &sKarmicTitleEvil24, &sKarmicTitleEvil25,
     &sKarmicTitleEvil26, &sKarmicTitleEvil27, &sKarmicTitleEvil28, &sKarmicTitleEvil29, &sKarmicTitleEvil30,
     &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30,
     &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30,
     &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30,
     &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30, &sKarmicTitleEvil30}
};

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> TESActorBaseData::fDefaultFatigue("fDefaultFatigue", 50.0f);
SettingT<GameSettingCollection> TESActorBaseData::fDefaultKarma("fDefaultKarma", 0.0f);
SettingT<GameSettingCollection> TESActorBaseData::fKarmaDeathPenalty("fKarmaDeathPenalty", 1.5f);
SettingT<GameSettingCollection> TESActorBaseData::iBaseDisposition("iBaseDisposition", 35);
SettingT<GameSettingCollection> TESActorBaseData::fAlignEvilMaxKarma("fAlignEvilMaxKarma", -250.0f);
SettingT<GameSettingCollection> TESActorBaseData::fAlignGoodMinKarma("fAlignGoodMinKarma", 250.0f);
SettingT<GameSettingCollection> TESActorBaseData::fAlignVeryEvilMaxKarma("fAlignVeryEvilMaxKarma", -750.0f);
SettingT<GameSettingCollection> TESActorBaseData::fAlignVeryGoodMinKarma("fAlignVeryGoodMinKarma", 750.0f);
SettingT<GameSettingCollection> fAlignMinKarma("fAlignMinKarma", -1000.0f);
SettingT<GameSettingCollection> fAlignMaxKarma("fAlignMaxKarma", 1000.0f);
SettingT<GameSettingCollection> sKarmicTitleGood01("sKarmicTitleGood01", "Vault Guardian");
SettingT<GameSettingCollection> sKarmicTitleGood02("sKarmicTitleGood02", "Vault Martyr");
SettingT<GameSettingCollection> sKarmicTitleGood03("sKarmicTitleGood03", "Sentinel");
SettingT<GameSettingCollection> sKarmicTitleGood04("sKarmicTitleGood04", "Defender");
SettingT<GameSettingCollection> sKarmicTitleGood05("sKarmicTitleGood05", "Dignitary");
SettingT<GameSettingCollection> sKarmicTitleGood06("sKarmicTitleGood06", "Peacekeeper");
SettingT<GameSettingCollection> sKarmicTitleGood07("sKarmicTitleGood07", "Ranger of the Wastes");
SettingT<GameSettingCollection> sKarmicTitleGood08("sKarmicTitleGood08", "Protector");
SettingT<GameSettingCollection> sKarmicTitleGood09("sKarmicTitleGood09", "Urban Avenger");
SettingT<GameSettingCollection> sKarmicTitleGood10("sKarmicTitleGood10", "Exemplar");
SettingT<GameSettingCollection> sKarmicTitleGood11("sKarmicTitleGood11", "Capitol Crusader");
SettingT<GameSettingCollection> sKarmicTitleGood12("sKarmicTitleGood12", "Paladin");
SettingT<GameSettingCollection> sKarmicTitleGood13("sKarmicTitleGood13", "Vault Legend");
SettingT<GameSettingCollection> sKarmicTitleGood14("sKarmicTitleGood14", "Ambassador of Peace");
SettingT<GameSettingCollection> sKarmicTitleGood15("sKarmicTitleGood15", "Urban Legend");
SettingT<GameSettingCollection> sKarmicTitleGood16("sKarmicTitleGood16", "Hero of the Wastes");
SettingT<GameSettingCollection> sKarmicTitleGood17("sKarmicTitleGood17", "Paragon");
SettingT<GameSettingCollection> sKarmicTitleGood18("sKarmicTitleGood18", "Wasteland Savior");
SettingT<GameSettingCollection> sKarmicTitleGood19("sKarmicTitleGood19", "Saint");
SettingT<GameSettingCollection> sKarmicTitleGood20("sKarmicTitleGood20", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleGood21("sKarmicTitleGood21", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleGood22("sKarmicTitleGood22", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleGood23("sKarmicTitleGood23", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleGood24("sKarmicTitleGood24", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleGood25("sKarmicTitleGood25", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleGood26("sKarmicTitleGood26", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleGood27("sKarmicTitleGood27", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleGood28("sKarmicTitleGood28", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleGood29("sKarmicTitleGood29", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleGood30("sKarmicTitleGood30", "Last, Best Hope of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral01("sKarmicTitleNeutral01", "Vault Dweller");
SettingT<GameSettingCollection> sKarmicTitleNeutral02("sKarmicTitleNeutral02", "Vault Renegade");
SettingT<GameSettingCollection> sKarmicTitleNeutral03("sKarmicTitleNeutral03", "Seeker");
SettingT<GameSettingCollection> sKarmicTitleNeutral04("sKarmicTitleNeutral04", "Wanderer");
SettingT<GameSettingCollection> sKarmicTitleNeutral05("sKarmicTitleNeutral05", "Citizen");
SettingT<GameSettingCollection> sKarmicTitleNeutral06("sKarmicTitleNeutral06", "Adventurer");
SettingT<GameSettingCollection> sKarmicTitleNeutral07("sKarmicTitleNeutral07", "Vagabond of the Wastes");
SettingT<GameSettingCollection> sKarmicTitleNeutral08("sKarmicTitleNeutral08", "Mercenary");
SettingT<GameSettingCollection> sKarmicTitleNeutral09("sKarmicTitleNeutral09", "Urban Ranger");
SettingT<GameSettingCollection> sKarmicTitleNeutral10("sKarmicTitleNeutral10", "Observer");
SettingT<GameSettingCollection> sKarmicTitleNeutral11("sKarmicTitleNeutral11", "Capitol Councilor");
SettingT<GameSettingCollection> sKarmicTitleNeutral12("sKarmicTitleNeutral12", "Keeper");
SettingT<GameSettingCollection> sKarmicTitleNeutral13("sKarmicTitleNeutral13", "Vault Descendant");
SettingT<GameSettingCollection> sKarmicTitleNeutral14("sKarmicTitleNeutral14", "Pinnacle of Survival");
SettingT<GameSettingCollection> sKarmicTitleNeutral15("sKarmicTitleNeutral15", "Urban Myth");
SettingT<GameSettingCollection> sKarmicTitleNeutral16("sKarmicTitleNeutral16", "Strider of the Wastes");
SettingT<GameSettingCollection> sKarmicTitleNeutral17("sKarmicTitleNeutral17", "Beholder");
SettingT<GameSettingCollection> sKarmicTitleNeutral18("sKarmicTitleNeutral18", "Wasteland Watcher");
SettingT<GameSettingCollection> sKarmicTitleNeutral19("sKarmicTitleNeutral19", "Super-Human");
SettingT<GameSettingCollection> sKarmicTitleNeutral20("sKarmicTitleNeutral20", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral21("sKarmicTitleNeutral21", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral22("sKarmicTitleNeutral22", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral23("sKarmicTitleNeutral23", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral24("sKarmicTitleNeutral24", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral25("sKarmicTitleNeutral25", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral26("sKarmicTitleNeutral26", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral27("sKarmicTitleNeutral27", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral28("sKarmicTitleNeutral28", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral29("sKarmicTitleNeutral29", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleNeutral30("sKarmicTitleNeutral30", "Paradigm of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil01("sKarmicTitleEvil01", "Vault Delinquent");
SettingT<GameSettingCollection> sKarmicTitleEvil02("sKarmicTitleEvil02", "Vault Outlaw");
SettingT<GameSettingCollection> sKarmicTitleEvil03("sKarmicTitleEvil03", "Opportunist");
SettingT<GameSettingCollection> sKarmicTitleEvil04("sKarmicTitleEvil04", "Plunderer");
SettingT<GameSettingCollection> sKarmicTitleEvil05("sKarmicTitleEvil05", "Fat Cat");
SettingT<GameSettingCollection> sKarmicTitleEvil06("sKarmicTitleEvil06", "Marauder");
SettingT<GameSettingCollection> sKarmicTitleEvil07("sKarmicTitleEvil07", "Pirate of the Wastes");
SettingT<GameSettingCollection> sKarmicTitleEvil08("sKarmicTitleEvil08", "Reaver");
SettingT<GameSettingCollection> sKarmicTitleEvil09("sKarmicTitleEvil09", "Urban Invader");
SettingT<GameSettingCollection> sKarmicTitleEvil10("sKarmicTitleEvil10", "Ne'er-do-well");
SettingT<GameSettingCollection> sKarmicTitleEvil11("sKarmicTitleEvil11", "Capitol Crimelord");
SettingT<GameSettingCollection> sKarmicTitleEvil12("sKarmicTitleEvil12", "Defiler");
SettingT<GameSettingCollection> sKarmicTitleEvil13("sKarmicTitleEvil13", "Vault Boogeyman");
SettingT<GameSettingCollection> sKarmicTitleEvil14("sKarmicTitleEvil14", "Harbinger of War");
SettingT<GameSettingCollection> sKarmicTitleEvil15("sKarmicTitleEvil15", "Urban Superstition");
SettingT<GameSettingCollection> sKarmicTitleEvil16("sKarmicTitleEvil16", "Villain of the Wastes");
SettingT<GameSettingCollection> sKarmicTitleEvil17("sKarmicTitleEvil17", "Fiend");
SettingT<GameSettingCollection> sKarmicTitleEvil18("sKarmicTitleEvil18", "Wasteland Destroyer");
SettingT<GameSettingCollection> sKarmicTitleEvil19("sKarmicTitleEvil19", "Evil Incarnate");
SettingT<GameSettingCollection> sKarmicTitleEvil20("sKarmicTitleEvil20", "Scourge of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil21("sKarmicTitleEvil21", "Scourge of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil22("sKarmicTitleEvil22", "Scourge of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil23("sKarmicTitleEvil23", "Scourge of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil24("sKarmicTitleEvil24", "Scourge of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil25("sKarmicTitleEvil25", "Scourge of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil26("sKarmicTitleEvil26", "Scourge of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil27("sKarmicTitleEvil27", "Scourge of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil28("sKarmicTitleEvil28", "Scourge of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil29("sKarmicTitleEvil29", "Scourge of Humanity");
SettingT<GameSettingCollection> sKarmicTitleEvil30("sKarmicTitleEvil30", "Scourge of Humanity");

void FACTION_RANK::Endian() {
    ENDIAN(pFaction, false);
}

// Swaps every field: the dwords with ENDIAN<long>, the shorts with ENDIAN<short>, in field
// order.
void ACTOR_BASE_DATA::Endian() {
    ENDIAN(iActorBaseFlags, false);
    ENDIAN(usFatigue, false);
    ENDIAN(usBartergold, false);
    ENDIAN(sLevel, false);
    ENDIAN(usCalcLevelMin, false);
    ENDIAN(usCalcLevelMax, false);
    ENDIAN(usSpeedMult, false);
    ENDIAN(fKarma, false);
    ENDIAN(usBaseDisposition, false);
    ENDIAN(sTemplateUseFlags, false);
}

TESActorBaseData::TESActorBaseData() {
    TESActorBaseData::InitializeDataComponent();
}

TESActorBaseData::~TESActorBaseData() {
    TESActorBaseData::ClearDataComponent();
}

void TESActorBaseData::InitializeDataComponent() {
    actorData.iActorBaseFlags = 0;
    actorData.usFatigue = static_cast<uint16_t>(static_cast<int32_t>(fDefaultFatigue.Float()));
    actorData.usBartergold = 0;
    actorData.sLevel = 1;
    actorData.usCalcLevelMin = 0;
    actorData.usCalcLevelMax = 0;
    actorData.usSpeedMult = 100;
    actorData.fKarma = fDefaultKarma.Float();
    actorData.usBaseDisposition = static_cast<uint16_t>(iBaseDisposition.UInt());
    pVoiceType = nullptr;
    pDeathItem = nullptr;
    pTemplateForm = nullptr;
    actorData.sTemplateUseFlags = 0;
    iChangeFlags = 0;
}

void TESActorBaseData::ClearDataComponent() {
    ClearFactionList();
}

void TESActorBaseData::CopyComponent(BaseFormComponent* apCopyFC) {
    TESActorBaseData* pCopy = dynamic_cast<TESActorBaseData*>(apCopyFC);
    if (!pCopy) {
        return;
    }

    SetFlags(*pCopy->GetFlags());
    SetFatigue(pCopy->GetFatigue());
    SetKarma(pCopy->GetKarma());
    SetBarterGold(pCopy->GetBarterGold());
    SetLevel(pCopy->GetLevelExact());
    SetCalcLevelMin(pCopy->GetCalcMin());
    SetCalcLevelMax(pCopy->GetCalcLevelMax());
    SetSpeedMult(pCopy->GetSpeedMultiplier());
    SetBaseDisposition(pCopy->GetBaseDisposition());
    CopyFactionList(pCopy);
    SetDeathItem(pCopy->GetDeathItem());
    SetVoiceType(pCopy->GetVoiceType());
    SetTemplateForm(pCopy->QTemplateForm());
    actorData.sTemplateUseFlags = pCopy->actorData.sTemplateUseFlags;
}

bool TESActorBaseData::CompareComponent(BaseFormComponent* apCompareFC) {
    TESActorBaseData* pCompare = dynamic_cast<TESActorBaseData*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    if (actorData.iActorBaseFlags != pCompare->actorData.iActorBaseFlags) {
        return true;
    }
    int32_t v4 = GetFatigue();
    if (v4 != pCompare->GetFatigue()) {
        return true;
    }
    double v5 = GetKarma();
    if (v5 != pCompare->GetKarma()) {
        return true;
    }
    int32_t BarterGold = GetBarterGold();
    if (BarterGold != pCompare->GetBarterGold()) {
        return true;
    }
    int32_t LevelExact = GetLevelExact();
    if (LevelExact != pCompare->GetLevelExact()) {
        return true;
    }
    int32_t CalcMin = GetCalcMin();
    if (CalcMin != pCompare->GetCalcMin()) {
        return true;
    }
    int32_t CalcLevelMax = GetCalcLevelMax();
    if (CalcLevelMax != pCompare->GetCalcLevelMax()) {
        return true;
    }
    TESLevItem* DeathItem = pDeathItem;
    if (DeathItem != pCompare->pDeathItem) {
        return true;
    }
    int32_t SpeedMultiplier = GetSpeedMultiplier();
    if (SpeedMultiplier != pCompare->GetSpeedMultiplier()) {
        return true;
    }
    int32_t BaseDisposition = GetBaseDisposition();
    if (BaseDisposition != pCompare->GetBaseDisposition()) {
        return true;
    }
    int32_t v13 = (&listFactions)->ItemsInList();
    if (v13 != (&pCompare->listFactions)->ItemsInList()) {
        return true;
    }
    for (FACTION_RANK* pFacRank : *&listFactions) {
        // The rank is sign-extended before comparing it with GetFactionRank's result.
        if (pFacRank && pCompare->GetFactionRank(pFacRank->pFaction, false) != static_cast<int8_t>(pFacRank->cRank)) {
            return true;
        }
    }
    BGSVoiceType* v14 = GetVoiceType();
    if (v14 != pCompare->GetVoiceType()) {
        return true;
    }
    TESForm* v15 = pTemplateForm;
    return v15 != pCompare->pTemplateForm || actorData.sTemplateUseFlags != pCompare->actorData.sTemplateUseFlags;
}

bool TESActorBaseData::GetInvulnerable() const {
    return GetFlag(BASE_CREATURE_INVULNERABLE);
}

bool TESActorBaseData::GetCantOpenDoors() const {
    return GetFlag(BASE_CREATURE_CANT_OPEN_DOORS);
}

bool TESActorBaseData::GetCanBeAllRaces() const {
    return GetFlag(BASE_CAN_BE_ALL_RACES);
}

bool TESActorBaseData::GetAutoCalcServiceFlags() const {
    return GetFlag(BASE_CREATURE_IMMOBILE);
}

uint16_t TESActorBaseData::GetFatigue() const {
    return actorData.usFatigue;
}

bool TESActorBaseData::IsInEvilFactionsOnly() {
    FACTION_RANK* v2;
    uint8_t v3;
    BSSimpleList<FACTION_RANK*>* ecx0 = TESActorBaseData::GetFactionList();
    v3 = (uint8_t)(!ecx0->IsEmpty());
    while (ecx0 && v3) {
        v2 = ecx0->m_item;
        if (v2) {
            if (!v2->pFaction->GetIsEvil()) {
                v3 = 0;
            }
        }
        ecx0 = ecx0->m_pkNext;
    }
    return (bool)v3;
}

void TESActorBaseData::InitItem(TESForm* apOwner) {
    TESFile* pownerFile;
    BSSimpleList<FACTION_RANK*>* pList = &listFactions;
    if (apOwner) {
        pownerFile = apOwner->GetFile(-1);
    }
    else {
        pownerFile = nullptr;
    }
    while (pList) {
        FACTION_RANK* pFacRank = pList->m_item;
        if (!pFacRank) {
            goto LABEL_10;
        }
        if (pFacRank->pFaction) {
            uint32_t tempID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFacRank->pFaction));
            TESForm::AddCompileIndex(tempID, pownerFile);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempID);
            TESFaction* v3 = dynamic_cast<TESFaction*>(FormByNumericID);
            pFacRank->pFaction = v3;
        }
        if (!pFacRank->pFaction) {
            pList->RemoveHead();
        }
        else {
        LABEL_10:;
            pList = pList->m_pkNext;
        }
    }
    uint32_t tempID_2 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pVoiceType));
    if (tempID_2) {
        TESForm::AddCompileIndex(tempID_2, pownerFile);
        TESForm* v4 = TESForm::GetFormByNumericID(tempID_2);
        pVoiceType = dynamic_cast<BGSVoiceType*>(v4);
    }
    tempID_2 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDeathItem));
    if (tempID_2) {
        TESForm::AddCompileIndex(tempID_2, pownerFile);
        TESForm* v5 = TESForm::GetFormByNumericID(tempID_2);
        pDeathItem = dynamic_cast<TESLevItem*>(v5);
        if (!pDeathItem) {
            const char* afKarma = apOwner->GetFormEditorID();
            int32_t aY = apOwner->iFormID;
            Warning("MASTERFILE: Unable to find death item (%08X) on ActorBase (%08X) '%s'.", tempID_2, aY, afKarma);
        }
    }
    tempID_2 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTemplateForm));
    if (tempID_2) {
        TESForm::AddCompileIndex(tempID_2, pownerFile);
        pTemplateForm = TESForm::GetFormByNumericID(tempID_2);
        if (!pTemplateForm) {
            const char* afKarmaa = apOwner->GetFormEditorID();
            int32_t aYa = apOwner->iFormID;
            Warning(
                "MASTERFILE: Unable to find template form (%08X) on ActorBase (%08X) '%s'.", tempID_2, aYa, afKarmaa
            );
        }
    }
    if (!pTemplateForm || !GetTemplateUseFlag(TESActorBaseData::TU_TRAITS)) {
        if (ibaddefaults == -1) {
            for (int32_t i = 0; i < 5; ++i) {
                if (TESActorBaseData::GetAlignmentForKarma(pfDefaultAlignmentValues[i]) != i) {
                    ibaddefaults = 1;
                    float* v6 = &TESActorBaseData::fAlignVeryGoodMinKarma.Float();
                    fgoodverygooddiff = *v6 - TESActorBaseData::fAlignGoodMinKarma.Float();
                    float* v7 = &TESActorBaseData::fAlignGoodMinKarma.Float();
                    fgoodbaddiff = *v7 - TESActorBaseData::fAlignEvilMaxKarma.Float();
                    float* v8 = &TESActorBaseData::fAlignVeryEvilMaxKarma.Float();
                    fbadverybaddiff = *v8 - TESActorBaseData::fAlignEvilMaxKarma.Float();
                    break;
                }
            }
            if (ibaddefaults != 1) {
                ibaddefaults = 0;
            }
        }
        if (ibaddefaults == 1) {
            if (FloatEQ(actorData.fKarma, pfDefaultAlignmentValues[1], 0.001f)) {
                actorData.fKarma = TESActorBaseData::fAlignGoodMinKarma.Float() - fgoodbaddiff * 0.5;
            }
            else {
                if (FloatEQ(actorData.fKarma, pfDefaultAlignmentValues[0], 0.001f)) {
                    actorData.fKarma = fgoodverygooddiff * 0.5 + TESActorBaseData::fAlignGoodMinKarma.Float();
                }
                else {
                    if (FloatEQ(actorData.fKarma, pfDefaultAlignmentValues[2], 0.001f)) {
                        actorData.fKarma = fbadverybaddiff * 0.5 + TESActorBaseData::fAlignEvilMaxKarma.Float();
                    }
                    else {
                        if (FloatEQ(actorData.fKarma, pfDefaultAlignmentValues[3], 0.001f)) {
                            actorData.fKarma = TESActorBaseData::fAlignVeryGoodMinKarma.Float() + 100.0;
                        }
                        else {
                            if (FloatEQ(actorData.fKarma, pfDefaultAlignmentValues[4], 0.001f)) {
                                actorData.fKarma = TESActorBaseData::fAlignVeryEvilMaxKarma.Float() - 100.0;
                            }
                            else {
                                Warning("Invalid karma value found. Unable to fix.");
                            }
                        }
                    }
                }
            }
        }
    }
}

int32_t TESActorBaseData::GetFactionRank(TESFaction* apFaction, bool abPC) {
    FACTION_RANK* pRank;
    BSSimpleList<FACTION_RANK*>* pIter;
    for (pIter = TESActorBaseData::GetFactionList(); pIter && pIter->m_item; pIter = pIter->m_pkNext) {
        pRank = pIter->m_item;
        if (abPC) {
            if (pRank && ((!pRank->pFaction->GetPCExpelled()) && (pRank->pFaction == apFaction))) {
                return (int32_t)((int8_t)pRank->cRank);
            }
        }
        else {
            if (pRank && (pRank->pFaction == apFaction)) {
                return (int32_t)((int8_t)pRank->cRank);
            }
        }
    }
    return (-1);
}

bool TESActorBaseData::GetKnockedDowns() {
    return (bool)((uint8_t)(!GetFlag(0x4000000u)));
}

void TESActorBaseData::ClearFactionList() {
    BSSimpleList<FACTION_RANK*>* pFactions = GetFactionList();
    while (pFactions && !pFactions->IsEmpty()) {
        FACTION_RANK* pEntry = pFactions->GetItem();
        ourFree(pEntry);
        pFactions->RemoveHead();
    }
}

void TESActorBaseData::SetFactionRank(TESFaction* apFaction, char acRank) {
    if (!apFaction) {
        return;
    }

    for (BSSimpleList<FACTION_RANK*>* pFactions = GetFactionList(); pFactions;) {
        FACTION_RANK* pEntry = pFactions->GetItem();
        if (pEntry && pEntry->pFaction == apFaction) {
            if (acRank == -1) {
                pFactions->RemoveHead();
            }
            else {
                pEntry->cRank = static_cast<uint8_t>(acRank);
            }
            return;
        }
        pFactions = pFactions->GetNext();
    }

    FACTION_RANK* pNewEntry = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
    pNewEntry->pFaction = apFaction;
    pNewEntry->cRank = static_cast<uint8_t>(acRank);
    GetFactionList()->AddHead(pNewEntry);
}

bool TESActorBaseData::IsInFaction(TESFaction* apFaction) {
    for (BSSimpleList<FACTION_RANK*>* pFactions = GetFactionList(); pFactions;) {
        FACTION_RANK* pEntry = pFactions->GetItem();
        if (pEntry && pEntry->pFaction == apFaction) {
            return true;
        }
        pFactions = pFactions->GetNext();
    }
    return false;
}

uint16_t TESActorBaseData::GetBarterGold() const {
    return actorData.usBartergold;
}

void TESActorBaseData::SetBarterGold(uint16_t ausNewgold) {
    actorData.usBartergold = ausNewgold;
    AddFormChange(2u);
}

// Adjusts this to the actor base (null-guarded), tests the result and calls RemoveChange. The test of the adjusted
// pointer can never fail for a non-null this.
void TESActorBaseData::RemoveFormChange(int32_t aiChangeFlags) {
    TESActorBase* pBase = static_cast<TESActorBase*>(this);
    if (pBase) {
        pBase->RemoveChange(aiChangeFlags);
    }
}

bool TESActorBaseData::GetFlag(uint32_t aiFlag) const {
    return (actorData.iActorBaseFlags & aiFlag) != 0;
}

uint16_t TESActorBaseData::GetCalcMin() const {
    return actorData.usCalcLevelMin;
}

uint16_t TESActorBaseData::GetSpeedMultiplier() const {
    return actorData.usSpeedMult;
}

bool TESActorBaseData::GetEssential() const {
    return GetFlag(BASE_ESSENTIAL);
}

bool TESActorBaseData::GetRespawn() const {
    return GetFlag(BASE_RESPAWN);
}

bool TESActorBaseData::GetMatchesPCLevel() const {
    return GetFlag(BASE_PC_LEVEL_MULT);
}

bool TESActorBaseData::GetNotPushable() const {
    return GetFlag(BASE_NOT_PUSHABLE);
}

// Whether bit `aeBit` of the template-use flags is set; out-of-range bits count as set.
bool TESActorBaseData::GetTemplateUseFlag(TEMPLATE_USE_FLAG aeFlag) const {
    if (aeFlag < 0 || aeFlag >= 16) {
        return true;
    }

    uint16_t mask = static_cast<uint16_t>(1 << aeFlag);
    return (actorData.sTemplateUseFlags & mask) != 0;
}

void TESActorBaseData::SetTemplateUseFlag(TEMPLATE_USE_FLAG aeFlag, bool abVal) {
    if (aeFlag < 0 || aeFlag >= 16) {
        return;
    }

    uint16_t mask = static_cast<uint16_t>(1 << aeFlag);

    if (abVal) {
        actorData.sTemplateUseFlags = actorData.sTemplateUseFlags | mask;
    }
    else {
        actorData.sTemplateUseFlags = actorData.sTemplateUseFlags & ~mask;
    }
}

bool TESActorBaseData::QCopiedTemplate() const {
    return GetTemplateUseFlag(TU_COPIED);
}

void TESActorBaseData::SetCopiedTemplate(bool abVal) {
    SetTemplateUseFlag(TU_COPIED, abVal);
}

void TESActorBaseData::SetTemplateUseFlags(uint16_t asVal) {
    actorData.sTemplateUseFlags = asVal;
}

// Whether the template actor is a leveled creature or leveled character list.
bool TESActorBaseData::IsTemplateLeveled() const {
    bool leveled = false;

    if (pTemplateForm) {
        int32_t type = pTemplateForm->GetFormType();

        if (type >= 0x2C && type <= 0x2D) {
            leveled = true;
        }
    }

    return leveled;
}

bool TESActorBaseData::GetUsesLeveledTemplate() const {
    return IsTemplateLeveled();
}

bool TESActorBaseData::HasLowLevelProcessing() const {
    return !GetFlag(BASE_NO_LOWLEVEL_PROCESSING);
}

int32_t* TESActorBaseData::GetFlags() {
    return reinterpret_cast<int32_t*>(&actorData.iActorBaseFlags);
}

TESForm* TESActorBaseData::QTemplateForm() const {
    return pTemplateForm;
}

// The PC-level multiplier is the level over 1000 rounded to a float; the product is truncated at extended
// precision, which a double holds exactly here.
uint16_t TESActorBaseData::GetLevel() const {
    uint16_t usLevel = GetLevelExact();
    if (GetMatchesPCLevel()) {
        float fMult = static_cast<int32_t>(usLevel) / 1000.0;
        TESBoundObject* pPlayerBase = pPlayer->GetObjectReference();
        TESActorBaseData* pPlayerData =
            pPlayerBase ? static_cast<TESActorBaseData*>(static_cast<TESActorBase*>(pPlayerBase)) : nullptr;
        if (pPlayerData) {
            double dLevel = static_cast<double>(static_cast<int32_t>(pPlayerData->GetLevelExact())) * fMult;
            usLevel = static_cast<uint16_t>(static_cast<int32_t>(dLevel));
        }

        if (GetCalcMin() > 0 && usLevel < GetCalcMin()) {
            usLevel = GetCalcMin();
        }
        else if (GetCalcLevelMax() > 0 && usLevel > GetCalcLevelMax()) {
            usLevel = GetCalcLevelMax();
        }
    }
    return usLevel;
}

// Replaces apRef's leveled base with the form picked from the template's leveled list: the container the list fills
// gives the pick (its first entry's object), the reference drops to low processing and loses its 3D unless forms are
// being reconstructed, the pathing requests forget its inventory, its container changes and script go, the template
// base is made and set, and the script and inventory are rebuilt (the changes dropped again when empty).
void TESActorBaseData::CalcTemplateForRef(TESObjectREFR* apRef) {
    if (!apRef || !GetUsesLeveledTemplate() || apRef->HasLeveledCreatureCalced()) {
        return;
    }
    TESLeveledList* pList = nullptr;
    if (QTemplateForm()->GetFormType() == LVLN_ID) {
        TESLevCharacter* pLevCharacter = static_cast<TESLevCharacter*>(QTemplateForm());
        pList = pLevCharacter ? static_cast<TESLeveledList*>(pLevCharacter) : nullptr;
    }
    else if (QTemplateForm()->GetFormType() == LVLC_ID) {
        TESLevCreature* pLevCreature = static_cast<TESLevCreature*>(QTemplateForm());
        pList = pLevCreature ? static_cast<TESLeveledList*>(pLevCreature) : nullptr;
    }
    if (!pList) {
        return;
    }
    // Builds the container in local storage and uses the storage address, not the constructor's result, as the game
    // does.
    alignas(TESContainer) uint8_t aContainerStorage[sizeof(TESContainer)];
    TESContainer* pContainer = reinterpret_cast<TESContainer*>(aContainerStorage);
    ::new (aContainerStorage) TESContainer;
    pList->CalculateCurrentFormListForRef(apRef, pContainer);
    TESBoundObject* pTemplate = pContainer->GetObjectList()->GetItem()->pObj;
    if (!pSaveLoadGame || !pSaveLoadGame->GetReconstructingForms()) {
        if (apRef->IsMobileObject()) {
            MobileObject* pMobile = static_cast<MobileObject*>(apRef);
            if (pMobile->GetCurrentAIProcess()) {
                pMobile->MovetoLow();
            }
        }
        apRef->Set3D(nullptr, true);
    }
    if (PathManager::QInstance()->QImpl()) {
        InventoryChanges* pChanges = apRef->GetExtra()->GetContainerChanges();
        PathManager::QInstance()->ClearInventoryPointersFromCurrentRequests(pChanges);
    }
    apRef->GetExtra()->RemoveContainerChanges();
    apRef->GetExtra()->RemoveScript();
    if (apRef->IsActor()) {
        static_cast<Actor*>(apRef)->SetWalkRunFlagDirty();
    }
    TESBoundObject* pOriginalBase = apRef->GetObjectReference();
    TESActorBase* pNewBase = TESActorBaseData::CreateTemplateActorBase(
        static_cast<TESActorBase*>(pOriginalBase), static_cast<TESActorBase*>(pTemplate)
    );
    apRef->SetLeveledCreature(static_cast<TESActorBase*>(pOriginalBase), static_cast<TESActorBase*>(pTemplate));
    apRef->SetObjectReference(pNewBase);
    apRef->InitScript();
    InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(apRef);
    pInventory->InitLeveledItems();
    pInventory->InitFromContainerExtra();
    pInventory->InitScripts();
    if (pInventory->IsEmpty()) {
        apRef->GetExtra()->RemoveContainerChanges();
    }
    pContainer->~TESContainer();
}

// Flags the owning actor base as changed in the save game.
void TESActorBaseData::AddFormChange(int32_t aiChangeFlags) {
    TESActorBase* actorBase = static_cast<TESActorBase*>(this);

    if (actorBase) {
        actorBase->AddChange(aiChangeFlags);
    }
}

void TESActorBaseData::AddChangeFlags(int32_t aiChangeFlags) {
    iChangeFlags |= aiChangeFlags;
}

void TESActorBaseData::SetFlags(int32_t aiNewFlags) {
    actorData.iActorBaseFlags = aiNewFlags;
    AddFormChange(2);
}

// A change of the sex flag records CHANGE_NPC_GENDER on the form this component belongs to, and only when the save has
// not recorded it yet; every other flag records CHANGE_ACTOR_BASE_DATA when abAddChange is set.
void TESActorBaseData::SetFlagBit(int32_t aiFlag, bool abVal, bool abAddChange) {
    bool bWasSet = (aiFlag & actorData.iActorBaseFlags) != 0;
    if (bWasSet != abVal) {
        if (aiFlag == BASE_FEMALE) {
            TESForm* pForm = dynamic_cast<TESForm*>(this);
            if (pForm) {
                BGSChangeFlags kChanges;
                kChanges.~BGSChangeFlags();
                ::new (&kChanges) BGSChangeFlags(CHANGE_NPC_GENDER);
                if (pSaveLoadGame->GetChange(pForm, kChanges)) {
                    RemoveFormChange(CHANGE_NPC_GENDER);
                }
                else {
                    AddFormChange(CHANGE_NPC_GENDER);
                }
            }
        }
        else if (abAddChange) {
            AddFormChange(CHANGE_ACTOR_BASE_DATA);
        }
    }
    if (abVal) {
        actorData.iActorBaseFlags |= aiFlag;
    }
    else {
        actorData.iActorBaseFlags &= ~aiFlag;
    }
}

uint16_t TESActorBaseData::GetSaveSize(uint32_t aiFlags) {
    uint16_t isize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        isize = 6;
    }
    if ((aiFlags & 2) != 0) {
        isize += 24;
    }
    uint16_t isize_2 = 5 * listFactions.ItemsInList() + (isize + 2);
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize_2,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 3150,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize_2, 3150,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp"
            );
        }
    }
    return isize_2;
}

void TESActorBaseData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_BASE_DATA)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&actorData), 0x18, 0);
    }
}

// The loaded flags would overwrite the sex flag, so the current one is put back.
void TESActorBaseData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_BASE_DATA)) {
        bool bFemale = GetFlag(BASE_FEMALE);
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&actorData), 0x18);
        SetFlagBit(BASE_FEMALE, bFemale, true);
    }
}

// Saves the old save format: a block header and a size slot when the save uses blocks, the base data when flagged,
// then the count and the faction ranks (each faction's form ID and rank). The block size is patched in afterwards.
void TESActorBaseData::SaveGameTES(uint32_t aiFlags) {
    uint32_t iFormID;
    uint8_t cRank;
    uint16_t usCount;
    int32_t apData;
    uint16_t usBlockSize;
    usBlockSize = 0;
    void* pBlockSize = nullptr;
    uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // 'BLOK', the block header.
        apData = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&apData, 4u);
        pBlockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, 2u);
    }
    if ((aiFlags & 2) != 0) {
        pSaveLoadGameOLD->SaveGameDataOLD(&actorData, 24u);
    }
    usCount = 0;
    uint16_t* v14 = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&usCount, 2u);
    for (BSSimpleList<FACTION_RANK*>* pNode = GetFactionList(); pNode; pNode = pNode->GetNext()) {
        FACTION_RANK* pRank = pNode->GetItem();
        if (pRank) {
            iFormID = pRank->pFaction->GetFormID();
            cRank = pRank->cRank;
            pSaveLoadGameOLD->SaveNumericID(&iFormID, 4u);
            pSaveLoadGameOLD->SaveGameDataOLD(&cRank, 1u);
            ++usCount;
        }
    }
    *v14 = usCount;
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* v8 = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* CurrentlySavingFormHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (CurrentlySavingFormHeader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(CurrentlySavingFormHeader->iFormID);
            int32_t iFlags = CurrentlySavingFormHeader->iFlags;
            const char* v2 = FormByNumericID->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(v8 - BufferPointer), CurrentlySavingFormHeader->iFormID, v2, iFlags, 3191,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s", static_cast<int32_t>(v8 - BufferPointer), 3191,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pCurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pBlockStart = static_cast<uint8_t*>(pBlockSize);
        if (pCurrent > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp", 3191
            );
        }
        *static_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pCurrent - pBlockStart);
    }
}

// Loads the old save format: a block header and size when the save uses blocks, the base data when flagged, then
// the faction ranks. The block's end is checked against its size afterwards, with a warning on an underrun or overrun.
void TESActorBaseData::LoadGameTES(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    uint8_t v17;
    uint32_t apFormID;
    uint16_t usCount;
    int32_t apData;
    uint16_t usBlockSize;
    usBlockSize = 0;
    uint8_t* BufferPointer = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&apData, 4u);
        // 'BLOK', the block header.
        if (apData != 0x424C4F4B) {
            LoadFormHeader* CurrentlyLoadingFormHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (CurrentlyLoadingFormHeader) {
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(CurrentlyLoadingFormHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp", 3200,
                    CurrentlyLoadingFormHeader->iFormID, FormByNumericID->GetFormEditorID(),
                    CurrentlyLoadingFormHeader->cVersion, CurrentlyLoadingFormHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp", 3200,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, 2u);
    }
    if ((aiFlags & 2) != 0) {
        pSaveLoadGameOLD->LoadGameDataOLD(&actorData, 24u);
    }
    ClearFactionList();
    pSaveLoadGameOLD->LoadGameDataOLD(&usCount, 2u);
    for (int32_t i = 0; i < usCount; ++i) {
        pSaveLoadGameOLD->LoadNumericID(&apFormID, 4u);
        pSaveLoadGameOLD->LoadGameDataOLD(&v17, 1u);
        TESFaction* v16 = static_cast<TESFaction*>(TESForm::GetFormByNumericID(apFormID));
        if (v16) {
            FACTION_RANK* a2 = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
            a2->pFaction = v16;
            a2->cRank = v17;
            (&listFactions)->AddHead(a2);
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* v13 = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* v14 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (v14) {
            TESForm* v12 = TESForm::GetFormByNumericID(v14->iFormID);
            if (v13 <= &BufferPointer[usBlockSize]) {
                if (v13 < &BufferPointer[usBlockSize]) {
                    int32_t iFlags = v14->iFlags;
                    int32_t cVersion = v14->cVersion;
                    const char* v4 = v12->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&BufferPointer[usBlockSize] - v13),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp", 3234,
                        v14->iFormID, v4, cVersion, iFlags
                    );
                }
            }
            else {
                int32_t v7 = v14->iFlags;
                int32_t v5 = v14->cVersion;
                const char* v3 = v12->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(v13 - &BufferPointer[usBlockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp", 3234, v14->iFormID,
                    v3, v5, v7
                );
            }
        }
        else {
            if (v13 <= &BufferPointer[usBlockSize]) {
                if (v13 < &BufferPointer[usBlockSize]) {
                    int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(&BufferPointer[usBlockSize] - v13),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp", 3234,
                        CurrentVersion
                    );
                }
            }
            else {
                int32_t v9 = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(v13 - &BufferPointer[usBlockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBaseData.cpp", 3234, v9
                );
            }
        }
    }
}

void TESActorBaseData::SetMatchesPCLevel(bool abVal) {
    SetFlagBit(BASE_PC_LEVEL_MULT, abVal, true);
}

void TESActorBaseData::SetRunsInLow(bool abValue, bool abAddChange) {
    SetFlagBit(BASE_NO_LOWLEVEL_PROCESSING, !abValue, abAddChange);
}

void TESActorBaseData::SetLevel(uint16_t asValue) {
    actorData.sLevel = static_cast<int16_t>(asValue);
    AddFormChange(2);
}

void TESActorBaseData::SetCalcLevelMin(uint16_t asVal) {
    actorData.usCalcLevelMin = asVal;
    AddFormChange(2);
}

void TESActorBaseData::SetCalcLevelMax(uint16_t asVal) {
    actorData.usCalcLevelMax = asVal;
    AddFormChange(2);
}

void TESActorBaseData::SetBaseDisposition(uint16_t asVal) {
    actorData.usBaseDisposition = asVal;
    AddFormChange(2);
}

void TESActorBaseData::SetFatigue(uint16_t ausFatigue) {
    actorData.usFatigue = ausFatigue;
    AddFormChange(2);
}

void TESActorBaseData::SetSpeedMult(uint16_t asSpeedMult) {
    actorData.usSpeedMult = asSpeedMult;
    AddFormChange(2);
}

uint16_t TESActorBaseData::GetLevelExact() const {
    return (uint16_t)actorData.sLevel;
}

uint16_t TESActorBaseData::GetCalcLevelMax() const {
    return actorData.usCalcLevelMax;
}

uint16_t TESActorBaseData::GetBaseDisposition() const {
    return actorData.usBaseDisposition;
}

TESLevItem* TESActorBaseData::GetDeathItem() {
    return pDeathItem;
}

void TESActorBaseData::SetDeathItem(TESLevItem* apItem) {
    pDeathItem = apItem;
}

void TESActorBaseData::SetVoiceType(BGSVoiceType* apVoiceType) {
    pVoiceType = apVoiceType;
}

void TESActorBaseData::SetKarma(float afKarma) {
    actorData.fKarma = afKarma;
    if (!((&fAlignMaxKarma)->Float() < ((double)actorData.fKarma))) {
        if ((&::fAlignMinKarma)->Float() > ((double)actorData.fKarma)) {
            actorData.fKarma = (&::fAlignMinKarma)->Float();
        }
    }
    else {
        actorData.fKarma = (&fAlignMaxKarma)->Float();
    }
    AddFormChange(2u);
}

void TESActorBaseData::CopyFactionList(TESActorBaseData* apCopy) {
    ClearFactionList();
    BSSimpleList<FACTION_RANK*>* pDest = GetFactionList();
    for (BSSimpleList<FACTION_RANK*>* pSource = apCopy->GetFactionList(); pSource;) {
        FACTION_RANK* pEntry = pSource->GetItem();
        if (pEntry) {
            FACTION_RANK* pNewEntry = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
            ourMemcpy(pNewEntry, pEntry, sizeof(FACTION_RANK));
            pDest->AddTail(pNewEntry);
        }
        BSSimpleList<FACTION_RANK*>* pNext = pDest->GetNext();
        if (pNext) {
            pDest = pNext;
        }
        pSource = pSource->GetNext();
    }
}

void TESActorBaseData::SetTemplateForm(TESForm* apTemplate) {
    pTemplateForm = apTemplate;
}

bool TESActorBaseData::GetNoRumors() {
    return GetFlag(0x2000);
}

bool TESActorBaseData::GetNoRotatingToHeadtrack() {
    return GetFlag(0x40000000);
}

void TESActorBaseData::RemoveChangeFlags(int32_t aiChangeFlags) {
    iChangeFlags &= (~aiChangeFlags);
}

void TESActorBaseData::SetEssential(bool abVal) {
    SetFlagBit(2u, abVal, true);
}

void TESActorBaseData::Save() {
    if (TESForm::QEndianSwapOnSave()) {
        actorData.Endian();
    }
    TESForm::__AddChunkData(ACBS_ID, &actorData, 24u);
    if (TESForm::QEndianSwapOnSave()) {
        actorData.Endian();
    }
    if (pDeathItem) {
        TESForm::AddChunk(INAM_ID, pDeathItem->iFormID);
    }
    if (pVoiceType) {
        TESForm::AddChunk(VTCK_ID, pVoiceType->iFormID);
    }
    if (pTemplateForm) {
        TESForm::AddChunk(TPLT_ID, pTemplateForm->iFormID);
    }
}

void TESActorBaseData::SetPCFactionFlags(PC_FACTION_FLAG aeflag) {
    for (FACTION_RANK* m_item : (&listFactions)->ItemsUntilNull()) {
        TESFaction* pfaction = m_item->pFaction;
        if (m_item) {
            if (pPlayer->GetActorBase()->GetFactionRank(pfaction, true) != -1) {
                if (aeflag) {
                    if (aeflag == PC_FACTIONFLAG_ATTACK) {
                        pfaction->SetPCAttack(true);
                    }
                    else {
                        if (aeflag == PC_FACTIONFLAG_PLAYER_IS_ENEMY) {
                            pfaction->SetPCAnEnemy(true);
                        }
                    }
                }
                else {
                    pfaction->SetPCMurder(true);
                }
            }
        }
    }
}

// The settings are read in the game's order; a NaN karma falls through to neutral.
TESActorBaseData::ALIGNMENT TESActorBaseData::GetAlignmentForKarma(float afKarma) {
    ALIGNMENT eAlignment = ALIGNMENT_NEUTRAL;
    if (fAlignEvilMaxKarma.Float() >= afKarma) {
        if (fAlignVeryEvilMaxKarma.Float() >= afKarma) {
            eAlignment = ALIGNMENT_VERY_EVIL;
        }
        else {
            eAlignment = ALIGNMENT_EVIL;
        }
    }
    else if (fAlignGoodMinKarma.Float() <= afKarma) {
        if (fAlignVeryGoodMinKarma.Float() <= afKarma) {
            eAlignment = ALIGNMENT_VERY_GOOD;
        }
        else {
            eAlignment = ALIGNMENT_GOOD;
        }
    }
    return eAlignment;
}

// The title of apActor's level (clamped to 1-50) in its karma's alignment row; very good karma uses the good row and
// very evil karma the evil row. The level is compared signed and the alignment unsigned.
const char* TESActorBaseData::GetKarmicTitle(Actor* apActor) {
    const char* pTitle = nullptr;
    if (apActor) {
        int32_t iLevel = apActor->GetLevel();
        iLevel = iLevel - 1;
        if (iLevel > 49) {
            iLevel = 49;
        }
        if (iLevel < 0) {
            iLevel = 0;
        }
        float fKarma = static_cast<float>(static_cast<ActorValueOwner*>(apActor)->GetActorValue(ActorValue::KARMA));
        ALIGNMENT eAlignment = GetAlignmentForKarma(fKarma);
        if (static_cast<uint32_t>(eAlignment) > ALIGNMENT_EVIL) {
            if (eAlignment == ALIGNMENT_VERY_GOOD) {
                eAlignment = ALIGNMENT_GOOD;
            }
            else {
                eAlignment = ALIGNMENT_EVIL;
            }
        }
        if (pKarmicTitles[eAlignment][iLevel]) {
            pTitle = pKarmicTitles[eAlignment][iLevel]->String();
        }
    }
    if (!pTitle) {
        pTitle = "";
    }
    return pTitle;
}

TESActorBase* TESActorBaseData::CreateTemplateActorBase(TESActorBase* apOriginalBase, TESActorBase* apTemplateBase) {
    bool bAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
    TESForm* pForm = TESDataHandler::CreateFormOfType(apOriginalBase->GetFormType());
    pForm->Copy(apOriginalBase);
    // Adjusts to the TESActorBaseData subobject without a null check, as the game does.
    TESActorBase* pBase = static_cast<TESActorBase*>(pForm);
    TESActorBaseData& rData = *pBase;
    rData.CopyFromTemplateForm(apTemplateBase);
    pSaveLoadGame->SetThreadAllowChanges(bAllowChanges);
    return pBase;
}

TESForm*
TESActorBaseData::GetLeveledTemplateInHierarchy(TESActorBase* apBase, uint16_t& aiOutCompositeTemplateUseFlags) {
    TESForm* pLeveled = nullptr;
    aiOutCompositeTemplateUseFlags = 0xFFFF;
    TESForm* pForm = apBase;
    while (pForm) {
        if (pForm->IsActorBase()) {
            TESActorBase* pBase = static_cast<TESActorBase*>(pForm);
            aiOutCompositeTemplateUseFlags = aiOutCompositeTemplateUseFlags & pBase->actorData.sTemplateUseFlags;
            pForm = pBase->QTemplateForm();
        }
        else {
            pLeveled = pForm;
            pForm = nullptr;
        }
    }
    return pLeveled;
}

BSSimpleList<ContainerObject*>* TESContainer::GetObjectList() {
    return &objectList;
}
