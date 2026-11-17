#include "Fallout/AI/ArrowProjectile.h"

#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkForceController.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
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
#include "Fallout/AI/bhkCharacterListenerArrow.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/rtti.h"

ArrowProjectile::CollisionData::CollisionData() {}

ArrowProjectile::ArrowProjectile()
	: psCollisionData(nullptr), bNeedsFinishInitLoadGame(false), bAddedToInventory(false), bPostForce(false),
	  iLoadGameFlags(0) {
	SetStillLoading(1);
	pCurrentProcess = new HighProcess();
	++iArrowCount;
}

ArrowProjectile::~ArrowProjectile() {
	--iArrowCount;
	ReferenceProcessLists.ProjectilePostProcessList.Remove(static_cast<MobileObject*>(this));
	NiAVObject* pav = TESObjectREFR::Get3D();
	if (pav && psCollisionData && psCollisionData->eCollisionType == 1) {
		bhkCollisionObject* pcol = FindFirstCollisionObject(pav);
		bhkRigidBody* parrowRB = pcol ? pcol->GetRigidBody() : nullptr;
		if (parrowRB) {
			hkVector4 centerOfMass;
			parrowRB->GetCenterOfMassInWorld(centerOfMass);
			TESArrowTargetListener::RemoveArrowTarget(centerOfMass, psCollisionData->pClosestObject);
		}
	}
	if (!bNeedsFinishInitLoadGame && psCollisionData && !psCollisionData->eCollisionType &&
		psCollisionData->pCollidee && psCollisionData->pCollidee->IsActor()) {
		MobileObject* pactor = static_cast<Actor*>(psCollisionData->pCollidee);
		if (pactor->pCurrentProcess) {
			pactor->pCurrentProcess->RemoveAttachedArrow(this);
		}
	}
	ourFree(psCollisionData);
	TESObjectREFR::Set3D(nullptr, true);
	if (!pDataHandler->GetClearingData()) {
		CleanUpOtherMobileObjects(0);
	}
}

NiAVObject* ArrowProjectile::Load3D(bool abBackgroundLoading) {
	NiFixedString v4;
	NiAVObject* v5 = MobileObject::Load3D(abBackgroundLoading);
	if (!v5) {
		return v5;
	}
	NiFixedString* a2 = ::new (static_cast<void*>(&v4)) NiFixedString(const_cast<char*>("Arrow"));
	int32_t v6 = 0;
	v5->SetName(*a2);
	v6 = (-1);
	return v5;
}

void ArrowProjectile::Revert(uint32_t aiFlags) {
	MobileObject::Revert(aiFlags);
	uint8_t result = (uint8_t)pSaveLoadGameOLD->IsUnk048Zero();
	if (result) {
		if (psCollisionData) {
			ourFree(psCollisionData);
		}
		psCollisionData = nullptr;
		return;
	}
}

uint16_t ArrowProjectile::GetSaveSize(uint32_t aiFlags) {
	uint16_t SaveSize = MobileObject::GetSaveSize(aiFlags);
	uint16_t v6 = SaveSize;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		SaveSize += 6;
	}
	uint16_t isize = SaveSize + 49;
	if (psCollisionData) {
		isize += 40;
		if (pSaveLoadGameOLD->GetCurrentVersion() >= 80u) {
			isize += 16;
		}
		if (psCollisionData->eCollisionType <= 1) {
			isize += 8;
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 84u) {
		++isize;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 85u) {
		++isize;
	}
	if (bDebugSaveBuffer.Bool()) {
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
			InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				isize - v6, pheader->iFormID, pform->GetFormEditorID(), pheader->iFlags, 2174,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp");
		} else {
			InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", isize - v6, 2174,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp");
		}
	}
	return isize;
}

// Before FinishInitLoadGame has run on a loaded arrow, pClosestObject still holds the packed collision object count
// (high word) and index (low word) LoadGame read, which are saved back as they are.
void ArrowProjectile::SaveGame(uint32_t aiFlags) {
	uint32_t icollideeID;
	NiQuaternion quat;
	uint32_t iblockHeader;
	uint32_t ienchantmentID;
	uint32_t ipoisonID;
	uint16_t iblockSize;
	uint32_t ishooterID;
	bool bsaveCollisionData;
	MobileObject::SaveGame(aiFlags);
	iblockSize = 0;
	uint16_t* pblockSize = 0;
	uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
	if (bDebugSaveBuffer.Bool()) {
		itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		iblockHeader = 0x424C4F4B;
		pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
		pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
	}
	pSaveLoadGameOLD->SaveGameDataOLD(&eState, 4u);
	SaveGameDataOLD(&fFadeAlpha, 4);
	SaveGameDataOLD(&fAge, 4);
	SaveGameDataOLD(&fSpeed, 4);
	SaveGameDataOLD(&fDamage, 4);
	SaveGameDataOLD(&fWaterSpeed, 4);
	SaveGameDataOLD(&DeltaS, 12);
	ishooterID = 0;
	if (pShooter) {
		ishooterID = pShooter->iFormID;
	}
	SaveNumericID(&ishooterID, 4);
	ienchantmentID = 0;
	if (pEnchantment) {
		ienchantmentID = pEnchantment->iFormID;
	}
	SaveNumericID(&ienchantmentID, 4);
	ipoisonID = 0;
	if (pPoison) {
		ipoisonID = pPoison->iFormID;
	}
	SaveNumericID(&ipoisonID, 4);
	bsaveCollisionData = psCollisionData != 0;
	SaveGameDataOLD(&bsaveCollisionData, 1);
	if (bsaveCollisionData) {
		pSaveLoadGameOLD->SaveGameDataOLD(psCollisionData, 4u);
		SaveGameDataOLD(&psCollisionData->kPoint, 12);
		SaveGameDataOLD(&psCollisionData->kNormal, 12);
		SaveGameDataOLD(&psCollisionData->kVelocity, 12);
		if (pSaveLoadGameOLD->GetCurrentVersion() >= 80u) {
			quat.FromRotation(psCollisionData->kRotation);
			pSaveLoadGameOLD->SaveGameDataOLD(&quat, 16u);
		}
		if (psCollisionData->eCollisionType <= 1) {
			uint16_t icollisionObjectCount;
			uint16_t icollisionObjectIndex;
			icollideeID = 0;
			if (psCollisionData->pCollidee) {
				icollideeID = psCollisionData->pCollidee->iFormID;
			}
			SaveNumericID(&icollideeID, 4);
			icollisionObjectCount = 0;
			icollisionObjectIndex = 0xFFFF;
			if (psCollisionData->pCollidee) {
				if (psCollisionData->pClosestObject) {
					if (bNeedsFinishInitLoadGame) {
						uint32_t iclosestObject =
							static_cast<uint32_t>(reinterpret_cast<uintptr_t>(psCollisionData->pClosestObject));
						icollisionObjectCount = static_cast<uint16_t>(iclosestObject >> 16);
						icollisionObjectIndex = static_cast<uint16_t>(iclosestObject);
					} else {
						bool bignoreWeaponBoneAndFadeNodes = psCollisionData->eCollisionType == 0;
						NiNode* v2 = psCollisionData->pCollidee->TESObjectREFR::Get3D();
						icollisionObjectCount = GetCollisionObjectCountInSceneGraph(v2, bignoreWeaponBoneAndFadeNodes,
							bignoreWeaponBoneAndFadeNodes, 1);
						bhkNiCollisionObject* pcol =
							bhkNiCollisionObject::GetbhkNiCollisionObject(psCollisionData->pClosestObject);
						NiNode* v3 = psCollisionData->pCollidee->TESObjectREFR::Get3D();
						icollisionObjectIndex = GetCollisionObjectIndexInSceneGraph(v3, pcol,
							bignoreWeaponBoneAndFadeNodes, bignoreWeaponBoneAndFadeNodes, 1);
					}
				}
			}
			SaveGameDataOLD(&icollisionObjectCount, 2);
			SaveGameDataOLD(&icollisionObjectIndex, 2);
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 84u) {
		SaveGameDataOLD(&bAddedToInventory, 1);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 85u) {
		SaveGameDataOLD(&bBowIgnoresWeaponsResist, 1);
	}
	if (bDebugSaveBuffer.Bool()) {
		uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
			uint32_t iFlags = pheader->iFlags;
			const char* v4 = pform->GetFormEditorID();
			InitializationFlow("SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v4, iFlags, 2282,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp");
		} else {
			InitializationFlow("SaveGame(): %-5i ending at line %i in file %s",
				static_cast<int32_t>(BufferPointer - itempStartPosition), 2282,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp");
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		// The block's size: the buffer's advance past the size word, compared as unsigned addresses.
		char* pBufferNow = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
		char* pBlockStart = static_cast<char*>(static_cast<void*>(pblockSize));
		if (pBufferNow > pBlockStart + 0xFFFF) {
			Warning("SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp", 2282);
		}
		*pblockSize = static_cast<uint16_t>(pBufferNow - pBlockStart);
	}
}

// The form IDs are kept in the pointer fields until InitLoadGame, and pClosestObject holds the packed collision object
// count (high word) and index (low word, sign-extended) until FinishInitLoadGame (before version 32: the object's name,
// a string this function allocates).
void ArrowProjectile::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	uint8_t inameLength;
	uint16_t icollisionObjectCount;
	NiQuaternion quat;
	uint32_t iblockHeader;
	uint32_t ienchantmentID;
	uint32_t ipoisonID;
	uint16_t iblockSize;
	uint32_t ishooterID;
	char bhasCollisionData;
	MobileObject::LoadGame(aiFlags, aiCurrentFlags);
	iblockSize = 0;
	uint8_t* istartBufferPosition = 0;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
		if (iblockHeader != 0x424C4F4B) {
			LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
			if (pheader) {
				TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
						"loading form is %08X %s with version %i and flags %08X",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp", 2293, pheader->iFormID,
					pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags);
			} else {
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
						"version is %i",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp", 2293,
					pSaveLoadGameOLD->GetCurrentVersion());
			}
		}
		istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
	}
	pSaveLoadGameOLD->LoadGameDataOLD(&eState, 4u);
	LoadGameDataOLD(&fFadeAlpha, 4);
	LoadGameDataOLD(&fAge, 4);
	LoadGameDataOLD(&fSpeed, 4);
	LoadGameDataOLD(&fDamage, 4);
	LoadGameDataOLD(&fWaterSpeed, 4);
	LoadGameDataOLD(&DeltaS, 12);
	LoadNumericID(&ishooterID, 4);
	pShooter = reinterpret_cast<Actor*>(static_cast<uintptr_t>(ishooterID));
	LoadNumericID(&ienchantmentID, 4);
	pEnchantment = reinterpret_cast<EnchantmentItem*>(static_cast<uintptr_t>(ienchantmentID));
	LoadNumericID(&ipoisonID, 4);
	pPoison = reinterpret_cast<AlchemyItem*>(static_cast<uintptr_t>(ipoisonID));
	if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
		pSaveLoadGameOLD->AdvanceBuffer(8u);
	}
	bhasCollisionData = 0;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u) {
		LoadGameDataOLD(&bhasCollisionData, 1);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
		char v12 = eState == ArrowProjectile::STATE_COLLISION || eState == ArrowProjectile::STATE_POST_COLLISION;
		bhasCollisionData = v12;
	}
	if (bhasCollisionData) {
		ArrowProjectile::CollisionData* v11;
		ArrowProjectile::CollisionData* apMemory =
			static_cast<ArrowProjectile::CollisionData*>(ourMalloc(sizeof(ArrowProjectile::CollisionData)));
		if (apMemory) {
			v11 = ::new (apMemory) ArrowProjectile::CollisionData;
		} else {
			v11 = 0;
		}
		psCollisionData = v11;
		psCollisionData->pClosestObject = 0;
		psCollisionData->pCollidee = 0;
		psCollisionData->kRotation = NiMatrix3_IDENTITY;
		pSaveLoadGameOLD->LoadGameDataOLD(psCollisionData, 4u);
		LoadGameDataOLD(&psCollisionData->kPoint, 12);
		LoadGameDataOLD(&psCollisionData->kNormal, 12);
		LoadGameDataOLD(&psCollisionData->kVelocity, 12);
		if (pSaveLoadGameOLD->GetCurrentVersion() >= 80u) {
			pSaveLoadGameOLD->LoadGameDataOLD(&quat, 16u);
			quat.ToRotation(psCollisionData->kRotation);
		}
		if (psCollisionData->eCollisionType <= 1) {
			uint32_t icollideeID;
			LoadNumericID(&icollideeID, 4);
			psCollisionData->pCollidee = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(icollideeID));
			if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u) {
				int16_t icollisionObjectIndex;
				LoadGameDataOLD(&icollisionObjectCount, 2);
				LoadGameDataOLD(&icollisionObjectIndex, 2);
				uint32_t iclosestObject = (static_cast<uint32_t>(icollisionObjectCount) << 16) +
					static_cast<uint32_t>(static_cast<int32_t>(icollisionObjectIndex));
				psCollisionData->pClosestObject = reinterpret_cast<NiAVObject*>(static_cast<uintptr_t>(iclosestObject));
			}
			if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
				LoadGameDataOLD(&inameLength, 1);
				char* pstring = static_cast<char*>(ourMalloc(inameLength + 1));
				ourMemset(pstring, 0, inameLength + 1);
				LoadGameDataOLD(pstring, inameLength);
				psCollisionData->pClosestObject = reinterpret_cast<NiAVObject*>(pstring);
			}
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 84u) {
		LoadGameDataOLD(&bAddedToInventory, 1);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 85u) {
		LoadGameDataOLD(&bBowIgnoresWeaponsResist, 1);
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
		if (pheader_2) {
			TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
			if (icurrentBufferPosition <= istartBufferPosition + iblockSize) {
				if (icurrentBufferPosition < istartBufferPosition + iblockSize) {
					uint32_t iFlags = pheader_2->iFlags;
					int32_t cVersion = pheader_2->cVersion;
					const char* v4 = pform_2->GetFormEditorID();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
							"form is %08X %s with version %i and flags %08X",
						static_cast<int32_t>(istartBufferPosition + iblockSize - icurrentBufferPosition),
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp", 2405,
						pheader_2->iFormID, v4, cVersion, iFlags);
				}
			} else {
				uint32_t v7 = pheader_2->iFlags;
				int32_t v5 = pheader_2->cVersion;
				const char* v3 = pform_2->GetFormEditorID();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
						"is %08X %s with version %i and flags %08X",
					static_cast<int32_t>(icurrentBufferPosition - (istartBufferPosition + iblockSize)),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp", 2405,
					pheader_2->iFormID, v3, v5, v7);
			}
		} else {
			if (icurrentBufferPosition <= istartBufferPosition + iblockSize) {
				if (icurrentBufferPosition < istartBufferPosition + iblockSize) {
					int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
							"%i",
						static_cast<int32_t>(istartBufferPosition + iblockSize - icurrentBufferPosition),
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp", 2405,
						CurrentVersion);
				}
			} else {
				int32_t v9 = pSaveLoadGameOLD->GetCurrentVersion();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
					static_cast<int32_t>(icurrentBufferPosition - (istartBufferPosition + iblockSize)),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\ArrowProjectile.cpp", 2405, v9);
			}
		}
	}
}

void ArrowProjectile::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
	MobileObject::InitLoadGame(aiFlags, aiOldFlags);
	uint32_t ishooterID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pShooter));
	if (ishooterID) {
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(ishooterID);
		pShooter = dynamic_cast<Actor*>(FormByNumericID);
	}
	uint32_t ienchantmentID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pEnchantment));
	if (ienchantmentID) {
		TESForm* v4 = TESForm::GetFormByNumericID(ienchantmentID);
		pEnchantment = dynamic_cast<EnchantmentItem*>(v4);
	}
	uint32_t ipoisonID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPoison));
	if (ipoisonID) {
		TESForm* v5 = TESForm::GetFormByNumericID(ipoisonID);
		pPoison = dynamic_cast<AlchemyItem*>(v5);
	}
	if (psCollisionData) {
		if (psCollisionData->eCollisionType <= 1) {
			uint32_t icollideeID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(psCollisionData->pCollidee));
			if (icollideeID) {
				TESForm* v6 = TESForm::GetFormByNumericID(icollideeID);
				psCollisionData->pCollidee = dynamic_cast<TESObjectREFR*>(v6);
			}
		}
	}
	PROCESS_TYPE CurrentProcessType = GetCurrentProcessType();
	ReferenceProcessLists.RemoveReference(this, CurrentProcessType);
	PROCESS_TYPE v8 = GetCurrentProcessType();
	ReferenceProcessLists.AddReference(this, v8, 0, 0, 0);
}

// pClosestObject holds what LoadGame read: the packed collision object count (high word) and index, or before
// version 32 the object's name, which is freed here.
void ArrowProjectile::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
	SetStillLoading(0);
	TESObjectREFR::Get3D();
	if (TESObjectREFR::Get3D()) {
		InitHavok(pShooter, 1.0f);
		MobileObject::FinishInitLoadGame(aiFlags, aiOldFlags);
		if (psCollisionData) {
			if (psCollisionData->eCollisionType <= 1) {
				if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u) {
					bool bignoreWeaponBoneAndFadeNodes = psCollisionData->eCollisionType == 0;
					int32_t icollisionObjectIndex =
						static_cast<int32_t>(reinterpret_cast<uintptr_t>(psCollisionData->pClosestObject));
					psCollisionData->pClosestObject = 0;
					if (psCollisionData->pCollidee) {
						NiNode* v3 = psCollisionData->pCollidee->TESObjectREFR::Get3D();
						if (static_cast<uint16_t>(icollisionObjectIndex >> 16) ==
							static_cast<uint16_t>(GetCollisionObjectCountInSceneGraph(v3, bignoreWeaponBoneAndFadeNodes,
								bignoreWeaponBoneAndFadeNodes, 1))) {
							if (icollisionObjectIndex != -1) {
								NiNode* v4 = psCollisionData->pCollidee->TESObjectREFR::Get3D();
								bhkNiCollisionObject* pcol =
									GetCollisionObjectByIndexInSceneGraph(v4, icollisionObjectIndex,
										bignoreWeaponBoneAndFadeNodes, bignoreWeaponBoneAndFadeNodes, 1);
								if (pcol) {
									psCollisionData->pClosestObject = pcol->m_pkSceneObject;
								}
							}
						} else {
							int32_t a4 = psCollisionData->pCollidee->iFormID;
							Warning("AI: Collision object count has changed on object %s %08",
								psCollisionData->pCollidee->GetFormEditorID(), a4);
						}
					}
				}
				if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
					char* pstring = reinterpret_cast<char*>(psCollisionData->pClosestObject);
					if (psCollisionData->pCollidee) {
						NiNode* v5 = psCollisionData->pCollidee->TESObjectREFR::Get3D();
						psCollisionData->pClosestObject =
							BSUtilities::GetObjectByName(static_cast<NiAVObject*>(v5), pstring);
					} else {
						psCollisionData->pClosestObject = 0;
					}
					ourFree(pstring);
				}
				if (!psCollisionData->pClosestObject) {
					psCollisionData->eCollisionType = 3;
				}
			}
		}
		pSaveLoadGameOLD->QueueFormForFinalization(this);
	} else {
		if (psCollisionData &&
			(psCollisionData->pCollidee &&
				(psCollisionData->pCollidee->GetDisabled() && psCollisionData->pCollidee->GetDelete()))) {
			SetDelete(1);
			psCollisionData->pClosestObject = 0;
		} else {
			bNeedsFinishInitLoadGame = 1;
			iLoadGameFlags = aiFlags;
		}
	}
}

bool ArrowProjectile::IsProjectile() {
	return true;
}

bool ArrowProjectile::MovetoHigh() {
	HighProcess* v2;
	if (pCurrentProcess && !pCurrentProcess->eLevel) {
		return 1;
	}
	PROCESS_TYPE aWhichList = pCurrentProcess->eLevel;
	ReferenceProcessLists.RemoveReference(this, aWhichList);
	HighProcess* apMemory = static_cast<HighProcess*>(ourMalloc(sizeof(HighProcess)));
	if (apMemory) {
		v2 = ::new (apMemory) HighProcess;
	} else {
		v2 = 0;
	}
	v2->Copy(pCurrentProcess);
	if (pCurrentProcess) {
		delete pCurrentProcess;
	}
	pCurrentProcess = v2;
	ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, 0, 0, 0);
	MoveHavok(0);
	pCurrentProcess->SetLastSeenLocation();
	return 1;
}

bool ArrowProjectile::MovetoMiddleHigh() {
	MiddleHighProcess* v2;
	if (pCurrentProcess && pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH) {
		return 1;
	}
	PROCESS_TYPE aWhichList = pCurrentProcess->eLevel;
	ReferenceProcessLists.RemoveReference(this, aWhichList);
	MiddleHighProcess* apMemory = static_cast<MiddleHighProcess*>(ourMalloc(sizeof(MiddleHighProcess)));
	if (apMemory) {
		v2 = ::new (apMemory) MiddleHighProcess;
	} else {
		v2 = 0;
	}
	v2->Copy(pCurrentProcess);
	if (pCurrentProcess) {
		delete pCurrentProcess;
	}
	pCurrentProcess = v2;
	ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_HIGH, 0, 0, 0);
	MoveHavok(0);
	pCurrentProcess->SetLastSeenLocation();
	return 1;
}

void ArrowProjectile::Process(float afDelta) {
	float Seconds = appTimer.GetSeconds();
	Update(Seconds);
}

bool ArrowProjectile::ChangeProcessLevel() {
	bool bchange = 1;
	bool bhighProcessOnly = 0;
	PROCESS_TYPE eoldLevel = pCurrentProcess->eLevel;
	PROCESS_TYPE enewLevel = PROCESS_TYPE_LOW;
	TESObjectCELL* pcell = GetParentCell();
	if (eState == ArrowProjectile::STATE_POST_COLLISION && (psCollisionData && psCollisionData->pCollidee)) {
		if (psCollisionData->pCollidee == pPlayer) {
			if (eoldLevel) {
				MovetoHigh();
			}
			return 1;
		}
		pcell = psCollisionData->pCollidee->GetParentCell();
	}
	if (Get3D() || pModelLoader->IsReferenceQueued(this)) {
		if (TES::pInstance->IsCellLoaded(pcell, 1)) {
			enewLevel = PROCESS_TYPE_HIGH;
		} else {
			if (TES::pInstance->IsCellLoaded(pcell, 0)) {
				enewLevel = PROCESS_TYPE_MIDDLE_HIGH;
			}
		}
	}
	if (eState == ArrowProjectile::STATE_FLYING) {
		bhighProcessOnly = 1;
	}
	if (eoldLevel == enewLevel) {
		return 1;
	}
	if (enewLevel) {
		if (enewLevel != PROCESS_TYPE_MIDDLE_HIGH || bhighProcessOnly) {
			if (this) {
				delete this;
			}
			return 0;
		} else {
			MovetoMiddleHigh();
		}
	} else {
		MovetoHigh();
	}
	return bchange;
}

bool ArrowProjectile::IsTransient() {
	return true;
}

void ArrowProjectile::Update(float afDelta) {
	NiMatrix3 rot;
	if (bNeedsFinishInitLoadGame) {
		bool bready = 1;
		if (psCollisionData) {
			if (psCollisionData->eCollisionType <= 1) {
				if (psCollisionData->pCollidee) {
					bready = psCollisionData->pCollidee->TESObjectREFR::Get3D() != 0;
				}
			}
		}
		if (bready) {
			bNeedsFinishInitLoadGame = 0;
			FinishInitLoadGame(iLoadGameFlags, 0);
			pSaveLoadGameOLD->FinalizeQueuedForms();
		}
	}
	NiAVObject* p3d = (NiAVObject*)Get3D();
	if (!(afDelta >= 0.0)) {
	LABEL_47:;
		if (p3d) {
			NiUpdateData UpdateData(afDelta, 1, 0);
			p3d->Update(UpdateData);
		}
		return;
	}
	fAge = fAge + afDelta;
	if (fArrowAgeMax.Float() < static_cast<double>(fAge)) {
		eState = ArrowProjectile::STATE_DELETING;
	}
	if (!GetDelete()) {
		if (eState || !GetCharController()) {
			if (eState == ArrowProjectile::STATE_POST_COLLISION && psCollisionData) {
				uint32_t ecolType = psCollisionData->eCollisionType;
				if (p3d) {
					if (!ecolType) {
						SetLocationOnReference(p3d->GetWorldTranslate());
						TESObjectCELL* pcell = 0;
						if (psCollisionData->pCollidee) {
							pcell = psCollisionData->pCollidee->GetParentCell();
						}
						if (pcell != GetParentCell()) {
							if (pcell) {
								ReferenceProcessLists.AddActorToTempChangeList(this);
							} else {
								SetDelete(1);
							}
						}
					}
				}
				if (p3d) {
					if (ecolType == 1 &&
						(!psCollisionData ||
							(!psCollisionData->pCollidee || !psCollisionData->pCollidee->TESObjectREFR::Get3D()))) {
						SetDelete(1);
					}
				}
			} else {
				if (eState == ArrowProjectile::STATE_DELETING) {
					fFadeAlpha = fFadeAlpha - afDelta / 3.0;
					if (fFadeAlpha <= 0.0) {
						fFadeAlpha = 0.0f;
						SetDelete(1);
					}
					if (p3d) {
						BSShaderUtil::RecursiveSetPropertyFadeAlpha(static_cast<NiNode*>(p3d), fFadeAlpha);
					}
				}
			}
		} else {
			float v8 = fSpeed * afDelta;
			NiPoint3 rdeltaMove(0.0f, v8, 0.0f);
			NiPoint3 oldLoc = GetLocationOnReference();
			bhkCharacterController* CharController = GetCharController();
			float z = CharController->QWaterHeight();
			double v7 = HK2NI(z);
			if ((&GetLocationOnReference())->z <= v7) {
				float ratio = (fWaterSpeed - afDelta * 1.0) / fWaterSpeed;
				fSpeed = fSpeed * ratio;
				fDamage = fDamage * ratio;
				bhkCharacterController* v3 = GetCharController();
				v3->SetFallTime(ratio);
				fWaterSpeed = fWaterSpeed - afDelta * 1.0;
				if (fWaterSpeed < 0.5) {
					Simulate(0);
					ProcessCollision();
				}
			}
			Move(afDelta, rdeltaMove, 15u);
			if (eState) {
				ArrowProjectile::ArrowState enextState =
					static_cast<ArrowProjectile::ArrowState>((eState == ArrowProjectile::STATE_DELETING) + 2);
				if (ProcessCollision()) {
					if (p3d) {
						SetLocationOnReference(p3d->GetTranslate());
					}
					eState = enextState;
					return;
				}
			}
			NiPoint3 newLoc = GetLocationOnReference();
			if (p3d) {
				float fy;
				float fz;
				float fx;
				p3d->SetTranslate(newLoc);
				rot = (*&p3d->m_kLocal).m_Rotate;
				rot.ToEulerAnglesZXY(fz, fx, fy);
				DeltaS = newLoc - oldLoc;
				float z_2 = DeltaS.z;
				float len = DeltaS.Length();
				float v6 = z_2 / len;
				fx = -NiASin(v6);
				rot.FromEulerAnglesZXY(fz, fx, fy);
				p3d->SetRotate(rot);
			}
		}
		goto LABEL_47;
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
	bhkWorld* pWorld;
	if (pCell) {
		pWorld = pCell->GetbhkWorld();
	} else {
		pWorld = nullptr;
	}
	if (!pController) {
		alignas(16) uint8_t aCinfoMemory[sizeof(bhkCharacterControllerCinfo)];
		bhkCharacterControllerCinfo* pCinfo = ::new (aCinfoMemory) bhkCharacterControllerCinfo(true);
		pCinfo->SetAcrobatics(0.0f);
		if (apShooter) {
			// 50.0f is the marksman constant.
			pCinfo->SetGravity(CombatFormulas::GetArrowGravity(afPower, 50.0f,
				static_cast<ActorValueOwner*>(apShooter)->GetClampedActorValue(ActorValue::LUCK)));
		}
		pCinfo->SetObject(Get3D());
		pCinfo->SetPosition(GetLocationOnReference());
		pCinfo->SetWorld(pWorld);
		// NiNew bhkSphereShape: 0x14 bytes.
		void* pShapeMemory = NiMemObject::operator new(sizeof(bhkSphereShape));
		bhkSphereShape* pShape;
		if (pShapeMemory) {
			pShape = ::new (pShapeMemory) bhkSphereShape(0.1f, true);
		} else {
			pShape = nullptr;
		}
		pCinfo->SetShape(pShape);
		pCinfo->SetAvoidBox(false);
		if (apShooter) {
			alignas(16) CFilter kFilter;
			static_cast<MobileObject*>(apShooter)->GetCollisionFilter(&kFilter.iFilter);
			pCinfo->SetColFilter(L_PROJECTILE, kFilter.GetGroup());
		} else {
			pCinfo->SetColFilter(L_PROJECTILE, ArrowProjectile::GetArrowGroup());
		}
		pCinfo->SetStartState(HK_CHARACTER_USER_STATE_1);
		// The listener is allocated with the 16-aligned allocator it shares with bhkRagdollController.
		void* pListenerMemory = bhkRagdollController::operator new(sizeof(bhkCharacterListenerArrow));
		bhkCharacterListenerArrow* pListener;
		if (pListenerMemory) {
			pListener = ::new (pListenerMemory) bhkCharacterListenerArrow(*pCinfo, this);
		} else {
			pListener = nullptr;
		}
		pController = pListener;
		pController->SetFallTime(1.0f);
		pController->SetQuickSimulate(Main::GetSceneIsComplex());
		pController->SetIsProjectile(true);
		// SWEEP-INLINE 0x633C90
		pCurrentProcess->SetCharController(NiPointer<bhkCharacterController>(pController));
		pController->SetMOBObject(this);
		pCinfo->~bhkCharacterControllerCinfo();
	}
	if (!pController) {
		return;
	}
	if (pCell && pWorld) {
		pController->SetWaterHeight(pCell->GetWaterHeight());
	} else {
		pController->SetWaterHeight(-3.4028235e+38f);
	}
	bhkWorld* pOldWorld = pController->GetWorld();
	pController->SetWorld(pWorld);
	NiNode* p3D = Get3D();
	NiAVObject* pOldObject = pController->GetNiObject();
	bool bAddToWorld = true;
	if (pOldObject != p3D) {
		pController->SetNiObject(p3D);
		if (pWorld && pWorld->QDebugDisplay()) {
			pController->BuildDisplayGeometry(0);
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
	} else {
		bhkWorld::UpdatePosition(p3D, true, false);
	}
}

bool ArrowProjectile::GetAddedToInventory() {
	return (bool)((uint8_t)bAddedToInventory);
}

bool ArrowProjectile::GetPostForce() {
	return (bool)((uint8_t)bPostForce);
}

void ArrowProjectile::SetPostForce(bool abValue) {
	bPostForce = (bool)((uint8_t)abValue);
}

void ArrowProjectile::ClearAttachedRef() {
	if (psCollisionData) {
		psCollisionData->pCollidee = nullptr;
	}
}

TESObjectREFR* ArrowProjectile::GetAttachedRef() {
	if (psCollisionData) {
		return psCollisionData->pCollidee;
	} else {
		return nullptr;
	}
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
		} else if (eLayer == L_BIPED) {
			fImpulse = fImpulse * fProjectileKnockMultBiped.Float();
		} else if (eLayer == L_PROPS) {
			fImpulse = fImpulse * fProjectileKnockMultProp.Float();
		} else {
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
		alignas(16) hkSimdReal kScale;
		kScale.~hkSimdReal();
		::new (&kScale) hkSimdReal(fImpulse);
		kImpulse.mul4(kScale);
		pWorld->markForWrite();
		static_cast<hkpRigidBody*>(**apTargetBody)->applyPointImpulse(kImpulse, kPoint);
		pWorld->unmarkForWrite();
	}
	SetPostForce(true);
}

char ArrowProjectile::ProcessCollisionTerrain(bhkRigidBody* apBody) {
	CFilter v8;
	CFilter a2a;
	DetachCharController();
	NiPoint3* p_kPoint = &psCollisionData->kPoint;
	NiNode* v2 = Get3D();
	v2->SetTranslate(*p_kPoint);
	TESObjectREFR::SetLocationOnReference(psCollisionData->kPoint);
	apBody->GetCollisionFilter(&a2a);
	(&a2a)->SetLayer(L_NONCOLLIDABLE);
	(&a2a)->SetDisabled(0);
	apBody->SetCollisionFilter(a2a);
	int32_t arrowGroup = ArrowProjectile::GetArrowGroup();
	CFilter* v4 = FNVD_RECONSTRUCT(CFilter)(&v8, L_NONCOLLIDABLE, arrowGroup, P_OTHER);
	apBody->SetCollisionFilter(*v4);
	return 1;
}

bool ArrowProjectile::ProcessCollisionActor(bhkRigidBody* apArrowRB) {
	BaseProcess* currentAIProcess;
	float v4;
	double localScale;
	ShadowSceneNode* shadowSceneNode;
	Actor* pCollidee;
	NiNode* pClosestObject;
	NiNode* apNode = Get3D();
	DetachCharController();
	if (psCollisionData->pCollidee && psCollisionData->pCollidee->IsActor()) {
		pClosestObject = static_cast<NiNode*>(psCollisionData->pClosestObject);
		if (pClosestObject) {
			bhkWorld::RemoveObjects(apNode, 1, 0);
			TESHavokUtilities::RemoveHavokFromSceneGraph(apNode);
			apNode->SetTranslate(psCollisionData->kPoint);
			apNode->SetRotate(psCollisionData->kRotation);
			localScale = (double)apNode->GetLocalScale();
			v4 = (float)(localScale / pClosestObject->GetWorldScale());
			apNode->SetScale(v4);
			shadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
			shadowSceneNode->RemoveObject(apNode);
			pClosestObject->AttachChild(apNode, (bool)1);
			shadowSceneNode->AddObject(apNode);
			pCollidee = static_cast<Actor*>(psCollisionData->pCollidee);
			if (pCollidee) {
				if (pCollidee->MobileObject::GetCurrentAIProcess()) {
					currentAIProcess = pCollidee->MobileObject::GetCurrentAIProcess();
					currentAIProcess->AddAttachedArrow(this);
				}
			}
			return (bool)1;
		} else {
			return (bool)0;
		}
	} else {
		Warning("AI: An arrow thinks it is colliding with an Actor, but there is no Actor in the collision data!");
		return (bool)0;
	}
}

bool ArrowProjectile::ProcessSimulate(bhkRigidBody* apArrowRB) {
	bhkCharacterController* charController;
	uint32_t filterGroup;
	CFilter* v5;
	int32_t arrowGroup;
	CFilter* v7;
	CFilter v11;
	CFilter v12;
	DetachCharController();
	NiNode* v2 = Get3D();
	bhkWorld::SetMotion(v2, hkpMotion::MOTION_DYNAMIC, 1, 0, 1);
	Actor* v14 = dynamic_cast<Actor*>(psCollisionData->pCollidee);
	if (v14 && v14->GetCharController()) {
		charController = v14->GetCharController();
		filterGroup = charController->GetFilterGroup();
		v5 = FNVD_RECONSTRUCT(CFilter)(&v12, L_CLUTTER, filterGroup, P_OTHER);
		apArrowRB->SetCollisionFilter(*v5);
	} else {
		arrowGroup = (int32_t)ArrowProjectile::GetArrowGroup();
		v7 = FNVD_RECONSTRUCT(CFilter)(&v11, L_CLUTTER, (uint32_t)arrowGroup, P_OTHER);
		apArrowRB->SetCollisionFilter(*v7);
	}
	apArrowRB->SetLinearVelocity(psCollisionData->kVelocity);
	hkpRigidBody* v8 = apArrowRB->Gethk();
	bhkCollisionObject* v13 = bhkCollisionObject::Getbhk(v8);
	if (v13) {
		v13->SetNotify(1);
	}
	return (bool)1;
}

void ArrowProjectile::CollideActor(const NiPoint3& aContactPoint, const NiPoint3& aContactNormal, Actor* apCollidee) {
	NiMatrix3 v36;
	hkSphere hitSphere;
	NiMatrix3 v50;
	alignas(16) NiPoint3 v61;
	NiMatrix3 tmpRot_2;
	NiMatrix3 rotation;
	float fpickLength;
	NiPoint3 v71;
	NiPoint3 v79;
	hkVector4 velocity;
	ArrowProjectile::CollisionData* v83;
	ArrowProjectile::CollisionData* v84 =
		static_cast<ArrowProjectile::CollisionData*>(ourMalloc(sizeof(ArrowProjectile::CollisionData)));
	if (v84) {
		v83 = ::new (v84) ArrowProjectile::CollisionData;
	} else {
		v83 = 0;
	}
	this->psCollisionData = v83;
	this->psCollisionData->eCollisionType = 0;
	this->psCollisionData->pClosestObject = 0;
	this->psCollisionData->pCollidee = 0;
	float* p_x = &this->psCollisionData->kNormal.x;
	*p_x = aContactNormal.x;
	p_x[1] = aContactNormal.y;
	p_x[2] = aContactNormal.z;
	this->psCollisionData->kPoint = aContactPoint;
	this->psCollisionData->kRotation = NiMatrix3_IDENTITY;
	this->psCollisionData->kVelocity = NiPoint3_ZERO;
	if (this->GetCharController()) {
		bhkCharacterController* CharController = this->GetCharController();
		FNVD_RECONSTRUCT(hkVector4)(&velocity, CharController->GethkLinearVelocity());
		if (-3.4028235e+38f == velocity.hkVector4::getSimdAt(0).operator float()) {
			if (this->Get3D()) {
				NiPoint3* fScalar = ::new (&v79) NiPoint3(NiPoint3_UNIT_Y * this->fSpeed);
				NiNode* v8 = this->Get3D();
				this->psCollisionData->kVelocity = (&v8->GetWorldTransform())->m_Rotate * *fScalar;
			}
		} else {
			HK2NI(this->psCollisionData->kVelocity, velocity);
		}
	}
	this->eState = ArrowProjectile::STATE_COLLISION;
	if (apCollidee) {
		bhkRigidBody* ptmpBody;
		hkVector4 pos;
		alignas(16) NiMatrix3 tmpRot;
		NiPoint3 dir;
		NiNode* proot;
		NiNode* v77 = this->Get3D();
		if (this->pShooter) {
			if (this->pShooter->GetCombatController()) {
				// Gets the collidee's position before the second GetCombatController call.
				const NiPoint3& rCollideePos = apCollidee->GetLocationOnReference();
				this->pShooter->GetCombatController()->ProjectileHitObstruction(rCollideePos, true);
			}
		}
		this->psCollisionData->pCollidee = apCollidee;
		apCollidee->ClearLastHitData();
		ExtraDataList* fAnglea = &this->psCollisionData->pCollidee->m_Extra;
		Script::SetActionFlag(this->GetObjectReference(), fAnglea, SCRIPT_ACTION_ONHITWITH);
		if (this->pShooter) {
			if (this->pShooter == pPlayer || this->pShooter->IsInCombat()) {
				bool v76 = apCollidee->IsDead(0);
				if (!v76 && apCollidee->IsDead(0) && (!apCollidee->IsCreature() || apCollidee->HasRagDoll())) {
					NiAVObject* p3d = (NiAVObject*)apCollidee->TESObjectREFR::Get3D();
					if (p3d) {
						hkVector4 loc;
						float* v10 = &fDeathForceRangedDamageMax.Float();
						float fmagnitude = CombatFormulas::GetDeathForce(*v10, 1);
						bhkForceController::RemoveForce(p3d);
						bhkWorld::SetMotion(p3d, hkpMotion::MOTION_DYNAMIC, 1, 0, 1);
						NiUpdateData UpdateData(0.0f, 0, 0);
						p3d->Update(UpdateData);
						alignas(16) NiPoint3 moveBack = (&v77->GetWorldTransform())->m_Rotate * NiPoint3_UNIT_Y;
						moveBack.Unitize();
						NiPoint3* v31 = ::new (&v71) NiPoint3(moveBack * -1024.0f);
						const NiPoint3* v11 = &this->GetLocationOnReference();
						moveBack = *v11 + *v31;
						NI2HK(loc, moveBack);
						TESHavokUtilities::AddExplosionImpulse(p3d, loc, fmagnitude, 0);
					}
				}
			}
		}
		bhkRigidBody* RigidBody = 0;
		if (apCollidee == pPlayer) {
			proot = static_cast<NiNode*>(pPlayer->Get3D(0));
		} else {
			proot = apCollidee->TESObjectREFR::Get3D();
		}
		fpickLength = 512.0f;
		float fAngleb = this->GetHeading(0);
		rotation.MakeZRotation(fAngleb);
		float fAnglec = this->GetLooking();
		tmpRot.MakeXRotation(fAnglec);
		rotation = *::new (&tmpRot_2) NiMatrix3(rotation * tmpRot);
		rotation.GetCol(1u, dir);
		NiPoint3::UnitizeVector(dir);
		NiPoint3* fAngled = ::new (&v61) NiPoint3(dir * fpickLength / 2.0f);
		const NiPoint3* v13 = &this->GetLocationOnReference();
		NiPoint3 origin = *v13 - *fAngled;
		char v58 = 0;
		bool fScalara = !apCollidee->IsDead(1);
		NiPoint3 v95 = dir;
		NiPoint3 v94 = origin;
		NiNode* v14 = apCollidee->Get3D();
		NiAVObject* ptargetObject = ArrowProjectile::PickSceneGraph(v14, v94, v95, fpickLength, fScalara);
		if (ptargetObject) {
			alignas(16) NiMatrix3 invertRot;
			float fpickLength_2 = fpickLength + 18.0;
			alignas(16) NiPoint3 move = dir * fpickLength_2;
			NiTransform invertTrans;
			(&ptargetObject->GetWorldTransform())->Invert(invertTrans);
			this->psCollisionData->kPoint = invertTrans * (origin + move);
			(&ptargetObject->GetWorldTransform())->m_Rotate.Inverse(invertRot);
			NiMatrix3* v17 = ::new (&v50) NiMatrix3(invertRot * (&v77->GetWorldTransform())->m_Rotate);
			this->psCollisionData->kRotation = *v17;
			this->psCollisionData->pClosestObject = ptargetObject;
			bhkCollisionObject* pcol = FindFirstCollisionObject(ptargetObject);
			if (pcol) {
				RigidBody = pcol->GetRigidBody();
			}
			v58 = 1;
		}
		bool v48 = 0;
		if (apCollidee->GetBlocked()) {
			if (apCollidee->pCurrentProcess) {
				if (CombatUtilities::IsTargetWithinStrikeAngle(apCollidee, this, 0)) {
					NiPoint3 v47 = *apCollidee->QLocationOnReference();
					float v46 = apCollidee->GetHeight() / 2.0 + v47.z;
					v48 = aContactPoint.z >= static_cast<double>(v46);
				}
			}
		}
		if (v58 && !v48) {
			goto LABEL_44;
		}
		::new (&hitSphere) NiBound;
		NI2HK(pos, aContactPoint);
		hitSphere.setPosition(pos);
		hitSphere.setRadius(200.0f);
		ptmpBody = 0;
		if (v48) {
			ptmpBody = static_cast<bhkRigidBody*>(bhkUtilFunctions::FindBipedPart(proot, 7));
		} else {
			bool v18 = apCollidee->IsDead(1);
			ptmpBody = bhkUtilFunctions::GetClosestBody(proot, hitSphere, !v18);
		}
		if (ptmpBody) {
			RigidBody = ptmpBody;
			bhkCollisionObject* pcol_2 = bhkCollisionObject::Getbhk(ptmpBody->operator*());
			if (pcol_2) {
				NiMatrix3 invertRot_2;
				ptargetObject = pcol_2->m_pkSceneObject;
				NiPoint3 move_2 = dir * 18.0f;
				NiTransform invertTrans_2;
				(&ptargetObject->GetWorldTransform())->Invert(invertTrans_2);
				this->psCollisionData->kPoint = invertTrans_2 * (ptargetObject->GetWorldTranslate() + move_2);
				(&ptargetObject->GetWorldTransform())->m_Rotate.Inverse(invertRot_2);
				NiMatrix3* v21 = ::new (&v36) NiMatrix3(invertRot_2 * (&v77->GetWorldTransform())->m_Rotate);
				this->psCollisionData->kRotation = *v21;
				this->psCollisionData->pClosestObject = ptargetObject;
			}
		}
		if (this->psCollisionData->pClosestObject) {
			bhkShape* pshape;
			bhkRigidBody* pbody;
			bhkCollisionObject* pcol_3;
		LABEL_44:;
			if (RigidBody) {
				NiPoint3 v96 = (&v77->GetWorldTransform())->m_Rotate * NiPoint3_UNIT_Y;
				this->ApplyCollisionForce(aContactPoint, v96, RigidBody);
			}
			if (this->psCollisionData->pClosestObject &&
				((pcol_3 = bhkCollisionObject::GetbhkCollisionObject(this->psCollisionData->pClosestObject)) == 0
						? pbody = 0
						: pbody = pcol_3->GetRigidBody(),
					(!pbody ? pshape = 0 : pshape = pbody->GetShape(), pshape))) {
				switch (pshape->GetMaterial()) {
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
					goto LABEL_56;
				}
				}
			} else {
			LABEL_56:;
				if (!apCollidee->IsCreature() || apCollidee->HasRagDoll()) {
					if (!this->pEnchantment) {
						uint32_t v22 = Random();
						if (v22 % 100 < iArrowInventoryChance.Int()) {
							apCollidee->AddObjecttoContainer(this->GetObjectReference(), 0, 1);
							this->bAddedToInventory = 1;
						}
					}
				} else {
					this->Simulate(apCollidee);
					this->SetState(ArrowProjectile::STATE_DELETING);
				}
			}
		} else {
			this->Simulate(apCollidee);
		}
	}
}

void ArrowProjectile::CollideTerrain(const NiPoint3& aContactPoint, const NiPoint3& aContactNormal) {
	NiPoint3 v18;
	hkVector4 velocity;
	ArrowProjectile::CollisionData* v24;
	ArrowProjectile::CollisionData* v25 =
		static_cast<ArrowProjectile::CollisionData*>(ourMalloc(sizeof(ArrowProjectile::CollisionData)));
	if (v25) {
		v24 = ::new (v25) ArrowProjectile::CollisionData;
	} else {
		v24 = 0;
	}
	this->psCollisionData = v24;
	alignas(16) NiPoint3 move = this->DeltaS;
	NiPoint3::UnitizeVector(move);
	move.operator*=(24.0f);
	this->psCollisionData->eCollisionType = 2;
	this->psCollisionData->pClosestObject = 0;
	this->psCollisionData->pCollidee = 0;
	this->psCollisionData->kNormal = aContactNormal;
	this->psCollisionData->kPoint = aContactPoint + move;
	this->psCollisionData->kRotation = NiMatrix3_IDENTITY;
	this->psCollisionData->kVelocity = NiPoint3_ZERO;
	if (this->GetCharController()) {
		bhkCharacterController* CharController = this->GetCharController();
		FNVD_RECONSTRUCT(hkVector4)(&velocity, CharController->GethkLinearVelocity());
		if (-3.4028235e+38f == velocity.hkVector4::getSimdAt(0).operator float()) {
			if (this->Get3D()) {
				NiPoint3* a2 = ::new (&v18) NiPoint3(NiPoint3_UNIT_Y * this->fSpeed);
				NiNode* v6 = this->Get3D();
				this->psCollisionData->kVelocity = (&v6->GetWorldTransform())->m_Rotate * *a2;
			}
		} else {
			HK2NI(this->psCollisionData->kVelocity, velocity);
		}
	}
	HavokMaterialType emat = TES::pInstance->GetLandMaterialType(aContactPoint);
	switch (emat) {
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
	NiPoint3 v34 = aContactPoint;
	float afVelocity = this->psCollisionData->kVelocity.Length();
	this->PlayArrowCollisionSound(afVelocity, v34, 0, emat);
	if (this->pShooter) {
		if (this->pShooter->GetCombatController()) {
			this->pShooter->GetCombatController()->ProjectileHitObstruction(aContactPoint, false);
		}
	}
	if (this->pShooter) {
		if (this->pEnchantment) {
			EnchantmentItem* LoopingSound = this->pEnchantment;
			if (LoopingSound) {
				this->pShooter->SetCurrentSpell(LoopingSound);
			} else {
				this->pShooter->SetCurrentSpell(0);
			}
			this->pShooter->SetDesiredTarget(0);
			TESBoundObject* a2a = this->GetObjectReference();
			this->pShooter->ReleaseCast(a2a);
		}
		if (this->pPoison) {
			AlchemyItem* v14 = this->pPoison;
			if (v14) {
				this->pShooter->SetCurrentSpell(v14);
			} else {
				this->pShooter->SetCurrentSpell(0);
			}
			this->pShooter->SetDesiredTarget(0);
			TESBoundObject* a2b = this->GetObjectReference();
			this->pShooter->ReleaseCast(a2b);
		}
		if (this->pBowEnchantment) {
			EnchantmentItem* pBowEnchant = this->pBowEnchantment;
			if (pBowEnchant) {
				this->pShooter->SetCurrentSpell(pBowEnchant);
			} else {
				this->pShooter->SetCurrentSpell(0);
			}
			this->pShooter->SetDesiredTarget(0);
			TESBoundObject* a2c = this->GetObjectReference();
			this->pShooter->ReleaseCast(a2c);
		}
	}
	if (this->pEnchantment) {
		this->SetState(ArrowProjectile::STATE_DELETING);
	}
}

void ArrowProjectile::Bounce(const NiPoint3& aContactPoint, const NiPoint3& aContactNormal) {
	NiPoint3 v11;
	hkVector4 velocity;
	if (psCollisionData) {
		if (aContactNormal.operator!=(NiPoint3_ZERO)) {
			float* p_x = &this->psCollisionData->kNormal.x;
			*p_x = aContactNormal.x;
			p_x[1] = aContactNormal.y;
			p_x[2] = aContactNormal.z;
		}
		if (aContactPoint.operator!=(NiPoint3_ZERO)) {
			this->psCollisionData->kPoint = aContactPoint;
		}
		this->psCollisionData->eCollisionType = 3;
		this->psCollisionData->pCollidee = 0;
	} else {
		ArrowProjectile::CollisionData* v15;
		ArrowProjectile::CollisionData* v16 =
			static_cast<ArrowProjectile::CollisionData*>(ourMalloc(sizeof(ArrowProjectile::CollisionData)));
		if (v16) {
			v15 = ::new (v16) ArrowProjectile::CollisionData;
		} else {
			v15 = 0;
		}
		this->psCollisionData = v15;
		this->psCollisionData->eCollisionType = 3;
		this->psCollisionData->pClosestObject = 0;
		this->psCollisionData->pCollidee = 0;
		float* v4 = &this->psCollisionData->kNormal.x;
		*v4 = aContactNormal.x;
		v4[1] = aContactNormal.y;
		v4[2] = aContactNormal.z;
		this->psCollisionData->kPoint = aContactPoint;
		this->psCollisionData->kRotation = NiMatrix3_IDENTITY;
		this->psCollisionData->kVelocity = NiPoint3_ZERO;
		if (this->GetCharController()) {
			bhkCharacterController* CharController = this->GetCharController();
			FNVD_RECONSTRUCT(hkVector4)(&velocity, CharController->GethkLinearVelocity());
			if (-3.4028235e+38f == velocity.hkVector4::getSimdAt(0).operator float()) {
				if (this->Get3D()) {
					NiPoint3* fScalar = ::new (&v11) NiPoint3(NiPoint3_UNIT_Y * this->fSpeed);
					NiNode* v7 = this->Get3D();
					this->psCollisionData->kVelocity = (&v7->GetWorldTransform())->m_Rotate * *fScalar;
				}
			} else {
				HK2NI(this->psCollisionData->kVelocity, velocity);
			}
		}
		this->eState = ArrowProjectile::STATE_COLLISION;
	}
}

void ArrowProjectile::Simulate(Actor* apCollidee) {
	NiPoint3 v9;
	hkVector4 velocity;
	if (psCollisionData) {
		this->psCollisionData->eCollisionType = 4;
	} else {
		ArrowProjectile::CollisionData* v13;
		ArrowProjectile::CollisionData* v14 =
			static_cast<ArrowProjectile::CollisionData*>(ourMalloc(sizeof(ArrowProjectile::CollisionData)));
		if (v14) {
			v13 = ::new (v14) ArrowProjectile::CollisionData;
		} else {
			v13 = 0;
		}
		this->psCollisionData = v13;
		this->psCollisionData->eCollisionType = 4;
		this->psCollisionData->pClosestObject = 0;
		this->psCollisionData->pCollidee = apCollidee;
		float* p_x = &this->psCollisionData->kNormal.x;
		*p_x = NiPoint3_ZERO.x;
		p_x[1] = NiPoint3_ZERO.y;
		p_x[2] = NiPoint3_ZERO.z;
		this->psCollisionData->kPoint = NiPoint3_ZERO;
		this->psCollisionData->kRotation = NiMatrix3_IDENTITY;
		this->psCollisionData->kVelocity = NiPoint3_ZERO;
		if (this->GetCharController()) {
			bhkCharacterController* CharController = this->GetCharController();
			FNVD_RECONSTRUCT(hkVector4)(&velocity, CharController->GethkLinearVelocity());
			if (-3.4028235e+38f == velocity.hkVector4::getSimdAt(0).operator float()) {
				if (this->Get3D()) {
					NiPoint3* fScalar = ::new (&v9) NiPoint3(NiPoint3_UNIT_Y * this->fSpeed);
					NiNode* v5 = this->Get3D();
					this->psCollisionData->kVelocity = (&v5->GetWorldTransform())->m_Rotate * *fScalar;
				}
			} else {
				HK2NI(this->psCollisionData->kVelocity, velocity);
			}
		}
	}
	this->eState = ArrowProjectile::STATE_COLLISION;
}

bool ArrowProjectile::ProcessCollision() {
	bhkCollisionObject* FirstCollisionObject;
	char v6 = 0;
	if (!psCollisionData) {
		return 0;
	}
	if (Get3D()) {
		NiNode* v2 = Get3D();
		FirstCollisionObject = FindFirstCollisionObject(v2);
	} else {
		FirstCollisionObject = 0;
	}
	if (FirstCollisionObject) {
		bhkRigidBody* pBody = FirstCollisionObject->GetRigidBody();
		switch (psCollisionData->eCollisionType) {
		case 0u: {
			v6 = ProcessCollisionActor(pBody);
			break;
		}
		case 1u: {
			v6 = ProcessCollisionReference(pBody);
			break;
		}
		case 2u: {
			v6 = ProcessCollisionTerrain(pBody);
			break;
		}
		case 3u: {
			v6 = ProcessBounce(pBody);
			break;
		}
		case 4u: {
			v6 = ProcessSimulate(pBody);
			break;
		}
		default: {
			break;
		}
		}
		if (!v6) {
			if (psCollisionData) {
				ourFree(psCollisionData);
				psCollisionData = 0;
			}
			eState = STATE_FLYING;
		}
		return v6;
	} else {
		Warning("AI: No collision data was found on this arrow");
		ourFree(psCollisionData);
		psCollisionData = 0;
		return 0;
	}
}

bool ArrowProjectile::ProcessCollisionReference(bhkRigidBody* apArrowRB) {
	hkVector4 centerOfMass;
	CFilter v11;
	alignas(16) CFilter apResult;
	DetachCharController();
	if (this->psCollisionData->pCollidee) {
		bhkCollisionObject* pcol = FindFirstCollisionObject(this->psCollisionData->pClosestObject);
		if (pcol) {
			bhkRigidBody* ptargetRB = pcol->GetRigidBody();
			NiNode* v4 = this->Get3D();
			bhkWorld::SetMotion(v4, hkpMotion::MOTION_KEYFRAMED, 1, 0, 1);
			CFilter* CollisionFilter = ptargetRB->GetCollisionFilter(&apResult);
			uint32_t Group = CollisionFilter->GetGroup();
			CFilter filter(L_PROJECTILE, Group, P_OTHER);
			CFilter* v7 = ptargetRB->GetCollisionFilter(&v11);
			if (v7->GetLayer() != L_CLUTTER) {
				filter.SetLayer(L_NONCOLLIDABLE);
			}
			apArrowRB->SetCollisionFilter(filter);
			BSSimpleList<void*>* pList = &ReferenceProcessLists.ProjectilePostProcessList;
			if (pList) {
				pList->AddHead(static_cast<MobileObject*>(this));
			}
			if (apArrowRB) {
				if (ptargetRB->GetDynamicMass() > 0.0) {
					apArrowRB->GetCenterOfMassInWorld(centerOfMass);
					TESArrowTargetListener::AddArrowTarget(centerOfMass, this->psCollisionData->pClosestObject);
				}
			}
			return 1;
		} else {
			return 0;
		}
	} else {
		Warning("AI: An arrow thinks it is colliding with an Reference, but there is no Reference in the collision "
				"data!");
		return 0;
	}
}

// The filter is passed by value.
bool ArrowProjectile::ProcessBounce(bhkRigidBody* apArrowRB) {
	CFilter apMemory;
	DetachCharController();
	if (!pSaveLoadGameOLD->GetLoading()) {
		NiPoint3* X = &psCollisionData->kPoint;
		NiNode* v2 = Get3D();
		v2->SetTranslate(*X);
		NiUpdateData UpdateData(0.0f, 0, 0);
		NiNode* v3 = Get3D();
		v3->Update(UpdateData);
	}
	NiNode* v4 = Get3D();
	bhkWorld::SetMotion(v4, hkpMotion::MOTION_DYNAMIC, 1, 0, 1);
	uint32_t ArrowGroup = ArrowProjectile::GetArrowGroup();
	CFilter* v6 = FNVD_RECONSTRUCT(CFilter)(&apMemory, L_CLUTTER, ArrowGroup, P_OTHER);
	apArrowRB->SetCollisionFilter(*v6);
	if (!pSaveLoadGameOLD->GetLoading()) {
		NiMatrix3 invrot;
		NiPoint3 accum = -1.0f * DeltaS;
		NiPoint3::UnitizeVector(accum);
		NiPoint3::UnitizeVector(psCollisionData->kNormal);
		float fnormalDotDirection = psCollisionData->kNormal.Dot(accum);
		NiPoint3 vton = 2.0f * (fnormalDotDirection * psCollisionData->kNormal - accum);
		accum.operator+=(vton);
		float v22 = fArrowBounceLinearSpeed.Float() * fSpeed;
		apArrowRB->SetLinearVelocity(v22 * accum);
		NiNode* v10 = Get3D();
		(&v10->GetWorldTransform())->m_Rotate.Inverse(invrot);
		NiPoint3 force = invrot * psCollisionData->kNormal;
		double v21 = NiAbs(force.y);
		if (NiAbs(force.z) < v21) {
			double v20 = NiAbs(force.y);
			force.z = NiSign(force.z) * v20;
		}
		force.y = 0.0f;
		NiPoint3::UnitizeVector(force);
		float* v11 = &fArrowBounceRotateSpeed.Float();
		float v19 = -force.x * *v11;
		float* v12 = &fArrowBounceRotateSpeed.Float();
		float X_4 = *v12 * force.z;
		NiPoint3 angle(X_4, 0.0f, v19);
		NiNode* v13 = Get3D();
		apArrowRB->SetAngularVelocity((&v13->GetWorldTransform())->m_Rotate * angle);
	}
	NiNode* v15 = Get3D();
	bhkCollisionObject* pcol = FindFirstCollisionObject(v15);
	if (pcol) {
		pcol->SetNotify(1);
	}
	return 1;
}

// Plays the collision sound of a stuck arrow's body against a4 at a3. The sound's location and velocity are constructed
// as a NiBound.
void ArrowProjectile::PlayArrowCollisionSound(float afVelocity, NiPoint3 a3, bhkRigidBody* a4, char a5) {
	ImpactMixer::COLLISION_SOUND apMemory;
	NiNode* arg0 = Get3D();
	if (arg0) {
		if (psCollisionData) {
			if (psCollisionData->eCollisionType == 1) {
				bhkRigidBody* Base;
				bhkCollisionObject* FirstCollisionObject = FindFirstCollisionObject(arg0);
				if (FirstCollisionObject) {
					Base = FirstCollisionObject->GetRigidBody();
				} else {
					Base = 0;
				}
				if (Base) {
					::new (&apMemory) NiBound;
					apMemory.fFriction = 0.5f;
					apMemory.pBody1 = Base;
					apMemory.pBody2 = a4;
					bhkShape* Shape = Base->GetShape();
					apMemory.cMaterialType1 = Shape->GetMaterial();
					apMemory.cMaterialType2 = a5;
					apMemory.Location = a3;
					apMemory.fVelocity = afVelocity;
					// The sound's ID is the sum of the two addresses, truncated to 32 bits.
					apMemory.iID =
						static_cast<int32_t>(reinterpret_cast<uintptr_t>(this) + reinterpret_cast<uintptr_t>(a4));
					ImpactMixer::PlayCollisionSound(apMemory);
				}
			}
		}
	}
}

uint32_t ArrowProjectile::GetArrowGroup() {
	if (!ArrowProjectile::iArrowGroup) {
		ArrowProjectile::iArrowGroup = (int32_t)bhkCollisionFilter::GetNewSystemGroup();
	}
	return ArrowProjectile::iArrowGroup;
}

int32_t ArrowProjectile::GetArrowCount() {
	return ArrowProjectile::iArrowCount;
}

void ArrowProjectile::DeleteAttachedArrows(TESObjectREFR* apRef, bool abInstant) {
	if (apRef) {
		if (!apRef->IsProjectile()) {
			bool v12 = 1;
			bool v11 = 1;
			if (apRef->IsActor()) {
				BaseProcess* pprocess = static_cast<Actor*>(apRef)->GetCurrentAIProcess();
				if (pprocess) {
					if (pprocess->GetAttachedArrowList()) {
						for (BSSimpleList<ArrowProjectile*>* plist = pprocess->GetAttachedArrowList();
							plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
							ArrowProjectile* parrow = plist->m_item;
							if (parrow) {
								if (parrow->GetAttachedRef() == apRef) {
									parrow->ClearAttachedRef();
									if (!parrow->GetDelete()) {
										if (abInstant) {
											parrow->SetDelete(1);
										} else {
											parrow->SetState(ArrowProjectile::STATE_DELETING);
										}
									}
								}
								if (parrow->GetDelete()) {
									parrow->Set3D(0, 1);
								}
							}
						}
						pprocess->GetAttachedArrowList()->RemoveAll();
					}
					if (pprocess->eLevel) {
						if (pprocess->eLevel == PROCESS_TYPE_MIDDLE_HIGH) {
							v12 = 0;
							v11 = 1;
						}
					} else {
						v12 = 1;
						v11 = 0;
					}
				}
			}
			if (v12) {
				for (uint32_t i = 0;
					i < (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
					MobileObject* pmob = (&ReferenceProcessLists.MobProcessArray)->GetItem(i);
					if (pmob->IsProjectile()) {
						ArrowProjectile* parrow_2 = dynamic_cast<ArrowProjectile*>(pmob);
						if (parrow_2) {
							if (parrow_2->GetAttachedRef() == apRef) {
								parrow_2->ClearAttachedRef();
								if (!pmob->GetDelete()) {
									if (abInstant) {
										parrow_2->SetDelete(1);
									} else {
										parrow_2->SetState(ArrowProjectile::STATE_DELETING);
									}
								}
							}
							if (parrow_2->GetDelete()) {
								parrow_2->Set3D(0, 1);
							}
						}
					}
				}
			}
			if (v11) {
				for (uint32_t i_2 =
						 (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);
					i_2 < (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
					++i_2) {
					MobileObject* pmob_2 = (&ReferenceProcessLists.MobProcessArray)->GetItem(i_2);
					if (pmob_2->IsProjectile()) {
						ArrowProjectile* parrow_3 = dynamic_cast<ArrowProjectile*>(pmob_2);
						if (parrow_3) {
							if (parrow_3->GetAttachedRef() == apRef) {
								parrow_3->ClearAttachedRef();
								if (!pmob_2->GetDelete()) {
									parrow_3->SetDelete(1);
								}
							}
							if (parrow_3->GetDelete()) {
								parrow_3->Set3D(0, 1);
							}
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
NiAVObject* ArrowProjectile::PickSceneGraph(NiAVObject* apObject, NiPoint3 aPickFrom, NiPoint3 aDirection,
	float& afPickLength, bool abIgnoreHead) {
	bool bHit = false;
	if (apObject) {
		bhkCollisionObject* pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(apObject);
		if (pCollisionObject) {
			bhkWorldObject* pWorldObject = pCollisionObject->GetWorldObject();
			bhkShape* pShape;
			if (pWorldObject) {
				pShape = pWorldObject->GetShape();
			} else {
				pShape = nullptr;
			}
			// GetCollisionFilter writes the filter word into the CFilter and returns its address, on which the CFilter
			// member is then called.
			CFilter kFilter;
			CFilter kBipedFilter;
			CFilter kPartFilter;
			CFilter kHeadFilter;
			if (pShape && (pWorldObject->GetCollisionFilter(&kFilter), kFilter.GetLayer()) == L_BIPED &&
				(pWorldObject->GetCollisionFilter(&kBipedFilter), kBipedFilter.GetBipedPart()) != P_SHIELD &&
				(pWorldObject->GetCollisionFilter(&kPartFilter), kPartFilter.GetBipedPart()) != P_QUIVER &&
				(!abIgnoreHead ||
					(pWorldObject->GetCollisionFilter(&kHeadFilter), kHeadFilter.GetBipedPart()) != P_HEAD)) {
				alignas(16) hkVector4 kFrom;
				NI2HK(kFrom, aPickFrom);
				alignas(16) hkVector4 kTo;
				NI2HK(kTo, aPickFrom + aDirection * afPickLength);
				hkpShapeRayCastInput kInput;
				kInput.~hkpShapeRayCastInput();
				::new (&kInput) hkpShapeRayCastInput;
				hkpShapeRayCastOutput kOutput;
				kOutput.~hkpShapeRayCastOutput();
				::new (&kOutput) hkpShapeRayCastOutput;
				alignas(16) hkTransform kTransform;
				kTransform.~hkTransform();
				::new (&kTransform) hkTransform;
				bhkRigidBody* pBody = static_cast<bhkRigidBody*>(pWorldObject);
				if (NiObject::IsKindOf_0(&bhkBlendCollisionObject::ms_RTTI, pCollisionObject) &&
					(!pWorldObject->IsAddedToWorld() ||
						static_cast<bhkBlendCollisionObject*>(pCollisionObject)->GetHeirGain() == 1.0)) {
					NI2HK(kTransform, apObject->GetWorldTransform());
					if (NiObject::IsKindOf_0(&bhkRigidBodyT::ms_RTTI, pWorldObject)) {
						bhkRigidBodyT* pBodyT = static_cast<bhkRigidBodyT*>(pWorldObject);
						alignas(16) hkTransform kLocal;
						kLocal.~hkTransform();
						::new (&kLocal) hkTransform;
						alignas(16) hkTransform kWorld;
						kWorld.~hkTransform();
						::new (&kWorld) hkTransform(kTransform);
						const hkVector4& rLocalPos = pBodyT->GetLocalPos();
						const hkQuaternion& rLocalRot = pBodyT->GetLocalRot();
						kLocal.set(rLocalRot, rLocalPos);
						kTransform.setMul(kWorld, kLocal);
					}
				} else {
					pBody->GetTransform(kTransform);
				}
				kInput.m_from.setTransformedInversePos(kTransform, kFrom);
				kInput.m_to.setTransformedInversePos(kTransform, kTo);
				pShape->operator*()->castRay(kInput, kOutput);
				if (kOutput.hasHit()) {
					afPickLength = afPickLength * kOutput.m_hitFraction;
					bHit = true;
				}
			}
		}
		NiNode* pNode = apObject->IsNode();
		if (pNode) {
			for (uint32_t i = 0; i < static_cast<uint32_t>(pNode->GetArrayCount()); ++i) {
				NiAVObject* pChild = pNode->GetAt(i);
				NiAVObject* pHit =
					PickSceneGraph(static_cast<NiNode*>(pChild), aPickFrom, aDirection, afPickLength, abIgnoreHead);
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

bool bhkWorldObject::IsAddedToWorld() {
	hkpWorldObject* pObject = Gethk();
	bool bAdded = false;
	if (pObject) {
		MarkRead();
		bAdded = pObject->isAddedToWorld();
		UnMarkRead();
	}
	return bAdded;
}
