#include "BSHavok/bhkRagdollController.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkBallAndSocketConstraint.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCachingShapePhantom.h"
#include "BSHavok/bhkCapsuleShape.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkLimitedHingeConstraint.h"
#include "BSHavok/bhkMalleableConstraint.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkPoseArray.h"
#include "BSHavok/bhkRagdollShareData.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyT.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Animation/Animation/Ik/FootPlacement/hkaFootPlacementIkSolver.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Animation/Animation/Ik/TwoJoints/hkaTwoJointsIkSolver.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapper.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperUtils.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaPose.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeletonUtils.h"
#include "Havok/Source/Animation/Ragdoll/Controller/RigidBody/hkaKeyFrameHierarchyUtility.h"
#include "Havok/Source/Animation/Ragdoll/Controller/RigidBody/hkaRagdollRigidBodyController.h"
#include "Havok/Source/Animation/Ragdoll/Instance/hkaRagdollInstance.h"
#include "Havok/Source/Animation/Ragdoll/PoseMatching/hkaPoseMatchingUtility.h"
#include "Havok/Source/Animation/Ragdoll/Utils/hkaRagdollUtils.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpRagdollMotorConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

SettingT<INISettingCollection> bhkRagdollController::bRagdollAnim("bRagdollAnim:RagdollAnim", true);
SettingT<INISettingCollection> bhkRagdollController::bFootIK("bFootIK:RagdollAnim", true);
SettingT<INISettingCollection> bhkRagdollController::bGrabIK("bGrabIK:RagdollAnim", true);
SettingT<INISettingCollection> bhkRagdollController::bLookIK("bLookIK:RagdollAnim", true);
SettingT<INISettingCollection> bhkRagdollController::bRagdollFeedback("bRagdollFeedback:RagdollAnim", true);
SettingT<INISettingCollection> bhkRagdollController::bPoseMatching("bPoseMatching:RagdollAnim", true);
SettingT<INISettingCollection> bhkRagdollController::fCameraDist("fCameraDist:RagdollAnim", 1000.0f);
SettingT<INISettingCollection> bhkRagdollController::fHierarchyGain("fHierarchyGain:RagdollAnim", 0.17f);
SettingT<INISettingCollection> bhkRagdollController::fVelocityDamping("fVelocityDamping:RagdollAnim", 0.0f);
SettingT<INISettingCollection> bhkRagdollController::fAccelerationGain("fAccelerationGain:RagdollAnim", 1.0f);
SettingT<INISettingCollection> bhkRagdollController::fVelocityGain("fVelocityGain:RagdollAnim", 0.6f);
SettingT<INISettingCollection> bhkRagdollController::fPositionGain("fPositionGain:RagdollAnim", 0.05f);
SettingT<INISettingCollection>
	bhkRagdollController::fPositionMaxLinearVelocity("fPositionMaxLinearVelocity:RagdollAnim", 14.0f);
SettingT<INISettingCollection>
	bhkRagdollController::fPositionMaxAngularVelocity("fPositionMaxAngularVelocity:RagdollAnim", 18.0f);
SettingT<INISettingCollection> bhkRagdollController::fSnapGain("fSnapGain:RagdollAnim", 0.1f);
SettingT<INISettingCollection> bhkRagdollController::fSnapMaxLinearVelocity("fSnapMaxLinearVelocity:RagdollAnim", 3.0f);
SettingT<INISettingCollection> bhkRagdollController::fSnapMaxAngularVelocity("fSnapMaxAngularVelocity:RagdollAnim",
	0.3f);
SettingT<INISettingCollection> bhkRagdollController::fSnapMaxLinearDistance("fSnapMaxLinearDistance:RagdollAnim", 0.3f);
SettingT<INISettingCollection> bhkRagdollController::fSnapMaxAngularDistance("fSnapMaxAngularDistance:RagdollAnim",
	1.0f);
SettingT<INISettingCollection> bhkRagdollController::fMaxTrackingDist("fMaxTrackingDist:LookIK", 1500.0f);
SettingT<INISettingCollection> bhkRagdollController::fMinTrackingDist("fMinTrackingDist:LookIK", 12.0f);
SettingT<INISettingCollection> gs_fAngleMax_LookIK("fAngleMax:LookIK", 3.5f);
SettingT<INISettingCollection> gs_fAngleMaxEase_LookIK("fAngleMaxEase:LookIK", 1.0f);
SettingT<INISettingCollection> bhkRagdollController::fEaseAngleShutOff("fEaseAngleShutOff:LookIK", 0.5f);
SettingT<INISettingCollection> gs_fOnOffGain_FootIK("fOnOffGain:FootIK", 0.01f);
SettingT<INISettingCollection> gs_fGroundAscendingGain_FootIK("fGroundAscendingGain:FootIK", 0.35f);
SettingT<INISettingCollection> gs_fGroundDescendingGain_FootIK("fGroundDescendingGain:FootIK", 0.3f);
SettingT<INISettingCollection> gs_fFootRaisedGain_FootIK("fFootRaisedGain:FootIK", 0.2f);
SettingT<INISettingCollection> gs_fFootPlantedGain_FootIK("fFootPlantedGain:FootIK", 1.0f);
SettingT<INISettingCollection> bhkRagdollController::bFootPlacementOn("bFootPlacementOn:FootIK", true);
SettingT<INISettingCollection> gs_fOriginalGroundHeightMS_FootIK("fOriginalGroundHeightMS:FootIK", -0.11f);
SettingT<INISettingCollection> gs_fAnkleOffset_FootIK("fAnkleOffset:FootIK", 0.2f);
SettingT<INISettingCollection> gs_fPelvisUpDownBias_FootIK("fPelvisUpDownBias:FootIK", 0.75f);
SettingT<INISettingCollection> bhkRagdollController::fPelvisOffsetDamping("fPelvisOffsetDamping:FootIK", 0.2f);
SettingT<INISettingCollection> bhkRagdollController::fVertErrorGain("fVertErrorGain:FootIK", 0.5f);
SettingT<INISettingCollection> bhkRagdollController::iNumFramesFootEaseOut("iNumFramesFootEaseOut:FootIK", 30);
SettingT<INISettingCollection> bhkRagdollController::fControllerTetherLen("fControllerTetherLen:FootIK", 6.0f);
SettingT<INISettingCollection> gs_fMaxStepVertError_FootIK("fMaxStepVertError:FootIK", 3.5f);
SettingT<INISettingCollection> bhkRagdollController::fMaxFootCastMilliSec("fMaxFootCastMilliSec:FootIK", 0.6f);
SettingT<INISettingCollection> bhkRagdollController::fDriveGain("fDriveGain:GrabIK", 0.25f);
SettingT<INISettingCollection> bhkRagdollController::uiFeedbackTimeout("uiFeedbackTimeout:RagdollFeedback", 1500u);
SettingT<INISettingCollection> bhkRagdollController::fRagdollFeedback("fRagdollFeedback:FootIK", 0.9f);
SettingT<INISettingCollection> bhkRagdollController::bRigidBodyController("bRigidBodyController:FootIK", true);
bhkRagdollController::PoseMap_t bhkRagdollController::PoseMap;
bhkRagdollController::SkeletonMap_t bhkRagdollController::SkeletonMap;

void* Reallocate(void* a1, uint32_t Size) {
	return (&MemoryManager_Singleton)->Reallocate(a1, 56 * Size);
}

// The exe inlines both pushBacks: it fetches the router each time (before the full check), grows the array through
// hkArrayUtil::_reserveMore with the router's heap when it is full, then stores and bumps the size. The list walk
// stops at the first empty item; the RTTI chain walk is NiObject::IsKindOf inlined.
void AddRagdollData(bhkRigidBody* apBody, hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arBones,
	hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints) {
	if (!apBody) {
		return;
	}
	hkpRigidBody* phkBody = static_cast<hkpRigidBody*>(apBody->phkObject);
	// EXE-INLINED none: hkArray<hkpRigidBody*>::pushBack
	hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
	if (arBones.m_size == (arBones.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*kRouter.m_heap, &arBones, sizeof(hkpRigidBody*));
	}
	arBones.m_data[arBones.m_size] = phkBody;
	arBones.m_size += 1;
	for (BSSimpleList<NiPointer<bhkSerializable>>* pNode = &apBody->kActionConstrants; pNode; pNode = pNode->m_pkNext) {
		bhkSerializable* pItem = pNode->m_item;
		if (!pItem) {
			break;
		}
		bhkConstraint* pConstraint =
			pItem->IsKindOf(&bhkConstraint::ms_RTTI) ? static_cast<bhkConstraint*>(pItem) : nullptr;
		if (pConstraint) {
			hkpConstraintInstance* phkConstraint = static_cast<hkpConstraintInstance*>(pConstraint->phkObject);
			// EXE-INLINED none: hkArray<hkpConstraintInstance*>::pushBack
			hkMemoryRouter& kConstraintRouter = hkMemoryRouter::getInstance();
			if (arConstraints.m_size ==
				(arConstraints.m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*kConstraintRouter.m_heap, &arConstraints, sizeof(hkpConstraintInstance*));
			}
			arConstraints.m_data[arConstraints.m_size] = phkConstraint;
			arConstraints.m_size += 1;
		}
	}
}

NiTransform& HK2NI(NiTransform& arNiTran, const hkQsTransform& arhkTran) {
	NiQuaternion v4;
	v4.m_fX = arhkTran.m_rotation.m_vec.m_quad[0];
	v4.m_fY = arhkTran.m_rotation.m_vec.m_quad[1];
	v4.m_fZ = arhkTran.m_rotation.m_vec.m_quad[2];
	v4.m_fW = arhkTran.m_rotation.m_vec.m_quad[3];
	v4.Normalize();
	v4.ToRotation(arNiTran.m_Rotate);
	HK2NI(arNiTran.m_Translate, arhkTran.m_translation);
	hkVector4 m_scale = arhkTran.m_scale;
	arNiTran.m_fScale = arhkTran.m_scale.m_quad[0];
	return arNiTran;
}

// hkaRagdollRigidBodyController's class allocator delete (the unwind of Init's new).
void FreeRagdollRigidBodyController(void* a1) {
	hkMemoryRouter* instance;
	if (a1) {
		instance = &hkMemoryRouter::getInstance();
		instance->m_heap->blockFree(a1, sizeof(hkaRagdollRigidBodyController));
	}
}

// hkaPoseMatchingUtility's class allocator delete (the unwind of InitPoseMatching's new).
void FreePoseMatchingUtility(void* a1) {
	hkMemoryRouter* instance;
	if (a1) {
		instance = &hkMemoryRouter::getInstance();
		instance->m_heap->blockFree(a1, sizeof(hkaPoseMatchingUtility));
	}
}

// Deletes the head skeleton under the Havok memory context.
bhkLookIKData::~bhkLookIKData() {
	TLSData* pTLSData = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLSData->eMemContext;
	pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
	if (pHeadSkel) {
		delete pHeadSkel;
	}
	pTLSData->eMemContext = eMemContext;
}

// Turns on the motor of every constraint the ragdoll's bone-to-rigid-body map names: a ragdoll joint gets its three
// motors and the identity target, a limited hinge its motor and a zero target angle. A malleable constraint is
// unwrapped first, and the type is read through the vtable both before and after the unwrap.
void bhkRagdollController::bhkRagdollPoweredConstraintController::startMotors(hkaRagdollInstance* ragdoll) {
	int32_t iBoneCount = (int32_t)ragdoll->m_skeleton->m_bones.m_size;
	for (int32_t iBone = 0; iBone < iBoneCount; ++iBone) {
		int32_t iBody = ragdoll->m_boneToRigidBodyMap.m_data[iBone] - 1;
		if (iBody < 0) {
			continue;
		}
		hkpConstraintInstance* pInstance = ragdoll->m_constraints.m_data[iBody];
		if (pInstance == nullptr) {
			continue;
		}
		hkpConstraintData* pData = pInstance->m_data;
		if (pData->getType() == hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE) {
			pData = static_cast<hkpMalleableConstraintData*>(pData)->getWrappedConstraintData();
		}
		if (pData == nullptr) {
			continue;
		}
		int32_t iType = pData->getType();
		if (iType == hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE) {
			hkpLimitedHingeConstraintData* pLimitedHinge = static_cast<hkpLimitedHingeConstraintData*>(pData);
			hkBool kEnabled;
			kEnabled.m_bool = 1;
			pLimitedHinge->setMotorActive(pInstance, kEnabled);
			pLimitedHinge->m_atoms.m_angMotor.m_targetAngle = 0.0f;
		} else if (iType == hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL) {
			hkpRagdollConstraintData* pRagdoll = static_cast<hkpRagdollConstraintData*>(pData);
			hkBool kEnabled;
			kEnabled.m_bool = 1;
			pRagdoll->setMotorsActive(pInstance, kEnabled);
			pRagdoll->setTarget(hkTransformIdentity.m_rotation);
		}
	}
}

void bhkRagdollController::bhkRagdollPoweredConstraintController::stopMotors(hkaRagdollInstance* ragdoll) {
	int32_t iNumBones = ragdoll->m_skeleton->m_bones.m_size;
	for (int32_t b = 0; b < iNumBones; ++b) {
		int32_t iConstraint = ragdoll->m_boneToRigidBodyMap.m_data[b] - 1;
		if (iConstraint >= 0) {
			hkpConstraintInstance* pInstance = ragdoll->m_constraints.m_data[iConstraint];
			if (pInstance) {
				hkpConstraintData* pData = pInstance->getDataRw();
				if (pData->getType() == hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE) {
					pData = static_cast<hkpMalleableConstraintData*>(pData)->getWrappedConstraintData();
				}
				if (pData) {
					int32_t eType = pData->getType();
					if (eType == hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE) {
						static_cast<hkpLimitedHingeConstraintData*>(pData)->setMotorActive(pInstance, hkBool(false));
					} else if (eType == hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL) {
						static_cast<hkpRagdollConstraintData*>(pData)->setMotorsActive(pInstance, hkBool(false));
					}
				}
			}
		}
	}
}

// Drives each powered constraint towards the bone's rotation in desiredPose: a ragdoll constraint gets it as its
// target frame, a limited hinge the angle about its axis as its motor's target angle.
void bhkRagdollController::bhkRagdollPoweredConstraintController::driveToPose(hkaRagdollInstance* ragdoll,
	const hkQsTransform* desiredPose) {
	int32_t iNumBones = ragdoll->m_skeleton->m_bones.m_size;
	for (int32_t b = 0; b < iNumBones; ++b) {
		int32_t iConstraint = ragdoll->m_boneToRigidBodyMap.m_data[b] - 1;
		if (iConstraint >= 0) {
			hkpConstraintInstance* pInstance = ragdoll->m_constraints.m_data[iConstraint];
			if (pInstance) {
				hkpConstraintData* pData = pInstance->getDataRw();
				if (pData->getType() == hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE) {
					pData = static_cast<hkpMalleableConstraintData*>(pData)->getWrappedConstraintData();
				}
				int32_t eType = pData->getType();
				if (eType == hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE) {
					hkpLimitedHingeConstraintData* pHinge = static_cast<hkpLimitedHingeConstraintData*>(pData);
					float angle;
					hkVector4 axis;
					hkQuaternion CBFromCA;
					hkQuaternion RBFromCB;
					hkQuaternion RAFromCA;
					hkQuaternion RBFromCA;
					hkQuaternion rest;
					RAFromCA.set(pHinge->m_atoms.m_transforms.m_transformA.m_rotation);
					RBFromCB.set(pHinge->m_atoms.m_transforms.m_transformB.m_rotation);
					RBFromCA.setMul(desiredPose[b].m_rotation, RAFromCA);
					CBFromCA.setInverseMul(RBFromCB, RBFromCA);
					// The exe builds (1, 0, 0, 0) from its 1.0f and 0.0f constants with unpcklps.
					axis.m_quad = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
					CBFromCA.decomposeRestAxis(axis, rest, angle);
					// EXE-INLINED none: hkpLimitedHingeConstraintData::setMotorTargetAngle
					pHinge->m_atoms.m_angMotor.m_targetAngle = angle;
				} else if (eType == hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL) {
					hkRotation target;
					target.set(desiredPose[b].m_rotation);
					static_cast<hkpRagdollConstraintData*>(pData)->setTarget(target);
				}
			}
		}
	}
}

// Every member cleared or set to its default, then the ragdoll of apObject is built (Init) and posed from apPose's
// 3D, whose bone transforms CopyPoseTo3D gathers when both are nodes.
bhkRagdollController::bhkRagdollController(NiAVObject* apObject, NiAVObject* apPose, const char* apScaledSkeletonName)
	: bQueueStopPoweredMotors(false), fCurrentMatchError(0.0f), eRBUpdateMethod(APPLY_VELOCITY), bFullyDynamic(false),
	  bEnableRagdollAnim(false), bInitRagdollAnim(false), bEaseOutRagdollAnim(false), bQueuedWarp(false),
	  pRagdollInstance(nullptr), pRagdollRBController(nullptr), bReinitRBController(false), pHeadPose(nullptr),
	  pRagdollRoot(nullptr), iGroup(0), spSkeletonDebug(nullptr), pSGPose(nullptr), bUsingMappers(false),
	  pControllerPhantom(nullptr), bInitLookIK(false), bEnableLookIK(false), bEaseOutLookIK(false),
	  bActiveLookIK(false), bApplyHeadOffset(false), bTargetOutsideHeadLimit(false), eCurrentBone(EYE_BONE),
	  fEyeHeading(0.0f), fEyePitch(0.0f), bTargetBehind(false), bHeadSwitchSides(false), fMaxEyePitchAngle(0.0f),
	  bInitGrabIK(false), bEnableGrabIK(false), bActiveGrabIK(false), bInitFootIK(false), bEnableFootIK(false),
	  bEaseOutFootIK(false), bActiveFootIK(false), bResetSolverWeight(false), bUsePhantomFootIKCasts(true),
	  iCurrFootEase(0), usCurrentCastIdx(0), usCurrentFootCastIdx(0), fVertFootIkError(0.0f),
	  bAlignFeetToTerrain(false), bFootEndCast(true), bUseCurrAnkleTransf(true), bDownCastFoot(false),
	  fDownCastDistance(0.0f), pMatchUtil(nullptr), spPoseA(nullptr), bInitPoseMatch(false), bEnablePoseMatch(false),
	  bActivePoseMatch(false), iCurrentMatchPoseIdx(-1), fPoseMatchDisplacementMax(20.0f), fPoseMatchTime(10000.0f),
	  bDisablePoseMatchOnDisplace(true), iEditorForcePoseIdx(-1), bInitRagdollFeedback(false),
	  bEnableRagdollFeedback(false), bActiveRagdollFeedback(false), bEaseOutFeedback(false), fCurrFeedbackInterp(1.0f),
	  uiFeedbackTimeoutCount(0), spRagdollSharedData(nullptr) {
	// PoseMatchTimer and FeedbackTimer clear themselves (BSPrecisionTimer's constructor).
	for (int32_t i = 0; i < NUM_LOOKIK_BONES; ++i) {
		LookIKBoneParams& rBone = BoneParamA[i];
		rBone.fLookAtMaxAngle = 1.0471976f;
		rBone.ssBoneIdx = -1;
		rBone.ssParentIdx = -1;
		rBone.FwdLS.m_quad = _mm_setzero_ps();
		rBone.FwdParentMS.m_quad = _mm_setzero_ps();
		rBone.bActive = 0;
		rBone.bLimitBoneRot = 1;
		rBone.fDeadHeading = 0.0f;
		rBone.fDeadPitch = 0.0f;
		rBone.PrevRotLS.m_vec.m_quad = hkQuadReal0001;
	}
	CurrentTargetWS.m_quad = _mm_setzero_ps();
	EyeOffset.m_quad = _mm_setzero_ps();
	CurrentHeadRotOffset.m_vec.m_quad = hkQuadReal0001;
	GrabIKState[0].bEaseOutGrabIK = 0;
	GrabIKState[1].bEaseOutGrabIK = 0;
	// The PDB's bTargetOutsideHeadLimit, which the exe clears.
	padding198[0] = 0;
	WorldFromModel.m_translation.m_quad = _mm_setzero_ps();
	WorldFromModel.m_rotation.m_vec.m_quad = hkQuadReal0001;
	const hkQuadReal kOne = {1.0f, 1.0f, 1.0f, 1.0f};
	WorldFromModel.m_scale.m_quad = kOne;
	NiNode* pSkeleton = apObject ? apObject->IsNode() : nullptr;
	NiNode* pPoseNode = apPose ? apPose->IsNode() : nullptr;
	bool bPrevUseVel = false;
	// CopyPoseTo3D fills the previous motion type, a dword.
	alignas(16) hkpMotion::MotionType ePrevMotion = hkpMotion::MOTION_INVALID;
	BSSimpleArray<BoneTransf, 1024> kPoseA;
	if (pSkeleton && pPoseNode) {
		CopyPoseTo3D(pSkeleton, pPoseNode, kPoseA, bPrevUseVel, ePrevMotion);
	}
	Init(apObject, apScaledSkeletonName);
	ResetPose(pSkeleton, kPoseA, bPrevUseVel, ePrevMotion);
	nHeadBone = 0;
}

// Releases the foot IK solvers, the ragdoll and its controller, the poses and the pose matching, and drops the shared
// pose array and share data from their maps when only the map and this controller still hold them; the members
// release the rest.
bhkRagdollController::~bhkRagdollController() {
	for (int32_t i = 0; i < LegInfoA.m_size; ++i) {
		LegInfoA.m_data[i].pSolver->removeReference();
	}
	LegInfoA.m_size = 0;
	if (pRagdollRBController) {
		pRagdollRBController->~hkaRagdollRigidBodyController();
		hkMemoryRouter::getInstance().m_heap->blockFree(pRagdollRBController, sizeof(hkaRagdollRigidBodyController));
	}
	if (pRagdollInstance) {
		pRagdollInstance->removeReference();
	}
	if (pHeadPose) {
		pHeadPose->~hkaPose();
		hkMemoryRouter::getInstance().m_heap->blockFree(pHeadPose, sizeof(hkaPose));
	}
	if (pSGPose) {
		pSGPose->~hkaPose();
		hkMemoryRouter::getInstance().m_heap->blockFree(pSGPose, sizeof(hkaPose));
	}
	// The exe looks the pose array up again (its result unused) before testing whether only the map and this
	// controller still hold it.
	PoseMap.GetAt(PoseAStr.GetHandle(), spPoseA);
	if (spPoseA && spPoseA->m_uiRefCount == 2) {
		PoseMap.RemoveAt(PoseAStr.GetHandle());
	}
	if (spRagdollSharedData && spRagdollSharedData->m_uiRefCount == 2) {
		SkeletonMap.RemoveAt(SkeletonShareName.GetHandle());
	}
	if (pMatchUtil) {
		pMatchUtil->~hkaPoseMatchingUtility();
		hkMemoryRouter::getInstance().m_heap->blockFree(pMatchUtil, sizeof(hkaPoseMatchingUtility));
	}
}

// The foot IK's ray cast: the last cast of this foot and cast index is reused while its ends lie within 0.01 of
// this one's; otherwise the ray goes to the world's pick, or to the controller's phantom. On a hit the fraction is
// corrected where the root's up axis is tilted and the hit lies above the root less 2, and the cast is recorded.
hkBool bhkRagdollController::castRay(const hkVector4& arFromWS, const hkVector4& arToWS, float& arHitFractionOut,
	hkVector4& arNormalWSOut) {
	bool bHit = false;
	float fHitFraction = 0.0f;
	FootRayCastResults& rLastCast = LegInfoA.m_data[usCurrentFootCastIdx].CastResults[usCurrentCastIdx];
	if (rLastCast.fFraction != -1.0f && rLastCast.From.equals3(arFromWS, 0.01f) &&
		rLastCast.To.equals3(arToWS, 0.01f)) {
		arNormalWSOut.m_quad = rLastCast.Norm.m_quad;
		FootRayCastResults& rCast = LegInfoA.m_data[usCurrentFootCastIdx].CastResults[usCurrentCastIdx];
		hkSimdReal kFraction;
		kFraction.~hkSimdReal();
		::new (&kFraction) hkSimdReal(rCast.fFraction);
		hkVector4 kCachedHit;
		kCachedHit.setInterpolate4(rCast.From, rCast.To, kFraction);
		bHit = true;
		arHitFractionOut = (arFromWS.m_quad[2] - kCachedHit.m_quad[2]) / (arFromWS.m_quad[2] - arToWS.m_quad[2]);
		fHitFraction = arHitFractionOut;
	} else {
		uint32_t uiGroup = iGroup;
		bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1,
			(1ull << L_STATIC) | (1ull << L_ANIMSTATIC) | (1ull << L_TRANSPARENT) | (1ull << L_TERRAIN) |
				(1ull << L_GROUND) | (1ull << L_DEADBIP));
		bhkPickData kPick;
		// EXE-INLINED 0x4A3C20
		kPick.m_enableShapeCollectionFilter.m_bool = false;
		kPick.m_hitFraction = 1.0f;
		kPick.m_extraInfo = -1;
		kPick.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
		kPick.m_shapeKeyIndex = 0;
		kPick.m_rootCollidable = nullptr;
		kPick.hkLength.m_quad = hkVector4Zero.m_quad;
		kPick.pCache = nullptr;
		kPick.pCloseCollector = nullptr;
		kPick.pAllCollector = nullptr;
		kPick.bPickFailed = false;
		kPick.m_filterInfo = (uiGroup << 16) | L_CUSTOMPICK1;
		kPick.m_from.m_quad = arFromWS.m_quad;
		kPick.m_to.m_quad = arToWS.m_quad;
		if (!bUsePhantomFootIKCasts) {
			// EXE-INLINED 0xC76320
			bhkWorld* pWorld = nullptr;
			hkaRagdollInstance* pInstance = pRagdollInstance;
			hkpRigidBody* pBody;
			ahkpWorld* pHkWorld;
			if (pInstance && (pBody = pInstance->m_rigidBodies.m_data[0]) != nullptr &&
				(pHkWorld = static_cast<ahkpWorld*>(pBody->m_world)) != nullptr) {
				pWorld = static_cast<bhkWorld*>(pHkWorld->m_userData);
			}
			if (pWorld->PickObject(kPick)) {
				arHitFractionOut = kPick.m_hitFraction;
				hkVector4 kNormal = kPick.m_normal;
				kNormal.normalize3();
				arNormalWSOut.m_quad = kNormal.m_quad;
				fHitFraction = arHitFractionOut;
				bHit = true;
			}
		} else {
			hkpCachingShapePhantomRay* pPhantom = pControllerPhantom;
			if (pPhantom) {
				FNVD_INLINED_LOCAL(hkpWorldRayCastInput, kInput, std::nullopt, std::nullopt);
				FNVD_INLINED_LOCAL(hkpWorldRayCastOutput, kOutput, std::nullopt, std::nullopt);
				// EXE-INLINED 0x4A3CC0
				kInput.m_enableShapeCollectionFilter.m_bool = false;
				kInput.m_filterInfo = (uiGroup << 16) | L_CUSTOMPICK1;
				// EXE-INLINED 0x4A3D00
				kOutput.m_hitFraction = 1.0f;
				kOutput.m_extraInfo = -1;
				kOutput.m_shapeKeys[0] = HK_INVALID_SHAPE_KEY;
				kOutput.m_shapeKeyIndex = 0;
				kOutput.m_rootCollidable = nullptr;
				kInput.m_from.m_quad = arFromWS.m_quad;
				kInput.m_to.m_quad = arToWS.m_quad;
				pPhantom->castRay(kInput, kOutput);
				if (kOutput.m_rootCollidable) {
					arHitFractionOut = kOutput.m_hitFraction;
					hkVector4 kNormal = kOutput.m_normal;
					kNormal.normalize3();
					arNormalWSOut.m_quad = kNormal.m_quad;
					fHitFraction = arHitFractionOut;
					bHit = true;
				}
			}
		}
	}
	hkSimdReal kT;
	kT.~hkSimdReal();
	::new (&kT) hkSimdReal(fHitFraction);
	hkVector4 kHitWS;
	kHitWS.setInterpolate4(arFromWS, arToWS, kT);
	if (bHit) {
		hkaPose* pPose = pSGPose;
		// EXE-INLINED none: hkaPose::getBoneModelSpace
		const hkQsTransform& rRoot = (pPose->m_boneFlags.m_data[0] & hkaPose::F_BONE_MODEL_DIRTY)
			? pPose->calculateBoneModelSpace(0)
			: pPose->m_modelPose.m_data[0];
		hkVector4 kRootWS;
		kRootWS.setTransformedPos(WorldFromModel, rRoot.m_translation);
		hkVector4 kUp;
		kUp.set(0.0f, 0.0f, 1.0f, 0.0f);
		pPose = pSGPose;
		// EXE-INLINED none: hkaPose::getBoneModelSpace
		const hkQsTransform& rRootRotation = (pPose->m_boneFlags.m_data[0] & hkaPose::F_BONE_MODEL_DIRTY)
			? pPose->calculateBoneModelSpace(0)
			: pPose->m_modelPose.m_data[0];
		hkVector4 kUpWS;
		kUpWS.setRotatedDir(rRootRotation.m_rotation, kUp);
		if (LegInfoA.m_size == 2) {
			float fUpZ = std::fabs(kUpWS.m_quad[2]);
			if (fUpZ > 0.2 && kRootWS.m_quad[2] - 2.0 < kHitWS.m_quad[2]) {
				arHitFractionOut =
					(arFromWS.m_quad[2] - kHitWS.m_quad[2] + 2.0) / (arFromWS.m_quad[2] - arToWS.m_quad[2]);
			}
		}
		if (!bAlignFeetToTerrain) {
			arNormalWSOut.m_quad = kUp.m_quad;
		}
		LegInfoA.m_data[usCurrentFootCastIdx].CastResults[usCurrentCastIdx].From.m_quad = arFromWS.m_quad;
		LegInfoA.m_data[usCurrentFootCastIdx].CastResults[usCurrentCastIdx].To.m_quad = arToWS.m_quad;
		LegInfoA.m_data[usCurrentFootCastIdx].CastResults[usCurrentCastIdx].Norm.m_quad = arNormalWSOut.m_quad;
		LegInfoA.m_data[usCurrentFootCastIdx].CastResults[usCurrentCastIdx].fFraction = arHitFractionOut;
	} else {
		LegInfoA.m_data[usCurrentFootCastIdx].CastResults[usCurrentCastIdx].fFraction = -1.0f;
	}
	++usCurrentCastIdx;
	hkBool kResult;
	kResult.m_bool = bHit;
	return kResult;
}

void bhkRagdollController::ForceHeadTrack() {
	if (eCurrentBone == bhkRagdollController::EYE_BONE) {
		BoneParamA[1].bActive = 1;
		eCurrentBone = bhkRagdollController::HEAD_BONE;
	}
}

void bhkRagdollController::SetLookIKActive(bool abActive) {
	bool bActiveLookIK;
	if (bEaseOutRagdollAnim) {
		bActiveLookIK = this->bActiveLookIK;
	} else {
		bActiveLookIK = bEnableLookIK && abActive;
	}
	this->bActiveLookIK = bActiveLookIK;
	if (!abActive) {
		BoneParamA[0].PrevRotLS.m_vec.m_quad = hkQuadReal0001;
		BoneParamA[1].PrevRotLS.m_vec.m_quad = hkQuadReal0001;
		BoneParamA[1].bLimitBoneRot = 1;
		BoneParamA[0].bLimitBoneRot = 1;
	}
}

bool bhkRagdollController::QRagdollAnimInit() {
	return (bool)((uint8_t)bInitRagdollAnim);
}

void bhkRagdollController::SyncRagdollTo3D() {
	NiMatrix3 result;
	NiTransform kWorldTransform;
	kWorldTransform = pRagdollRoot->m_kWorld;
	kWorldTransform.m_Rotate = (result = ScaleInv * kWorldTransform.m_Rotate);
	kWorldTransform.m_fScale = 1.0;
	NI2HK(WorldFromModel, kWorldTransform);
	GetSGPoseLocalSpace();
	RAGDOLL_UPDATE_TYPE eRBUpdateMethod = this->eRBUpdateMethod;
	this->eRBUpdateMethod = WARP;
	UpdateRagdollRigidBodies(0);
	hkaRagdollRigidBodyController* pRagdollRBController = this->pRagdollRBController;
	this->eRBUpdateMethod = eRBUpdateMethod;
	if (pRagdollRBController) {
		pRagdollRBController->reinitialize();
	}
}

void bhkRagdollController::SetUseHeadPose(bool abUseHead) {
	bTargetOutsideHeadLimit = (bool)((uint8_t)abUseHead);
}

// The exe's cleanup-only C++ EH frame is left out.
bool bhkRagdollController::InitLookIK(const char* apNodeName) {
	bool bResult = bInitRagdollAnim;
	if (bResult) {
		bResult = InitBoneParams(pSGPose, apNodeName, bhkRagdollController::HEAD_BONE);
		if (bResult) {
			eCurrentBone = bhkRagdollController::HEAD_BONE;
			if (bTargetOutsideHeadLimit) {
				if (!spRagdollSharedData.m_pObject->pSharedLookData) {
					// EXE-INLINED none: bhkLookIKData::bhkLookIKData
					bhkLookIKData* pLookData =
						static_cast<bhkLookIKData*>(MemoryManager::AlignedAllocate16(sizeof(bhkLookIKData)));
					if (pLookData) {
						pLookData->pHeadSkel = nullptr;
					}
					spRagdollSharedData.m_pObject->pSharedLookData = pLookData;
					// Havok's operator new (no null test), then the constructor, inlined.
					hkaSkeleton* pSkeleton = static_cast<hkaSkeleton*>(hkMemoryRouter::getInstance()
							.m_heap->blockAlloc(sizeof(hkaSkeleton)));
					pSkeleton->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkaSkeleton));
					::new (pSkeleton) hkaSkeleton;
					spRagdollSharedData.m_pObject->pSharedLookData->pHeadSkel = pSkeleton;
					{
						// EXE-INLINED none: hkArray<hkaBone>::setSize, with hkArrayUtil's destruct and construct of
						// the bones
						hkArray<hkaBone>& rBones = spRagdollSharedData.m_pObject->pSharedLookData->pHeadSkel->m_bones;
						hkMemoryAllocator& rHeap = *hkMemoryRouter::getInstance().m_heap;
						rBones._reserve(rHeap, 2);
						int32_t iOldCount = rBones.m_size - 2;
						if (iOldCount > 0) {
							hkaBone* pOld = rBones.m_data + 2;
							do {
								pOld->m_name.~hkStringPtr();
								++pOld;
							} while (--iOldCount);
						}
						int32_t iNewCount = 2 - rBones.m_size;
						hkaBone* pNew = rBones.m_data + rBones.m_size;
						for (int32_t i = 0; i < iNewCount; ++i) {
							// Placement new's null test.
							hkaBone* pBone = &pNew[i];
							if (pBone) {
								::new (&pBone->m_name) hkStringPtr;
								pBone->m_lockTranslation.m_bool = 0;
							}
						}
						rBones.m_size = 2;
					}
					spRagdollSharedData.m_pObject->pSharedLookData->pHeadSkel->m_parentIndices.setSize(2);
					spRagdollSharedData.m_pObject->pSharedLookData->pHeadSkel->m_referencePose.setSize(2);
					spRagdollSharedData.m_pObject->pSharedLookData->pHeadSkel->m_parentIndices.m_data[0] = -1;
					spRagdollSharedData.m_pObject->pSharedLookData->pHeadSkel->m_parentIndices.m_data[1] = 0;
					NiTransform kTransform;
					kTransform.MakeIdentity();
					NI2HK(spRagdollSharedData.m_pObject->pSharedLookData->pHeadSkel->m_referencePose.m_data[0],
						kTransform);
					kTransform.m_Translate.x = 9.0f;
					kTransform.m_Translate.y = 6.0f;
					kTransform.m_Translate.z = 0.0f;
					NI2HK(spRagdollSharedData.m_pObject->pSharedLookData->pHeadSkel->m_referencePose.m_data[1],
						kTransform);
					spRagdollSharedData.m_pObject->pSharedLookData->FwdParentMS = BoneParamA[1].FwdParentMS;
					spRagdollSharedData.m_pObject->pSharedLookData->FwdLS = BoneParamA[1].FwdLS;
				}
				// EXE-INLINED none: hkaPose::hkaPose(const hkaSkeleton*)
				hkaPose* pPose =
					static_cast<hkaPose*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkaPose)));
				if (pPose) {
					pPose->m_skeleton = spRagdollSharedData.m_pObject->pSharedLookData->pHeadSkel;
					pPose->m_localPose.m_data = nullptr;
					pPose->m_localPose.m_size = 0;
					pPose->m_localPose.m_capacityAndFlags = hkArrayBase<hkQsTransform>::DONT_DEALLOCATE_FLAG;
					pPose->m_modelPose.m_data = nullptr;
					pPose->m_modelPose.m_size = 0;
					pPose->m_modelPose.m_capacityAndFlags = hkArrayBase<hkQsTransform>::DONT_DEALLOCATE_FLAG;
					pPose->m_boneFlags.m_data = nullptr;
					pPose->m_boneFlags.m_size = 0;
					pPose->m_boneFlags.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
					pPose->m_modelInSync.m_bool = 0;
					pPose->m_localInSync.m_bool = 0;
					pPose->m_floatSlotValues.m_data = nullptr;
					pPose->m_floatSlotValues.m_size = 0;
					pPose->m_floatSlotValues.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
					int32_t iNumBones = pPose->m_skeleton->m_bones.m_size;
					pPose->m_modelPose.setSize(iNumBones);
					pPose->m_localPose.setSize(iNumBones);
					pPose->m_boneFlags.setSize(iNumBones);
					pPose->m_floatSlotValues.reserveExactly((pPose->m_skeleton->m_floatSlots.m_size + 3) & ~3);
					// The SDK's setSize(n, 0.0f), which src's hkArray leaves out: the storage grown, the new slots
					// zeroed (the exe's rep stosd).
					int32_t iNumSlots = pPose->m_skeleton->m_floatSlots.m_size;
					pPose->m_floatSlotValues.reserve(iNumSlots);
					for (int32_t i = pPose->m_floatSlotValues.m_size; i < iNumSlots; ++i) {
						pPose->m_floatSlotValues.m_data[i] = 0.0f;
					}
					pPose->m_floatSlotValues.m_size = iNumSlots;
				}
				pHeadPose = pPose;
				pPose->setToReferencePose();
				pHeadPose->syncModelSpace();
				hkaPose* pLookPose = pHeadPose;
				BoneParamA[0].ssBoneIdx = 1;
				hkVector4 kForward;
				kForward.set(0.0f, 1.0f, 0.0f, 0.0f);
				BoneParamA[0].FwdLS = kForward;
				BoneParamA[0].FwdParentMS = kForward;
				BoneParamA[0].ssParentIdx = 0;
				BoneParamA[0].pPose = pLookPose;
				eCurrentBone = bhkRagdollController::EYE_BONE;
			}
		}
		bInitLookIK = bResult;
	}
	return bResult;
}

// The exe copies both limits with fld/fstp.
void bhkRagdollController::SetLookIKDeadZone(bhkRagdollController::LookIKBone aeBone, float afHeadingMax,
	float afPitchMax) {
	BoneParamA[aeBone].fDeadHeading = afHeadingMax;
	BoneParamA[aeBone].fDeadPitch = afPitchMax;
}

bool bhkRagdollController::QLookIKEnable() {
	return (bool)((uint8_t)bEnableLookIK);
}

void bhkRagdollController::SetAlignFeetToTerrain(bool abAlign) {
	bAlignFeetToTerrain = abAlign;
}

// Builds a foot placement solver for each leg named in arboneNameA (thigh, its first child the calf, the calf's first
// child the foot) and keeps them only when every leg got one. The exe's cleanup-only C++ EH frame is left out.
// KEEP (partcopy): bytes 0xA-0xF (after m_ankleIndex) and 0x75-0x7F (after m_useFootLocking) of setupData are written
// by nothing (Setup::Setup 0xCD8920 and ExtractBipedSetupData 0xC77AC0 skip them), and hkaFootPlacementIkSolver's
// constructor (0xCD8820) copies the Setup whole (0x80 bytes, rep movsd) into each new solver.
void bhkRagdollController::InitFootIK(BSSimpleArray<const char*, 1024>& arboneNameA) {
	uint32_t iNumFeet = arboneNameA.iSize;
	if (!iNumFeet || !bInitRagdollAnim) {
		return;
	}
	FNVD_CONSTRUCTED_LOCAL(hkaFootPlacementIkSolver::Setup, setupData, nullptr);
	bool bextractResults = ExtractBipedSetupData(setupData, arboneNameA);
	const hkaSkeleton* pskeleton = spRagdollSharedData.m_pObject->pHighSkel;
	if (!bextractResults) {
		return;
	}
	setupData.m_originalGroundHeightMS = gs_fOriginalGroundHeightMS_FootIK.uValue.f;
	setupData.m_skeleton = pskeleton;
	setupData.m_worldUpDirectionWS.set(0.0f, 0.0f, 1.0f, 0.0f);
	setupData.m_modelUpDirectionMS = setupData.m_worldUpDirectionWS;
	// EXE-INLINED none: hkArrayBase<FootIkLegInfo>::_setSize
	// The reserve, then FootIkLegInfo's constructor on each new element; no destructor runs on a shrink.
	LegInfoA.reserve(static_cast<int32_t>(iNumFeet));
	int32_t iNumNew = static_cast<int32_t>(iNumFeet) - LegInfoA.m_size;
	FootIkLegInfo* pNewLeg = &LegInfoA.m_data[LegInfoA.m_size];
	for (; iNumNew > 0; --iNumNew, ++pNewLeg) {
		if (pNewLeg) {
			pNewLeg->fResultError = 0.0f;
			pNewLeg->pSolver = nullptr;
			pNewLeg->RagdollFootIdx = -1;
		}
	}
	LegInfoA.m_size = static_cast<int32_t>(iNumFeet);
	for (uint32_t i = 0; i < iNumFeet; ++i) {
		const char* pBoneName = arboneNameA.pBuffer[i];
		NiAVObject* pcalfObj = nullptr;
		FNVD_INLINED_LOCAL(NiFixedString, boneName, std::nullopt, std::nullopt);
		if (pBoneName) {
			boneName.m_kHandle = NiGlobalStringTable::AddString(pBoneName);
		} else {
			boneName.m_kHandle = NiGlobalStringTable::NULL_STRING;
		}
		NiAVObject* pthighObj = BSUtilities::GetObjectByName(pRagdollRoot, boneName, true);
		// The exe's inlined destructor of boneName.
		// EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
		NiGlobalStringTable::ReleaseHandle(boneName.GetHandle());
		// The exe reads the children through the object it tested with IsNode, not through IsNode's result.
		if (pthighObj && pthighObj->IsNode() && static_cast<NiNode*>(pthighObj)->m_kChildren.m_usSize) {
			pcalfObj = static_cast<NiNode*>(pthighObj)->m_kChildren.m_pBase[0].m_pObject;
		}
		NiAVObject* pfootObj = nullptr;
		if (pcalfObj && pcalfObj->IsNode() && static_cast<NiNode*>(pcalfObj)->m_kChildren.m_usSize) {
			pfootObj = static_cast<NiNode*>(pcalfObj)->m_kChildren.m_pBase[0].m_pObject;
		}
		bhkCollisionObject* pthighCO = pthighObj ? bhkUtilFunctions::FindNextCollisionObject(pthighObj) : nullptr;
		bhkCollisionObject* pcalfCO = pcalfObj ? bhkUtilFunctions::FindNextCollisionObject(pcalfObj) : nullptr;
		bhkCollisionObject* pfootCO = pfootObj ? bhkUtilFunctions::FindNextCollisionObject(pfootObj) : nullptr;
		hkCapsuleShapeCinfo* pfootCapsuleInfo = GetCapsuleBoneShapeInfo(pfootObj);
		float ffootRad = pfootCapsuleInfo ? pfootCapsuleInfo->m_radius : 0.0f;
		setupData.m_raycastDistanceUp = ffootRad + 4.000000059604645;
		setupData.m_raycastDistanceDown = ffootRad + 6.9999998807907104;
		fDownCastDistance = setupData.m_raycastDistanceDown;
		const char* phipStr = (pthighCO ? pthighCO->m_pkSceneObject->m_kName : EmptyFS).GetHandle();
		const char* pkneeStr = (pcalfCO ? pcalfCO->m_pkSceneObject->m_kName : EmptyFS).GetHandle();
		const char* panklStr = (pfootCO ? pfootCO->m_pkSceneObject->m_kName : EmptyFS).GetHandle();
		if (pkneeStr && panklStr && pthighCO != pcalfCO && pthighCO != pfootCO && pfootCO != pcalfCO) {
			setupData.m_hipIndex = hkaSkeletonUtils::findBoneWithName(*pskeleton, phipStr, nullptr);
			setupData.m_kneeIndex = hkaSkeletonUtils::findBoneWithName(*pskeleton, pkneeStr, nullptr);
			setupData.m_ankleIndex = hkaSkeletonUtils::findBoneWithName(*pskeleton, panklStr, nullptr);
			if (setupData.m_hipIndex != -1 && setupData.m_kneeIndex != -1 && setupData.m_ankleIndex != -1) {
				void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkaFootPlacementIkSolver));
				static_cast<hkaFootPlacementIkSolver*>(pMemory)->m_memSizeAndFlags =
					static_cast<uint16_t>(sizeof(hkaFootPlacementIkSolver));
				LegInfoA.m_data[i].pSolver = ::new (pMemory) hkaFootPlacementIkSolver(setupData);
			}
		}
		if (spRagdollSharedData.m_pObject->pSGToRagdoll) {
			for (int32_t j = 0; j < spRagdollSharedData.m_pObject->pSGToRagdoll->m_mapping.m_simpleMappings.m_size;
				++j) {
				const hkaSkeletonMapperData::SimpleMapping& rMapping =
					spRagdollSharedData.m_pObject->pSGToRagdoll->m_mapping.m_simpleMappings.m_data[j];
				if (rMapping.m_boneA == setupData.m_ankleIndex) {
					LegInfoA.m_data[i].RagdollFootIdx = rMapping.m_boneB;
				}
			}
		}
	}
	bool bHasFootSolver = true;
	if (iNumFeet) {
		for (uint32_t i = 0; i < iNumFeet && bHasFootSolver; ++i) {
			if (!LegInfoA.m_data[i].pSolver) {
				bHasFootSolver = false;
			}
		}
		if (!bHasFootSolver) {
			for (uint32_t i = 0; i < iNumFeet; ++i) {
				hkaFootPlacementIkSolver* pSolver = LegInfoA.m_data[i].pSolver;
				if (pSolver) {
					pSolver->removeReference();
				}
			}
			LegInfoA.m_size = 0;
		}
	}
	bInitFootIK = bHasFootSolver;
}

bool bhkRagdollController::QFootIKEnable() {
	return (bool)((uint8_t)bEnableFootIK);
}

bool bhkRagdollController::QGrabIKEnable() {
	return (bool)((uint8_t)bEnableGrabIK);
}

void bhkRagdollController::SetCollisionGroup(uint32_t aiGroup) {
	iGroup = aiGroup;
}

// Builds the pose matcher from the named pose array: the three bone indices (looked up by name where -1 is given),
// then each pose of the array, taken to model space, as a candidate. The exe's cleanup-only C++ EH frame is left out.
// The two pose buffers of each pass are the inlined hkLocalBuffer<hkQsTransform>: the thread's stack allocator hands
// out the bone count's poses rounded up to 128 bytes and takes them back, the later buffer first.
void bhkRagdollController::InitPoseMatching(const char* apFileName, int16_t asRootIndex, int16_t asBone2Index,
	int16_t asBone3Index) {
	if (!bInitRagdollAnim) {
		return;
	}
	const char* pOldHandle = PoseAStr.GetHandle();
	if (pOldHandle != apFileName) {
		PoseAStr.m_kHandle = NiGlobalStringTable::AddString(apFileName);
		NiGlobalStringTable::ReleaseHandle(pOldHandle);
	}
	const hkaSkeleton* pskel = spRagdollSharedData.m_pObject->pLowSkel;
	int16_t root = asRootIndex;
	if (asRootIndex == -1) {
		root = hkaSkeletonUtils::findBoneWithName(*pskel, "Bip01 NonAccum", nullptr);
	}
	int16_t other = asBone2Index;
	if (asBone2Index == -1) {
		other = hkaSkeletonUtils::findBoneWithName(*pskel, "Bip01 Head", nullptr);
	}
	int16_t another = asBone3Index;
	if (asBone3Index == -1) {
		another = hkaSkeletonUtils::findBoneWithName(*pskel, "Bip01 R UpperArm", nullptr);
	}
	hkVector4 UP;
	UP.set(0.0f, 0.0f, 1.0f, 0.0f);
	if (!IsPoweredRagdoll() && root != -1 && other != -1 && another != -1) {
		return;
	}
	void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkaPoseMatchingUtility));
	pMatchUtil = pMemory ? new (pMemory) hkaPoseMatchingUtility(root, other, another, UP) : nullptr;
	bhkRagdollController::PoseMap.GetAt(apFileName, spPoseA);
	bhkPoseArray* pPoseArray = spPoseA.m_pObject;
	if (!pPoseArray) {
		return;
	}
	if (pPoseArray->uiNumPose) {
		uint32_t i = 0;
		do {
			bhkPoseArray::bhkPose* pPose = spPoseA.m_pObject->PoseA[i];
			int32_t iNumBones = pskel->m_bones.m_size;
			int32_t iBufferSize = (iNumBones * static_cast<int32_t>(sizeof(hkQsTransform)) + 0x7F) & ~0x7F;
			// EXE-INLINED none: hkLocalBuffer<hkQsTransform>::hkLocalBuffer
			hkMemoryRouter& kLocalRouter = hkMemoryRouter::getInstance();
			char* pLocalBuffer = kLocalRouter.m_stack.m_next.m_storage;
			kLocalRouter.m_stack.m_next.m_storage = pLocalBuffer + iBufferSize;
			// EXE-INLINED none: hkLocalBuffer<hkQsTransform>::hkLocalBuffer
			hkMemoryRouter& kModelRouter = hkMemoryRouter::getInstance();
			char* pModelBuffer = kModelRouter.m_stack.m_next.m_storage;
			kModelRouter.m_stack.m_next.m_storage = pModelBuffer + iBufferSize;
			// The stack allocator's bytes are untyped memory; the exe uses them as the poses.
			hkQsTransform* pLocalPose = reinterpret_cast<hkQsTransform*>(pLocalBuffer);
			hkQsTransform* pModelPose = reinterpret_cast<hkQsTransform*>(pModelBuffer);
			for (int32_t iBone = 0; iBone < iNumBones; ++iBone) {
				pLocalPose[iBone] = pPose->BoneTransfA[iBone];
			}
			hkaSkeletonUtils::transformLocalPoseToModelPose(iNumBones, pskel->m_parentIndices.m_data, pLocalPose,
				pModelPose);
			pMatchUtil->addCandidatePose(pModelPose, nullptr, 0.0f);
			// EXE-INLINED 0xC7B130
			hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pModelBuffer;
			// EXE-INLINED 0xC7B130
			hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pLocalBuffer;
			++i;
		} while (i < spPoseA.m_pObject->uiNumPose);
	}
	bInitPoseMatch = spPoseA.m_pObject->BoneMatch(pskel);
}

void bhkRagdollController::SetFeedbackData(RagdollFeedbackData& arFeedbackData) {
	spRagdollSharedData.m_pObject->pFeedbackData = &arFeedbackData;
	bInitRagdollFeedback = 1;
	bEnableRagdollFeedback = arFeedbackData.bEnabled;
	hkaKeyFrameHierarchyUtility::ControlData* rcontrolData = pRagdollRBController->m_controlDataPalette.begin();
	rcontrolData->m_hierarchyGain = arFeedbackData.SystemData.fHeirGain;
	rcontrolData->m_velocityDamping = arFeedbackData.SystemData.fVelDamp;
	rcontrolData->m_accelerationGain = arFeedbackData.SystemData.fAccGain;
	rcontrolData->m_velocityGain = arFeedbackData.SystemData.fVelGain;
	rcontrolData->m_positionGain = arFeedbackData.SystemData.fPosGain;
	rcontrolData->m_positionMaxLinearVelocity = arFeedbackData.SystemData.fPosMaxLinVel;
	rcontrolData->m_positionMaxAngularVelocity = arFeedbackData.SystemData.fPosMaxAngVel;
	rcontrolData->m_snapGain = arFeedbackData.SystemData.fSnapGain;
	rcontrolData->m_snapMaxLinearVelocity = arFeedbackData.SystemData.fSnapMaxLinVel;
	rcontrolData->m_snapMaxAngularVelocity = arFeedbackData.SystemData.fSnapMaxAngVel;
	rcontrolData->m_snapMaxLinearDistance = arFeedbackData.SystemData.fSnapMaxLinDist;
	rcontrolData->m_snapMaxAngularDistance = arFeedbackData.SystemData.fSnapMaxAngDist;
}

// The exe's cleanup-only C++ EH frame is left out. Turning the feedback on or off notes each dynamic bone's
// velocities, switches the feedback's dynamic bones to dynamic (on) or keyframed (off), then gives back every
// velocity that changed by more than 0.001. The two velocity buffers are the inlined hkLocalBuffer<hkVector4>: the
// thread's stack allocator hands out the bone count's vectors rounded up to 128 bytes and takes them back, the later
// buffer first.
void bhkRagdollController::SetRagdollFeedbackActive(int32_t abActive) {
	bool bActive = static_cast<uint8_t>(abActive) != 0;
	if (!bEnableRagdollFeedback || (!bActive && !bActiveRagdollFeedback)) {
		return;
	}
	if (bActiveRagdollFeedback && bActive) {
		FeedbackTimer.Reset(bhkRagdollController::fFeedbackTimeMS);
		return;
	}
	UpdateConstraintsWorld(abActive);
	if (bActive) {
		float fTime = bhkRagdollController::fFeedbackTimeMS;
		FeedbackTimer.iStartTime = BSPrecisionTimer::GetTimer();
		FeedbackTimer.iTargetTime = static_cast<int64_t>(BSPrecisionTimer::fFrequencyMS * fTime);
	} else {
		FeedbackTimer.iTargetTime = 0;
	}
	int32_t iNumBones = pRagdollInstance->m_skeleton->m_bones.m_size;
	int32_t iBufferSize = (iNumBones * static_cast<int32_t>(sizeof(hkVector4)) + 0x7F) & ~0x7F;
	// EXE-INLINED none: hkLocalBuffer<hkVector4>::hkLocalBuffer
	hkMemoryRouter& kLinearRouter = hkMemoryRouter::getInstance();
	char* pLinearBuffer = kLinearRouter.m_stack.m_next.m_storage;
	kLinearRouter.m_stack.m_next.m_storage = pLinearBuffer + iBufferSize;
	// EXE-INLINED none: hkLocalBuffer<hkVector4>::hkLocalBuffer
	hkMemoryRouter& kAngularRouter = hkMemoryRouter::getInstance();
	char* pAngularBuffer = kAngularRouter.m_stack.m_next.m_storage;
	kAngularRouter.m_stack.m_next.m_storage = pAngularBuffer + iBufferSize;
	// The stack allocator's bytes are untyped memory; the exe uses them as the velocities.
	hkVector4* pLinearVelA = reinterpret_cast<hkVector4*>(pLinearBuffer);
	hkVector4* pAngularVelA = reinterpret_cast<hkVector4*>(pAngularBuffer);
	hkString::memSet(pLinearVelA, 0, iNumBones * static_cast<int32_t>(sizeof(hkVector4)));
	hkString::memSet(pAngularVelA, 0, iNumBones * static_cast<int32_t>(sizeof(hkVector4)));
	for (int32_t i = 0; i < iNumBones; ++i) {
		hkaRagdollInstance* pInstance = pRagdollInstance;
		int32_t iBody = pInstance->m_boneToRigidBodyMap.m_data[i];
		if (iBody >= 0) {
			hkpRigidBody* pRigidBody = pInstance->m_rigidBodies.m_data[iBody];
			if (pRigidBody) {
				hkpMotion::MotionType eMotion =
					static_cast<hkpMotion::MotionType>(pRigidBody->m_motion.m_type.m_storage);
				if (bhkRigidBody::IsMotionTypeDynamic(eMotion)) {
					pLinearVelA[i] = pRigidBody->m_motion.m_linearVelocity;
					pAngularVelA[i] = pRigidBody->m_motion.m_angularVelocity;
				}
			}
		}
	}
	for (int32_t i = 0; i < iNumBones; ++i) {
		hkaRagdollInstance* pInstance = pRagdollInstance;
		int32_t iBody = pInstance->m_boneToRigidBodyMap.m_data[i];
		if (iBody < 0) {
			continue;
		}
		hkpRigidBody* pRigidBody = pInstance->m_rigidBodies.m_data[iBody];
		if (!pRigidBody) {
			continue;
		}
		// The bone's index in the feedback's dynamic bones, or -1.
		const BSSimpleArray<int16_t, 1024>& rDynamicBoneIdxA =
			spRagdollSharedData.m_pObject->pFeedbackData->DynamicBoneIdxA;
		uint32_t uiNumDynamic = rDynamicBoneIdxA.iSize;
		int16_t* pDynamicIdx = rDynamicBoneIdxA.pBuffer;
		int32_t iDynamicIdx = -1;
		for (uint32_t j = 0; j < uiNumDynamic; ++j) {
			if (pDynamicIdx && pDynamicIdx[j] == static_cast<int16_t>(i)) {
				iDynamicIdx = static_cast<int32_t>(j);
				break;
			}
		}
		if (iDynamicIdx != -1) {
			if (bActive) {
				if (pRigidBody->m_motion.m_type.m_storage == hkpMotion::MOTION_KEYFRAMED ||
					pRigidBody->m_collidable.m_broadPhaseHandle.m_objectQualityType != HK_COLLIDABLE_QUALITY_MOVING) {
					pRigidBody->setMotionType(hkpMotion::MOTION_DYNAMIC, HK_ENTITY_ACTIVATION_DO_ACTIVATE,
						HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK);
					pRigidBody->m_collidable.m_broadPhaseHandle.m_objectQualityType = HK_COLLIDABLE_QUALITY_MOVING;
					pRigidBody->enableDeactivation(false);
				}
			} else if (pRigidBody->m_motion.m_type.m_storage != hkpMotion::MOTION_KEYFRAMED ||
				pRigidBody->m_collidable.m_broadPhaseHandle.m_objectQualityType != HK_COLLIDABLE_QUALITY_KEYFRAMED) {
				pRigidBody->setMotionType(hkpMotion::MOTION_KEYFRAMED, HK_ENTITY_ACTIVATION_DO_ACTIVATE,
					HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK);
				pRigidBody->m_collidable.m_broadPhaseHandle.m_objectQualityType = HK_COLLIDABLE_QUALITY_KEYFRAMED;
				pRigidBody->enableDeactivation(true);
			}
		}
		if (!pLinearVelA[i].equals4(pRigidBody->m_motion.m_linearVelocity)) {
			// EXE-INLINED 0x5616D0
			pRigidBody->activate();
			static_cast<hkpMotion*>(&pRigidBody->m_motion)->setLinearVelocity(pLinearVelA[i]);
		}
		if (!pAngularVelA[i].equals4(pRigidBody->m_motion.m_angularVelocity)) {
			// EXE-INLINED 0x561800
			pRigidBody->activate();
			static_cast<hkpMotion*>(&pRigidBody->m_motion)->setAngularVelocity(pAngularVelA[i]);
		}
	}
	bActiveRagdollFeedback = bActive;
	// EXE-INLINED none: hkLocalBuffer<hkVector4>::~hkLocalBuffer
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pAngularBuffer;
	// EXE-INLINED none: hkLocalBuffer<hkVector4>::~hkLocalBuffer
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pLinearBuffer;
}

void bhkRagdollController::DisableRagdollAnim(bool abUpdateConstraints) {
	if (bInitRagdollAnim) {
		bool bActiveFootIK = this->bActiveFootIK;
		bEnableRagdollAnim = 0;
		bEaseOutRagdollAnim = 0;
		this->bActiveFootIK = 0;
		if (bActiveFootIK) {
			bResetSolverWeight = 1;
		}
		bActiveGrabIK = 0;
		bActiveLookIK = 0;
		BoneParamA[0].PrevRotLS.m_vec.m_quad = hkQuadReal0001;
		BoneParamA[1].PrevRotLS.m_vec.m_quad = hkQuadReal0001;
		BoneParamA[1].bLimitBoneRot = 1;
		BoneParamA[0].bLimitBoneRot = 1;
		if (bEnableRagdollFeedback) {
			SetRagdollFeedbackActive(0);
		}
		bhkRagdollShareData* m_pObject = spRagdollSharedData.m_pObject;
		if (m_pObject) {
			int32_t m_size = m_pObject->pHighSkel->m_bones.m_size;
			for (int32_t i = 0; i < m_size; ++i) {
				bhkNiCollisionObject* pcol = SGBonesA.pBuffer[i]->m_spCollisionObject.m_pObject;
				if (pcol) {
					bhkBlendCollisionObject* pblendCol = pcol->IsBhkBlendCollisionObject();
					if (pblendCol) {
						pblendCol->bRagdollUpdated = 0;
					}
				}
			}
		}
		bEaseOutLookIK = 0;
		GrabIKState[1].fReachWeight = 0.0f;
		GrabIKState[1].bEnabled = 0;
		GrabIKState[0].fReachWeight = 0.0f;
		GrabIKState[0].bEnabled = 0;
		bEaseOutFootIK = 0;
		fCurrFeedbackInterp = 1.0f;
		iCurrFootEase = 0;
		bEaseOutFeedback = 0;
		uiFeedbackTimeoutCount = 0;
		if (abUpdateConstraints) {
			hkpConstraintOwner* m_owner;
			hkaRagdollInstance* pRagdollInstance = this->pRagdollInstance;
			hkpConstraintInstance** m_data = pRagdollInstance->m_constraints.begin();
			if (*m_data) {
				m_owner = (*m_data)->getOwner();
			} else {
				m_owner = 0;
			}
			hkpRigidBody** v12 = pRagdollInstance->m_rigidBodies.begin();
			if (*v12) {
				ahkpWorld* m_world = static_cast<ahkpWorld*>((*v12)->m_world);
				if (m_world) {
					if (!m_owner) {
						bhkWorld* m_userData = static_cast<bhkWorld*>(m_world->m_userData);
						for (int32_t j = 0; j < pRagdollInstance->m_constraints.m_size; ++j) {
							if (!pRagdollInstance->m_constraints.m_data[j]->getOwner()) {
								m_userData->Add(pRagdollInstance->m_constraints.m_data[j]);
							}
						}
					}
				}
			}
		}
	}
}

bool bhkRagdollController::GetRagdollAnimEnable() {
	return (bool)((uint8_t)bEnableRagdollAnim);
}

hkpRigidBody* bhkRagdollController::GetRigidBodyHead() {
	// The exe inlines hkaRagdollInstance's bone-to-rigid-body lookup here.
	hkaRagdollInstance* pInstance = pRagdollInstance;
	if (pInstance) {
		int32_t iBody = pInstance->m_boneToRigidBodyMap.m_data[nHeadBone];
		if (iBody >= 0)
			return pInstance->m_rigidBodies.m_data[iBody];
	}
	return nullptr;
}

void bhkRagdollController::EnableRagdollFeedback(bool abEnable) {
	if (bInitRagdollFeedback && abEnable) {
		bEnableRagdollFeedback = true;
	} else {
		if (bActiveRagdollFeedback)
			SetRagdollFeedbackActive(0);
		bEnableRagdollFeedback = false;
	}
}

void bhkRagdollController::ClearVertError() {
	fVertFootIkError = (float)0.0;
}

void bhkRagdollController::SetEaseOutLookAtIK(bool abEnable) {
	bEaseOutLookIK = (bool)((uint8_t)abEnable);
}

bool bhkRagdollController::QRagdollAnimEaseOut() {
	return (bool)((uint8_t)bEaseOutRagdollAnim);
}

uint8_t bhkRagdollController::GetEaseOutLookAtIK() {
	return (uint8_t)bEaseOutLookIK;
}

void bhkRagdollController::SetLookAtTarget(NiPoint3* apTarget, bool abEnable) {
	NI2HK(CurrentTargetWS, *apTarget);
	CurrentTargetWS.operator()(3) = 0.0;
	if (abEnable) {
		SetLookIKActive(1);
		bEaseOutLookIK = 0;
		bEaseOutRagdollAnim = 0;
	}
}

void bhkRagdollController::SetMaxEyePitch(float afAngle) {
	fMaxEyePitchAngle = afAngle;
}

uint8_t bhkRagdollController::GetLookIKActive() {
	return (uint8_t)bActiveLookIK;
}

void bhkRagdollController::GetEyeAngles(float& arHeading, float& arPitch) {
	arHeading = fEyeHeading;
	arPitch = fEyePitch;
}

void bhkRagdollController::UseFootDownCast(bool abCastDown) {
	bDownCastFoot = (bool)((uint8_t)abCastDown);
}

void bhkRagdollController::SetUseFootEndCast(bool abCast) {
	bFootEndCast = (bool)((uint8_t)abCast);
}

bhkWorld* bhkRagdollController::GetWorld() const {
	hkaRagdollInstance* pRagdollInstance;
	hkpRigidBody* v2;
	ahkpWorld* m_world;
	pRagdollInstance = this->pRagdollInstance;
	if (pRagdollInstance &&
		(((v2 = *pRagdollInstance->m_rigidBodies.m_data) != 0) &&
			((m_world = static_cast<ahkpWorld*>(v2->m_world)) != 0))) {
		return static_cast<bhkWorld*>(m_world->m_userData);
	} else {
		return nullptr;
	}
}

void bhkRagdollController::SetFootIKActive(bool abActive) {
	uint8_t v3;
	uint8_t bFootIKActive = (uint8_t)bActiveFootIK;
	if (bEaseOutRagdollAnim) {
		v3 = (uint8_t)bActiveFootIK;
	} else {
		v3 = (uint8_t)(bEnableFootIK && abActive);
	}
	bActiveFootIK = (bool)v3;
	if (bFootIKActive && (!v3)) {
		bResetSolverWeight = (bool)1;
	}
}

// The exe's EH frame only releases the stack buffer on unwind; it is left out. The pose buffer is the inlined
// hkLocalBuffer<hkQsTransform>: the thread's stack allocator hands out the bone count's poses rounded up to 128
// bytes and takes them back at the end.
void bhkRagdollController::DoRagdollAnim(bool abUpdateBones) {
	if (bEaseOutRagdollAnim && !bActiveLookIK && !bActiveFootIK && !bActiveGrabIK && !bActiveRagdollFeedback) {
		bEaseOutRagdollAnim = false;
		bEnableRagdollAnim = false;
	}
	if (!bRagdollAnim.uValue.b || !bEnableRagdollAnim) {
		return;
	}
	NiTransform kWorldFromModel = pRagdollRoot->m_pkParent->m_kWorld * pRagdollRoot->m_kLocal;
	kWorldFromModel.m_Rotate = ScaleInv * kWorldFromModel.m_Rotate;
	kWorldFromModel.m_fScale = 1.0f;
	NI2HK(WorldFromModel, kWorldFromModel);
	if (!bFullyDynamic) {
		GetSGPoseLocalSpace();
		UpdateDebugLines(SKEL_KEYFRAMED);
		if (bFootIK.uValue.b && (bActiveFootIK || bResetSolverWeight)) {
			DoFootIK(WorldFromModel);
		}
		if (bGrabIK.uValue.b && bActiveGrabIK) {
			DoGrabIK(WorldFromModel);
		}
		if (bLookIK.uValue.b && bActiveLookIK) {
			DoLookAtIK(WorldFromModel);
		}
	}
	int32_t iNumBones = (int32_t)pRagdollInstance->m_skeleton->m_bones.m_size;
	bool bUseDrivePose = false;
	// EXE-INLINED none: hkLocalBuffer<hkQsTransform>::hkLocalBuffer
	hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
	char* pBuffer = kRouter.m_stack.m_next.m_storage;
	kRouter.m_stack.m_next.m_storage = pBuffer + ((iNumBones * 0x30 + 0x7F) & ~0x7F);
	// The stack allocator's bytes are untyped memory; the exe uses them as the poses.
	hkQsTransform* pDrivePose = reinterpret_cast<hkQsTransform*>(pBuffer);
	hkQsTransform* pRigidBodyPose = nullptr;
	if (bPoseMatching.uValue.b && bActivePoseMatch) {
		DoPoseMatching(WorldFromModel, pDrivePose, bUseDrivePose);
		if (bUseDrivePose) {
			pRigidBodyPose = pDrivePose;
		}
	}
	UpdateRagdollRigidBodies(pRigidBodyPose);
	UpdateDebugLines(SKEL_RAGDOLL);
	if (!bFullyDynamic) {
		if (bRagdollFeedback.uValue.b && bActiveRagdollFeedback) {
			DoRagdollFeedback();
		}
		UpdateDebugLines(SKEL_FINAL);
		SetSGPoseLocalSpace();
	}
	// EXE-INLINED 0xC7B130
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pBuffer;
}

// Sets each bone of the scene graph pose from the scene graph bone's local transform (its translation scaled by the
// ragdoll root's world scale), then syncs the pose's model space and copies its local space into AnimPoseLS.
// The exe inlines the SDK's hkaPose::setBoneLocalSpace with makeAllChildrenLocalSpace, getBoneLocalSpace and
// getBoneModelSpace, and hkQsTransform's setMulInverseMul and setMul, whose quaternion product stays a call
// (0xC66320). Its /GS cookie is left out.
void bhkRagdollController::GetSGPoseLocalSpace() {
	// EXE-INLINED none: hkaPose::getBoneModelSpace
	auto getBoneModelSpace = [](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_MODEL_DIRTY) {
			return arPose.calculateBoneModelSpace(boneIdx);
		}
		return arPose.m_modelPose.m_data[boneIdx];
	};
	// EXE-INLINED none: hkaPose::getBoneLocalSpace
	auto getBoneLocalSpace = [&](hkaPose& arPose, int32_t boneIdx) {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_LOCAL_DIRTY) {
			const int16_t parentIdx = arPose.m_skeleton->m_parentIndices.m_data[boneIdx];
			const hkQsTransform* pModelPose = arPose.m_modelPose.m_data;
			hkQsTransform& rLocal = arPose.m_localPose.m_data[boneIdx];
			if (parentIdx == -1) {
				rLocal.m_translation = pModelPose[boneIdx].m_translation;
				rLocal.m_rotation = pModelPose[boneIdx].m_rotation;
				rLocal.m_scale = pModelPose[boneIdx].m_scale;
			} else {
				const hkQsTransform& rParentMS = getBoneModelSpace(arPose, parentIdx);
				// EXE-INLINED none: hkQsTransform::setMulInverseMul
				hkQsTransform kInverse;
				kInverse.setInverse(rParentMS);
				// EXE-INLINED none: hkQsTransform::setMul
				hkVector4 kExtraTrans;
				kExtraTrans.setRotatedDir(kInverse.m_rotation, pModelPose[boneIdx].m_translation);
				rLocal.m_translation.setAdd4(kInverse.m_translation, kExtraTrans);
				rLocal.m_rotation.setMul(kInverse.m_rotation, pModelPose[boneIdx].m_rotation);
				rLocal.m_scale.setMul4(kInverse.m_scale, pModelPose[boneIdx].m_scale);
			}
			arPose.m_boneFlags.m_data[boneIdx] &= ~static_cast<uint32_t>(hkaPose::F_BONE_LOCAL_DIRTY);
		}
	};
	// EXE-INLINED none: hkaPose::makeAllChildrenLocalSpace: the bone count is read once.
	auto makeAllChildrenLocalSpace = [&](hkaPose& arPose, int32_t boneIdx) {
		const int32_t numBones = arPose.m_skeleton->m_bones.m_size;
		arPose.m_boneFlags.m_data[boneIdx] |= hkaPose::F_BONE_INTERNAL_FLAG1;
		for (int32_t b = boneIdx + 1; b < numBones; ++b) {
			const int16_t parentId = arPose.m_skeleton->m_parentIndices.m_data[b];
			if (parentId != -1 && (arPose.m_boneFlags.m_data[parentId] & hkaPose::F_BONE_INTERNAL_FLAG1)) {
				getBoneLocalSpace(arPose, b);
				arPose.m_boneFlags.m_data[b] |= hkaPose::F_BONE_INTERNAL_FLAG1;
				arPose.m_modelInSync.m_bool = 0;
			}
		}
		for (int32_t b = boneIdx + 1; b < numBones; ++b) {
			uint32_t& ruiFlags = arPose.m_boneFlags.m_data[b];
			if (ruiFlags & hkaPose::F_BONE_INTERNAL_FLAG1) {
				ruiFlags =
					(ruiFlags & ~static_cast<uint32_t>(hkaPose::F_BONE_INTERNAL_FLAG1)) | hkaPose::F_BONE_MODEL_DIRTY;
			}
		}
	};

	const int32_t iNumBones = spRagdollSharedData.m_pObject->pHighSkel->m_bones.m_size;
	const float fScale = pRagdollRoot->m_kWorld.m_fScale;
	for (int32_t i = 0; i < iNumBones; ++i) {
		hkQsTransform kTransf;
		NI2HK(kTransf, SGBonesA.pBuffer[i]->m_kLocal);
		kTransf.m_translation.m_quad = _mm_mul_ps(_mm_set1_ps(fScale), kTransf.m_translation.m_quad);
		// EXE-INLINED none: hkaPose::setBoneLocalSpace
		hkaPose* pPose = pSGPose;
		makeAllChildrenLocalSpace(*pPose, i);
		pPose->m_localPose.m_data[i].m_translation = kTransf.m_translation;
		pPose->m_localPose.m_data[i].m_rotation = kTransf.m_rotation;
		pPose->m_localPose.m_data[i].m_scale = kTransf.m_scale;
		pPose->m_boneFlags.m_data[i] = hkaPose::F_BONE_MODEL_DIRTY;
		pPose->m_modelInSync.m_bool = 0;
	}
	pSGPose->syncModelSpace();
	const hkArray<hkQsTransform, hkContainerHeapAllocator>& rSyncedPoseLS = pSGPose->getSyncedPoseLocalSpace();
	hkString::memCpy(AnimPoseLS.begin(), rSyncedPoseLS.m_data, sizeof(hkQsTransform) * iNumBones);
}

// Sets each scene graph bone's local transform from the scene graph pose's local space (its translation scaled
// back by the inverse of the ragdoll root's world scale; the scale is kept) and marks its blend collision object
// as updated by the ragdoll. The exe inlines the SDK's hkaPose::getBoneLocalSpace with getBoneModelSpace and
// hkQsTransform's setMulInverseMul and setMul, whose quaternion product stays a call (0xC66320). Its /GS cookie is
// left out.
void bhkRagdollController::SetSGPoseLocalSpace() {
	// EXE-INLINED none: hkaPose::getBoneModelSpace
	auto getBoneModelSpace = [](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_MODEL_DIRTY) {
			return arPose.calculateBoneModelSpace(boneIdx);
		}
		return arPose.m_modelPose.m_data[boneIdx];
	};
	// EXE-INLINED none: hkaPose::getBoneLocalSpace
	auto getBoneLocalSpace = [&](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_LOCAL_DIRTY) {
			const int16_t parentIdx = arPose.m_skeleton->m_parentIndices.m_data[boneIdx];
			const hkQsTransform* pModelPose = arPose.m_modelPose.m_data;
			hkQsTransform& rLocal = arPose.m_localPose.m_data[boneIdx];
			if (parentIdx == -1) {
				rLocal.m_translation = pModelPose[boneIdx].m_translation;
				rLocal.m_rotation = pModelPose[boneIdx].m_rotation;
				rLocal.m_scale = pModelPose[boneIdx].m_scale;
			} else {
				const hkQsTransform& rParentMS = getBoneModelSpace(arPose, parentIdx);
				// EXE-INLINED none: hkQsTransform::setMulInverseMul
				hkQsTransform kInverse;
				kInverse.setInverse(rParentMS);
				// EXE-INLINED none: hkQsTransform::setMul
				hkVector4 kExtraTrans;
				kExtraTrans.setRotatedDir(kInverse.m_rotation, pModelPose[boneIdx].m_translation);
				rLocal.m_translation.setAdd4(kInverse.m_translation, kExtraTrans);
				rLocal.m_rotation.setMul(kInverse.m_rotation, pModelPose[boneIdx].m_rotation);
				rLocal.m_scale.setMul4(kInverse.m_scale, pModelPose[boneIdx].m_scale);
			}
			arPose.m_boneFlags.m_data[boneIdx] &= ~static_cast<uint32_t>(hkaPose::F_BONE_LOCAL_DIRTY);
		}
		return arPose.m_localPose.m_data[boneIdx];
	};

	const int32_t iNumBones = spRagdollSharedData.m_pObject->pHighSkel->m_bones.m_size;
	pSGPose->syncLocalSpace();
	const float fInvScale = 1.0 / pRagdollRoot->m_kWorld.m_fScale;
	for (int32_t i = 0; i < iNumBones; ++i) {
		NiAVObject* pBone = SGBonesA.pBuffer[i];
		const hkQsTransform& rLocal = getBoneLocalSpace(*pSGPose, i);
		NiTransform kTransf;
		HK2NI(kTransf, rLocal);
		kTransf.m_Translate.x = kTransf.m_Translate.x * fInvScale;
		pBone->m_kLocal.m_Translate.x = kTransf.m_Translate.x;
		kTransf.m_Translate.y = kTransf.m_Translate.y * fInvScale;
		pBone->m_kLocal.m_Translate.y = kTransf.m_Translate.y;
		kTransf.m_Translate.z = fInvScale * kTransf.m_Translate.z;
		pBone->m_kLocal.m_Translate.z = kTransf.m_Translate.z;
		pBone->m_kLocal.m_Rotate = kTransf.m_Rotate;
		NiObject* pCollision = pBone->m_spCollisionObject.m_pObject;
		if (pCollision) {
			bhkBlendCollisionObject* pBlendCollision = pCollision->IsBhkBlendCollisionObject();
			if (pBlendCollision) {
				pBlendCollision->bRagdollUpdated = true;
			}
		}
	}
}

// Drives the ragdoll's rigid bodies to the animation's pose: the given local pose, or the scene graph's pose mapped
// onto the ragdoll's skeleton. The exe's cleanup-only C++ EH frame is left out. The two pose buffers are the inlined
// hkLocalBuffer<hkQsTransform>: the thread's stack allocator hands out the bone count's poses rounded up to 128 bytes
// and takes them back at the end, the later buffer first.
void bhkRagdollController::UpdateRagdollRigidBodies(const hkQsTransform* apRagdollPoseLS) {
	const hkaSkeleton* pSkeleton = pRagdollInstance->m_skeleton;
	int32_t iNumBones = pSkeleton->m_bones.m_size;
	int32_t iBufferSize = (iNumBones * static_cast<int32_t>(sizeof(hkQsTransform)) + 0x7F) & ~0x7F;
	// EXE-INLINED none: hkLocalBuffer<hkQsTransform>::hkLocalBuffer
	hkMemoryRouter& kLocalRouter = hkMemoryRouter::getInstance();
	char* pLocalBuffer = kLocalRouter.m_stack.m_next.m_storage;
	kLocalRouter.m_stack.m_next.m_storage = pLocalBuffer + iBufferSize;
	// EXE-INLINED none: hkLocalBuffer<hkQsTransform>::hkLocalBuffer
	hkMemoryRouter& kModelRouter = hkMemoryRouter::getInstance();
	char* pModelBuffer = kModelRouter.m_stack.m_next.m_storage;
	kModelRouter.m_stack.m_next.m_storage = pModelBuffer + iBufferSize;
	// The stack allocator's bytes are untyped memory; the exe uses them as the poses.
	hkQsTransform* pPoseLocal = reinterpret_cast<hkQsTransform*>(pLocalBuffer);
	hkQsTransform* pPoseModel = reinterpret_cast<hkQsTransform*>(pModelBuffer);
	RAGDOLL_UPDATE_TYPE ePrevMethod = eRBUpdateMethod;
	if (bQueuedWarp) {
		eRBUpdateMethod = WARP;
	}
	if (bQueueStopPoweredMotors) {
		bhkRagdollPoweredConstraintController::stopMotors(pRagdollInstance);
		bQueueStopPoweredMotors = false;
	}
	if (apRagdollPoseLS) {
		for (int32_t i = 0; i < iNumBones; ++i) {
			// EXE-INLINED 0xC6CD30
			pPoseLocal[i].m_translation.m_quad = apRagdollPoseLS[i].m_translation.m_quad;
			pPoseLocal[i].m_rotation.m_vec.m_quad = apRagdollPoseLS[i].m_rotation.m_vec.m_quad;
			pPoseLocal[i].m_scale.m_quad = apRagdollPoseLS[i].m_scale.m_quad;
		}
	} else {
		hkaSkeletonUtils::transformLocalPoseToModelPose(iNumBones, pSkeleton->m_parentIndices.m_data,
			pSkeleton->m_referencePose.m_data, pPoseModel);
		const hkQsTransform* pReferencePose = pSkeleton->m_referencePose.m_data;
		const hkArray<hkQsTransform>& kSyncedPose = pSGPose->getSyncedPoseModelSpace();
		hkaSkeletonMapper::mapPose(spRagdollSharedData.m_pObject->pSGToRagdoll, kSyncedPose.m_data, pReferencePose,
			pPoseModel, hkaSkeletonMapper::CURRENT_POSE);
		if (eRBUpdateMethod != WARP) {
			hkaSkeletonUtils::transformModelPoseToLocalPose(iNumBones, pSkeleton->m_parentIndices.m_data, pPoseModel,
				pPoseLocal);
		}
	}
	if (eRBUpdateMethod == WARP) {
		pRagdollInstance->setPoseModelSpace(pPoseModel, WorldFromModel);
	} else if (eRBUpdateMethod == APPLY_VELOCITY) {
		if (bReinitRBController) {
			pRagdollRBController->reinitialize();
			bReinitRBController = false;
		}
		if (bhkWorld::fDeltaTime > 0.0f) {
			// EXE-INLINED none: hkArray<hkaKeyFrameHierarchyUtility::Output>::hkArray
			hkMemoryAllocator& kHeap = *hkMemoryRouter::getInstance().m_heap;
			hkaKeyFrameHierarchyUtility::Output* pStress;
			int32_t iStressCapacityAndFlags;
			if (iNumBones) {
				// EXE-INLINED none: hkMemoryAllocator::_bufAlloc
				// The byte count bufAlloc updates sits in a 16-aligned slot of the exe's realigned frame.
				alignas(16) int32_t iStressBytes =
					iNumBones * hkSizeOfTypeOrVoid<hkaKeyFrameHierarchyUtility::Output>::val;
				pStress = static_cast<hkaKeyFrameHierarchyUtility::Output*>(kHeap.bufAlloc(iStressBytes));
				int32_t iNumStress = iStressBytes / hkSizeOfTypeOrVoid<hkaKeyFrameHierarchyUtility::Output>::val;
				iStressCapacityAndFlags = iNumStress
					? iNumStress
					: static_cast<int32_t>(hkArrayBase<hkaKeyFrameHierarchyUtility::Output>::DONT_DEALLOCATE_FLAG);
			} else {
				pStress = nullptr;
				iStressCapacityAndFlags =
					static_cast<int32_t>(hkArrayBase<hkaKeyFrameHierarchyUtility::Output>::DONT_DEALLOCATE_FLAG);
			}
			pRagdollRBController->driveToPose(bhkWorld::fDeltaTime, pPoseLocal, WorldFromModel, pStress);
			// A body moving faster than its maximum linear velocity makes the ragdoll warp to the pose.
			hkaRagdollInstance* pInstance = pRagdollInstance;
			bool bTooFast = false;
			for (int32_t iBone = 0; iBone < pInstance->m_skeleton->m_bones.m_size && !bTooFast; ++iBone) {
				int32_t iBody = pInstance->m_boneToRigidBodyMap.m_data[iBone];
				if (iBody >= 0) {
					hkpRigidBody* pBody = pInstance->m_rigidBodies.m_data[iBody];
					if (pBody) {
						__m128 kVelocitySq = _mm_mul_ps(pBody->m_motion.m_linearVelocity.m_quad,
							pBody->m_motion.m_linearVelocity.m_quad);
						// EXE-INLINED 0x577BF0
						float fMaxVelocity = pBody->m_motion.m_motionState.m_maxLinearVelocity;
						__m128 kSpeedSq = _mm_add_ss(_mm_shuffle_ps(kVelocitySq, kVelocitySq, 0xAA),
							_mm_add_ss(_mm_shuffle_ps(kVelocitySq, kVelocitySq, 0x55), kVelocitySq));
						float fSpeedSq = _mm_cvtss_f32(kSpeedSq);
						float fMaxVelocitySq = fMaxVelocity * fMaxVelocity;
						bTooFast = fMaxVelocitySq < fSpeedSq;
					}
				}
			}
			if (bTooFast) {
				pInstance->setPoseModelSpace(pPoseModel, WorldFromModel);
				bReinitRBController = true;
			}
			// EXE-INLINED none: hkArray<hkaKeyFrameHierarchyUtility::Output>::~hkArray
			hkMemoryAllocator& kFreeHeap = *hkMemoryRouter::getInstance().m_heap;
			if ((iStressCapacityAndFlags &
					static_cast<int32_t>(hkArrayBase<hkaKeyFrameHierarchyUtility::Output>::DONT_DEALLOCATE_FLAG)) ==
				0) {
				kFreeHeap._bufFree<hkaKeyFrameHierarchyUtility::Output>(pStress,
					iStressCapacityAndFlags & hkArrayBase<hkaKeyFrameHierarchyUtility::Output>::CAPACITY_MASK);
			}
		}
	} else if (eRBUpdateMethod == DRIVE_CONSTRAINTS) {
		bhkRagdollPoweredConstraintController::startMotors(pRagdollInstance);
		bhkRagdollPoweredConstraintController::driveToPose(pRagdollInstance, pPoseLocal);
		eRBUpdateMethod = NONE;
	}
	if (bQueuedWarp) {
		eRBUpdateMethod = ePrevMethod;
		bQueuedWarp = false;
	}
	// EXE-INLINED 0xC7B130
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pModelBuffer;
	// EXE-INLINED 0xC7B130
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pLocalBuffer;
}

// The exe's cleanup-only C++ EH frame is left out. The pose buffer is the inlined hkLocalBuffer<hkQsTransform>: the
// thread's stack allocator hands out the bone count's poses rounded up to 128 bytes and takes them back at the end.
void bhkRagdollController::DoPoseMatching(const hkQsTransform& arWorldFromModel, hkQsTransform* apRagdollDrivePoseLS,
	bool& arbUseDrivePose) {
	const NiPoint3& rRootLoc = pRagdollRoot->m_kWorld.m_Translate;
	float fDeltaX = PoseMatchRootLoc.x - rRootLoc.x;
	float fDeltaY = PoseMatchRootLoc.y - rRootLoc.y;
	float fDeltaZ = PoseMatchRootLoc.z - rRootLoc.z;
	float fSqrLength = fDeltaY * fDeltaY + fDeltaX * fDeltaX + fDeltaZ * fDeltaZ;
	float fDisplacement = std::sqrt((double)fSqrLength);
	if (BSPrecisionTimer::GetTimer() - PoseMatchTimer.iStartTime > PoseMatchTimer.iTargetTime ||
		(bDisablePoseMatchOnDisplace && fPoseMatchDisplacementMax < static_cast<double>(fDisplacement)) ||
		iEditorForcePoseIdx != -1) {
		if (iEditorForcePoseIdx == -1) {
			PoseMatchRootLoc = pRagdollRoot->m_kWorld.m_Translate;
			const hkaSkeleton* pskel = pRagdollInstance->m_skeleton;
			int32_t iNumBones = pskel->m_bones.m_size;
			hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
			// EXE-INLINED none: hkLocalBuffer<hkQsTransform>::hkLocalBuffer
			char* pBuffer = kRouter.m_stack.m_next.m_storage;
			kRouter.m_stack.m_next.m_storage =
				pBuffer + ((iNumBones * static_cast<int32_t>(sizeof(hkQsTransform)) + 0x7F) & ~0x7F);
			// The stack allocator's bytes are untyped memory; the exe uses them as the poses.
			hkQsTransform* pPoseMS = reinterpret_cast<hkQsTransform*>(pBuffer);
			pRagdollInstance->getPoseModelSpace(pPoseMS, arWorldFromModel);
			int32_t imatchIdx = pMatchUtil->findBestCandidatePoseIndex(pPoseMS, fCurrentMatchError);
			if (!(f0_69999999 > static_cast<double>(fCurrentMatchError)) || imatchIdx == iCurrentMatchPoseIdx ||
				(iCurrentMatchPoseIdx = imatchIdx, imatchIdx == -1)) {
				if (!arbUseDrivePose) {
					eRBUpdateMethod = bhkRagdollController::NONE;
					arbUseDrivePose = false;
					bQueueStopPoweredMotors = true;
				}
			} else {
				spPoseA.m_pObject->CreatePoseForRagdoll(apRagdollDrivePoseLS, pskel, static_cast<uint32_t>(imatchIdx));
				eRBUpdateMethod = bhkRagdollController::DRIVE_CONSTRAINTS;
				arbUseDrivePose = true;
			}
			// EXE-INLINED 0xC7B130
			hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pBuffer;
		} else {
			spPoseA.m_pObject->CreatePoseForRagdoll(apRagdollDrivePoseLS, pRagdollInstance->m_skeleton,
				static_cast<uint32_t>(iEditorForcePoseIdx));
			eRBUpdateMethod = bhkRagdollController::DRIVE_CONSTRAINTS;
			arbUseDrivePose = true;
			iCurrentMatchPoseIdx = iEditorForcePoseIdx;
			iEditorForcePoseIdx = -1;
		}
		float fTime = fPoseMatchTime;
		PoseMatchTimer.iStartTime = BSPrecisionTimer::GetTimer();
		PoseMatchTimer.iTargetTime = static_cast<int64_t>(fTime * BSPrecisionTimer::fFrequencyMS);
	}
}

// Draws one skeleton of the debug node: each bone with a parent gets the next line child of the skeleton's line node,
// from the parent's model space position to the bone's, at the root's inverse scale; the ragdoll's lines are coloured
// blue for keyframed bodies. The exe's cleanup-only C++ EH frame (it frees posMSRag on unwind) is left out. The
// ragdoll's pose array is the inlined hkArray<hkQsTransform>(int) constructor and destructor.
void bhkRagdollController::UpdateDebugLines(bhkRagdollController::DEBUG_SKEL aeSkeleton) {
	float fscaleInv = 1.0f / pRagdollRoot->m_kWorld.m_fScale;
	if (!spSkeletonDebug.m_pObject) {
		return;
	}
	NiAVObject* plinePar = spSkeletonDebug.m_pObject->m_kChildren.m_pBase[aeSkeleton].m_pObject;
	NiNode* plineNode = plinePar->IsNode();
	if ((plinePar->m_uFlags & NiAVObject::APP_CULLED) != 0) {
		return;
	}
	NiAVObject* pRoot = pRagdollRoot;
	pRoot->m_uFlags |= NiAVObject::ALWAYS_DRAW;
	if (!pRoot->m_kWorldBound) {
		pRoot->SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(BSNiBound))));
		BSNiBound* pBound = pRoot->m_kWorldBound;
		pBound->m_kCenter.x = NiPoint3_ZERO.x;
		pBound->m_kCenter.y = NiPoint3_ZERO.y;
		float z = NiPoint3_ZERO.z;
		static_cast<NiBound&>(*pBound).m_fRadius = 0.0f;
		pBound->m_kCenter.z = z;
	}
	static_cast<NiBound&>(*pRoot->m_kWorldBound).m_fRadius = 1.0f;
	NiNode* pDebug = spSkeletonDebug.m_pObject;
	pDebug->m_uFlags |= NiAVObject::ALWAYS_DRAW;
	if (!pDebug->m_kWorldBound) {
		pDebug->SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(BSNiBound))));
		BSNiBound* pBound = pDebug->m_kWorldBound;
		pBound->m_kCenter.x = NiPoint3_ZERO.x;
		pBound->m_kCenter.y = NiPoint3_ZERO.y;
		float z = NiPoint3_ZERO.z;
		static_cast<NiBound&>(*pBound).m_fRadius = 0.0f;
		pBound->m_kCenter.z = z;
	}
	static_cast<NiBound&>(*pDebug->m_kWorldBound).m_fRadius = 1.0f;
	plineNode->m_uFlags |= NiAVObject::ALWAYS_DRAW;
	if (!plineNode->m_kWorldBound) {
		plineNode->SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(BSNiBound))));
		BSNiBound* pBound = plineNode->m_kWorldBound;
		pBound->m_kCenter.x = NiPoint3_ZERO.x;
		pBound->m_kCenter.y = NiPoint3_ZERO.y;
		float z = NiPoint3_ZERO.z;
		static_cast<NiBound&>(*pBound).m_fRadius = 0.0f;
		pBound->m_kCenter.z = z;
	}
	static_cast<NiBound&>(*plineNode->m_kWorldBound).m_fRadius = 1.0f;
	bhkRagdollShareData* pShareData = spRagdollSharedData.m_pObject;
	hkaSkeleton* pskel =
		aeSkeleton == bhkRagdollController::SKEL_RAGDOLL ? pShareData->pLowSkel : pShareData->pHighSkel;
	int32_t inumBones = pskel->m_bones.m_size;
	// EXE-INLINED none: hkArray<hkQsTransform>::hkArray
	FNVD_INLINED_LOCAL(hkArray<hkQsTransform>, posMSRag, std::nullopt, std::nullopt);
	int32_t iNumAlloc = inumBones;
	hkMemoryAllocator& kHeap = *hkMemoryRouter::getInstance().m_heap;
	posMSRag.m_data = iNumAlloc ? kHeap._bufAlloc<hkQsTransform>(iNumAlloc) : nullptr;
	posMSRag.m_size = iNumAlloc;
	posMSRag.m_capacityAndFlags =
		iNumAlloc ? iNumAlloc : static_cast<int32_t>(hkArrayBase<hkQsTransform>::DONT_DEALLOCATE_FLAG);
	pRagdollInstance->getPoseModelSpace(posMSRag.m_data, WorldFromModel);
	hkArray<hkQsTransform>* pposMS =
		aeSkeleton == bhkRagdollController::SKEL_RAGDOLL ? &posMSRag : &pSGPose->accessSyncedPoseModelSpace();
	uint32_t uiLineIdx = 0;
	for (uint32_t ui = 0; ui < static_cast<uint32_t>(inumBones); ++ui) {
		if (pskel->m_parentIndices.m_data[ui] == -1) {
			continue;
		}
		NiAVObject* pChild = plineNode->m_kChildren.m_pBase[uiLineIdx].m_pObject;
		if (!pChild) {
			continue;
		}
		NiLines* pline = static_cast<NiLines*>(pChild);
		pline->m_uFlags |= NiAVObject::ALWAYS_DRAW;
		if (!pline->m_kWorldBound) {
			pline->SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(BSNiBound))));
			BSNiBound* pBound = pline->m_kWorldBound;
			pBound->m_kCenter.x = NiPoint3_ZERO.x;
			pBound->m_kCenter.y = NiPoint3_ZERO.y;
			float z = NiPoint3_ZERO.z;
			static_cast<NiBound&>(*pBound).m_fRadius = 0.0f;
			pBound->m_kCenter.z = z;
		}
		static_cast<NiBound&>(*pline->m_kWorldBound).m_fRadius = 1.0f;
		if (aeSkeleton == bhkRagdollController::SKEL_RAGDOLL) {
			hkaRagdollInstance* pInstance = pRagdollInstance;
			NiColorA color(0.0f, 0.75f, 1.0f, 1.0f);
			int32_t iBody = pInstance->m_boneToRigidBodyMap.m_data[ui];
			hkBool kFixed;
			if (iBody >= 0 && pInstance->m_rigidBodies.m_data[iBody] &&
				pInstance->getRigidBodyOfBone(ui)->isFixedOrKeyframed(&kFixed)->m_bool) {
				color = NiColorA::Blue;
			}
			NiLines* plineColor = static_cast<NiLines*>(plineNode->m_kChildren.m_pBase[uiLineIdx].m_pObject);
			plineColor->m_spModelData.m_pObject->m_pkColor[0] = color;
			plineColor->m_spModelData.m_pObject->m_pkColor[1] = color;
		}
		hkQsTransform* pPoses = pposMS->m_data;
		__m128 kScale = _mm_set_ss(fscaleInv);
		hkVector4 startHK;
		startHK.m_quad = _mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0),
			pPoses[pskel->m_parentIndices.m_data[ui]].m_translation.m_quad);
		hkVector4 endHK;
		endHK.m_quad = _mm_mul_ps(pPoses[ui].m_translation.m_quad, _mm_shuffle_ps(kScale, kScale, 0));
		NiPoint3 start;
		NiPoint3 end;
		HK2NI(start, startHK);
		HK2NI(end, endHK);
		NiPoint3* pverts = pline->m_spModelData.m_pObject->m_pkVertex;
		pverts[0] = start;
		pverts[1] = end;
		++uiLineIdx;
	}
	// EXE-INLINED none: hkArray<hkQsTransform>::~hkArray
	hkMemoryAllocator& kFreeHeap = *hkMemoryRouter::getInstance().m_heap;
	if ((posMSRag.m_capacityAndFlags & static_cast<int32_t>(hkArrayBase<hkQsTransform>::DONT_DEALLOCATE_FLAG)) == 0) {
		kFreeHeap._bufFree<hkQsTransform>(posMSRag.m_data,
			posMSRag.m_capacityAndFlags & hkArrayBase<hkQsTransform>::CAPACITY_MASK);
	}
}

// The foot IK: the root bone is raised by the vertical error (by its gain, tethered to the controller's tether
// length), each leg's solver places its foot (timed into fFootCastTimer), and with bAlignFeetToTerrain the ankle's
// local transform is the halfway blend of the solved one (turned about a limited hinge's axis only, where the foot's
// ragdoll constraint is one) and the kept AnkleLS; otherwise the ankle is put back as it was before the solve. The
// pelvis error then follows the legs' errors, clamped to [-5, 5], and eases out over iNumFramesFootEaseOut frames.
// The exe inlines the SDK's hkaPose accessors (getBoneModelSpace, getBoneLocalSpace, makeAllChildrenLocalSpace,
// setBoneModelSpace with PROPAGATE, setBoneLocalSpace) and hkQsTransform's setMulInverseMul and setInterpolate4; it
// copies transforms member by member, not through hkQsTransform::operator= (0xC6CD30). Its /GS cookie is left out.
void bhkRagdollController::DoFootIK(hkQsTransform& arWorldFromModel) {
	// EXE-INLINED none: hkaPose::getBoneModelSpace
	auto getBoneModelSpace = [](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_MODEL_DIRTY) {
			return arPose.calculateBoneModelSpace(boneIdx);
		}
		return arPose.m_modelPose.m_data[boneIdx];
	};
	// EXE-INLINED none: hkaPose::getBoneLocalSpace
	auto getBoneLocalSpace = [&](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_LOCAL_DIRTY) {
			const int16_t parentIdx = arPose.m_skeleton->m_parentIndices.m_data[boneIdx];
			const hkQsTransform* pModelPose = arPose.m_modelPose.m_data;
			hkQsTransform& rLocal = arPose.m_localPose.m_data[boneIdx];
			if (parentIdx == -1) {
				rLocal.m_translation = pModelPose[boneIdx].m_translation;
				rLocal.m_rotation = pModelPose[boneIdx].m_rotation;
				rLocal.m_scale = pModelPose[boneIdx].m_scale;
			} else {
				const hkQsTransform& rParentMS = getBoneModelSpace(arPose, parentIdx);
				rLocal.setMulInverseMul(rParentMS, pModelPose[boneIdx]);
			}
			arPose.m_boneFlags.m_data[boneIdx] &= ~static_cast<uint32_t>(hkaPose::F_BONE_LOCAL_DIRTY);
		}
		return arPose.m_localPose.m_data[boneIdx];
	};
	// EXE-INLINED none: hkaPose::makeAllChildrenLocalSpace: the bone count is read once.
	auto makeAllChildrenLocalSpace = [&](hkaPose& arPose, int32_t boneIdx) {
		const int32_t numBones = arPose.m_skeleton->m_bones.m_size;
		arPose.m_boneFlags.m_data[boneIdx] |= hkaPose::F_BONE_INTERNAL_FLAG1;
		for (int32_t b = boneIdx + 1; b < numBones; ++b) {
			const int16_t parentId = arPose.m_skeleton->m_parentIndices.m_data[b];
			if (parentId != -1 && (arPose.m_boneFlags.m_data[parentId] & hkaPose::F_BONE_INTERNAL_FLAG1)) {
				getBoneLocalSpace(arPose, b);
				arPose.m_boneFlags.m_data[b] |= hkaPose::F_BONE_INTERNAL_FLAG1;
				arPose.m_modelInSync.m_bool = 0;
			}
		}
		for (int32_t b = boneIdx + 1; b < numBones; ++b) {
			uint32_t& ruiFlags = arPose.m_boneFlags.m_data[b];
			if (ruiFlags & hkaPose::F_BONE_INTERNAL_FLAG1) {
				ruiFlags =
					(ruiFlags & ~static_cast<uint32_t>(hkaPose::F_BONE_INTERNAL_FLAG1)) | hkaPose::F_BONE_MODEL_DIRTY;
			}
		}
	};

	FNVD_CONSTRUCTED_LOCAL(hkaFootPlacementIkSolver::Input, kInput, nullptr);
	kInput.m_worldFromModel.m_translation = arWorldFromModel.m_translation;
	kInput.m_worldFromModel.m_rotation = arWorldFromModel.m_rotation;
	kInput.m_worldFromModel.m_scale = arWorldFromModel.m_scale;
	const bool bResetWeight = bResetSolverWeight;
	kInput.m_onOffGain = bResetWeight ? 1.0f : gs_fOnOffGain_FootIK.uValue.f;
	kInput.m_groundAscendingGain = gs_fGroundAscendingGain_FootIK.uValue.f;
	kInput.m_groundDescendingGain = gs_fGroundDescendingGain_FootIK.uValue.f;
	kInput.m_footRaisedGain = gs_fFootRaisedGain_FootIK.uValue.f;
	kInput.m_footPlantedGain = gs_fFootPlantedGain_FootIK.uValue.f;
	kInput.m_footPlacementOn = !bResetWeight && !bEaseOutFootIK;
	kInput.m_raycastInterface = this;

	// The root bone, raised by the vertical error.
	const hkQsTransform& rRootMS = getBoneModelSpace(*pSGPose, 0);
	hkQsTransform kRoot;
	kRoot.m_translation = rRootMS.m_translation;
	kRoot.m_rotation = rRootMS.m_rotation;
	kRoot.m_scale = rRootMS.m_scale;
	const float fRaisedZ = kRoot.m_translation.m_quad[2] + fVertFootIkError;
	float fTargetZ = fVertErrorGain.uValue.f * fVertFootIkError + kRoot.m_translation.m_quad[2];
	const float fDiff = fTargetZ - fRaisedZ;
	const float fTether = fControllerTetherLen.uValue.f;
	if (fTether < fDiff) {
		fTargetZ = fTether + fRaisedZ;
	}
	if (-fTether > fDiff) {
		fTargetZ = fRaisedZ - fTether;
	}
	const float fDeltaZ = fTargetZ - kRoot.m_translation.m_quad[2];
	kRoot.m_translation.m_quad[2] = fTargetZ;
	hkVector4 kOffset;
	kOffset.set(0.0f, 0.0f, fDeltaZ, 0.0f);
	kInput.m_worldFromModel.m_translation.setAdd4(kOffset, kInput.m_worldFromModel.m_translation);
	// EXE-INLINED none: hkaPose::setBoneModelSpace (PROPAGATE)
	hkaPose* pPose = pSGPose;
	makeAllChildrenLocalSpace(*pPose, 0);
	pPose->m_modelPose.m_data[0].m_translation = kRoot.m_translation;
	pPose->m_modelPose.m_data[0].m_rotation = kRoot.m_rotation;
	pPose->m_modelPose.m_data[0].m_scale = kRoot.m_scale;
	pPose->m_boneFlags.m_data[0] = hkaPose::F_BONE_LOCAL_DIRTY;
	pPose->m_localInSync.m_bool = 0;

	for (int32_t i = 0; i < LegInfoA.m_size; ++i) {
		hkaFootPlacementIkSolver* pSolver = LegInfoA.m_data[i].pSolver;
		const hkVector4 kFootEndLS = pSolver->m_setup.m_footEndLS;
		pSolver->m_setup.m_raycastDistanceDown = fDownCastDistance;
		if (!bFootEndCast) {
			LegInfoA.m_data[i].pSolver->m_setup.m_footEndLS.setZero4();
		}
		const int32_t iAnkle = LegInfoA.m_data[i].pSolver->m_setup.m_ankleIndex;
		// The ankle's local transform before the solve.
		const hkQsTransform& rLocalBefore = getBoneLocalSpace(*pSGPose, iAnkle);
		hkQsTransform kAnkleLocal;
		kAnkleLocal.m_translation = rLocalBefore.m_translation;
		kAnkleLocal.m_rotation = rLocalBefore.m_rotation;
		kAnkleLocal.m_scale = rLocalBefore.m_scale;
		const hkQsTransform& rAnkleMS = getBoneModelSpace(*pSGPose, LegInfoA.m_data[i].pSolver->m_setup.m_ankleIndex);
		kInput.m_originalAnkleTransformMS.m_translation = rAnkleMS.m_translation;
		kInput.m_originalAnkleTransformMS.m_rotation = rAnkleMS.m_rotation;
		kInput.m_originalAnkleTransformMS.m_scale = rAnkleMS.m_scale;
		usCurrentFootCastIdx = static_cast<uint16_t>(i);
		usCurrentCastIdx = 0;
		const float fStartTime = NiGetCurrentTimeInSec();
		hkaFootPlacementIkSolver::Output kOutput;
		LegInfoA.m_data[i].pSolver->doFootPlacement(kInput, kOutput, *pSGPose);
		const float fEndTime = NiGetCurrentTimeInSec();
		LegInfoA.m_data[i].fResultError = kOutput.m_verticalError;
		fFootCastTimer = (fEndTime - fStartTime) * 1000.0 + fFootCastTimer;
		// The ankle's local transform after the solve.
		const hkQsTransform& rLocalAfter = getBoneLocalSpace(*pSGPose, iAnkle);
		hkQsTransform kNewLocal;
		kNewLocal.m_translation = rLocalAfter.m_translation;
		kNewLocal.m_rotation = rLocalAfter.m_rotation;
		kNewLocal.m_scale = rLocalAfter.m_scale;
		if (bAlignFeetToTerrain) {
			if (bUseCurrAnkleTransf) {
				hkQsTransform& rAnkleLS = LegInfoA.m_data[i].AnkleLS;
				rAnkleLS.m_translation = kNewLocal.m_translation;
				rAnkleLS.m_rotation = kNewLocal.m_rotation;
				rAnkleLS.m_scale = kNewLocal.m_scale;
			}
			const int16_t iFootBone = LegInfoA.m_data[i].RagdollFootIdx;
			if (iFootBone != -1) {
				// EXE-INLINED none: hkaRagdollInstance::getConstraintOfBone
				hkaRagdollInstance* pInstance = pRagdollInstance;
				const int32_t iConstraint = pInstance->m_boneToRigidBodyMap.m_data[iFootBone] - 1;
				hkpConstraintInstance* pConstraint =
					iConstraint >= 0 ? pInstance->m_constraints.m_data[iConstraint] : nullptr;
				hkpConstraintData* pData = pConstraint->getDataRw();
				if (pData->getType() == hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE) {
					pData = static_cast<hkpMalleableConstraintData*>(pData)->getWrappedConstraintData();
				}
				// A limited hinge foot turns about the hinge's axis only.
				if (pData->getType() == hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE) {
					hkpLimitedHingeConstraintData* pHinge = static_cast<hkpLimitedHingeConstraintData*>(pData);
					hkVector4 kAxis = pHinge->m_atoms.m_transforms.m_transformA.m_rotation
										  .getColumn(pHinge->m_atoms.m_angLimit.m_limitAxis);
					hkQuaternion kRest;
					float fAngle;
					kNewLocal.m_rotation.decomposeRestAxis(kAxis, kRest, fAngle);
					kNewLocal.m_rotation.setAxisAngle(kAxis, fAngle);
				}
			}
			hkSimdReal kHalf;
			kHalf.~hkSimdReal();
			::new (&kHalf) hkSimdReal(0.5f);
			kAnkleLocal.setInterpolate4(kNewLocal, LegInfoA.m_data[i].AnkleLS, kHalf);
			hkQsTransform& rAnkleLS = LegInfoA.m_data[i].AnkleLS;
			rAnkleLS.m_translation = kAnkleLocal.m_translation;
			rAnkleLS.m_rotation = kAnkleLocal.m_rotation;
			rAnkleLS.m_scale = kAnkleLocal.m_scale;
		}
		// EXE-INLINED none: hkaPose::setBoneLocalSpace
		pPose = pSGPose;
		makeAllChildrenLocalSpace(*pPose, iAnkle);
		pPose->m_localPose.m_data[iAnkle].m_translation = kAnkleLocal.m_translation;
		pPose->m_localPose.m_data[iAnkle].m_rotation = kAnkleLocal.m_rotation;
		pPose->m_localPose.m_data[iAnkle].m_scale = kAnkleLocal.m_scale;
		pPose->m_boneFlags.m_data[iAnkle] = hkaPose::F_BONE_MODEL_DIRTY;
		pPose->m_modelInSync.m_bool = 0;
		LegInfoA.m_data[i].pSolver->m_setup.m_footEndLS = kFootEndLS;
	}

	if (bUseCurrAnkleTransf) {
		bUseCurrAnkleTransf = false;
	}
	const float fBias = bDownCastFoot ? 1.0f : gs_fPelvisUpDownBias_FootIK.uValue.f;
	// EXE-INLINED 0xC9E0F0
	const float fError0 = LegInfoA.m_data[0].fResultError;
	const float fError1 = LegInfoA.m_data[1].fResultError;
	const float fMinError = (fError0 < fError1) ? fError0 : fError1;
	// EXE-INLINED 0xC98210
	const float fError0b = LegInfoA.m_data[0].fResultError;
	const float fError1b = LegInfoA.m_data[1].fResultError;
	const float fMaxError = (fError0b > fError1b) ? fError0b : fError1b;
	const bool bEaseOut = bEaseOutFootIK;
	if (!bEaseOut) {
		const float fBlend = fBias * fMinError + (1.0f - fBias) * fMaxError;
		const float fStep = fBlend * fPelvisOffsetDamping.uValue.f;
		fVertFootIkError = fStep + fVertFootIkError;
	}
	const float fError = fVertFootIkError;
	// EXE-INLINED 0xC98210
	const float fAboveMin = (fError > -5.0f) ? fError : -5.0f;
	// EXE-INLINED 0xC9E0F0
	const float fClamped = (fAboveMin < 5.0f) ? fAboveMin : 5.0f;
	fVertFootIkError = fClamped;
	if (bEaseOut) {
		fVertFootIkError = fClamped * 0.800000011920929;
		++iCurrFootEase;
		bResetSolverWeight = false;
		if (iNumFramesFootEaseOut.uValue.uint == static_cast<uint32_t>(iCurrFootEase)) {
			iCurrFootEase = 0;
			bEaseOutFootIK = false;
			bActiveFootIK = false;
			bUseCurrAnkleTransf = true;
		}
	} else {
		bResetSolverWeight = false;
	}
}

// Drives the two grab IK chains towards their targets. A hand that is not easing out takes its target's world
// position, converted to Havok units with a zero w, as the new reach and the +Y axis as the new reach normal, blended
// into the previous reach by the hand's gain (the exe blends with SSE, (1 - gain) * old + gain * new). The reach
// weight then eases towards 1 (towards 0 while easing out) by the drive gain, and the hand is disabled once it drops
// below 0.01. Otherwise the previous reach is taken into model space and a two-joint IK solve poses the shoulder,
// elbow and hand bones with the reach weight as the angle difference and the first joint's gain. The exe keeps the
// reach weight and the unit scale on the x87 stack across the loop; they are float locals here. Its 16-byte frame
// alignment comes from the hkVector4 locals, and its stack cookie check is left out.
void bhkRagdollController::DoGrabIK(hkQsTransform& arWorldFromModel) {
	for (uint32_t uiHand = 0; uiHand < 2; ++uiHand) {
		GrabIKInfo& kInfo = GrabIKState[uiHand];
		if (!kInfo.bEnabled) {
			continue;
		}
		float fReach;
		if (!kInfo.bEaseOutGrabIK) {
			const NiPoint3& kTargetWS = kInfo.pTarget->m_kWorld.m_Translate;
			alignas(16) hkVector4 kReachWS;
			kReachWS.m_quad[0] = kTargetWS.x * bhkConvert::fBS2HkScaleSC;
			kReachWS.m_quad[1] = kTargetWS.y * bhkConvert::fBS2HkScaleSC;
			kReachWS.m_quad[2] = kTargetWS.z * bhkConvert::fBS2HkScaleSC;
			kReachWS.m_quad[3] = 0.0f;
			alignas(16) hkVector4 kReachNormWS;
			kReachNormWS.m_quad[0] = 0.0f;
			kReachNormWS.m_quad[1] = 1.0f;
			kReachNormWS.m_quad[2] = 0.0f;
			kReachNormWS.m_quad[3] = 0.0f;
			if (0.0f < kInfo.fReachWeight) {
				const __m128 kGain = _mm_set1_ps(kInfo.fIntrpGain);
				const __m128 kOldGain = _mm_sub_ps(_mm_set1_ps(1.0f), kGain);
				_mm_store_ps(&reinterpret_cast<float*>(&kInfo.PrevReachLocWS.m_quad)[0],
					_mm_add_ps(_mm_mul_ps(kOldGain,
								   _mm_load_ps(&reinterpret_cast<float*>(&kInfo.PrevReachLocWS.m_quad)[0])),
						_mm_mul_ps(kGain, _mm_load_ps(&reinterpret_cast<float*>(&kReachWS.m_quad)[0]))));
				_mm_store_ps(&reinterpret_cast<float*>(&kInfo.PrevReachNormWS.m_quad)[0],
					_mm_add_ps(_mm_mul_ps(kOldGain,
								   _mm_load_ps(&reinterpret_cast<float*>(&kInfo.PrevReachNormWS.m_quad)[0])),
						_mm_mul_ps(kGain, _mm_load_ps(&reinterpret_cast<float*>(&kReachNormWS.m_quad)[0]))));
			} else {
				kInfo.PrevReachLocWS = kReachWS;
				kInfo.PrevReachNormWS = kReachNormWS;
			}
			fReach = 1.0f;
		} else {
			fReach = 0.0f;
		}
		fReach = (fReach - kInfo.fReachWeight) * bhkRagdollController::fDriveGain.uValue.f + kInfo.fReachWeight;
		kInfo.fReachWeight = fReach;
		if (fReach < 0.01f && kInfo.bEaseOutGrabIK) {
			kInfo.bEaseOutGrabIK = false;
			kInfo.fReachWeight = 0.0f;
			kInfo.bEnabled = false;
			continue;
		}
		alignas(16) hkVector4 kEndTargetMS;
		kEndTargetMS.setTransformedInversePos(arWorldFromModel, kInfo.PrevReachLocWS);
		hkaTwoJointsIkSolver::Setup kSetup;
		kSetup.~Setup();
		::new (&kSetup) hkaTwoJointsIkSolver::Setup;
		kSetup.m_maxHingeAngleDifference = kInfo.fReachWeight;
		kSetup.m_firstJointIdx = GrabSetup[uiHand].usShoulderIdx;
		kSetup.m_secondJointIdx = GrabSetup[uiHand].usElbowIdx;
		kSetup.m_firstJointIkGain = kInfo.fReachWeight;
		kSetup.m_endBoneIdx = GrabSetup[uiHand].usHandIdx;
		kSetup.m_hingeAxisLS.m_quad[0] = 0.0f;
		kSetup.m_hingeAxisLS.m_quad[1] = 0.0f;
		kSetup.m_hingeAxisLS.m_quad[2] = 1.0f;
		kSetup.m_hingeAxisLS.m_quad[3] = 0.0f;
		kSetup.m_endTargetMS = kEndTargetMS;
		hkBool kSolved;
		hkaTwoJointsIkSolver::solve(&kSolved, kSetup, *pSGPose);
	}
	if (!GrabIKState[0].bEnabled && !GrabIKState[1].bEnabled) {
		bActiveGrabIK = false;
	}
}

// Turns the head toward the look-at target and writes the turned head back into the scene graph pose, its
// descendants following it; with the head limit exceeded, also turns the eyes and records their heading and pitch
// from the head pose's bone 1, the head bone staying active while either is past its dead zone.
// The exe inlines the SDK's hkaPose accessors (getBoneModelSpace, getBoneLocalSpace, makeAllChildrenLocalSpace,
// setBoneModelSpace with PROPAGATE) and hkQsTransform's setMulInverseMul and setMul; it copies transforms member by
// member, not through hkQsTransform::operator= (0xC6CD30). Its /GS cookie is left out.
void bhkRagdollController::DoLookAtIK(hkQsTransform& arWorldFromModel) {
	// EXE-INLINED none: hkaPose::getBoneModelSpace
	auto getBoneModelSpace = [](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_MODEL_DIRTY) {
			return arPose.calculateBoneModelSpace(boneIdx);
		}
		return arPose.m_modelPose.m_data[boneIdx];
	};
	// EXE-INLINED none: hkaPose::getBoneLocalSpace
	auto getBoneLocalSpace = [&](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_LOCAL_DIRTY) {
			const int16_t parentIdx = arPose.m_skeleton->m_parentIndices.m_data[boneIdx];
			const hkQsTransform* pModelPose = arPose.m_modelPose.m_data;
			hkQsTransform& rLocal = arPose.m_localPose.m_data[boneIdx];
			if (parentIdx == -1) {
				rLocal.m_translation = pModelPose[boneIdx].m_translation;
				rLocal.m_rotation = pModelPose[boneIdx].m_rotation;
				rLocal.m_scale = pModelPose[boneIdx].m_scale;
			} else {
				const hkQsTransform& rParentMS = getBoneModelSpace(arPose, parentIdx);
				rLocal.setMulInverseMul(rParentMS, pModelPose[boneIdx]);
			}
			arPose.m_boneFlags.m_data[boneIdx] &= ~static_cast<uint32_t>(hkaPose::F_BONE_LOCAL_DIRTY);
		}
		return arPose.m_localPose.m_data[boneIdx];
	};
	// EXE-INLINED none: hkaPose::makeAllChildrenLocalSpace: the bone count is read once.
	auto makeAllChildrenLocalSpace = [&](hkaPose& arPose, int32_t boneIdx) {
		const int32_t numBones = arPose.m_skeleton->m_bones.m_size;
		arPose.m_boneFlags.m_data[boneIdx] |= hkaPose::F_BONE_INTERNAL_FLAG1;
		for (int32_t b = boneIdx + 1; b < numBones; ++b) {
			const int16_t parentId = arPose.m_skeleton->m_parentIndices.m_data[b];
			if (parentId != -1 && (arPose.m_boneFlags.m_data[parentId] & hkaPose::F_BONE_INTERNAL_FLAG1)) {
				getBoneLocalSpace(arPose, b);
				arPose.m_boneFlags.m_data[b] |= hkaPose::F_BONE_INTERNAL_FLAG1;
				arPose.m_modelInSync.m_bool = 0;
			}
		}
		for (int32_t b = boneIdx + 1; b < numBones; ++b) {
			uint32_t& ruiFlags = arPose.m_boneFlags.m_data[b];
			if (ruiFlags & hkaPose::F_BONE_INTERNAL_FLAG1) {
				ruiFlags =
					(ruiFlags & ~static_cast<uint32_t>(hkaPose::F_BONE_INTERNAL_FLAG1)) | hkaPose::F_BONE_MODEL_DIRTY;
			}
		}
	};

	AdjustCurrentTarget();
	if (BoneParamA[1].bActive || !bTargetOutsideHeadLimit) {
		eCurrentBone = bhkRagdollController::HEAD_BONE;
		BoneTrack(bhkRagdollController::HEAD_BONE, arWorldFromModel);
	}
	hkQsTransform& rCurrHeadMS = pSGPose->accessBoneModelSpace(BoneParamA[1].ssBoneIdx, hkaPose::PROPAGATE);
	rCurrHeadMS.m_rotation.setMul(rCurrHeadMS.m_rotation, CurrentHeadRotOffset);
	// EXE-INLINED none: hkaPose::setBoneModelSpace (PROPAGATE)
	const int32_t iHeadIdx = BoneParamA[1].ssBoneIdx;
	hkaPose* pPose = pSGPose;
	makeAllChildrenLocalSpace(*pPose, iHeadIdx);
	pPose->m_modelPose.m_data[iHeadIdx].m_translation = rCurrHeadMS.m_translation;
	pPose->m_modelPose.m_data[iHeadIdx].m_rotation = rCurrHeadMS.m_rotation;
	pPose->m_modelPose.m_data[iHeadIdx].m_scale = rCurrHeadMS.m_scale;
	pPose->m_boneFlags.m_data[iHeadIdx] = hkaPose::F_BONE_LOCAL_DIRTY;
	pPose->m_localInSync.m_bool = 0;
	if (!bTargetOutsideHeadLimit) {
		return;
	}

	// The eyes' world transform: the head's model space one taken to world space.
	const hkQsTransform& rHeadMS = getBoneModelSpace(*pSGPose, BoneParamA[1].ssBoneIdx);
	hkQsTransform kEyeWS;
	kEyeWS.setMul(arWorldFromModel, rHeadMS);
	BoneParamA[1].bActive = BoneTrack(bhkRagdollController::EYE_BONE, kEyeWS) || eCurrentBone;
	hkVector4 kX;
	kX.set(1.0f, 0.0f, 0.0f, 0.0f);
	hkVector4 kY;
	kY.set(0.0f, 1.0f, 0.0f, 0.0f);
	hkVector4 kDown;
	kDown.set(0.0f, 0.0f, -1.0f, 0.0f);
	// The head pose's bone 1 forward (+Y) direction, in its model space.
	const hkQsTransform& rEyeMS = getBoneModelSpace(*pHeadPose, 1);
	hkVector4 kFwd;
	kFwd.setRotatedDir(rEyeMS.m_rotation, kY);
	// EXE-INLINED 0x62B0A0
	__m128 kDownProd = _mm_mul_ps(kFwd.m_quad, kDown.m_quad);
	const float fDownDot = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kDownProd, kDownProd, _MM_SHUFFLE(2, 2, 2, 2)),
		_mm_add_ss(_mm_shuffle_ps(kDownProd, kDownProd, _MM_SHUFFLE(1, 1, 1, 1)), kDownProd)));
	const float fDownAngle = std::acos(fDownDot);
	const float fHeading = 1.5707963705062866 - fDownAngle;
	// EXE-INLINED 0x62B0A0
	__m128 kXProd = _mm_mul_ps(kFwd.m_quad, kX.m_quad);
	fEyeHeading = fHeading;
	const float fXDot = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kXProd, kXProd, _MM_SHUFFLE(2, 2, 2, 2)),
		_mm_add_ss(_mm_shuffle_ps(kXProd, kXProd, _MM_SHUFFLE(1, 1, 1, 1)), kXProd)));
	const float fXAngle = std::acos(fXDot);
	const float fPitch = 1.5707963705062866 - fXAngle;
	fEyePitch = fPitch;
	const float fAbsPitch = std::fabs(fPitch);
	if (BoneParamA[0].fDeadPitch < fAbsPitch) {
		BoneParamA[1].bActive = true;
	}
	const float fAbsHeading = std::fabs(fHeading);
	if (BoneParamA[0].fDeadHeading < fAbsHeading) {
		BoneParamA[1].bActive = true;
	}
	pHeadPose->setToReferencePose();
}

// Blends the ragdoll's pose into the scene graph's: for each of the feedback's dynamic bones, the rotation of the
// scene graph's pose (mapped onto the ragdoll) is slerped toward the ragdoll's own local rotation by
// fCurrFeedbackInterp, and the result is mapped back onto the scene graph pose; once the feedback time is up the
// feedback is turned off. The exe's cleanup-only C++ EH frame is left out. The four pose buffers are the inlined
// hkLocalBuffer<hkQsTransform>: the thread's stack allocator hands out the bone count's poses rounded up to 128 bytes
// and takes them back, the later buffer first.
void bhkRagdollController::DoRagdollFeedback() {
	if (BSPrecisionTimer::GetTimer() - FeedbackTimer.iStartTime > FeedbackTimer.iTargetTime) {
		SetRagdollFeedbackActive(0);
		return;
	}
	const hkaSkeleton* pskel = pRagdollInstance->m_skeleton;
	int32_t inumRagdollBones = pskel->m_bones.m_size;
	int32_t iBufferSize = (inumRagdollBones * static_cast<int32_t>(sizeof(hkQsTransform)) + 0x7F) & ~0x7F;
	// EXE-INLINED none: hkLocalBuffer<hkQsTransform>::hkLocalBuffer
	hkMemoryRouter& kRagdollLocalRouter = hkMemoryRouter::getInstance();
	char* pRagdollLocalBuffer = kRagdollLocalRouter.m_stack.m_next.m_storage;
	kRagdollLocalRouter.m_stack.m_next.m_storage = pRagdollLocalBuffer + iBufferSize;
	// EXE-INLINED none: hkLocalBuffer<hkQsTransform>::hkLocalBuffer
	hkMemoryRouter& kRagdollModelRouter = hkMemoryRouter::getInstance();
	char* pRagdollModelBuffer = kRagdollModelRouter.m_stack.m_next.m_storage;
	kRagdollModelRouter.m_stack.m_next.m_storage = pRagdollModelBuffer + iBufferSize;
	// The stack allocator's bytes are untyped memory; the exe uses them as the poses.
	hkQsTransform* pRagdollPoseLS = reinterpret_cast<hkQsTransform*>(pRagdollLocalBuffer);
	hkQsTransform* pRagdollPoseMS = reinterpret_cast<hkQsTransform*>(pRagdollModelBuffer);
	pRagdollInstance->getPoseModelSpace(pRagdollPoseMS, WorldFromModel);
	hkaSkeletonUtils::transformModelPoseToLocalPose(inumRagdollBones, pskel->m_parentIndices.m_data, pRagdollPoseMS,
		pRagdollPoseLS);
	// EXE-INLINED 0xC7B130
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pRagdollModelBuffer;
	// EXE-INLINED none: hkLocalBuffer<hkQsTransform>::hkLocalBuffer
	hkMemoryRouter& kLocalRouter = hkMemoryRouter::getInstance();
	char* pLocalBuffer = kLocalRouter.m_stack.m_next.m_storage;
	kLocalRouter.m_stack.m_next.m_storage = pLocalBuffer + iBufferSize;
	// EXE-INLINED none: hkLocalBuffer<hkQsTransform>::hkLocalBuffer
	hkMemoryRouter& kModelRouter = hkMemoryRouter::getInstance();
	char* pModelBuffer = kModelRouter.m_stack.m_next.m_storage;
	kModelRouter.m_stack.m_next.m_storage = pModelBuffer + iBufferSize;
	// The stack allocator's bytes are untyped memory; the exe uses them as the poses.
	hkQsTransform* pPoseLS = reinterpret_cast<hkQsTransform*>(pLocalBuffer);
	hkQsTransform* pPoseMS = reinterpret_cast<hkQsTransform*>(pModelBuffer);
	hkaSkeletonUtils::transformLocalPoseToModelPose(inumRagdollBones, pskel->m_parentIndices.m_data,
		pskel->m_referencePose.m_data, pPoseMS);
	hkQsTransform* pReferencePose = pskel->m_referencePose.m_data;
	const hkArray<hkQsTransform, hkContainerHeapAllocator>& rSGPoseMS = pSGPose->getSyncedPoseModelSpace();
	hkaSkeletonMapper::mapPose(spRagdollSharedData.m_pObject->pSGToRagdoll, rSGPoseMS.m_data, pReferencePose, pPoseMS,
		hkaSkeletonMapper::CURRENT_POSE);
	hkaSkeletonUtils::transformModelPoseToLocalPose(inumRagdollBones, pskel->m_parentIndices.m_data, pPoseMS, pPoseLS);
	int32_t iNumBodies = pRagdollInstance->m_rigidBodies.m_size;
	for (int32_t i = 0; i < iNumBodies; ++i) {
		// The bone's index in the feedback's dynamic bones, or -1.
		const BSSimpleArray<int16_t, 1024>& rDynamicBoneIdxA =
			spRagdollSharedData.m_pObject->pFeedbackData->DynamicBoneIdxA;
		uint32_t uiNumDynamic = rDynamicBoneIdxA.iSize;
		int16_t* pDynamicIdx = rDynamicBoneIdxA.pBuffer;
		int32_t iDynamicIdx = -1;
		for (uint32_t j = 0; j < uiNumDynamic; ++j) {
			if (pDynamicIdx && pDynamicIdx[j] == static_cast<int16_t>(i)) {
				iDynamicIdx = static_cast<int32_t>(j);
				break;
			}
		}
		if (iDynamicIdx != -1) {
			hkQuaternion kRotation;
			kRotation.setSlerp(pRagdollPoseLS[i].m_rotation, pPoseLS[i].m_rotation, fCurrFeedbackInterp);
			pPoseLS[i].m_rotation = kRotation;
		}
	}
	hkaSkeletonUtils::transformLocalPoseToModelPose(inumRagdollBones, pskel->m_parentIndices.m_data, pPoseLS, pPoseMS);
	hkQsTransform* pSGPoseMS = pSGPose->accessSyncedPoseModelSpace().begin();
	const hkArray<hkQsTransform, hkContainerHeapAllocator>& rSGPoseLS = pSGPose->getSyncedPoseLocalSpace();
	hkaSkeletonMapper::mapPose(spRagdollSharedData.m_pObject->pRagdollToSG, pPoseMS, rSGPoseLS.m_data, pSGPoseMS,
		hkaSkeletonMapper::CURRENT_POSE);
	pSGPose->accessUnsyncedPoseModelSpace();
	// EXE-INLINED 0xC7B130
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pModelBuffer;
	// EXE-INLINED 0xC7B130
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pLocalBuffer;
	// EXE-INLINED 0xC7B130
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pRagdollLocalBuffer;
}

// Shows or hides the three debug line sets (a hidden object has flag bit 0 set) and attaches the debug node to the
// ragdoll's root while any is shown, else detaches it.
void bhkRagdollController::ShowDebugLines(bool a1, bool a2, bool a3) {
	NiAVObject* pLines0 = spSkeletonDebug.m_pObject->m_kChildren.m_pBase[0].m_pObject;
	if (a1) {
		pLines0->m_uFlags &= ~1u;
	} else {
		pLines0->m_uFlags |= 1u;
	}
	NiAVObject* pLines1 = spSkeletonDebug.m_pObject->m_kChildren.m_pBase[1].m_pObject;
	if (a2) {
		pLines1->m_uFlags &= ~1u;
	} else {
		pLines1->m_uFlags |= 1u;
	}
	NiAVObject* pLines2 = spSkeletonDebug.m_pObject->m_kChildren.m_pBase[2].m_pObject;
	if (a3) {
		pLines2->m_uFlags &= ~1u;
	} else {
		pLines2->m_uFlags |= 1u;
	}
	NiNode* pRoot = pRagdollRoot->IsNode();
	if (pRoot) {
		if (a1 || a2 || a3) {
			NiNode* pDebug = spSkeletonDebug.m_pObject;
			if (!pDebug->m_pkParent) {
				pRoot->AttachChild(pDebug, true);
			}
		} else {
			pRoot->DetachChild(spSkeletonDebug.m_pObject);
		}
	}
}

// The source's GetCinfo is the shape's CreateCinfo, called through the vtable; the exe keeps its created flag in the
// dead apBone argument slot.
hkCapsuleShapeCinfo* bhkRagdollController::GetCapsuleBoneShapeInfo(NiAVObject* apBone) {
	bhkCapsuleShape* pboneShape = nullptr;
	if (apBone) {
		bhkCollisionObject* pboneCO = bhkUtilFunctions::FindNextCollisionObject(apBone);
		if (pboneCO) {
			// EXE-INLINED 0x6FA820
			bhkRigidBody* pboneRB = pboneCO->spWorldObject;
			if (pboneRB) {
				// EXE-INLINED 0x4AE6A0
				// EXE-INLINED 0x4AE750
				hkpWorldObject* pWorldObject = static_cast<hkpWorldObject*>(pboneRB->phkObject);
				if (pWorldObject) {
					// EXE-INLINED 0x4AE700
					pboneRB->MarkRead();
					hkpLinkedCollidable* pCollidable = &pWorldObject->m_collidable;
					pboneRB->UnMarkRead();
					// EXE-INLINED 0x4AE6C0
					if (pCollidable) {
						const hkpShape* pShape = pCollidable->m_shape;
						// EXE-INLINED 0x4A3A40
						if (pShape) {
							pboneShape = static_cast<bhkCapsuleShape*>(pShape->m_userData);
						}
					}
				}
			}
		}
	}
	hkCapsuleShapeCinfo* pboneCapsuleInfo = nullptr;
	if (pboneShape) {
		bool bCreated;
		pboneCapsuleInfo = static_cast<hkCapsuleShapeCinfo*>(&pboneShape->CreateCinfo(bCreated));
	}
	return pboneCapsuleInfo;
}

// Looks the grab IK arm bones up in the high skeleton (false when one is missing), then clears both hands' state.
bool bhkRagdollController::InitGrabIK() {
	bool bret = bInitRagdollAnim;
	if (bret) {
		const hkaSkeleton* pskel = spRagdollSharedData.m_pObject->pHighSkel;
		if (pskel) {
			GrabSetup[0].usShoulderIdx = hkaSkeletonUtils::findBoneWithName(*pskel, "Bip01 L UpperArm", 0);
			GrabSetup[0].usElbowIdx = hkaSkeletonUtils::findBoneWithName(*pskel, "Bip01 L ForeArm", 0);
			GrabSetup[0].usHandIdx = hkaSkeletonUtils::findBoneWithName(*pskel, "Bip01 L Hand", 0);
			GrabSetup[1].usShoulderIdx = hkaSkeletonUtils::findBoneWithName(*pskel, "Bip01 R UpperArm", 0);
			GrabSetup[1].usElbowIdx = hkaSkeletonUtils::findBoneWithName(*pskel, "Bip01 R ForeArm", 0);
			GrabSetup[1].usHandIdx = hkaSkeletonUtils::findBoneWithName(*pskel, "Bip01 R Hand", 0);
			if (GrabSetup[0].usShoulderIdx == -1 || (GrabSetup[0].usElbowIdx == -1 || GrabSetup[0].usHandIdx == -1)) {
				bret = 0;
			}
			if (GrabSetup[1].usShoulderIdx == -1 || (GrabSetup[1].usElbowIdx == -1 || GrabSetup[1].usHandIdx == -1)) {
				bret = 0;
			}
		}
	}
	for (int32_t i = 0; i < 2; ++i) {
		GrabIKState[i].fIntrpGain = 0.0f;
		GrabIKState[i].pTarget = nullptr;
		GrabIKState[i].fReachWeight = 0.0f;
		GrabIKState[i].bEnabled = false;
	}
	return bret;
}

// The exe compares the angle with the limit at x87 width (the float setting times the double degree-to-radian
// constant, never stored) and stores the limit as a float only where it clamps: the product is written twice, so
// that each use keeps the exe's width.
float bhkRagdollController::LimitBoneRot(bhkRagdollController::LookIKBone aeBone, const hkQuaternion& arIKBoneRotLS) {
	float fdiffAngle = 0.0f;
	LookIKBoneParams& rBoneParams = BoneParamA[aeBone];
	if (!arIKBoneRotLS.m_vec.equals3(rBoneParams.PrevRotLS.m_vec, 0.001f)) {
		hkQuaternion adjRot;
		adjRot.setMulInverse(arIKBoneRotLS, rBoneParams.PrevRotLS);
		fdiffAngle = adjRot.getAngle();
		float fAngleMax;
		if (bEaseOutLookIK) {
			fAngleMax = gs_fAngleMaxEase_LookIK.uValue.f;
		} else {
			fAngleMax = gs_fAngleMax_LookIK.uValue.f;
		}
		if (fdiffAngle > fAngleMax * 0.01745329238474369) {
			fdiffAngle = fAngleMax * 0.01745329238474369;
			hkVector4 axis;
			adjRot.getAxis(axis);
			axis.normalize3();
			adjRot.setAxisAngle(axis, fdiffAngle);
		}
		hkQuaternion newRotLS;
		newRotLS.setMul(adjRot, rBoneParams.PrevRotLS);
		newRotLS.normalize();
		hkQsTransform& curHeadTransfLS = rBoneParams.pPose->accessBoneLocalSpace(rBoneParams.ssBoneIdx);
		curHeadTransfLS.m_rotation = newRotLS;
		rBoneParams.PrevRotLS = newRotLS;
	}
	return fdiffAngle;
}

bool bhkRagdollController::IsPoweredRagdoll() {
	bhkRagdollController* pThis = this;
	hkaRagdollInstance* pRagdollInstance = pThis->pRagdollInstance;
	bhkRagdollController* v8 = pThis;
	char v2 = 1;
	if (pRagdollInstance) {
		int32_t v3 = 0;
		if (pRagdollInstance->m_constraints.m_size > 0) {
			while (1) {
				int32_t v5;
				bool v6;
				if (!v2) {
					return 0;
				}
				hkpRagdollConstraintData* m_data =
					static_cast<hkpRagdollConstraintData*>(pRagdollInstance->m_constraints.m_data[v3]->getDataRw());
				if (m_data->getType() == 13 || m_data->getType() == 12) {
				LABEL_13:;
					v2 = 0;
					goto LABEL_14;
				}
				v5 = m_data->getType() - 2;
				if (v5) {
					if (v5 != 5) {
						goto LABEL_14;
					}
					if (!m_data->getTwistMotor() || !m_data->getConeMotor()) {
						goto LABEL_13;
					}
					v6 = m_data->getPlaneMotor() == 0;
				} else {
					v6 = std::bit_cast<int32_t>(m_data->m_atoms.m_ragdollMotors.m_target_bRca.m_col0.m_quad[0]) == 0;
				}
				if (v6) {
					goto LABEL_13;
				}
			LABEL_14:;
				if (++v3 >= pRagdollInstance->m_constraints.m_size) {
					if (!v2) {
						return 0;
					}
					pThis = v8;
					return pThis->pRagdollInstance != 0;
				}
			}
		}
	}
	return pThis->pRagdollInstance != 0;
}

void bhkRagdollController::DampenDriveGains() {
	RagdollFeedbackData* pFeedbackData = spRagdollSharedData.m_pObject->pFeedbackData;
	if (pFeedbackData) {
		hkaKeyFrameHierarchyUtility::ControlData* m_data = pRagdollRBController->m_controlDataPalette.begin();
		m_data->m_velocityGain = pFeedbackData->SystemData.fVelGain * 0.30000001192092896;
		m_data->m_positionGain =
			spRagdollSharedData.m_pObject->pFeedbackData->SystemData.fPosGain * 0.10000000149011612;
		m_data->m_snapGain = spRagdollSharedData.m_pObject->pFeedbackData->SystemData.fSnapGain * 0.0;
		bhkRagdollShareData* m_pObject = spRagdollSharedData.m_pObject;
		uiFeedbackImpulseWeak =
			static_cast<int64_t>(static_cast<double>(m_pObject->pFeedbackData->SystemData.uiImpulseMult1) * 0.25);
		uiFeedbackImpulseStrong =
			static_cast<int64_t>(0.25 * static_cast<double>(m_pObject->pFeedbackData->SystemData.uiImpulseMult2));
	}
}

void bhkRagdollController::ResetDriveGains() {
	RagdollFeedbackData* pFeedbackData = spRagdollSharedData.m_pObject->pFeedbackData;
	if (pFeedbackData) {
		hkaKeyFrameHierarchyUtility::ControlData* m_data = pRagdollRBController->m_controlDataPalette.begin();
		m_data->m_velocityGain = pFeedbackData->SystemData.fVelGain;
		m_data->m_positionGain = spRagdollSharedData.m_pObject->pFeedbackData->SystemData.fPosGain;
		m_data->m_snapGain = spRagdollSharedData.m_pObject->pFeedbackData->SystemData.fSnapGain;
		bhkRagdollShareData* m_pObject = spRagdollSharedData.m_pObject;
		fCurrFeedbackInterp = m_pObject->pFeedbackData->SystemData.fKeyDynIntrp;
		uiFeedbackImpulseWeak =
			static_cast<int64_t>(static_cast<double>(m_pObject->pFeedbackData->SystemData.uiImpulseMult1) * 0.25);
		uiFeedbackImpulseStrong =
			static_cast<int64_t>(0.25 * static_cast<double>(m_pObject->pFeedbackData->SystemData.uiImpulseMult2));
	}
}

bool bhkRagdollController::Leg3DSelectiveUpdate() {
	int32_t v1 = 0;
	bool bret = 0;
	for (int32_t i = 0; i < LegInfoA.m_size; ++i) {
		if (bret) {
			break;
		}
		hkaFootPlacementIkSolver* pSolver = LegInfoA.m_data[v1].pSolver;
		uint32_t m_ankleIndex = pSolver->m_setup.m_ankleIndex;
		uint32_t iSize = SGBonesA.iSize;
		NiAVObject* v6 = iSize <= m_ankleIndex ? 0 : SGBonesA.pBuffer[m_ankleIndex];
		uint32_t m_kneeIndex = pSolver->m_setup.m_kneeIndex;
		NiAVObject* v8 = iSize <= m_kneeIndex ? 0 : SGBonesA.pBuffer[m_kneeIndex];
		uint32_t m_hipIndex = pSolver->m_setup.m_hipIndex;
		NiAVObject* v10 = iSize <= m_hipIndex ? 0 : SGBonesA.pBuffer[m_hipIndex];
		bret = (v6 && (v6->m_uFlags & NiAVObject::SELECTIVE_UPDATE) == 0) ||
			((v8 && (v8->m_uFlags & NiAVObject::SELECTIVE_UPDATE) == 0) ||
				(v10 && (v10->m_uFlags & NiAVObject::SELECTIVE_UPDATE) == 0));
		++v1;
	}
	return bret;
}

// Keeps the look-at target in reach of the head. The target taken into model space and normalized tells whether it
// lies behind the model (a negative y). While easing out, the target becomes the head's position plus three times the
// head's forward direction (in world space); while behind, the head's position offset 5 units sideways (towards the
// target's side) in model space, then taken into world space. Finally a target farther than fMaxTrackLen from the
// head is pulled back onto that distance. The head's position is its model space translation (plus the eye's local
// translation when tracking outside the head limit) taken into world space. The exe inlines the SDK's normalize3,
// dot3, length3, setRotatedDir and hkaPose::getBoneModelSpace; its /GS cookie is left out.
void bhkRagdollController::AdjustCurrentTarget() {
	// EXE-INLINED none: hkaPose::getBoneModelSpace
	auto getBoneModelSpace = [](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_MODEL_DIRTY) {
			return arPose.calculateBoneModelSpace(boneIdx);
		}
		return arPose.m_modelPose.m_data[boneIdx];
	};

	hkVector4 kTargetMS;
	kTargetMS.setTransformedInversePos(WorldFromModel, CurrentTargetWS);
	kTargetMS.normalize3();
	// The exe builds (0, 1, 0, 0) from its 0.0f and 1.0f constants with unpcklps.
	hkVector4 kUp;
	kUp.m_quad = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
	const float fFacing = _mm_cvtss_f32(kTargetMS.dot3(kUp).getQuad());
	const bool bBehind = fFacing < 0.0f;
	bTargetBehind = bBehind;
	if (bEaseOutLookIK) {
		// The exe loads the bone's forward direction before it looks the head's model space up.
		const int32_t iHeadIdx = BoneParamA[1].ssBoneIdx;
		hkVector4 kFwdLS;
		kFwdLS.m_quad = BoneParamA[1].FwdLS.m_quad;
		const hkQsTransform& rHeadMS = getBoneModelSpace(*pSGPose, iHeadIdx);
		hkVector4 kFwdMS;
		kFwdMS.setRotatedDir(rHeadMS.m_rotation, kFwdLS);
		hkVector4 kFwdWS;
		kFwdWS.setRotatedDir(WorldFromModel.m_rotation, kFwdMS);
		hkVector4 kOffset;
		kOffset.m_quad = _mm_mul_ps(_mm_set1_ps(3.0f), kFwdWS.m_quad);
		// The head's world position: the eye's local translation when tracking outside the head limit, through the
		// head's model space transform and the world transform.
		hkVector4 kPos;
		kPos.m_quad = _mm_setzero_ps();
		if (bTargetOutsideHeadLimit) {
			kPos.m_quad = _mm_add_ps(kPos.m_quad, BoneParamA[0].pPose->accessBoneLocalSpace(1).m_translation.m_quad);
		}
		const hkQsTransform& rHeadPosMS = getBoneModelSpace(*pSGPose, BoneParamA[1].ssBoneIdx);
		kPos.setTransformedPos(rHeadPosMS, kPos);
		kPos.setTransformedPos(WorldFromModel, kPos);
		CurrentTargetWS.m_quad = _mm_add_ps(kPos.m_quad, kOffset.m_quad);
	} else if (bBehind) {
		hkVector4 kLocal;
		kLocal.setTransformedInversePos(WorldFromModel, CurrentTargetWS);
		float fOffset = 5.0f;
		if (kLocal.m_quad[0] < 0.0f) {
			fOffset = -5.0f;
		}
		hkVector4 kPos;
		kPos.m_quad = _mm_setzero_ps();
		if (bTargetOutsideHeadLimit) {
			kPos.m_quad = _mm_add_ps(BoneParamA[0].pPose->accessBoneLocalSpace(1).m_translation.m_quad, kPos.m_quad);
		}
		const hkQsTransform& rHeadMS = getBoneModelSpace(*pSGPose, BoneParamA[1].ssBoneIdx);
		kPos.setTransformedPos(rHeadMS, kPos);
		// x87: fld, fadd, fstp dword.
		const float fX = kPos.m_quad[0] + fOffset;
		kPos.m_quad[0] = fX;
		kPos.setTransformedPos(WorldFromModel, kPos);
		CurrentTargetWS.m_quad = kPos.m_quad;
	}
	hkVector4 kPos;
	kPos.m_quad = _mm_setzero_ps();
	if (bTargetOutsideHeadLimit) {
		kPos.m_quad = _mm_add_ps(kPos.m_quad, BoneParamA[0].pPose->accessBoneLocalSpace(1).m_translation.m_quad);
	}
	const hkQsTransform& rHeadMS = getBoneModelSpace(*pSGPose, BoneParamA[1].ssBoneIdx);
	kPos.setTransformedPos(rHeadMS, kPos);
	kPos.setTransformedPos(WorldFromModel, kPos);
	hkVector4 kDiff;
	kDiff.setSub4(CurrentTargetWS, kPos);
	const float fLength = _mm_cvtss_f32(kDiff.length3().getQuad());
	if (fMaxTrackLen < fLength) {
		kDiff.normalize3();
		CurrentTargetWS.m_quad = _mm_add_ps(_mm_mul_ps(kDiff.m_quad, _mm_set1_ps(fMaxTrackLen)), kPos.m_quad);
	}
}

// The look-at IK of bone aeBone: the target is taken into model space, and for the head (when it tracks outside its
// limit) pushed out along the head-to-target direction by the eye's offset projected on the head's up axis, with the
// eye's offset as the eye position. The solver turns the bone towards it within the cone about the parent's forward
// axis. The local transform from before the solve is put back when the solved model-space rotation is not valid;
// once the remaining angle is under fEaseAngleShutOff while easing out, the bone's previous rotation is reset and
// the IK hands over to the eye or stops. Returns true when the solver did not reach the target.
// The exe inlines the SDK's hkaPose accessors (getBoneModelSpace, getBoneLocalSpace, makeAllChildrenLocalSpace,
// setBoneLocalSpace) and hkQsTransform's setMulInverseMul (with hkQuaternion::setMul out of line); it copies
// transforms member by member, not through hkQsTransform::operator= (0xC6CD30). Its /GS cookie is left out.
bool bhkRagdollController::BoneTrack(bhkRagdollController::LookIKBone aeBone, hkQsTransform& arWorldFromModel) {
	// EXE-INLINED none: hkaPose::getBoneModelSpace
	auto getBoneModelSpace = [](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_MODEL_DIRTY) {
			return arPose.calculateBoneModelSpace(boneIdx);
		}
		return arPose.m_modelPose.m_data[boneIdx];
	};
	// EXE-INLINED none: hkaPose::getBoneLocalSpace
	auto getBoneLocalSpace = [&](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_LOCAL_DIRTY) {
			const int16_t parentIdx = arPose.m_skeleton->m_parentIndices.m_data[boneIdx];
			const hkQsTransform* pModelPose = arPose.m_modelPose.m_data;
			hkQsTransform& rLocal = arPose.m_localPose.m_data[boneIdx];
			if (parentIdx == -1) {
				rLocal.m_translation = pModelPose[boneIdx].m_translation;
				rLocal.m_rotation = pModelPose[boneIdx].m_rotation;
				rLocal.m_scale = pModelPose[boneIdx].m_scale;
			} else {
				const hkQsTransform& rParentMS = getBoneModelSpace(arPose, parentIdx);
				rLocal.setMulInverseMul(rParentMS, pModelPose[boneIdx]);
			}
			arPose.m_boneFlags.m_data[boneIdx] &= ~static_cast<uint32_t>(hkaPose::F_BONE_LOCAL_DIRTY);
		}
		return arPose.m_localPose.m_data[boneIdx];
	};
	// EXE-INLINED none: hkaPose::makeAllChildrenLocalSpace: the bone count is read once.
	auto makeAllChildrenLocalSpace = [&](hkaPose& arPose, int32_t boneIdx) {
		const int32_t numBones = arPose.m_skeleton->m_bones.m_size;
		arPose.m_boneFlags.m_data[boneIdx] |= hkaPose::F_BONE_INTERNAL_FLAG1;
		for (int32_t b = boneIdx + 1; b < numBones; ++b) {
			const int16_t parentId = arPose.m_skeleton->m_parentIndices.m_data[b];
			if (parentId != -1 && (arPose.m_boneFlags.m_data[parentId] & hkaPose::F_BONE_INTERNAL_FLAG1)) {
				getBoneLocalSpace(arPose, b);
				arPose.m_boneFlags.m_data[b] |= hkaPose::F_BONE_INTERNAL_FLAG1;
				arPose.m_modelInSync.m_bool = 0;
			}
		}
		for (int32_t b = boneIdx + 1; b < numBones; ++b) {
			uint32_t& ruiFlags = arPose.m_boneFlags.m_data[b];
			if (ruiFlags & hkaPose::F_BONE_INTERNAL_FLAG1) {
				ruiFlags =
					(ruiFlags & ~static_cast<uint32_t>(hkaPose::F_BONE_INTERNAL_FLAG1)) | hkaPose::F_BONE_MODEL_DIRTY;
			}
		}
	};

	LookIKBoneParams& rParams = BoneParamA[aeBone];
	hkaLookAtIkSolver::Setup setup;
	setup.m_limitAngle = rParams.fLookAtMaxAngle;
	setup.m_eyePositionLS.setZero4();
	setup.m_fwdLS = rParams.FwdLS;
	// The limit cone's axis: the parent's forward axis in model space.
	const hkQsTransform& rParentMS = getBoneModelSpace(*rParams.pPose, rParams.ssParentIdx);
	setup.m_limitAxisMS.setRotatedDir(rParentMS.m_rotation, rParams.FwdParentMS);
	setup.m_limitAxisMS.normalize3();
	hkVector4 targetMS;
	targetMS.setTransformedInversePos(arWorldFromModel, CurrentTargetWS);

	if (bTargetOutsideHeadLimit && aeBone == HEAD_BONE) {
		// The head's up axis in its local space: the head's forward axis crossed with the eye's offset (bone 1 of the
		// eye's pose; the exe pushes aeBone, 1 here), crossed with the forward axis again.
		hkVector4 kSide;
		kSide.setCross(BoneParamA[HEAD_BONE].FwdLS, BoneParamA[EYE_BONE].pPose->accessBoneLocalSpace(1).m_translation);
		kSide.normalize3();
		hkVector4 kUp;
		kUp.setCross(kSide, BoneParamA[HEAD_BONE].FwdLS);
		kUp.normalize3();
		const hkVector4& rEyeOffsetLS = BoneParamA[EYE_BONE].pPose->accessBoneLocalSpace(1).m_translation;
		const float fEyeUp = _mm_cvtss_f32(_mm_div_ss(kUp.dot3(rEyeOffsetLS).getQuad(), kUp.length3().getQuad()));
		// The exe copies the quotient with fld/fstp and scales by the copy.
		const float fEyeUpCopy = fEyeUp;
		const hkQsTransform& rHeadMS = getBoneModelSpace(*BoneParamA[HEAD_BONE].pPose, BoneParamA[HEAD_BONE].ssBoneIdx);
		hkVector4 kToTarget;
		kToTarget.setSub4(targetMS, rHeadMS.m_translation);
		const float fDistance = static_cast<float>(kToTarget.length3());
		kToTarget.normalize3();
		if (fEyeUp > fDistance) {
			hkSimdReal kEyeUp;
			kEyeUp.~hkSimdReal();
			::new (&kEyeUp) hkSimdReal(fEyeUpCopy);
			targetMS.setAddMul4(targetMS, kToTarget, kEyeUp);
		}
		setup.m_eyePositionLS = BoneParamA[EYE_BONE].pPose->accessBoneLocalSpace(1).m_translation;
	}

	// The bone's local transform before the solve; the first one becomes the previous rotation.
	hkQsTransform kPrevLS;
	{
		const hkQsTransform& rLS = getBoneLocalSpace(*rParams.pPose, rParams.ssBoneIdx);
		kPrevLS.m_translation = rLS.m_translation;
		kPrevLS.m_rotation = rLS.m_rotation;
		kPrevLS.m_scale = rLS.m_scale;
	}
	if (rParams.PrevRotLS.m_vec.equals3(hkVector4::getZero(), 0.001f)) {
		rParams.PrevRotLS = kPrevLS.m_rotation;
	}
	hkQsTransform& rBoneMS = rParams.pPose->accessBoneModelSpace(rParams.ssBoneIdx, hkaPose::PROPAGATE);
	hkBool kSolved;
	const bool bSolved = hkaLookAtIkSolver::solve(&kSolved, setup, targetMS, 1.0f, rBoneMS, nullptr)->m_bool != 0;

	hkQsTransform kBoneLS;
	{
		const hkQsTransform& rLS = getBoneLocalSpace(*rParams.pPose, rParams.ssBoneIdx);
		kBoneLS.m_translation = rLS.m_translation;
		kBoneLS.m_rotation = rLS.m_rotation;
		kBoneLS.m_scale = rLS.m_scale;
	}
	const float fAngle = rParams.bLimitBoneRot ? LimitBoneRot(aeBone, kBoneLS.m_rotation) : 0.0f;

	hkBool kRotationOk;
	if (!getBoneModelSpace(*rParams.pPose, rParams.ssBoneIdx).m_rotation.isOk(&kRotationOk)->m_bool) {
		// EXE-INLINED none: hkaPose::setBoneLocalSpace
		hkaPose* pPose = rParams.pPose;
		const int32_t iBone = rParams.ssBoneIdx;
		makeAllChildrenLocalSpace(*pPose, iBone);
		hkQsTransform& rLocal = pPose->m_localPose.m_data[iBone];
		rLocal.m_translation = kPrevLS.m_translation;
		rLocal.m_rotation = kPrevLS.m_rotation;
		rLocal.m_scale = kPrevLS.m_scale;
		pPose->m_boneFlags.m_data[iBone] = hkaPose::F_BONE_MODEL_DIRTY;
		pPose->m_modelInSync.m_bool = 0;
	}
	rParams.bLimitBoneRot = true;

	if (bEaseOutLookIK && fEaseAngleShutOff.uValue.f * 0.01745329238474369 > fAngle) {
		rParams.PrevRotLS.setIdentity();
		if (aeBone != EYE_BONE && bTargetOutsideHeadLimit) {
			BoneParamA[eCurrentBone].bActive = false;
			eCurrentBone = static_cast<LookIKBone>(eCurrentBone - 1);
		} else {
			bEaseOutLookIK = false;
			bActiveLookIK = false;
		}
	}
	return !bSolved;
}

// Finds aeBone's bone named apNodeName and its parent in the high skeleton; when both exist, sets the bone's forward
// directions (the y axis turned by the bone's and the parent's model space rotation in apPose, the bone's one
// normalized), takes the shared look data's directions for the head and points the bone at the scene graph pose.
// The world transform with the inverse scale applied is converted to Havok but not used. The exe inlines the SDK's
// hkaPose::getBoneModelSpace and hkVector4::normalize3. Its /GS cookie is left out.
bool bhkRagdollController::InitBoneParams(hkaPose* apPose, const char* apNodeName,
	bhkRagdollController::LookIKBone aeBone) {
	// EXE-INLINED none: hkaPose::getBoneModelSpace
	auto getBoneModelSpace = [](hkaPose& arPose, int32_t boneIdx) -> const hkQsTransform& {
		if (arPose.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_MODEL_DIRTY) {
			return arPose.calculateBoneModelSpace(boneIdx);
		}
		return arPose.m_modelPose.m_data[boneIdx];
	};

	bool bResult = true;
	const int16_t ssBone =
		hkaSkeletonUtils::findBoneWithName(*spRagdollSharedData.m_pObject->pHighSkel, apNodeName, nullptr);
	LookIKBoneParams& rParams = BoneParamA[aeBone];
	rParams.ssBoneIdx = ssBone;
	if (ssBone == -1) {
		bResult = false;
	} else {
		const int16_t ssParent = spRagdollSharedData.m_pObject->pHighSkel->m_parentIndices.m_data[ssBone];
		rParams.ssParentIdx = ssParent;
		if (ssParent == -1) {
			bResult = false;
		}
	}
	bEaseOutLookIK = false;
	if (bResult) {
		NiTransform kWorld = pRagdollRoot->m_kWorld;
		kWorld.m_Rotate = ScaleInv * kWorld.m_Rotate;
		kWorld.m_fScale = 1.0f;
		hkQsTransform worldFromModel;
		NI2HK(worldFromModel, kWorld);
		// The exe builds (0, 1, 0, 0) from its 0.0f and 1.0f constants with unpcklps.
		hkVector4 kForward;
		kForward.m_quad = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
		const hkQsTransform& rBoneMS = getBoneModelSpace(*apPose, rParams.ssBoneIdx);
		const hkQsTransform& rParentMS = getBoneModelSpace(*apPose, rParams.ssParentIdx);
		// The exe's cross product is dir x imag (0xC794CA-0xC794F3, 0xC79542-0xC7955D): the SDK's
		// setRotatedInverseDir form, not setRotatedDir's imag x dir.
		rParams.FwdParentMS.setRotatedInverseDir(rParentMS.m_rotation, kForward);
		rParams.FwdLS.setRotatedInverseDir(rBoneMS.m_rotation, kForward);
		rParams.FwdLS.normalize3();
		if (spRagdollSharedData.m_pObject->pSharedLookData && aeBone == HEAD_BONE) {
			BoneParamA[HEAD_BONE].FwdParentMS = spRagdollSharedData.m_pObject->pSharedLookData->FwdParentMS;
			BoneParamA[HEAD_BONE].FwdLS = spRagdollSharedData.m_pObject->pSharedLookData->FwdLS;
		}
		BoneParamA[aeBone].pPose = pSGPose;
	}
	return bResult;
}

// Warns (through hkError) about every simple mapping of both skeleton mappers whose second bone locks its translation
// while the mapping still translates (by more than 0.01) and the ragdoll has no bhkRigidBodyT on the mapping's
// ragdoll bone; returns whether it warned, or true when the shared data or a mapper is missing. The first loop casts
// the body's user data through the out-of-line NiObject::DynamicCast, the second inlines it.
// The exe's cleanup-only C++ EH frame and its /GS cookie are left out.
bool bhkRagdollController::CheckMappersForWarnings() {
	alignas(16) FNVD_INLINED_LOCAL(hkErrStream, kStream2, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(hkErrStream, kStream1, std::nullopt, std::nullopt);
	bhkRagdollShareData* pShareData = spRagdollSharedData.m_pObject;
	bool bWarned = false;
	if (!pShareData || !pShareData->pSGToRagdoll || !pShareData->pRagdollToSG) {
		return true;
	}
	for (int32_t i = 0; i < spRagdollSharedData.m_pObject->pSGToRagdoll->m_mapping.m_simpleMappings.m_size; ++i) {
		const hkaSkeletonMapper* pMapper = spRagdollSharedData.m_pObject->pSGToRagdoll;
		const hkaSkeletonMapperData::SimpleMapping& rMapping = pMapper->m_mapping.m_simpleMappings.m_data[i];
		const hkaBone& rBoneA = pMapper->m_mapping.m_skeletonA->m_bones.m_data[rMapping.m_boneA];
		const hkaBone& rBoneB = pMapper->m_mapping.m_skeletonB->m_bones.m_data[rMapping.m_boneB];
		hkaRagdollInstance* pInstance = pRagdollInstance;
		const int32_t iBody = pInstance->m_boneToRigidBodyMap.m_data[rMapping.m_boneB];
		NiObject* pRigidBody = nullptr;
		if (iBody >= 0) {
			hkpRigidBody* pBody = pInstance->m_rigidBodies.m_data[iBody];
			if (pBody) {
				NiObject* pUser = pBody->getUserData();
				if (pUser) {
					pRigidBody = pUser->DynamicCastMember(&bhkRigidBodyT::ms_RTTI);
				}
			}
		}
		if (rBoneB.m_lockTranslation.m_bool && !pRigidBody) {
			const float fError = static_cast<float>(rMapping.m_aFromBTransform.m_translation.length3());
			if (fError > 0.01f) {
				char acBuffer[512];
				::new (&kStream1) hkErrStream(acBuffer, 512);
				kStream1 << "Bones A:" << rBoneA.m_name << " and B:" << rBoneB.m_name
						 << " are not fully aligned (error :" << fError << ").";
				hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xBADBADBA), acBuffer,
					"D:\\_Fallout3\\Platforms\\Common\\Code\\BSHavok\\bhkRagdollController.cpp", 3103);
				FNVD_DESTRUCT(hkOstream)(&kStream1);
				bWarned = true;
			}
		}
	}
	for (int32_t i = 0; i < spRagdollSharedData.m_pObject->pRagdollToSG->m_mapping.m_simpleMappings.m_size; ++i) {
		const hkaSkeletonMapper* pMapper = spRagdollSharedData.m_pObject->pRagdollToSG;
		const hkaSkeletonMapperData::SimpleMapping& rMapping = pMapper->m_mapping.m_simpleMappings.m_data[i];
		const hkaBone& rBoneA = pMapper->m_mapping.m_skeletonA->m_bones.m_data[rMapping.m_boneA];
		const hkaBone& rBoneB = pMapper->m_mapping.m_skeletonB->m_bones.m_data[rMapping.m_boneB];
		hkaRagdollInstance* pInstance = pRagdollInstance;
		const int32_t iBody = pInstance->m_boneToRigidBodyMap.m_data[rMapping.m_boneA];
		NiObject* pRigidBody = nullptr;
		if (iBody >= 0) {
			hkpRigidBody* pBody = pInstance->m_rigidBodies.m_data[iBody];
			if (pBody) {
				NiObject* pUser = pBody->getUserData();
				if (pUser) {
					pRigidBody = pUser->DynamicCast(&bhkRigidBodyT::ms_RTTI);
				}
			}
		}
		if (rBoneB.m_lockTranslation.m_bool && !pRigidBody) {
			const float fError = static_cast<float>(rMapping.m_aFromBTransform.m_translation.length3());
			if (fError > 0.01f) {
				char acBuffer[512];
				::new (&kStream2) hkErrStream(acBuffer, 512);
				kStream2 << "Bones A:" << rBoneA.m_name << " and B:" << rBoneB.m_name
						 << " are not fully aligned (error :" << fError << ").";
				hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xBADBADBA), acBuffer,
					"D:\\_Fallout3\\Platforms\\Common\\Code\\BSHavok\\bhkRagdollController.cpp", 3129);
				FNVD_DESTRUCT(hkOstream)(&kStream2);
				bWarned = true;
			}
		}
	}
	return bWarned;
}

void bhkRagdollController::EnableRagdollAnim() {
	if (!bEnableRagdollAnim) {
		hkaRagdollRigidBodyController* pRagdollRBController = this->pRagdollRBController;
		if (pRagdollRBController) {
			pRagdollRBController->reinitialize();
		}
	}
	bool bActiveFootIK = this->bActiveFootIK;
	bool bInitRagdollAnim = this->bInitRagdollAnim;
	bool v5 = !bEnableFootIK;
	bEnableRagdollAnim = bInitRagdollAnim;
	bEaseOutRagdollAnim = 0;
	this->bActiveFootIK = !v5;
	if (bActiveFootIK && v5) {
		bResetSolverWeight = 1;
	}
	v5 = !bActiveRagdollFeedback;
	bActiveGrabIK = bEnableGrabIK;
	if (v5 && bInitRagdollAnim) {
		hkpConstraintOwner* m_owner;
		hkaRagdollInstance* pRagdollInstance = this->pRagdollInstance;
		hkpConstraintInstance** m_data = pRagdollInstance->m_constraints.begin();
		if (*m_data) {
			m_owner = (*m_data)->getOwner();
		} else {
			m_owner = 0;
		}
		hkpRigidBody** v9 = pRagdollInstance->m_rigidBodies.begin();
		if (*v9) {
			ahkpWorld* m_world = static_cast<ahkpWorld*>((*v9)->m_world);
			if (m_world) {
				if (m_owner) {
					bhkWorld* m_userData = static_cast<bhkWorld*>(m_world->m_userData);
					for (int32_t i = 0; i < pRagdollInstance->m_constraints.m_size; ++i) {
						if (pRagdollInstance->m_constraints.m_data[i]->getOwner()) {
							m_userData->RemoveConstraint(pRagdollInstance->m_constraints.m_data[i]);
						}
					}
				}
			}
		}
	}
}

// Builds the shared high skeleton from the scene graph's bones the first time, then the scene-graph pose on it. The
// bone array and the bone map are locals the exe constructs and destroys inline, the map first at the end; both sit
// in 16-aligned slots of the realigned frame. The exe's cleanup-only C++ EH frame is left out.
void bhkRagdollController::CreateSGSkeleton() {
	if (pRagdollRoot) {
		NiAVObject* pObj = bhkUtilFunctions::FindNextCollisionObject(pRagdollRoot)->m_pkSceneObject;
		using BoneArray = BSSimpleArray<bhkRagdollController::bhkSkeletonBone*, 1024>;
		alignas(16) FNVD_CONSTRUCTED_LOCAL(BoneArray, kBones, &BoneArray::Destruct);
		using BoneMap = NiTMap<NiAVObject*, int16_t>;
		alignas(16) FNVD_CONSTRUCTED_LOCAL(BoneMap, kBoneMap, &BoneMap::Destruct, 37u);
		CreateSGSkeletonRec(pObj, kBones, kBoneMap);
		if (!spRagdollSharedData.m_pObject->pHighSkel) {
			// Havok's operator new (no null test), then the constructor, inlined.
			hkaSkeleton* pSkeleton =
				static_cast<hkaSkeleton*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkaSkeleton)));
			pSkeleton->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkaSkeleton));
			::new (pSkeleton) hkaSkeleton;
			spRagdollSharedData.m_pObject->pHighSkel = pSkeleton;
			int32_t iBoneCount = static_cast<int32_t>(kBones.iSize);
			{
				// EXE-INLINED none: hkArray<hkaBone>::setSize, with hkArrayUtil's destruct and construct of the bones
				hkArray<hkaBone>& rBones = spRagdollSharedData.m_pObject->pHighSkel->m_bones;
				hkMemoryAllocator& rHeap = *hkMemoryRouter::getInstance().m_heap;
				rBones._reserve(rHeap, iBoneCount);
				int32_t iOldCount = rBones.m_size - iBoneCount;
				if (iOldCount > 0) {
					hkaBone* pOld = rBones.m_data + iBoneCount;
					do {
						pOld->m_name.~hkStringPtr();
						++pOld;
					} while (--iOldCount);
				}
				int32_t iNewCount = iBoneCount - rBones.m_size;
				hkaBone* pNew = rBones.m_data + rBones.m_size;
				for (int32_t i = 0; i < iNewCount; ++i) {
					// Placement new's null test.
					hkaBone* pBone = &pNew[i];
					if (pBone) {
						::new (&pBone->m_name) hkStringPtr;
						pBone->m_lockTranslation.m_bool = 0;
					}
				}
				rBones.m_size = iBoneCount;
			}
			spRagdollSharedData.m_pObject->pHighSkel->m_parentIndices.setSize(iBoneCount);
			spRagdollSharedData.m_pObject->pHighSkel->m_referencePose.setSize(iBoneCount);
			for (uint32_t i = 0; i < kBones.iSize; ++i) {
				hkaBone& rBone = spRagdollSharedData.m_pObject->pHighSkel->m_bones.m_data[i];
				rBone.m_name = hkString::strDup(kBones.pBuffer[i]->name);
				spRagdollSharedData.m_pObject->pHighSkel->m_bones.m_data[i].m_lockTranslation.m_bool = 1;
				hkQsTransform& rReference = spRagdollSharedData.m_pObject->pHighSkel->m_referencePose.m_data[i];
				const hkQsTransform& rSource = kBones.pBuffer[i]->transform;
				rReference.m_translation.m_quad = rSource.m_translation.m_quad;
				rReference.m_rotation.m_vec.m_quad = rSource.m_rotation.m_vec.m_quad;
				rReference.m_scale.m_quad = rSource.m_scale.m_quad;
				spRagdollSharedData.m_pObject->pHighSkel->m_parentIndices.m_data[i] = kBones.pBuffer[i]->parentIndice;
			}
		}
		for (uint32_t i = 0; i < kBones.iSize; ++i) {
			bhkRagdollController::bhkSkeletonBone* pBone = kBones.pBuffer[i];
			if (pBone) {
				// An aligned block: the byte in front of the bone holds the padding up to the block's start.
				uint8_t* pMemory = static_cast<uint8_t*>(static_cast<void*>(pBone));
				MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
			}
		}
		// The exe calls _Deallocate through the array's vtable: a pointer keeps the call indirect.
		BoneArray* pBoneArray = &kBones;
		if (kBones.pBuffer) {
			// EXE-INLINED 0x6A8500
			pBoneArray->_Deallocate(kBones.pBuffer);
			kBones.pBuffer = nullptr;
			kBones.iReservedSize = 0;
			kBones.iSize = 0;
		}
		// EXE-INLINED none: hkaPose::hkaPose(const hkaSkeleton*)
		hkaPose* pPose = static_cast<hkaPose*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkaPose)));
		if (pPose) {
			pPose->m_skeleton = spRagdollSharedData.m_pObject->pHighSkel;
			pPose->m_localPose.m_data = nullptr;
			pPose->m_localPose.m_size = 0;
			pPose->m_localPose.m_capacityAndFlags = hkArrayBase<hkQsTransform>::DONT_DEALLOCATE_FLAG;
			pPose->m_modelPose.m_data = nullptr;
			pPose->m_modelPose.m_size = 0;
			pPose->m_modelPose.m_capacityAndFlags = hkArrayBase<hkQsTransform>::DONT_DEALLOCATE_FLAG;
			pPose->m_boneFlags.m_data = nullptr;
			pPose->m_boneFlags.m_size = 0;
			pPose->m_boneFlags.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
			pPose->m_modelInSync.m_bool = 0;
			pPose->m_localInSync.m_bool = 0;
			pPose->m_floatSlotValues.m_data = nullptr;
			pPose->m_floatSlotValues.m_size = 0;
			pPose->m_floatSlotValues.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
			int32_t iNumBones = pPose->m_skeleton->m_bones.m_size;
			pPose->m_modelPose.setSize(iNumBones);
			pPose->m_localPose.setSize(iNumBones);
			pPose->m_boneFlags.setSize(iNumBones);
			pPose->m_floatSlotValues.reserveExactly((pPose->m_skeleton->m_floatSlots.m_size + 3) & ~3);
			// The SDK's setSize(n, 0.0f), which src's hkArray leaves out: the storage grown, the new slots zeroed
			// (the exe's rep stosd).
			int32_t iNumSlots = pPose->m_skeleton->m_floatSlots.m_size;
			pPose->m_floatSlotValues.reserve(iNumSlots);
			for (int32_t i = pPose->m_floatSlotValues.m_size; i < iNumSlots; ++i) {
				pPose->m_floatSlotValues.m_data[i] = 0.0f;
			}
			pPose->m_floatSlotValues.m_size = iNumSlots;
		}
		pSGPose = pPose;
		pPose->setToReferencePose();
		AnimPoseLS.setSize(spRagdollSharedData.m_pObject->pHighSkel->m_bones.m_size);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void bhkRagdollController::Init(NiAVObject* apObject, const char* apScaledSkeletonName) {
	// EXE-INLINED none: NiFixedString::operator=(const char*)
	const char* pOldName = SkeletonShareName.GetHandle();
	if (pOldName != apScaledSkeletonName) {
		SkeletonShareName.m_kHandle = NiGlobalStringTable::AddString(apScaledSkeletonName);
		NiGlobalStringTable::ReleaseHandle(pOldName);
	}
	// Without a scaled skeleton name, the object's own name keys the shared data.
	if (!apScaledSkeletonName) {
		// EXE-INLINED 0x43BA10 (NiFixedString::operator=)
		const char* pName = apObject->m_kName.GetHandle();
		if (SkeletonShareName.GetHandle() != pName) {
			NiGlobalStringTable::AcquireHandle(pName);
			NiGlobalStringTable::ReleaseHandle(SkeletonShareName.GetHandle());
			SkeletonShareName.m_kHandle = pName;
		}
	}
	SkeletonMap.GetAt(SkeletonShareName.GetHandle(), spRagdollSharedData);
	if (!spRagdollSharedData.m_pObject) {
		bhkRagdollShareData* pNewShareData =
			static_cast<bhkRagdollShareData*>(NiMemObject::operator new(sizeof(bhkRagdollShareData)));
		if (pNewShareData) {
			::new (pNewShareData) bhkRagdollShareData;
		}
		spRagdollSharedData = pNewShareData;
		SkeletonMap.SetAt(SkeletonShareName.GetHandle(), spRagdollSharedData);
	}
	if (bhkUtilFunctions::FindNextCollisionObject(apObject)) {
		bhkCollisionObject* pcol = bhkUtilFunctions::FindNextCollisionObject(apObject);
		pRagdollRoot = pcol->m_pkSceneObject->m_pkParent;
		// EXE-INLINED none: hkLocalArray<hkpRigidBody *>::hkLocalArray(64) (hkAllocateStack); 64 pointers fill whole
		// 128-byte blocks, so the stack block is not rounded up.
		FNVD_INLINED_LOCAL(hkLocalArray<hkpRigidBody*>, rbarray, std::nullopt, std::nullopt);
		rbarray.m_data = nullptr;
		rbarray.m_size = 0;
		rbarray.m_capacityAndFlags = hkArrayBase<hkpRigidBody*>::DONT_DEALLOCATE_FLAG;
		hkStackMemory& rbStack = hkMemoryRouter::getInstance().m_stack;
		char* pRbBlock = rbStack.m_next.m_storage;
		rbStack.m_next.m_storage = pRbBlock + 64 * sizeof(hkpRigidBody*);
		rbarray.m_data = static_cast<hkpRigidBody**>(static_cast<void*>(pRbBlock));
		rbarray.m_capacityAndFlags = 64 | hkArrayBase<hkpRigidBody*>::DONT_DEALLOCATE_FLAG;
		rbarray.m_localMemory = rbarray.m_data;
		// EXE-INLINED none: hkLocalArray<hkpConstraintInstance *>::hkLocalArray(64) (hkAllocateStack)
		FNVD_INLINED_LOCAL(hkLocalArray<hkpConstraintInstance*>, constarray, std::nullopt, std::nullopt);
		constarray.m_data = nullptr;
		constarray.m_size = 0;
		constarray.m_capacityAndFlags = hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG;
		hkStackMemory& constStack = hkMemoryRouter::getInstance().m_stack;
		char* pConstBlock = constStack.m_next.m_storage;
		constStack.m_next.m_storage = pConstBlock + 64 * sizeof(hkpConstraintInstance*);
		constarray.m_data = static_cast<hkpConstraintInstance**>(static_cast<void*>(pConstBlock));
		constarray.m_capacityAndFlags = 64 | hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG;
		constarray.m_localMemory = constarray.m_data;
		FindBones(pcol->m_pkSceneObject, rbarray, constarray);
		hkResult ires = hkaRagdollUtils::reorderForRagdoll(rbarray, constarray);
		if (ires != HK_FAILURE) {
			// The low skeleton is built once per shared data, posed at the scene object's transform composed with
			// the rigid body's local transform.
			bhkRagdollShareData* pShareData = spRagdollSharedData.m_pObject;
			if (!pShareData->pLowSkel) {
				pShareData->pLowSkel = hkaRagdollUtils::constructSkeletonForRagdoll(rbarray, constarray);
				NI2HK(spRagdollSharedData.m_pObject->pLowSkel->m_referencePose.m_data[0],
					pcol->m_pkSceneObject->m_kLocal);
				bhkRigidBodyT* prbT = nullptr;
				bhkRigidBody* pWorldObject = pcol->spWorldObject;
				if (pWorldObject) {
					// The exe calls NiObject::DynamicCast out of line (0x653290).
					prbT = static_cast<bhkRigidBodyT*>(pWorldObject->DynamicCastMember(&bhkRigidBodyT::ms_RTTI));
				}
				NiTransform rbTransf;
				rbTransf.MakeIdentity();
				if (prbT) {
					hkTransform rbTransfHK;
					rbTransfHK.m_translation = prbT->kLocalPos;
					rbTransfHK.m_rotation.set(prbT->kLocalRot);
					HK2NI(rbTransf, rbTransfHK);
				}
				// The exe builds the product in a 16-aligned slot of its realigned frame, then copies it.
				alignas(16) NiTransform kRootProduct = pcol->m_pkSceneObject->m_kLocal * rbTransf;
				NiTransform rootTransf = kRootProduct;
				NI2HK(spRagdollSharedData.m_pObject->pLowSkel->m_referencePose.m_data[0], rootTransf);
			}
			if (spRagdollSharedData.m_pObject->pLowSkel) {
				pRagdollInstance =
					hkaRagdollUtils::createRagdollInstanceFromSkeleton(spRagdollSharedData.m_pObject->pLowSkel, rbarray,
						constarray);
				// The last bone whose name contains "Head" is the head bone.
				hkaSkeleton* skel = spRagdollSharedData.m_pObject->pLowSkel;
				for (int32_t x = 0; x < skel->m_bones.m_size; ++x) {
					if (std::strstr(skel->m_bones.m_data[x].m_name.cString(), "Head")) {
						nHeadBone = x;
					}
				}
				if (pRagdollInstance) {
					void* pMemory =
						hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkaRagdollRigidBodyController));
					hkaRagdollRigidBodyController* pController = nullptr;
					if (pMemory) {
						pController = ::new (pMemory) hkaRagdollRigidBodyController(pRagdollInstance);
					}
					pRagdollRBController = pController;
					hkaKeyFrameHierarchyUtility::ControlData& rcontrolData =
						pController->m_controlDataPalette.m_data[0];
					rcontrolData.m_hierarchyGain = fHierarchyGain.uValue.f;
					rcontrolData.m_velocityDamping = fVelocityDamping.uValue.f;
					rcontrolData.m_accelerationGain = fAccelerationGain.uValue.f;
					rcontrolData.m_velocityGain = fVelocityGain.uValue.f;
					rcontrolData.m_positionGain = fPositionGain.uValue.f;
					rcontrolData.m_positionMaxLinearVelocity = fPositionMaxLinearVelocity.uValue.f;
					rcontrolData.m_positionMaxAngularVelocity = fPositionMaxAngularVelocity.uValue.f;
					rcontrolData.m_snapGain = fSnapGain.uValue.f;
					rcontrolData.m_snapMaxLinearVelocity = fSnapMaxLinearVelocity.uValue.f;
					rcontrolData.m_snapMaxAngularVelocity = fSnapMaxAngularVelocity.uValue.f;
					rcontrolData.m_snapMaxLinearDistance = fSnapMaxLinearDistance.uValue.f;
					rcontrolData.m_snapMaxAngularDistance = fSnapMaxAngularDistance.uValue.f;
					bInitRagdollAnim = true;
					bInitGrabIK = InitGrabIK();
				}
			}
			CreateSGSkeleton();
			// The mappers between the scene graph's (high) skeleton and the ragdoll's (low) one, built once per
			// shared data, with neither root bone's translation locked.
			bhkRagdollShareData* pMapShareData = spRagdollSharedData.m_pObject;
			if (!pMapShareData->pSGToRagdoll && !pMapShareData->pRagdollToSG) {
				pMapShareData->pHighSkel->m_bones.m_data[0].m_lockTranslation.m_bool = false;
				spRagdollSharedData.m_pObject->pLowSkel->m_bones.m_data[0].m_lockTranslation.m_bool = false;
				FNVD_CONSTRUCTED_LOCAL(hkaSkeletonMapperData, high_low_data, &hkaSkeletonMapperData::Destruct);
				FNVD_CONSTRUCTED_LOCAL(hkaSkeletonMapperData, low_high_data, &hkaSkeletonMapperData::Destruct);
				alignas(16) hkaSkeletonMapperUtils::Params params;
				params.Init();
				bhkRagdollShareData* pParamShareData = spRagdollSharedData.m_pObject;
				params.m_skeletonA = pParamShareData->pHighSkel;
				params.m_skeletonB = pParamShareData->pLowSkel;
				params.m_autodetectChains.m_bool = false;
				hkaSkeletonMapperUtils::createMapping(params, high_low_data, low_high_data);
				params.Free();
				CheckMappersForWarnings();
				void* pSGToRagdollMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkaSkeletonMapper));
				static_cast<hkaSkeletonMapper*>(pSGToRagdollMemory)->m_memSizeAndFlags = sizeof(hkaSkeletonMapper);
				spRagdollSharedData.m_pObject->pSGToRagdoll =
					::new (pSGToRagdollMemory) hkaSkeletonMapper(&high_low_data);
				void* pRagdollToSGMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkaSkeletonMapper));
				static_cast<hkaSkeletonMapper*>(pRagdollToSGMemory)->m_memSizeAndFlags = sizeof(hkaSkeletonMapper);
				spRagdollSharedData.m_pObject->pRagdollToSG =
					::new (pRagdollToSGMemory) hkaSkeletonMapper(&low_high_data);
			}
		}

		constarray.~hkLocalArray<hkpConstraintInstance*>();
		rbarray.~hkLocalArray<hkpRigidBody*>();
	}
}

bool bhkRagdollController::GetEyeOffset(NiPoint3& arOffset) {
	bool bret = 0;
	if (bTargetOutsideHeadLimit) {
		hkVector4 eyePosWS;
		const hkQsTransform* v7;
		hkVector4 eyePosWS_2;
		eyePosWS.m_quad = (&BoneParamA[0].pPose->accessBoneLocalSpace(1))->m_translation.m_quad;
		int32_t ssBoneIdx = BoneParamA[1].ssBoneIdx;
		hkaPose* pSGPose = this->pSGPose;
		eyePosWS_2.m_quad = eyePosWS.m_quad;
		if ((pSGPose->m_boneFlags.m_data[ssBoneIdx] & 2) != 0) {
			v7 = &pSGPose->calculateBoneModelSpace(ssBoneIdx);
		} else {
			v7 = &pSGPose->m_modelPose.m_data[ssBoneIdx];
		}
		eyePosWS_2.setTransformedPos(*v7, eyePosWS_2);
		eyePosWS_2.setTransformedPos(WorldFromModel, eyePosWS_2);
		HK2NI(arOffset, eyePosWS_2);
		return 1;
	}
	return bret;
}

void bhkRagdollController::ApplyBoneTransforms() {
	NiTransform transform;
	int32_t v2 = 0;
	int32_t m_size = spRagdollSharedData.m_pObject->pHighSkel->m_bones.m_size;
	float v6 = 1.0 / pRagdollRoot->m_kWorld.m_fScale;
	if (m_size > 0) {
		int32_t v5 = 0;
		do {
			NiAVObject* v3 = SGBonesA.pBuffer[v2];
			HK2NI(transform, AnimPoseLS.m_data[v5++]);
			++v2;
			transform.m_Translate.x = transform.m_Translate.x * v6;
			double y = transform.m_Translate.y;
			v3->m_kLocal.m_Translate.x = transform.m_Translate.x;
			transform.m_Translate.y = y * v6;
			v3->m_kLocal.m_Translate.y = transform.m_Translate.y;
			transform.m_Translate.z = v6 * transform.m_Translate.z;
			v3->m_kLocal.m_Translate.z = transform.m_Translate.z;
			rt::memcpy(&v3->m_kLocal, &transform, 36u);
		} while (v2 < m_size);
	}
}

// The leg's foot placement setup from the ragdoll: the thigh named by arboneNameA's first entry, its first child (the
// calf) and the calf's first child (the foot), their capsule shapes and the first constraint of the calf's and of the
// foot's rigid body. The knee is a limited hinge, possibly wrapped in a malleable constraint; the ankle is one too,
// else a ball and socket. False, with arSetupData untouched, when any of them is missing.
// The exe's cleanup-only C++ EH frame is left out.
bool bhkRagdollController::ExtractBipedSetupData(hkaFootPlacementIkSolver::Setup& arSetupData,
	BSSimpleArray<const char*, 1024>& arboneNameA) {
	FNVD_INLINED_LOCAL(NiFixedString, apName, std::nullopt, std::nullopt);
	const char* pcThighName = arboneNameA.pBuffer[0];
	// EXE-INLINED 0x438170
	if (pcThighName) {
		apName.m_kHandle = NiGlobalStringTable::AddString(pcThighName);
	} else {
		apName.m_kHandle = NiGlobalStringTable::NULL_STRING;
	}
	NiAVObject* pthighObj = BSUtilities::GetObjectByName(pRagdollRoot, apName, true);
	// The exe's inlined destructor of apName.
	// EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
	NiGlobalStringTable::ReleaseHandle(apName.GetHandle());
	NiAVObject* pcalfObj = nullptr;
	if (pthighObj && pthighObj->IsNode() && static_cast<NiNode*>(pthighObj)->m_kChildren.m_usSize > 0) {
		pcalfObj = static_cast<NiNode*>(pthighObj)->m_kChildren.m_pBase[0].m_pObject;
	}
	NiAVObject* pfootObj = nullptr;
	if (pcalfObj && pcalfObj->IsNode() && static_cast<NiNode*>(pcalfObj)->m_kChildren.m_usSize > 0) {
		pfootObj = static_cast<NiNode*>(pcalfObj)->m_kChildren.m_pBase[0].m_pObject;
	}
	if (!pthighObj || !pcalfObj || !pfootObj) {
		return false;
	}
	hkCapsuleShapeCinfo* pthighCapsuleInfo = GetCapsuleBoneShapeInfo(pthighObj);
	hkCapsuleShapeCinfo* pcalfCapsuleInfo = GetCapsuleBoneShapeInfo(pcalfObj);
	hkCapsuleShapeCinfo* pfootCapsuleInfo = GetCapsuleBoneShapeInfo(pfootObj);
	BSSimpleList<NiPointer<bhkSerializable>>* pkneeConstrList = nullptr;
	bhkNiCollisionObject* pcalfCollision = pcalfObj->m_spCollisionObject.m_pObject;
	if (pcalfCollision) {
		bhkCollisionObject* pcalfCO =
			static_cast<bhkCollisionObject*>(pcalfCollision->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
		if (pcalfCO) {
			bhkRigidBody* pcalfRB = pcalfCO->spWorldObject;
			if (pcalfRB) {
				pkneeConstrList = &pcalfRB->kActionConstrants;
			}
		}
	}
	BSSimpleList<NiPointer<bhkSerializable>>* pfootConstrList = nullptr;
	bhkNiCollisionObject* pfootCollision = pfootObj->m_spCollisionObject.m_pObject;
	if (pfootCollision) {
		bhkCollisionObject* pfootCO =
			static_cast<bhkCollisionObject*>(pfootCollision->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
		if (pfootCO) {
			bhkRigidBody* pfootRB = pfootCO->spWorldObject;
			if (pfootRB) {
				pfootConstrList = &pfootRB->kActionConstrants;
			}
		}
	}
	if (!pthighCapsuleInfo || !pcalfCapsuleInfo || !pfootCapsuleInfo || !pkneeConstrList || !pfootConstrList) {
		return false;
	}

	// The knee: a limited hinge, else the limited hinge data a malleable constraint wraps.
	hkpLimitedHingeConstraintData* pKneeConstrData = nullptr;
	bhkLimitedHingeConstraint* pKneeConstr =
		static_cast<bhkLimitedHingeConstraint*>(NiObject::DynamicCastStatic(&bhkLimitedHingeConstraint::ms_RTTI,
			pkneeConstrList->m_item));
	if (pKneeConstr) {
		pKneeConstrData = static_cast<hkpLimitedHingeConstraintData*>(pKneeConstr->GetCinfo().pData);
	}
	if (!pKneeConstr) {
		bhkMalleableConstraint* pMalKneeConstr =
			static_cast<bhkMalleableConstraint*>(NiObject::DynamicCastStatic(&bhkMalleableConstraint::ms_RTTI,
				pkneeConstrList->m_item));
		if (pMalKneeConstr) {
			hkMalleableConstraintCinfo& malKneeCInfo = pMalKneeConstr->GetCinfo();
			hkpMalleableConstraintData* pMalKneeData = static_cast<hkpMalleableConstraintData*>(malKneeCInfo.pData);
			hkpConstraintData* pKneeWrapData = pMalKneeData ? pMalKneeData->getWrappedConstraintData() : nullptr;
			if (pKneeWrapData && pKneeWrapData->getType() == hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE) {
				pKneeConstrData = static_cast<hkpLimitedHingeConstraintData*>(pKneeWrapData);
			}
		}
	}
	if (!pKneeConstrData) {
		return false;
	}

	// The knee's limits in degrees, moved in by 10 and 20, back to radians and to their cosines. Each step is rounded
	// to a float, as the exe stores it.
	float fMaxAngle = 90.0 - pKneeConstrData->m_atoms.m_angLimit.m_maxAngle * 57.2957763671875 - 10.0;
	fMaxAngle = fMaxAngle * 0.01745329238474369;
	float fcosMax = std::cos(fMaxAngle);
	fcosMax = -fcosMax;
	float fMinAngle = pKneeConstrData->m_atoms.m_angLimit.m_minAngle * 57.2957763671875 + 90.0 - 20.0;
	fMinAngle = fMinAngle * 0.01745329238474369;
	float fcosMin = std::cos(fMinAngle);
	hkVector4 cAxis = pKneeConstrData->m_atoms.m_transforms.m_transformA.getRotation().getColumn(0);

	// The ankle: a limited hinge, else the limited hinge data a malleable constraint wraps, else a ball and socket.
	bhkLimitedHingeConstraint* pAnklConstr =
		static_cast<bhkLimitedHingeConstraint*>(NiObject::DynamicCastStatic(&bhkLimitedHingeConstraint::ms_RTTI,
			pfootConstrList->m_item));
	hkpLimitedHingeConstraintData* pAnklConstrData = nullptr;
	if (pAnklConstr) {
		pAnklConstrData = static_cast<hkpLimitedHingeConstraintData*>(pAnklConstr->GetCinfo().pData);
	}
	if (!pAnklConstr) {
		bhkMalleableConstraint* pMalAnklConstr =
			static_cast<bhkMalleableConstraint*>(NiObject::DynamicCastStatic(&bhkMalleableConstraint::ms_RTTI,
				pfootConstrList->m_item));
		if (pMalAnklConstr) {
			hkMalleableConstraintCinfo& malAnklCInfo = pMalAnklConstr->GetCinfo();
			hkpMalleableConstraintData* pMalAnklData = static_cast<hkpMalleableConstraintData*>(malAnklCInfo.pData);
			hkpConstraintData* pAnklWrapData = pMalAnklData ? pMalAnklData->getWrappedConstraintData() : nullptr;
			if (pAnklWrapData && pAnklWrapData->getType() == hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE) {
				pAnklConstrData = static_cast<hkpLimitedHingeConstraintData*>(pAnklWrapData);
			}
		}
	}
	hkVector4 cAnklPos;
	if (pAnklConstrData) {
		cAnklPos = pAnklConstrData->m_atoms.m_transforms.m_transformA.getTranslation();
	} else {
		bhkBallAndSocketConstraint* pBallAnklConstr =
			static_cast<bhkBallAndSocketConstraint*>(NiObject::DynamicCastStatic(&bhkBallAndSocketConstraint::ms_RTTI,
				pfootConstrList->m_item));
		hkpBallAndSocketConstraintData* pBallConstrData = nullptr;
		if (pBallAnklConstr) {
			pBallConstrData = static_cast<hkpBallAndSocketConstraintData*>(pBallAnklConstr->GetCinfo()->pData);
		}
		if (!pBallConstrData) {
			return false;
		}
		cAnklPos = pBallConstrData->m_atoms.m_pivots.m_translationA;
		// Here the exe also takes the thigh's and the calf's capsule lengths (vertex A minus vertex B, length3) and
		// never reads them: left out, as they have no effect.
	}

	// The foot capsule's axis, vertex A minus vertex B.
	hkVector4 cStartFoot;
	// EXE-INLINED 0x4B4D10
	cStartFoot.set(pfootCapsuleInfo->m_vertexA.fX, pfootCapsuleInfo->m_vertexA.fY, pfootCapsuleInfo->m_vertexA.fZ,
		pfootCapsuleInfo->m_vertexA.fW);
	hkVector4 cEndFoot;
	// EXE-INLINED 0x4B4D10
	cEndFoot.set(pfootCapsuleInfo->m_vertexB.fX, pfootCapsuleInfo->m_vertexB.fY, pfootCapsuleInfo->m_vertexB.fZ,
		pfootCapsuleInfo->m_vertexB.fW);
	hkVector4 cEndFootFromAnkle;
	cEndFootFromAnkle.setSub4(cStartFoot, cEndFoot);
	float fAnklAdj = gs_fAnkleOffset_FootIK.Float() + cAnklPos(0);

	arSetupData.m_kneeAxisLS = cAxis;
	arSetupData.m_kneeAxisLS.normalize3();
	// A foot shorter than half a unit gets no end offset.
	float fFootLen = cEndFootFromAnkle.length3();
	if (fFootLen < 0.5f) {
		cEndFootFromAnkle.setZero4();
	}
	arSetupData.m_footEndLS = cEndFootFromAnkle;
	arSetupData.m_minAnkleHeightMS = -gs_fMaxStepVertError_FootIK.Float();
	arSetupData.m_maxAnkleHeightMS = pfootCapsuleInfo->m_vertexA.fX * 2.0 +
		(pcalfCapsuleInfo->m_vertexA.fX - pcalfCapsuleInfo->m_vertexB.fX) + fAnklAdj;
	arSetupData.m_footPlantedAnkleHeightMS = pfootCapsuleInfo->m_vertexA.fX;
	arSetupData.m_footRaisedAnkleHeightMS = arSetupData.m_maxAnkleHeightMS - 2.0 * fAnklAdj;
	arSetupData.m_cosineMaxKneeAngle = fcosMax;
	arSetupData.m_cosineMinKneeAngle = fcosMin;
	return true;
}

void bhkRagdollController::UpdateConstraintsWorld(int32_t abAdd) {
	hkpConstraintOwner* m_owner;
	hkaRagdollInstance* pRagdollInstance = this->pRagdollInstance;
	hkpConstraintInstance** m_data = pRagdollInstance->m_constraints.begin();
	if (*m_data) {
		m_owner = (*m_data)->getOwner();
	} else {
		m_owner = 0;
	}
	hkpRigidBody** v5 = pRagdollInstance->m_rigidBodies.begin();
	if (*v5) {
		ahkpWorld* m_world = static_cast<ahkpWorld*>((*v5)->m_world);
		if (m_world) {
			if (static_cast<uint8_t>(abAdd)) {
				if (m_owner) {
					return;
				}
			} else {
				if (!m_owner) {
					return;
				}
			}
			bhkWorld* m_userData = static_cast<bhkWorld*>(m_world->m_userData);
			int32_t v8 = 0;
			if (static_cast<uint8_t>(abAdd)) {
				if (pRagdollInstance->m_constraints.m_size > 0) {
					do {
						hkpConstraintInstance** v9 = pRagdollInstance->m_constraints.begin();
						if (!v9[v8]->getOwner()) {
							m_userData->Add(v9[v8]);
						}
						++v8;
					} while (v8 < pRagdollInstance->m_constraints.m_size);
				}
			} else {
				if (pRagdollInstance->m_constraints.m_size > 0) {
					do {
						if (pRagdollInstance->m_constraints.m_data[v8]->getOwner()) {
							m_userData->RemoveConstraint(pRagdollInstance->m_constraints.m_data[v8]);
						}
						++v8;
					} while (v8 < pRagdollInstance->m_constraints.m_size);
				}
			}
		}
	}
}

// Sizes arBoneRBA to the ragdoll instance's rigid body count and copies its bodies in.
void bhkRagdollController::GetRBForBones(hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arBoneRBA) {
	if (pRagdollInstance) {
		arBoneRBA.setSize(pRagdollInstance->m_rigidBodies.m_size);
		hkaRagdollInstance* pInstance = pRagdollInstance;
		int32_t iNumBodies = pInstance->m_rigidBodies.m_size;
		hkpRigidBody** pSource = pInstance->m_rigidBodies.begin();
		hkpRigidBody** pDest = arBoneRBA.begin();
		for (int32_t i = 0; i < iNumBodies; ++i) {
			pDest[i] = pSource[i];
		}
	}
}

void bhkRagdollController::SetGrabIKEnabled(bhkRagdollController::GRAB_IK_ARM arArm, bool abEnable) {
	if (abEnable) {
		bActiveGrabIK = bEaseOutRagdollAnim ? bActiveGrabIK : bEnableGrabIK;
	}
	if (bActiveGrabIK) {
		GrabIKState[arArm].bEnabled = abEnable;
	}
}

void bhkRagdollController::SetGrabIKIntrpGain(bhkRagdollController::GRAB_IK_ARM arArm, float afGain) {
	GrabIKState[arArm].fIntrpGain = afGain;
}

void bhkRagdollController::SetGrabIKTarget(bhkRagdollController::GRAB_IK_ARM arArm, NiAVObject* apTarget) {
	GrabIKState[arArm].pTarget = apTarget;
}

// Adds apObj as a skeleton bone (its name, its local transform in Havok units, its parent's bone index, the
// translation scaled by the ragdoll root's world scale when it has a parent bone) and as a scene graph bone, then
// recurses into the children of a node whose collision object is a blended one, not a weapon's.
void bhkRagdollController::CreateSGSkeletonRec(NiAVObject* apObj,
	BSSimpleArray<bhkRagdollController::bhkSkeletonBone*, 1024>& arBoneA, NiTMap<NiAVObject*, int16_t>& arboneMap) {
	// EXE-INLINED 0xC74EA0
	// The exe treats the allocation as untyped memory: it aligns the pointer itself and writes the padding length
	// into the byte before it.
	uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(bhkSkeletonBone) + 0x10));
	const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
	pMemory += ucPadding;
	pMemory[-1] = ucPadding;
	bhkSkeletonBone* pbone = reinterpret_cast<bhkSkeletonBone*>(pMemory);
	pbone->name = apObj->m_kName.GetHandle();
	NI2HK(pbone->transform, apObj->m_kLocal);
	// The exe's slot is 16-aligned in its realigned frame (esp+0x10 after the and esp, -16 prologue, 0xC7B548); it
	// stores the -1 as a whole dword there before GetAt.
	alignas(16) int16_t ival = -1;
	arboneMap.GetAt(apObj->m_pkParent, ival);
	pbone->parentIndice = ival;
	if (ival != -1) {
		pbone->transform.m_translation.m_quad =
			_mm_mul_ps(_mm_set1_ps(pRagdollRoot->m_kWorld.m_fScale), pbone->transform.m_translation.m_quad);
	}
	arboneMap.SetAt(apObj, static_cast<int16_t>(arBoneA.iSize));
	// The index first: the call may move the buffer.
	uint32_t uiBoneIndex = arBoneA.AddUninitialized();
	arBoneA.pBuffer[uiBoneIndex] = pbone;
	uint32_t uiSGBoneIndex = SGBonesA.AddUninitialized();
	SGBonesA.pBuffer[uiSGBoneIndex] = apObj;
	NiNode* pnode = apObj->IsNode();
	if (!pnode) {
		return;
	}
	for (uint32_t i = 0; i < pnode->m_kChildren.m_usSize; ++i) {
		NiAVObject* pchild = pnode->m_kChildren.m_pBase[i].m_pObject;
		bhkBlendCollisionObject* pchildBlendCol = nullptr;
		bool bWeapon = false;
		if (pchild) {
			bhkCollisionObject* pchildCol = bhkUtilFunctions::FindNextCollisionObject(pchild);
			if (pchildCol) {
				pchildBlendCol = pchildCol->IsBhkBlendCollisionObject();
				if (pchildBlendCol) {
					bWeapon = pchildBlendCol->IsKindOf(&WeaponObject::ms_RTTI);
				}
			}
		}
		if (pchild && pchildBlendCol && !bWeapon) {
			CreateSGSkeletonRec(pchild, arBoneA, arboneMap);
		}
	}
}

bool bhkRagdollController::QLookIK() {
	return (&bhkRagdollController::bLookIK)->Bool();
}

void bhkRagdollController::operator delete(void* apMemory) {
	if (apMemory) {
		uint8_t* pMemory = static_cast<uint8_t*>(apMemory);
		MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
	}
}

void bhkRagdollController::ResetFootCastTimer() {
	bhkRagdollController::fFootCastTimer = (float)0.0;
}

float bhkRagdollController::QMaxTrackingDistance() {
	return (&bhkRagdollController::fMaxTrackingDist)->Float();
}

float bhkRagdollController::QMinTrackingDistance() {
	return (&bhkRagdollController::fMinTrackingDist)->Float();
}

float bhkRagdollController::QCameraDistance() {
	return (&bhkRagdollController::fCameraDist)->Float();
}

// The exe reloads the buffer and the node for the rotation.
void bhkRagdollController::ResetPose(NiNode* apSkeleton, BSSimpleArray<BoneTransf, 1024>& arPoseA, bool abPrevUseVel,
	hkpMotion::MotionType aePrevMotion) {
	for (uint32_t ui = 0; ui < arPoseA.iSize; ++ui) {
		BoneTransf& rBone = arPoseA.pBuffer[ui];
		rBone.pObj->m_kLocal.m_Translate = rBone.LocalTransform.m_Translate;
		arPoseA.pBuffer[ui].pObj->m_kLocal.m_Rotate = arPoseA.pBuffer[ui].LocalTransform.m_Rotate;
	}
	NiUpdateData data;
	// EXE-INLINED 0x43D410
	data.fTime = 0.0f;
	data.bUpdateControllers = false;
	data.bParallelUpdate = false;
	data.bFoundParticles = false;
	data.bFoundMorphController = false;
	data.bSceneGraphChange = false;
	apSkeleton->Update(data);
	if (aePrevMotion != hkpMotion::MOTION_KEYFRAMED && aePrevMotion != hkpMotion::MOTION_INVALID) {
		bhkWorld::SetMotion(apSkeleton, aePrevMotion, 1, 0, 1);
	}
	if (abPrevUseVel) {
		bhkUtilFunctions::SetUseVelocityRecurse(abPrevUseVel, apSkeleton);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void bhkRagdollController::CopyPoseTo3DRec(NiNode* apSkeleton, NiAVObject* apPose,
	BSSimpleArray<BoneTransf, 1024>& arPoseA) {
	FNVD_INLINED_LOCAL(NiFixedString, poseBoneName, std::nullopt, std::nullopt);
	BoneTransf bone;
	const char* pHandle = apPose->m_kName.GetHandle();
	NiGlobalStringTable::AcquireHandle(pHandle);
	poseBoneName.m_kHandle = apPose->m_kName.GetHandle();
	NiAVObject* pbonePose = apPose;
	if (poseBoneName.Equals("Bip01 NonAccum")) {
		pbonePose = apPose->m_pkParent;
	}
	NiAVObject* pboneSkel = BSUtilities::GetObjectByName(apSkeleton, poseBoneName, true);
	if (pboneSkel) {
		bone.pObj = pboneSkel;
		bone.LocalTransform = pboneSkel->m_kLocal;
		uint32_t uiIndex = arPoseA.AddUninitialized();
		arPoseA._ConstructItems(&arPoseA.pBuffer[uiIndex], 1u);
		arPoseA.pBuffer[uiIndex] = bone;
		const NiTransform& localTransf = pbonePose->m_kLocal;
		pboneSkel->m_kLocal.m_Rotate = localTransf.m_Rotate;
		pboneSkel->m_kLocal.m_Translate = localTransf.m_Translate;
	}
	NiNode* pnode = apPose->IsNode();
	if (pnode) {
		for (uint32_t i = 0; i < pnode->m_kChildren.m_usSize; ++i) {
			NiAVObject* pChild = pnode->m_kChildren.m_pBase[i].m_pObject;
			if (pChild) {
				CopyPoseTo3DRec(apSkeleton, pChild, arPoseA);
			}
		}
	}
	// The exe's inlined destructor of poseBoneName.
	// EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
	NiGlobalStringTable::ReleaseHandle(poseBoneName.GetHandle());
}

void bhkRagdollController::CopyPoseTo3D(NiNode* apSkeleton, NiNode* apPose, BSSimpleArray<BoneTransf, 1024>& arPoseA,
	bool& arbPrevUseVel, hkpMotion::MotionType& arePrevMotion) {
	bhkCollisionObject* pcolObj = bhkUtilFunctions::FindNextCollisionObject(apSkeleton);
	bool bUseVel = false;
	if (pcolObj) {
		uint16_t usFlags = pcolObj->usFlags;
		if ((usFlags & bhkNiCollisionObject::USEVEL_POS) != 0 && (usFlags & bhkNiCollisionObject::RESET_POS) == 0) {
			bUseVel = true;
		}
	}
	arbPrevUseVel = bUseVel;
	if (bUseVel) {
		bhkUtilFunctions::SetUseVelocityRecurse(0, apSkeleton);
	}
	if (pcolObj) {
		bhkRigidBody* pbody = pcolObj->spWorldObject;
		hkpRigidBody* phkBody;
		if (pbody && (phkBody = static_cast<hkpRigidBody*>(pbody->phkObject)) != 0) {
			arePrevMotion = static_cast<hkpMotion::MotionType>(phkBody->m_motion.m_type.m_storage);
		} else {
			arePrevMotion = hkpMotion::MOTION_FIXED;
		}
	}
	if (arePrevMotion != hkpMotion::MOTION_KEYFRAMED) {
		bhkWorld::SetMotion(apSkeleton, hkpMotion::MOTION_KEYFRAMED, 1, 0, 1);
	}
	if (apSkeleton) {
		if (apPose) {
			NiUpdateData data;
			CopyPoseTo3DRec(apSkeleton, apPose, arPoseA);
			data.fTime = 0.0f;
			data.bUpdateControllers = 0;
			data.bParallelUpdate = 0;
			data.bFoundParticles = 0;
			data.bFoundMorphController = 0;
			data.bSceneGraphChange = 0;
			apSkeleton->Update(data);
		}
	}
}

// Adds to arRigidBodies and arConstraints the Havok body and constraints of every blended rigid body under apObject
// that is not a weapon's, then recurses into the children of a node.
void bhkRagdollController::FindBones(NiAVObject* apObject,
	hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
	hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints) {
	if (!apObject) {
		return;
	}
	NiObject* pCollisionObject = apObject->m_spCollisionObject.m_pObject;
	if (pCollisionObject) {
		bhkBlendCollisionObject* pcol = static_cast<bhkBlendCollisionObject*>(pCollisionObject
				->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI));
		if (pcol) {
			WeaponObject* pweap = static_cast<WeaponObject*>(pcol->DynamicCastMember(&WeaponObject::ms_RTTI));
			if (pcol->spWorldObject) {
				bhkRigidBody* prb = static_cast<bhkRigidBody*>(static_cast<NiObject*>(pcol->spWorldObject)
						->DynamicCastMember(&bhkRigidBody::ms_RTTI));
				if (prb && !pweap) {
					AddRagdollData(prb, arRigidBodies, arConstraints);
				}
			}
		}
	}
	NiNode* pnode = static_cast<NiNode*>(apObject->DynamicCastMember(&NiNode::ms_RTTI));
	if (pnode) {
		for (uint32_t I = 0; I < pnode->m_kChildren.m_usSize; ++I) {
			FindBones(pnode->m_kChildren.m_pBase[I].m_pObject, arRigidBodies, arConstraints);
		}
	}
}

// Fills apCinfo from the wrapped capsule: the convex shape's data, then the two end points (zero without a wrapped
// object). The exe tests this and reads phkObject again after the first copy.
void bhkCapsuleShape::GetCinfo(hkCapsuleShapeCinfo* apCinfo) {
	CopyCinfo(*apCinfo);
	// EXE-INLINED 0x4AE750
	hkpCapsuleShape* pCapsule = static_cast<hkpCapsuleShape*>(this ? phkObject : nullptr);
	apCinfo->m_vertexA = pCapsule ? pCapsule->m_vertexA : hkVector4Zero;
	// EXE-INLINED 0x4AE750
	pCapsule = static_cast<hkpCapsuleShape*>(this ? phkObject : nullptr);
	apCinfo->m_vertexB = pCapsule ? pCapsule->m_vertexB : hkVector4Zero;
}

// The constructor leaves pFeedbackData unset, as the exe's does.
bhkRagdollShareData::bhkRagdollShareData()
	: pLowSkel(nullptr), pHighSkel(nullptr), pSGToRagdoll(nullptr), pRagdollToSG(nullptr), pSharedLookData(nullptr) {}

// Releases the skeletons, the mappers and the look IK data; the look IK data's 16-aligned block keeps its alignment
// padding byte in front of the pointer. The exe deletes pHighSkel before the mappers, whose data then releases its
// reference on the freed skeleton (a decrement in a dead free-list block); src releases the mappers first, which
// leaves the same objects alive and freed.
bhkRagdollShareData::~bhkRagdollShareData() {
	if (pLowSkel) {
		pLowSkel->removeReference();
	}
	if (pSGToRagdoll) {
		pSGToRagdoll->removeReference();
	}
	if (pRagdollToSG) {
		pRagdollToSG->removeReference();
	}
	if (pHighSkel) {
		delete pHighSkel;
	}
	if (pSharedLookData) {
		pSharedLookData->~bhkLookIKData();
		uint8_t* pMemory = static_cast<uint8_t*>(static_cast<void*>(pSharedLookData));
		MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
	}
}

// Copies arData into this: the dynamic bone indices, then the enabled flag and the feedback settings.
void RagdollFeedbackData::Copy(const RagdollFeedbackData& arData) {
	uint32_t uiSize = arData.DynamicBoneIdxA.iSize;
	// The exe calls _Allocate and _Deallocate through the array's vtable.
	BSSimpleArray<int16_t>* pArray = &DynamicBoneIdxA;
	// EXE-INLINED 0x5EEE80
	if (!uiSize) {
		if (pArray->pBuffer) {
			pArray->_Deallocate(pArray->pBuffer);
			pArray->pBuffer = nullptr;
			pArray->iReservedSize = 0;
			pArray->iSize = 0;
		}
	} else {
		uint32_t uiReservedSize = pArray->iReservedSize;
		if (uiSize > uiReservedSize) {
			if (uiReservedSize) {
				pArray->_ReallocateBuffer(uiSize, pArray->iSize);
			} else {
				pArray->pBuffer = pArray->_Allocate(uiSize);
			}
			pArray->iReservedSize = uiSize;
			pArray->iSize = uiSize;
		} else if (uiSize >= pArray->iSize) {
			pArray->iSize = uiSize;
		} else {
			pArray->iSize = uiSize;
			if (uiSize <= uiReservedSize >> 2) {
				pArray->_ReallocateBuffer(uiSize, uiSize);
				pArray->iReservedSize = uiSize;
			}
		}
	}
	for (uint32_t ui = 0; ui < arData.DynamicBoneIdxA.iSize; ++ui) {
		DynamicBoneIdxA.pBuffer[ui] = arData.DynamicBoneIdxA.pBuffer[ui];
	}
	bEnabled = arData.bEnabled;
	// The exe copies the floats with fld/fstp and the two counts as dwords.
	const RagdollFeedbackSystemData& rSource = arData.SystemData;
	SystemData.fKeyDynIntrp = rSource.fKeyDynIntrp;
	SystemData.fHeirGain = rSource.fHeirGain;
	SystemData.fPosGain = rSource.fPosGain;
	SystemData.fVelGain = rSource.fVelGain;
	SystemData.fAccGain = rSource.fAccGain;
	SystemData.fSnapGain = rSource.fSnapGain;
	SystemData.fVelDamp = rSource.fVelDamp;
	SystemData.fSnapMaxLinVel = rSource.fSnapMaxLinVel;
	SystemData.fSnapMaxAngVel = rSource.fSnapMaxAngVel;
	SystemData.fSnapMaxLinDist = rSource.fSnapMaxLinDist;
	SystemData.fSnapMaxAngDist = rSource.fSnapMaxAngDist;
	SystemData.fPosMaxLinVel = rSource.fPosMaxLinVel;
	SystemData.fPosMaxAngVel = rSource.fPosMaxAngVel;
	SystemData.uiImpulseMult1 = rSource.uiImpulseMult1;
	SystemData.uiImpulseMult2 = rSource.uiImpulseMult2;
}

// Converts the Havok vector arhkV's three components to game units. The exe fetches each component out of line,
// through hkVector4::getSimdAt and hkSimdReal::operator float, and converts it with HK2NI(float). The exe realigns
// its frame to 16 for the three hidden hkSimdReal results it passes getSimdAt; hkSimdReal is alignas(16), so src's
// temporaries are aligned the same way. The exe's stack cookie check is left out.
NiPoint3& HK2NI(NiPoint3& arNiP, const hkVector4& arhkV) {
	arNiP.x = HK2NI(static_cast<float>(arhkV.getSimdAt(0)));
	arNiP.y = HK2NI(static_cast<float>(arhkV.getSimdAt(1)));
	arNiP.z = HK2NI(static_cast<float>(arhkV.getSimdAt(2)));
	return arNiP;
}

// Converts the game-unit point arNiP's three components to Havok units into arhkV, then zeroes the fourth. Each
// component goes through the out-of-line NI2HK(float) and is stored through the out-of-line hkVector4::operator().
hkVector4& NI2HK(hkVector4& arhkV, const NiPoint3& arNiP) {
	float fComponent = NI2HK(arNiP.x);
	arhkV(0) = fComponent;
	fComponent = NI2HK(arNiP.y);
	arhkV(1) = fComponent;
	fComponent = NI2HK(arNiP.z);
	arhkV(2) = fComponent;
	arhkV(3) = 0.0f;
	return arhkV;
}

// The exe builds the column once through 0x6815C0 (a trivial accessor: no store) and reads each Havok column
// through 0x4B4CF0, the out-of-line copy of the inline getColumn.
NiMatrix3& HK2NI(NiMatrix3& arNiRot, const hkRotation& arhkRot) {
	NiPoint3 nic;
	for (uint32_t i = 0; i < 3; ++i) {
		HK2NINS(nic, arhkRot.getColumn(i));
		arNiRot.SetCol(i, nic);
	}
	return arNiRot;
}

// Converts the game transform arNiTran to Havok units: the rotation, then the translation, both through the
// out-of-line conversions, the translation stored through hkTransform::setTranslation's out-of-line copy. The exe's
// calls to the shared identity function 0x6815C0 only fetch the transform's and the local vector's addresses
// (m_rotation sits at offset 0), so src passes them directly.
hkTransform& NI2HK(hkTransform& arhkTran, const NiTransform& arNiTran) {
	hkVector4 kTranslation;
	NI2HK(arhkTran.getRotation(), arNiTran.m_Rotate);
	arhkTran.setTranslation(NI2HK(kTranslation, arNiTran.m_Translate));
	return arhkTran;
}

// The exe builds the column once through 0x6815C0 (a trivial accessor: no store) and reaches each Havok column
// through 0x4B4CF0, the out-of-line copy of the inline getColumn.
hkRotation& NI2HK(hkRotation& arhkRot, const NiMatrix3& arNiRot) {
	NiPoint3 nic;
	for (uint32_t i = 0; i < 3; ++i) {
		arNiRot.GetCol(i, nic);
		NI2HKNS(arhkRot.getColumn(i), nic);
	}
	return arhkRot;
}

hkQsTransform& NI2HK(hkQsTransform& arhkQ, const NiTransform& arNiQ) {
	alignas(16) NiQuaternion niquat;
	niquat.FromRotation(arNiQ.m_Rotate);
	arhkQ.m_rotation.m_vec.m_quad[0u] = niquat.m_fX;
	arhkQ.m_rotation.m_vec.m_quad[1] = niquat.m_fY;
	arhkQ.m_rotation.m_vec.m_quad[2] = niquat.m_fZ;
	arhkQ.m_rotation.m_vec.m_quad[3] = niquat.m_fW;
	arhkQ.m_rotation.normalize();
	// The translation, scaled to Havok units (the exe builds it in the quaternion's stack slot).
	hkVector4 kTranslation;
	kTranslation.m_quad[0] = arNiQ.m_Translate.x * bhkConvert::fBS2HkScaleSC;
	kTranslation.m_quad[1] = arNiQ.m_Translate.y * bhkConvert::fBS2HkScaleSC;
	kTranslation.m_quad[2] = bhkConvert::fBS2HkScaleSC * arNiQ.m_Translate.z;
	kTranslation.m_quad[3] = 0.0f;
	arhkQ.m_translation = kTranslation;
	arhkQ.m_scale.m_quad = _mm_shuffle_ps(_mm_set_ss(arNiQ.m_fScale), _mm_set_ss(arNiQ.m_fScale), 0);
	return arhkQ;
}

hkaRaycastInterface::~hkaRaycastInterface() {}

// The filtered cast hits nothing here: the exe returns false without reading its arguments.
hkBool hkaRaycastInterface::castRay(const hkVector4& fromWS, const hkVector4& toWS, uint32_t collisionFilterInfo,
	float& hitFractionOut, hkVector4& normalWS) {
	hkBool kResult;
	kResult.m_bool = 0;
	return kResult;
}

void hkaRaycastInterface::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}

hkaSkeletonMapperUtils::Params::Params(const hkaSkeletonMapperUtils::Params& other)
	: m_skeletonA(other.m_skeletonA), m_skeletonB(other.m_skeletonB), m_compareNames(other.m_compareNames),
	  m_positionMatchTolerance(other.m_positionMatchTolerance),
	  m_rotationMatchTolerance(other.m_rotationMatchTolerance), m_userChains(), m_userMappingsAtoB(),
	  m_userMappingsBtoA(), m_autodetectSimple(other.m_autodetectSimple), m_autodetectChains(other.m_autodetectChains),
	  m_mappingType(other.m_mappingType), m_motionParams(other.m_motionParams) {
	m_userChains = other.m_userChains;
	m_userMappingsAtoB = other.m_userMappingsAtoB;
	m_userMappingsBtoA = other.m_userMappingsBtoA;
}

hkaSkeletonMapperUtils::Params* hkaSkeletonMapperUtils::Params::Init() {
	m_positionMatchTolerance = (float)0.0;
	m_skeletonA = nullptr;
	m_rotationMatchTolerance = 0.5235987901687622f;
	m_skeletonB = nullptr;
	m_compareNames = nullptr;
	m_userChains.m_data = nullptr;
	m_userChains.m_size = (uint32_t)0;
	m_userChains.m_capacityAndFlags = 0x80000000u;
	m_userMappingsAtoB.m_capacityAndFlags = 0x80000000u;
	m_userMappingsAtoB.m_data = nullptr;
	m_userMappingsAtoB.m_size = (uint32_t)0;
	m_userMappingsBtoA.m_capacityAndFlags = 0x80000000u;
	m_userMappingsBtoA.m_data = nullptr;
	m_userMappingsBtoA.m_size = (uint32_t)0;
	m_autodetectSimple.m_bool = 1;
	m_autodetectChains.m_bool = 1;
	m_mappingType.m_storage = 0;
	m_motionParams.m_extractedMotionFactorA = (float)1.0;
	m_motionParams.m_extractedMotionFactorB = (float)1.0;
	m_motionParams.m_mode.m_storage = 0;
	m_motionParams.m_extractedMotionBoneA = 0;
	m_motionParams.m_extractedMotionBoneB = 0;
	return this;
}

// The exe's cleanup-only C++ EH frame is left out.
void hkaSkeletonMapperUtils::Params::Free() {
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t m_capacityAndFlags = m_userMappingsBtoA.m_capacityAndFlags;
	m_userMappingsBtoA.m_size = 0;
	if (m_capacityAndFlags >= 0) {
		m_heap->bufFree(m_userMappingsBtoA.begin(), sizeof(hkaSkeletonMapperUtils::UserMapping) * m_capacityAndFlags);
	}
	m_userMappingsBtoA.m_data = 0;
	m_userMappingsBtoA.m_capacityAndFlags = hkArrayBase<hkaSkeletonMapperUtils::UserMapping>::DONT_DEALLOCATE_FLAG;
	hkMemoryAllocator* v4 = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v5 = m_userMappingsAtoB.m_capacityAndFlags;
	m_userMappingsAtoB.m_size = 0;
	if ((v5 & 0x80000000u) == 0) {
		v4->bufFree(m_userMappingsAtoB.begin(), sizeof(hkaSkeletonMapperUtils::UserMapping) * v5);
	}
	m_userMappingsAtoB.m_data = 0;
	m_userMappingsAtoB.m_capacityAndFlags = hkArrayBase<hkaSkeletonMapperUtils::UserMapping>::DONT_DEALLOCATE_FLAG;
	hkMemoryAllocator* v6 = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v7 = m_userChains.m_capacityAndFlags;
	m_userChains.m_size = 0;
	if ((v7 & 0x80000000u) == 0) {
		v6->bufFree(m_userChains.begin(), sizeof(hkaSkeletonMapperUtils::UserChain) * v7);
	}
	m_userChains.m_capacityAndFlags = hkArrayBase<hkaSkeletonMapperUtils::UserChain>::DONT_DEALLOCATE_FLAG;
	m_userChains.m_data = 0;
}

void hkaPose::operator delete(void* apMemory) {
	if (apMemory) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		Instance->m_heap->blockFree(apMemory, sizeof(hkaPose));
	}
}

// The type info's finish-loaded-object callback for hkaSkeleton: its finish constructor, inlined.
void finishLoadedObjecthkaSkeleton(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkaSkeleton(flag);
	}
}

const hkClass& hkaSkeleton::staticClass() {
	return hkaSkeletonClass;
}

float hkMath::acos(float r) {
	double v1 = r;
	float ra = std::fabs((double)r);
	if (!(ra >= 1.0f)) {
		return std::acos(v1);
	} else {
		bool v2 = v1 > 0.0;
		double v3 = 0.0;
		if (!v2) {
			return 3.1415927f;
		}
		return v3;
	}
}
