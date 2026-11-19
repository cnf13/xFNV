#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"

#include "Havok/Source/Animation/Animation/Animation/DeltaCompressed/hkaDeltaCompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/Interleaved/hkaInterleavedUncompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/SplineCompressed/hkaSplineCompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/Util/hkaAnimationPreviewColorContainer.h"
#include "Havok/Source/Animation/Animation/Animation/WaveletCompressed/hkaWaveletCompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnimationBinding.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrack.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Animation/Animation/Deform/Skinning/hkaMeshBinding.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperClass.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperData.h"
#include "Havok/Source/Animation/Animation/Motion/Default/hkaDefaultAnimatedReferenceFrame.h"
#include "Havok/Source/Animation/Animation/Motion/hkaAnimatedReferenceFrame.h"
#include "Havok/Source/Animation/Animation/MotionAnalysis/hkaFootstepAnalysisInfo.h"
#include "Havok/Source/Animation/Animation/MotionAnalysis/hkaFootstepAnalysisInfoContainer.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBoneAttachment.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Animation/Animation/hkaAnimationContainer.h"
#include "Havok/Source/Animation/Ragdoll/Controller/RigidBody/hkaKeyFrameHierarchyUtility.h"
#include "Havok/Source/Animation/Ragdoll/Instance/hkaRagdollInstance.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/BitField/hkBitField.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/DebugUtil/TraceStream/hkTraceStream.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamAnalyzer.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamFrameInfo.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkDefaultClassNameRegistry.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/LocalFrame/hkLocalFrame.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPointMaterial.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/hkMeshShape.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/hkMeshVertexBuffer.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/SceneData/AlignScene/hkAlignSceneToNodeOptions.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedFloat.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedMatrix.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedQuaternion.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedVector.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroup.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxEnum.h"
#include "Havok/Source/Common/SceneData/Camera/hkxCamera.h"
#include "Havok/Source/Common/SceneData/Environment/hkxEnvironment.h"
#include "Havok/Source/Common/SceneData/Graph/hkxNode.h"
#include "Havok/Source/Common/SceneData/Light/hkxLight.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterial.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterialClass.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterialEffect.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterialShader.h"
#include "Havok/Source/Common/SceneData/Material/hkxTextureFile.h"
#include "Havok/Source/Common/SceneData/Material/hkxTextureInplace.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxEdgeSelectionChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxTriangleSelectionChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexFloatDataChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexIntDataChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexSelectionChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexVectorDataChannel.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxIndexBuffer.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxMesh.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxVertexBuffer.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxVertexDescription.h"
#include "Havok/Source/Common/SceneData/Scene/hkxScene.h"
#include "Havok/Source/Common/SceneData/Selection/hkxNodeSelectionSet.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileHeader.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileSectionHeader.h"
#include "Havok/Source/Common/Serialize/ResourceDatabase/hkResourceHandle.h"
#include "Havok/Source/Common/Serialize/Util/hkDynamicClassNameRegistry.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Common/Serialize/Util/hkRootLevelContainer.h"
#include "Havok/Source/Common/Serialize/Util/hkStaticClassNameRegistry.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Filter/DefaultConvexList/hkpDefaultConvexListFilter.h"
#include "Havok/Source/Physics/Collide/Filter/Group/hkpGroupFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpConvexListFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/CompressedMesh/hkpCompressedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShapeClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpNamedMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/SimpleMesh/hkpSimpleMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/StorageExtendedMesh/hkpStorageExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/StorageExtendedMesh/hkpStorageExtendedMeshShapeClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesConnectivity.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexList/hkpConvexListShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexPieceMesh/hkpConvexPieceMeshShapeClass.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/FastMesh/hkpFastMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/StorageMesh/hkpStorageMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/CompressedSampledHeightField/hkpCompressedSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/StorageSampledHeightField/hkpStorageSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpSphereRepShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/MultiRay/hkpMultiRayShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/PhantomCallback/hkpPhantomCallbackShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConeLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinSoftConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpOverwritePivotConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpPulleyConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpRagdollMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpStiffSpringConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpTwistLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpArrayActionClass.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpMotorAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Common/hkpMaterial.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpParametricCurve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPlane/hkpPointToPlaneConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Rotational/hkpRotationalConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Wheel/hkpWheelConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/BallSocket/hkpBallSocketChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/HingeLimits/hkpHingeLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/Powered/hkpPoweredChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/RagdollLimits/hkpRagdollLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/StiffSpring/hkpStiffSpringChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstanceAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintDataScheme.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Callback/hkpCallbackConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Pulley/hkpPulleyConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstanceClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpBreakableBody.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpCharacterMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpCachingShapePhantomClass.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/Memory/hkWorldMemoryAvailableWatchDog.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/Dashpot/hkpDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Actions/Reorient/hkpReorientAction.h"
#include "Havok/Source/Physics/Utilities/Actions/Spring/hkpSpringAction.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyCinfo.h"
#include "Havok/Source/Physics/Utilities/Collide/Filter/ConstrainedSystem/hkpConstrainedSystemFilter.h"
#include "Havok/Source/Physics/Utilities/Constraint/Chain/hkpPoweredChainMapper.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Deprecated/H1Group/hkpGroupCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpPhysicsSystemWithContacts.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntry.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntryClass.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarker.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarkerList.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayRbTransforms.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpHavokSnapshot.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpPhysicsData.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpBallGun.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpGravityGun.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpMountedBallGun.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpProjectileGun.h"
#include "Havok/Source/Physics/Vehicle/AeroDynamics/Default/hkpVehicleDefaultAerodynamics.h"
#include "Havok/Source/Physics/Vehicle/AeroDynamics/hkpVehicleAerodynamics.h"
#include "Havok/Source/Physics/Vehicle/Brake/Default/hkpVehicleDefaultBrake.h"
#include "Havok/Source/Physics/Vehicle/Brake/hkpVehicleBrake.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/Default/hkpVehicleDefaultAnalogDriverInput.h"
#include "Havok/Source/Physics/Vehicle/DriverInput/hkpVehicleDriverInput.h"
#include "Havok/Source/Physics/Vehicle/Engine/Default/hkpVehicleDefaultEngine.h"
#include "Havok/Source/Physics/Vehicle/Engine/hkpVehicleEngine.h"
#include "Havok/Source/Physics/Vehicle/Friction/hkpVehicleFriction.h"
#include "Havok/Source/Physics/Vehicle/Manager/LinearCastBatchingManager/hkpVehicleLinearCastBatchingManager.h"
#include "Havok/Source/Physics/Vehicle/Manager/RayCastBatchingManager/hkpVehicleRayCastBatchingManager.h"
#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleCastBatchingManager.h"
#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleManager.h"
#include "Havok/Source/Physics/Vehicle/Steering/Default/hkpVehicleDefaultSteering.h"
#include "Havok/Source/Physics/Vehicle/Steering/hkpVehicleSteering.h"
#include "Havok/Source/Physics/Vehicle/Suspension/Default/hkpVehicleDefaultSuspension.h"
#include "Havok/Source/Physics/Vehicle/Suspension/hkpVehicleSuspension.h"
#include "Havok/Source/Physics/Vehicle/Transmission/Default/hkpVehicleDefaultTransmission.h"
#include "Havok/Source/Physics/Vehicle/Transmission/hkpVehicleTransmission.h"
#include "Havok/Source/Physics/Vehicle/TyreMarks/hkpTyremarksInfo.h"
#include "Havok/Source/Physics/Vehicle/VelocityDamper/Default/hkpVehicleDefaultVelocityDamper.h"
#include "Havok/Source/Physics/Vehicle/VelocityDamper/hkpVehicleVelocityDamper.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/LinearCast/hkpVehicleLinearCastWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/RayCast/hkpVehicleRayCastWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/RejectChassisListener/hkpRejectChassisListener.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"
#include "SDK/MSVC/stdlib.h"

// Havok SDK signature (hkSerialize.lib, tied by bytes,data).
static hkReferencedObject* hkCreateBuiltInTypeRegistry();

// Every hkTypeInfo record linked into the executable, null terminated; hkDefaultBuiltinTypeRegistry:: init registers
// them.
hkBuiltinTypeRegistry::StaticLinkedTypeInfosArray hkBuiltinTypeRegistry::StaticLinkedTypeInfos{
    &hkBaseObjectTypeInfo,
    &hkLocalFrameGroupTypeInfo,
    &hkMonitorStreamColorTableTypeInfo,
    &hkOstreamTypeInfo,
    &hkReferencedObjectTypeInfo,
    &hkSimpleLocalFrameTypeInfo,
    &hkTraceStreamTypeInfo,
    &hkMemoryResourceContainerTypeInfo,
    &hkMemoryResourceHandleTypeInfo,
    &hkAlignSceneToNodeOptionsTypeInfo,
    &hkxAnimatedFloatTypeInfo,
    &hkxAnimatedMatrixTypeInfo,
    &hkxAnimatedQuaternionTypeInfo,
    &hkxAnimatedVectorTypeInfo,
    &hkxAttributeHolderTypeInfo,
    &hkxCameraTypeInfo,
    &hkxEdgeSelectionChannelTypeInfo,
    &hkxEnumTypeInfo,
    &hkxEnvironmentTypeInfo,
    &hkxIndexBufferTypeInfo,
    &hkxLightTypeInfo,
    &hkxMaterialTypeInfo,
    &hkxMaterialEffectTypeInfo,
    &hkxMaterialShaderTypeInfo,
    &hkxMaterialShaderSetTypeInfo,
    &hkxMeshTypeInfo,
    &hkxMeshSectionTypeInfo,
    &hkxMeshUserChannelInfoTypeInfo,
    &hkxNodeTypeInfo,
    &hkxNodeSelectionSetTypeInfo,
    &hkxSceneTypeInfo,
    &hkxSkinBindingTypeInfo,
    &hkxSparselyAnimatedBoolTypeInfo,
    &hkxSparselyAnimatedEnumTypeInfo,
    &hkxSparselyAnimatedIntTypeInfo,
    &hkxSparselyAnimatedStringTypeInfo,
    &hkxTextureFileTypeInfo,
    &hkxTextureInplaceTypeInfo,
    &hkxTriangleSelectionChannelTypeInfo,
    &hkxVertexBufferTypeInfo,
    &hkxVertexFloatDataChannelTypeInfo,
    &hkxVertexIntDataChannelTypeInfo,
    &hkxVertexSelectionChannelTypeInfo,
    &hkxVertexVectorDataChannelTypeInfo,
    &hkMemoryMeshShapeTypeInfo,
    &hkpBoxShapeTypeInfo,
    &hkpBvShapeTypeInfo,
    &hkpCapsuleShapeTypeInfo,
    &hkpCollisionFilterListTypeInfo,
    &hkpCompressedMeshShapeTypeInfo,
    &hkpCompressedSampledHeightFieldShapeTypeInfo,
    &hkpConvexListShapeTypeInfo,
    &hkpConvexPieceMeshShapeTypeInfo,
    &hkpConvexTransformShapeTypeInfo,
    &hkpConvexTranslateShapeTypeInfo,
    &hkpConvexVerticesConnectivityTypeInfo,
    &hkpConvexVerticesShapeTypeInfo,
    &hkpCylinderShapeTypeInfo,
    &hkpDefaultConvexListFilterTypeInfo,
    &hkpExtendedMeshShapeTypeInfo,
    &hkpFastMeshShapeTypeInfo,
    &hkpGroupFilterTypeInfo,
    &hkpListShapeTypeInfo,
    &hkpMeshShapeTypeInfo,
    &hkpMoppBvTreeShapeTypeInfo,
    &hkpMultiRayShapeTypeInfo,
    &hkpMultiSphereShapeTypeInfo,
    &hkpNullCollisionFilterTypeInfo,
    &hkpPlaneShapeTypeInfo,
    &hkpRemoveTerminalsMoppModifierTypeInfo,
    &hkpShapeInfoTypeInfo,
    &hkpSimpleMeshShapeTypeInfo,
    &hkpSingleShapeContainerTypeInfo,
    &hkpSphereShapeTypeInfo,
    &hkpStorageExtendedMeshShapeTypeInfo,
    &hkpStorageExtendedMeshShapeMeshSubpartStorageTypeInfo,
    &hkpStorageExtendedMeshShapeShapeSubpartStorageTypeInfo,
    &hkpStorageMeshShapeTypeInfo,
    &hkpStorageMeshShapeSubpartStorageTypeInfo,
    &hkpStorageSampledHeightFieldShapeTypeInfo,
    &hkpTransformShapeTypeInfo,
    &hkpTriSampledHeightFieldBvTreeShapeTypeInfo,
    &hkpTriSampledHeightFieldCollectionTypeInfo,
    &hkpTriangleShapeTypeInfo,
    &hkpAabbPhantomTypeInfo,
    &hkpBallAndSocketConstraintDataTypeInfo,
    &hkpBallSocketChainDataTypeInfo,
    &hkpBoxMotionTypeInfo,
    &hkpBreakableConstraintDataTypeInfo,
    &hkpCachingShapePhantomTypeInfo,
    &hkpCallbackConstraintMotorTypeInfo,
    &hkpCharacterMotionTypeInfo,
    &hkpConstraintChainInstanceTypeInfo,
    &hkpConstraintChainInstanceActionTypeInfo,
    &hkpConstraintInstanceTypeInfo,
    &hkpDefaultWorldMemoryWatchDogTypeInfo,
    &hkpEntityTypeInfo,
    &hkpFixedRigidMotionTypeInfo,
    &hkpGenericConstraintDataTypeInfo,
    &hkpHingeConstraintDataTypeInfo,
    &hkpHingeLimitsDataTypeInfo,
    &hkpKeyframedRigidMotionTypeInfo,
    &hkpLimitedHingeConstraintDataTypeInfo,
    &hkpLinearParametricCurveTypeInfo,
    &hkpMalleableConstraintDataTypeInfo,
    &hkpMaxSizeMotionTypeInfo,
    &hkpPhysicsSystemTypeInfo,
    &hkpPointToPathConstraintDataTypeInfo,
    &hkpPointToPlaneConstraintDataTypeInfo,
    &hkpPositionConstraintMotorTypeInfo,
    &hkpPoweredChainDataTypeInfo,
    &hkpPrismaticConstraintDataTypeInfo,
    &hkpPulleyConstraintDataTypeInfo,
    &hkpRagdollConstraintDataTypeInfo,
    &hkpRagdollLimitsDataTypeInfo,
    &hkpRigidBodyTypeInfo,
    &hkpRotationalConstraintDataTypeInfo,
    &hkpSimpleShapePhantomTypeInfo,
    &hkpSimulationTypeInfo,
    &hkpSphereMotionTypeInfo,
    &hkpSpringDamperConstraintMotorTypeInfo,
    &hkpStiffSpringChainDataTypeInfo,
    &hkpStiffSpringConstraintDataTypeInfo,
    &hkpThinBoxMotionTypeInfo,
    &hkpVelocityConstraintMotorTypeInfo,
    &hkpWheelConstraintDataTypeInfo,
    &hkpWorldTypeInfo,
    &hkpWorldCinfoTypeInfo,
    &hkpConvexPieceStreamDataTypeInfo,
    &hkpMoppCodeTypeInfo,
    &hkpAngularDashpotActionTypeInfo,
    &hkpBallGunTypeInfo,
    &hkpConstrainedSystemFilterTypeInfo,
    &hkpDashpotActionTypeInfo,
    &hkpDisableEntityCollisionFilterTypeInfo,
    &hkpDisplayBindingDataTypeInfo,
    &hkpDisplayBindingDataPhysicsSystemTypeInfo,
    &hkpDisplayBindingDataRigidBodyTypeInfo,
    &hkpFirstPersonGunTypeInfo,
    &hkpGravityGunTypeInfo,
    &hkpGroupCollisionFilterTypeInfo,
    &hkpMotorActionTypeInfo,
    &hkpMountedBallGunTypeInfo,
    &hkpMouseSpringActionTypeInfo,
    &hkpPhysicsDataTypeInfo,
    &hkpPhysicsSystemWithContactsTypeInfo,
    &hkpPoweredChainMapperTypeInfo,
    &hkpProjectileGunTypeInfo,
    &hkpReorientActionTypeInfo,
    &hkpSerializedAgentNnEntryTypeInfo,
    &hkpSerializedDisplayMarkerTypeInfo,
    &hkpSerializedDisplayMarkerListTypeInfo,
    &hkpSerializedDisplayRbTransformsTypeInfo,
    &hkpSpringActionTypeInfo,
    &hkpRejectChassisListenerTypeInfo,
    &hkpTyremarksInfoTypeInfo,
    &hkpTyremarksWheelTypeInfo,
    &hkpVehicleDataTypeInfo,
    &hkpVehicleDefaultAerodynamicsTypeInfo,
    &hkpVehicleDefaultAnalogDriverInputTypeInfo,
    &hkpVehicleDefaultBrakeTypeInfo,
    &hkpVehicleDefaultEngineTypeInfo,
    &hkpVehicleDefaultSteeringTypeInfo,
    &hkpVehicleDefaultSuspensionTypeInfo,
    &hkpVehicleDefaultTransmissionTypeInfo,
    &hkpVehicleDefaultVelocityDamperTypeInfo,
    &hkpVehicleDriverInputAnalogStatusTypeInfo,
    &hkpVehicleInstanceTypeInfo,
    &hkpVehicleLinearCastBatchingManagerTypeInfo,
    &hkpVehicleLinearCastWheelCollideTypeInfo,
    &hkpVehicleManagerTypeInfo,
    &hkpVehicleRayCastBatchingManagerTypeInfo,
    &hkpVehicleRayCastWheelCollideTypeInfo,
    &hkaAnimationBindingTypeInfo,
    &hkaAnimationContainerTypeInfo,
    &hkaAnimationPreviewColorContainerTypeInfo,
    &hkaBoneAttachmentTypeInfo,
    &hkaDefaultAnimatedReferenceFrameTypeInfo,
    &hkaDeltaCompressedAnimationTypeInfo,
    &hkaFootstepAnalysisInfoTypeInfo,
    &hkaFootstepAnalysisInfoContainerTypeInfo,
    &hkaInterleavedUncompressedAnimationTypeInfo,
    &hkaMeshBindingTypeInfo,
    &hkaSkeletonTypeInfo,
    &hkaSkeletonMapperTypeInfo,
    &hkaSplineCompressedAnimationTypeInfo,
    &hkaWaveletCompressedAnimationTypeInfo,
    &hkaRagdollInstanceTypeInfo,
    &hkAabbTypeInfo,
    &hkAabbUint32TypeInfo,
    &hkArrayTypeAttributeTypeInfo,
    &hkBitFieldTypeInfo,
    &hkClassTypeInfo,
    &hkClassEnumTypeInfo,
    &hkClassEnumItemTypeInfo,
    &hkClassMemberTypeInfo,
    &hkColorTypeInfo,
    &hkContactPointTypeInfo,
    &hkContactPointMaterialTypeInfo,
    &hkCustomAttributesTypeInfo,
    &hkCustomAttributesAttributeTypeInfo,
    &hkDataObjectTypeAttributeTypeInfo,
    &hkDescriptionAttributeTypeInfo,
    &hkDocumentationAttributeTypeInfo,
    &hkGeometryTypeInfo,
    &hkGeometryTriangleTypeInfo,
    &hkGizmoAttributeTypeInfo,
    &hkLinkAttributeTypeInfo,
    &hkModelerNodeTypeAttributeTypeInfo,
    &hkMonitorStreamColorTableColorPairTypeInfo,
    &hkMonitorStreamFrameInfoTypeInfo,
    &hkMonitorStreamStringMapTypeInfo,
    &hkMonitorStreamStringMapStringMapTypeInfo,
    &hkMotionStateTypeInfo,
    &hkMultiThreadCheckTypeInfo,
    &hkRangeInt32AttributeTypeInfo,
    &hkRangeRealAttributeTypeInfo,
    &hkSemanticsAttributeTypeInfo,
    &hkSphereTypeInfo,
    &hkSweptTransformTypeInfo,
    &hkTraceStreamTitleTypeInfo,
    &hkUiAttributeTypeInfo,
    &hkMemoryResourceHandleExternalLinkTypeInfo,
    &hkPackfileHeaderTypeInfo,
    &hkPackfileSectionHeaderTypeInfo,
    &hkRootLevelContainerTypeInfo,
    &hkRootLevelContainerNamedVariantTypeInfo,
    &hkxAttributeTypeInfo,
    &hkxAttributeGroupTypeInfo,
    &hkxEnumItemTypeInfo,
    &hkxEnvironmentVariableTypeInfo,
    &hkxMaterialTextureStageTypeInfo,
    &hkxNodeAnnotationDataTypeInfo,
    &hkxVertexBufferVertexDataTypeInfo,
    &hkxVertexDescriptionTypeInfo,
    &hkxVertexDescriptionElementDeclTypeInfo,
    &hkMeshSectionTypeInfo,
    &hkMeshSectionCinfoTypeInfo,
    &hkpAgent1nSectorTypeInfo,
    &hkpBroadPhaseHandleTypeInfo,
    &hkpCdBodyTypeInfo,
    &hkpCollidableTypeInfo,
    &hkpCollidableBoundingVolumeDataTypeInfo,
    &hkpCompressedMeshShapeBigTriangleTypeInfo,
    &hkpCompressedMeshShapeChunkTypeInfo,
    &hkpCompressedMeshShapeConvexPieceTypeInfo,
    &hkpConvexVerticesShapeFourVectorsTypeInfo,
    &hkpExtendedMeshShapeShapesSubpartTypeInfo,
    &hkpExtendedMeshShapeSubpartTypeInfo,
    &hkpExtendedMeshShapeTrianglesSubpartTypeInfo,
    &hkpLinkedCollidableTypeInfo,
    &hkpListShapeChildInfoTypeInfo,
    &hkpMeshMaterialTypeInfo,
    &hkpMeshShapeSubpartTypeInfo,
    &hkpMultiRayShapeRayTypeInfo,
    &hkpNamedMeshMaterialTypeInfo,
    &hkpSimpleMeshShapeTriangleTypeInfo,
    &hkpStorageExtendedMeshShapeMaterialTypeInfo,
    &hkpTypedBroadPhaseHandleTypeInfo,
    &hkpWeldingUtilityTypeInfo,
    &hkpBallAndSocketConstraintDataAtomsTypeInfo,
    &hkpBallSocketChainDataConstraintInfoTypeInfo,
    &hkpConstraintInstanceSmallArraySerializeOverrideTypeTypeInfo,
    &hkpEntityExtendedListenersTypeInfo,
    &hkpEntitySmallArraySerializeOverrideTypeTypeInfo,
    &hkpEntitySpuCollisionCallbackTypeInfo,
    &hkpGenericConstraintDataSchemeTypeInfo,
    &hkpGenericConstraintDataSchemeConstraintInfoTypeInfo,
    &hkpHingeConstraintDataAtomsTypeInfo,
    &hkpHingeLimitsDataAtomsTypeInfo,
    &hkpLimitedHingeConstraintDataAtomsTypeInfo,
    &hkpMaterialTypeInfo,
    &hkpPointToPlaneConstraintDataAtomsTypeInfo,
    &hkpPoweredChainDataConstraintInfoTypeInfo,
    &hkpPrismaticConstraintDataAtomsTypeInfo,
    &hkpPropertyTypeInfo,
    &hkpPropertyValueTypeInfo,
    &hkpPulleyConstraintDataAtomsTypeInfo,
    &hkpRagdollConstraintDataAtomsTypeInfo,
    &hkpRagdollLimitsDataAtomsTypeInfo,
    &hkpRotationalConstraintDataAtomsTypeInfo,
    &hkpSimpleShapePhantomCollisionDetailTypeInfo,
    &hkpStiffSpringChainDataConstraintInfoTypeInfo,
    &hkpStiffSpringConstraintDataAtomsTypeInfo,
    &hkpWheelConstraintDataAtomsTypeInfo,
    &hkp2dAngConstraintAtomTypeInfo,
    &hkpAngConstraintAtomTypeInfo,
    &hkpAngFrictionConstraintAtomTypeInfo,
    &hkpAngLimitConstraintAtomTypeInfo,
    &hkpAngMotorConstraintAtomTypeInfo,
    &hkpBallSocketConstraintAtomTypeInfo,
    &hkpBridgeAtomsTypeInfo,
    &hkpBridgeConstraintAtomTypeInfo,
    &hkpCenterOfMassChangerModifierConstraintAtomTypeInfo,
    &hkpConeLimitConstraintAtomTypeInfo,
    &hkpConstraintAtomTypeInfo,
    &hkpIgnoreModifierConstraintAtomTypeInfo,
    &hkpLinConstraintAtomTypeInfo,
    &hkpLinFrictionConstraintAtomTypeInfo,
    &hkpLinLimitConstraintAtomTypeInfo,
    &hkpLinMotorConstraintAtomTypeInfo,
    &hkpLinSoftConstraintAtomTypeInfo,
    &hkpMassChangerModifierConstraintAtomTypeInfo,
    &hkpModifierConstraintAtomTypeInfo,
    &hkpMovingSurfaceModifierConstraintAtomTypeInfo,
    &hkpOverwritePivotConstraintAtomTypeInfo,
    &hkpPulleyConstraintAtomTypeInfo,
    &hkpRagdollMotorConstraintAtomTypeInfo,
    &hkpSetLocalRotationsConstraintAtomTypeInfo,
    &hkpSetLocalTransformsConstraintAtomTypeInfo,
    &hkpSetLocalTranslationsConstraintAtomTypeInfo,
    &hkpSimpleContactConstraintAtomTypeInfo,
    &hkpSimpleContactConstraintDataInfoTypeInfo,
    &hkpSoftContactModifierConstraintAtomTypeInfo,
    &hkpStiffSpringConstraintAtomTypeInfo,
    &hkpTwistLimitConstraintAtomTypeInfo,
    &hkpViscousSurfaceModifierConstraintAtomTypeInfo,
    &hkpMoppCodeCodeInfoTypeInfo,
    &hkpMoppCodeReindexedTerminalTypeInfo,
    &hkpCharacterProxyCinfoTypeInfo,
    &hkpMassPropertiesTypeInfo,
    &hkpPoweredChainMapperLinkInfoTypeInfo,
    &hkpPoweredChainMapperTargetTypeInfo,
    &hkpSerializedDisplayRbTransformsDisplayTransformPairTypeInfo,
    &hkpSerializedSubTrack1nInfoTypeInfo,
    &hkpSerializedTrack1nInfoTypeInfo,
    &hkpTyremarkPointTypeInfo,
    &hkpVehicleDataWheelComponentParamsTypeInfo,
    &hkpVehicleDefaultBrakeWheelBrakingPropertiesTypeInfo,
    &hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersTypeInfo,
    &hkpVehicleFrictionDescriptionTypeInfo,
    &hkpVehicleFrictionDescriptionAxisDescriptionTypeInfo,
    &hkpVehicleFrictionStatusTypeInfo,
    &hkpVehicleFrictionStatusAxisStatusTypeInfo,
    &hkpVehicleInstanceWheelInfoTypeInfo,
    &hkpVehicleLinearCastWheelCollideWheelStateTypeInfo,
    &hkpVehicleSuspensionSuspensionWheelParametersTypeInfo,
    &hkaAnnotationTrackTypeInfo,
    &hkaAnnotationTrackAnnotationTypeInfo,
    &hkaBoneTypeInfo,
    &hkaDeltaCompressedAnimationQuantizationFormatTypeInfo,
    &hkaMeshBindingMappingTypeInfo,
    &hkaSkeletonLocalFrameOnBoneTypeInfo,
    &hkaSkeletonMapperDataTypeInfo,
    &hkaSkeletonMapperDataChainMappingTypeInfo,
    &hkaSkeletonMapperDataSimpleMappingTypeInfo,
    &hkaSplineCompressedAnimationAnimationCompressionParamsTypeInfo,
    &hkaSplineCompressedAnimationTrackCompressionParamsTypeInfo,
    &hkaWaveletCompressedAnimationCompressionParamsTypeInfo,
    &hkaWaveletCompressedAnimationQuantizationFormatTypeInfo,
    &hkaKeyFrameHierarchyUtilityTypeInfo,
    &hkaKeyFrameHierarchyUtilityControlDataTypeInfo,
    nullptr
};

// Every hkClass linked into the executable, null terminated; hkDefaultBuiltinTypeRegistry::init registers them with
// StaticLinkedTypeInfos, and hkHavokDefaultClassRegistry names them.
hkBuiltinTypeRegistry::StaticLinkedClassesArray hkBuiltinTypeRegistry::StaticLinkedClasses{
    &hkBaseObjectClass,
    &hkLocalFrameGroupClass,
    &hkMonitorStreamColorTableClass,
    &hkOstreamClass,
    &hkReferencedObjectClass,
    &hkSimpleLocalFrameClass,
    &hkTraceStreamClass,
    &hkMemoryResourceContainerClass,
    &hkMemoryResourceHandleClass,
    &hkAlignSceneToNodeOptionsClass,
    &hkxAnimatedFloatClass,
    &hkxAnimatedMatrixClass,
    &hkxAnimatedQuaternionClass,
    &hkxAnimatedVectorClass,
    &hkxAttributeHolderClass,
    &hkxCameraClass,
    &hkxEdgeSelectionChannelClass,
    &hkxEnumClass,
    &hkxEnvironmentClass,
    &hkxIndexBufferClass,
    &hkxLightClass,
    &hkxMaterialClass,
    &hkxMaterialEffectClass,
    &hkxMaterialShaderClass,
    &hkxMaterialShaderSetClass,
    &hkxMeshClass,
    &hkxMeshSectionClass,
    &hkxMeshUserChannelInfoClass,
    &hkxNodeClass,
    &hkxNodeSelectionSetClass,
    &hkxSceneClass,
    &hkxSkinBindingClass,
    &hkxSparselyAnimatedBoolClass,
    &hkxSparselyAnimatedEnumClass,
    &hkxSparselyAnimatedIntClass,
    &hkxSparselyAnimatedStringClass,
    &hkxTextureFileClass,
    &hkxTextureInplaceClass,
    &hkxTriangleSelectionChannelClass,
    &hkxVertexBufferClass,
    &hkxVertexFloatDataChannelClass,
    &hkxVertexIntDataChannelClass,
    &hkxVertexSelectionChannelClass,
    &hkxVertexVectorDataChannelClass,
    &hkMemoryMeshShapeClass,
    &hkpBoxShapeClass,
    &hkpBvShapeClass,
    &hkpCapsuleShapeClass,
    &hkpCollisionFilterListClass,
    &hkpCompressedMeshShapeClass,
    &hkpCompressedSampledHeightFieldShapeClass,
    &hkpConvexListShapeClass,
    &hkpConvexPieceMeshShapeClass,
    &hkpConvexTransformShapeClass,
    &hkpConvexTranslateShapeClass,
    &hkpConvexVerticesConnectivityClass,
    &hkpConvexVerticesShapeClass,
    &hkpCylinderShapeClass,
    &hkpDefaultConvexListFilterClass,
    &hkpExtendedMeshShapeClass,
    &hkpFastMeshShapeClass,
    &hkpGroupFilterClass,
    &hkpListShapeClass,
    &hkpMeshShapeClass,
    &hkpMoppBvTreeShapeClass,
    &hkpMultiRayShapeClass,
    &hkpMultiSphereShapeClass,
    &hkpNullCollisionFilterClass,
    &hkpPlaneShapeClass,
    &hkpRemoveTerminalsMoppModifierClass,
    &hkpShapeInfoClass,
    &hkpSimpleMeshShapeClass,
    &hkpSingleShapeContainerClass,
    &hkpSphereShapeClass,
    &hkpStorageExtendedMeshShapeClass,
    &hkpStorageExtendedMeshShapeMeshSubpartStorageClass,
    &hkpStorageExtendedMeshShapeShapeSubpartStorageClass,
    &hkpStorageMeshShapeClass,
    &hkpStorageMeshShapeSubpartStorageClass,
    &hkpStorageSampledHeightFieldShapeClass,
    &hkpTransformShapeClass,
    &hkpTriSampledHeightFieldBvTreeShapeClass,
    &hkpTriSampledHeightFieldCollectionClass,
    &hkpTriangleShapeClass,
    &hkpAabbPhantomClass,
    &hkpBallAndSocketConstraintDataClass,
    &hkpBallSocketChainDataClass,
    &hkpBoxMotionClass,
    &hkpBreakableConstraintDataClass,
    &hkpCachingShapePhantomClass,
    &hkpCallbackConstraintMotorClass,
    &hkpCharacterMotionClass,
    &hkpConstraintChainInstanceClass,
    &hkpConstraintChainInstanceActionClass,
    &hkpConstraintInstanceClass,
    &hkpDefaultWorldMemoryWatchDogClass,
    &hkpEntityClass,
    &hkpFixedRigidMotionClass,
    &hkpGenericConstraintDataClass,
    &hkpHingeConstraintDataClass,
    &hkpHingeLimitsDataClass,
    &hkpKeyframedRigidMotionClass,
    &hkpLimitedHingeConstraintDataClass,
    &hkpLinearParametricCurveClass,
    &hkpMalleableConstraintDataClass,
    &hkpMaxSizeMotionClass,
    &hkpPhysicsSystemClass,
    &hkpPointToPathConstraintDataClass,
    &hkpPointToPlaneConstraintDataClass,
    &hkpPositionConstraintMotorClass,
    &hkpPoweredChainDataClass,
    &hkpPrismaticConstraintDataClass,
    &hkpPulleyConstraintDataClass,
    &hkpRagdollConstraintDataClass,
    &hkpRagdollLimitsDataClass,
    &hkpRigidBodyClass,
    &hkpRotationalConstraintDataClass,
    &hkpSimpleShapePhantomClass,
    &hkpSimulationClass,
    &hkpSphereMotionClass,
    &hkpSpringDamperConstraintMotorClass,
    &hkpStiffSpringChainDataClass,
    &hkpStiffSpringConstraintDataClass,
    &hkpThinBoxMotionClass,
    &hkpVelocityConstraintMotorClass,
    &hkpWheelConstraintDataClass,
    &hkpWorldClass,
    &hkpWorldCinfoClass,
    &hkpConvexPieceStreamDataClass,
    &hkpMoppCodeClass,
    &hkpAngularDashpotActionClass,
    &hkpBallGunClass,
    &hkpConstrainedSystemFilterClass,
    &hkpDashpotActionClass,
    &hkpDisableEntityCollisionFilterClass,
    &hkpDisplayBindingDataClass,
    &hkpDisplayBindingDataPhysicsSystemClass,
    &hkpDisplayBindingDataRigidBodyClass,
    &hkpFirstPersonGunClass,
    &hkpGravityGunClass,
    &hkpGroupCollisionFilterClass,
    &hkpMotorActionClass,
    &hkpMountedBallGunClass,
    &hkpMouseSpringActionClass,
    &hkpPhysicsDataClass,
    &hkpPhysicsSystemWithContactsClass,
    &hkpPoweredChainMapperClass,
    &hkpProjectileGunClass,
    &hkpReorientActionClass,
    &hkpSerializedAgentNnEntryClass,
    &hkpSerializedDisplayMarkerClass,
    &hkpSerializedDisplayMarkerListClass,
    &hkpSerializedDisplayRbTransformsClass,
    &hkpSpringActionClass,
    &hkpRejectChassisListenerClass,
    &hkpTyremarksInfoClass,
    &hkpTyremarksWheelClass,
    &hkpVehicleDataClass,
    &hkpVehicleDefaultAerodynamicsClass,
    &hkpVehicleDefaultAnalogDriverInputClass,
    &hkpVehicleDefaultBrakeClass,
    &hkpVehicleDefaultEngineClass,
    &hkpVehicleDefaultSteeringClass,
    &hkpVehicleDefaultSuspensionClass,
    &hkpVehicleDefaultTransmissionClass,
    &hkpVehicleDefaultVelocityDamperClass,
    &hkpVehicleDriverInputAnalogStatusClass,
    &hkpVehicleInstanceClass,
    &hkpVehicleLinearCastBatchingManagerClass,
    &hkpVehicleLinearCastWheelCollideClass,
    &hkpVehicleManagerClass,
    &hkpVehicleRayCastBatchingManagerClass,
    &hkpVehicleRayCastWheelCollideClass,
    &hkaAnimationBindingClass,
    &hkaAnimationContainerClass,
    &hkaAnimationPreviewColorContainerClass,
    &hkaBoneAttachmentClass,
    &hkaDefaultAnimatedReferenceFrameClass,
    &hkaDeltaCompressedAnimationClass,
    &hkaFootstepAnalysisInfoClass,
    &hkaFootstepAnalysisInfoContainerClass,
    &hkaInterleavedUncompressedAnimationClass,
    &hkaMeshBindingClass,
    &hkaSkeletonClass,
    &hkaSkeletonMapperClass,
    &hkaSplineCompressedAnimationClass,
    &hkaWaveletCompressedAnimationClass,
    &hkaRagdollInstanceClass,
    &hkAabbClass,
    &hkAabbUint32Class,
    &hkArrayTypeAttributeClass,
    &hkBitFieldClass,
    &hkClassClass,
    &hkClassEnumClass,
    &hkClassEnumItemClass,
    &hkClassMemberClass,
    &hkColorClass,
    &hkContactPointClass,
    &hkContactPointMaterialClass,
    &hkCustomAttributesClass,
    &hkCustomAttributesAttributeClass,
    &hkDataObjectTypeAttributeClass,
    &hkDescriptionAttributeClass,
    &hkDocumentationAttributeClass,
    &hkGeometryClass,
    &hkGeometryTriangleClass,
    &hkGizmoAttributeClass,
    &hkLinkAttributeClass,
    &hkModelerNodeTypeAttributeClass,
    &hkMonitorStreamColorTableColorPairClass,
    &hkMonitorStreamFrameInfoClass,
    &hkMonitorStreamStringMapClass,
    &hkMonitorStreamStringMapStringMapClass,
    &hkMotionStateClass,
    &hkMultiThreadCheckClass,
    &hkRangeInt32AttributeClass,
    &hkRangeRealAttributeClass,
    &hkSemanticsAttributeClass,
    &hkSphereClass,
    &hkSweptTransformClass,
    &hkTraceStreamTitleClass,
    &hkUiAttributeClass,
    &hkMemoryResourceHandleExternalLinkClass,
    &hkPackfileHeaderClass,
    &hkPackfileSectionHeaderClass,
    &hkRootLevelContainerClass,
    &hkRootLevelContainerNamedVariantClass,
    &hkxAttributeClass,
    &hkxAttributeGroupClass,
    &hkxEnumItemClass,
    &hkxEnvironmentVariableClass,
    &hkxMaterialTextureStageClass,
    &hkxNodeAnnotationDataClass,
    &hkxVertexBufferVertexDataClass,
    &hkxVertexDescriptionClass,
    &hkxVertexDescriptionElementDeclClass,
    &hkMeshSectionClass,
    &hkMeshSectionCinfoClass,
    &hkpAgent1nSectorClass,
    &hkpBroadPhaseHandleClass,
    &hkpCdBodyClass,
    &hkpCollidableClass,
    &hkpCollidableBoundingVolumeDataClass,
    &hkpCompressedMeshShapeBigTriangleClass,
    &hkpCompressedMeshShapeChunkClass,
    &hkpCompressedMeshShapeConvexPieceClass,
    &hkpConvexVerticesShapeFourVectorsClass,
    &hkpExtendedMeshShapeShapesSubpartClass,
    &hkpExtendedMeshShapeSubpartClass,
    &hkpExtendedMeshShapeTrianglesSubpartClass,
    &hkpLinkedCollidableClass,
    &hkpListShapeChildInfoClass,
    &hkpMeshMaterialClass,
    &hkpMeshShapeSubpartClass,
    &hkpMultiRayShapeRayClass,
    &hkpNamedMeshMaterialClass,
    &hkpSimpleMeshShapeTriangleClass,
    &hkpStorageExtendedMeshShapeMaterialClass,
    &hkpTypedBroadPhaseHandleClass,
    &hkpWeldingUtilityClass,
    &hkpBallAndSocketConstraintDataAtomsClass,
    &hkpBallSocketChainDataConstraintInfoClass,
    &hkpConstraintInstanceSmallArraySerializeOverrideTypeClass,
    &hkpEntityExtendedListenersClass,
    &hkpEntitySmallArraySerializeOverrideTypeClass,
    &hkpEntitySpuCollisionCallbackClass,
    &hkpGenericConstraintDataSchemeClass,
    &hkpGenericConstraintDataSchemeConstraintInfoClass,
    &hkpHingeConstraintDataAtomsClass,
    &hkpHingeLimitsDataAtomsClass,
    &hkpLimitedHingeConstraintDataAtomsClass,
    &hkpMaterialClass,
    &hkpPointToPlaneConstraintDataAtomsClass,
    &hkpPoweredChainDataConstraintInfoClass,
    &hkpPrismaticConstraintDataAtomsClass,
    &hkpPropertyClass,
    &hkpPropertyValueClass,
    &hkpPulleyConstraintDataAtomsClass,
    &hkpRagdollConstraintDataAtomsClass,
    &hkpRagdollLimitsDataAtomsClass,
    &hkpRotationalConstraintDataAtomsClass,
    &hkpSimpleShapePhantomCollisionDetailClass,
    &hkpStiffSpringChainDataConstraintInfoClass,
    &hkpStiffSpringConstraintDataAtomsClass,
    &hkpWheelConstraintDataAtomsClass,
    &hkp2dAngConstraintAtomClass,
    &hkpAngConstraintAtomClass,
    &hkpAngFrictionConstraintAtomClass,
    &hkpAngLimitConstraintAtomClass,
    &hkpAngMotorConstraintAtomClass,
    &hkpBallSocketConstraintAtomClass,
    &hkpBridgeAtomsClass,
    &hkpBridgeConstraintAtomClass,
    &hkpCenterOfMassChangerModifierConstraintAtomClass,
    &hkpConeLimitConstraintAtomClass,
    &hkpConstraintAtomClass,
    &hkpIgnoreModifierConstraintAtomClass,
    &hkpLinConstraintAtomClass,
    &hkpLinFrictionConstraintAtomClass,
    &hkpLinLimitConstraintAtomClass,
    &hkpLinMotorConstraintAtomClass,
    &hkpLinSoftConstraintAtomClass,
    &hkpMassChangerModifierConstraintAtomClass,
    &hkpModifierConstraintAtomClass,
    &hkpMovingSurfaceModifierConstraintAtomClass,
    &hkpOverwritePivotConstraintAtomClass,
    &hkpPulleyConstraintAtomClass,
    &hkpRagdollMotorConstraintAtomClass,
    &hkpSetLocalRotationsConstraintAtomClass,
    &hkpSetLocalTransformsConstraintAtomClass,
    &hkpSetLocalTranslationsConstraintAtomClass,
    &hkpSimpleContactConstraintAtomClass,
    &hkpSimpleContactConstraintDataInfoClass,
    &hkpSoftContactModifierConstraintAtomClass,
    &hkpStiffSpringConstraintAtomClass,
    &hkpTwistLimitConstraintAtomClass,
    &hkpViscousSurfaceModifierConstraintAtomClass,
    &hkpMoppCodeCodeInfoClass,
    &hkpMoppCodeReindexedTerminalClass,
    &hkpCharacterProxyCinfoClass,
    &hkpMassPropertiesClass,
    &hkpPoweredChainMapperLinkInfoClass,
    &hkpPoweredChainMapperTargetClass,
    &hkpSerializedDisplayRbTransformsDisplayTransformPairClass,
    &hkpSerializedSubTrack1nInfoClass,
    &hkpSerializedTrack1nInfoClass,
    &hkpTyremarkPointClass,
    &hkpVehicleDataWheelComponentParamsClass,
    &hkpVehicleDefaultBrakeWheelBrakingPropertiesClass,
    &hkpVehicleDefaultSuspensionWheelSpringSuspensionParametersClass,
    &hkpVehicleFrictionDescriptionClass,
    &hkpVehicleFrictionDescriptionAxisDescriptionClass,
    &hkpVehicleFrictionStatusClass,
    &hkpVehicleFrictionStatusAxisStatusClass,
    &hkpVehicleInstanceWheelInfoClass,
    &hkpVehicleLinearCastWheelCollideWheelStateClass,
    &hkpVehicleSuspensionSuspensionWheelParametersClass,
    &hkaAnnotationTrackClass,
    &hkaAnnotationTrackAnnotationClass,
    &hkaBoneClass,
    &hkaDeltaCompressedAnimationQuantizationFormatClass,
    &hkaMeshBindingMappingClass,
    &hkaSkeletonLocalFrameOnBoneClass,
    &hkaSkeletonMapperDataClass,
    &hkaSkeletonMapperDataChainMappingClass,
    &hkaSkeletonMapperDataSimpleMappingClass,
    &hkaSplineCompressedAnimationAnimationCompressionParamsClass,
    &hkaSplineCompressedAnimationTrackCompressionParamsClass,
    &hkaWaveletCompressedAnimationCompressionParamsClass,
    &hkaWaveletCompressedAnimationQuantizationFormatClass,
    &hkaKeyFrameHierarchyUtilityClass,
    &hkaKeyFrameHierarchyUtilityControlDataClass,
    &hkLocalFrameClass,
    &hkStreamWriterClass,
    &hkResourceBaseClass,
    &hkResourceContainerClass,
    &hkResourceHandleClass,
    &hkMeshMaterialClass,
    &hkMeshShapeClass,
    &hkMeshVertexBufferClass,
    &hkMoppBvTreeShapeBaseClass,
    &hkpBvTreeShapeClass,
    &hkpCollidableCollidableFilterClass,
    &hkpCollisionFilterClass,
    &hkpConvexListFilterClass,
    &hkpConvexShapeClass,
    &hkpConvexTransformShapeBaseClass,
    &hkpHeightFieldShapeClass,
    &hkpPhantomCallbackShapeClass,
    &hkpRayCollidableFilterClass,
    &hkpRayShapeCollectionFilterClass,
    &hkpSampledHeightFieldShapeClass,
    &hkpShapeClass,
    &hkpShapeCollectionClass,
    &hkpShapeCollectionFilterClass,
    &hkpShapeContainerClass,
    &hkpSphereRepShapeClass,
    &hkWorldMemoryAvailableWatchDogClass,
    &hkpActionClass,
    &hkpArrayActionClass,
    &hkpBinaryActionClass,
    &hkpBreakableBodyClass,
    &hkpConstraintChainDataClass,
    &hkpConstraintDataClass,
    &hkpConstraintMotorClass,
    &hkpLimitedForceConstraintMotorClass,
    &hkpMotionClass,
    &hkpParametricCurveClass,
    &hkpPhantomClass,
    &hkpShapePhantomClass,
    &hkpUnaryActionClass,
    &hkpWorldObjectClass,
    &hkpVehicleAerodynamicsClass,
    &hkpVehicleBrakeClass,
    &hkpVehicleCastBatchingManagerClass,
    &hkpVehicleDriverInputClass,
    &hkpVehicleDriverInputStatusClass,
    &hkpVehicleEngineClass,
    &hkpVehicleSteeringClass,
    &hkpVehicleSuspensionClass,
    &hkpVehicleTransmissionClass,
    &hkpVehicleVelocityDamperClass,
    &hkpVehicleWheelCollideClass,
    &hkaAnimatedReferenceFrameClass,
    &hkaAnimationClass,
    nullptr
};
hkStaticClassNameRegistry hkHavokCurrentClasses::hkHavokDefaultClassRegistry(
    hkBuiltinTypeRegistry::StaticLinkedClasses,
    -1,
    "Havok-7.1.0-r1"
);

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkBuiltinTypeRegistry(
    "hkBuiltinTypeRegistry",
    &hkCreateBuiltInTypeRegistry,
    reinterpret_cast<void**>(&hkBuiltinTypeRegistry::s_instance)
);

static hkReferencedObject* hkCreateBuiltInTypeRegistry() {
    if (!hkVtableClassRegistry::s_instance ||
        (!hkDefaultClassNameRegistry::s_instance || !hkTypeInfoRegistry::s_instance)) {
        return nullptr;
    }
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkDefaultBuiltinTypeRegistry* pRegistry =
        static_cast<hkDefaultBuiltinTypeRegistry*>(Instance->m_heap->blockAlloc(sizeof(hkDefaultBuiltinTypeRegistry)));
    pRegistry->m_memSizeAndFlags = sizeof(hkDefaultBuiltinTypeRegistry);
    return ::new (pRegistry) hkDefaultBuiltinTypeRegistry;
}

// The SDK's addType: the class goes into the class name registry under its name, the type info into the type info
// registry, and a class with interfaces (a vtable) into the vtable registry, each through the registry's virtual.
void hkBuiltinTypeRegistry::addType(const hkTypeInfo* info, const hkClass* klass) {
    hkDefaultClassNameRegistry::s_instance->registerClass(klass, klass->getName());
    hkTypeInfoRegistry::s_instance->registerTypeInfo(info);
    if (klass->getNumInterfaces() > 0) {
        hkVtableClassRegistry::s_instance->registerVtable(info->m_vtable, klass);
    }
}

hkDefaultBuiltinTypeRegistry::hkDefaultBuiltinTypeRegistry() {
    init();
}

hkTypeInfoRegistry* hkDefaultBuiltinTypeRegistry::getTypeInfoRegistry() {
    return hkTypeInfoRegistry::s_instance;
}

hkClassNameRegistry* hkDefaultBuiltinTypeRegistry::getClassNameRegistry() {
    return hkDefaultClassNameRegistry::s_instance;
}

hkVtableClassRegistry* hkDefaultBuiltinTypeRegistry::getVtableClassRegistry() {
    return hkVtableClassRegistry::s_instance;
}

// A new class name registry, type info registry and vtable registry each replace their singleton's instance (the old
// one loses a reference), then init is called.
void hkDefaultBuiltinTypeRegistry::reinitialize() {
    // EXE-INLINED 0x56D280
    hkDefaultClassNameRegistry* pClassReg = static_cast<hkDefaultClassNameRegistry*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkDefaultClassNameRegistry))
    );
    pClassReg->m_memSizeAndFlags = sizeof(hkDefaultClassNameRegistry);
    ::new (pClassReg) hkDefaultClassNameRegistry;
    // EXE-INLINED none: hkSingleton<hkDefaultClassNameRegistry>::replaceInstance
    if (hkDefaultClassNameRegistry::s_instance) {
        hkDefaultClassNameRegistry::s_instance->removeReferenceLockUnchecked();
    }
    hkDefaultClassNameRegistry::s_instance = pClassReg;

    // EXE-INLINED 0x56D280
    hkTypeInfoRegistry* pTypeReg =
        static_cast<hkTypeInfoRegistry*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkTypeInfoRegistry)));
    pTypeReg->m_memSizeAndFlags = sizeof(hkTypeInfoRegistry);
    ::new (pTypeReg) hkTypeInfoRegistry;
    // EXE-INLINED none: hkSingleton<hkTypeInfoRegistry>::replaceInstance
    if (hkTypeInfoRegistry::s_instance) {
        hkTypeInfoRegistry::s_instance->removeReferenceLockUnchecked();
    }
    hkTypeInfoRegistry::s_instance = pTypeReg;

    // EXE-INLINED 0x56D280
    hkVtableClassRegistry* pVtableReg = static_cast<hkVtableClassRegistry*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkVtableClassRegistry))
    );
    pVtableReg->m_memSizeAndFlags = sizeof(hkVtableClassRegistry);
    ::new (pVtableReg) hkVtableClassRegistry;
    // EXE-INLINED none: hkSingleton<hkVtableClassRegistry>::replaceInstance
    if (hkVtableClassRegistry::s_instance) {
        hkVtableClassRegistry::s_instance->removeReferenceLockUnchecked();
    }
    hkVtableClassRegistry::s_instance = pVtableReg;
    init();
}

// Merges the built-in classes into the class name registry, registers every built-in type info, then the vtables of the
// classes that have one. The type info registry's instance is read once, before the loop.
void hkDefaultBuiltinTypeRegistry::init() {
    hkDefaultClassNameRegistry::s_instance->merge(hkHavokCurrentClasses::hkHavokDefaultClassRegistry);
    hkTypeInfoRegistry* pTypeInfoRegistry = hkTypeInfoRegistry::s_instance;
    for (const hkTypeInfo* const* ppInfo = StaticLinkedTypeInfos; *ppInfo; ++ppInfo) {
        pTypeInfoRegistry->registerTypeInfo(*ppInfo);
    }
    hkVtableClassRegistry::s_instance->registerList(StaticLinkedTypeInfos, StaticLinkedClasses);
}

// The string map goes with its own destructor.
hkTypeInfoRegistry::~hkTypeInfoRegistry() {}

// Files info under its type name.
void hkTypeInfoRegistry::registerTypeInfo(const hkTypeInfo* info) {
    m_map.insert(info->m_typeName, info);
}

// Finishes obj through the type info filed under className, when the info has a finish function, with the registry's
// flag; returns the info, or null.
const hkTypeInfo* hkTypeInfoRegistry::finishLoadedObject(void* obj, const char* className) const {
    const hkTypeInfo* pInfo = m_map.getWithDefault(className, nullptr);
    if (!pInfo) {
        return nullptr;
    }
    if (pInfo->m_finishLoadedObjectFunction) {
        pInfo->m_finishLoadedObjectFunction(obj, m_finishFlag.m_finishing);
    }
    return pInfo;
}

const hkTypeInfo* hkTypeInfoRegistry::cleanupLoadedObject(void* obj, const char* className) const {
    const hkTypeInfo* result = m_map.getWithDefault(className, 0);
    if (result) {
        void (*m_cleanupLoadedObjectFunction)(void*) = result->m_cleanupLoadedObjectFunction;
        if (m_cleanupLoadedObjectFunction) {
            m_cleanupLoadedObjectFunction(obj);
        }
        return result;
    }
    return result;
}

// Files every type info of mergeFrom here under its own name (the value is read before the key, as the game does).
void hkTypeInfoRegistry::merge(const hkTypeInfoRegistry& mergeFrom) {
    const hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>& kFrom = mergeFrom.m_map.m_map;
    hkBool bValid;
    Dummy* it = kFrom.getIterator();
    bValid = kFrom.isValid(it);
    while (bValid.m_bool) {
        hkUlong value = kFrom.getValue(it);
        hkUlong key = kFrom.getKey(it);
        m_map.m_map.insert(key, value);
        it = kFrom.getNext(it);
        bValid = kFrom.isValid(it);
    }
}

// The type info filed under typeName, or null.
const hkTypeInfo* hkTypeInfoRegistry::getTypeInfo(const char* typeName) const {
    return m_map.getWithDefault(typeName, nullptr);
}

// Appends the class of every used slot of the map to classes; an empty slot holds the key -1.
void hkVtableClassRegistry::getClasses(hkArray<const hkClass*, hkContainerHeapAllocator>& classes) const {
    int32_t m_hashMod = m_map.m_map.m_hashMod;
    int32_t m_hashMod_1 = 0;
    if (m_hashMod >= 0) {
        hkUlongMapBase::Pair* m_elem = m_map.m_map.m_elem;
        do {
            if (m_elem->key != -1) {
                break;
            }
            ++m_hashMod_1;
            ++m_elem;
        }
        while (m_hashMod_1 <= m_hashMod);
    }
    if (m_hashMod_1 <= m_hashMod) {
        int32_t m_hashMod_2;
        int32_t m_hashMod_3;
        do {
            const hkClass* val = m_map.FromMapInt<const hkClass*>(m_map.m_map.m_elem[m_hashMod_1].val);
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            if (classes.m_size == (classes.m_capacityAndFlags & hkArrayBase<const hkClass*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*Instance->m_heap, &classes, sizeof(const hkClass*));
            }
            classes.m_data[classes.m_size++] = val;
            m_hashMod_2 = m_map.m_map.m_hashMod;
            m_hashMod_3 = m_hashMod_1 + 1;
            if (m_hashMod_1 + 1 <= m_hashMod_2) {
                hkUlongMapBase::Pair* v10 = &m_map.m_map.m_elem[m_hashMod_3];
                do {
                    if (v10->key != -1) {
                        break;
                    }
                    ++m_hashMod_3;
                    ++v10;
                }
                while (m_hashMod_3 <= m_hashMod_2);
            }
            m_hashMod_1 = m_hashMod_3;
        }
        while (m_hashMod_3 <= m_hashMod_2);
    }
}

hkDynamicClassNameRegistry::~hkDynamicClassNameRegistry() {}

const hkClass* hkDynamicClassNameRegistry::getClassByName(const char* className) const {
    return m_map.getWithDefault(className, nullptr);
}

// Appends every class of the map to classes.
void hkDynamicClassNameRegistry::getClasses(hkArray<const hkClass*, hkContainerHeapAllocator>& classes) const {
    hkBool bValid;
    Dummy* it = m_map.m_map.getIterator();
    bValid = m_map.m_map.isValid(it);
    while (bValid.m_bool) {
        const hkClass* pClass = m_map.getValue(it);
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if (classes.m_size == (classes.m_capacityAndFlags & hkArrayBase<const hkClass*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, &classes, sizeof(const hkClass*));
        }
        int32_t m_size = classes.m_size;
        const hkClass** ppSlot = &classes.m_data[m_size];
        classes.m_size = m_size + 1;
        *ppSlot = pClass;
        it = m_map.m_map.getNext(it);
        bValid = m_map.m_map.isValid(it);
    }
}

// Files klass under name, or under the class's own name when name is null.
void hkDynamicClassNameRegistry::registerClass(const hkClass* klass, const char* name) {
    if (!name) {
        name = klass->getName();
    }
    m_map.insert(name, klass);
}

// Registers each class of the null-terminated list under its own name, through the virtual registerClass.
void hkDynamicClassNameRegistry::registerList(const hkClass* const* classes) {
    for (const hkClass* const* ci = classes; *ci != nullptr; ++ci) {
        registerClass(*ci, nullptr);
    }
}

// The string map overload: registerClass(class, nullptr) for each class of classes, walked through its hkCachedHashMap.
void hkDynamicClassNameRegistry::merge(const hkStringMap<const hkClass*, hkContainerHeapAllocator>& classes) {
    hkBool bValid;
    Dummy* it = classes.m_map.getIterator();
    bValid = classes.m_map.isValid(it);
    while (bValid.m_bool) {
        registerClass(classes.getValue(it), nullptr);
        it = classes.m_map.getNext(it);
        bValid = classes.m_map.isValid(it);
    }
}

// Registers every class of mergeFrom under its own name.
void hkDynamicClassNameRegistry::merge(const hkClassNameRegistry& mergeFrom) {
    hkArray<const hkClass*> classes;
    mergeFrom.getClasses(classes);
    for (const hkClass* pClass : classes) {
        registerClass(pClass, nullptr);
    }
}
