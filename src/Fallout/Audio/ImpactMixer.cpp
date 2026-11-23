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

#include <limits>

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

// Location is built by NiPoint3's default constructor; the rest is deliberately left unset.
ImpactMixer::COLLISION_SOUND::COLLISION_SOUND() {}

void ImpactMixer::PlayJumpSound(TESObjectREFR* apRef, int32_t aiFootType) {
    AutoMemContext kContext(
        MEM_CONTEXT_AUDIO,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Audio\\ImpactMixer.cpp",
        941
    );
    if (Interface::IsInMenuMode()) {
        return;
    }
    NiPoint3 kRefPos = apRef->GetLocationOnReference();
    BSSoundHandle kSound;
    BSSoundHandle th_s;
    TESSound* pSound = nullptr;
    TESSound* pArmorSound = nullptr;
    char bUse2DSound = 0;
    float fMaxFootstepDistance = gs_fMaxFootstepDistance_Audio.Float();
    if (Main::GetSceneIsComplex()) {
        fMaxFootstepDistance = fMaxFootstepDistance / 2.0;
    }
    float fListenerDistance = 0.0f;
    if (BSAudio::QInstance().pListener) {
        BSAudioListener* pListener = BSAudio::QInstance().pListener;
        NiPoint3 kListenerPos = pListener->GetPosition();
        kListenerPos -= kRefPos;
        fListenerDistance = kListenerPos.Length();
        double dLength = kListenerPos.Length();
        if (fMaxFootstepDistance < dLength) {
            return;
        }
    }
    if (apRef->IsActor()) {
        Actor* pActor = dynamic_cast<Actor*>(apRef);
        if (pActor->IsDead(false)) {
            return;
        }
        if (pActor->IsCreature()) {
            TESBoundObject* pBaseForm = apRef->GetObjectReference();
            TESCreature* pCreature = dynamic_cast<TESCreature*>(pBaseForm);
            pSound = pCreature->PickCreatureSound(CREA_SOUND_JUMP);
            if (pSound) {
                uint8_t ucMaxDistance = pSound->GetMaxDistance();
                if (fListenerDistance <= static_cast<double>(100 * ucMaxDistance)) {
                    uint32_t uiFormID = pSound->iFormID;
                    BSAudio* pAudio = &BSAudio::QInstance();
                    kSound = pAudio->GetSoundHandleByNumericID(
                        uiFormID,
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
                TESWorldSpace* pWorldSpace = TES::pInstance->pWorldSpace;
                pInterior = pDataHandler->GetCellFromWorldCoord(kRefPos.x, kRefPos.y, pWorldSpace, false);
            }
            char bUnderwater = 0;
            if (!pActor->IsSwimming() && pInterior) {
                double dWaterHeight = pActor->GetRelevantWaterHeight();
                if (kRefPos.z < dWaterHeight &&
                    ((pInterior->GetInterior() && pInterior->HasWater()) || !pInterior->GetInterior())) {
                    bUnderwater = 1;
                }
            }

            MobileObject* pMobile = pActor;
            if (!TES::pInstance->pInteriorCell && pInterior && pInterior->GetLand()) {
                COORD_DATA coordData;
                TESObjectLAND* pLand = pInterior->GetLand();
                pLand->GetCoordData(&coordData, kRefPos, true);
                uint16_t iCoordVertex = coordData.iVertidx;
                uint8_t ucBlock = coordData.iBlock;
                pLand = pInterior->GetLand();
                TESLandTexture* pMainTexture = pLand->GetMainTexture(ucBlock, iCoordVertex);
                float fHeight = 0.0f;
                eMaterialType = BHK_MATERIAL_STONE;
                if (bUnderwater) {
                    eMaterialType = BHK_MATERIAL_WATER;
                }
                else if (
                    pMobile &&
                    pMobile->GetCharController() &&
                    pMobile->GetCharController()->eSupportMaterial != BHK_MATERIAL_MAX
                ) {
                    eMaterialType = pMobile->GetCharController()->eSupportMaterial;
                }
                else if (pMainTexture) {
                    eMaterialType = pMainTexture->GetHavokMaterialType();
                    if (pInterior->GetLandHeight(kRefPos, fHeight)) {
                        kRefPos.z = fHeight;
                    }
                }
            }
            else if (bUnderwater) {
                eMaterialType = BHK_MATERIAL_WATER;
            }
            else if (pMobile && pMobile->GetCharController()) {
                eMaterialType = pMobile->GetCharController()->eSupportMaterial;
            }
            int32_t iJumpIndex = 9;
            int32_t iFootstepIndex = ImpactMixer::GetFootstepIndexForType(eMaterialType);
            pSound = ImpactMixer::pFootSteps[iFootstepIndex][iJumpIndex];
            if (pActor->IsPowerBodyArmorWorn()) {
                if (!ImpactMixer::pJumpPower) {
                    ImpactMixer::pJumpPower = pDataHandler->GetSound("FSTJumpPower");
                }
                // Loads the power armour sound but plays the heavy armour one, deliberately.
                pArmorSound = ImpactMixer::pJumpHeavy;
            }
            else if (pActor->IsHeavyBodyArmorWorn()) {
                if (!ImpactMixer::pJumpHeavy) {
                    ImpactMixer::pJumpHeavy = pDataHandler->GetSound("FSTJumpArmor");
                }
                pArmorSound = ImpactMixer::pJumpHeavy;
            }
            if (apRef == pPlayer && !pPlayer->IsActually3rdPerson()) {
                bUse2DSound = 1;
            }
            if (bUse2DSound) {
                uint32_t uiAudioFlags = BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT;
                if (pSound) {
                    const char* pcFilename = pSound->GetFilename();
                    BSAudio* pAudio = &BSAudio::QInstance();
                    kSound = pAudio->GetSoundHandleByFilename(pcFilename, uiAudioFlags, pSound);
                    kSound.SetVolume(gs_fPlayerFootVolume_Audio.Float());
                    kSound.Play(false);
                }
                if (pArmorSound) {
                    const char* pcFilename = pArmorSound->GetFilename();
                    BSAudio* pAudio = &BSAudio::QInstance();
                    th_s = pAudio->GetSoundHandleByFilename(pcFilename, uiAudioFlags, pArmorSound);
                    th_s.SetVolume(gs_fPlayerFootVolume_Audio.Float());
                    th_s.Play(false);
                }
            }
            else {
                uint32_t uiAudioFlags = BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT;
                if (!FalloutAudio::SoundDistanceTest(pSound, kRefPos, 1.0f) &&
                    !FalloutAudio::SoundDistanceTest(pArmorSound, kRefPos, 1.0f)) {
                    return;
                }
                if (pSound) {
                    uint8_t ucMaxDistance = pSound->GetMaxDistance();
                    if (fListenerDistance <= static_cast<double>(100 * ucMaxDistance)) {
                        const char* pcFilename = pSound->GetFilename();
                        BSAudio* pAudio = &BSAudio::QInstance();
                        kSound = pAudio->GetSoundHandleByFilename(pcFilename, uiAudioFlags, pSound);
                        float fZ = kRefPos.z - 128.0;
                        kSound.SetPosition(kRefPos.x, kRefPos.y, fZ);
                        if (apRef->Get3D()) {
                            NiNode* p3D = apRef->Get3D();
                            kSound.SetObjectToFollow(p3D);
                        }
                        float fVolume = RandomFloat(0.9f, 1.0f);
                        kSound.SetVolume(fVolume);
                        kSound.Play(false);
                    }
                }
                if (pArmorSound) {
                    uint8_t ucMaxDistance = pArmorSound->GetMaxDistance();
                    if (fListenerDistance <= static_cast<double>(100 * ucMaxDistance)) {
                        const char* pcFilename = pArmorSound->GetFilename();
                        BSAudio* pAudio = &BSAudio::QInstance();
                        th_s = pAudio->GetSoundHandleByFilename(pcFilename, uiAudioFlags, pArmorSound);
                        float fZ = kRefPos.z - 128.0;
                        th_s.SetPosition(kRefPos.x, kRefPos.y, fZ);
                        if (apRef->Get3D()) {
                            NiNode* p3D = apRef->Get3D();
                            th_s.SetObjectToFollow(p3D);
                        }
                        float fVolume = 1.0 - RandomFloat(0.0f, 0.1f);
                        th_s.SetVolume(fVolume);
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
        NiPoint3 origin = NiPoint3::ZERO;
        bool b2DAttackSound = false;
        uint32_t iflags = 0;
        bool bin1stPerson = !pPlayer->IsActually3rdPerson();
        bool bUseModSound = false;
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
        uint8_t ucMaxN = 0;
        if (pPlayer->GetParentCell() &&
            pPlayer->GetParentCell()->GetInterior() &&
            !pPlayer->GetParentCell()->GetFakeExterior()) {
            fModNV = gs_fWeaponInteriorNearVolumeMod.Float();
            fModFV = gs_fWeaponInteriorFarVolumeMod.Float();
            fModNA = gs_fWeaponInteriorNearAttenuationMod.Float();
            fModFA = gs_fWeaponInteriorFarAttenuationMod.Float();
        }

        if (abPlayer) {
            iflags |= BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE;
            if (pPlayer->pCurrentProcess) {
                if (pPlayer->pCurrentProcess->GetCurrentWeapon()) {
                    ItemChange* pweap = pPlayer->pCurrentProcess->GetCurrentWeapon();
                    if (pweap && (pweap->HasModEffectActive(11) || pweap->HasModEffectActive(16))) {
                        bUseModSound = true;
                    }
                }
            }
        }
        if (apWeapon) {
            if (abPlayer && apWeapon->GetAttackSound2D(false)) {
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
                BSAudio* pAudio = &BSAudio::QInstance();
                NiPoint3 distanceVector = origin - pAudio->GetListenerPosition();
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
                    else if (pfurthest) {
                        pcurrImpactHandle = pfurthest;
                        pfurthest->uiIdentifier = static_cast<Actor*>(apSource);
                        pfurthest->uiTimestamp = BSGetTickCount();
                        pfurthest->fRelativeDistance = fdistanceSqr;
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
                        const char* pcFilename = psound->GetFilename();
                        pAudio = &BSAudio::QInstance();
                        pcurrImpactHandle->soundNear = pAudio->GetSoundHandleByFilename(pcFilename, iflags, psound);
                        ucMinN = psound->GetMinDistance();
                        ucMaxN = psound->GetMaxDistance();
                    }
                    if (psound2) {
                        pcurrImpactHandle->soundFar.SetVolume(0.0f);
                        pcurrImpactHandle->soundFar.Stop();
                        const char* pcFilename = psound2->GetFilename();
                        pAudio = &BSAudio::QInstance();
                        pcurrImpactHandle->soundFar = pAudio->GetSoundHandleByFilename(pcFilename, iflags, psound2);
                    }
                    if (abPlayer || !aOriginNode) {
                        if (apSource->Get3D() && (!abPlayer || !bin1stPerson)) {
                            NiNode* p3D = apSource->Get3D();
                            pcurrImpactHandle->soundNear.SetObjectToFollow(p3D);
                            p3D = apSource->Get3D();
                            pcurrImpactHandle->soundFar.SetObjectToFollow(p3D);
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
                        float fMax = static_cast<double>(ucMaxN) * fModNA * 100.0;
                        float fMin = static_cast<double>(ucMinN) * 5.0;
                        pcurrImpactHandle->soundNear.SetMinMax(fMin, fMax);
                        fMax = static_cast<double>(ucMaxN) * fModNA * 100.0;
                        fMin = static_cast<double>(ucMinN) * 5.0;
                        pcurrImpactHandle->soundFar.SetMinMax(fMin, fMax);
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
                    apDest,
                    259u,
                    "FST%s%s",
                    ImpactMixer::GetMaterialSoundName(iMat),
                    ImpactMixer::pStepSoundNames[i]
                );
                ImpactMixer::pFootSteps[iMat][i] = pDataHandler->GetSound(apDest);
                if (!ImpactMixer::pFootSteps[iMat][i] && ImpactMixer::uiReplacementMaterials[iMat] != 13) {
                    BSsprintf(
                        apDest,
                        259u,
                        "FST%s%s",
                        ImpactMixer::GetMaterialSoundName(ImpactMixer::uiReplacementMaterials[iMat]),
                        ImpactMixer::pStepSoundNames[i]
                    );
                    ImpactMixer::pFootSteps[iMat][i] = pDataHandler->GetSound(apDest);
                    if (!ImpactMixer::pFootSteps[iMat][i] &&
                        ImpactMixer::uiReplacementMaterials[ImpactMixer::uiReplacementMaterials[iMat]] != 13) {
                        BSsprintf(
                            apDest,
                            259u,
                            "FST%s%s",
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
        for (WeaponNearFarSound& rHandle : ImpactMixer::GunshotHandles) {
            rHandle.uiIdentifier = nullptr;
            rHandle.uiTimestamp = 0;
            rHandle.fRelativeDistance = std::numeric_limits<float>::max();
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
        MEM_CONTEXT_AUDIO,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Audio\\ImpactMixer.cpp",
        271
    );
    pContext->~AutoMemContext();
}

void ImpactMixer::PlayCollisionSound(ImpactMixer::COLLISION_SOUND& arDataIn) {
    CFilter kFilter2;
    CFilter kFilter1;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_AUDIO,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Audio\\ImpactMixer.cpp",
        564
    );
    if (arDataIn.pBody1 == arDataIn.pBody2 && arDataIn.pBody1) {
        return;
    }
    static BSCriticalSection kcriticalSection;
    kcriticalSection.Enter("ImpactMixer::PlayCollisionSound");
    uint32_t ibattleFlag = 0;
    float fmass1 = arDataIn.pBody1 ? arDataIn.pBody1->GetDynamicMass() : 5.0f;
    float fmass2 = arDataIn.pBody2 ? arDataIn.pBody2->GetDynamicMass() : 5.0f;
    bool bshellCasing = false;
    TESSound* info = nullptr;
    TESSound* info2 = nullptr;
    if ((arDataIn.pBody1 && arDataIn.pBody1->GetCollisionFilter(&kFilter1)->GetLayer() == L_SHELLCASING) ||
        (arDataIn.pBody2 && arDataIn.pBody2->GetCollisionFilter(&kFilter2)->GetLayer() == L_SHELLCASING)) {
        bshellCasing = true;
    }
    if (bshellCasing) {
        ibattleFlag = BSGameSound::SOUND_FLAG_BATTLE_SOUND;
        float fcasingMass = fmass2 < static_cast<double>(fmass1) ? fmass1 : fmass2;
        if (fcasingMass < 1.0) {
            info = pDataHandler->GetSound("PHYCasingShotgun");
        }
        else if (FloatEQ(fcasingMass, 2.0f, 0.25f)) {
            info = pDataHandler->GetSound("PHYCasingPistol");
        }
        else {
            info = pDataHandler->GetSound("PHYCasingRifle");
        }
        info2 = nullptr;
    }
    else {
        info = ImpactMixer::GetSoundForType(arDataIn.cMaterialType1, fmass1, arDataIn.fVelocity);
        info2 = ImpactMixer::GetSoundForType(arDataIn.cMaterialType2, fmass2, arDataIn.fVelocity);
    }
    if (FalloutAudio::SoundDistanceTest(info, arDataIn.Location, 1.0f) ||
        FalloutAudio::SoundDistanceTest(info2, arDataIn.Location, 1.0f)) {
        BSSoundHandle SoundHand;
        BSSoundHandle SoundHand2;
        uint32_t uiCurrentTime = appTimer.iLastTime;
        uint32_t uiSinceLastPlayed = 0;
        if (ImpactMixer::pImpactMap.GetAt(arDataIn.iID, uiSinceLastPlayed) &&
            uiCurrentTime - uiSinceLastPlayed < ImpactMixer::iCollisionSoundTimeDelta.Int()) {
            kcriticalSection.Leave();
        }
        else {
            ImpactMixer::pImpactMap.SetAt(arDataIn.iID, uiCurrentTime);
            if (info &&
                (arDataIn.cMaterialType1 != BHK_MATERIAL_SKIN || arDataIn.cMaterialType2 != BHK_MATERIAL_SKIN)) {
                uint32_t uiFlags = ibattleFlag |
                    (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT);
                uint32_t uiFormID = info->iFormID;
                SoundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiFormID, uiFlags);
            }

            if (info2 &&
                (arDataIn.cMaterialType1 != BHK_MATERIAL_SKIN || arDataIn.cMaterialType2 != BHK_MATERIAL_SKIN)) {
                uint32_t uiFlags = ibattleFlag |
                    (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT);
                uint32_t uiFormID = info2->iFormID;
                SoundHand2 = BSAudio::QInstance().GetSoundHandleByNumericID(uiFormID, uiFlags);
            }

            double dVelRatio = arDataIn.fVelocity / 400.0 > 1.0 ? 1.0 : arDataIn.fVelocity / 400.0;
            uint32_t velVol = static_cast<int64_t>((1.0 - dVelRatio) * 3000.0);
            double dFreq = velVol >= 1500 ? (static_cast<double>(velVol) - 4500.0) / -3000.0 : 1.0;
            float fModFrequency = dFreq;
            if (info) {
                SoundHand.SetPriority(255u);
                SoundHand.SetPosition(arDataIn.Location.x, arDataIn.Location.y, arDataIn.Location.z);
                SoundHand.SetStaticAttenuation(velVol);
                SoundHand.SetModFrequency(fModFrequency);
                SoundHand.Play(false);
            }
            if (info2) {
                SoundHand2.SetPriority(255u);
                SoundHand2.SetPosition(arDataIn.Location.x, arDataIn.Location.y, arDataIn.Location.z);
                SoundHand2.SetStaticAttenuation(velVol);
                SoundHand2.SetModFrequency(fModFrequency);
                SoundHand2.Play(false);
            }
            ImpactMixer::lastloc = arDataIn.Location;
            int32_t itimeOut = 4 * ImpactMixer::iCollisionSoundTimeDelta.Int();
            NiTMapItem<int32_t, uint32_t>* pos = ImpactMixer::pImpactMap.GetFirstPos();
            int32_t itime = appTimer.iLastTime;
            uint32_t iloopcount = 0;
            while (pos) {
                int32_t key;
                uint32_t uiLastPlayed;
                ++iloopcount;
                ImpactMixer::pImpactMap.GetNext(pos, key, uiLastPlayed);
                // The time difference is compared signed.
                if (static_cast<int32_t>(itime - uiLastPlayed) > itimeOut) {
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

bool ImpactMixer::GetGunshotsInitialized() {
    return (ImpactMixer::iiInitializationFlags & GUNSHOTS_INITIALIZED) != 0;
}

void ImpactMixer::SetGunshotsInitialized(bool abIn) {
    if (abIn) {
        ImpactMixer::iiInitializationFlags |= GUNSHOTS_INITIALIZED;
    }
    else {
        ImpactMixer::iiInitializationFlags &= ~GUNSHOTS_INITIALIZED;
    }
}

void ImpactMixer::SetAltMaterialSoundName(int32_t aiMat, const char* apAltName) {
    ImpactMixer::pAltMaterialSoundNames[aiMat] = apAltName;
}

void ImpactMixer::PlayFootstep(TESObjectREFR* apRef, int32_t aiFootType) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_AUDIO,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Audio\\ImpactMixer.cpp",
        735
    );
    if (Interface::IsInMenuMode()) {
        return;
    }
    NiPoint3 loc = apRef->GetLocationOnReference();
    BSSoundHandle SoundHand;
    BSSoundHandle SoundHand2;
    TESSound* infoGround = nullptr;
    TESSound* infoShoe = nullptr;
    bool b1stPersonPlayerFoot = false;
    float ftestDistance = gs_fMaxFootstepDistance_Audio.Float();
    if (Main::GetSceneIsComplex()) {
        ftestDistance = ftestDistance / 2.0;
    }
    float fdistance = 0.0f;
    if (BSAudio::QInstance().pListener) {
        BSAudioListener* pListener = BSAudio::QInstance().pListener;
        NiPoint3 listenPt = pListener->GetPosition();
        listenPt -= loc;
        fdistance = listenPt.Length();
        double dDistance = listenPt.Length();
        if (ftestDistance < dDistance) {
            return;
        }
    }
    if (apRef->IsActor()) {
        MobileObject* pmob = dynamic_cast<Actor*>(apRef);
        if (pmob->IsDead(false)) {
            return;
        }
        if (pmob->IsCreature()) {
            TESBoundObject* pobj = apRef->GetObjectReference();
            TESCreature* pcreature = dynamic_cast<TESCreature*>(pobj);
            infoGround = pcreature->PickCreatureSound(static_cast<CREATURE_SOUND_TYPE>(aiFootType));
            if (infoGround) {
                uint8_t ucMaxDistance = infoGround->GetMaxDistance();
                if (fdistance <= static_cast<double>(100 * ucMaxDistance)) {
                    uint32_t uiFormID = infoGround->iFormID;
                    SoundHand = BSAudio::QInstance().GetSoundHandleByNumericID(
                        uiFormID,
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
                TESWorldSpace* pWorldSpace = TES::pInstance->pWorldSpace;
                pcell = pDataHandler->GetCellFromWorldCoord(loc.x, loc.y, pWorldSpace, false);
            }
            bool bWaterStep = false;
            bool bStairs = false;
            if (!static_cast<Actor*>(pmob)->Actor::IsSwimming() && pcell) {
                double dWaterHeight = pmob->GetRelevantWaterHeight();
                if (loc.z < dWaterHeight && ((pcell->GetInterior() && pcell->HasWater()) || !pcell->GetInterior())) {
                    bWaterStep = true;
                }
            }

            bhkCharacterController* pCharController = pmob->GetCharController();
            if (static_cast<uint8_t>(pCharController->GetOnStairs())) {
                bStairs = true;
            }
            if (!TES::pInstance->pInteriorCell && pcell && pcell->GetLand()) {
                COORD_DATA cdata;
                TESObjectLAND* pLand = pcell->GetLand();
                pLand->GetCoordData(&cdata, loc, true);
                uint16_t usVertex = cdata.iVertidx;
                uint8_t ucBlock = cdata.iBlock;
                pLand = pcell->GetLand();
                TESLandTexture* plandTex = pLand->GetMainTexture(ucBlock, usVertex);
                float fheight = 0.0f;
                groundType = BHK_MATERIAL_STONE;
                if (bWaterStep) {
                    groundType = BHK_MATERIAL_WATER;
                }
                else if (
                    pmob && pmob->GetCharController() && pmob->GetCharController()->eSupportMaterial != BHK_MATERIAL_MAX
                ) {
                    groundType = pmob->GetCharController()->eSupportMaterial;
                }
                else if (plandTex) {
                    groundType = plandTex->GetHavokMaterialType();
                    if (pcell->GetLandHeight(loc, fheight)) {
                        loc.z = fheight;
                    }
                }
            }
            else if (bWaterStep) {
                groundType = BHK_MATERIAL_WATER;
            }
            else if (pmob && pmob->GetCharController()) {
                groundType = pmob->GetCharController()->eSupportMaterial;
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
            else if (bStairs) {
                stepType = 7 - (aiFootType != 0);
            }
            else if (static_cast<Actor*>(pmob)->Actor::IsRunning()) {
                stepType = 3 - (aiFootType != 0);
            }
            matType = ImpactMixer::GetFootstepIndexForType(groundType);
            infoGround = ImpactMixer::pFootSteps[matType][stepType];
            if (static_cast<Actor*>(pmob)->Actor::IsPowerBodyArmorWorn()) {
                if (!ImpactMixer::pPowerArmorStepL || !ImpactMixer::pPowerArmorStepR) {
                    ImpactMixer::pPowerArmorStepR = pDataHandler->GetSound("FSTArmorPowerR");
                    ImpactMixer::pPowerArmorStepL = pDataHandler->GetSound("FSTArmorPowerL");
                }
                infoShoe = aiFootType ? ImpactMixer::pPowerArmorStepL : ImpactMixer::pPowerArmorStepR;
            }
            else if (static_cast<Actor*>(pmob)->Actor::IsHeavyBodyArmorWorn()) {
                if (!ImpactMixer::pHeavyArmorStepL || !ImpactMixer::pHeavyArmorStepR) {
                    ImpactMixer::pHeavyArmorStepR = pDataHandler->GetSound("FSTArmorR");
                    ImpactMixer::pHeavyArmorStepL = pDataHandler->GetSound("FSTArmorL");
                }
                infoShoe = aiFootType ? ImpactMixer::pHeavyArmorStepL : ImpactMixer::pHeavyArmorStepR;
            }
            if (apRef == pPlayer && !pPlayer->IsActually3rdPerson()) {
                b1stPersonPlayerFoot = true;
            }
            if (b1stPersonPlayerFoot) {
                uint32_t iflags = BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT;
                if (infoGround) {
                    const char* pcFilename = infoGround->GetFilename();
                    SoundHand = BSAudio::QInstance().GetSoundHandleByFilename(pcFilename, iflags, infoGround);
                    SoundHand.SetVolume(gs_fPlayerFootVolume_Audio.Float());
                    SoundHand.Play(false);
                }
                if (infoShoe) {
                    const char* pcFilename = infoShoe->GetFilename();
                    SoundHand2 = BSAudio::QInstance().GetSoundHandleByFilename(pcFilename, iflags, infoShoe);
                    SoundHand2.SetVolume(gs_fPlayerFootVolume_Audio.Float());
                    SoundHand2.Play(false);
                }
            }
            else {
                uint32_t iflags = BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_FOOTSTEPS |
                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT;
                if (infoGround) {
                    uint8_t ucMaxDistance = infoGround->GetMaxDistance();
                    if (fdistance <= static_cast<double>(100 * ucMaxDistance)) {
                        const char* pcFilename = infoGround->GetFilename();
                        SoundHand = BSAudio::QInstance().GetSoundHandleByFilename(pcFilename, iflags, infoGround);
                        float fZ = loc.z - 128.0;
                        SoundHand.SetPosition(loc.x, loc.y, fZ);
                        if (apRef->Get3D()) {
                            NiNode* p3D = apRef->Get3D();
                            SoundHand.SetObjectToFollow(p3D);
                        }
                        float fVolume = RandomFloat(0.9f, 1.0f);
                        SoundHand.SetVolume(fVolume);
                        SoundHand.Play(false);
                    }
                }
                if (infoShoe) {
                    uint8_t ucMaxDistance = infoShoe->GetMaxDistance();
                    if (fdistance <= static_cast<double>(100 * ucMaxDistance)) {
                        const char* pcFilename = infoShoe->GetFilename();
                        SoundHand2 = BSAudio::QInstance().GetSoundHandleByFilename(pcFilename, iflags, infoShoe);
                        float fZ = loc.z - 128.0;
                        SoundHand2.SetPosition(loc.x, loc.y, fZ);
                        if (apRef->Get3D()) {
                            NiNode* p3D = apRef->Get3D();
                            SoundHand2.SetObjectToFollow(p3D);
                        }
                        float fVolume = 1.0 - RandomFloat(0.0f, 0.1f);
                        SoundHand2.SetVolume(fVolume);
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
        TESSound* pSound = apDefWeapon->GetBlockSound();
        TESSound* pBlockSound = apDefWeapon->GetBlockSound();
        const char* pcFilename = pBlockSound->GetFilename();
        SoundHand = BSAudio::QInstance().GetSoundHandleByFilename(pcFilename, 386u, pSound);
    }
    else {
        int32_t idefWeaponType = apDefWeapon ? apDefWeapon->GetWeaponType() : -1;
        if (idefWeaponType == -1) {
            SoundHand = BSAudio::QInstance().GetSoundHandleByName("WPNBlockHand", 386u);
        }
        else {
            SoundHand = BSAudio::QInstance().GetSoundHandleByName("WPNBlockMelee", 386u);
        }
    }
    double dMinX = BSMIN(aAttacker.x, aDefender.x);
    double dMaxX = BSMAX(aAttacker.x, aDefender.x);
    float fX = (dMaxX - BSMIN(aAttacker.x, aDefender.x)) / 2.0 + dMinX;
    double dMinY = BSMIN(aAttacker.y, aDefender.y);
    double dMaxY = BSMAX(aAttacker.y, aDefender.y);
    float fY = (dMaxY - BSMIN(aAttacker.y, aDefender.y)) / 2.0 + dMinY;
    double dMinZ = BSMIN(aAttacker.z, aDefender.z);
    double dMaxZ = BSMAX(aAttacker.z, aDefender.z);
    float fZ = (dMaxZ - BSMIN(aAttacker.z, aDefender.z)) / 2.0 + dMinZ;
    SoundHand.SetPosition(fX, fY, fZ);
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
                info = pDataHandler->GetSound("CWoodSmall");
            }
            else if (afMass < 15.0) {
                info = pDataHandler->GetSound("CWoodMedium");
            }
            else {
                info = pDataHandler->GetSound("CWoodLarge");
            }
            break;
        }
        case 9: {
            if (!(afMass < 9.999999747378752e-05)) {
                if (!(gs_fWoodMediumMassMin_Audio.Float() > static_cast<double>(afMass))) {
                    if (!(gs_fWoodLargeMassMin_Audio.Float() > static_cast<double>(afMass))) {
                        info = pDataHandler->GetSound("CWoodLarge");
                    }
                    else {
                        info = pDataHandler->GetSound("CWoodMedium");
                    }
                }
                else {
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
        MEM_CONTEXT_AUDIO,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Audio\\ImpactMixer.cpp",
        1699
    );
    NiPoint3 loc = apRef->GetLocationOnReference();
    Actor* pactor = dynamic_cast<Actor*>(apRef);
    if (pactor && pactor->pCurrentProcess) {
        BSSoundHandle SoundHand;
        ItemChange* pweapdata = pactor->pCurrentProcess->GetCurrentWeapon();
        TESBoundObject* pbobj = pweapdata ? pweapdata->pContainerObj : nullptr;
        TESObjectWEAP* pweap = nullptr;
        if (pbobj) {
            pweap = dynamic_cast<TESObjectWEAP*>(pbobj);
        }
        if (pweap) {
            TESSound* pactionSound = nullptr;
            if (aiType == 9) {
                pactionSound = pweap->GetEquipSound();
            }
            else if (aiType == 10) {
                pactionSound = pweap->GetUnequipSound();
            }
            if (!pactionSound || !FalloutAudio::SoundDistanceTest(pactionSound, loc, 1.0f)) {
                return;
            }
            bool bSneaker = pactor->IsSneaking();
            if (pactor == pPlayer) {
                const char* pcFilename = pactionSound->GetFilename();
                SoundHand = BSAudio::QInstance().GetSoundHandleByFilename(
                    pcFilename,
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT |
                        BSGameSound::SOUND_FLAG_UNKBIT31,
                    pactionSound
                );
            }
            else {
                const char* pcFilename = pactionSound->GetFilename();
                SoundHand = BSAudio::QInstance().GetSoundHandleByFilename(
                    pcFilename,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_IMPACT,
                    pactionSound
                );
                SoundHand.SetPosition(loc.x, loc.y, loc.z);
                NiNode* p3D = apRef->Get3D();
                SoundHand.SetObjectToFollow(p3D);
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
