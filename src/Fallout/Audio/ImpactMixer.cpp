#include "Fallout/Audio/ImpactMixer.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioListener.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterListener.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/CreatureSounds.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESLandTexture.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/WeaponNearFarSound.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"

NiPoint3 ImpactMixer::lastloc;
SettingT<INISettingCollection> ImpactMixer::iCollisionSoundTimeDelta("iCollisionSoundTimeDelta:Audio", 333);
SettingT<INISettingCollection> gs_fWoodMediumMassMin_Audio("fWoodMediumMassMin:Audio", 7.0f);
SettingT<INISettingCollection> gs_fWoodLargeMassMin_Audio("fWoodLargeMassMin:Audio", 15.0f);
SettingT<INISettingCollection> gs_fStoneMediumMassMin_Audio("fStoneMediumMassMin:Audio", 5.0f);
SettingT<INISettingCollection> gs_fStoneLargeMassMin_Audio("fStoneLargeMassMin:Audio", 30.0f);
SettingT<INISettingCollection> gs_fEarthMediumMassMin_Audio("fEarthMediumMassMin:Audio", 5.0f);
SettingT<INISettingCollection> gs_fEarthLargeMassMin_Audio("fEarthLargeMassMin:Audio", 30.0f);
SettingT<INISettingCollection> gs_fSkinMediumMassMin_Audio("fSkinMediumMassMin:Audio", 5.0f);
SettingT<INISettingCollection> gs_fSkinLargeMassMin_Audio("fSkinLargeMassMin:Audio", 30.0f);
SettingT<INISettingCollection> ImpactMixer::fChainMediumMass("fChainMediumMassMin:Audio", 5.0f);
SettingT<INISettingCollection> ImpactMixer::fChainLargeMass("fChainLargeMassMin:Audio", 30.0f);
SettingT<INISettingCollection> gs_fMetalMediumMassMin_Audio("fMetalMediumMassMin:Audio", 8.0f);
SettingT<INISettingCollection> gs_fMetalLargeMassMin_Audio("fMetalLargeMassMin:Audio", 25.0f);
SettingT<INISettingCollection> ImpactMixer::fGlassMediumMass("fGlassMediumMassMin:Audio", 8.0f);
SettingT<INISettingCollection> ImpactMixer::fGlassLargeMass("fGlassLargeMassMin:Audio", 25.0f);
SettingT<INISettingCollection> ImpactMixer::fMedWpnWeight("fMediumWeaponWeightMin:Audio", 8.0f);
SettingT<INISettingCollection> ImpactMixer::fLargeWpnWeight("fLargeWeaponWeightMin:Audio", 25.0f);
SettingT<INISettingCollection> ImpactMixer::fMedWpnSpeed("fMediumWeaponSpeedMax:Audio", 1.1f);
SettingT<INISettingCollection> ImpactMixer::fLargeWpnSpeed("fLargeWeaponSpeedMax:Audio", 0.95f);
SettingT<INISettingCollection> gs_fCollisionSoundHeavyThreshold_Audio("fCollisionSoundHeavyThreshold:Audio", 100.0f);
SettingT<INISettingCollection> gs_fPlayerFootVolume_Audio("fPlayerFootVolume:Audio", 0.9f);
SettingT<INISettingCollection> gs_fMaxFootstepDistance_Audio("fMaxFootstepDistance:Audio", 1100.0f);
SettingT<INISettingCollection> ImpactMixer::iMaxImpactSoundCount("iMaxImpactSoundCount:Audio", 32);
ImpactMixer::ImpactTimeMap ImpactMixer::pImpactMap;
ImpactMixer::GunshotHandleArray ImpactMixer::GunshotHandles;

// Location is built by NiPoint3's default constructor; the rest is deliberately left unset, as the game does.
ImpactMixer::COLLISION_SOUND::COLLISION_SOUND() {}

void ImpactMixer::PlayJumpSound(TESObjectREFR* apRef, int32_t aiFootType) {
    AutoMemContext kContext(
        MEM_CONTEXT_AUDIO, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Audio\\ImpactMixer.cpp", 941
    );
    if (Interface::IsInMenuMode()) {
        return;
    }
    NiPoint3 kRefPos = apRef->GetLocationOnReference();
    BSSoundHandle kSound;
    BSSoundHandle th_s;
    TESSound* a5 = nullptr;
    TESSound* pArmorSound = nullptr;
    char bUse2DSound = 0;
    float fMaxFootstepDistance = gs_fMaxFootstepDistance_Audio.Float();
    if (Main::GetSceneIsComplex()) {
        fMaxFootstepDistance = fMaxFootstepDistance / 2.0;
    }
    float fListenerDistance = 0.0f;
    if ((&BSAudio::QInstance())->pListener) {
        BSAudioListener* pListener = (&BSAudio::QInstance())->pListener;
        NiPoint3 kListenerPos = pListener->GetPosition();
        kListenerPos -= kRefPos;
        fListenerDistance = kListenerPos.Length();
        double v2 = kListenerPos.Length();
        if (fMaxFootstepDistance < v2) {
            goto LABEL_7;
        }
    }
    if (apRef->IsActor()) {
        Actor* pActor;
        pActor = dynamic_cast<Actor*>(apRef);
        if (pActor->IsDead(false)) {
        LABEL_7:;
            return;
        }
        if (pActor->IsCreature()) {
            TESBoundObject* pBaseForm = apRef->GetObjectReference();
            TESCreature* pCreature = dynamic_cast<TESCreature*>(pBaseForm);
            TESSound* pAnimSound = pCreature->PickCreatureSound(CREA_SOUND_JUMP);
            a5 = pAnimSound;
            if (pAnimSound) {
                uint8_t MaxDistance = a5->GetMaxDistance();
                if (fListenerDistance <= static_cast<double>(100 * MaxDistance)) {
                    uint32_t pSoundName_8 = a5->iFormID;
                    BSAudio* v4 = &BSAudio::QInstance();
                    kSound = v4->GetSoundHandleByNumericID(
                        pSoundName_8,
                        BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
                            BSGameSound::SOUND_FLAG_ONE_SHOT
                    );
                }
            }
            kSound.SetPosition(kRefPos.x, kRefPos.y, kRefPos.z);
            if (pCreature->Get3D()) {
                NiAVObject* p3D = pCreature->Get3D();
                kSound.SetObjectToFollow(p3D);
            }
            kSound.SetVolume(0.98f);
            kSound.Play(false);
        }
        else {
            HavokMaterialType eMaterialType = BHK_MATERIAL_STONE;
            TESObjectCELL* pInterior = TES::pInstance->pInteriorCell;
            if (!pInterior) {
                TESWorldSpace* pSoundName_8a = TES::pInstance->pWorldSpace;
                pInterior = pDataHandler->GetCellFromWorldCoord(kRefPos.x, kRefPos.y, pSoundName_8a, false);
            }
            char bUnderwater = 0;
            // The stairs flag is set as PlayFootstep does, but is never read here, as the game does.
            [[maybe_unused]] bool bStairs = false;
            if ((!pActor->IsSwimming()) && (pInterior)) {
                double RelevantWaterHeight = pActor->GetRelevantWaterHeight();
                if (kRefPos.z < RelevantWaterHeight &&
                    ((pInterior->GetInterior() && pInterior->HasWater()) || !pInterior->GetInterior())) {
                    bUnderwater = 1;
                }
            }

            MobileObject* v59 = (MobileObject*)pActor;
            bhkCharacterController* CharController = pActor->GetCharController();
            if (static_cast<uint8_t>(CharController->GetOnStairs())) {
                bStairs = true;
            }
            if (!TES::pInstance->pInteriorCell && (pInterior && pInterior->GetLand())) {
                COORD_DATA coordData;
                TESObjectLAND* Land = pInterior->GetLand();
                Land->GetCoordData(&coordData, kRefPos, true);
                uint16_t iCoordVertex = coordData.iVertidx;
                uint8_t pSoundName_8b = coordData.iBlock;
                TESObjectLAND* v9 = pInterior->GetLand();
                TESLandTexture* MainTexture = v9->GetMainTexture(pSoundName_8b, iCoordVertex);
                float arHeight = 0.0f;
                eMaterialType = BHK_MATERIAL_STONE;
                if (bUnderwater) {
                    eMaterialType = BHK_MATERIAL_WATER;
                }
                else {
                    if (v59 &&
                        (v59->GetCharController() && v59->GetCharController()->eSupportMaterial != BHK_MATERIAL_MAX)) {
                        eMaterialType = v59->GetCharController()->eSupportMaterial;
                    }
                    else {
                        if (MainTexture) {
                            eMaterialType = MainTexture->GetHavokMaterialType();
                            if (pInterior->GetLandHeight(kRefPos, arHeight)) {
                                kRefPos.z = arHeight;
                            }
                        }
                    }
                }
            }
            else {
                if (bUnderwater) {
                    eMaterialType = BHK_MATERIAL_WATER;
                }
                else {
                    if ((v59) && (v59->GetCharController())) {
                        eMaterialType = v59->GetCharController()->eSupportMaterial;
                    }
                }
            }
            int32_t v62 = 9;
            int32_t FootstepIndexForType = 0;
            FootstepIndexForType = ImpactMixer::GetFootstepIndexForType(eMaterialType);
            a5 = ImpactMixer::pFootSteps[FootstepIndexForType][v62];
            if (pActor->IsPowerBodyArmorWorn()) {
                if (!ImpactMixer::pJumpPower) {
                    ImpactMixer::pJumpPower = pDataHandler->GetSound("FSTJumpPower");
                }
                // Loads the power armour sound but plays the heavy armour one, deliberately, as the game does.
                pArmorSound = ImpactMixer::pJumpHeavy;
            }
            else {
                if (pActor->IsHeavyBodyArmorWorn()) {
                    if (!ImpactMixer::pJumpHeavy) {
                        ImpactMixer::pJumpHeavy = pDataHandler->GetSound("FSTJumpArmor");
                    }
                    pArmorSound = ImpactMixer::pJumpHeavy;
                }
            }
            if (apRef == pPlayer && !pPlayer->IsActually3rdPerson()) {
                bUse2DSound = 1;
            }
            if (bUse2DSound) {
                uint32_t eAudioFlags = BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT;
                if (a5) {
                    const char* pSoundName_4 = a5->GetFilename();
                    BSAudio* v10 = &BSAudio::QInstance();
                    kSound = v10->GetSoundHandleByFilename(pSoundName_4, eAudioFlags, a5);
                    float* v11 = &gs_fPlayerFootVolume_Audio.Float();
                    kSound.SetVolume(*v11);
                    kSound.Play(false);
                }
                if (pArmorSound) {
                    const char* pSoundName_4a = pArmorSound->GetFilename();
                    BSAudio* v12 = &BSAudio::QInstance();
                    th_s = v12->GetSoundHandleByFilename(pSoundName_4a, eAudioFlags, pArmorSound);
                    float* v13 = &gs_fPlayerFootVolume_Audio.Float();
                    th_s.SetVolume(*v13);
                    th_s.Play(false);
                }
            }
            else {
                uint32_t aeAudioFlags = BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT;
                if (!FalloutAudio::SoundDistanceTest(a5, kRefPos, 1.0f) &&
                    !FalloutAudio::SoundDistanceTest(pArmorSound, kRefPos, 1.0f)) {
                    return;
                }
                if (a5) {
                    uint8_t v14 = a5->GetMaxDistance();
                    if (fListenerDistance <= static_cast<double>(100 * v14)) {
                        const char* pSoundName_4b = a5->GetFilename();
                        BSAudio* v15 = &BSAudio::QInstance();
                        kSound = v15->GetSoundHandleByFilename(pSoundName_4b, aeAudioFlags, a5);
                        float v40 = kRefPos.z - 128.0;
                        kSound.SetPosition(kRefPos.x, kRefPos.y, v40);
                        if (apRef->Get3D()) {
                            NiNode* v16 = apRef->Get3D();
                            kSound.SetObjectToFollow(v16);
                        }
                        float iCoordVertexd = RandomFloat(0.9f, 1.0f);
                        kSound.SetVolume(iCoordVertexd);
                        kSound.Play(false);
                    }
                }
                if (pArmorSound) {
                    uint8_t v17 = pArmorSound->GetMaxDistance();
                    if (fListenerDistance <= static_cast<double>(100 * v17)) {
                        const char* pSoundName_4c = pArmorSound->GetFilename();
                        BSAudio* v18 = &BSAudio::QInstance();
                        th_s = v18->GetSoundHandleByFilename(pSoundName_4c, aeAudioFlags, pArmorSound);
                        float v38 = kRefPos.z - 128.0;
                        th_s.SetPosition(kRefPos.x, kRefPos.y, v38);
                        if (apRef->Get3D()) {
                            NiNode* v19 = apRef->Get3D();
                            th_s.SetObjectToFollow(v19);
                        }
                        float v37 = 1.0 - RandomFloat(0.0f, 0.1f);
                        th_s.SetVolume(v37);
                        th_s.Play(false);
                    }
                }
            }
        }
    }
}

void ImpactMixer::PlayGunshot(
    TESObjectWEAP* apWeapon,
    TESObjectREFR* apSource,
    NiAVObject* aOriginNode,
    bool abPlayer
) {
    if (ImpactMixer::GetGunshotsInitialized()) {
        BSSoundHandle handl;
        BSSoundHandle handl2;
        NiPoint3 origin = NiPoint3_ZERO;
        bool b2DAttackSound = false;
        uint32_t iflags = 0;
        bool bin1stPerson = !pPlayer->IsActually3rdPerson();
        bool bUseModSound = false;
        float sqrDist = 3.4028235e+38f;
        WeaponNearFarSound* pcurrImpactHandle = nullptr;
        WeaponNearFarSound* poldest = nullptr;
        WeaponNearFarSound* pfurthest = nullptr;
        WeaponNearFarSound* psameSource = nullptr;
        bool boldestIsValid = true;
        float fModNV = 1.0f;
        float fModNA = 1.0f;
        float fModFV = 1.0f;
        float fModFA = 1.0f;
        uint8_t ucMinN = 0;
        uint8_t ucMinF = 0;
        uint8_t ucMaxN = 0;
        uint8_t ucMaxF = 0;
        if ((pPlayer->GetParentCell()) &&
            (pPlayer->GetParentCell()->GetInterior() && !pPlayer->GetParentCell()->GetFakeExterior())) {
            fModNV = gs_fWeaponInteriorNearVolumeMod.Float();
            fModFV = gs_fWeaponInteriorFarVolumeMod.Float();
            fModNA = gs_fWeaponInteriorNearAttenuationMod.Float();
            fModFA = gs_fWeaponInteriorFarAttenuationMod.Float();
        }

        if (abPlayer) {
            iflags |= BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE;
            if (pPlayer->pCurrentProcess) {
                BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
                if (CurrentAIProcess->GetCurrentWeapon()) {
                    BaseProcess* v22 = pPlayer->pCurrentProcess;
                    ItemChange* pweap = v22->GetCurrentWeapon();
                    if ((pweap) && (pweap->HasModEffectActive(11) || pweap->HasModEffectActive(16))) {
                        bUseModSound = true;
                    }
                }
            }
        }
        if (apWeapon) {
            if ((abPlayer) && (apWeapon->GetAttackSound2D(false))) {
                b2DAttackSound = true;
            }

            if (b2DAttackSound) {
                iflags = static_cast<BSGameSound::SoundFlags>(
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_BATTLE_SOUND |
                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IS_2D_GUNFIRE
                );
            }
            else {
                iflags = abPlayer && bin1stPerson
                    ? static_cast<BSGameSound::SoundFlags>(
                          BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_BATTLE_SOUND |
                          BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IS_2D_GUNFIRE |
                          BSGameSound::SOUND_FLAG_FIRST_PERSON
                      )
                    : static_cast<BSGameSound::SoundFlags>(
                          BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_BATTLE_SOUND |
                          BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IS_2D_GUNFIRE
                      );
            }
            if (abPlayer || !aOriginNode) {
                if (apSource->Get3D()) {
                    origin = apSource->Get3D()->GetWorldTranslate();
                }
            }
            else {
                origin = aOriginNode->GetWorldTranslate();
            }
            if (apWeapon->GetAttackSound(bUseModSound)) {
                TESSound* psound = nullptr;
                TESSound* psound2 = nullptr;
                if (b2DAttackSound) {
                    psound = apWeapon->GetAttackSound2D(bUseModSound);
                }
                else {
                    psound = apWeapon->GetAttackSound(bUseModSound);
                    psound2 = apWeapon->GetAttackSound2(bUseModSound);
                }
                bool bs1d = FalloutAudio::SoundDistanceTest(psound, origin, fModNA);
                bool bs2d = FalloutAudio::SoundDistanceTest(psound2, origin, fModFA);
                if (!bs1d && !bs2d) {
                    return;
                }
                if (!bs1d) {
                    psound = nullptr;
                }
                if (!bs2d) {
                    psound2 = nullptr;
                }
                BSAudio* v4 = &BSAudio::QInstance();
                NiPoint3 distanceVector = origin - v4->GetListenerPosition();
                float fdistanceSqr = distanceVector.x * distanceVector.x + distanceVector.y * distanceVector.y +
                    distanceVector.z * distanceVector.z;
                for (int32_t i = 0; i < 20; ++i) {
                    if (ImpactMixer::GunshotHandles[i].uiIdentifier == apSource) {
                        psameSource = &ImpactMixer::GunshotHandles[i];
                        if (!abPlayer) {
                            break;
                        }
                    }
                    if (poldest) {
                        if (ImpactMixer::GunshotHandles[i].uiTimestamp <= poldest->uiTimestamp) {
                            poldest = &ImpactMixer::GunshotHandles[i];
                        }
                    }
                    else {
                        poldest = &ImpactMixer::GunshotHandles[i];
                    }
                    if (abPlayer && ImpactMixer::GunshotHandles[i].uiIdentifier != pPlayer) {
                        if (pfurthest) {
                            if (ImpactMixer::GunshotHandles[i].fRelativeDistance >
                                    static_cast<double>(pfurthest->fRelativeDistance) &&
                                ImpactMixer::GunshotHandles[i].fRelativeDistance > static_cast<double>(fdistanceSqr)) {
                                pfurthest = &ImpactMixer::GunshotHandles[i];
                            }
                        }
                        else {
                            pfurthest = &ImpactMixer::GunshotHandles[i];
                        }
                    }
                }
                if (poldest) {
                    boldestIsValid = poldest->soundNear.IsValid();
                }
                if (!poldest || boldestIsValid) {
                    if (psameSource) {
                        pcurrImpactHandle = psameSource;
                        psameSource->uiTimestamp = BSGetTickCount();
                        psameSource->fRelativeDistance = fdistanceSqr;
                    }
                    else {
                        if (pfurthest) {
                            pcurrImpactHandle = pfurthest;
                            pfurthest->uiIdentifier = static_cast<Actor*>(apSource);
                            pfurthest->uiTimestamp = BSGetTickCount();
                            pfurthest->fRelativeDistance = fdistanceSqr;
                        }
                    }
                }
                else {
                    pcurrImpactHandle = poldest;
                    poldest->uiIdentifier = static_cast<Actor*>(apSource);
                    poldest->uiTimestamp = BSGetTickCount();
                    poldest->fRelativeDistance = fdistanceSqr;
                }
                if (pcurrImpactHandle) {
                    if (psound) {
                        pcurrImpactHandle->soundNear.SetVolume(0.0f);
                        pcurrImpactHandle->soundNear.Stop();
                        const char* arPos = psound->GetFilename();
                        BSAudio* v6 = &BSAudio::QInstance();
                        pcurrImpactHandle->soundNear = v6->GetSoundHandleByFilename(arPos, iflags, psound);
                        ucMinN = psound->GetMinDistance();
                        ucMaxN = psound->GetMaxDistance();
                    }
                    if (psound2) {
                        pcurrImpactHandle->soundFar.SetVolume(0.0f);
                        pcurrImpactHandle->soundFar.Stop();
                        const char* arPosa = psound2->GetFilename();
                        BSAudio* v7 = &BSAudio::QInstance();
                        pcurrImpactHandle->soundFar = v7->GetSoundHandleByFilename(arPosa, iflags, psound2);
                        ucMinF = psound2->GetMinDistance();
                        ucMaxF = psound2->GetMaxDistance();
                    }
                    if (abPlayer || !aOriginNode) {
                        if ((apSource->Get3D()) && (!abPlayer || !bin1stPerson)) {
                            NiNode* v8 = apSource->Get3D();
                            pcurrImpactHandle->soundNear.SetObjectToFollow(v8);
                            NiNode* v9 = apSource->Get3D();
                            pcurrImpactHandle->soundFar.SetObjectToFollow(v9);
                        }
                    }
                    else {
                        pcurrImpactHandle->soundNear.SetObjectToFollow(aOriginNode);
                        pcurrImpactHandle->soundFar.SetObjectToFollow(aOriginNode);
                    }
                    if (psound) {
                        pcurrImpactHandle->soundNear.SetPriority(1u);
                        pcurrImpactHandle->soundNear.SetPosition(origin.x, origin.y, origin.z);
                        pcurrImpactHandle->soundNear.SetVolume(fModNV);
                        pcurrImpactHandle->soundNear.Play(false);
                    }
                    if (psound2) {
                        pcurrImpactHandle->soundFar.SetPriority(1u);
                        pcurrImpactHandle->soundFar.SetPosition(origin.x, origin.y, origin.z);
                        pcurrImpactHandle->soundFar.SetVolume(fModFV);
                        pcurrImpactHandle->soundFar.Play(false);
                    }
                    if (!b2DAttackSound) {
                        float v19 = static_cast<double>(ucMaxN) * fModNA * 100.0;
                        float v18 = static_cast<double>(ucMinN) * 5.0;
                        pcurrImpactHandle->soundNear.SetMinMax(v18, v19);
                        float v17 = static_cast<double>(ucMaxN) * fModNA * 100.0;
                        float v16 = static_cast<double>(ucMinN) * 5.0;
                        pcurrImpactHandle->soundFar.SetMinMax(v16, v17);
                        pcurrImpactHandle->soundNear.SetPosition(origin.x, origin.y, origin.z);
                        pcurrImpactHandle->soundFar.SetPosition(origin.x, origin.y, origin.z);
                    }
                }
            }
        }
    }
}

const char* ImpactMixer::GetMaterialSoundName(int32_t aiMat) {
    if (ImpactMixer::pAltMaterialSoundNames[aiMat]) {
        return ImpactMixer::pAltMaterialSoundNames[aiMat];
    }

    return ImpactMixer::pMaterialSoundNames[aiMat];
}

void ImpactMixer::InitFootsteps() {
    if (!ImpactMixer::bFootStepInitialized) {
        ImpactMixer::bFootStepInitialized = true;
        for (int32_t iMat = 0; iMat < 13; ++iMat) {
            for (int32_t i = 0; i < 10; ++i) {
                char apDest[268];
                BSsprintf(
                    apDest, 259u, "FST%s%s", ImpactMixer::GetMaterialSoundName(iMat), ImpactMixer::pStepSoundNames[i]
                );
                ImpactMixer::pFootSteps[iMat][i] = pDataHandler->GetSound(apDest);
                if (!ImpactMixer::pFootSteps[iMat][i] && ImpactMixer::uiReplacementMaterials[iMat] != 13) {
                    BSsprintf(
                        apDest, 259u, "FST%s%s",
                        ImpactMixer::GetMaterialSoundName(ImpactMixer::uiReplacementMaterials[iMat]),
                        ImpactMixer::pStepSoundNames[i]
                    );
                    ImpactMixer::pFootSteps[iMat][i] = pDataHandler->GetSound(apDest);
                    if (!ImpactMixer::pFootSteps[iMat][i] &&
                        ImpactMixer::uiReplacementMaterials[ImpactMixer::uiReplacementMaterials[iMat]] != 13) {
                        BSsprintf(
                            apDest, 259u, "FST%s%s",
                            ImpactMixer::GetMaterialSoundName(
                                ImpactMixer::uiReplacementMaterials[ImpactMixer::uiReplacementMaterials[iMat]]
                            ),
                            ImpactMixer::pStepSoundNames[i]
                        );
                        ImpactMixer::pFootSteps[iMat][i] = pDataHandler->GetSound(apDest);
                    }
                }
                if (!ImpactMixer::pFootSteps[iMat][i]) {
                    ImpactMixer::pFootSteps[iMat][i] = pDataHandler->GetSound("FSTConcSolidWalkR");
                }
            }
        }
    }
}

void ImpactMixer::InitGunshots() {
    if (!ImpactMixer::GetGunshotsInitialized()) {
        ImpactMixer::SetGunshotsInitialized(true);
        for (int32_t i = 0; i < 20; ++i) {
            ImpactMixer::GunshotHandles[i].uiIdentifier = nullptr;
            ImpactMixer::GunshotHandles[i].uiTimestamp = 0;
            ImpactMixer::GunshotHandles[i].fRelativeDistance = 3.4028235e+38f;
        }
    }
}

void ImpactMixer::ReInitFootsteps() {
    ImpactMixer::bFootStepInitialized = false;
    ImpactMixer::InitFootsteps();
}

void ImpactMixer::PlayHitSound(
    Actor* apAttacker,
    float afDamage,
    float afDefDamage,
    Actor* apTarget,
    int32_t aiWeaponType,
    int32_t aiArmorType,
    int32_t aiShieldType,
    bool abFakeHit,
    bool abEnchantedWeapon
) {
    // Builds and destroys the context in place with its constructor and destructor.
    alignas(AutoMemContext) uint8_t aContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (aContext) AutoMemContext(
        MEM_CONTEXT_AUDIO, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Audio\\ImpactMixer.cpp", 271
    );
    pContext->~AutoMemContext();
}

void ImpactMixer::PlayCollisionSound(ImpactMixer::COLLISION_SOUND& arDataIn) {
    CFilter v19;
    CFilter apResult;
    uint32_t val;
    uint32_t uiSinceLastPlayed;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_AUDIO, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Audio\\ImpactMixer.cpp", 564
    );
    if (arDataIn.pBody1 == arDataIn.pBody2 && arDataIn.pBody1) {}
    else {
        CFilter* CollisionFilter;
        CFilter* v2;
        float fmass2;
        float fmass1;
        NiTMapItem<int32_t, uint32_t>* pos;
        int32_t itime;
        [[maybe_unused]] float fratio;
        static BSCriticalSection kcriticalSection;
        kcriticalSection.Enter("ImpactMixer::PlayCollisionSound");
        // fmeterspersec and fratio are computed and never read, as the game does.
        [[maybe_unused]] float fmeterspersec = arDataIn.fVelocity * 0.014287499710917473;
        fratio = arDataIn.fVelocity / 200.0;
        uint32_t ibattleFlag = 0;
        if (arDataIn.cMaterialType1 == BHK_MATERIAL_MAX && arDataIn.cMaterialType2 == BHK_MATERIAL_ORGANIC) {
            fratio = fratio * 4.0;
        }
        if (arDataIn.pBody1) {
            fmass1 = arDataIn.pBody1->GetDynamicMass();
        }
        else {
            fmass1 = 5.0f;
        }
        if (arDataIn.pBody2) {
            fmass2 = arDataIn.pBody2->GetDynamicMass();
        }
        else {
            fmass2 = 5.0f;
        }
        bool bshellCasing = false;
        TESSound* info = nullptr;
        TESSound* info2 = nullptr;
        if ((arDataIn.pBody1 &&
             (CollisionFilter = arDataIn.pBody1->GetCollisionFilter(&apResult),
              CollisionFilter->GetLayer() == L_SHELLCASING)) ||
            (arDataIn.pBody2 && (v2 = arDataIn.pBody2->GetCollisionFilter(&v19), v2->GetLayer() == L_SHELLCASING))) {
            bshellCasing = true;
        }
        if (bshellCasing) {
            float fcasingMass;
            ibattleFlag = BSGameSound::SOUND_FLAG_BATTLE_SOUND;
            if (!(fmass2 < static_cast<double>(fmass1))) {
                fcasingMass = fmass2;
            }
            else {
                fcasingMass = fmass1;
            }
            if (!(fcasingMass < 1.0)) {
                if (FloatEQ(fcasingMass, 2.0f, 0.25f)) {
                    info = pDataHandler->GetSound("PHYCasingPistol");
                }
                else {
                    info = pDataHandler->GetSound("PHYCasingRifle");
                }
            }
            else {
                info = pDataHandler->GetSound("PHYCasingShotgun");
            }
            info2 = nullptr;
        }
        else {
            info = ImpactMixer::GetSoundForType(arDataIn.cMaterialType1, fmass1, arDataIn.fVelocity);
            info2 = ImpactMixer::GetSoundForType(arDataIn.cMaterialType2, fmass2, arDataIn.fVelocity);
        }
        if (FalloutAudio::SoundDistanceTest(info, arDataIn.Location, 1.0f) ||
            FalloutAudio::SoundDistanceTest(info2, arDataIn.Location, 1.0f)) {
            uint32_t v3;
            BSSoundHandle SoundHand;
            BSSoundHandle SoundHand2;
            uint32_t uiCurrentTime = appTimer.iLastTime;
            uiSinceLastPlayed = 0;
            if (ImpactMixer::pImpactMap.GetAt(arDataIn.iID, uiSinceLastPlayed) &&
                (v3 = uiCurrentTime - uiSinceLastPlayed, v3 < ImpactMixer::iCollisionSoundTimeDelta.Int())) {
                kcriticalSection.Leave();
            }
            else {
                double velFreq;
                double v11;
                ImpactMixer::pImpactMap.SetAt(arDataIn.iID, uiCurrentTime);
                if ((info) &&
                    (arDataIn.cMaterialType1 != BHK_MATERIAL_SKIN || arDataIn.cMaterialType2 != BHK_MATERIAL_SKIN)) {
                    uint32_t arPos_8 = ibattleFlag |
                        (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                         BSGameSound::SOUND_FLAG_IMPACT);
                    uint32_t arPos_4 = info->iFormID;
                    BSAudio* v4 = &BSAudio::QInstance();
                    SoundHand = v4->GetSoundHandleByNumericID(arPos_4, arPos_8);
                }

                if ((info2) &&
                    (arDataIn.cMaterialType1 != BHK_MATERIAL_SKIN || arDataIn.cMaterialType2 != BHK_MATERIAL_SKIN)) {
                    uint32_t arPos_8a = ibattleFlag |
                        (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                         BSGameSound::SOUND_FLAG_IMPACT);
                    uint32_t arPos_4a = info2->iFormID;
                    BSAudio* v5 = &BSAudio::QInstance();
                    SoundHand2 = v5->GetSoundHandleByNumericID(arPos_4a, arPos_8a);
                }

                if (!(arDataIn.fVelocity / 400.0 > 1.0)) {
                    v11 = arDataIn.fVelocity / 400.0;
                }
                else {
                    v11 = 1.0;
                }
                uint32_t velVol = static_cast<int64_t>((1.0 - v11) * 3000.0);
                if (velVol >= 1500) {
                    velFreq = (static_cast<double>(velVol) - 4500.0) / -3000.0;
                }
                else {
                    velFreq = 1.0;
                }
                float velFreq_2 = velFreq;
                if (info) {
                    SoundHand.SetPriority(255u);
                    SoundHand.SetPosition(arDataIn.Location.x, arDataIn.Location.y, arDataIn.Location.z);
                    SoundHand.SetStaticAttenuation(velVol);
                    SoundHand.SetModFrequency(velFreq_2);
                    SoundHand.Play(false);
                }
                if (info2) {
                    SoundHand2.SetPriority(255u);
                    SoundHand2.SetPosition(arDataIn.Location.x, arDataIn.Location.y, arDataIn.Location.z);
                    SoundHand2.SetStaticAttenuation(velVol);
                    SoundHand2.SetModFrequency(velFreq_2);
                    SoundHand2.Play(false);
                }
                ImpactMixer::lastloc = arDataIn.Location;
                int32_t itimeOut = 4 * ImpactMixer::iCollisionSoundTimeDelta.Int();
                pos = ImpactMixer::pImpactMap.GetFirstPos();
                itime = appTimer.iLastTime;
                uint32_t iloopcount = 0;
                while (pos) {
                    int32_t key;
                    ++iloopcount;
                    ImpactMixer::pImpactMap.GetNext(pos, key, val);
                    // The time difference is compared signed.
                    if (static_cast<int32_t>(itime - val) > itimeOut) {
                        ImpactMixer::pImpactMap.RemoveAt(key);
                        pos = ImpactMixer::pImpactMap.GetFirstPos();
                    }
                    if (iloopcount > 256) {
                        ImpactMixer::pImpactMap.RemoveAll_1();
                        pos = nullptr;
                    }
                }
                kcriticalSection.Leave();
            }
        }
        else {
            kcriticalSection.Leave();
        }
    }
}

bool ImpactMixer::GetGunshotsInitialized() {
    return (ImpactMixer::iiInitializationFlags & 2) != 0;
}

void ImpactMixer::SetGunshotsInitialized(bool abIn) {
    uint32_t v1;
    if (abIn) {
        v1 = ImpactMixer::iiInitializationFlags | 2;
    }
    else {
        v1 = ImpactMixer::iiInitializationFlags & 0xFFFFFFFDu;
    }
    ImpactMixer::iiInitializationFlags = v1;
}

void ImpactMixer::SetAltMaterialSoundName(int32_t aiMat, const char* apAltName) {
    ImpactMixer::pAltMaterialSoundNames[aiMat] = apAltName;
}

void ImpactMixer::PlayFootstep(TESObjectREFR* apRef, int32_t aiFootType) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_AUDIO, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Audio\\ImpactMixer.cpp", 735
    );
    if (Interface::IsInMenuMode()) {
        return;
    }
    NiPoint3 loc = apRef->GetLocationOnReference();
    BSSoundHandle SoundHand;
    BSSoundHandle SoundHand2;
    TESSound* infoGround = nullptr;
    TESSound* infoShoe_3 = nullptr;
    bool b1stPersonPlayerFoot = false;
    float ftestDistance = gs_fMaxFootstepDistance_Audio.Float();
    if (Main::GetSceneIsComplex()) {
        ftestDistance = ftestDistance / 2.0;
    }
    float fdistance = 0.0f;
    if ((&BSAudio::QInstance())->pListener) {
        BSAudioListener* v47 = (&BSAudio::QInstance())->pListener;
        NiPoint3 listenPt = v47->GetPosition();
        listenPt -= loc;
        fdistance = listenPt.Length();
        double fdistance_2 = listenPt.Length();
        if (ftestDistance < fdistance_2) {
            goto LABEL_7;
        }
    }
    if (apRef->IsActor()) {
        MobileObject* pmob;
        pmob = dynamic_cast<Actor*>(apRef);
        if (pmob->IsDead(false)) {
        LABEL_7:;
            return;
        }
        if (pmob->IsCreature()) {
            TESBoundObject* pobj = apRef->GetObjectReference();
            TESCreature* pcreature = dynamic_cast<TESCreature*>(pobj);
            TESSound* pcreatureSound = pcreature->PickCreatureSound(static_cast<CREATURE_SOUND_TYPE>(aiFootType));
            infoGround = pcreatureSound;
            if (pcreatureSound) {
                uint8_t MaxDistance = infoGround->GetMaxDistance();
                if (fdistance <= static_cast<double>(100 * MaxDistance)) {
                    uint32_t aPosition_8 = infoGround->iFormID;
                    BSAudio* v4 = &BSAudio::QInstance();
                    SoundHand = v4->GetSoundHandleByNumericID(
                        aPosition_8,
                        BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
                            BSGameSound::SOUND_FLAG_ONE_SHOT
                    );
                }
            }
            SoundHand.SetPosition(loc.x, loc.y, loc.z);
            if (pcreature->Get3D()) {
                NiAVObject* p3D = pcreature->Get3D();
                SoundHand.SetObjectToFollow(p3D);
            }
            SoundHand.SetVolume(0.98f);
            SoundHand.Play(false);
        }
        else {
            HavokMaterialType groundType = BHK_MATERIAL_STONE;
            TESObjectCELL* pcell = TES::pInstance->pInteriorCell;
            if (!pcell) {
                TESWorldSpace* aPosition_8a = TES::pInstance->pWorldSpace;
                pcell = pDataHandler->GetCellFromWorldCoord(loc.x, loc.y, aPosition_8a, false);
            }
            bool bWaterStep = false;
            bool bStairs = false;
            if ((!static_cast<Actor*>(pmob)->Actor::IsSwimming()) && (pcell)) {
                double RelevantWaterHeight = pmob->GetRelevantWaterHeight();
                if (loc.z < RelevantWaterHeight &&
                    ((pcell->GetInterior() && pcell->HasWater()) || !pcell->GetInterior())) {
                    bWaterStep = true;
                }
            }

            bhkCharacterController* CharController = pmob->GetCharController();
            if (static_cast<uint8_t>(CharController->GetOnStairs())) {
                bStairs = true;
            }
            if (!TES::pInstance->pInteriorCell && (pcell && pcell->GetLand())) {
                COORD_DATA cdata;
                TESObjectLAND* Land = pcell->GetLand();
                Land->GetCoordData(&cdata, loc, true);
                uint16_t afZ = cdata.iVertidx;
                uint8_t aPosition_8b = cdata.iBlock;
                TESObjectLAND* v9 = pcell->GetLand();
                TESLandTexture* plandTex = v9->GetMainTexture(aPosition_8b, afZ);
                float fheight = 0.0f;
                groundType = BHK_MATERIAL_STONE;
                if (bWaterStep) {
                    groundType = BHK_MATERIAL_WATER;
                }
                else {
                    if (pmob &&
                        (pmob->GetCharController() &&
                         pmob->GetCharController()->eSupportMaterial != BHK_MATERIAL_MAX)) {
                        groundType = pmob->GetCharController()->eSupportMaterial;
                    }
                    else {
                        if (plandTex) {
                            groundType = plandTex->GetHavokMaterialType();
                            if (pcell->GetLandHeight(loc, fheight)) {
                                loc.z = fheight;
                            }
                        }
                    }
                }
            }
            else {
                if (bWaterStep) {
                    groundType = BHK_MATERIAL_WATER;
                }
                else {
                    if ((pmob) && (pmob->GetCharController())) {
                        groundType = pmob->GetCharController()->eSupportMaterial;
                    }
                }
            }
            if (aiFootType > 1) {
                aiFootType -= 2;
            }
            int32_t stepType = aiFootType == 0;
            // matType is zeroed before the call that sets it.
            int32_t matType = 0;
            if (static_cast<Actor*>(pmob)->Actor::IsSneaking()) {
                stepType = 5 - (aiFootType != 0);
            }
            else {
                if (bStairs) {
                    stepType = 7 - (aiFootType != 0);
                }
                else {
                    if (static_cast<Actor*>(pmob)->Actor::IsRunning()) {
                        stepType = 3 - (aiFootType != 0);
                    }
                }
            }
            matType = ImpactMixer::GetFootstepIndexForType(groundType);
            infoGround = ImpactMixer::pFootSteps[matType][stepType];
            if (static_cast<Actor*>(pmob)->Actor::IsPowerBodyArmorWorn()) {
                TESSound* infoShoe;
                if (!ImpactMixer::pPowerArmorStepL || !ImpactMixer::pPowerArmorStepR) {
                    ImpactMixer::pPowerArmorStepR = pDataHandler->GetSound("FSTArmorPowerR");
                    ImpactMixer::pPowerArmorStepL = pDataHandler->GetSound("FSTArmorPowerL");
                }
                if (aiFootType) {
                    infoShoe = ImpactMixer::pPowerArmorStepL;
                }
                else {
                    infoShoe = ImpactMixer::pPowerArmorStepR;
                }
                infoShoe_3 = infoShoe;
            }
            else {
                if (static_cast<Actor*>(pmob)->Actor::IsHeavyBodyArmorWorn()) {
                    TESSound* infoShoe_2;
                    if (!ImpactMixer::pHeavyArmorStepL || !ImpactMixer::pHeavyArmorStepR) {
                        ImpactMixer::pHeavyArmorStepR = pDataHandler->GetSound("FSTArmorR");
                        ImpactMixer::pHeavyArmorStepL = pDataHandler->GetSound("FSTArmorL");
                    }
                    if (aiFootType) {
                        infoShoe_2 = ImpactMixer::pHeavyArmorStepL;
                    }
                    else {
                        infoShoe_2 = ImpactMixer::pHeavyArmorStepR;
                    }
                    infoShoe_3 = infoShoe_2;
                }
            }
            if (apRef == pPlayer && !pPlayer->IsActually3rdPerson()) {
                b1stPersonPlayerFoot = true;
            }
            if (b1stPersonPlayerFoot) {
                uint32_t iflags = BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT;
                if (infoGround) {
                    const char* aPosition_4 = infoGround->GetFilename();
                    BSAudio* v10 = &BSAudio::QInstance();
                    SoundHand = v10->GetSoundHandleByFilename(aPosition_4, iflags, infoGround);
                    float* v11 = &gs_fPlayerFootVolume_Audio.Float();
                    SoundHand.SetVolume(*v11);
                    SoundHand.Play(false);
                }
                if (infoShoe_3) {
                    const char* aPosition_4a = infoShoe_3->GetFilename();
                    BSAudio* v12 = &BSAudio::QInstance();
                    SoundHand2 = v12->GetSoundHandleByFilename(aPosition_4a, iflags, infoShoe_3);
                    float* v13 = &gs_fPlayerFootVolume_Audio.Float();
                    SoundHand2.SetVolume(*v13);
                    SoundHand2.Play(false);
                }
            }
            else {
                uint32_t iflags_2 = BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT;
                if (infoGround) {
                    uint8_t v14 = infoGround->GetMaxDistance();
                    if (fdistance <= static_cast<double>(100 * v14)) {
                        const char* aPosition_4b = infoGround->GetFilename();
                        BSAudio* v15 = &BSAudio::QInstance();
                        SoundHand = v15->GetSoundHandleByFilename(aPosition_4b, iflags_2, infoGround);
                        float v40 = loc.z - 128.0;
                        SoundHand.SetPosition(loc.x, loc.y, v40);
                        if (apRef->Get3D()) {
                            NiNode* v16 = apRef->Get3D();
                            SoundHand.SetObjectToFollow(v16);
                        }
                        float afZd = RandomFloat(0.9f, 1.0f);
                        SoundHand.SetVolume(afZd);
                        SoundHand.Play(false);
                    }
                }
                if (infoShoe_3) {
                    uint8_t v17 = infoShoe_3->GetMaxDistance();
                    if (fdistance <= static_cast<double>(100 * v17)) {
                        const char* aPosition_4c = infoShoe_3->GetFilename();
                        BSAudio* v18 = &BSAudio::QInstance();
                        SoundHand2 = v18->GetSoundHandleByFilename(aPosition_4c, iflags_2, infoShoe_3);
                        float v38 = loc.z - 128.0;
                        SoundHand2.SetPosition(loc.x, loc.y, v38);
                        if (apRef->Get3D()) {
                            NiNode* v19 = apRef->Get3D();
                            SoundHand2.SetObjectToFollow(v19);
                        }
                        float v37 = 1.0 - RandomFloat(0.0f, 0.1f);
                        SoundHand2.SetVolume(v37);
                        SoundHand2.Play(false);
                    }
                }
            }
        }
    }
}

void ImpactMixer::PlayWeaponBlock(
    int32_t aiAttWeaponType,
    TESObjectWEAP* apDefWeapon,
    NiPoint3 aAttacker,
    NiPoint3 aDefender,
    bool bEnchantedWeapon
) {
    BSSoundHandle SoundHand;
    if (apDefWeapon && apDefWeapon->GetBlockSound()) {
        TESSound* arPos_8 = apDefWeapon->GetBlockSound();
        TESSound* BlockSound = apDefWeapon->GetBlockSound();
        const char* arPos = BlockSound->GetFilename();
        BSAudio* v6 = &BSAudio::QInstance();
        SoundHand = v6->GetSoundHandleByFilename(arPos, 386u, arPos_8);
    }
    else {
        int32_t idefWeaponType;
        if (apDefWeapon) {
            idefWeaponType = apDefWeapon->GetWeaponType();
        }
        else {
            idefWeaponType = -1;
        }
        if (idefWeaponType == -1) {
            BSAudio* v7 = &BSAudio::QInstance();
            SoundHand = v7->GetSoundHandleByName("WPNBlockHand", 386u);
        }
        else {
            BSAudio* v8 = &BSAudio::QInstance();
            SoundHand = v8->GetSoundHandleByName("WPNBlockMelee", 386u);
        }
    }
    double v16 = BSMIN(aAttacker.x, aDefender.x);
    double v15 = BSMAX(aAttacker.x, aDefender.x);
    float ecx0 = (v15 - BSMIN(aAttacker.x, aDefender.x)) / 2.0 + v16;
    double v14 = BSMIN(aAttacker.y, aDefender.y);
    double v13 = BSMAX(aAttacker.y, aDefender.y);
    float v26 = (v13 - BSMIN(aAttacker.y, aDefender.y)) / 2.0 + v14;
    double v12 = BSMIN(aAttacker.z, aDefender.z);
    double v11 = BSMAX(aAttacker.z, aDefender.z);
    float v27 = (v11 - BSMIN(aAttacker.z, aDefender.z)) / 2.0 + v12;
    SoundHand.SetPosition(ecx0, v26, v27);
    SoundHand.Play(false);
}

TESSound* ImpactMixer::GetSoundForType(int32_t aiType, float afMass, float afVelocity) {
    TESSound* info = nullptr;
    switch (aiType) {
        case 0:
        case 15: {
            if (!(afMass < 9.999999747378752e-05)) {
                if (!(gs_fStoneMediumMassMin_Audio.Float() > static_cast<double>(afMass))) {
                    if (!(gs_fStoneLargeMassMin_Audio.Float() > static_cast<double>(afMass))) {
                        info = pDataHandler->GetSound("CStoneLarge");
                    }
                    else {
                        info = pDataHandler->GetSound("CStoneMedium");
                    }
                }
                else {
                    info = pDataHandler->GetSound("CStoneSmall");
                }
            }
            else {
                info = pDataHandler->GetSound("CStoneStatic");
            }
            break;
        }
        case 1: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYGenericClothH");
            }
            else {
                info = pDataHandler->GetSound("PHYGenericClothL");
            }
            break;
        }
        case 2: {
            if (!(gs_fEarthMediumMassMin_Audio.Float() > static_cast<double>(afMass))) {
                if (!(gs_fEarthLargeMassMin_Audio.Float() > static_cast<double>(afMass))) {
                    info = pDataHandler->GetSound("CEarthLarge");
                }
                else {
                    info = pDataHandler->GetSound("CEarthMedium");
                }
            }
            else {
                info = pDataHandler->GetSound("CEarthSmall");
            }
            break;
        }
        case 3: {
            if (!(afMass < 6.0)) {
                if (!(afMass < 15.0)) {
                    info = pDataHandler->GetSound("CGlassLarge");
                }
                else {
                    info = pDataHandler->GetSound("CGlassMedium");
                }
            }
            else {
                info = pDataHandler->GetSound("CGlassSmall");
            }
            break;
        }
        case 4: {
            if (!(afMass < 6.0)) {
                if (!(afMass < 70.0)) {
                    info = pDataHandler->GetSound("CGrassLarge");
                }
                else {
                    info = pDataHandler->GetSound("CGrassMedium");
                }
            }
            else {
                info = pDataHandler->GetSound("CGrassSmall");
            }
            break;
        }
        case 5: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                if (!(afMass < 9.999999747378752e-05)) {
                    if (!(gs_fMetalMediumMassMin_Audio.Float() > static_cast<double>(afMass))) {
                        if (!(gs_fMetalLargeMassMin_Audio.Float() > static_cast<double>(afMass))) {
                            info = pDataHandler->GetSound("PHYGenericMetalLargeH");
                        }
                        else {
                            info = pDataHandler->GetSound("PHYGenericMetalMediumH");
                        }
                    }
                    else {
                        info = pDataHandler->GetSound("PHYGenericMetalSmallH");
                    }
                }
                else {
                    info = pDataHandler->GetSound("PHYGenericMetalStaticH");
                }
            }
            else {
                if (!(afMass < 9.999999747378752e-05)) {
                    if (!(gs_fMetalMediumMassMin_Audio.Float() > static_cast<double>(afMass))) {
                        if (!(gs_fMetalLargeMassMin_Audio.Float() > static_cast<double>(afMass))) {
                            info = pDataHandler->GetSound("PHYGenericMetalLargeL");
                        }
                        else {
                            info = pDataHandler->GetSound("PHYGenericMetalMediumL");
                        }
                    }
                    else {
                        info = pDataHandler->GetSound("PHYGenericMetalSmallL");
                    }
                }
                else {
                    info = pDataHandler->GetSound("PHYGenericMetalStaticL");
                }
            }
            break;
        }
        case 6: {
            info = pDataHandler->GetSound("COrganicSmall");
            break;
        }
        case 7: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                if (!(gs_fSkinMediumMassMin_Audio.Float() > static_cast<double>(afMass))) {
                    if (!(gs_fSkinLargeMassMin_Audio.Float() > static_cast<double>(afMass))) {
                        info = pDataHandler->GetSound("PHYSkinLargeH");
                    }
                    else {
                        info = pDataHandler->GetSound("PHYSkinMediumH");
                    }
                }
                else {
                    info = pDataHandler->GetSound("PHYSkinSmallH");
                }
            }
            else {
                if (!(gs_fSkinMediumMassMin_Audio.Float() > static_cast<double>(afMass))) {
                    if (!(gs_fSkinLargeMassMin_Audio.Float() > static_cast<double>(afMass))) {
                        info = pDataHandler->GetSound("PHYSkinLargeL");
                    }
                    else {
                        info = pDataHandler->GetSound("PHYSkinMediumL");
                    }
                }
                else {
                    info = pDataHandler->GetSound("PHYSkinSmallL");
                }
            }
            break;
        }
        case 8: {
            if (afMass < 6.0) {
                goto LABEL_37;
            }
            if (afMass < 15.0) {
                goto LABEL_39;
            }
            goto LABEL_40;
        }
        case 9: {
            if (!(afMass < 9.999999747378752e-05)) {
                if (!(gs_fWoodMediumMassMin_Audio.Float() > static_cast<double>(afMass))) {
                    if (!(gs_fWoodLargeMassMin_Audio.Float() > static_cast<double>(afMass))) {
                    LABEL_40:;
                        info = pDataHandler->GetSound("CWoodLarge");
                    }
                    else {
                    LABEL_39:;
                        info = pDataHandler->GetSound("CWoodMedium");
                    }
                }
                else {
                LABEL_37:;
                    info = pDataHandler->GetSound("CWoodSmall");
                }
            }
            else {
                info = pDataHandler->GetSound("CWoodStatic");
            }
            break;
        }
        case 10: {
            info = pDataHandler->GetSound("CSpecialHeavyStone");
            break;
        }
        case 11: {
            info = pDataHandler->GetSound("CSpecialHeavyMetal");
            break;
        }
        case 12: {
            info = pDataHandler->GetSound("CSpecialHeavyWood");
            break;
        }
        case 13: {
            info = pDataHandler->GetSound("PHYChain");
            break;
        }
        case 14: {
            info = pDataHandler->GetSound("PHYBottlecaps");
            break;
        }
        case 16: {
            if (!(afMass < 9.999999747378752e-05)) {
                if (!(gs_fMetalMediumMassMin_Audio.Float() > static_cast<double>(afMass))) {
                    if (!(gs_fMetalLargeMassMin_Audio.Float() > static_cast<double>(afMass))) {
                        info = pDataHandler->GetSound("PHYGenericMetalHollowLarge");
                    }
                    else {
                        info = pDataHandler->GetSound("PHYGenericMetalHollowMedium");
                    }
                }
                else {
                    info = pDataHandler->GetSound("PHYGenericMetalHollowSmall");
                }
            }
            else {
                info = pDataHandler->GetSound("PHYGenericMetalHollowStatic");
            }
            break;
        }
        case 17: {
            if (!(afMass < 9.999999747378752e-05)) {
                if (!(gs_fMetalMediumMassMin_Audio.Float() > static_cast<double>(afMass))) {
                    if (!(gs_fMetalLargeMassMin_Audio.Float() > static_cast<double>(afMass))) {
                        info = pDataHandler->GetSound("PHYGenericMetalSheetLarge");
                    }
                    else {
                        info = pDataHandler->GetSound("PHYGenericMetalSheetMedium");
                    }
                }
                else {
                    info = pDataHandler->GetSound("PHYGenericMetalSheetSmall");
                }
            }
            else {
                info = pDataHandler->GetSound("PHYGenericMetalSheetStatic");
            }
            break;
        }
        case 19: {
            info = nullptr;
            break;
        }
        case 20: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYVehicleMetalBodyH");
            }
            else {
                info = pDataHandler->GetSound("PHYVehicleMetalBodyL");
            }
            break;
        }
        case 21: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYVehicleMetalSolidH");
            }
            else {
                info = pDataHandler->GetSound("PHYVehicleMetalSolidL");
            }
            break;
        }
        case 22: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYVehicleMetalHollowH");
            }
            else {
                info = pDataHandler->GetSound("PHYVehicleMetalHollowL");
            }
            break;
        }
        case 23: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYBarrelH");
            }
            else {
                info = pDataHandler->GetSound("PHYBarrelL");
            }
            break;
        }
        case 24: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYBottleH");
            }
            else {
                info = pDataHandler->GetSound("PHYBottleL");
            }
            break;
        }
        case 25: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYCanSodaH");
            }
            else {
                info = pDataHandler->GetSound("PHYCanSodaL");
            }
            break;
        }
        case 26: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYWeaponPistolH");
            }
            else {
                info = pDataHandler->GetSound("PHYWeaponPistolL");
            }
            break;
        }
        case 27: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYWeaponRifleH");
            }
            else {
                info = pDataHandler->GetSound("PHYWeaponRifleL");
            }
            break;
        }
        case 28: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYShoppingCartH");
            }
            else {
                info = pDataHandler->GetSound("PHYShoppingCartL");
            }
            break;
        }
        case 29: {
            if (!(gs_fCollisionSoundHeavyThreshold_Audio.Float() > static_cast<double>(afVelocity))) {
                info = pDataHandler->GetSound("PHYLunchboxH");
            }
            else {
                info = pDataHandler->GetSound("PHYLunchboxL");
            }
            break;
        }
        case 30: {
            info = pDataHandler->GetSound("PHYBabyRattle");
            break;
        }
        case 31: {
            info = pDataHandler->GetSound("PHYRubberBall");
            break;
        }
        case 32: {
            info = pDataHandler->GetSound("PHYChainlink");
            break;
        }
        case 33: {
            info = pDataHandler->GetSound("PHYTile");
            break;
        }
        case 34: {
            info = pDataHandler->GetSound("PHYCarpet");
            break;
        }
        case 35: {
            info = pDataHandler->GetSound("PHYTumbleweed");
            break;
        }
        default: {
            return info;
        }
    }
    return info;
}

int32_t ImpactMixer::GetFootstepIndexForType(HavokMaterialType aiType) {
    if (aiType > BHK_MATERIAL_STONE && aiType < BHK_MATERIAL_MAX) {
        return MaterialEquivalents[aiType];
    }

    return 0;
}

void ImpactMixer::PlayEquipSound(TESObjectREFR* apRef, int32_t aiType) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_AUDIO, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Audio\\ImpactMixer.cpp", 1699
    );
    NiPoint3 loc = apRef->GetLocationOnReference();
    float ftestDistance = gs_fMaxFootstepDistance_Audio.Float();
    bool b2Handed = false;
    Actor* pactor = dynamic_cast<Actor*>(apRef);
    if (pactor && pactor->pCurrentProcess) {
        TESObjectWEAP* pweap;
        TESBoundObject* pbobj;
        BSSoundHandle SoundHand;
        BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
        ItemChange* pweapdata = CurrentAIProcess->GetCurrentWeapon();
        if (pweapdata) {
            pbobj = pweapdata->pContainerObj;
        }
        else {
            pbobj = nullptr;
        }
        if (pbobj) {
            pweap = dynamic_cast<TESObjectWEAP*>(pbobj);
        }
        else {
            pweap = nullptr;
        }
        if (pweap) {
            TESSound* pactionSound = nullptr;
            if (aiType == 9) {
                pactionSound = pweap->GetEquipSound();
            }
            else {
                if (aiType == 10) {
                    pactionSound = pweap->GetUnequipSound();
                }
            }
            if (!pactionSound || !FalloutAudio::SoundDistanceTest(pactionSound, loc, 1.0f)) {
                return;
            }
            bool bSneaker = pactor->IsSneaking();
            if (pactor == pPlayer) {
                const char* a2 = pactionSound->GetFilename();
                BSAudio* v2 = &BSAudio::QInstance();
                SoundHand = v2->GetSoundHandleByFilename(
                    a2,
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT |
                        BSGameSound::SOUND_FLAG_UNKBIT31,
                    pactionSound
                );
            }
            else {
                const char* a2a = pactionSound->GetFilename();
                BSAudio* v3 = &BSAudio::QInstance();
                SoundHand = v3->GetSoundHandleByFilename(
                    a2a,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                    pactionSound
                );
                SoundHand.SetPosition(loc.x, loc.y, loc.z);
                NiNode* v4 = apRef->Get3D();
                SoundHand.SetObjectToFollow(v4);
            }
            if (bSneaker) {
                SoundHand.SetVolume(0.4f);
            }
            else {
                SoundHand.SetVolume(1.0f);
            }
            SoundHand.Play(false);
        }
    }
}

void ImpactMixer::DeInitialize() {
    ImpactMixer::pImpactMap.RemoveAll_1();
}
