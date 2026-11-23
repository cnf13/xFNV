#include "Fallout/Magic/SunDamageEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"

// Whether the sun-damage archetype took Create. Stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::SUN_DAMAGE,
    &SunDamageEffect::Create
);

SunDamageEffect::SunDamageEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ActiveEffect(apCaster, apItem, apEffect), fGlowEffect(0.0f), bHitEffectApplied(false),
    bDoNotTakeDamageNextUpdate(false) {}

SunDamageEffect::~SunDamageEffect() {}

ActiveEffect* SunDamageEffect::Copy() {
    SunDamageEffect* pCopy;
    pCopy = new SunDamageEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    pCopy->fGlowEffect = fGlowEffect;
    pCopy->bHitEffectApplied = bHitEffectApplied;
    pCopy->bDoNotTakeDamageNextUpdate = bDoNotTakeDamageNextUpdate;
    return pCopy;
}

void SunDamageEffect::Update(float afSecondsElapsed) {
    if (bDoNotTakeDamageNextUpdate) {
        bDoNotTakeDamageNextUpdate = false;
    }
    else if (GetTarget()) {
        MagicTarget* pTarget = GetTarget();
        if (pTarget->GetTargetStatsObject()) {
            pTarget = GetTarget();
            Actor* pActor = static_cast<Actor*>(pTarget->GetTargetStatsObject());
            TESObjectCELL* pParentCell = pActor->GetParentCell();
            if (!pActor->GetInterior() || (pParentCell && pParentCell->GetFakeExterior())) {
                float fDamage = GetDamageAmount();
                UpdateHitEffect(fDamage > 0.009999999776482582);
                if (pActor == pPlayer) {
                    if (fDamage > 0.009999999776482582) {
                        pPlayer->DoDamage(fDamage * afSecondsElapsed, 0.0, nullptr);
                        if (!(SunDamageEffect::fPlayerPainCountdown < 0.0)) {
                            if (SunDamageEffect::fPlayerPainCountdown == 3.4028234663852886e+38) {
                                SunDamageEffect::fPlayerPainCountdown = fMagicSunDamagePainInitialDelay.Float();
                            }
                        }
                        else {
                            pPlayer->TriggerPain(true, true);
                            SunDamageEffect::fPlayerPainCountdown = fMagicSunDamagePainTimer.Float();
                        }
                        SunDamageEffect::fPlayerPainCountdown = SunDamageEffect::fPlayerPainCountdown -
                            afSecondsElapsed;
                    }
                    Sky* pSky = TES::pInstance->GetSky();
                    if (pSky->GetCurrentWeather()) {
                        pSky->GetCurrentWeather()->GetWeatherData(TESWeather::WD_SUN_DAMAGE, 1.0, 0.0);
                    }
                    if (pSky->GetLastWeather()) {
                        pSky->GetWeatherPercent();
                        pSky->GetLastWeather()->GetWeatherData(TESWeather::WD_SUN_DAMAGE, 1.0, 0.0);
                    }
                }
            }
            else {
                UpdateHitEffect(false);
                if (pActor == pPlayer) {
                    SunDamageEffect::fPlayerPainCountdown = fMagicSunDamagePainInitialDelay.Float();
                }
            }
        }
    }
}

// Save version 0x71 added 4 bytes.
uint16_t SunDamageEffect::GetSaveSize(Actor* apActor) {
    uint16_t usSize = ActiveEffect::GetSaveSize(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x71) {
        usSize += 4;
    }
    return usSize;
}

void SunDamageEffect::SaveGame(Actor* apActor) {
    ActiveEffect::SaveGame(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x71) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fGlowEffect, sizeof(fGlowEffect));
    }
}

void SunDamageEffect::LoadGame(Actor* apActor) {
    ActiveEffect::LoadGame(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x71) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fGlowEffect, sizeof(fGlowEffect));
    }
}

bool SunDamageEffect::IsCausingDamage() {
    MagicTarget* pPlayerTarget = static_cast<MagicTarget*>(pPlayer);
    return ActiveEffect::GetTarget() == pPlayerTarget && GetDamageAmount() > 0.009999999776482582;
}

void SunDamageEffect::CopyData(ActiveEffect* apDestination) {
    ActiveEffect::CopyData(apDestination);
}

// Only characters take the sun's damage.
bool SunDamageEffect::CheckTarget(MagicTarget* apTarget) {
    return dynamic_cast<Character*>(apTarget) != nullptr;
}

float SunDamageEffect::GetDamageAmount() {
    void* pCurrentWeather;
    void* pLastWeather;
    double dWaterLevel;
    float fWeatherData;
    TESObjectCELL* pParentCell;
    Actor* pActor;
    MagicTarget* pTarget;
    float fWeatherPercent;
    float fCurrentPart;
    if (ActiveEffect::GetTarget()) {
        pTarget = ActiveEffect::GetTarget();
        pActor = static_cast<Actor*>(pTarget->GetTargetStatsObject());
    }
    else {
        pActor = nullptr;
    }
    if (pActor) {
        pParentCell = pActor->TESObjectREFR::GetParentCell();
    }
    else {
        pParentCell = nullptr;
    }
    if (!pActor || (pActor->GetInterior() && (!pParentCell || !pParentCell->GetFakeExterior()))) {
        return 0.0;
    }
    Sky* pSky = TES::pInstance->TES::GetSky();
    float fGameHour = pSky->GetCurrentGameHour();
    float fSunriseBegin = pSky->GetSunriseBegin();
    float fSunsetEnd = pSky->GetSunsetEnd();
    if (pSky->GetCurrentWeather()) {
        pCurrentWeather = pSky->GetCurrentWeather();
        fWeatherData = static_cast<TESWeather*>(pCurrentWeather)->GetWeatherData(TESWeather::WD_SUN_DAMAGE, 1.0, 0.0);
    }
    else {
        fWeatherData = 0.0f;
    }
    float fSunDamage = fWeatherData;
    if (pSky->GetLastWeather()) {
        fWeatherPercent = pSky->GetWeatherPercent();
        fCurrentPart = fWeatherData * fWeatherPercent;
        pLastWeather = pSky->GetLastWeather();
        fSunDamage = static_cast<float>(
            static_cast<TESWeather*>(pLastWeather)->GetWeatherData(TESWeather::WD_SUN_DAMAGE, 1.0, 0.0) *
                (1.0 - fWeatherPercent) +
            fCurrentPart
        );
    }
    uint8_t ucUnderwater = 0;
    pParentCell = pActor->TESObjectREFR::GetParentCell();
    if (pParentCell->HasWater()) {
        dWaterLevel = pActor->GetRelevantWaterHeight();
        ucUnderwater = pActor->GetLocationOnReference().z <= dWaterLevel;
    }
    float fMagnitude = this->fMagnitude;
    return MagicFormulas::CalcSunDamage(
        fMagnitude,
        fGameHour,
        fSunriseBegin,
        fSunsetEnd,
        true,
        fSunDamage,
        ucUnderwater
    );
}

void SunDamageEffect::UpdateHitEffect(bool abDamaging) {
    TESEffectShader* pEffectData = GetEffectSetting()->data.pEffectShader;
    if (pEffectData && pTarget && eSpellType == MagicSystem::ABILITY) {
        if (abDamaging) {
            if (!this->pHitEffects || this->pHitEffects->IsEmpty()) {
                MagicShaderHitEffect* pHitEffect;
                if (this->pHitEffects) {
                    this->pHitEffects->RemoveAll();
                    BSSimpleList<MagicHitEffect*>* pHitEffects = this->pHitEffects;
                    if (pHitEffects) {
                        delete pHitEffects;
                    }
                    this->pHitEffects = nullptr;
                }
                pHitEffect = new MagicShaderHitEffect(pTarget->GetTargetStatsObject(), pEffectData, -1.0f);
                if (pHitEffect->Init()) {
                    ReferenceProcessLists.AddTempEffect(pHitEffect);
                    BSSimpleList<MagicHitEffect*>* pList = new BSSimpleList<MagicHitEffect*>(pHitEffect);
                    this->pHitEffects = pList;
                    pHitEffect->pEffect = this;
                }
                else {
                    if (pHitEffect) {
                        delete pHitEffect;
                    }
                    pHitEffect = nullptr;
                }
            }
        }
        else if (this->pHitEffects) {
            for (BSSimpleList<MagicHitEffect*>* pList = this->pHitEffects; pList && !pList->IsEmpty();
                 pList = pList->m_pkNext) {
                MagicHitEffect* pHitEffect = pList->m_item;
                pHitEffect->bFinished = true;
                pHitEffect->pEffect = nullptr;
            }
            this->pHitEffects->RemoveAll();
            if (this->pHitEffects) {
                delete this->pHitEffects;
            }
            this->pHitEffects = nullptr;
            ReferenceProcessLists.FinishMagicShaderHitEffect(pTarget->GetTargetStatsObject(), pEffectData);
        }
    }
}

ActiveEffect* SunDamageEffect::Create(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect) {
    return new SunDamageEffect(apCaster, apItem, apEffect);
}
