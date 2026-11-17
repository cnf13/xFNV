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
static char bRegistered =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::SUN_DAMAGE, &SunDamageEffect::Create);

SunDamageEffect::SunDamageEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ActiveEffect(apCaster, apItem, apEffect), fGlowEffect(0.0f), bHitEffectApplied(false),
    bDoNotTakeDamageNextUpdate(false) {}

SunDamageEffect::~SunDamageEffect() {}

ActiveEffect* SunDamageEffect::Copy() {
    SunDamageEffect* v2;
    void* v4 = ourMalloc(sizeof(SunDamageEffect));
    if (v4) {
        v2 = ::new (v4) SunDamageEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    CopyData(v2);
    v2->fGlowEffect = fGlowEffect;
    v2->bHitEffectApplied = (bool)((uint8_t)bHitEffectApplied);
    v2->bDoNotTakeDamageNextUpdate = (bool)((uint8_t)bDoNotTakeDamageNextUpdate);
    return v2;
}

void SunDamageEffect::Update(float afSecondsElapsed) {
    if (bDoNotTakeDamageNextUpdate) {
        bDoNotTakeDamageNextUpdate = 0;
    }
    else {
        if (GetTarget()) {
            MagicTarget* pTarget = GetTarget();
            if (pTarget->GetTargetStatsObject()) {
                pTarget = GetTarget();
                Actor* pActor = static_cast<Actor*>(pTarget->GetTargetStatsObject());
                TESObjectCELL* ParentCell = pActor->GetParentCell();
                if (!pActor->GetInterior() || (ParentCell && ParentCell->GetFakeExterior())) {
                    float fDamage = GetDamageAmount();
                    UpdateHitEffect(fDamage > 0.009999999776482582);
                    if (pActor == pPlayer) {
                        if (fDamage > 0.009999999776482582) {
                            float v4 = fDamage * afSecondsElapsed;
                            pPlayer->DoDamage(v4, 0.0, 0);
                            if (!(SunDamageEffect::fPlayerPainCountdown < 0.0)) {
                                if (SunDamageEffect::fPlayerPainCountdown == 3.4028234663852886e+38) {
                                    SunDamageEffect::fPlayerPainCountdown = fMagicSunDamagePainInitialDelay.Float();
                                }
                            }
                            else {
                                pPlayer->TriggerPain(1, 1);
                                SunDamageEffect::fPlayerPainCountdown = fMagicSunDamagePainTimer.Float();
                            }
                            SunDamageEffect::fPlayerPainCountdown =
                                SunDamageEffect::fPlayerPainCountdown - afSecondsElapsed;
                        }
                        Sky* CurrentAIProcess = TES::pInstance->GetSky();
                        if (CurrentAIProcess->GetCurrentWeather()) {
                            TESWeather* Thread = CurrentAIProcess->GetCurrentWeather();
                            Thread->GetWeatherData(TESWeather::WD_SUN_DAMAGE, 1.0, 0.0);
                        }
                        if (CurrentAIProcess->GetLastWeather()) {
                            CurrentAIProcess->GetWeatherPercent();
                            TESWeather* Time = CurrentAIProcess->GetLastWeather();
                            Time->GetWeatherData(TESWeather::WD_SUN_DAMAGE, 1.0, 0.0);
                        }
                    }
                }
                else {
                    UpdateHitEffect(0);
                    if (pActor == pPlayer) {
                        SunDamageEffect::fPlayerPainCountdown = fMagicSunDamagePainInitialDelay.Float();
                    }
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
        pSaveLoadGameOLD->SaveGameDataOLD(&fGlowEffect, 4);
    }
}

void SunDamageEffect::LoadGame(Actor* apActor) {
    ActiveEffect::LoadGame(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x71) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fGlowEffect, 4);
    }
}

bool SunDamageEffect::IsCausingDamage() {
    MagicTarget* v2;
    if (pPlayer) {
        v2 = static_cast<MagicTarget*>(pPlayer);
    }
    else {
        v2 = nullptr;
    }
    return (bool)((uint8_t)((ActiveEffect::GetTarget() == v2) && (GetDamageAmount() > 0.009999999776482582)));
}

void SunDamageEffect::CopyData(ActiveEffect* apDestination) {
    ActiveEffect::CopyData(apDestination);
}

// Only characters take the sun's damage.
bool SunDamageEffect::CheckTarget(MagicTarget* apTarget) {
    return dynamic_cast<Character*>(apTarget) != nullptr;
}

float SunDamageEffect::GetDamageAmount() {
    void* mainThreadID;
    void* v2;
    double waterLevel;
    float weatherData;
    TESObjectCELL* parentCell;
    Actor* pActor;
    MagicTarget* pTarget;
    float weatherPercent;
    float v14;
    if (ActiveEffect::GetTarget()) {
        pTarget = ActiveEffect::GetTarget();
        pActor = static_cast<Actor*>(pTarget->GetTargetStatsObject());
    }
    else {
        pActor = nullptr;
    }
    if (pActor) {
        parentCell = pActor->TESObjectREFR::GetParentCell();
    }
    else {
        parentCell = nullptr;
    }
    if ((!pActor) || (pActor->GetInterior() && ((!parentCell) || (!parentCell->GetFakeExterior())))) {
        return 0.0;
    }
    Sky* currentProcess = TES::pInstance->TES::GetSky();
    float a2 = (float)currentProcess->GetCurrentGameHour();
    float a3 = (float)currentProcess->GetSunriseBegin();
    float a4 = (float)currentProcess->GetSunsetEnd();
    if (currentProcess->GetCurrentWeather()) {
        mainThreadID = currentProcess->GetCurrentWeather();
        weatherData = static_cast<TESWeather*>(mainThreadID)->GetWeatherData(TESWeather::WD_SUN_DAMAGE, 1.0, 0.0);
    }
    else {
        weatherData = (float)0.0;
    }
    float v13 = weatherData;
    if (currentProcess->GetLastWeather()) {
        weatherPercent = (float)currentProcess->GetWeatherPercent();
        v14 = weatherData * weatherPercent;
        v2 = currentProcess->GetLastWeather();
        v13 = (float)((static_cast<TESWeather*>(v2)->GetWeatherData(TESWeather::WD_SUN_DAMAGE, 1.0, 0.0) *
                       (1.0 - weatherPercent)) +
                      v14);
    }
    uint8_t v18 = 0;
    TESObjectCELL* v3 = pActor->TESObjectREFR::GetParentCell();
    if (v3->HasWater()) {
        waterLevel = (double)pActor->GetRelevantWaterHeight();
        v18 = (uint8_t)(pActor->GetLocationOnReference().z <= waterLevel);
    }
    float fMagnitude = this->fMagnitude;
    return MagicFormulas::CalcSunDamage(fMagnitude, a2, a3, a4, 1, v13, v18);
}

void SunDamageEffect::UpdateHitEffect(bool abDamaging) {
    EffectSetting* EffectSetting_ = GetEffectSetting();
    TESEffectShader* pEffectData = EffectSetting_->data.pEffectShader;
    if (pEffectData) {
        if (pTarget) {
            if (eSpellType == MagicSystem::ABILITY) {
                if (abDamaging) {
                    if (!this->pHitEffects || this->pHitEffects->IsEmpty()) {
                        MagicShaderHitEffect* peffect;
                        if (this->pHitEffects) {
                            this->pHitEffects->RemoveAll();
                            BSSimpleList<MagicHitEffect*>* pHitEffects = this->pHitEffects;
                            if (pHitEffects) {
                                delete pHitEffects;
                            }
                            this->pHitEffects = 0;
                        }
                        void* apMemory = NiMemObject::operator new(sizeof(MagicShaderHitEffect));
                        if (apMemory) {
                            MagicTarget* Target = pTarget;
                            TESObjectREFR* v3 = Target->GetTargetStatsObject();
                            peffect = ::new (apMemory) MagicShaderHitEffect(v3, pEffectData, -1.0f);
                        }
                        else {
                            peffect = 0;
                        }
                        MagicShaderHitEffect* peffect_2 = peffect;
                        if (peffect->Init()) {
                            BSSimpleList<MagicHitEffect*>* v7;
                            ReferenceProcessLists.AddTempEffect(peffect_2);
                            void* v12 = ourMalloc(sizeof(BSSimpleList<MagicHitEffect*>));
                            if (v12) {
                                MagicHitEffect* item = peffect_2;
                                v7 = ::new (v12) BSSimpleList<MagicHitEffect*>(item);
                            }
                            else {
                                v7 = 0;
                            }
                            this->pHitEffects = v7;
                            peffect_2->pEffect = this;
                        }
                        else {
                            if (peffect_2) {
                                delete peffect_2;
                            }
                            peffect_2 = 0;
                        }
                    }
                }
                else {
                    if (this->pHitEffects) {
                        for (BSSimpleList<MagicHitEffect*>* plist = this->pHitEffects; plist && !plist->IsEmpty();
                             plist = plist->m_pkNext) {
                            MagicHitEffect* peffect_3 = plist->m_item;
                            peffect_3->bFinished = 1;
                            peffect_3->pEffect = 0;
                        }
                        this->pHitEffects->RemoveAll();
                        if (this->pHitEffects) {
                            delete this->pHitEffects;
                        }
                        this->pHitEffects = 0;
                        MagicTarget* v6 = pTarget;
                        TESObjectREFR* v5 = v6->GetTargetStatsObject();
                        ReferenceProcessLists.FinishMagicShaderHitEffect(v5, pEffectData);
                    }
                }
            }
        }
    }
}

ActiveEffect* SunDamageEffect::Create(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
    void* v5 = ourMalloc(sizeof(SunDamageEffect));
    if (v5) {
        return ::new (v5) SunDamageEffect(a1, a2, a3);
    }
    else {
        return nullptr;
    }
}
