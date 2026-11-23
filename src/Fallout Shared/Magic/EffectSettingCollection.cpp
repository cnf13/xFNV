#include "Fallout Shared/Magic/EffectSettingCollection.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "SDK/MSVC/rtti.h"

EffectSettingCollection EffectSettingCollection::Instance;

EffectSettingCollection::~EffectSettingCollection() {
    ClearData();
}

void EffectSettingCollection::ClearData() {
    for (BSSimpleList<EffectSetting*>* pNode = this; pNode; pNode = pNode->m_pkNext) {
        EffectSetting* pSetting = pNode->m_item;
        if (!pSetting) {
            break;
        }
        delete pSetting;
    }
    RemoveAll();
}

void EffectSettingCollection::AddEffect(EffectSetting* apSetting) {
    if (apSetting) {
        EffectSettingCollection::Instance.AddHead(apSetting);
    }
}

void EffectSettingCollection::InitItems() {
    for (EffectSetting* pSetting : EffectSettingCollection::Instance.ItemsUntilNull()) {
        pSetting->InitItem();
    }
}

EffectSettingCollection* EffectSettingCollection::Construct(void* apMemory) {
    EffectSettingCollection* pThis = static_cast<EffectSettingCollection*>(apMemory);
    ::new (pThis) BSSimpleList<EffectSetting*>;
    return pThis;
}

EffectSetting* EffectSettingCollection::CreateDefaultEffect(
    uint32_t aiFormID,
    const char* acEditorID,
    const char* apEffectName,
    ActorValue::Index aiAssociatedSkill,
    float afBaseCost,
    ActorValue::Index aeAssociatedActorVaue,
    uint32_t aiFlags,
    ActorValue::Index aiResistVariable,
    int32_t aiEffectArchetype
) {
    if (TESForm::GetFormByNumericID(aiFormID)) {
        return nullptr;
    }
    EffectSetting* pSettings = new EffectSetting;
    pSettings->SetFormID(aiFormID, true);
    pSettings->SetFormEditorID(acEditorID);
    pSettings->data.iFlags = aiFlags;
    pSettings->SetEffectName(apEffectName);
    pSettings->SetBaseCost(afBaseCost);
    pSettings->data.eAssociatedActorValue = aeAssociatedActorVaue;
    pSettings->data.iAssociatedSkill = aiAssociatedSkill;
    pSettings->data.iResistVariable = aiResistVariable;
    pSettings->SetEffectArchetype(aiEffectArchetype);
    EffectSettingCollection::AddEffect(pSettings);
    return pSettings;
}

void EffectSettingCollection::CreateDefaultEffects() {
    CreateDefaultEffect(
        0x14A,
        "ScriptEffect",
        "Script Effect",
        ActorValue::SKILL_NONE,
        0.0f,
        ActorValue::VALUE_NONE,
        0x170,
        ActorValue::VALUE_NONE,
        1
    );
    CreateDefaultEffect(
        0x14B,
        "FireDamage",
        "Fire Damage",
        ActorValue::SKILL_NONE,
        0.0f,
        ActorValue::HEALTH,
        0x20000475,
        ActorValue::FIRE_RESISTANCE,
        0
    );
    CreateDefaultEffect(
        0x14C,
        "FrostDamage",
        "Frost Damage",
        ActorValue::SKILL_NONE,
        0.0f,
        ActorValue::HEALTH,
        0x20000475,
        ActorValue::VALUE_NONE,
        0
    );
    CreateDefaultEffect(
        0x14D,
        "ShockDamage",
        "Shock Damage",
        ActorValue::SKILL_NONE,
        0.0f,
        ActorValue::HEALTH,
        0x20000475,
        ActorValue::VALUE_NONE,
        0
    );
    CreateDefaultEffect(
        0x14E,
        "RestoreHealth",
        "Restore Health",
        ActorValue::SKILL_NONE,
        0.0f,
        ActorValue::HEALTH,
        0x70,
        ActorValue::VALUE_NONE,
        0
    );
    pUsageMonitorEffectSetting = CreateDefaultEffect(
        0x14F,
        "UMON",
        "Usage Monitor Effect",
        ActorValue::SKILL_NONE,
        0.0f,
        ActorValue::VALUE_NONE,
        0x12,
        ActorValue::VALUE_NONE,
        1
    );
}

EffectSetting* EffectSettingCollection::GetUsageMonitorEffectSetting() {
    return pUsageMonitorEffectSetting;
}

void EffectSettingCollection::Reset() {
    static_cast<EffectSettingCollection*>(&EffectSettingCollection::Instance)->ClearData();
}

int32_t EffectSettingCollection::GetKnownEffectsSaveSize() {
    int32_t iHeaderSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iHeaderSize = 6;
    }
    int32_t iSize = iHeaderSize + 4;
    for (EffectSetting* pSetting : EffectSettingCollection::Instance.ItemsUntilNull()) {
        if (pSetting->IsFlagSet(EffectSetting::PLAYER_KNOWS)) {
            iSize += 4;
        }
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                iSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                119,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Magic\\EffectSettingCollection.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                iSize,
                119,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Magic\\EffectSettingCollection.cpp"
            );
        }
    }
    return iSize;
}

const char* TESFullName::GetFullName() {
    if (cFullName.pString) {
        return cFullName.pString;
    }

    return "";
}

const char* TESFullName::GetFullName(TESForm* apObj) {
    TESFullName* pFullName = dynamic_cast<TESFullName*>(apObj);
    if (pFullName) {
        return pFullName->GetFullName();
    }

    return "";
}
