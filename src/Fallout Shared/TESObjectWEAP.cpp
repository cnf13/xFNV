#include "Fallout Shared/TESObjectWEAP.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/BSTestObjects.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSAmmoForm.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSClipRoundsForm.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSRepairItemList.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageDataUseWeapon.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAmmoEffect.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESAttackDamageForm.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectIMOD.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout Shared/TempEffects/BSTempEffectDebris.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

// Fire's guarded static: the scope offset, a function-local static built on first use.
static NiPoint3 offset{};

// The guard of offset (bit 0).
static uint32_t uiOffsetGuard;

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> bLaserSights("bLaserSights:Combat", false);
SettingT<INISettingCollection> bAimSights("bAimSights:Combat", false);
SettingT<INISettingCollection> bForceNPCsUseAmmo("bForceNPCsUseAmmo:Combat", false);

// Reads the WEAP record's chunks; only a WEAP record (type 0x28) is accepted. When a chunk is empty (malformed data),
// the EDID, NNAM and VANM buffers are initialised to "" (one zeroed byte, allocated only for an empty chunk) and the
// VNAM local to 0.
bool TESObjectWEAP::Load(TESFile* apFile) {
    if ((uint8_t)apFile->GetTESForm() != WEAP_ID) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                TESForm* pForm = this;
                pForm->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: {
                TESBoundObject* pObject = this;
                pObject->LoadObjectBound(apFile);
                break;
            }
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case MODL_ID:
            case MODD_ID:
            case MODT_ID:
            case MOSD_ID: TESModel::LoadModelChunk(static_cast<TESModelTextureSwap*>(this), apFile); break;
            case MODS_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                apFile->GetChunkData(pBuffer, uiChunkSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(
                    pBuffer, uiChunkSize, usFormVersion
                );
                break;
            }
            case ICON_ID: TESTexture::LoadTextureChunk(static_cast<TESIcon*>(this), apFile); break;
            case MICO_ID: BGSMessageIcon::LoadMessageIcon(static_cast<BGSMessageIcon*>(this), apFile); break;
            case SCRI_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                TESScriptableForm* pScriptable = static_cast<TESScriptableForm*>(this);
                pScriptable->SetFormScript((Script*)uiFormID);
                pScriptable->InitItem(this);
                break;
            }
            case EITM_ID:
            case ENAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                static_cast<TESEnchantableForm*>(this)->SetFormEnchanting((EnchantmentItem*)uiFormID);
                break;
            }
            case ANAM_ID:
            case EAMT_ID: {
                int16_t sAmount = 0;
                apFile->LoadWord(&sAmount);
                static_cast<TESEnchantableForm*>(this)->iAmountofEnchantment = (uint16_t)sAmount;
                break;
            }
            case NAM0_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                // Stores the form ID straight into the ammo pointer; InitItem resolves it.
                static_cast<BGSAmmoForm*>(this)->pFormAmmo =
                    reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID));
                break;
            }
            case DEST_ID:
            case DSTD_ID:
                BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
                break;
            case REPL_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                // The form ID is stored straight into the list pointer; InitItem resolves it.
                static_cast<BGSRepairItemList*>(this)->pListForm =
                    reinterpret_cast<BGSListForm*>(static_cast<uintptr_t>(uiFormID));
                break;
            }
            case ETYP_ID: {
                uint32_t uiEquipType = 0;
                apFile->GetChunkData<uint32_t>(uiEquipType);
                static_cast<BGSEquipType*>(this)->SetEquipType(static_cast<EQUIP_TYPE>(uiEquipType));
                break;
            }
            case BIPL_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                static_cast<BGSBipedModelList*>(this)->SetBipedModelList((BGSListForm*)uiFormID);
                break;
            }
            case YNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                static_cast<BGSPickupPutdownSounds*>(this)->SetPickupSound((TESSound*)uiFormID);
                break;
            }
            case ZNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                static_cast<BGSPickupPutdownSounds*>(this)->SetPutdownSound((TESSound*)uiFormID);
                break;
            }
            case DATA_ID: LoadData(apFile, nullptr, 0); break;
            case DNAM_ID: {
                if (apFile->GetFormVersion() >= 12) {
                    if (apFile->GetFormVersion() >= 14) {
                        ourMemset(&data, 0, sizeof(data));
                        data.fSemiAutomaticFireDelayMin = 0.1f;
                        data.fSemiAutomaticFireDelayMax = 0.5f;
                        apFile->GetChunkData(&data, 0xCC);
#if XFNV_64BIT
                        // The file's record is 0xCC bytes, the projectile's 4-byte form ID at 0x24.
                        TESFile::WidenFormIDRecords(
                            &data, 1, apFile->GetChunkSize() < 0xCC ? apFile->GetChunkSize() : 0xCC, sizeof(data),
                            {{0x24, offsetof(OBJ_WEAP, pProjectile)}}
                        );
#endif
                        if (apFile->QEndian()) {
                            data.Endian();
                        }
                        if (data.fLimbDamageMult <= 0.0) {
                            data.fLimbDamageMult = 1.0f;
                        }
                        uint8_t ucHandGrip = data.cHandGripAnim;
                        if ((ucHandGrip < 0xE6) || ((ucHandGrip > 0xEB) && (ucHandGrip != 0xFF))) {
                            data.cHandGripAnim = 0xFF;
                        }
                        if (!ActorValue::IsSkill(data.eSkill)) {
                            data.eSkill = ActorValue::UNARMED;
                        }
                        if (!ActorValue::IsResistance(data.eResistance) &&
                            (data.eResistance != static_cast<ActorValue::Index>(-1))) {
                            data.eResistance = static_cast<ActorValue::Index>(-1);
                        }
                    }
                    else {
#if XFNV_64BIT
                        ourMemset(&data, 0, sizeof(data));
#else
                        ourMemset(&data, 0, sizeof(data));
#endif
                        data.fSemiAutomaticFireDelayMin = 0.1f;
                        data.fSemiAutomaticFireDelayMax = 0.5f;
                        uint32_t uiChunkSize = apFile->GetChunkSize();
                        apFile->GetChunkData(&data, uiChunkSize);
#if XFNV_64BIT
                        // The file's record is at most 0xCC bytes, the projectile's 4-byte form ID at 0x24.
                        TESFile::WidenFormIDRecords(
                            &data, 1, uiChunkSize < 0xCC ? uiChunkSize : 0xCC, sizeof(data),
                            {{0x24, offsetof(OBJ_WEAP, pProjectile)}}
                        );
#endif
                        if (apFile->QEndian()) {
                            data.Endian();
                        }
                        if ((data.cAttackAnim < 0xA8) && (data.cAttackAnim >= 0x56)) {
                            data.cAttackAnim += 6;
                        }
                    }
                }
                else {
#if XFNV_64BIT
                    ourMemset(&data, 0, sizeof(data));
#else
                    ourMemset(&data, 0, sizeof(data));
#endif
                    data.fSemiAutomaticFireDelayMin = 0.1f;
                    data.fSemiAutomaticFireDelayMax = 0.5f;
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    apFile->GetChunkData(&data, uiChunkSize);
#if XFNV_64BIT
                    // The file's record is at most 0xCC bytes, the projectile's 4-byte form ID at 0x24.
                    TESFile::WidenFormIDRecords(
                        &data, 1, uiChunkSize < 0xCC ? uiChunkSize : 0xCC, sizeof(data),
                        {{0x24, offsetof(OBJ_WEAP, pProjectile)}}
                    );
#endif
                    if (apFile->QEndian()) {
                        data.Endian();
                    }
                    if ((data.cAttackAnim < 0xA8) && (data.cAttackAnim >= 0x38)) {
                        data.cAttackAnim += 0x12;
                        if ((data.cAttackAnim < 0xA8) && (data.cAttackAnim >= 0x56)) {
                            data.cAttackAnim += 6;
                        }
                    }
                }
                break;
            }
            case WNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                Set1stPersonObject((TESObjectSTAT*)uiFormID);
                break;
            }
            case MOD2_ID:
            case NAM1_ID: {
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                    apFile->GetChunkData(pBuffer, 0);
                    TESModel* pModel = &ShellCasingModel;
                    pModel->SetModel(pBuffer);
                }
                else {
                    TESModel* pModel = &ShellCasingModel;
                    pModel->SetModel(nullptr);
                }
                break;
            }
            case MO2T_ID: TESModel::LoadModelTextureChunk(&ShellCasingModel, apFile); break;
            case MOD3_ID: {
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                    apFile->GetChunkData(pBuffer, 0);
                    TESModel* pModel = &ScopeModel;
                    pModel->SetModel(pBuffer);
                }
                else {
                    TESModel* pModel = &ScopeModel;
                    pModel->SetModel(nullptr);
                }
                break;
            }
            case MO3T_ID: TESModel::LoadModelTextureChunk(&ScopeModel, apFile); break;
            case MOD4_ID: {
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                    apFile->GetChunkData(pBuffer, 0);
                    TESModel* pModel = &WorldModel;
                    pModel->SetModel(pBuffer);
                }
                else {
                    TESModel* pModel = &WorldModel;
                    pModel->SetModel(nullptr);
                }
                break;
            }
            case MO4T_ID: TESModel::LoadModelTextureChunk(&WorldModel, apFile); break;
            case MWD1_ID: {
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                    apFile->GetChunkData(pBuffer, 0);
                    TESModel* pModel = &WorldModelMod1;
                    pModel->SetModel(pBuffer);
                }
                else {
                    TESModel* pModel = &WorldModelMod1;
                    pModel->SetModel(nullptr);
                }
                break;
            }
            case MWT1_ID: TESModel::LoadModelTextureChunk(&WorldModelMod1, apFile); break;
            case MWD2_ID: {
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                    apFile->GetChunkData(pBuffer, 0);
                    TESModel* pModel = &WorldModelMod2;
                    pModel->SetModel(pBuffer);
                }
                else {
                    TESModel* pModel = &WorldModelMod2;
                    pModel->SetModel(nullptr);
                }
                break;
            }
            case MWT2_ID: TESModel::LoadModelTextureChunk(&WorldModelMod2, apFile); break;
            case MWD3_ID: {
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                    apFile->GetChunkData(pBuffer, 0);
                    TESModel* pModel = &WorldModelMod12;
                    pModel->SetModel(pBuffer);
                }
                else {
                    TESModel* pModel = &WorldModelMod12;
                    pModel->SetModel(nullptr);
                }
                break;
            }
            case MWT3_ID: TESModel::LoadModelTextureChunk(&WorldModelMod12, apFile); break;
            case MWD4_ID: {
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                    apFile->GetChunkData(pBuffer, 0);
                    TESModel* pModel = &WorldModelMod3;
                    pModel->SetModel(pBuffer);
                }
                else {
                    TESModel* pModel = &WorldModelMod3;
                    pModel->SetModel(nullptr);
                }
                break;
            }
            case MWT4_ID: TESModel::LoadModelTextureChunk(&WorldModelMod3, apFile); break;
            case MWD5_ID: {
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                    apFile->GetChunkData(pBuffer, 0);
                    TESModel* pModel = &WorldModelMod13;
                    pModel->SetModel(pBuffer);
                }
                else {
                    TESModel* pModel = &WorldModelMod13;
                    pModel->SetModel(nullptr);
                }
                break;
            }
            case MWT5_ID: TESModel::LoadModelTextureChunk(&WorldModelMod13, apFile); break;
            case MWD6_ID: {
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                    apFile->GetChunkData(pBuffer, 0);
                    TESModel* pModel = &WorldModelMod23;
                    pModel->SetModel(pBuffer);
                }
                else {
                    TESModel* pModel = &WorldModelMod23;
                    pModel->SetModel(nullptr);
                }
                break;
            }
            case MWT6_ID: TESModel::LoadModelTextureChunk(&WorldModelMod23, apFile); break;
            case MWD7_ID: {
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                    apFile->GetChunkData(pBuffer, 0);
                    TESModel* pModel = &WorldModelMod123;
                    pModel->SetModel(pBuffer);
                }
                else {
                    TESModel* pModel = &WorldModelMod123;
                    pModel->SetModel(nullptr);
                }
                break;
            }
            case MWT7_ID: TESModel::LoadModelTextureChunk(&WorldModelMod123, apFile); break;
            case WMI1_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetItemMod(MOD_ONE, (TESObjectIMOD*)uiFormID);
                break;
            }
            case WMI2_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetItemMod(MOD_TWO, (TESObjectIMOD*)uiFormID);
                break;
            }
            case WMI3_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetItemMod(MOD_THREE, (TESObjectIMOD*)uiFormID);
                break;
            }
            case WNM1_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                Set1stPersonModObject(1, (TESObjectSTAT*)uiFormID);
                break;
            }
            case WNM2_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                Set1stPersonModObject(2, (TESObjectSTAT*)uiFormID);
                break;
            }
            case WNM3_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                Set1stPersonModObject(3, (TESObjectSTAT*)uiFormID);
                break;
            }
            case WNM4_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                Set1stPersonModObject(4, (TESObjectSTAT*)uiFormID);
                break;
            }
            case WNM5_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                Set1stPersonModObject(5, (TESObjectSTAT*)uiFormID);
                break;
            }
            case WNM6_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                Set1stPersonModObject(6, (TESObjectSTAT*)uiFormID);
                break;
            }
            case WNM7_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                Set1stPersonModObject(7, (TESObjectSTAT*)uiFormID);
                break;
            }
            case WMS1_ID: {
                uint32_t uiValue = 0;
                apFile->GetChunkData<uint32_t>(uiValue);
                if (pModSilencedSound) {
                    pModSilencedSound2 = (TESSound*)uiValue;
                }
                else {
                    SetModAttackSound((TESSound*)uiValue);
                }
                break;
            }
            case WMS2_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                pModSilencedSound2D = (TESSound*)uiFormID;
                break;
            }
            case INAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetImpactDataSet((BGSImpactDataSet*)uiFormID);
                break;
            }
            case NNAM_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                EmbeddedWeaponNode.Set(pBuffer, 0);
                break;
            }
            case SNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                if (GetAttackSound(false)) {
                    SetAttackSound2((TESSound*)uiFormID);
                }
                else {
                    SetAttackSound((TESSound*)uiFormID);
                }
                break;
            }
            case TNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetAttackFailSound((TESSound*)uiFormID);
                break;
            }
            case UNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetIdleSound((TESSound*)uiFormID);
                break;
            }
            case VNAM_ID: {
                uint32_t uiSoundLevel = 0;
                apFile->GetChunkData<uint32_t>(uiSoundLevel);
                eSoundLevel = (uiSoundLevel <= 3) ? static_cast<SOUND_LEVEL>(uiSoundLevel) : SOUND_LEVEL_NORMAL;
                break;
            }
            case XNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                pAttackSound2D = (TESSound*)uiFormID;
                break;
            }
            case CRDT_ID: apFile->GetChunkData(&criticalData, 0x10);
#if XFNV_64BIT
                // The file's record is 0x10 bytes, the effect's 4-byte form ID at 0xC.
                TESFile::WidenFormIDRecords(
                    &criticalData, 1, 0x10, sizeof(criticalData), {{0xC, offsetof(OBJ_WEAP_CRITICAL, pEffect)}}
                );
#endif
                if (apFile->QEndian()) {
                    criticalData.Endian();
                }
                break;
            case VATS_ID: apFile->GetChunkData(&specialVATSData, 0x14);
#if XFNV_64BIT
                // The file's record is 0x14 bytes, the effect's 4-byte form ID first.
                TESFile::WidenFormIDRecords(
                    &specialVATSData, 1, 0x14, sizeof(specialVATSData),
                    {{0, offsetof(OBJ_WEAP_VATS_SPECIAL, pVATSSpecialEffect)}}
                );
#endif
                if (apFile->QEndian()) {
                    specialVATSData.Endian();
                }
                break;
            case EFSD_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetScopeEffect((TESEffectShader*)uiFormID);
                break;
            }
            case VANM_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                VATSAttackName.Set(pBuffer, 0);
                break;
            }
            case NAM6_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetMeleeBlockSound((TESSound*)uiFormID);
                break;
            }
            case NAM7_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetAttackLoopSound((TESSound*)uiFormID);
                break;
            }
            case NAM8_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetUnEquipSound((TESSound*)uiFormID);
                break;
            }
            case NAM9_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                SetEquipSound((TESSound*)uiFormID);
                break;
            }
            default: break;
        }
        if (!apFile->NextChunk()) {
            break;
        }
    }
    if ((!GetNumProjectiles(false, true, nullptr)) && (GetProjectile(nullptr))) {
        SetNumProjectiles(true);
    }

    if (data.eType >= 14) {
        // GetFormID is called before GetFormEditorID, as the game evaluates the arguments right to left.
        uint32_t uiFormID = GetFormID();
        TESForm* pForm = this;
        const char* pEditorID = pForm->GetFormEditorID();
        Warning("MASTERFILE: Clearing invalid type on weapon '%s' (%08X).", pEditorID, uiFormID);
        data.eType = 0;
    }
    return true;
}

// Swaps the fields read from a big-endian file. The condition level, the projectile and the power attack animation are
// left alone, and the skill requirement is swapped right after the strength requirement.
void OBJ_WEAP::Endian() {
    ENDIAN(fSpeed, false);
    ENDIAN(fReach, false);
    ENDIAN(fMinSpread, false);
    ENDIAN(fSpread, false);
    ENDIAN(fDrift, false);
    ENDIAN(fIronFOV, false);
    ENDIAN(fMinRange, false);
    ENDIAN(fMaxRange, false);
    ENDIAN(eHitBehavior, false);
    ENDIAN(iFlagsEx, false);
    ENDIAN(fAttackMult, false);
    ENDIAN(fShotsPerSec, false);
    ENDIAN(fActionPoints, false);
    ENDIAN(fFiringRumbleLeftMotorStrength, false);
    ENDIAN(fFiringRumbleRightMotorStrength, false);
    ENDIAN(fFiringRumbleDuration, false);
    ENDIAN(fDamageToWeaponMult, false);
    ENDIAN(fAnimShotsPerSecond, false);
    ENDIAN(fAnimReloadTime, false);
    ENDIAN(fAnimJamTime, false);
    ENDIAN(fAimArc, false);
    ENDIAN(eSkill, false);
    ENDIAN(eRumblePattern, false);
    ENDIAN(fRumbleWavelength, false);
    ENDIAN(fLimbDamageMult, false);
    ENDIAN(eResistance, false);
    ENDIAN(fIronSightUseMult, false);
    ENDIAN(fSemiAutomaticFireDelayMin, false);
    ENDIAN(fSemiAutomaticFireDelayMax, false);
    ENDIAN(fCookTimer, false);
    ENDIAN(eModActionOne, false);
    ENDIAN(eModActionTwo, false);
    ENDIAN(eModActionThree, false);
    ENDIAN(fModActionOneValue, false);
    ENDIAN(fModActionTwoValue, false);
    ENDIAN(fModActionThreeValue, false);
    ENDIAN(iStrengthRequirement, false);
    ENDIAN(iSkillRequirement, false);
    ENDIAN(fAmmoRegenRate, false);
    ENDIAN(fKillImpulse, false);
    ENDIAN(fModActionOneValueTwo, false);
    ENDIAN(fModActionTwoValueTwo, false);
    ENDIAN(fModActionThreeValueTwo, false);
    ENDIAN(fKillImpulseDistance, false);
}

void OBJ_WEAP_CRITICAL::Endian() {
    ENDIAN(sCriticalDamage, false);
    ENDIAN(fCriticalChanceMult, false);
}

void OBJ_WEAP_VATS_SPECIAL::Endian() {
    ENDIAN(fVATSSpecialAP, false);
    ENDIAN(fVATSSpecialMultiplier, false);
    ENDIAN(fVATSSkillRequired, false);
}

TESObjectWEAP::TESObjectWEAP() {
    cFormType = WEAP_ID;
    TESObjectWEAP::InitializeData();
}

TESObjectWEAP::~TESObjectWEAP() {
    TESObjectWEAP::ClearData();
}

void TESObjectWEAP::InitializeData() {
    ourMemset(&data, 0, sizeof(data));
    data.cAmmoPerShot = 1;
    data.fShotsPerSec = 5.0f;
    data.fMinSpread = 0.0f;
    data.fSpread = 10.0f;
    data.fDrift = 1.0f;
    data.cVATSToHitChance = 5;
    data.cReloadAnim = 0;
    data.cAttackAnim = -1;
    data.cHandGripAnim = -1;
    data.pProjectile = nullptr;
    data.cNumProjectiles = 1;
    data.fMinRange = 500.0f;
    data.fMaxRange = 2000.0f;
    data.eHitBehavior = WHB_NORMAL;
    data.fActionPoints = 0.0f;
    data.fFiringRumbleLeftMotorStrength = 0.5f;
    data.fFiringRumbleRightMotorStrength = 1.0f;
    data.fFiringRumbleDuration = 0.33f;
    data.fDamageToWeaponMult = 1.0f;
    data.eSkill = ActorValue::UNARMED;
    data.fLimbDamageMult = 1.0f;
    data.fIronSightUseMult = 0.0f;
    data.eResistance = ActorValue::VALUE_NONE;
    data.fSemiAutomaticFireDelayMin = 0.1f;
    data.fSemiAutomaticFireDelayMax = 0.5f;
    data.iStrengthRequirement = 0;
    data.iSkillRequirement = 0;
    data.fModActionOneValue = 0.0f;
    data.fModActionTwoValue = 0.0f;
    data.fModActionThreeValue = 0.0f;
    data.fModActionOneValueTwo = 0.0f;
    data.fModActionTwoValueTwo = 0.0f;
    data.fModActionThreeValueTwo = 0.0f;
    data.fKillImpulse = 1.0f;
    data.cPowerAttackOverrideAnim = static_cast<uint32_t>(
        (data.cPowerAttackOverrideAnim & 0xFFFFFF00u) | (static_cast<uint32_t>(static_cast<uint8_t>((-1))))
    );
    data.iModReloadClipAnimation = -1;
    data.iModFireAnimation = -1;
    criticalData.sCriticalDamage = 0;
    criticalData.fCriticalChanceMult = 1.0f;
    criticalData.bEffectOnDeath = 1;
    criticalData.pEffect = nullptr;
    specialVATSData.fVATSSpecialAP = 0.0f;
    specialVATSData.fVATSSpecialMultiplier = 0.0f;
    specialVATSData.pVATSSpecialEffect = nullptr;
    specialVATSData.fVATSSkillRequired = 0.0f;
    specialVATSData.bSilent = 1;
    specialVATSData.bModRequired = 0;
    pScopeEffect = nullptr;
    pAttackSound = nullptr;
    pAttackSound2 = nullptr;
    pAttackSound2D = nullptr;
    pAttackLoop = nullptr;
    pAttackFailSound = nullptr;
    pMeleeBlockSound = nullptr;
    pIdleSound = nullptr;
    pEquipSound = nullptr;
    pUnEquipSound = nullptr;
    pModSilencedSound = nullptr;
    pModSilencedSound2 = nullptr;
    pModSilencedSound2D = nullptr;
    pImpactDataSet = nullptr;
    p1stPersonObject = nullptr;
    p1stPersonMod1Object = nullptr;
    p1stPersonMod2Object = nullptr;
    p1stPersonMod3Object = nullptr;
    p1stPersonMod12Object = nullptr;
    p1stPersonMod13Object = nullptr;
    p1stPersonMod23Object = nullptr;
    p1stPersonMod123Object = nullptr;
    pModObjectOne = nullptr;
    pModObjectTwo = nullptr;
    pModObjectThree = nullptr;
    ShellCasingModel.InitializeDataComponent();
    ScopeModel.InitializeDataComponent();
    WorldModel.InitializeDataComponent();
    WorldModelMod1.InitializeDataComponent();
    WorldModelMod2.InitializeDataComponent();
    WorldModelMod3.InitializeDataComponent();
    WorldModelMod12.InitializeDataComponent();
    WorldModelMod13.InitializeDataComponent();
    WorldModelMod23.InitializeDataComponent();
    WorldModelMod123.InitializeDataComponent();
    SetCastingType(MagicSystem::WHEN_STRIKES);
    eSoundLevel = SOUND_LEVEL_NORMAL;
    bIsLoopingReload = 0;
    if (GetReloadAnim(false) >= ANIM_GROUP_RELOAD_W) {
        bIsLoopingReload = 1;
    }
}

void TESObjectWEAP::ClearData() {
    BaseFormComponent* pComponent = &ShellCasingModel;
    pComponent->ClearDataComponent();
    pComponent = &ScopeModel;
    pComponent->ClearDataComponent();
    pComponent = &WorldModel;
    pComponent->ClearDataComponent();
    pComponent = &WorldModelMod1;
    pComponent->ClearDataComponent();
    pComponent = &WorldModelMod2;
    pComponent->ClearDataComponent();
    pComponent = &WorldModelMod3;
    pComponent->ClearDataComponent();
    pComponent = &WorldModelMod12;
    pComponent->ClearDataComponent();
    pComponent = &WorldModelMod13;
    pComponent->ClearDataComponent();
    pComponent = &WorldModelMod23;
    pComponent->ClearDataComponent();
    pComponent = &WorldModelMod123;
    pComponent->ClearDataComponent();
    EmbeddedWeaponNode.Set(nullptr, 0);
    VATSAttackName.Set(nullptr, 0);
}

void TESObjectWEAP::Save() {
    SpellItem* v3;
    SpellItem* v4;
    BGSProjectile* CastingType;
    StartForm();
    TESFullName::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    SaveFlags(MODD_ID);
    TESTexture::Save(ICON_ID);
    BGSMessageIcon::Save(MICO_ID);
    TESScriptableForm::Save();
    SaveSubRecord();
    BGSAmmoForm::Save();
    BGSDestructibleObjectForm::Save();
    BGSRepairItemList::Save();
    BGSEquipType::Save();
    BGSBipedModelList::Save();
    BGSPickupPutdownSounds::Save();
    ShellCasingModel.Save(MOD2_ID, MO2T_ID);
    if (HasScope()) {
        ScopeModel.Save(MOD3_ID, MO3T_ID);
        if (pScopeEffect) {
            TESForm::AddChunk(EFSD_ID, pScopeEffect->iFormID);
        }
    }
    WorldModel.Save(MOD4_ID, MO4T_ID);
    WorldModelMod1.Save(MWD1_ID, MWT1_ID, MODS_ID);
    WorldModelMod2.Save(MWD2_ID, MWT2_ID, MODS_ID);
    WorldModelMod3.Save(MWD4_ID, MWT4_ID, MODS_ID);
    WorldModelMod12.Save(MWD3_ID, MWT3_ID, MODS_ID);
    WorldModelMod13.Save(MWD5_ID, MWT5_ID, MODS_ID);
    WorldModelMod23.Save(MWD6_ID, MWT6_ID, MODS_ID);
    WorldModelMod123.Save(MWD7_ID, MWT7_ID, MODS_ID);
    if (EmbeddedWeaponNode.GetLength()) {
        uint32_t Length = EmbeddedWeaponNode.GetLength();
        TESForm::AddChunkArray(NNAM_ID, EmbeddedWeaponNode.pString, Length + 1);
    }
    if (VATSAttackName.GetLength()) {
        uint32_t v2 = VATSAttackName.GetLength();
        TESForm::AddChunkArray(VANM_ID, VATSAttackName.pString, v2 + 1);
    }
    if (pImpactDataSet) {
        TESForm::AddChunk(INAM_ID, pImpactDataSet->iFormID);
    }
    if (p1stPersonObject) {
        TESForm::AddChunk(WNAM_ID, p1stPersonObject->iFormID);
    }
    if (p1stPersonMod1Object) {
        TESForm::AddChunk(WNM1_ID, p1stPersonMod1Object->iFormID);
    }
    if (p1stPersonMod2Object) {
        TESForm::AddChunk(WNM2_ID, p1stPersonMod2Object->iFormID);
    }
    if (p1stPersonMod3Object) {
        TESForm::AddChunk(WNM4_ID, p1stPersonMod3Object->iFormID);
    }
    if (p1stPersonMod12Object) {
        TESForm::AddChunk(WNM3_ID, p1stPersonMod12Object->iFormID);
    }
    if (p1stPersonMod23Object) {
        TESForm::AddChunk(WNM6_ID, p1stPersonMod23Object->iFormID);
    }
    if (p1stPersonMod13Object) {
        TESForm::AddChunk(WNM5_ID, p1stPersonMod13Object->iFormID);
    }
    if (p1stPersonMod123Object) {
        TESForm::AddChunk(WNM7_ID, p1stPersonMod123Object->iFormID);
    }
    if (pModObjectOne) {
        TESForm::AddChunk(WMI1_ID, pModObjectOne->iFormID);
    }
    if (pModObjectTwo) {
        TESForm::AddChunk(WMI2_ID, pModObjectTwo->iFormID);
    }
    if (pModObjectThree) {
        TESForm::AddChunk(WMI3_ID, pModObjectThree->iFormID);
    }
    if (pAttackSound) {
        TESForm::AddChunk(SNAM_ID, pAttackSound->iFormID);
    }
    if (pAttackSound2) {
        TESForm::AddChunk(SNAM_ID, pAttackSound2->iFormID);
    }
    if (pAttackSound2D) {
        TESForm::AddChunk(XNAM_ID, pAttackSound2D->iFormID);
    }
    if (pAttackLoop) {
        TESForm::AddChunk(NAM7_ID, pAttackLoop->iFormID);
    }
    if (pAttackFailSound) {
        TESForm::AddChunk(TNAM_ID, pAttackFailSound->iFormID);
    }
    if (pMeleeBlockSound) {
        TESForm::AddChunk(NAM6_ID, pMeleeBlockSound->iFormID);
    }
    if (this->GetIdleSound()) {
        TESForm::AddChunk(UNAM_ID, this->GetIdleSound()->iFormID);
    }
    if (this->GetEquipSound()) {
        TESForm::AddChunk(NAM9_ID, this->GetEquipSound()->iFormID);
    }
    if (pUnEquipSound) {
        TESForm::AddChunk(NAM8_ID, pUnEquipSound->iFormID);
    }
    if (pModSilencedSound) {
        TESForm::AddChunk(WMS1_ID, pModSilencedSound->iFormID);
    }
    if (pModSilencedSound2) {
        TESForm::AddChunk(WMS1_ID, pModSilencedSound2->iFormID);
    }
    if (pModSilencedSound2D) {
        TESForm::AddChunk(WMS2_ID, pModSilencedSound2D->iFormID);
    }
    SaveData();
    BGSProjectile* psaveProjectile = data.pProjectile;
    if (psaveProjectile) {
        CastingType = reinterpret_cast<BGSProjectile*>(static_cast<uintptr_t>(data.pProjectile->iFormID));
    }
    else {
        CastingType = nullptr;
    }
    data.pProjectile = CastingType;
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__AddChunkData(DNAM_ID, &data, 204u);
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    SpellItem* psaveCriticalEffect = criticalData.pEffect;
    if (psaveCriticalEffect) {
        v4 = reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(criticalData.pEffect->iFormID));
    }
    else {
        v4 = nullptr;
    }
    criticalData.pEffect = v4;
    if (TESForm::QEndianSwapOnSave()) {
        criticalData.Endian();
    }
    TESForm::__AddChunkData(CRDT_ID, &criticalData, 16u);
    if (TESForm::QEndianSwapOnSave()) {
        criticalData.Endian();
    }
    SpellItem* psaveVATSSpecialEffect = specialVATSData.pVATSSpecialEffect;
    if (psaveVATSSpecialEffect) {
        v3 = reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(specialVATSData.pVATSSpecialEffect->iFormID));
    }
    else {
        v3 = nullptr;
    }
    specialVATSData.pVATSSpecialEffect = v3;
    if (TESForm::QEndianSwapOnSave()) {
        specialVATSData.Endian();
    }
    TESForm::__AddChunkData(VATS_ID, &specialVATSData, 20u);
    if (TESForm::QEndianSwapOnSave()) {
        specialVATSData.Endian();
    }
    TESForm::AddChunk(VNAM_ID, static_cast<uint32_t>(eSoundLevel));
    data.pProjectile = psaveProjectile;
    criticalData.pEffect = psaveCriticalEffect;
    specialVATSData.pVATSSpecialEffect = psaveVATSSpecialEffect;
    CloseForm();
}

uint16_t TESObjectWEAP::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
    return (uint16_t)(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
}

void TESObjectWEAP::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::SaveGame(aiFlags);
}

void TESObjectWEAP::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    static_cast<TESValueForm*>(this)->SaveGame(apSaveGameBuffer);
}

void TESObjectWEAP::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESObjectWEAP::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    static_cast<TESValueForm*>(this)->LoadGame(apLoadGameBuffer);
}

void TESObjectWEAP::InitItem() {
    if (!GetInitialized()) {
        BGSBipedModelList::InitItem(this);
        TESScriptableForm::InitItem(this);
        TESEnchantableForm::InitItem(this);
        BGSAmmoForm::InitItem(this);
        BGSDestructibleObjectForm::InitItem(this);
        TESModelTextureSwap::InitItem(this);
        BGSRepairItemList::InitItem(this);
        BGSPickupPutdownSounds::InitItem(this);
        uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetProjectile(nullptr)));
        if (itempInt) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempInt);
            data.pProjectile = dynamic_cast<BGSProjectile*>(FormByNumericID);
            if (!GetProjectile(nullptr)) {
                int32_t CastingType = iFormID;
                const char* v61 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find projectile (%08X) on weapon '%s' (%08X).", itempInt, v61, CastingType
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pScopeEffect));
        if (itempInt) {
            TESFile* v4 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v4);
            TESForm* v5 = TESForm::GetFormByNumericID(itempInt);
            TESEffectShader* v6 = dynamic_cast<TESEffectShader*>(v5);
            SetScopeEffect(v6);
            if (!pScopeEffect) {
                int32_t v90 = iFormID;
                const char* v62 = GetFormEditorID();
                Warning("MASTERFILE: Could not find scope effect (%08X) on weapon '%s' (%08X).", itempInt, v62, v90);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetCriticalEffect()));
        if (itempInt) {
            TESFile* v7 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v7);
            TESForm* v8 = TESForm::GetFormByNumericID(itempInt);
            SpellItem* v9 = dynamic_cast<SpellItem*>(v8);
            SetCriticalEffect(v9);
            if (!GetCriticalEffect()) {
                int32_t v91 = iFormID;
                const char* v63 = GetFormEditorID();
                Warning("MASTERFILE: Could not find Spell Item (%08X) on weapon '%s' (%08X).", itempInt, v63, v91);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetVATSSpecialAttackEffect()));
        if (itempInt) {
            TESFile* v10 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v10);
            TESForm* v11 = TESForm::GetFormByNumericID(itempInt);
            SpellItem* v12 = dynamic_cast<SpellItem*>(v11);
            SetVATSSpecialAttackEffect(v12);
            if (!GetVATSSpecialAttackEffect()) {
                int32_t v92 = iFormID;
                const char* v64 = GetFormEditorID();
                Warning("MASTERFILE: Could not find vats Spell Item (%08X) on weapon '%s' (%08X).", itempInt, v64, v92);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAttackSound));
        if (itempInt) {
            TESFile* v13 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v13);
            TESForm* v14 = TESForm::GetFormByNumericID(itempInt);
            pAttackSound = dynamic_cast<TESSound*>(v14);
            if (!pAttackSound) {
                int32_t v93 = iFormID;
                const char* v65 = GetFormEditorID();
                Warning("MASTERFILE: Could not find open sound (%08X) for weapon '%s' (%08X).", itempInt, v65, v93);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAttackSound2));
        if (itempInt) {
            TESFile* v15 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v15);
            TESForm* v16 = TESForm::GetFormByNumericID(itempInt);
            pAttackSound2 = dynamic_cast<TESSound*>(v16);
            if (!pAttackSound2) {
                int32_t v94 = iFormID;
                const char* v66 = GetFormEditorID();
                Warning("MASTERFILE: Could not find open sound2 (%08X) for weapon '%s' (%08X).", itempInt, v66, v94);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAttackSound2D));
        if (itempInt) {
            TESFile* v17 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v17);
            TESForm* v18 = TESForm::GetFormByNumericID(itempInt);
            pAttackSound2D = dynamic_cast<TESSound*>(v18);
            if (!pAttackSound2D) {
                int32_t v95 = iFormID;
                const char* v67 = GetFormEditorID();
                Warning("MASTERFILE: Could not find open sound (%08X) for weapon '%s' (%08X).", itempInt, v67, v95);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAttackLoop));
        if (itempInt) {
            TESFile* v19 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v19);
            TESForm* v20 = TESForm::GetFormByNumericID(itempInt);
            pAttackLoop = dynamic_cast<TESSound*>(v20);
            if (!pAttackLoop) {
                int32_t v96 = iFormID;
                const char* v68 = GetFormEditorID();
                Warning("MASTERFILE: Could not find sound (%08X) for weapon '%s' (%08X).", itempInt, v68, v96);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAttackFailSound));
        if (itempInt) {
            TESFile* v21 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v21);
            TESForm* v22 = TESForm::GetFormByNumericID(itempInt);
            pAttackFailSound = dynamic_cast<TESSound*>(v22);
            if (!pAttackFailSound) {
                int32_t v97 = iFormID;
                const char* v69 = GetFormEditorID();
                Warning("MASTERFILE: Could not find open sound (%08X) for weapon '%s' (%08X).", itempInt, v69, v97);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pMeleeBlockSound));
        if (itempInt) {
            TESFile* v23 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v23);
            TESForm* v24 = TESForm::GetFormByNumericID(itempInt);
            pMeleeBlockSound = dynamic_cast<TESSound*>(v24);
            if (!pMeleeBlockSound) {
                int32_t v98 = iFormID;
                const char* v70 = GetFormEditorID();
                Warning("MASTERFILE: Could not find open sound (%08X) for weapon '%s' (%08X).", itempInt, v70, v98);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->GetIdleSound()));
        if (itempInt) {
            TESFile* v25 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v25);
            TESForm* v26 = TESForm::GetFormByNumericID(itempInt);
            pIdleSound = dynamic_cast<TESSound*>(v26);
            if (!this->GetIdleSound()) {
                int32_t v99 = iFormID;
                const char* v71 = GetFormEditorID();
                Warning("MASTERFILE: Could not find idle sound (%08X) for weapon '%s' (%08X).", itempInt, v71, v99);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->GetEquipSound()));
        if (itempInt) {
            TESFile* v27 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v27);
            TESForm* v28 = TESForm::GetFormByNumericID(itempInt);
            pEquipSound = dynamic_cast<TESSound*>(v28);
            if (!this->GetEquipSound()) {
                int32_t v100 = iFormID;
                const char* v72 = GetFormEditorID();
                Warning("MASTERFILE: Could not find equip sound (%08X) for weapon '%s' (%08X).", itempInt, v72, v100);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pUnEquipSound));
        if (itempInt) {
            TESFile* v29 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v29);
            TESForm* v30 = TESForm::GetFormByNumericID(itempInt);
            pUnEquipSound = dynamic_cast<TESSound*>(v30);
            if (!pUnEquipSound) {
                int32_t v101 = iFormID;
                const char* v73 = GetFormEditorID();
                Warning("MASTERFILE: Could not find unequip sound (%08X) for weapon '%s' (%08X).", itempInt, v73, v101);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pModSilencedSound));
        if (itempInt) {
            TESFile* v31 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v31);
            TESForm* v32 = TESForm::GetFormByNumericID(itempInt);
            pModSilencedSound = dynamic_cast<TESSound*>(v32);
            if (!pModSilencedSound) {
                int32_t v102 = iFormID;
                const char* v74 = GetFormEditorID();
                Warning("MASTERFILE: Could not find unequip sound (%08X) for weapon '%s' (%08X).", itempInt, v74, v102);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pModSilencedSound2));
        if (itempInt) {
            TESFile* v33 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v33);
            TESForm* v34 = TESForm::GetFormByNumericID(itempInt);
            pModSilencedSound2 = dynamic_cast<TESSound*>(v34);
            if (!pModSilencedSound2) {
                int32_t v103 = iFormID;
                const char* v75 = GetFormEditorID();
                Warning("MASTERFILE: Could not find unequip sound (%08X) for weapon '%s' (%08X).", itempInt, v75, v103);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pModSilencedSound2D));
        if (itempInt) {
            TESFile* v35 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v35);
            TESForm* v36 = TESForm::GetFormByNumericID(itempInt);
            pModSilencedSound2D = dynamic_cast<TESSound*>(v36);
            if (!pModSilencedSound2D) {
                int32_t v104 = iFormID;
                const char* v76 = GetFormEditorID();
                Warning("MASTERFILE: Could not find unequip sound (%08X) for weapon '%s' (%08X).", itempInt, v76, v104);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pImpactDataSet));
        if (itempInt) {
            TESFile* v37 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v37);
            TESForm* v38 = TESForm::GetFormByNumericID(itempInt);
            pImpactDataSet = dynamic_cast<BGSImpactDataSet*>(v38);
            if (!pImpactDataSet) {
                int32_t v105 = iFormID;
                const char* v77 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find impact data set (%08X) for weapon '%s' (%08X).", itempInt, v77, v105
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p1stPersonObject));
        if (itempInt) {
            TESFile* v39 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v39);
            TESForm* v40 = TESForm::GetFormByNumericID(itempInt);
            p1stPersonObject = dynamic_cast<TESObjectSTAT*>(v40);
            if (!p1stPersonObject) {
                int32_t v106 = iFormID;
                const char* v78 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find 1st person model object (%08X) for weapon '%s' (%08X).", itempInt, v78,
                    v106
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p1stPersonMod1Object));
        if (itempInt) {
            TESFile* v41 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v41);
            TESForm* v42 = TESForm::GetFormByNumericID(itempInt);
            p1stPersonMod1Object = dynamic_cast<TESObjectSTAT*>(v42);
            if (!p1stPersonMod1Object) {
                int32_t v107 = iFormID;
                const char* v79 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find 1st person model object (%08X) for weapon '%s' (%08X).", itempInt, v79,
                    v107
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p1stPersonMod2Object));
        if (itempInt) {
            TESFile* v43 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v43);
            TESForm* v44 = TESForm::GetFormByNumericID(itempInt);
            p1stPersonMod2Object = dynamic_cast<TESObjectSTAT*>(v44);
            if (!p1stPersonMod2Object) {
                int32_t v108 = iFormID;
                const char* v80 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find 1st person model object (%08X) for weapon '%s' (%08X).", itempInt, v80,
                    v108
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p1stPersonMod3Object));
        if (itempInt) {
            TESFile* v45 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v45);
            TESForm* v46 = TESForm::GetFormByNumericID(itempInt);
            p1stPersonMod3Object = dynamic_cast<TESObjectSTAT*>(v46);
            if (!p1stPersonMod3Object) {
                int32_t v109 = iFormID;
                const char* v81 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find 1st person model object (%08X) for weapon '%s' (%08X).", itempInt, v81,
                    v109
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p1stPersonMod12Object));
        if (itempInt) {
            TESFile* v47 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v47);
            TESForm* v48 = TESForm::GetFormByNumericID(itempInt);
            p1stPersonMod12Object = dynamic_cast<TESObjectSTAT*>(v48);
            if (!p1stPersonMod12Object) {
                int32_t v110 = iFormID;
                const char* v82 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find 1st person model object (%08X) for weapon '%s' (%08X).", itempInt, v82,
                    v110
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p1stPersonMod13Object));
        if (itempInt) {
            TESFile* v49 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v49);
            TESForm* v50 = TESForm::GetFormByNumericID(itempInt);
            p1stPersonMod13Object = dynamic_cast<TESObjectSTAT*>(v50);
            if (!p1stPersonMod13Object) {
                int32_t v111 = iFormID;
                const char* v83 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find 1st person model object (%08X) for weapon '%s' (%08X).", itempInt, v83,
                    v111
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p1stPersonMod23Object));
        if (itempInt) {
            TESFile* v51 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v51);
            TESForm* v52 = TESForm::GetFormByNumericID(itempInt);
            p1stPersonMod23Object = dynamic_cast<TESObjectSTAT*>(v52);
            if (!p1stPersonMod23Object) {
                int32_t v112 = iFormID;
                const char* v84 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find 1st person model object (%08X) for weapon '%s' (%08X).", itempInt, v84,
                    v112
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p1stPersonMod123Object));
        if (itempInt) {
            TESFile* v53 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v53);
            TESForm* v54 = TESForm::GetFormByNumericID(itempInt);
            p1stPersonMod123Object = dynamic_cast<TESObjectSTAT*>(v54);
            if (!p1stPersonMod123Object) {
                int32_t v113 = iFormID;
                const char* v85 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find 1st person model object (%08X) for weapon '%s' (%08X).", itempInt, v85,
                    v113
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pModObjectOne));
        if (itempInt) {
            TESFile* v55 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v55);
            TESForm* v56 = TESForm::GetFormByNumericID(itempInt);
            pModObjectOne = dynamic_cast<TESObjectIMOD*>(v56);
            if (!pModObjectOne) {
                int32_t v114 = iFormID;
                const char* v86 = GetFormEditorID();
                Warning("MASTERFILE: Could not find misc object (%08X) for weapon '%s' (%08X).", itempInt, v86, v114);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pModObjectTwo));
        if (itempInt) {
            TESFile* v57 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v57);
            TESForm* v58 = TESForm::GetFormByNumericID(itempInt);
            pModObjectTwo = dynamic_cast<TESObjectIMOD*>(v58);
            if (!pModObjectTwo) {
                int32_t v115 = iFormID;
                const char* v87 = GetFormEditorID();
                Warning("MASTERFILE: Could not find misc object (%08X) for weapon '%s' (%08X).", itempInt, v87, v115);
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pModObjectThree));
        if (itempInt) {
            TESFile* v59 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v59);
            TESForm* v60 = TESForm::GetFormByNumericID(itempInt);
            pModObjectThree = dynamic_cast<TESObjectIMOD*>(v60);
            if (!pModObjectThree) {
                int32_t v116 = iFormID;
                const char* v88 = GetFormEditorID();
                Warning("MASTERFILE: Could not find misc object (%08X) for weapon '%s' (%08X).", itempInt, v88, v116);
            }
        }
        if (data.fSpeed <= 0.0) {
            data.fSpeed = 1.0f;
        }
        if (data.fAttackMult <= 0.0) {
            data.fAttackMult = 1.0f;
        }
        if (bForceNPCsUseAmmo.Bool()) {
            SetNPCsUseAmmo(true);
        }
        bIsLoopingReload = 0;
        if (data.cReloadAnim >= 19u) {
            bIsLoopingReload = 1;
        }
        SetInitialized(true);
    }
}

void TESObjectWEAP::Copy(TESForm* apCopy) {
    TESObjectWEAP* weap = dynamic_cast<TESObjectWEAP*>(apCopy);
    if (weap) {
        CopyAllComponents(apCopy);
        ShellCasingModel.CopyComponent(&weap->ShellCasingModel);
        EmbeddedWeaponNode.Set(weap->EmbeddedWeaponNode.pString, 0);
        VATSAttackName.Set(weap->VATSAttackName.pString, 0);
        WorldModel.CopyComponent(&weap->WorldModel);
        ScopeModel.CopyComponent(&weap->ScopeModel);
        TESEffectShader* v2 = weap->pScopeEffect;
        SetScopeEffect(v2);
        SetImpactDataSet(weap->pImpactDataSet);
        Set1stPersonObject(weap->p1stPersonObject);
        TESObjectSTAT* v3 = weap->Get1stPersonModObject(1);
        Set1stPersonModObject(1, v3);
        TESObjectSTAT* v4 = weap->Get1stPersonModObject(2);
        Set1stPersonModObject(2, v4);
        TESObjectSTAT* v5 = weap->Get1stPersonModObject(3);
        Set1stPersonModObject(3, v5);
        TESObjectSTAT* v6 = weap->Get1stPersonModObject(4);
        Set1stPersonModObject(4, v6);
        TESObjectSTAT* v7 = weap->Get1stPersonModObject(5);
        Set1stPersonModObject(5, v7);
        TESObjectSTAT* v8 = weap->Get1stPersonModObject(6);
        Set1stPersonModObject(6, v8);
        TESObjectSTAT* v9 = weap->Get1stPersonModObject(7);
        Set1stPersonModObject(7, v9);
        TESObjectIMOD* ModItem = weap->GetModItem(1u);
        SetItemMod(1, ModItem);
        TESObjectIMOD* v11 = weap->GetModItem(2u);
        SetItemMod(2, v11);
        TESObjectIMOD* v12 = weap->GetModItem(4u);
        SetItemMod(4, v12);
        WorldModelMod1.CopyComponent(&weap->WorldModelMod1);
        WorldModelMod2.CopyComponent(&weap->WorldModelMod2);
        WorldModelMod3.CopyComponent(&weap->WorldModelMod3);
        WorldModelMod12.CopyComponent(&weap->WorldModelMod12);
        WorldModelMod13.CopyComponent(&weap->WorldModelMod13);
        WorldModelMod23.CopyComponent(&weap->WorldModelMod23);
        WorldModelMod123.CopyComponent(&weap->WorldModelMod123);
        ourMemcpy(&data, &weap->data, sizeof(data));
        ourMemcpy(&criticalData, &weap->criticalData, sizeof(criticalData));
        ourMemcpy(&specialVATSData, &weap->specialVATSData, sizeof(specialVATSData));
        TESSound* AttackSound = weap->GetAttackSound(false);
        SetAttackSound(AttackSound);
        TESSound* AttackSound2 = weap->GetAttackSound2(false);
        SetAttackSound2(AttackSound2);
        pAttackSound2D = weap->GetAttackSound2D(false);
        TESSound* v15 = weap->Get3DLoopingShotSound();
        SetAttackLoopSound(v15);
        TESSound* BlockSound = weap->GetBlockSound();
        SetMeleeBlockSound(BlockSound);
        TESSound* AttackFailSound = weap->GetAttackFailSound();
        SetAttackFailSound(AttackFailSound);
        TESSound* IdleSound = weap->GetIdleSound();
        SetIdleSound(IdleSound);
        TESSound* EquipSound = weap->GetEquipSound();
        SetEquipSound(EquipSound);
        TESSound* UnequipSound = weap->GetUnequipSound();
        SetUnEquipSound(UnequipSound);
        SetModAttackSound(weap->pModSilencedSound);
        pModSilencedSound2 = weap->pModSilencedSound2;
        pModSilencedSound2D = weap->pModSilencedSound2D;
        eSoundLevel = weap->GetSoundLevel(0);
        cFormType = weap->GetFormType();
    }
}

bool TESObjectWEAP::Compare(TESForm* apCompare) {
    TESObjectWEAP* pComp = dynamic_cast<TESObjectWEAP*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    // Each model's CompareComponent is called through a base pointer so the call stays virtual; a call on the member
    // itself would be devirtualised.
    BaseFormComponent* pModel = &ShellCasingModel;
    if (pModel->CompareComponent(&pComp->ShellCasingModel)) {
        return true;
    }
    pModel = &WorldModel;
    if (pModel->CompareComponent(&pComp->WorldModel)) {
        return true;
    }
    pModel = &WorldModelMod1;
    if (pModel->CompareComponent(&pComp->WorldModelMod1)) {
        return true;
    }
    pModel = &WorldModelMod2;
    if (pModel->CompareComponent(&pComp->WorldModelMod2)) {
        return true;
    }
    pModel = &WorldModelMod3;
    if (pModel->CompareComponent(&pComp->WorldModelMod3)) {
        return true;
    }
    pModel = &WorldModelMod12;
    if (pModel->CompareComponent(&pComp->WorldModelMod12)) {
        return true;
    }
    pModel = &WorldModelMod13;
    if (pModel->CompareComponent(&pComp->WorldModelMod13)) {
        return true;
    }
    pModel = &WorldModelMod23;
    if (pModel->CompareComponent(&pComp->WorldModelMod23)) {
        return true;
    }
    pModel = &WorldModelMod123;
    if (pModel->CompareComponent(&pComp->WorldModelMod123)) {
        return true;
    }
    pModel = &ScopeModel;
    if (pModel->CompareComponent(&pComp->ScopeModel)) {
        return true;
    }
    if (pScopeEffect != pComp->pScopeEffect) {
        return true;
    }
    BGSImpactDataSet* ImpactDataSet = pImpactDataSet;
    if (ImpactDataSet != pComp->pImpactDataSet) {
        return true;
    }
    TESObjectSTAT* v5 = p1stPersonObject;
    if (v5 != pComp->p1stPersonObject) {
        return true;
    }
    TESObjectSTAT* v6 = Get1stPersonModObject(1);
    if (v6 != pComp->Get1stPersonModObject(1)) {
        return true;
    }
    TESObjectSTAT* v7 = Get1stPersonModObject(2);
    if (v7 != pComp->Get1stPersonModObject(2)) {
        return true;
    }
    TESObjectSTAT* v8 = Get1stPersonModObject(3);
    if (v8 != pComp->Get1stPersonModObject(3)) {
        return true;
    }
    TESObjectSTAT* v9 = Get1stPersonModObject(4);
    if (v9 != pComp->Get1stPersonModObject(4)) {
        return true;
    }
    TESObjectSTAT* v10 = Get1stPersonModObject(5);
    if (v10 != pComp->Get1stPersonModObject(5)) {
        return true;
    }
    TESObjectSTAT* v11 = Get1stPersonModObject(6);
    if (v11 != pComp->Get1stPersonModObject(6)) {
        return true;
    }
    TESObjectSTAT* v12 = Get1stPersonModObject(7);
    if (v12 != pComp->Get1stPersonModObject(7)) {
        return true;
    }
    TESObjectIMOD* ModItem = GetModItem(1u);
    if (ModItem != pComp->GetModItem(1u)) {
        return true;
    }
    TESObjectIMOD* v14 = GetModItem(2u);
    if (v14 != pComp->GetModItem(2u)) {
        return true;
    }
    TESObjectIMOD* v15 = GetModItem(4u);
    if (v15 != pComp->GetModItem(4u)) {
        return true;
    }
    if (EmbeddedWeaponNode.StrCmp(pComp->EmbeddedWeaponNode, true)) {
        return true;
    }
    if (VATSAttackName.StrCmp(pComp->VATSAttackName, true)) {
        return true;
    }
    const void* TargetLocations = (const void*)&pComp->data;
    if (std::memcmp(&data, TargetLocations, sizeof(data))) {
        return true;
    }
    if (std::memcmp(&criticalData, &pComp->criticalData, sizeof(criticalData))) {
        return true;
    }
    if (std::memcmp(&specialVATSData, &pComp->specialVATSData, sizeof(specialVATSData))) {
        return true;
    }
    TESSound* AttackSound = GetAttackSound(false);
    if (AttackSound != pComp->GetAttackSound(false)) {
        return true;
    }
    TESSound* AttackSound2 = GetAttackSound2(false);
    if (AttackSound2 != pComp->GetAttackSound2(false)) {
        return true;
    }
    TESSound* AttackSound2D = GetAttackSound2D(false);
    if (AttackSound2D != pComp->GetAttackSound2D(false)) {
        return true;
    }
    TESSound* v20 = Get3DLoopingShotSound();
    if (v20 != pComp->Get3DLoopingShotSound()) {
        return true;
    }
    TESSound* AttackFailSound = GetAttackFailSound();
    if (AttackFailSound != pComp->GetAttackFailSound()) {
        return true;
    }
    TESSound* BlockSound = GetBlockSound();
    if (BlockSound != pComp->GetBlockSound()) {
        return true;
    }
    TESSound* IdleSound = GetIdleSound();
    if (IdleSound != pComp->GetIdleSound()) {
        return true;
    }
    TESSound* EquipSound = GetEquipSound();
    if (EquipSound != pComp->GetEquipSound()) {
        return true;
    }
    TESSound* UnequipSound = GetUnequipSound();
    if (UnequipSound != pComp->GetUnequipSound()) {
        return true;
    }
    if (pModSilencedSound != pComp->pModSilencedSound) {
        return true;
    }
    if (pModSilencedSound2 != pComp->pModSilencedSound2) {
        return true;
    }
    return pModSilencedSound2D != pComp->pModSilencedSound2D || eSoundLevel != pComp->GetSoundLevel(0);
}

bool TESObjectWEAP::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aicount
) {
    dynamic_cast<Actor*>(apActionRef);
    return (bool)((uint8_t)TESBoundObject::Activate(apItemActivated, apActionRef, abIdFlag, apObjecttoGet, aicount));
}

const char* TESObjectWEAP::GetObjectTypeName() {
    return TESObjectWEAP::pWeaponNames[data.eType];
}

// BGSPreloadable override. The projectile is converted to its BGSPreloadable base without a null check, as the game
// does, hence the reference conversions.
void TESObjectWEAP::QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    if (ShellCasingModel.GetModelLength()) {
        pModelLoader->QueueModel(&ShellCasingModel, aePriority, apParent, LOD_MULT_NONE, true, false, false);
    }
    if (GetProjectile(nullptr)) {
        BGSPreloadable& rProjectile = *GetProjectile(nullptr);
        rProjectile.QueueFiles(aePriority, apParent);
    }
    if (pImpactDataSet) {
        BGSPreloadable& rImpactDataSet = *pImpactDataSet;
        rImpactDataSet.QueueFiles(aePriority, apParent);
    }
}

bool TESObjectWEAP::IsLongBurst() {
    return (((int32_t)(data.iFlagsEx & 2048)) != 0);
}

bool TESObjectWEAP::GetUse3PISAnims() {
    return ((data.iFlagsEx & 256) == 0);
}

bool TESObjectWEAP::GetUse1PISAnims() {
    return ((data.cFlags & 64) == 0);
}

void TESObjectWEAP::QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent, bool bLoadScope) {
    if (ShellCasingModel.GetModelLength()) {
        pModelLoader->QueueModel(&ShellCasingModel, aePriority, apParent, LOD_MULT_NONE, true, false, false);
    }
    if (bLoadScope && HasScope() && ScopeModel.GetModelLength()) {
        ENUM_LOD_MULT eLODMult = TES::GetLODMult(this);
        pModelLoader->QueueModel(&ScopeModel, aePriority, apParent, eLODMult, true, false, false);
    }
    if (GetProjectile(nullptr)) {
        BGSPreloadable* pPreloadable = GetProjectile(nullptr);
        pPreloadable->QueueFiles(aePriority, apParent);
    }
    if (pImpactDataSet) {
        BGSPreloadable* pPreloadable = pImpactDataSet;
        pPreloadable->QueueFiles(aePriority, apParent);
    }
}

// The casing starts at the ShellCasingNode with the node's rotation, flies along the node's Z axis (randomised, at the
// eject speed, plus the source's character-controller velocity) and spins about a randomised X axis. A first person
// player's casing lives at most 2 seconds and is flagged first person; SetIs1stPerson is called even when the
// allocation failed, as the game does. The locals keep the game's evaluation order. The source is an actor: IsActor is
// tested first and then it is used as one.
void TESObjectWEAP::ExpelShellCasing(TESObjectREFR* apSourceRef) {
    Actor* apSource = static_cast<Actor*>(apSourceRef);
    TESForm* pSourceForm = apSource;
    if (!apSource || !pSourceForm->IsActor() || !GetShellCasingModel()->GetModelLength() || Main::GetSceneIsComplex()) {
        return;
    }
    NiAVObject* pSource3D;
    if (apSource == pPlayer) {
        pSource3D = pPlayer->GetCurrent3D();
    }
    else {
        pSource3D = apSource->Get3D();
    }
    NiAVObject* pCasingNode = BSUtilities::GetObjectByName(pSource3D, "ShellCasingNode");
    if (!pCasingNode) {
        return;
    }
    bool bInView = true;
    NiCamera* pCamera = Main::WorldRootCamera();
    if (!PointInFrustum(pCasingNode->GetWorldTranslate(), pCamera, 128.0f)) {
        bInView = false;
    }
    const NiPoint3& kCameraPos = Main::WorldRootCamera()->GetWorldTranslate();
    NiPoint3 kToCasing = pCasingNode->GetWorldTranslate() - kCameraPos;
    double dDistance = kToCasing.Length();
    // Out of view only when the setting is below the distance (not when unordered).
    if (fGunShellCameraDistance.Float() < dDistance) {
        bInView = false;
    }
    if (!bInView) {
        return;
    }
    TESModel* pCasingModel = GetShellCasingModel();
    const char* pFilename = pCasingModel->GetModel();
    NiPoint3 kPosition = pCasingNode->GetWorldTranslate();
    NiMatrix3 kRotation = pCasingNode->GetWorldTransform().m_Rotate;
    NiPoint3 kStartLinearVelocity;
    pCasingNode->GetWorldTransform().m_Rotate.GetCol(2, kStartLinearVelocity);
    float fNegDirection = -fGunShellDirectionRandomize.Float();
    float fDirection = fGunShellDirectionRandomize.Float();
    float fRandZ = RandomFloat(fNegDirection, fDirection);
    float fRandY = RandomFloat(fNegDirection, fDirection);
    float fRandX = RandomFloat(fNegDirection, fDirection);
    kStartLinearVelocity += NiPoint3(fRandX, fRandY, fRandZ);
    kStartLinearVelocity *= fGunShellEjectSpeed.Float();
    if (apSource->GetCharController()) {
        NiPoint3 kControllerVelocity;
        HK2NI(kControllerVelocity, static_cast<bhkCharacterController*>(apSource->GetCharController())->OutVelocity);
        kStartLinearVelocity += kControllerVelocity;
    }
    NiPoint3 kStartAngularVelocity;
    NiPoint3 kSpinAxis = NiPoint3_UNIT_X;
    float fNegRotate = -fGunShellRotateRandomize.Float();
    float fRotate = fGunShellRotateRandomize.Float();
    float fAxisScale = RandomFloat(fNegRotate, fRotate) + 1.0f;
    kSpinAxis *= fAxisScale;
    NiPoint3::UnitizeVector(kSpinAxis);
    float fSpinZ = fGunShellRotateSpeed.Float() * kSpinAxis.z;
    float fSpinX = fGunShellRotateSpeed.Float() * kSpinAxis.x;
    NiPoint3 kLocalSpin(fSpinX, 0.0f, fSpinZ);
    kStartAngularVelocity = pCasingNode->GetWorldTransform().m_Rotate * kLocalSpin;
    if (apSource == pPlayer && !pPlayer->Is3rdPerson() && !pVATS.IsInVats()) {
        void* pMemory = NiMemObject::operator new(sizeof(BSTempEffectDebris));
        BSTempEffectDebris* pDebris = nullptr;
        if (pMemory) {
            float fLifetime = BSMIN(fGunShellLifetime.Float(), 2.0f);
            TESObjectCELL* pCell = apSource->GetParentCell();
            pDebris = ::new (pMemory) BSTempEffectDebris(
                pCell, fLifetime, pFilename, apSource, kPosition, kRotation, kStartLinearVelocity,
                kStartAngularVelocity, 1.0f, false, true
            );
        }
        pDebris->SetIs1stPerson(true);
    }
    else {
        void* pMemory = NiMemObject::operator new(sizeof(BSTempEffectDebris));
        if (pMemory) {
            float fLifetime = fGunShellLifetime.Float();
            TESObjectCELL* pCell = apSource->GetParentCell();
            ::new (pMemory) BSTempEffectDebris(
                pCell, fLifetime, pFilename, apSource, kPosition, kRotation, kStartLinearVelocity,
                kStartAngularVelocity, 1.0f, false, true
            );
        }
    }
}

// Fires the weapon from apSource. For the player first rolls for the ammo's consumed item; then (unless a continuous
// beam is running) works out the release point (the raised location, the weapon bone of a thrown weapon or the fire
// node), the aim angles (moved for a scope, from the node, the combat aim point or a use-weapon target), draws the
// debug laser and aim sights, adds up the spread, rumbles the pad, launches every projectile of every shot (spreading
// each by a random angle unless it always hits, and counting VATS shots), uses up the ammo and damages the weapon.
void TESObjectWEAP::Fire(TESObjectREFR* apSource) {
    if (!apSource) {
        return;
    }
    Actor* pactor = apSource->IsActor() ? static_cast<Actor*>(apSource) : nullptr;
    if (pactor == pPlayer) {
        TESForm* pAmmo = GetCurrentAmmo(pPlayer);
        if (pAmmo) {
            TESForm* pItemReward = nullptr;
            TESForm* pobject = static_cast<TESAmmo*>(pAmmo)->GetAmmoConsumedForm();
            float fPercent = static_cast<TESAmmo*>(pAmmo)->GetAmmoConsumedPercent();
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_CHANCE_FOR_AMMO_ITEM, pPlayer, this, &fPercent);
            // The unsigned remainder is converted to float through a 64-bit integer.
            float fdieRoll = static_cast<float>(static_cast<int64_t>(Random() % 100));
            if (pobject && fPercent >= fdieRoll) {
                if (pobject->IsBoundObject()) {
                    pItemReward = pobject;
                }
                if (pItemReward) {
                    pPlayer->AddObjecttoContainer(static_cast<TESBoundObject*>(pItemReward), nullptr, 1);
                }
            }
        }
    }
    if (pactor && pactor->pContinuousBeamPersistant) {
        return;
    }

    bool bisPlayer = pactor == pPlayer;
    VATS_COMMAND* pvatsCommand = nullptr;
    BaseProcess* pactorAIProcess = pactor ? pactor->GetCurrentAIProcess() : nullptr;
    ItemChange* pweaponItem = pactorAIProcess ? pactorAIProcess->GetCurrentWeapon() : nullptr;
    NiPoint3 releasePoint;
    if (pactor) {
        NiPoint3 kRaise = NiPoint3_UNIT_Z * 96.0f;
        releasePoint = *apSource->QLocationOnReference() + kRaise;
    }
    else {
        releasePoint = *apSource->QLocationOnReference();
    }
    float faimLooking = pactor ? pactor->GetLooking() : apSource->GetAngleOnReference().x;
    float faimHeading = pactor ? pactor->GetHeading(false) : apSource->GetAngleOnReference().z;

    NiAVObject* pfireObject = nullptr;
    if (pactorAIProcess &&
        (GetWeaponType() == WEAPON_ONE_HAND_GRENADE || GetWeaponType() == WEAPON_ONE_HAND_MINE ||
         GetWeaponType() == WEAPON_ONE_HAND_LUNCHBOX_MINE || GetWeaponType() == WEAPON_ONE_HAND_THROWN)) {
        // The player's next throwable stays hidden in the hand while more than one is left.
        if (bisPlayer && pactorAIProcess && pactorAIProcess->GetCurrentAmmo() &&
            pactorAIProcess->GetCurrentAmmo()->GetCount() > 1) {
            pfireObject = pactorAIProcess->GetWeaponBone(pPlayer->GetBiped(pPlayer->Is3rdPerson()));
            if (pfireObject && pfireObject->IsNode() && pfireObject->IsNode()->GetAtChecked(0)) {
                pfireObject->IsNode()->GetAt(0)->SetAppCulled(true);
            }
        }
        pfireObject = pactorAIProcess->GetWeaponBone(pactor->GetCurrentBiped());
        if (pfireObject) {
            releasePoint = pfireObject->GetWorldTranslate();
        }
        if (bisPlayer) {
            pPlayer->ClearAutoAim();
        }
    }
    else {
        if (pactorAIProcess) {
            pfireObject = apSource->GetFireNode();
        }
        else {
            NiAVObject* p3D = bisPlayer ? pPlayer->GetCurrent3D() : apSource->Get3D();
            pfireObject = GetFireNode(p3D);
        }
        if (pfireObject) {
            releasePoint = pfireObject->GetWorldTranslate();
            if ((pVATS.GetMode() != VATS::VATS_PLAYBACK && pactor == pPlayer && pPlayer->GetIronSights()) &&
                (GetScopeModel()->GetModelLength())) {
                // Whether the scope is in use: always for a built-in scope, else when the scope mod is on.
                bool bRoot = false;
                if (HasModScope()) {
                    BaseProcess* pProc = pPlayer->GetCurrentAIProcess();
                    if (pProc && pProc->GetCurrentWeapon() &&
                        pProc->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_IRON_SITES)) {
                        bRoot = true;
                    }
                }
                else {
                    bRoot = true;
                }
                // Move the projectile node back 5 units when using a scope.
                NiCamera* pcam = Main::WorldRootCamera();
                if (bRoot && pcam) {
                    if (!(uiOffsetGuard & 1)) {
                        uiOffsetGuard |= 1;
                        ::new (&offset) NiPoint3(0.0f, -5.0f, 0.0f);
                    }
                    const NiTransform* pCamWorld;
                    pCamWorld = &pcam->GetWorldTransform();
                    releasePoint = *pCamWorld * offset;
                }
            }

            if (apSource == pPlayer && !pPlayer->IsActually3rdPerson()) {
                faimHeading = pPlayer->GetHeading(false);
                faimLooking = pPlayer->GetLooking();
            }
            else {
                float funused;
                const NiTransform* pFireWorld;
                pFireWorld = &pfireObject->GetWorldTransform();
                pFireWorld->m_Rotate.ToEulerAnglesZXY(faimHeading, faimLooking, funused);
            }

            // Debug laser sight: a long thin cylinder hung off the fire node, built once.
            if (::bLaserSights.Bool()) {
                NiNode* pattachNode = pfireObject->IsNode();
                if (!pattachNode) {
                    NiAVObject* pParent = pfireObject->GetParent();
                    pattachNode = pParent->IsNode();
                }
                if (pattachNode) {
                    NiPointer<NiAVObject> splaserSight = pattachNode->GetObjectByName(FixedStrings::QLaserSightsName());
                    if (!splaserSight.operator->()) {
                        NiColorA color;
                        color.~NiColorA();
                        ::new (&color) NiColorA(1.0f, 0.0f, 0.0f, 1.0f);
                        splaserSight = BSTestObjects::CreateCylinder(1.0f, 1.0f, 4096.0f, 10, 10, &color);
                        NiFixedString& rName = FixedStrings::QLaserSightsName();
                        splaserSight->SetName(rName);
                        NiMatrix3 rot;
                        rot.~NiMatrix3();
                        ::new (&rot) NiMatrix3;
                        rot.MakeXRotation(1.5707964f);
                        splaserSight->SetRotate(rot);
                        AddNoLightingPropertyRecurse(splaserSight.operator->());
                        BSShaderManager::PrepareObject(splaserSight.operator->(), 0, 0);
                        pattachNode->AttachChild(splaserSight.operator->(), true);
                        splaserSight->UpdateProperties();
                        NiUpdateData UpdateData;
                        UpdateData.~NiUpdateData();
                        ::new (&UpdateData) NiUpdateData(0.0f, false, false);
                        splaserSight->Update(UpdateData);
                    }
                }
            }
            ImpactMixer::PlayGunshot(this, apSource, pfireObject, bisPlayer);
        }
    }

    float fskill = pactor ? pactor->GetGunSkill(GUN_SKILL_GUN) : fGunReferenceSkill.Float();
    bool bUseSpreadMod = false;
    if (pactor && pactor->GetCurrentAIProcess() && pweaponItem &&
        pweaponItem->HasModEffectActive(WEAPON_MOD_DECREASE_SPREAD)) {
        bUseSpreadMod = true;
    }
    // The minimum spread is kept as a double and the sum unrounded until the float store.
    double fminSpread = GetMinSpreadDegrees(bUseSpreadMod);
    float fspread = GetSpreadDegrees(bUseSpreadMod);
    float fspreadRad = static_cast<float>((static_cast<double>(fspread) * fskill + fminSpread) * 0.01745329238474369);
    if (pactor && pactor->GetCurrentAIProcess()) {
        BSSimpleList<TESAmmoEffect*>* pEffects =
            GetCurrentAmmo(pactor) ? static_cast<TESAmmo*>(GetCurrentAmmo(pactor))->GetAmmoEffectList() : nullptr;
        fspreadRad = TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_SPREAD, pEffects, fspreadRad);
    }

    float fdriftRad = 0.0f;
    CombatController* pcombatController = nullptr;
    if (!bisPlayer && pactor) {
        NiPoint3 targetedPoint;
        targetedPoint.~NiPoint3();
        ::new (&targetedPoint) NiPoint3;
        pcombatController = pactor->GetCombatController();
        if (pcombatController) {
            if (!pcombatController->GetAimLocation(&targetedPoint, &releasePoint)) {
                if (IsThrownWeapon()) {
                    pactor->GetAnimation()->BlendOut(AGS_WEAPON, false);
                }
                return;
            }
            NiPoint3 targetVector = targetedPoint - releasePoint;
            faimHeading = GetUnclampedZAngleFromVector(targetVector);
            float fsinLooking = targetVector.z / targetVector.Length();
            faimLooking = -NiASin(fsinLooking);

            // Debug aim sight: a box from the release point to the aim point under the cell's marker node.
            if (::bAimSights.Bool()) {
                NiAVObject* pattachNode = pactor->GetParentCell()->GetMarkerNode();
                if (pattachNode) {
                    NiPointer<NiAVObject> spaimSight = pattachNode->GetObjectByName(FixedStrings::QAimSightsName());
                    if (spaimSight.operator->()) {
                        NiNode* pParent = spaimSight->GetParent();
                        pParent->DetachChild(spaimSight.operator->());
                    }
                    NiColorA color;
                    color.~NiColorA();
                    ::new (&color) NiColorA(0.0f, 1.0f, 0.0f, 1.0f);
                    spaimSight =
                        BSTestObjects::CreateBox(releasePoint, targetedPoint, 0.5f, 0.5f, 0.5f, 0.5f, &color, nullptr);
                    NiFixedString& rName = FixedStrings::QAimSightsName();
                    spaimSight->SetName(rName);
                    // The marker node is a NiNode, so AttachChild is called on it.
                    static_cast<NiNode*>(pattachNode)->AttachChild(spaimSight.operator->(), true);
                    spaimSight->UpdateProperties();
                    NiUpdateData UpdateData;
                    UpdateData.~NiUpdateData();
                    ::new (&UpdateData) NiUpdateData(0.0f, false, false);
                    spaimSight->Update(UpdateData);
                }
            }
        }
        else if (
            pactorAIProcess && pactorAIProcess->GetPackageThatIsRunning() &&
            pactorAIProcess->GetPackageThatIsRunning()->GetPackType() == PACKAGE_TYPE_USE_WEAPON
        ) {
            ActorPackageData* pdata = pactorAIProcess->GetInstanceDataThatIsRunning();
            if (pdata) {
                UseWeaponActorPackageData* puseWeaponData = static_cast<UseWeaponActorPackageData*>(pdata);
                TESObjectREFR* pattackTarget = puseWeaponData->GetAttackTarget();
                if (!pattackTarget) {
                    return;
                }
                bool abHighArc = false;
                BGSProjectile* pprojectile = GetProjectile(nullptr);
                if (pprojectile) {
                    float fspeedMult = abHighArc ? gs_fGrenadeHighArcSpeedPercentage.Float() : 1.0f;
                    float fprojectileSpeed = pprojectile->GetSpeed() * fspeedMult;
                    targetedPoint = CombatUtilities::GetProjectedPointForTarget(
                        pactor, pattackTarget, nullptr, pprojectile, fprojectileSpeed, abHighArc
                    );
                }
                else {
                    targetedPoint = CombatUtilities::GetTargetFiringLocation(
                        static_cast<Actor*>(pattackTarget), ACTOR_SEGMENT_INVIEW_MIDDLE
                    );
                }
                NiPoint3 targetVector = targetedPoint - releasePoint;
                // Firing at the package's own location: shoot level along the heading instead.
                if (pactorAIProcess->GetPackageThatIsRunning()->GetLocationReference(pactor) == pattackTarget) {
                    targetVector.z = 0.0f;
                    NiSinCos(pactor->GetHeading(false), targetVector.x, targetVector.y);
                    targetVector *= 512.0f;
                    targetedPoint = releasePoint + targetVector;
                }
                faimHeading = GetUnclampedZAngleFromVector(targetVector);
                float fsinLooking = targetVector.z / targetVector.Length();
                faimLooking = -NiASin(fsinLooking);
            }
        }
        // The NPC's gun wobble, and its process's aim drift.
        float fwobble = pactor->GetGunSkill(GUN_SKILL_ACTOR);
        float* pfmaxWobble = &fNPCMaxGunWobbleAngle.Float();
        fspreadRad = static_cast<float>(static_cast<double>(fwobble) * *pfmaxWobble * 0.01745329238474369 + fspreadRad);
        BaseProcess* pProc = pactor->GetCurrentAIProcess();
        fdriftRad = pProc->GetAimLooking();
        fspreadRad = fspreadRad + fdriftRad;
    }

    if (((apSource == pPlayer && pactorAIProcess && pactorAIProcess->GetCurrentAmmo()) || IsRockItLauncher()) &&
        !Interface::IsInMenuMode()) {
        Controls* pcontrols = appMain->GetControls();
        float fduration;
        fduration = GetFiringRumbleDuration();
        if (fduration > 0.0f) {
            int32_t iduration;
            iduration = static_cast<int32_t>(GetFiringRumbleDuration() * 1000.0);
            float frightMotor;
            frightMotor = GetFiringRumbleRightMotorStrength();
            float fleftMotor = GetFiringRumbleLeftMotorStrength();
            pcontrols->Rumble(fleftMotor, frightMotor, iduration, 0, CONSTANT, false, false);
        }
        fduration = GetFiringRumbleDuration();
        if (fduration > 0.0f) {
            pcontrols->Rumble(0.5f, 1.0f, 330, 0, CONSTANT, false, false);
        }
    }

    // GetShotsToFire's result is kept as a signed byte count.
    int8_t cnumShots = pactorAIProcess ? static_cast<int8_t>(pactorAIProcess->GetShotsToFire()) : 1;
    bool bUseSplit = false;
    if (pweaponItem) {
        bUseSplit = pweaponItem->HasModEffectActive(WEAPON_MOD_SPLIT_BEAM);
    }
    if (bUseSplit) {
        fspreadRad = GetModEffectValue(WEAPON_MOD_SPLIT_BEAM, 1) * fspreadRad;
    }
    int32_t itotalProjectiles = GetNumProjectiles(false, false, pactor) * cnumShots;
    int32_t irealShots = 0;
    NiPointer<ProjectilePickCache> spcache(nullptr);
    if (pVATS.GetMode() == VATS::VATS_PLAYBACK && pactor == pVATSAttacker) {
        pvatsCommand = pVATS.GetCurrentAction();
    }
    bool balwaysHit = false;
    bool bdoNoDamage = false;
    TESPackage* ppack = pactor ? pactor->GetCurrentPackage() : nullptr;
    if (ppack) {
        const TESUseWeaponPackageData* ppackData = ppack->GetUseWeaponPackageData();
        if (ppackData) {
            balwaysHit = ppackData->GetAlwaysHit();
            bdoNoDamage = ppackData->GetDoNoDamage();
        }
    }
    bool bfakeShots = false;
    if (pvatsCommand && pvatsCommand->cFakeFireShots && ActionPoints::IsAttackAction(pvatsCommand->eAction)) {
        bfakeShots = true;
    }
    if (pactor && pactorAIProcess) {
        pactorAIProcess->AttackCallback(itotalProjectiles);
    }
    // The Mysterious Stranger and Miss Fortune always hit the VATS target.
    bool bstranger = false;
    TESObjectREFR* pforcedTarget = nullptr;
    if (pvatsCommand && pactor && (pactor == VATS::pStranger || pactor == VATS::pMissFortune)) {
        bstranger = true;
        balwaysHit = true;
        pforcedTarget = pvatsCommand->pTarget;
    }
    if (balwaysHit && !bstranger) {
        ActorPackageData* pdata = pactorAIProcess->GetInstanceDataThatIsRunning();
        if (pdata && pdata->GetPackageType() == PACKAGE_TYPE_USE_WEAPON) {
            pforcedTarget = static_cast<UseWeaponActorPackageData*>(pdata)->GetAttackTarget();
        }
    }

    while (cnumShots) {
        bool bshotIsFake =
            bfakeShots && ((fVATSShotBurstFraction < 0.0 && pvatsCommand->cFakeFireShots) || !pvatsCommand->cFireShots);
        bool bfirst = true;
        Projectile* plastProjectile = nullptr;
        for (uint8_t cshots = GetNumProjectiles(bUseSplit, false, pactor); cshots; --cshots) {
            float flooking = faimLooking;
            float fheading = faimHeading;
            float fspreadLooking = 0.0f;
            float fspreadHeading = 0.0f;
            if (!balwaysHit) {
                float fradius = RandomFloat(0.0f, fspreadRad);
                float frandomAngle = RandomFloat(0.0f, 6.2831855f);
                fspreadLooking = std::sin(frandomAngle) * fradius;
                fspreadHeading = std::cos(frandomAngle) * fradius;
                flooking = flooking + fspreadLooking;
                fheading = fheading + fspreadHeading;
            }
            Actor* pActorSource = nullptr;
            if (apSource->IsActor()) {
                pActorSource = static_cast<Actor*>(apSource);
            }
            bool bnoDamage = bdoNoDamage || bshotIsFake;
            BGSProjectile* pbase = GetProjectile(pActorSource);
            plastProjectile = Projectile::Launch(
                pbase, apSource, pcombatController, this, releasePoint, fheading, flooking, spcache.operator->(),
                pforcedTarget, balwaysHit, bnoDamage, fspreadHeading, fspreadLooking, nullptr
            );
            if (pvatsCommand && (pvatsCommand->cFireShots || pvatsCommand->cFakeFireShots) &&
                ActionPoints::IsAttackAction(pvatsCommand->eAction) && !bshotIsFake &&
                (!IsAutomaticWeapon() || (!pVATS.GetFirstProjectile() && IsAutomaticWeapon()))) {
                pVATS.SetFirstProjectile(plastProjectile);
            }
            if (bfirst) {
                bfirst = false;
            }
            else {
                plastProjectile->SetProjectileTarget(nullptr);
            }
        }
        if (!bshotIsFake) {
            ++irealShots;
        }
        if (pvatsCommand && (pvatsCommand->cFireShots || pvatsCommand->cFakeFireShots) &&
            ActionPoints::IsAttackAction(pvatsCommand->eAction)) {
            if (bshotIsFake && pvatsCommand->cFakeFireShots) {
                --pvatsCommand->cFakeFireShots;
            }
            else {
                int32_t iammoPerShot = GetAmmoPerShot();
                if (iammoPerShot < 1) {
                    iammoPerShot = 1;
                }
                pvatsCommand->cFireShots = static_cast<uint8_t>(pvatsCommand->cFireShots - iammoPerShot);
            }
            // The burst time is read through the first Setting::Float pointer after BSMAX returns.
            float* pfburstTime = &fVATSShotBurstTime.Float();
            float flongBurstTime = fVATSShotLongBurstTime.Float();
            float fburstTime = fVATSShotBurstTime.Float();
            float fmaxBurstTime = BSMAX(fburstTime, flongBurstTime);
            float frealRatio = *pfburstTime / fmaxBurstTime;
            fVATSShotBurstFraction = fVATSShotBurstFraction + frealRatio;
            if (frealRatio <= fVATSShotBurstFraction) {
                fVATSShotBurstFraction = static_cast<float>(fVATSShotBurstFraction - 1.0);
            }
            if (!bshotIsFake && !pvatsCommand->cFireShots) {
                pVATS.SetLastProjectile(plastProjectile);
            }
        }
        --cnumShots;
    }

    if (pactor) {
        // One shot in four is free with the ammo-regeneration mod.
        int32_t ifreeShots = 0;
        if (pactor == pPlayer) {
            BaseProcess* pProc = pPlayer->GetCurrentAIProcess();
            if (pProc && pProc->GetCurrentWeapon()) {
                float fvalue = 0.0f;
                if (pProc->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_AMMO_REGEN_SHOT, fvalue)) {
                    pPlayer->SetShotsSinceLastAmmoRegenTick(
                        static_cast<uint8_t>(pPlayer->GetShotsSinceLastAmmoRegenTick() + irealShots)
                    );
                    if (pPlayer->GetShotsSinceLastAmmoRegenTick() >= 4) {
                        pPlayer->SetShotsSinceLastAmmoRegenTick(
                            static_cast<uint8_t>(pPlayer->GetShotsSinceLastAmmoRegenTick() - 4)
                        );
                        ifreeShots = 1;
                    }
                }
            }
        }
        pactor->UseAmmo(irealShots - ifreeShots);
    }
    // pactorAIProcess is null-checked only for the grenade type, as the game does.
    if ((pactorAIProcess && GetWeaponType() == WEAPON_ONE_HAND_GRENADE) || GetWeaponType() == WEAPON_ONE_HAND_MINE ||
        GetWeaponType() == WEAPON_ONE_HAND_LUNCHBOX_MINE || GetWeaponType() == WEAPON_ONE_HAND_THROWN) {
        if (pactorAIProcess->GetCurrentAmmo() && pactorAIProcess->GetCurrentAmmo()->GetCount() > 0) {
            if (pfireObject) {
                NiAVObject* pchild = pfireObject->IsNode()->GetAtChecked(0);
                if (pchild) {
                    pchild->SetAppCulled(true);
                }
            }
        }
        else {
            pactor->RemoveWeapon();
        }
    }
    spcache = nullptr;
    int32_t eType = GetWeaponType();
    if (eType >= WEAPON_ONE_HAND_PISTOL && eType <= WEAPON_TWO_HAND_LAUNCHER && pactor && pactor->IsPc() &&
        pactorAIProcess && pactorAIProcess->GetCurrentWeapon() && !IsEmbeddedWeapon()) {
        float fweaponDamage = CombatFormulas::CalcDamageToWeapon(pactor);
        BaseProcess* pProc = pactor->GetCurrentAIProcess();
        pactor->DamageEquipment(pProc->GetCurrentWeapon(), fweaponDamage, false);
    }
    if (IsAutomaticWeapon()) {
        ExpelShellCasing(apSource);
    }
}

TESModel* TESObjectWEAP::GetShellCasingModel() {
    return &ShellCasingModel;
}

uint8_t TESObjectWEAP::GetIsLoopingReload() const {
    return bIsLoopingReload;
}

float TESObjectWEAP::GetSpeed() const {
    return data.fSpeed;
}

float TESObjectWEAP::GetIronSightFOV() const {
    return data.fIronFOV;
}

SpellItem* TESObjectWEAP::GetCriticalEffect() const {
    return criticalData.pEffect;
}

SpellItem* TESObjectWEAP::GetVATSSpecialAttackEffect() const {
    return specialVATSData.pVATSSpecialEffect;
}

void TESObjectWEAP::SetVATSSpecialAttackEffect(SpellItem* apVal) {
    specialVATSData.pVATSSpecialEffect = apVal;
}

TESSound* TESObjectWEAP::GetAttackFailSound() const {
    return pAttackFailSound;
}

void TESObjectWEAP::SetAttackFailSound(TESSound* apSnd) {
    pAttackFailSound = apSnd;
}

float TESObjectWEAP::GetSemiAutomaticFireDelayMin() const {
    return data.fSemiAutomaticFireDelayMin;
}

float TESObjectWEAP::GetSemiAutomaticFireDelayMax() const {
    return data.fSemiAutomaticFireDelayMax;
}

float TESObjectWEAP::GetAnimShotsPerSecond() const {
    return data.fAnimShotsPerSecond;
}

float TESObjectWEAP::GetAnimJamTime() const {
    return data.fAnimJamTime;
}

float TESObjectWEAP::GetCriticalChanceMult() const {
    return criticalData.fCriticalChanceMult;
}

ActorValue::Index TESObjectWEAP::GetWeaponSkill() const {
    return data.eSkill;
}

float TESObjectWEAP::GetIronSightUseMult() const {
    return data.fIronSightUseMult;
}

WEAPON_TYPE TESObjectWEAP::GetWeaponType() const {
    return static_cast<WEAPON_TYPE>(data.eType);
}

uint16_t TESObjectWEAP::GetCriticalDamage() const {
    return uint16_t(criticalData.sCriticalDamage);
}

// The explosion's damage and radius are compared against the settings so that only a strictly greater (ordered) value
// passes.
COMBAT_WEAPON_TYPE TESObjectWEAP::GetCombatWeaponType() {
    COMBAT_WEAPON_TYPE eResult;
    switch (GetWeaponType()) {
        case WEAPON_HAND_TO_HAND:
        case WEAPON_ONE_HAND_MELEE:
        case WEAPON_TWO_HAND_MELEE: eResult = COMBAT_WEAPON_MELEE; break;
        case WEAPON_ONE_HAND_PISTOL:
        case WEAPON_ONE_HAND_PISTOL_ENERGY:
        case WEAPON_TWO_HAND_RIFLE:
        case WEAPON_TWO_HAND_AUTOMATIC:
        case WEAPON_TWO_HAND_RIFLE_ENERGY:
        case WEAPON_TWO_HAND_HANDLE:
        case WEAPON_TWO_HAND_LAUNCHER:
        case WEAPON_ONE_HAND_THROWN: {
            BGSProjectile* pProjectile = GetProjectile(nullptr);
            eResult = COMBAT_WEAPON_RANGED;
            if (pProjectile && pProjectile->GetExplosion()) {
                double fDamage = pProjectile->GetExplosion()->GetDamage();
                if (fDangerousProjectileExplosionDamage.Float() < fDamage) {
                    double fRadius = pProjectile->GetExplosion()->GetRadiusBSUnits();
                    if (fDangerousProjectileExplosionRadius.Float() < fRadius) {
                        eResult = COMBAT_WEAPON_RANGED_EXPLOSIVE;
                    }
                }
            }
            break;
        }
        case WEAPON_ONE_HAND_GRENADE: eResult = COMBAT_WEAPON_GRENADE; break;
        case WEAPON_ONE_HAND_MINE:
        case WEAPON_ONE_HAND_LUNCHBOX_MINE: eResult = COMBAT_WEAPON_MINE; break;
        default:
            Warning(
                "COMBAT: Unknown weapon type %i encountered in function TESObjectWEAP::GetCombatWeaponType().",
                GetWeaponType()
            );
            eResult = COMBAT_WEAPON_MELEE;
            break;
    }
    return eResult;
}

void TESObjectWEAP::SetAttackSound(TESSound* apSnd) {
    pAttackSound = apSnd;
}

void TESObjectWEAP::SetAttackSound2(TESSound* apSnd) {
    pAttackSound2 = apSnd;
}

void TESObjectWEAP::SetModAttackSound(TESSound* apSnd) {
    pModSilencedSound = apSnd;
}

void TESObjectWEAP::SetIdleSound(TESSound* apSnd) {
    pIdleSound = apSnd;
}

void TESObjectWEAP::SetEquipSound(TESSound* apSnd) {
    pEquipSound = apSnd;
}

void TESObjectWEAP::SetUnEquipSound(TESSound* apSnd) {
    pUnEquipSound = apSnd;
}

void TESObjectWEAP::Set1stPersonObject(TESObjectSTAT* apObject) {
    p1stPersonObject = apObject;
}

void TESObjectWEAP::SetImpactDataSet(BGSImpactDataSet* apSet) {
    pImpactDataSet = apSet;
}

void TESObjectWEAP::SetAttackLoopSound(TESSound* apSound) {
    pAttackLoop = apSound;
}

bool TESObjectWEAP::GetPreventUseInNormalCombat() const {
    return (data.iFlagsEx & WEAPON_FLAG_EX_PREVENT_USE_IN_NORMAL_COMBAT) != 0;
}

bool TESObjectWEAP::GetNPCsUseAmmo() const {
    return (data.iFlagsEx & WEAPON_FLAG_EX_NPCS_USE_AMMO) != 0;
}

bool TESObjectWEAP::GetIsPlayerOnly() const {
    return (data.iFlagsEx & WEAPON_FLAG_EX_PLAYER_ONLY) != 0;
}

bool TESObjectWEAP::IsAutomaticWeapon() const {
    return (data.cFlags & WEAPON_FLAG_AUTOMATIC) != 0;
}

bool TESObjectWEAP::GetBurstShot() const {
    return (data.iFlagsEx & WEAPON_FLAG_EX_BURST_SHOT) != 0;
}

bool TESObjectWEAP::GetIsPlayable() const {
    return (data.cFlags & WEAPON_FLAG_NOT_PLAYABLE) == 0;
}

const char* TESObjectWEAP::GetEmbeddedWeaponNode() {
    if (IsEmbeddedWeapon()) {
        return EmbeddedWeaponNode.pString;
    }

    return nullptr;
}

ActorValue::Index TESObjectWEAP::GetEmbeddedWeaponConditionValue() {
    int32_t v3 = (-1);
    if (IsEmbeddedWeapon()) {
        v3 = data.cEmbeddedConditionValue + 25;
        if (v3 >= 32) {
            return static_cast<ActorValue::Index>(31);
        }
    }
    return static_cast<ActorValue::Index>(v3);
}

void TESObjectWEAP::Set1stPersonModObject(uint8_t iModSlots, TESObjectSTAT* apObject) {
    if (iModSlots & MOD_ONE) {
        if (iModSlots & MOD_TWO) {
            if (iModSlots & MOD_THREE) {
                p1stPersonMod123Object = apObject;
            }
            else {
                p1stPersonMod12Object = apObject;
            }
        }
        else if (iModSlots & MOD_THREE) {
            p1stPersonMod13Object = apObject;
        }
        else {
            p1stPersonMod1Object = apObject;
        }
    }
    else if (iModSlots & MOD_TWO) {
        if (iModSlots & MOD_THREE) {
            p1stPersonMod23Object = apObject;
        }
        else {
            p1stPersonMod2Object = apObject;
        }
    }
    else if (iModSlots & MOD_THREE) {
        p1stPersonMod3Object = apObject;
    }
}

bool TESObjectWEAP::HasScope() {
    return ((data.cFlags & 4) != 0);
}

TESModel* TESObjectWEAP::GetScopeModel() {
    return &ScopeModel;
}

bool TESObjectWEAP::HasModScope() {
    return ((((int32_t)data.iFlagsEx) & 8192) != 0);
}

TESObjectSTAT* TESObjectWEAP::Get1stPersonObject() {
    return this->p1stPersonObject;
}

TESObjectSTAT* TESObjectWEAP::Get1stPersonModObject(uint8_t iModSlots) {
    if ((iModSlots & 1) != 0) {
        if ((iModSlots & 2) != 0) {
            if ((iModSlots & 4) != 0) {
                return p1stPersonMod123Object;
            }

            return p1stPersonMod12Object;
        }

        if ((iModSlots & 4) != 0) {
            return p1stPersonMod13Object;
        }

        return p1stPersonMod1Object;
    }

    if ((iModSlots & 2) != 0) {
        if ((iModSlots & 4) != 0) {
            return p1stPersonMod23Object;
        }

        return p1stPersonMod2Object;
    }

    if ((iModSlots & 4) != 0) {
        return p1stPersonMod3Object;
    }

    return p1stPersonObject;
}

TESModelTextureSwap* TESObjectWEAP::GetModTESModel(uint8_t iSlots, bool abNoHandToHand) {
    TESModelTextureSwap* p_WorldModelMod123 = nullptr;
    if (GetWeaponType() || abNoHandToHand) {
        if ((iSlots & 1) != 0) {
            if ((iSlots & 2) != 0) {
                if ((iSlots & 4) != 0) {
                    if (*(&WorldModelMod123)->GetModel()) {
                        p_WorldModelMod123 = &WorldModelMod123;
                    }
                }
                else {
                    if (*(&WorldModelMod12)->GetModel()) {
                        p_WorldModelMod123 = &WorldModelMod12;
                    }
                }
            }
            else {
                if ((iSlots & 4) != 0) {
                    if (*(&WorldModelMod13)->GetModel()) {
                        p_WorldModelMod123 = &WorldModelMod13;
                    }
                }
                else {
                    if (*(&WorldModelMod1)->GetModel()) {
                        p_WorldModelMod123 = &WorldModelMod1;
                    }
                }
            }
        }
        else {
            if ((iSlots & 2) != 0) {
                if ((iSlots & 4) != 0) {
                    if (*(&WorldModelMod23)->GetModel()) {
                        p_WorldModelMod123 = &WorldModelMod23;
                    }
                }
                else {
                    if (*(&WorldModelMod2)->GetModel()) {
                        p_WorldModelMod123 = &WorldModelMod2;
                    }
                }
            }
            else {
                if (((iSlots & 4) != 0) && (*(&WorldModelMod3)->GetModel())) {
                    p_WorldModelMod123 = &WorldModelMod3;
                }
            }
        }
        if ((!p_WorldModelMod123) || (TESForm::GetFormID() == 1521108)) {
            if (this) {
                return static_cast<TESModelTextureSwap*>(this);
            }

            return nullptr;
        }
        return p_WorldModelMod123;
    }

    if (*(&WorldModel)->GetModel()) {
        return static_cast<TESModelTextureSwap*>(&WorldModel);
    }

    if (this) {
        return static_cast<TESModelTextureSwap*>(this);
    }

    return nullptr;
}

TESSound* TESObjectWEAP::GetAttackSound(bool bModSound) const {
    return bModSound ? pModSilencedSound : pAttackSound;
}

TESSound* TESObjectWEAP::GetAttackSound2(bool bModSound) const {
    return bModSound ? pModSilencedSound2 : pAttackSound2;
}

TESSound* TESObjectWEAP::GetAttackSound2D(bool bModSound) const {
    return bModSound ? pModSilencedSound2D : pAttackSound2D;
}

bool TESObjectWEAP::IsRangedWeapon() const {
    return data.eType >= WEAPON_ONE_HAND_PISTOL && data.eType <= WEAPON_ONE_HAND_THROWN;
}

bool TESObjectWEAP::IsGunWeapon() const {
    return data.eType >= WEAPON_ONE_HAND_PISTOL && data.eType <= WEAPON_TWO_HAND_LAUNCHER;
}

bool TESObjectWEAP::IsThrownWeapon() const {
    return IsRangedWeapon() && !IsGunWeapon();
}

void TESObjectWEAP::SetItemMod(int32_t aiModSlot, TESObjectIMOD* apItemMod) {
    if (aiModSlot == MOD_ONE) {
        pModObjectOne = apItemMod;
    }
    else if (aiModSlot == MOD_TWO) {
        pModObjectTwo = apItemMod;
    }
    else if (aiModSlot == MOD_THREE) {
        pModObjectThree = apItemMod;
    }
}

WEAPON_MOD_EFFECT TESObjectWEAP::GetModEffect(uint32_t iModSlot) const {
    if (iModSlot == MOD_ONE) {
        return data.eModActionOne;
    }
    if (iModSlot == MOD_TWO) {
        return data.eModActionTwo;
    }
    if (iModSlot == MOD_THREE) {
        return data.eModActionThree;
    }
    return static_cast<WEAPON_MOD_EFFECT>(0);
}

// Mod effect 8 is the weapon's attack multiplier bonus.
float TESObjectWEAP::GetAttackSpeed(bool abModValue) const {
    if (abModValue) {
        return const_cast<TESObjectWEAP*>(this)->GetModEffectValue(static_cast<WEAPON_MOD_EFFECT>(8), 0) +
            data.fAttackMult;
    }
    return data.fAttackMult;
}

ANIM_GROUP_ENUM TESObjectWEAP::GetReloadAnim(bool bModValue) const {
    if (bModValue) {
        // The byte is sign-extended and compared with 0xFF, so the test always holds and the mod's value is always
        // taken, as the game does.
        if (static_cast<int32_t>(static_cast<int8_t>(data.iModReloadClipAnimation)) != 0xFF) {
            return static_cast<ANIM_GROUP_ENUM>(int8_t(data.iModReloadClipAnimation) + ANIM_GROUP_RELOAD_A);
        }
        return static_cast<ANIM_GROUP_ENUM>(data.cReloadAnim + ANIM_GROUP_RELOAD_A);
    }
    return static_cast<ANIM_GROUP_ENUM>(data.cReloadAnim + ANIM_GROUP_RELOAD_A);
}

// Mod effect 2 is the clip size bonus, truncated to a byte.
int32_t TESObjectWEAP::GetFormClipRounds(bool bModValue) const {
    if (bModValue) {
        uint8_t ucModRounds =
            uint8_t(int32_t(const_cast<TESObjectWEAP*>(this)->GetModEffectValue(static_cast<WEAPON_MOD_EFFECT>(2), 0)));
        return cClipRounds + ucModRounds;
    }
    return cClipRounds;
}

TESModel* TESObjectWEAP::GetWorldTESModel() {
    TESModel* pModel = nullptr;
    // Called through a pointer so that clang keeps the virtual call, as the game does, instead of devirtualising the
    // member's call.
    if (*(&WorldModel)->GetModel() != '\0') {
        pModel = &WorldModel;
    }
    if (pModel == nullptr) {
        pModel = this ? static_cast<TESModelTextureSwap*>(this) : nullptr;
    }
    return pModel;
}

BGSProjectile* TESObjectWEAP::GetProjectile(Actor* apWeaponHolder) {
    TESForm* pAmmo = GetCurrentAmmo(apWeaponHolder);
    if (pAmmo) {
        BGSProjectile* pAmmoProjectile = static_cast<TESAmmo*>(pAmmo)->GetProjectile();
        if (pAmmoProjectile) {
            return pAmmoProjectile;
        }
    }
    return data.pProjectile;
}

// Converts from the actor's ActorValueOwner subobject back to the actor (null stays null).
BGSProjectile* TESObjectWEAP::GetProjectile_(ActorValueOwner* apWeaponHolder) {
    return GetProjectile(static_cast<Actor*>(apWeaponHolder));
}

void TESObjectWEAP::SetWeaponType(WEAPON_TYPE aeType) {
    data.eType = (int8_t)((int32_t)aeType);
}

const char* TESObjectWEAP::GetFireNodeName() {
    if (IsEmbeddedWeapon() && (GetEmbeddedWeaponNode() && *GetEmbeddedWeaponNode())) {
        return GetEmbeddedWeaponNode();
    }

    return "ProjectileNode";
}

float TESObjectWEAP::GetModEffectValue(WEAPON_MOD_EFFECT eEffect, uint8_t cValue) {
    if (data.eModActionOne != eEffect) {
        if (data.eModActionTwo == eEffect) {
            if (!cValue) {
                return data.fModActionTwoValue;
            }
            if (cValue == 1) {
                return data.fModActionTwoValueTwo;
            }
        }
        else {
            if (data.eModActionThree != eEffect) {
                return 0.0;
            }
            if (!cValue) {
                return data.fModActionThreeValue;
            }
            if (cValue == 1) {
                return data.fModActionThreeValueTwo;
            }
        }
        return 0.0;
    }
    if (cValue) {
        if (cValue == 1) {
            return data.fModActionOneValueTwo;
        }
        return 0.0;
    }
    return data.fModActionOneValue;
}

TESAmmo* TESObjectWEAP::GetCurrentAmmo(Actor* apWeaponHolder) {
    MobileObject* pHolder = apWeaponHolder;
    if (pHolder && pHolder->GetCurrentAIProcess() && pHolder->GetCurrentAIProcess()->GetCurrentAmmo()) {
        TESBoundObject* pAmmoObject = pHolder->GetCurrentAIProcess()->GetCurrentAmmo()->GetContainerObject();
        TESBoundObject* pWeaponObject = nullptr;
        if (pHolder->GetCurrentAIProcess()->GetCurrentWeapon()) {
            pWeaponObject = pHolder->GetCurrentAIProcess()->GetCurrentWeapon()->GetContainerObject();
        }
        if (pAmmoObject != pWeaponObject && dynamic_cast<TESAmmo*>(pAmmoObject)) {
            return static_cast<TESAmmo*>(pAmmoObject);
        }
    }
    return static_cast<TESAmmo*>(BGSAmmoForm::GetAmmoHelper());
}

// The ammo's count is tested as a signed compare on its zero-extended byte, so any nonzero count passes. The split beam
// mod value is added and truncated to an int whose low byte is kept.
uint8_t TESObjectWEAP::GetNumProjectiles(bool bUseModValue, bool bIgnoreAmmo, Actor* apWeaponHolder) {
    uint8_t cNumProjectiles = data.cNumProjectiles;
    if (!bIgnoreAmmo && GetCurrentAmmo(apWeaponHolder) && GetCurrentAmmo(apWeaponHolder)->GetNumProjectiles() > 0) {
        uint8_t cAmmoProjectiles = GetCurrentAmmo(apWeaponHolder)->GetNumProjectiles();
        cNumProjectiles = uint8_t(cAmmoProjectiles * GetAmmoPerShot());
    }
    if (bUseModValue) {
        cNumProjectiles = uint8_t(int32_t(GetModEffectValue(WEAPON_MOD_SPLIT_BEAM, 0) + cNumProjectiles));
    }
    return cNumProjectiles;
}

uint8_t TESObjectWEAP::GetAmmoPerShot() {
    return data.cAmmoPerShot;
}

// The minimum spread; with bModValue, less the spread mods' decrease, at least 0.
float TESObjectWEAP::GetMinSpreadDegrees(bool bModValue) {
    if (bModValue) {
        float fMinSpread = data.fMinSpread - GetModEffectValue(WEAPON_MOD_DECREASE_SPREAD, 0);
        return BSMAX(0.0f, fMinSpread);
    }
    return data.fMinSpread;
}

// The spread; with bModValue, less the spread mods' decrease, at least 0.
float TESObjectWEAP::GetSpreadDegrees(bool bModValue) {
    if (bModValue) {
        float fSpread = data.fSpread - GetModEffectValue(WEAPON_MOD_DECREASE_SPREAD, 0);
        return BSMAX(0.0f, fSpread);
    }
    return data.fSpread;
}

float TESObjectWEAP::GetAmmoRegenRate(bool bModValue) {
    if (bModValue) {
        return (GetModEffectValue(static_cast<WEAPON_MOD_EFFECT>(6), 0) + data.fAmmoRegenRate);
    }

    return data.fAmmoRegenRate;
}

bool TESObjectWEAP::IsMeleeWeapon() {
    return (bool)((uint8_t)(((uint32_t)((int32_t)((uint8_t)data.eType))) <= 2u));
}

float TESObjectWEAP::GetMinRange() {
    return data.fMinRange;
}

bool TESObjectWEAP::GetIsRangeFixed() {
    return ((((int32_t)data.iFlagsEx) & 32) != 0);
}

ANIM_GROUP_ENUM TESObjectWEAP::GetAttackAnim() {
    return static_cast<ANIM_GROUP_ENUM>(data.cAttackAnim);
}

uint8_t TESObjectWEAP::GetHandGripAnim() const {
    return data.cHandGripAnim;
}

TESSound* TESObjectWEAP::GetBlockSound() const {
    return pMeleeBlockSound;
}

TESSound* TESObjectWEAP::GetUnequipSound() const {
    return pUnEquipSound;
}

float TESObjectWEAP::GetMaxRange() {
    return data.fMaxRange;
}

uint32_t TESObjectWEAP::GetStrRequired() const {
    return data.iStrengthRequirement;
}

float TESObjectWEAP::GetVATSAP() const {
    return specialVATSData.fVATSSkillRequired;
}

float TESObjectWEAP::GetVATSSkill() const {
    return specialVATSData.fVATSSpecialAP;
}

TESSound* TESObjectWEAP::Get3DLoopingShotSound() const {
    return pAttackLoop;
}

float TESObjectWEAP::GetAimArc() {
    return data.fAimArc;
}

TESObjectIMOD* TESObjectWEAP::GetModItem(uint32_t iModSlot) {
    switch (iModSlot) {
        case 1u: {
            return pModObjectOne;
        }
        case 2u: {
            return pModObjectTwo;
        }
        case 4u: {
            return pModObjectThree;
        }
    }
    return nullptr;
}

void TESObjectWEAP::SetScopeEffect(TESEffectShader* apEffect) {
    pScopeEffect = apEffect;
}

void TESObjectWEAP::SetNPCsUseAmmo(bool abVal) {
    uint32_t iFlagsEx;
    uint32_t v3;
    if (abVal) {
        data.iFlagsEx |= 2u;
        iFlagsEx = data.iFlagsEx;
    }
    else {
        data.iFlagsEx &= (~2u);
        v3 = data.iFlagsEx;
    }
    uint32_t v4 = data.iFlagsEx;
}

void TESObjectWEAP::SetNumProjectiles(uint8_t acVal) {
    data.cNumProjectiles = acVal;
}

bool TESObjectWEAP::IsTwoHandedWeapon() {
    int8_t result;
    switch ((uint8_t)data.eType) {
        case 2u:
        case 5u:
        case 6u:
        case 8u:
        case 9u: {
            result = 1;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return (bool)result;
}

uint8_t TESObjectWEAP::GetVATSToHitChance(bool bModValue) {
    if (bModValue) {
        return (uint8_t)(data.cVATSToHitChance + ((int32_t)GetModEffectValue(static_cast<WEAPON_MOD_EFFECT>(13), 0)));
    }

    return data.cVATSToHitChance;
}

bool TESObjectWEAP::GetCantDrop() {
    return ((data.cFlags & 8) != 0);
}

bool TESObjectWEAP::GetNoJamAfterReload() {
    return ((data.iFlagsEx & 4) != 0);
}

bool TESObjectWEAP::GetIsMinorCrime() {
    return ((((int32_t)data.iFlagsEx) & 16) != 0);
}

bool TESObjectWEAP::HasNightScope() {
    return ((data.iFlagsEx & 4096) != 0);
}

const char* TESObjectWEAP::GetVATSAttackName() {
    return VATSAttackName.pString;
}

bool TESObjectWEAP::IsActionPointOverride() {
    return ((((int32_t)data.iFlagsEx) & 8) != 0);
}

void TESObjectWEAP::SetMeleeBlockSound(TESSound* apSnd) {
    pMeleeBlockSound = apSnd;
}

uint32_t TESObjectWEAP::GetModifiedSoundLevel(bool abSilent, bool abSilencer) {
    int32_t v5 = 0;
    int32_t uiSoundLevel = (int32_t)eSoundLevel;
    if (abSilent) {
        uiSoundLevel = 2;
    }
    if (abSilencer) {
        uiSoundLevel = 1;
    }
    switch (uiSoundLevel) {
        case 0: {
            return (&iSoundLevelLoud)->UInt();
        }
        case 1: {
            return (&iSoundLevelNormal)->UInt();
        }
        case 2: {
            return (&iSoundLevelSilent)->UInt();
        }
    }
    return (uint32_t)v5;
}

BGSImpactData* TESObjectWEAP::GetImpactDataForMaterial(HavokMaterialType Material) {
    int32_t v3 = 0;
    if (pImpactDataSet) {
        return pImpactDataSet->GetImpactDataForMaterial(Material);
    }
    return reinterpret_cast<BGSImpactData*>(static_cast<uintptr_t>(v3));
}

BGSImpactDataSet* TESObjectWEAP::GetImpactDataSet() {
    return pImpactDataSet;
}

void TESObjectWEAP::SetCriticalEffect(SpellItem* apVal) {
    criticalData.pEffect = reinterpret_cast<SpellItem*>(reinterpret_cast<uintptr_t>(apVal));
}

int32_t TESObjectWEAP::GetPowerAttackAnimOverride() {
    return (int32_t)((uint8_t)((int32_t)data.cPowerAttackOverrideAnim));
}

NiAVObject* TESObjectWEAP::GetFireNode(NiAVObject* apObject) {
    const char* blockName = GetFireNodeName();
    NiNode* objectByName = static_cast<NiNode*>(BSUtilities::GetObjectByName(apObject, blockName));
    if (!objectByName) {
        return BSUtilities::GetObjectByName(apObject, "##ProjectileNode");
    }
    return objectByName;
}

// The release point: the actor's location raised by its height, then moved to the weapon bone for thrown weapons (types
// 10 to 13) or to the fire node otherwise. GetWeaponType is called once per type tested, stopping at the first match.
// The reference is taken as the player character, as the callers pass it.
NiPoint3 TESObjectWEAP::CalculateReleasePoint(TESObjectREFR* apSource) {
    PlayerCharacter* apActor = static_cast<PlayerCharacter*>(apSource);
    Actor* pActor = apActor;
    PlayerCharacter* pActorRef = pActor->IsActor() ? apActor : nullptr;
    BaseProcess* pProcess = pActorRef ? pActorRef->GetCurrentAIProcess() : nullptr;
    bool bIsPlayer = (pActorRef == pPlayer);
    NiPoint3 kReleasePoint = *apActor->QLocationOnReference();
    if (pActorRef) {
        kReleasePoint.z += pActorRef->GetHeight();
    }
    NiAVObject* pFireNode = nullptr;
    if (pActorRef && pProcess &&
        (GetWeaponType() == 10 || GetWeaponType() == 11 || GetWeaponType() == 12 || GetWeaponType() == 13)) {
        BipedAnim* pBiped;
        if (bIsPlayer) {
            pBiped = pPlayer->GetBiped(pPlayer->Is3rdPerson());
        }
        else {
            pBiped = pActorRef->GetCurrentBiped();
        }
        NiNode* pWeaponBone = pProcess->GetWeaponBone(pBiped);
        if (pWeaponBone) {
            kReleasePoint = pWeaponBone->GetWorldTranslate();
        }
    }
    else {
        if (pProcess) {
            pFireNode = pActor->GetFireNode();
        }
        else {
            NiAVObject* pNode;
            if (bIsPlayer) {
                pNode = pPlayer->GetCurrent3D();
            }
            else {
                pNode = pActor->Get3D();
            }
            pFireNode = GetFireNode(pNode);
        }
        if (pFireNode) {
            kReleasePoint = pFireNode->GetWorldTranslate();
        }
    }
    return NiPoint3(kReleasePoint);
}

double TESObjectWEAP::GetModValue(int32_t a1, char a2) {
    switch (a1) {
        case 1: {
            if (!a2) {
                return data.fModActionOneValue;
            }
            if (a2 == 1) {
                return data.fModActionOneValueTwo;
            }
            break;
        }
        case 2: {
            if (!a2) {
                return data.fModActionTwoValue;
            }
            if (a2 == 1) {
                return data.fModActionTwoValueTwo;
            }
            break;
        }
        case 4: {
            if (!a2) {
                return data.fModActionThreeValue;
            }
            if (a2 == 1) {
                return data.fModActionThreeValueTwo;
            }
            break;
        }
    }
    return 0.0;
}

uint8_t TESObjectWEAP::GetCritDamageFlags() {
    return criticalData.bEffectOnDeath;
}

SOUND_LEVEL TESObjectWEAP::GetSoundLevel(char a1) {
    if (a1) {
        return SOUND_LEVEL_SILENT;
    }

    return eSoundLevel;
}

TESObjectREFR* UseWeaponActorPackageData::GetAttackTarget() {
    return pAttackTarget;
}

uint8_t TESAmmo::GetNumProjectiles() {
    return NVdata.cProjectilePerAmmo;
}

NiFixedString& FixedStrings::QLaserSightsName() {
    return *pLaserSight;
}

NiFixedString& FixedStrings::QAimSightsName() {
    return *pAimSight;
}
