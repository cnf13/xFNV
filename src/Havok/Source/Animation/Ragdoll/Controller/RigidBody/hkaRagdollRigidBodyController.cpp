#include "Havok/Source/Animation/Ragdoll/Controller/RigidBody/hkaRagdollRigidBodyController.h"

#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Animation/Ragdoll/Controller/RigidBody/hkaKeyFrameHierarchyUtility.h"
#include "Havok/Source/Animation/Ragdoll/Instance/hkaRagdollInstance.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "SDK/MSVC/string.h"

hkaRagdollRigidBodyController::hkaRagdollRigidBodyController(hkaRagdollInstance* a1) : m_ragdollInstance(a1) {
	m_bodyData.m_controlDataIndices = nullptr;
	m_bodyData.m_boneWeights = nullptr;
	a1->addReference();
	// One palette entry, the default control data.
	m_controlDataPalette.setSize(1);
	m_controlDataPalette[0] = hkaKeyFrameHierarchyUtility::ControlData();
	// Each body's parent body: the skeleton's parents when every bone has a body, else the body of the nearest
	// ancestor bone that has one (-1 for none).
	const int32_t iNumBodies = a1->m_rigidBodies.getSize();
	m_rbParentIndices.setSize(iNumBodies);
	for (int32_t i = 0; i < iNumBodies; ++i) {
		m_rbParentIndices[i] = -1;
	}
	const hkaSkeleton* pSkeleton = a1->m_skeleton;
	if (iNumBodies == pSkeleton->m_bones.getSize()) {
		for (int32_t iBone = 0; iBone < pSkeleton->m_bones.getSize(); ++iBone) {
			m_rbParentIndices[iBone] = pSkeleton->m_parentIndices.m_data[iBone];
		}
	} else {
		const int32_t* piBoneToBody = a1->m_boneToRigidBodyMap.begin();
		for (int32_t iBone = 0; iBone < pSkeleton->m_bones.getSize(); ++iBone) {
			const int32_t iBody = piBoneToBody[iBone];
			if (iBody < 0) {
				continue;
			}
			int16_t iParentBody = -1;
			for (int32_t iParent = pSkeleton->m_parentIndices.m_data[iBone]; iParent >= 0;
				iParent = pSkeleton->m_parentIndices.m_data[iParent]) {
				if (piBoneToBody[iParent] >= 0) {
					iParentBody = static_cast<int16_t>(piBoneToBody[iParent]);
					break;
				}
			}
			m_rbParentIndices[iBody] = iParentBody;
		}
	}
	m_bodyData.m_numRigidBodies = iNumBodies;
	m_bodyData.m_rigidBodies = a1->m_rigidBodies.begin();
	m_bodyData.m_parentIndices = m_rbParentIndices.begin();
	m_internalData.setSize(iNumBodies);
	hkaKeyFrameHierarchyUtility::initialize(m_bodyData, m_internalData.begin());
}

// The arrays free themselves; the reference on the ragdoll is the controller's own.
hkaRagdollRigidBodyController::~hkaRagdollRigidBodyController() {
	m_ragdollInstance->removeReference();
}

void hkaRagdollRigidBodyController::reinitialize() {
	hkaKeyFrameHierarchyUtility::initialize(m_bodyData, m_internalData.m_data);
}

const hkaRagdollInstance* hkaRagdollRigidBodyController::getRagdollInstance() const {
	return m_ragdollInstance;
}

void hkaRagdollRigidBodyController::setBoneWeights(float* boneWeights) {
	m_bodyData.m_boneWeights = boneWeights;
}

// Builds the key-frame data (worldFromModel, the internal reference pose) and the control data indices (only when there
// is one per rigid body) for applyKeyFrame. When the ragdoll has fewer rigid bodies than bones, the desired pose is
// first gathered per rigid body into a local array on the stack allocator.
void hkaRagdollRigidBodyController::driveToPose(float deltaTime, const hkQsTransform* poseLocal,
	const hkQsTransform& worldFromModel, hkaKeyFrameHierarchyUtility::Output* stressOut) {
	alignas(16) hkaKeyFrameHierarchyUtility::KeyFrameData keyFrameData;
	keyFrameData.m_internalReferencePose = m_internalData.m_data;
	keyFrameData.m_worldFromRoot.m_translation = worldFromModel.m_translation;
	const int32_t numRigidBodies = m_bodyData.m_numRigidBodies;
	keyFrameData.m_worldFromRoot.m_rotation = worldFromModel.m_rotation;
	keyFrameData.m_worldFromRoot.m_scale = worldFromModel.m_scale;
	m_bodyData.m_controlDataIndices =
		(m_bodyIndexToPaletteIndex.m_size == numRigidBodies) ? m_bodyIndexToPaletteIndex.m_data : nullptr;
	if (m_ragdollInstance->m_rigidBodies.m_size == m_ragdollInstance->m_skeleton->m_bones.m_size) {
		keyFrameData.m_desiredPoseLocalSpace = poseLocal;
		hkaKeyFrameHierarchyUtility::applyKeyFrame(deltaTime, keyFrameData, m_bodyData, m_controlDataPalette.m_data,
			stressOut);
	} else {
		// The stack allocator's bytes are untyped memory; the local array keeps its elements there.
		FNVD_INLINED_LOCAL(hkLocalArray<hkQsTransform>, poseLocalSpace, std::nullopt, std::nullopt);
		{
			// EXE-INLINED none: hkLocalArray<hkQsTransform>::hkLocalArray(numRigidBodies) (hkAllocateStack)
			poseLocalSpace.m_data = nullptr;
			poseLocalSpace.m_size = 0;
			poseLocalSpace.m_capacityAndFlags = hkArrayBase<hkQsTransform>::DONT_DEALLOCATE_FLAG;
			hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
			char* pBlock = stack.m_next.m_storage;
			stack.m_next.m_storage =
				pBlock + ((numRigidBodies * static_cast<int32_t>(sizeof(hkQsTransform)) + 127) & ~127);
			poseLocalSpace.m_data = reinterpret_cast<hkQsTransform*>(pBlock);
			poseLocalSpace.m_capacityAndFlags = numRigidBodies | hkArrayBase<hkQsTransform>::DONT_DEALLOCATE_FLAG;
			poseLocalSpace.m_localMemory = poseLocalSpace.m_data;
		}
		poseLocalSpace.setSizeUnchecked(numRigidBodies);
		for (int32_t i = 0; i < m_ragdollInstance->m_skeleton->m_bones.m_size; ++i) {
			const int32_t rb = m_ragdollInstance->m_boneToRigidBodyMap.m_data[i];
			if (rb >= 0) {
				// EXE-INLINED 0xC6CD30: hkQsTransform::operator= (three movaps copies)
				hkQsTransform& rDest = poseLocalSpace.m_data[rb];
				rDest.m_translation.m_quad = poseLocal[i].m_translation.m_quad;
				rDest.m_rotation.m_vec.m_quad = poseLocal[i].m_rotation.m_vec.m_quad;
				rDest.m_scale.m_quad = poseLocal[i].m_scale.m_quad;
			}
		}
		keyFrameData.m_desiredPoseLocalSpace = poseLocalSpace.m_data;
		hkaKeyFrameHierarchyUtility::applyKeyFrame(deltaTime, keyFrameData, m_bodyData, m_controlDataPalette.m_data,
			stressOut);
		poseLocalSpace.~hkLocalArray<hkQsTransform>();
	}
}
