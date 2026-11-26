#include "Fallout/AI/ArrowProjectile.h"

#include "BSCore/BSTimer.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkForceController.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyT.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESArrowTargetListener.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/bhkCharacterListenerArrow.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/rtti.h"

#include <limits>

ArrowProjectile::CollisionData::CollisionData() {}

ArrowProjectile::ArrowProjectile()
  : psCollisionData(nullptr), bNeedsFinishInitLoadGame(false), bAddedToInventory(false), bPostForce(false),
    iLoadGameFlags(0) {
    SetStillLoading(true);
    pCurrentProcess = new HighProcess();
    ++iArrowCount;
}

ArrowProjectile::~ArrowProjectile() {
    --iArrowCount;
    ReferenceProcessLists.ProjectilePostProcessList.Remove(static_cast<MobileObject*>(this));
    NiAVObject* p3D = TESObjectREFR::Get3D();
    if (p3D && psCollisionData && psCollisionData->eCollisionType == 1) {
        bhkCollisionObject* pCol = FindFirstCollisionObject(p3D);
        bhkRigidBody* pArrowRB = pCol ? pCol->GetRigidBody() : nullptr;
        if (pArrowRB) {
            hkVector4 centerOfMass;
            pArrowRB->GetCenterOfMassInWorld(centerOfMass);
            TESArrowTargetListener::RemoveArrowTarget(centerOfMass, psCollisionData->pClosestObject);
        }
    }
    if (!bNeedsFinishInitLoadGame &&
        psCollisionData &&
        !psCollisionData->eCollisionType &&
        psCollisionData->pCollidee &&
        psCollisionData->pCollidee->IsActor()) {
        MobileObject* pActor = static_cast<Actor*>(psCollisionData->pCollidee);
        if (pActor->pCurrentProcess) {
            pActor->pCurrentProcess->RemoveAttachedArrow(this);
        }
    }
    ourFree(psCollisionData);
    TESObjectREFR::Set3D(nullptr, true);
    if (!pDataHandler->GetClearingData()) {
        CleanUpOtherMobileObjects(false);
    }
}

NiAVObject* ArrowProjectile::Load3D(bool abBackgroundLoading) {
    NiFixedString kName;
    NiAVObject* p3D = MobileObject::Load3D(abBackgroundLoading);
    if (!p3D) {
        return p3D;
    }
    ::new (static_cast<void*>(&kName)) NiFixedString(const_cast<char*>("Arrow"));
    p3D->SetName(kName);
    return p3D;
}

void ArrowProjectile::Revert(uint32_t aiFlags) {
    MobileObject::Revert(aiFlags);
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        if (psCollisionData) {
            ourFree(psCollisionData);
        }
        psCollisionData = nullptr;
    }
}

uint16_t ArrowProjectile::GetSaveSize(uint32_t aiFlags) {
    uint16_t iSize = MobileObject::GetSaveSize(aiFlags);
    uint16_t iBaseSize = iSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iSize += 6;
    }
    iSize = iSize + 49;
    if (psCollisionData) {
        iSize += 40;
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 80u) {
            iSize += 16;
        }
        if (psCollisionData->eCollisionType <= 1) {
            iSize += 8;
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 84u) {
        ++iSize;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 85u) {
        ++iSize;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                iSize - iBaseSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                2174,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                iSize - iBaseSize,
                2174,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp"
            );
        }
    }
    return iSize;
}

// Before FinishInitLoadGame has run on a loaded arrow, pClosestObject still holds the packed collision object count
// (high word) and index (low word) LoadGame read, which are saved back as they are.
void ArrowProjectile::SaveGame(uint32_t aiFlags) {
    uint32_t iCollideeID;
    NiQuaternion kQuat;
    uint32_t iBlockHeader;
    uint32_t iEnchantmentID;
    uint32_t iPoisonID;
    uint16_t iBlockSize;
    uint32_t iShooterID;
    bool bSaveCollisionData;
    MobileObject::SaveGame(aiFlags);
    iBlockSize = 0;
    uint16_t* pBlockSize = nullptr;
    uint8_t* pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iBlockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockHeader, sizeof(iBlockHeader));
        pBlockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockSize, sizeof(iBlockSize));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&eState, sizeof(eState));
    SaveGameDataOLD(&fFadeAlpha, sizeof(fFadeAlpha));
    SaveGameDataOLD(&fAge, sizeof(fAge));
    SaveGameDataOLD(&fSpeed, sizeof(fSpeed));
    SaveGameDataOLD(&fDamage, sizeof(fDamage));
    SaveGameDataOLD(&fWaterSpeed, sizeof(fWaterSpeed));
    SaveGameDataOLD(&DeltaS, sizeof(DeltaS));
    iShooterID = 0;
    if (pShooter) {
        iShooterID = pShooter->iFormID;
    }
    SaveNumericID(&iShooterID, 4);
    iEnchantmentID = 0;
    if (pEnchantment) {
        iEnchantmentID = pEnchantment->iFormID;
    }
    SaveNumericID(&iEnchantmentID, 4);
    iPoisonID = 0;
    if (pPoison) {
        iPoisonID = pPoison->iFormID;
    }
    SaveNumericID(&iPoisonID, 4);
    bSaveCollisionData = psCollisionData != nullptr;
    SaveGameDataOLD(&bSaveCollisionData, sizeof(bSaveCollisionData));
    if (bSaveCollisionData) {
        pSaveLoadGameOLD->SaveGameDataOLD(psCollisionData, 4u);
        SaveGameDataOLD(&psCollisionData->kPoint, sizeof(psCollisionData->kPoint));
        SaveGameDataOLD(&psCollisionData->kNormal, sizeof(psCollisionData->kNormal));
        SaveGameDataOLD(&psCollisionData->kVelocity, sizeof(psCollisionData->kVelocity));
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 80u) {
            kQuat.FromRotation(psCollisionData->kRotation);
            pSaveLoadGameOLD->SaveGameDataOLD(&kQuat, sizeof(kQuat));
        }
        if (psCollisionData->eCollisionType <= 1) {
            uint16_t iCollisionObjectCount;
            uint16_t iCollisionObjectIndex;
            iCollideeID = 0;
            if (psCollisionData->pCollidee) {
                iCollideeID = psCollisionData->pCollidee->iFormID;
            }
            SaveNumericID(&iCollideeID, 4);
            iCollisionObjectCount = 0;
            iCollisionObjectIndex = 0xFFFF;
            if (psCollisionData->pCollidee && psCollisionData->pClosestObject) {
                if (bNeedsFinishInitLoadGame) {
                    uint32_t iClosestObject = static_cast<uint32_t>(
                        reinterpret_cast<uintptr_t>(psCollisionData->pClosestObject)
                    );
                    iCollisionObjectCount = static_cast<uint16_t>(iClosestObject >> 16);
                    iCollisionObjectIndex = static_cast<uint16_t>(iClosestObject);
                }
                else {
                    bool bIgnoreWeaponBoneAndFadeNodes = psCollisionData->eCollisionType == 0;
                    NiNode* pCollidee3D = psCollisionData->pCollidee->TESObjectREFR::Get3D();
                    iCollisionObjectCount = GetCollisionObjectCountInSceneGraph(
                        pCollidee3D,
                        bIgnoreWeaponBoneAndFadeNodes,
                        bIgnoreWeaponBoneAndFadeNodes,
                        true
                    );
                    bhkNiCollisionObject* pCol = bhkNiCollisionObject::GetbhkNiCollisionObject(
                        psCollisionData->pClosestObject
                    );
                    pCollidee3D = psCollisionData->pCollidee->TESObjectREFR::Get3D();
                    iCollisionObjectIndex = GetCollisionObjectIndexInSceneGraph(
                        pCollidee3D,
                        pCol,
                        bIgnoreWeaponBoneAndFadeNodes,
                        bIgnoreWeaponBoneAndFadeNodes,
                        true
                    );
                }
            }

            SaveGameDataOLD(&iCollisionObjectCount, sizeof(iCollisionObjectCount));
            SaveGameDataOLD(&iCollisionObjectIndex, sizeof(iCollisionObjectIndex));
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 84u) {
        SaveGameDataOLD(&bAddedToInventory, sizeof(bAddedToInventory));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 85u) {
        SaveGameDataOLD(&bBowIgnoresWeaponsResist, sizeof(bBowIgnoresWeaponsResist));
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pBufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t iFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                pHeader->iFormID,
                pEditorID,
                iFlags,
                2282,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                2282,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block's size: the buffer's advance past the size word, compared as unsigned addresses.
        char* pBufferNow = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        char* pBlockStart = static_cast<char*>(static_cast<void*>(pBlockSize));
        if (pBufferNow > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp",
                2282
            );
        }
        *pBlockSize = static_cast<uint16_t>(pBufferNow - pBlockStart);
    }
}

// The form IDs are kept in the pointer fields until InitLoadGame, and pClosestObject holds the packed collision object
// count (high word) and index (low word, sign-extended) until FinishInitLoadGame (before version 32: the object's name,
// a string this function allocates).
void ArrowProjectile::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    uint8_t iNameLength;
    uint16_t iCollisionObjectCount;
    NiQuaternion kQuat;
    uint32_t iBlockHeader;
    uint32_t iEnchantmentID;
    uint32_t iPoisonID;
    uint16_t iBlockSize;
    uint32_t iShooterID;
    char bHasCollisionData;
    MobileObject::LoadGame(aiFlags, aiCurrentFlags);
    iBlockSize = 0;
    uint8_t* pStartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iBlockHeader, sizeof(iBlockHeader));
        if (iBlockHeader != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp",
                    2293,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp",
                    2293,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pStartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iBlockSize, sizeof(iBlockSize));
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&eState, sizeof(eState));
    LoadGameDataOLD(&fFadeAlpha, sizeof(fFadeAlpha));
    LoadGameDataOLD(&fAge, sizeof(fAge));
    LoadGameDataOLD(&fSpeed, sizeof(fSpeed));
    LoadGameDataOLD(&fDamage, sizeof(fDamage));
    LoadGameDataOLD(&fWaterSpeed, sizeof(fWaterSpeed));
    LoadGameDataOLD(&DeltaS, sizeof(DeltaS));
    LoadNumericID(&iShooterID, 4);
    pShooter = reinterpret_cast<Actor*>(static_cast<uintptr_t>(iShooterID));
    LoadNumericID(&iEnchantmentID, 4);
    pEnchantment = reinterpret_cast<EnchantmentItem*>(static_cast<uintptr_t>(iEnchantmentID));
    LoadNumericID(&iPoisonID, 4);
    pPoison = reinterpret_cast<AlchemyItem*>(static_cast<uintptr_t>(iPoisonID));
    if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
        pSaveLoadGameOLD->AdvanceBuffer(8u);
    }
    bHasCollisionData = 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u) {
        LoadGameDataOLD(&bHasCollisionData, sizeof(bHasCollisionData));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
        bHasCollisionData = eState == ArrowProjectile::STATE_COLLISION ||
            eState == ArrowProjectile::STATE_POST_COLLISION;
    }
    if (bHasCollisionData) {
        psCollisionData = new ArrowProjectile::CollisionData;
        psCollisionData->pClosestObject = nullptr;
        psCollisionData->pCollidee = nullptr;
        psCollisionData->kRotation = NiMatrix3::IDENTITY;
        pSaveLoadGameOLD->LoadGameDataOLD(psCollisionData, 4u);
        LoadGameDataOLD(&psCollisionData->kPoint, sizeof(psCollisionData->kPoint));
        LoadGameDataOLD(&psCollisionData->kNormal, sizeof(psCollisionData->kNormal));
        LoadGameDataOLD(&psCollisionData->kVelocity, sizeof(psCollisionData->kVelocity));
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 80u) {
            pSaveLoadGameOLD->LoadGameDataOLD(&kQuat, sizeof(kQuat));
            kQuat.ToRotation(psCollisionData->kRotation);
        }
        if (psCollisionData->eCollisionType <= 1) {
            uint32_t iCollideeID;
            LoadNumericID(&iCollideeID, 4);
            psCollisionData->pCollidee = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(iCollideeID));
            if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u) {
                int16_t iCollisionObjectIndex;
                LoadGameDataOLD(&iCollisionObjectCount, sizeof(iCollisionObjectCount));
                LoadGameDataOLD(&iCollisionObjectIndex, sizeof(iCollisionObjectIndex));
                uint32_t iClosestObject = (static_cast<uint32_t>(iCollisionObjectCount) << 16) +
                    static_cast<uint32_t>(static_cast<int32_t>(iCollisionObjectIndex));
                psCollisionData->pClosestObject = reinterpret_cast<NiAVObject*>(static_cast<uintptr_t>(iClosestObject));
            }
            if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
                LoadGameDataOLD(&iNameLength, sizeof(iNameLength));
                char* pString = static_cast<char*>(ourMalloc(iNameLength + 1));
                ourMemset(pString, 0, iNameLength + 1);
                LoadGameDataOLD(pString, iNameLength);
                psCollisionData->pClosestObject = reinterpret_cast<NiAVObject*>(pString);
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 84u) {
        LoadGameDataOLD(&bAddedToInventory, sizeof(bAddedToInventory));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 85u) {
        LoadGameDataOLD(&bBowIgnoresWeaponsResist, sizeof(bBowIgnoresWeaponsResist));
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pCurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pCurrentBufferPosition <= pStartBufferPosition + iBlockSize) {
                if (pCurrentBufferPosition < pStartBufferPosition + iBlockSize) {
                    uint32_t iFlags = pHeader->iFlags;
                    int32_t iVersion = pHeader->cVersion;
                    const char* pEditorID = pForm->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(pStartBufferPosition + iBlockSize - pCurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp",
                        2405,
                        pHeader->iFormID,
                        pEditorID,
                        iVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pHeader->iFlags;
                int32_t iVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pCurrentBufferPosition - (pStartBufferPosition + iBlockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp",
                    2405,
                    pHeader->iFormID,
                    pEditorID,
                    iVersion,
                    iFlags
                );
            }
        }
        else if (pCurrentBufferPosition <= pStartBufferPosition + iBlockSize) {
            if (pCurrentBufferPosition < pStartBufferPosition + iBlockSize) {
                int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    static_cast<int32_t>(pStartBufferPosition + iBlockSize - pCurrentBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp",
                    2405,
                    iCurrentVersion
                );
            }
        }
        else {
            int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pCurrentBufferPosition - (pStartBufferPosition + iBlockSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp",
                2405,
                iCurrentVersion
            );
        }
    }
}

void ArrowProjectile::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    MobileObject::InitLoadGame(aiFlags, aiOldFlags);
    uint32_t iShooterID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pShooter));
    if (iShooterID) {
        TESForm* pForm = TESForm::GetFormByNumericID(iShooterID);
        pShooter = dynamic_cast<Actor*>(pForm);
    }
    uint32_t iEnchantmentID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pEnchantment));
    if (iEnchantmentID) {
        TESForm* pForm = TESForm::GetFormByNumericID(iEnchantmentID);
        pEnchantment = dynamic_cast<EnchantmentItem*>(pForm);
    }
    uint32_t iPoisonID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPoison));
    if (iPoisonID) {
        TESForm* pForm = TESForm::GetFormByNumericID(iPoisonID);
        pPoison = dynamic_cast<AlchemyItem*>(pForm);
    }
    if (psCollisionData && psCollisionData->eCollisionType <= 1) {
        uint32_t iCollideeID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(psCollisionData->pCollidee));
        if (iCollideeID) {
            TESForm* pForm = TESForm::GetFormByNumericID(iCollideeID);
            psCollisionData->pCollidee = dynamic_cast<TESObjectREFR*>(pForm);
        }
    }

    PROCESS_TYPE eProcessType = GetCurrentProcessType();
    ReferenceProcessLists.RemoveReference(this, eProcessType);
    eProcessType = GetCurrentProcessType();
    ReferenceProcessLists.AddReference(this, eProcessType, false, false, nullptr);
}

// pClosestObject holds what LoadGame read: the packed collision object count (high word) and index, or before
// version 32 the object's name, which is freed here.
void ArrowProjectile::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    SetStillLoading(false);
    TESObjectREFR::Get3D();
    if (TESObjectREFR::Get3D()) {
        InitHavok(pShooter, 1.0f);
        MobileObject::FinishInitLoadGame(aiFlags, aiOldFlags);
        if (psCollisionData && psCollisionData->eCollisionType <= 1) {
            if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u) {
                bool bIgnoreWeaponBoneAndFadeNodes = psCollisionData->eCollisionType == 0;
                int32_t iCollisionObjectIndex = static_cast<int32_t>(
                    reinterpret_cast<uintptr_t>(psCollisionData->pClosestObject)
                );
                psCollisionData->pClosestObject = nullptr;
                if (psCollisionData->pCollidee) {
                    NiNode* pCollidee3D = psCollisionData->pCollidee->TESObjectREFR::Get3D();
                    if (static_cast<uint16_t>(iCollisionObjectIndex >> 16) ==
                        static_cast<uint16_t>(GetCollisionObjectCountInSceneGraph(
                            pCollidee3D,
                            bIgnoreWeaponBoneAndFadeNodes,
                            bIgnoreWeaponBoneAndFadeNodes,
                            true
                        ))) {
                        if (iCollisionObjectIndex != -1) {
                            pCollidee3D = psCollisionData->pCollidee->TESObjectREFR::Get3D();
                            bhkNiCollisionObject* pCol = GetCollisionObjectByIndexInSceneGraph(
                                pCollidee3D,
                                iCollisionObjectIndex,
                                bIgnoreWeaponBoneAndFadeNodes,
                                bIgnoreWeaponBoneAndFadeNodes,
                                true
                            );
                            if (pCol) {
                                psCollisionData->pClosestObject = pCol->m_pkSceneObject;
                            }
                        }
                    }
                    else {
                        int32_t iCollideeID = psCollisionData->pCollidee->iFormID;
                        Warning(
                            "AI: Collision object count has changed on object %s %08",
                            psCollisionData->pCollidee->GetFormEditorID(),
                            iCollideeID
                        );
                    }
                }
            }
            if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
                char* pString = reinterpret_cast<char*>(psCollisionData->pClosestObject);
                if (psCollisionData->pCollidee) {
                    NiNode* pCollidee3D = psCollisionData->pCollidee->TESObjectREFR::Get3D();
                    psCollisionData->pClosestObject = BSUtilities::GetObjectByName(
                        static_cast<NiAVObject*>(pCollidee3D),
                        pString
                    );
                }
                else {
                    psCollisionData->pClosestObject = nullptr;
                }
                ourFree(pString);
            }
            if (!psCollisionData->pClosestObject) {
                psCollisionData->eCollisionType = 3;
            }
        }

        pSaveLoadGameOLD->QueueFormForFinalization(this);
    }
    else if (
        psCollisionData &&
        psCollisionData->pCollidee &&
        psCollisionData->pCollidee->GetDisabled() &&
        psCollisionData->pCollidee->GetDelete()
    ) {
        SetDelete(true);
        psCollisionData->pClosestObject = nullptr;
    }
    else {
        bNeedsFinishInitLoadGame = true;
        iLoadGameFlags = aiFlags;
    }
}

bool ArrowProjectile::IsProjectile() {
    return true;
}

bool ArrowProjectile::MovetoHigh() {
    if (pCurrentProcess && !pCurrentProcess->eLevel) {
        return true;
    }
    ReferenceProcessLists.RemoveReference(this, pCurrentProcess->eLevel);
    HighProcess* pProcess = new HighProcess;
    pProcess->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = pProcess;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, false, false, nullptr);
    MoveHavok(false);
    pCurrentProcess->SetLastSeenLocation();
    return true;
}

bool ArrowProjectile::MovetoMiddleHigh() {
    if (pCurrentProcess && pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH) {
        return true;
    }
    ReferenceProcessLists.RemoveReference(this, pCurrentProcess->eLevel);
    MiddleHighProcess* pProcess = new MiddleHighProcess;
    pProcess->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = pProcess;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_HIGH, false, false, nullptr);
    MoveHavok(false);
    pCurrentProcess->SetLastSeenLocation();
    return true;
}

void ArrowProjectile::Process(float afDelta) {
    float fSeconds = appTimer.GetSeconds();
    Update(fSeconds);
}

bool ArrowProjectile::ChangeProcessLevel() {
    bool bChange = true;
    bool bHighProcessOnly = false;
    PROCESS_TYPE eOldLevel = pCurrentProcess->eLevel;
    PROCESS_TYPE eNewLevel = PROCESS_TYPE_LOW;
    TESObjectCELL* pCell = GetParentCell();
    if (eState == ArrowProjectile::STATE_POST_COLLISION && psCollisionData && psCollisionData->pCollidee) {
        if (psCollisionData->pCollidee == pPlayer) {
            if (eOldLevel) {
                MovetoHigh();
            }
            return true;
        }
        pCell = psCollisionData->pCollidee->GetParentCell();
    }
    if (Get3D() || pModelLoader->IsReferenceQueued(this)) {
        if (TES::pInstance->IsCellLoaded(pCell, true)) {
            eNewLevel = PROCESS_TYPE_HIGH;
        }
        else if (TES::pInstance->IsCellLoaded(pCell, false)) {
            eNewLevel = PROCESS_TYPE_MIDDLE_HIGH;
        }
    }
    if (eState == ArrowProjectile::STATE_FLYING) {
        bHighProcessOnly = true;
    }
    if (eOldLevel == eNewLevel) {
        return true;
    }
    if (eNewLevel) {
        if (eNewLevel != PROCESS_TYPE_MIDDLE_HIGH || bHighProcessOnly) {
            if (this) {
                delete this;
            }
            return false;
        }

        MovetoMiddleHigh();
    }
    else {
        MovetoHigh();
    }
    return bChange;
}

bool ArrowProjectile::IsTransient() {
    return true;
}

void ArrowProjectile::Update(float afDelta) {
    NiMatrix3 rot;
    if (bNeedsFinishInitLoadGame) {
        bool bReady = true;
        if (psCollisionData && psCollisionData->eCollisionType <= 1 && psCollisionData->pCollidee) {
            bReady = psCollisionData->pCollidee->TESObjectREFR::Get3D() != nullptr;
        }

        if (bReady) {
            bNeedsFinishInitLoadGame = false;
            FinishInitLoadGame(iLoadGameFlags, 0);
            pSaveLoadGameOLD->FinalizeQueuedForms();
        }
    }
    NiAVObject* p3D = Get3D();
    if (afDelta >= 0.0) {
        fAge = fAge + afDelta;
        if (fArrowAgeMax.Float() < static_cast<double>(fAge)) {
            eState = ArrowProjectile::STATE_DELETING;
        }
        if (GetDelete()) {
            return;
        }
        if (eState || !GetCharController()) {
            if (eState == ArrowProjectile::STATE_POST_COLLISION && psCollisionData) {
                uint32_t eColType = psCollisionData->eCollisionType;
                if (p3D && !eColType) {
                    SetLocationOnReference(p3D->GetWorldTranslate());
                    TESObjectCELL* pCell = nullptr;
                    if (psCollisionData->pCollidee) {
                        pCell = psCollisionData->pCollidee->GetParentCell();
                    }
                    if (pCell != GetParentCell()) {
                        if (pCell) {
                            ReferenceProcessLists.AddActorToTempChangeList(this);
                        }
                        else {
                            SetDelete(true);
                        }
                    }
                }

                if (p3D &&
                    eColType == 1 &&
                    (!psCollisionData ||
                     !psCollisionData->pCollidee ||
                     !psCollisionData->pCollidee->TESObjectREFR::Get3D())) {
                    SetDelete(true);
                }
            }
            else if (eState == ArrowProjectile::STATE_DELETING) {
                fFadeAlpha = fFadeAlpha - afDelta / 3.0;
                if (fFadeAlpha <= 0.0) {
                    fFadeAlpha = 0.0f;
                    SetDelete(true);
                }
                if (p3D) {
                    BSShaderUtil::RecursiveSetPropertyFadeAlpha(static_cast<NiNode*>(p3D), fFadeAlpha);
                }
            }
        }
        else {
            float fDistance = fSpeed * afDelta;
            NiPoint3 rdeltaMove(0.0f, fDistance, 0.0f);
            NiPoint3 oldLoc = GetLocationOnReference();
            bhkCharacterController* pController = GetCharController();
            float fHavokWaterHeight = pController->QWaterHeight();
            double dWaterHeight = HK2NI(fHavokWaterHeight);
            if (GetLocationOnReference().z <= dWaterHeight) {
                float fRatio = (fWaterSpeed - afDelta * 1.0) / fWaterSpeed;
                fSpeed = fSpeed * fRatio;
                fDamage = fDamage * fRatio;
                pController = GetCharController();
                pController->SetFallTime(fRatio);
                fWaterSpeed = fWaterSpeed - afDelta * 1.0;
                if (fWaterSpeed < 0.5) {
                    Simulate(nullptr);
                    ProcessCollision();
                }
            }
            Move(afDelta, rdeltaMove, 15u);
            if (eState) {
                ArrowProjectile::ArrowState eNextState = static_cast<ArrowProjectile::ArrowState>(
                    (eState == ArrowProjectile::STATE_DELETING) + 2
                );
                if (ProcessCollision()) {
                    if (p3D) {
                        SetLocationOnReference(p3D->GetTranslate());
                    }
                    eState = eNextState;
                    return;
                }
            }
            NiPoint3 newLoc = GetLocationOnReference();
            if (p3D) {
                float fy;
                float fz;
                float fx;
                p3D->SetTranslate(newLoc);
                rot = p3D->m_kLocal.m_Rotate;
                rot.ToEulerAnglesZXY(fz, fx, fy);
                DeltaS = newLoc - oldLoc;
                float fDeltaZ = DeltaS.z;
                float fLength = DeltaS.Length();
                float fSinPitch = fDeltaZ / fLength;
                fx = -NiASin(fSinPitch);
                rot.FromEulerAnglesZXY(fz, fx, fy);
                p3D->SetRotate(rot);
            }
        }
    }
    if (p3D) {
        NiUpdateData kUpdateData(afDelta, true, false);
        p3D->Update(kUpdateData);
    }
}

void ArrowProjectile::SetState(ArrowState aeState) {
    eState = aeState;
}

// Gives a high-process arrow a character controller (a 0.1-unit sphere in the projectile layer, in the shooter's
// collision group or else the arrow group) when it has none, then puts the controller in the cell's world at the
// arrow's position, 8 units higher when it changes worlds.
void ArrowProjectile::InitHavok(Actor* apShooter, float afPower) {
    if (!pCurrentProcess || pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_HIGH) {
        return;
    }
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    TESObjectCELL* pCell = GetParentCell();
    bhkWorld* pWorld = pCell ? pCell->GetbhkWorld() : nullptr;
    if (!pController) {
        alignas(16) uint8_t aCinfoMemory[sizeof(bhkCharacterControllerCinfo)];
        bhkCharacterControllerCinfo* pCinfo = ::new (aCinfoMemory) bhkCharacterControllerCinfo(true);
        pCinfo->SetAcrobatics(0.0f);
        if (apShooter) {
            // 50.0f is the marksman constant.
            pCinfo->SetGravity(
                CombatFormulas::GetArrowGravity(
                    afPower,
                    50.0f,
                    static_cast<ActorValueOwner*>(apShooter)->GetClampedActorValue(ActorValue::LUCK)
                )
            );
        }
        pCinfo->SetObject(Get3D());
        pCinfo->SetPosition(GetLocationOnReference());
        pCinfo->SetWorld(pWorld);
        // NiNew bhkSphereShape: 0x14 bytes.
        bhkSphereShape* pShape = new bhkSphereShape(0.1f, true);
        pCinfo->SetShape(pShape);
        pCinfo->SetAvoidBox(false);
        if (apShooter) {
            alignas(16) CFilter kFilter;
            static_cast<MobileObject*>(apShooter)->GetCollisionFilter(&kFilter.iFilter);
            pCinfo->SetColFilter(L_PROJECTILE, kFilter.GetGroup());
        }
        else {
            pCinfo->SetColFilter(L_PROJECTILE, ArrowProjectile::GetArrowGroup());
        }
        pCinfo->SetStartState(HK_CHARACTER_USER_STATE_1);
        pController = new bhkCharacterListenerArrow(*pCinfo, this);
        pController->SetFallTime(1.0f);
        pController->SetQuickSimulate(Main::GetSceneIsComplex());
        pController->SetIsProjectile(true);
        pCurrentProcess->SetCharController(NiPointer<bhkCharacterController>(pController));
        pController->SetMOBObject(this);
        pCinfo->~bhkCharacterControllerCinfo();
    }
    if (!pController) {
        return;
    }
    if (pCell && pWorld) {
        pController->SetWaterHeight(pCell->GetWaterHeight());
    }
    else {
        pController->SetWaterHeight(-std::numeric_limits<float>::max());
    }
    bhkWorld* pOldWorld = pController->GetWorld();
    pController->SetWorld(pWorld);
    NiNode* p3D = Get3D();
    NiAVObject* pOldObject = pController->GetNiObject();
    bool bAddToWorld = true;
    if (pOldObject != p3D) {
        pController->SetNiObject(p3D);
        if (pWorld && pWorld->QDebugDisplay()) {
            pController->BuildDisplayGeometry(nullptr);
            BSShaderUtil::RecursiveSetPropertyWireframe(pController->GetNiObject(), 1);
        }
        bhkWorld::SetMotion(p3D, hkpMotion::MOTION_KEYFRAMED, true, false, true);
        bAddToWorld = true;
    }
    alignas(16) NiPoint3 kPosition = GetLocationOnReference();
    if (pOldWorld != pWorld) {
        if (pOldWorld) {
            bhkWorld::RemoveObjects(p3D, true, false);
        }
        kPosition.z = static_cast<float>(kPosition.z + 8.0);
        bAddToWorld = true;
    }
    if (!pWorld) {
        return;
    }
    pController->SetPosition(kPosition);
    if (bAddToWorld) {
        CFilter kFilter;
        pController->GetCollisionFilter(&kFilter);
        uint32_t uiGroup = kFilter.GetGroup();
        pWorld->AddObjects(p3D, true, false, uiGroup, false);
    }
    else {
        bhkWorld::UpdatePosition(p3D, true, false);
    }
}

bool ArrowProjectile::GetAddedToInventory() {
    return bAddedToInventory;
}

bool ArrowProjectile::GetPostForce() {
    return bPostForce;
}

void ArrowProjectile::SetPostForce(bool abValue) {
    bPostForce = abValue;
}

void ArrowProjectile::ClearAttachedRef() {
    if (psCollisionData) {
        psCollisionData->pCollidee = nullptr;
    }
}

TESObjectREFR* ArrowProjectile::GetAttachedRef() {
    if (psCollisionData) {
        return psCollisionData->pCollidee;
    }

    return nullptr;
}

float ArrowProjectile::GetSpeed() {
    return fSpeed;
}

// Pushes the body the arrow hit once, scaled by the arrow's speed, the body's layer and mass.
void ArrowProjectile::ApplyCollisionForce(NiPoint3 aContactPoint, NiPoint3 aDirection, bhkRigidBody* apTargetBody) {
    if (!apTargetBody || GetPostForce()) {
        return;
    }
    // Slot 38 (+0x98).
    ahkpWorld* pWorld = static_cast<ahkpWorld*>(apTargetBody->GethkWorldRw());
    if (pWorld) {
        float fImpulse = GetSpeed();
        CFilter kFilter;
        apTargetBody->GetCollisionFilter(&kFilter);
        COL_LAYER eLayer = kFilter.GetLayer();
        if (eLayer == L_TRAP) {
            fImpulse = fImpulse * fProjectileKnockMultTrap.Float();
        }
        else if (eLayer == L_BIPED) {
            fImpulse = fImpulse * fProjectileKnockMultBiped.Float();
        }
        else if (eLayer == L_PROPS) {
            fImpulse = fImpulse * fProjectileKnockMultProp.Float();
        }
        else {
            fImpulse = fImpulse * fProjectileKnockMultClutter.Float();
        }
        float fMass = apTargetBody->GetMass();
        if (fProjectileKnockMinMass.Float() > fMass) {
            fImpulse = fMass / fProjectileKnockMinMass.Float() * fImpulse;
        }
        fImpulse = fImpulse * fProjectileCollisionImpulseScale.Float();
        hkVector4 kImpulse;
        NI2HK(kImpulse, aDirection);
        hkVector4 kPoint;
        NI2HK(kPoint, aContactPoint);
        kImpulse.normalize3();
        alignas(16) hkSimdReal kScale(fImpulse);
        kImpulse.mul4(kScale);
        pWorld->markForWrite();
        static_cast<hkpRigidBody*>(**apTargetBody)->applyPointImpulse(kImpulse, kPoint);
        pWorld->unmarkForWrite();
    }
    SetPostForce(true);
}

char ArrowProjectile::ProcessCollisionTerrain(bhkRigidBody* apBody) {
    CFilter kFilter;
    DetachCharController();
    NiPoint3* pPoint = &psCollisionData->kPoint;
    NiNode* p3D = Get3D();
    p3D->SetTranslate(*pPoint);
    TESObjectREFR::SetLocationOnReference(psCollisionData->kPoint);
    apBody->GetCollisionFilter(&kFilter);
    kFilter.SetLayer(L_NONCOLLIDABLE);
    kFilter.SetDisabled(0);
    apBody->SetCollisionFilter(kFilter);
    int32_t iArrowGroup = ArrowProjectile::GetArrowGroup();
    apBody->SetCollisionFilter(CFilter(L_NONCOLLIDABLE, iArrowGroup, P_OTHER));
    return 1;
}

bool ArrowProjectile::ProcessCollisionActor(bhkRigidBody* apArrowRB) {
    NiNode* pNode = Get3D();
    DetachCharController();
    if (psCollisionData->pCollidee && psCollisionData->pCollidee->IsActor()) {
        NiNode* pClosestObject = static_cast<NiNode*>(psCollisionData->pClosestObject);
        if (pClosestObject) {
            bhkWorld::RemoveObjects(pNode, true, false);
            TESHavokUtilities::RemoveHavokFromSceneGraph(pNode);
            pNode->SetTranslate(psCollisionData->kPoint);
            pNode->SetRotate(psCollisionData->kRotation);
            double dLocalScale = pNode->GetLocalScale();
            float fScale = static_cast<float>(dLocalScale / pClosestObject->GetWorldScale());
            pNode->SetScale(fScale);
            ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
            pShadowSceneNode->RemoveObject(pNode);
            pClosestObject->AttachChild(pNode, true);
            pShadowSceneNode->AddObject(pNode);
            Actor* pCollidee = static_cast<Actor*>(psCollisionData->pCollidee);
            if (pCollidee && pCollidee->MobileObject::GetCurrentAIProcess()) {
                BaseProcess* pProcess = pCollidee->MobileObject::GetCurrentAIProcess();
                pProcess->AddAttachedArrow(this);
            }

            return true;
        }

        return false;
    }

    Warning("AI: An arrow thinks it is colliding with an Actor, but there is no Actor in the collision data!");
    return false;
}

bool ArrowProjectile::ProcessSimulate(bhkRigidBody* apArrowRB) {
    DetachCharController();
    NiNode* p3D = Get3D();
    bhkWorld::SetMotion(p3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
    Actor* pActor = dynamic_cast<Actor*>(psCollisionData->pCollidee);
    if (pActor && pActor->GetCharController()) {
        bhkCharacterController* pCharController = pActor->GetCharController();
        uint32_t uiFilterGroup = pCharController->GetFilterGroup();
        apArrowRB->SetCollisionFilter(CFilter(L_CLUTTER, uiFilterGroup, P_OTHER));
    }
    else {
        int32_t iArrowGroup = static_cast<int32_t>(ArrowProjectile::GetArrowGroup());
        apArrowRB->SetCollisionFilter(CFilter(L_CLUTTER, static_cast<uint32_t>(iArrowGroup), P_OTHER));
    }
    apArrowRB->SetLinearVelocity(psCollisionData->kVelocity);
    hkpRigidBody* pRigidBody = apArrowRB->Gethk();
    bhkCollisionObject* pCollisionObject = bhkCollisionObject::Getbhk(pRigidBody);
    if (pCollisionObject) {
        pCollisionObject->SetNotify(true);
    }
    return true;
}

void ArrowProjectile::CollideActor(const NiPoint3& aContactPoint, const NiPoint3& aContactNormal, Actor* apCollidee) {
    hkSphere hitSphere;
    this->psCollisionData = new ArrowProjectile::CollisionData;
    this->psCollisionData->eCollisionType = 0;
    this->psCollisionData->pClosestObject = nullptr;
    this->psCollisionData->pCollidee = nullptr;
    NiPoint3* pNormal = &this->psCollisionData->kNormal;
    pNormal->x = aContactNormal.x;
    pNormal->y = aContactNormal.y;
    pNormal->z = aContactNormal.z;
    this->psCollisionData->kPoint = aContactPoint;
    this->psCollisionData->kRotation = NiMatrix3::IDENTITY;
    this->psCollisionData->kVelocity = NiPoint3::ZERO;
    if (this->GetCharController()) {
        bhkCharacterController* pController = this->GetCharController();
        hkVector4 velocity(pController->GethkLinearVelocity());
        if (-std::numeric_limits<float>::max() == velocity.hkVector4::getSimdAt(0)) {
            if (this->Get3D()) {
                NiPoint3 kForward = NiPoint3::UNIT_Y * this->fSpeed;
                NiNode* p3D = this->Get3D();
                this->psCollisionData->kVelocity = p3D->GetWorldTransform().m_Rotate * kForward;
            }
        }
        else {
            HK2NI(this->psCollisionData->kVelocity, velocity);
        }
    }
    this->eState = ArrowProjectile::STATE_COLLISION;
    if (apCollidee) {
        bhkRigidBody* pTmpBody;
        hkVector4 pos;
        alignas(16) NiMatrix3 tmpRot;
        NiPoint3 dir;
        NiNode* pRoot;
        NiNode* pArrow3D = this->Get3D();
        if (this->pShooter && this->pShooter->GetCombatController()) {
            // Gets the collidee's position before the second GetCombatController call.
            const NiPoint3& rCollideePos = apCollidee->GetLocationOnReference();
            this->pShooter->GetCombatController()->ProjectileHitObstruction(rCollideePos, true);
        }

        this->psCollisionData->pCollidee = apCollidee;
        apCollidee->ClearLastHitData();
        ExtraDataList* pCollideeExtra = &this->psCollisionData->pCollidee->m_Extra;
        Script::SetActionFlag(this->GetObjectReference(), pCollideeExtra, SCRIPT_ACTION_ONHITWITH);
        if (this->pShooter && (this->pShooter == pPlayer || this->pShooter->IsInCombat())) {
            bool bWasDead = apCollidee->IsDead(false);
            if (!bWasDead && apCollidee->IsDead(false) && (!apCollidee->IsCreature() || apCollidee->HasRagDoll())) {
                NiAVObject* pCollidee3D = apCollidee->TESObjectREFR::Get3D();
                if (pCollidee3D) {
                    hkVector4 loc;
                    float fMagnitude = CombatFormulas::GetDeathForce(fDeathForceRangedDamageMax.Float(), true);
                    bhkForceController::RemoveForce(pCollidee3D);
                    bhkWorld::SetMotion(pCollidee3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
                    NiUpdateData kUpdateData(0.0f, false, false);
                    pCollidee3D->Update(kUpdateData);
                    alignas(16) NiPoint3 moveBack = pArrow3D->GetWorldTransform().m_Rotate * NiPoint3::UNIT_Y;
                    moveBack.Unitize();
                    NiPoint3 kOffset = moveBack * -1024.0f;
                    moveBack = this->GetLocationOnReference() + kOffset;
                    NI2HK(loc, moveBack);
                    TESHavokUtilities::AddExplosionImpulse(pCollidee3D, loc, fMagnitude, nullptr);
                }
            }
        }

        bhkRigidBody* pRigidBody = nullptr;
        if (apCollidee == pPlayer) {
            pRoot = static_cast<NiNode*>(pPlayer->Get3D(false));
        }
        else {
            pRoot = apCollidee->TESObjectREFR::Get3D();
        }
        float fpickLength = 512.0f;
        NiMatrix3 rotation;
        float fHeading = this->GetHeading(false);
        rotation.MakeZRotation(fHeading);
        float fLooking = this->GetLooking();
        tmpRot.MakeXRotation(fLooking);
        rotation = rotation * tmpRot;
        rotation.GetCol(1u, dir);
        NiPoint3::UnitizeVector(dir);
        alignas(16) NiPoint3 kHalfPick = dir * fpickLength / 2.0f;
        NiPoint3 origin = this->GetLocationOnReference() - kHalfPick;
        char bPicked = 0;
        bool bIgnoreHead = !apCollidee->IsDead(true);
        NiPoint3 kPickDirection = dir;
        NiPoint3 kPickFrom = origin;
        NiNode* pCollidee3D = apCollidee->Get3D();
        NiAVObject* pTargetObject = ArrowProjectile::PickSceneGraph(
            pCollidee3D,
            kPickFrom,
            kPickDirection,
            fpickLength,
            bIgnoreHead
        );
        if (pTargetObject) {
            alignas(16) NiMatrix3 invertRot;
            float fMoveLength = fpickLength + 18.0;
            alignas(16) NiPoint3 move = dir * fMoveLength;
            NiTransform invertTrans;
            pTargetObject->GetWorldTransform().Invert(invertTrans);
            this->psCollisionData->kPoint = invertTrans * (origin + move);
            pTargetObject->GetWorldTransform().m_Rotate.Inverse(invertRot);
            this->psCollisionData->kRotation = invertRot * pArrow3D->GetWorldTransform().m_Rotate;
            this->psCollisionData->pClosestObject = pTargetObject;
            bhkCollisionObject* pCol = FindFirstCollisionObject(pTargetObject);
            if (pCol) {
                pRigidBody = pCol->GetRigidBody();
            }
            bPicked = 1;
        }
        bool bHitHigh = false;
        if (apCollidee->GetBlocked() &&
            apCollidee->pCurrentProcess &&
            CombatUtilities::IsTargetWithinStrikeAngle(apCollidee, this, nullptr)) {
            NiPoint3 kCollideePos = *apCollidee->QLocationOnReference();
            float fMidHeight = apCollidee->GetHeight() / 2.0 + kCollideePos.z;
            bHitHigh = aContactPoint.z >= static_cast<double>(fMidHeight);
        }

        if (!bPicked || bHitHigh) {
            ::new (&hitSphere) NiBound;
            NI2HK(pos, aContactPoint);
            hitSphere.setPosition(pos);
            hitSphere.setRadius(200.0f);
            pTmpBody = nullptr;
            if (bHitHigh) {
                pTmpBody = static_cast<bhkRigidBody*>(bhkUtilFunctions::FindBipedPart(pRoot, 7));
            }
            else {
                bool bDead = apCollidee->IsDead(true);
                pTmpBody = bhkUtilFunctions::GetClosestBody(pRoot, hitSphere, !bDead);
            }
            if (pTmpBody) {
                pRigidBody = pTmpBody;
                bhkCollisionObject* pCol = bhkCollisionObject::Getbhk(**pTmpBody);
                if (pCol) {
                    NiMatrix3 invertRot;
                    pTargetObject = pCol->m_pkSceneObject;
                    NiPoint3 move = dir * 18.0f;
                    NiTransform invertTrans;
                    pTargetObject->GetWorldTransform().Invert(invertTrans);
                    this->psCollisionData->kPoint = invertTrans * (pTargetObject->GetWorldTranslate() + move);
                    pTargetObject->GetWorldTransform().m_Rotate.Inverse(invertRot);
                    this->psCollisionData->kRotation = invertRot * pArrow3D->GetWorldTransform().m_Rotate;
                    this->psCollisionData->pClosestObject = pTargetObject;
                }
            }
            if (!this->psCollisionData->pClosestObject) {
                this->Simulate(apCollidee);
                return;
            }
        }
        if (pRigidBody) {
            NiPoint3 kArrowDirection = pArrow3D->GetWorldTransform().m_Rotate * NiPoint3::UNIT_Y;
            this->ApplyCollisionForce(aContactPoint, kArrowDirection, pRigidBody);
        }
        if (this->psCollisionData->pClosestObject) {
            bhkCollisionObject* pCol = bhkCollisionObject::GetbhkCollisionObject(this->psCollisionData->pClosestObject);
            bhkRigidBody* pBody = pCol ? pCol->GetRigidBody() : nullptr;
            bhkShape* pShape = pBody ? pBody->GetShape() : nullptr;
            if (pShape) {
                switch (pShape->GetMaterial()) {
                    case BHK_MATERIAL_STONE:
                    case BHK_MATERIAL_GLASS:
                    case BHK_MATERIAL_METAL:
                    case BHK_MATERIAL_HEAVYSTONE:
                    case BHK_MATERIAL_HEAVYMETAL:
                    case BHK_MATERIAL_CHAIN:
                    case BHK_MATERIAL_ELEVATOR: {
                        this->Bounce(aContactPoint, aContactNormal);
                        return;
                    }
                    default: {
                        break;
                    }
                }
            }
        }
        if (!apCollidee->IsCreature() || apCollidee->HasRagDoll()) {
            if (!this->pEnchantment) {
                uint32_t uiRoll = Random();
                if (uiRoll % 100 < iArrowInventoryChance.Int()) {
                    apCollidee->AddObjecttoContainer(this->GetObjectReference(), nullptr, 1);
                    this->bAddedToInventory = true;
                }
            }
        }
        else {
            this->Simulate(apCollidee);
            this->SetState(ArrowProjectile::STATE_DELETING);
        }
    }
}

void ArrowProjectile::CollideTerrain(const NiPoint3& aContactPoint, const NiPoint3& aContactNormal) {
    this->psCollisionData = new ArrowProjectile::CollisionData;
    alignas(16) NiPoint3 move = this->DeltaS;
    NiPoint3::UnitizeVector(move);
    move *= 24.0f;
    this->psCollisionData->eCollisionType = 2;
    this->psCollisionData->pClosestObject = nullptr;
    this->psCollisionData->pCollidee = nullptr;
    this->psCollisionData->kNormal = aContactNormal;
    this->psCollisionData->kPoint = aContactPoint + move;
    this->psCollisionData->kRotation = NiMatrix3::IDENTITY;
    this->psCollisionData->kVelocity = NiPoint3::ZERO;
    if (this->GetCharController()) {
        bhkCharacterController* pController = this->GetCharController();
        hkVector4 velocity(pController->GethkLinearVelocity());
        if (-std::numeric_limits<float>::max() == velocity.hkVector4::getSimdAt(0)) {
            if (this->Get3D()) {
                NiPoint3 kForward = NiPoint3::UNIT_Y * this->fSpeed;
                NiNode* p3D = this->Get3D();
                this->psCollisionData->kVelocity = p3D->GetWorldTransform().m_Rotate * kForward;
            }
        }
        else {
            HK2NI(this->psCollisionData->kVelocity, velocity);
        }
    }
    HavokMaterialType eMaterial = TES::pInstance->GetLandMaterialType(aContactPoint);
    switch (eMaterial) {
        case BHK_MATERIAL_STONE:
        case BHK_MATERIAL_GLASS:
        case BHK_MATERIAL_METAL:
        case BHK_MATERIAL_HEAVYSTONE:
        case BHK_MATERIAL_HEAVYMETAL:
        case BHK_MATERIAL_CHAIN:
        case BHK_MATERIAL_ELEVATOR: {
            this->Bounce(aContactPoint, aContactNormal);
            break;
        }
        default: {
            break;
        }
    }
    this->eState = ArrowProjectile::STATE_COLLISION;
    NiPoint3 kContactPoint = aContactPoint;
    float fVelocity = this->psCollisionData->kVelocity.Length();
    this->PlayArrowCollisionSound(fVelocity, kContactPoint, nullptr, eMaterial);
    if (this->pShooter && this->pShooter->GetCombatController()) {
        this->pShooter->GetCombatController()->ProjectileHitObstruction(aContactPoint, false);
    }

    if (this->pShooter) {
        if (this->pEnchantment) {
            this->pShooter->SetCurrentSpell(this->pEnchantment);
            this->pShooter->SetDesiredTarget(nullptr);
            TESBoundObject* pObject = this->GetObjectReference();
            this->pShooter->ReleaseCast(pObject);
        }
        if (this->pPoison) {
            this->pShooter->SetCurrentSpell(this->pPoison);
            this->pShooter->SetDesiredTarget(nullptr);
            TESBoundObject* pObject = this->GetObjectReference();
            this->pShooter->ReleaseCast(pObject);
        }
        if (this->pBowEnchantment) {
            this->pShooter->SetCurrentSpell(this->pBowEnchantment);
            this->pShooter->SetDesiredTarget(nullptr);
            TESBoundObject* pObject = this->GetObjectReference();
            this->pShooter->ReleaseCast(pObject);
        }
    }
    if (this->pEnchantment) {
        this->SetState(ArrowProjectile::STATE_DELETING);
    }
}

void ArrowProjectile::Bounce(const NiPoint3& aContactPoint, const NiPoint3& aContactNormal) {
    if (psCollisionData) {
        if (aContactNormal != NiPoint3::ZERO) {
            NiPoint3* pNormal = &this->psCollisionData->kNormal;
            pNormal->x = aContactNormal.x;
            pNormal->y = aContactNormal.y;
            pNormal->z = aContactNormal.z;
        }
        if (aContactPoint != NiPoint3::ZERO) {
            this->psCollisionData->kPoint = aContactPoint;
        }
        this->psCollisionData->eCollisionType = 3;
        this->psCollisionData->pCollidee = nullptr;
    }
    else {
        this->psCollisionData = new ArrowProjectile::CollisionData;
        this->psCollisionData->eCollisionType = 3;
        this->psCollisionData->pClosestObject = nullptr;
        this->psCollisionData->pCollidee = nullptr;
        NiPoint3* pNormal = &this->psCollisionData->kNormal;
        pNormal->x = aContactNormal.x;
        pNormal->y = aContactNormal.y;
        pNormal->z = aContactNormal.z;
        this->psCollisionData->kPoint = aContactPoint;
        this->psCollisionData->kRotation = NiMatrix3::IDENTITY;
        this->psCollisionData->kVelocity = NiPoint3::ZERO;
        if (this->GetCharController()) {
            bhkCharacterController* pController = this->GetCharController();
            hkVector4 velocity(pController->GethkLinearVelocity());
            if (-std::numeric_limits<float>::max() == velocity.hkVector4::getSimdAt(0)) {
                if (this->Get3D()) {
                    NiPoint3 kForward = NiPoint3::UNIT_Y * this->fSpeed;
                    NiNode* p3D = this->Get3D();
                    this->psCollisionData->kVelocity = p3D->GetWorldTransform().m_Rotate * kForward;
                }
            }
            else {
                HK2NI(this->psCollisionData->kVelocity, velocity);
            }
        }
        this->eState = ArrowProjectile::STATE_COLLISION;
    }
}

void ArrowProjectile::Simulate(Actor* apCollidee) {
    if (psCollisionData) {
        this->psCollisionData->eCollisionType = 4;
    }
    else {
        this->psCollisionData = new ArrowProjectile::CollisionData;
        this->psCollisionData->eCollisionType = 4;
        this->psCollisionData->pClosestObject = nullptr;
        this->psCollisionData->pCollidee = apCollidee;
        NiPoint3* pNormal = &this->psCollisionData->kNormal;
        pNormal->x = NiPoint3::ZERO.x;
        pNormal->y = NiPoint3::ZERO.y;
        pNormal->z = NiPoint3::ZERO.z;
        this->psCollisionData->kPoint = NiPoint3::ZERO;
        this->psCollisionData->kRotation = NiMatrix3::IDENTITY;
        this->psCollisionData->kVelocity = NiPoint3::ZERO;
        if (this->GetCharController()) {
            bhkCharacterController* pController = this->GetCharController();
            hkVector4 velocity(pController->GethkLinearVelocity());
            if (-std::numeric_limits<float>::max() == velocity.hkVector4::getSimdAt(0)) {
                if (this->Get3D()) {
                    NiPoint3 kForward = NiPoint3::UNIT_Y * this->fSpeed;
                    NiNode* p3D = this->Get3D();
                    this->psCollisionData->kVelocity = p3D->GetWorldTransform().m_Rotate * kForward;
                }
            }
            else {
                HK2NI(this->psCollisionData->kVelocity, velocity);
            }
        }
    }
    this->eState = ArrowProjectile::STATE_COLLISION;
}

bool ArrowProjectile::ProcessCollision() {
    bhkCollisionObject* pCollisionObject;
    char bProcessed = 0;
    if (!psCollisionData) {
        return false;
    }
    if (Get3D()) {
        NiNode* p3D = Get3D();
        pCollisionObject = FindFirstCollisionObject(p3D);
    }
    else {
        pCollisionObject = nullptr;
    }
    if (pCollisionObject) {
        bhkRigidBody* pBody = pCollisionObject->GetRigidBody();
        switch (psCollisionData->eCollisionType) {
            case 0u: {
                bProcessed = ProcessCollisionActor(pBody);
                break;
            }
            case 1u: {
                bProcessed = ProcessCollisionReference(pBody);
                break;
            }
            case 2u: {
                bProcessed = ProcessCollisionTerrain(pBody);
                break;
            }
            case 3u: {
                bProcessed = ProcessBounce(pBody);
                break;
            }
            case 4u: {
                bProcessed = ProcessSimulate(pBody);
                break;
            }
            default: {
                break;
            }
        }
        if (!bProcessed) {
            if (psCollisionData) {
                ourFree(psCollisionData);
                psCollisionData = nullptr;
            }
            eState = STATE_FLYING;
        }
        return bProcessed;
    }

    Warning("AI: No collision data was found on this arrow");
    ourFree(psCollisionData);
    psCollisionData = nullptr;
    return false;
}

bool ArrowProjectile::ProcessCollisionReference(bhkRigidBody* apArrowRB) {
    hkVector4 centerOfMass;
    CFilter kLayerFilter;
    alignas(16) CFilter kGroupFilter;
    DetachCharController();
    if (this->psCollisionData->pCollidee) {
        bhkCollisionObject* pCol = FindFirstCollisionObject(this->psCollisionData->pClosestObject);
        if (pCol) {
            bhkRigidBody* pTargetRB = pCol->GetRigidBody();
            NiNode* p3D = this->Get3D();
            bhkWorld::SetMotion(p3D, hkpMotion::MOTION_KEYFRAMED, true, false, true);
            CFilter* pGroupFilter = pTargetRB->GetCollisionFilter(&kGroupFilter);
            uint32_t uiGroup = pGroupFilter->GetGroup();
            CFilter filter(L_PROJECTILE, uiGroup, P_OTHER);
            CFilter* pLayerFilter = pTargetRB->GetCollisionFilter(&kLayerFilter);
            if (pLayerFilter->GetLayer() != L_CLUTTER) {
                filter.SetLayer(L_NONCOLLIDABLE);
            }
            apArrowRB->SetCollisionFilter(filter);
            BSSimpleList<void*>* pList = &ReferenceProcessLists.ProjectilePostProcessList;
            if (pList) {
                pList->AddHead(static_cast<MobileObject*>(this));
            }
            if (apArrowRB && pTargetRB->GetDynamicMass() > 0.0) {
                apArrowRB->GetCenterOfMassInWorld(centerOfMass);
                TESArrowTargetListener::AddArrowTarget(centerOfMass, this->psCollisionData->pClosestObject);
            }

            return true;
        }

        return false;
    }

    Warning(
        "AI: An arrow thinks it is colliding with an Reference, but there is no Reference in the collision "
        "data!"
    );
    return false;
}

// The filter is passed by value.
bool ArrowProjectile::ProcessBounce(bhkRigidBody* apArrowRB) {
    DetachCharController();
    if (!pSaveLoadGameOLD->GetLoading()) {
        NiPoint3* pPoint = &psCollisionData->kPoint;
        NiNode* p3D = Get3D();
        p3D->SetTranslate(*pPoint);
        NiUpdateData kUpdateData(0.0f, false, false);
        p3D = Get3D();
        p3D->Update(kUpdateData);
    }
    NiNode* p3D = Get3D();
    bhkWorld::SetMotion(p3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
    uint32_t uiArrowGroup = ArrowProjectile::GetArrowGroup();
    apArrowRB->SetCollisionFilter(CFilter(L_CLUTTER, uiArrowGroup, P_OTHER));
    if (!pSaveLoadGameOLD->GetLoading()) {
        NiMatrix3 invrot;
        NiPoint3 accum = -1.0f * DeltaS;
        NiPoint3::UnitizeVector(accum);
        NiPoint3::UnitizeVector(psCollisionData->kNormal);
        float fNormalDotDirection = psCollisionData->kNormal.Dot(accum);
        NiPoint3 vton = 2.0f * (fNormalDotDirection * psCollisionData->kNormal - accum);
        accum += vton;
        float fBounceSpeed = fArrowBounceLinearSpeed.Float() * fSpeed;
        apArrowRB->SetLinearVelocity(fBounceSpeed * accum);
        p3D = Get3D();
        p3D->GetWorldTransform().m_Rotate.Inverse(invrot);
        NiPoint3 force = invrot * psCollisionData->kNormal;
        double dAbsY = NiAbs(force.y);
        if (NiAbs(force.z) < dAbsY) {
            dAbsY = NiAbs(force.y);
            force.z = NiSign(force.z) * dAbsY;
        }
        force.y = 0.0f;
        NiPoint3::UnitizeVector(force);
        float fRotateZ = -force.x * fArrowBounceRotateSpeed.Float();
        float fRotateX = fArrowBounceRotateSpeed.Float() * force.z;
        NiPoint3 angle(fRotateX, 0.0f, fRotateZ);
        p3D = Get3D();
        apArrowRB->SetAngularVelocity(p3D->GetWorldTransform().m_Rotate * angle);
    }
    p3D = Get3D();
    bhkCollisionObject* pCol = FindFirstCollisionObject(p3D);
    if (pCol) {
        pCol->SetNotify(true);
    }
    return true;
}

// Plays the collision sound of a stuck arrow's body against apBody at aLocation. The sound's location and velocity are
// constructed as a NiBound.
void ArrowProjectile::PlayArrowCollisionSound(
    float afVelocity,
    NiPoint3 aLocation,
    bhkRigidBody* apBody,
    char acMaterial
) {
    ImpactMixer::COLLISION_SOUND kSound;
    NiNode* p3D = Get3D();
    if (p3D && psCollisionData && psCollisionData->eCollisionType == 1) {
        bhkRigidBody* pArrowBody;
        bhkCollisionObject* pCollisionObject = FindFirstCollisionObject(p3D);
        if (pCollisionObject) {
            pArrowBody = pCollisionObject->GetRigidBody();
        }
        else {
            pArrowBody = nullptr;
        }
        if (pArrowBody) {
            ::new (&kSound) NiBound;
            kSound.fFriction = 0.5f;
            kSound.pBody1 = pArrowBody;
            kSound.pBody2 = apBody;
            bhkShape* pShape = pArrowBody->GetShape();
            kSound.cMaterialType1 = pShape->GetMaterial();
            kSound.cMaterialType2 = acMaterial;
            kSound.Location = aLocation;
            kSound.fVelocity = afVelocity;
            // The sound's ID is the sum of the two addresses, truncated to 32 bits.
            kSound.iID = static_cast<int32_t>(reinterpret_cast<uintptr_t>(this) + reinterpret_cast<uintptr_t>(apBody));
            ImpactMixer::PlayCollisionSound(kSound);
        }
    }
}

uint32_t ArrowProjectile::GetArrowGroup() {
    if (!ArrowProjectile::iArrowGroup) {
        ArrowProjectile::iArrowGroup = static_cast<int32_t>(bhkCollisionFilter::GetNewSystemGroup());
    }
    return ArrowProjectile::iArrowGroup;
}

int32_t ArrowProjectile::GetArrowCount() {
    return ArrowProjectile::iArrowCount;
}

void ArrowProjectile::DeleteAttachedArrows(TESObjectREFR* apRef, bool abInstant) {
    if (apRef && !apRef->IsProjectile()) {
        bool bSearchHigh = true;
        bool bSearchMiddleHigh = true;
        if (apRef->IsActor()) {
            BaseProcess* pProcess = static_cast<Actor*>(apRef)->GetCurrentAIProcess();
            if (pProcess) {
                if (pProcess->GetAttachedArrowList()) {
                    for (BSSimpleList<ArrowProjectile*>* pList = pProcess->GetAttachedArrowList();
                         pList && !pList->IsEmpty();
                         pList = pList->m_pkNext) {
                        ArrowProjectile* pArrow = pList->m_item;
                        if (pArrow) {
                            if (pArrow->GetAttachedRef() == apRef) {
                                pArrow->ClearAttachedRef();
                                if (!pArrow->GetDelete()) {
                                    if (abInstant) {
                                        pArrow->SetDelete(true);
                                    }
                                    else {
                                        pArrow->SetState(ArrowProjectile::STATE_DELETING);
                                    }
                                }
                            }
                            if (pArrow->GetDelete()) {
                                pArrow->Set3D(nullptr, true);
                            }
                        }
                    }
                    pProcess->GetAttachedArrowList()->RemoveAll();
                }
                if (pProcess->eLevel) {
                    if (pProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH) {
                        bSearchHigh = false;
                        bSearchMiddleHigh = true;
                    }
                }
                else {
                    bSearchHigh = true;
                    bSearchMiddleHigh = false;
                }
            }
        }
        if (bSearchHigh) {
            for (uint32_t i = 0; i < ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
                 ++i) {
                MobileObject* pMob = ReferenceProcessLists.MobProcessArray.GetItem(i);
                if (pMob->IsProjectile()) {
                    ArrowProjectile* pArrow = dynamic_cast<ArrowProjectile*>(pMob);
                    if (pArrow) {
                        if (pArrow->GetAttachedRef() == apRef) {
                            pArrow->ClearAttachedRef();
                            if (!pMob->GetDelete()) {
                                if (abInstant) {
                                    pArrow->SetDelete(true);
                                }
                                else {
                                    pArrow->SetState(ArrowProjectile::STATE_DELETING);
                                }
                            }
                        }
                        if (pArrow->GetDelete()) {
                            pArrow->Set3D(nullptr, true);
                        }
                    }
                }
            }
        }
        if (bSearchMiddleHigh) {
            for (uint32_t i = ReferenceProcessLists.MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);
                 i < ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
                 ++i) {
                MobileObject* pMob = ReferenceProcessLists.MobProcessArray.GetItem(i);
                if (pMob->IsProjectile()) {
                    ArrowProjectile* pArrow = dynamic_cast<ArrowProjectile*>(pMob);
                    if (pArrow) {
                        if (pArrow->GetAttachedRef() == apRef) {
                            pArrow->ClearAttachedRef();
                            if (!pMob->GetDelete()) {
                                pArrow->SetDelete(true);
                            }
                        }
                        if (pArrow->GetDelete()) {
                            pArrow->Set3D(nullptr, true);
                        }
                    }
                }
            }
        }
    }
}

// The object of apObject's scene graph hit by a pick from aPickFrom along aDirection of length afPickLength, or null;
// afPickLength shrinks to the hit fraction. A biped body's shield, quiver and (with abIgnoreHead) head are skipped. A
// blend collision object out of the world, or blended fully, takes its object's world transform (offset by a
// bhkRigidBodyT's local transform); any other body asks its Havok body. The ray is taken into the body's space and cast
// against its shape.
NiAVObject* ArrowProjectile::PickSceneGraph(
    NiAVObject* apObject,
    NiPoint3 aPickFrom,
    NiPoint3 aDirection,
    float& afPickLength,
    bool abIgnoreHead
) {
    bool bHit = false;
    if (apObject) {
        bhkCollisionObject* pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(apObject);
        if (pCollisionObject) {
            bhkWorldObject* pWorldObject = pCollisionObject->GetWorldObject();
            bhkShape* pShape;
            if (pWorldObject) {
                pShape = pWorldObject->GetShape();
            }
            else {
                pShape = nullptr;
            }
            // GetCollisionFilter writes the filter word into the CFilter and returns its address, on which the CFilter
            // member is then called.
            CFilter kFilter;
            CFilter kBipedFilter;
            CFilter kPartFilter;
            CFilter kHeadFilter;
            if (pShape &&
                (pWorldObject->GetCollisionFilter(&kFilter), kFilter.GetLayer()) == L_BIPED &&
                (pWorldObject->GetCollisionFilter(&kBipedFilter), kBipedFilter.GetBipedPart()) != P_SHIELD &&
                (pWorldObject->GetCollisionFilter(&kPartFilter), kPartFilter.GetBipedPart()) != P_QUIVER &&
                (!abIgnoreHead ||
                 (pWorldObject->GetCollisionFilter(&kHeadFilter), kHeadFilter.GetBipedPart()) != P_HEAD)) {
                alignas(16) hkVector4 kFrom;
                NI2HK(kFrom, aPickFrom);
                alignas(16) hkVector4 kTo;
                NI2HK(kTo, aPickFrom + aDirection * afPickLength);
                hkpShapeRayCastInput kInput;
                hkpShapeRayCastOutput kOutput;
                alignas(16) hkTransform kTransform;
                bhkRigidBody* pBody = static_cast<bhkRigidBody*>(pWorldObject);
                if (NiObject::IsKindOf_0(&bhkBlendCollisionObject::ms_RTTI, pCollisionObject) &&
                    (!pWorldObject->IsAddedToWorld() ||
                     static_cast<bhkBlendCollisionObject*>(pCollisionObject)->GetHeirGain() == 1.0)) {
                    NI2HK(kTransform, apObject->GetWorldTransform());
                    if (NiObject::IsKindOf_0(&bhkRigidBodyT::ms_RTTI, pWorldObject)) {
                        bhkRigidBodyT* pBodyT = static_cast<bhkRigidBodyT*>(pWorldObject);
                        alignas(16) hkTransform kLocal;
                        alignas(16) hkTransform kWorld(kTransform);
                        const hkVector4& rLocalPos = pBodyT->GetLocalPos();
                        const hkQuaternion& rLocalRot = pBodyT->GetLocalRot();
                        kLocal.set(rLocalRot, rLocalPos);
                        kTransform.setMul(kWorld, kLocal);
                    }
                }
                else {
                    pBody->GetTransform(kTransform);
                }
                kInput.m_from.setTransformedInversePos(kTransform, kFrom);
                kInput.m_to.setTransformedInversePos(kTransform, kTo);
                (**pShape)->castRay(kInput, kOutput);
                if (kOutput.hasHit()) {
                    afPickLength = afPickLength * kOutput.m_hitFraction;
                    bHit = true;
                }
            }
        }
        NiNode* pNode = apObject->IsNode();
        if (pNode) {
            for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                NiAVObject* pChild = pNode->GetAt(i);
                NiAVObject* pHit = PickSceneGraph(
                    static_cast<NiNode*>(pChild),
                    aPickFrom,
                    aDirection,
                    afPickLength,
                    abIgnoreHead
                );
                if (pHit) {
                    return pHit;
                }
            }
        }
    }
    if (bHit) {
        return apObject;
    }
    return nullptr;
}
