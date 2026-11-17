#include "Fallout/Magic/ScriptEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"

// Whether the script archetype took Instantiate: stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::SCRIPT, &ScriptEffect::Instantiate);

ScriptEffect::ScriptEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ActiveEffect(apCaster, apItem, apEffect), pScript(apEffect->GetScript()), pEffectLocals(nullptr) {}

ScriptEffect::~ScriptEffect() {
    if (pEffectLocals) {
        delete pEffectLocals;
    }
}

// Allocates a copy, constructs it from this effect's caster, spell and effect item, then copies the effect data.
ActiveEffect* ScriptEffect::Copy() {
    void* pMemory = ourMalloc(sizeof(ScriptEffect));
    ScriptEffect* pCopy;
    if (pMemory) {
        pCopy = ::new (pMemory) ScriptEffect(pCaster, pSpell, pEffect);
    }
    else {
        pCopy = nullptr;
    }
    CopyData(pCopy);
    return pCopy;
}

void ScriptEffect::Update(float afSecondsElapsed) {
    TESObjectREFR* v2;
    ScriptLocals* pLocals;
    MagicTarget* unk24;
    if (pScript) {
        unk24 = ActiveEffect::GetTarget();
        pLocals = pEffectLocals;
        v2 = unk24->GetTargetStatsObject();
        pScript->RunScriptEffectUpdate(v2, pLocals, afSecondsElapsed);
    }
}

// Save version 0x2A added a word; the effect's locals follow.
uint16_t ScriptEffect::GetSaveSize(Actor* apActor) {
    uint16_t usSize = ActiveEffect::GetSaveSize(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x2A) {
        usSize += 2;
    }
    if (pEffectLocals) {
        usSize += pEffectLocals->GetSaveSize();
    }
    return usSize;
}

// A byte saying whether locals follow, then the locals.
void ScriptEffect::SaveGame(BGSSaveFormBuffer* apBuffer) {
    ActiveEffect::SaveGame(apBuffer);
    uint8_t bHasLocals = pEffectLocals != nullptr;
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHasLocals), 1, 0);
    if (bHasLocals) {
        pEffectLocals->SaveGame(apBuffer);
    }
}

// Save version 0x2A added the locals' size; older saves always carry locals.
void ScriptEffect::SaveGame(Actor* apActor) {
    ActiveEffect::SaveGame(apActor);
    uint16_t usSize = 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x2A) {
        if (pEffectLocals) {
            usSize = pEffectLocals->GetSaveSize();
        }
        pSaveLoadGameOLD->SaveGameDataOLD(&usSize, 2);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x2A) {
        usSize = 1;
    }
    if (usSize) {
        pEffectLocals->SaveGame();
    }
}

void ScriptEffect::LoadGame(BGSLoadFormBuffer* a1) {
    char apData;
    ActiveEffect::LoadGame(a1);
    apData = 0;
    a1->LoadData(&apData, 1u);
    if (apData) {
        if (!pEffectLocals) {
            pEffectLocals = pScript->CloneLocals();
        }
        if (pEffectLocals) {
            pEffectLocals->LoadGame(a1);
        }
        else {
            ScriptLocals apMemory;
            apMemory.LoadGame(a1);
        }
    }
}

// Without a script to clone locals from, the saved locals are skipped.
void ScriptEffect::LoadGame(Actor* apActor) {
    ActiveEffect::LoadGame(apActor);
    uint16_t usSize = 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x2A) {
        pSaveLoadGameOLD->LoadGameDataOLD(&usSize, 2);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x2A) {
        usSize = 1;
    }
    if (usSize) {
        if (!pEffectLocals) {
            pEffectLocals = pScript->CloneLocals();
        }
        if (pEffectLocals) {
            pEffectLocals->LoadGame();
        }
        else {
            pSaveLoadGameOLD->AdvanceBuffer(usSize);
        }
    }
}

void ScriptEffect::Revert(Actor* a1) {
    ActiveEffect::Revert(a1);
    if (pEffectLocals) {
        pEffectLocals->Revert_0();
    }
}

void ScriptEffect::CopyData(ActiveEffect* apDestination) {
    ActiveEffect::CopyData(apDestination);
    ScriptEffect* pse = dynamic_cast<ScriptEffect*>(apDestination);
    if (pse) {
        pse->pScript = pScript;
        if (pScript) {
            if (pEffectLocals) {
                pse->pEffectLocals = pScript->CloneLocals();
                pse->pEffectLocals->CopyActionList(pEffectLocals);
                for (BSSimpleList<SCRIPT_LOCAL*>* plist = pEffectLocals->m_pLocalList; plist && !plist->IsEmpty();
                     plist = plist->m_pkNext) {
                    pse->pEffectLocals->SetVariable(plist->m_item->uiID, plist->m_item->fValue);
                }
                if (pEffectLocals->m_pScriptEffectData) {
                    pse->pEffectLocals->m_pScriptEffectData =
                        static_cast<SCRIPT_EFFECT_DATA*>(ourMalloc(sizeof(SCRIPT_EFFECT_DATA)));
                }
            }
        }
    }
}

// Gives the effect its own copy of the script's locals and runs the script's effect-start block on the target.
void ScriptEffect::Start() {
    if (pScript) {
        pEffectLocals = pScript->CloneLocals();
        MagicTarget* pMagicTarget = GetTarget();
        ScriptLocals* pLocals = pEffectLocals;
        pScript->RunScriptEffectStart(pMagicTarget->GetTargetStatsObject(), pLocals);
    }
}

void ScriptEffect::Finish() {
    if (pScript) {
        MagicTarget* Target = pTarget;
        ScriptLocals* pEffectLocals = this->pEffectLocals;
        TESObjectREFR* v1 = Target->GetTargetStatsObject();
        pScript->RunScriptEffectFinish(v1, pEffectLocals);
    }
    if (this->pEffectLocals) {
        ScriptLocals* apThis = this->pEffectLocals;
        if (apThis) {
            delete apThis;
        }
        this->pEffectLocals = 0;
    }
}

ActiveEffect* ScriptEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* v5 = ourMalloc(sizeof(ScriptEffect));
    if (v5) {
        return ::new (v5) ScriptEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}
