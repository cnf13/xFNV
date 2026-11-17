#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bilateral/hkp1dBilateralConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpParametricCurve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <xmmintrin.h>

using hkpPointToPathConstraintDataOrientationConstraintTypeEnumItemsArray = const hkClassEnum::Item[5];
static hkpPointToPathConstraintDataOrientationConstraintTypeEnumItemsArray
	hkpPointToPathConstraintDataOrientationConstraintTypeEnumItems{
		{hkpPointToPathConstraintData::CONSTRAIN_ORIENTATION_INVALID, "CONSTRAIN_ORIENTATION_INVALID"},
		{hkpPointToPathConstraintData::CONSTRAIN_ORIENTATION_NONE, "CONSTRAIN_ORIENTATION_NONE"},
		{hkpPointToPathConstraintData::CONSTRAIN_ORIENTATION_ALLOW_SPIN, "CONSTRAIN_ORIENTATION_ALLOW_SPIN"},
		{hkpPointToPathConstraintData::CONSTRAIN_ORIENTATION_TO_PATH, "CONSTRAIN_ORIENTATION_TO_PATH"},
		{hkpPointToPathConstraintData::CONSTRAIN_ORIENTATION_MAX_ID, "CONSTRAIN_ORIENTATION_MAX_ID"}};
using hkpPointToPathConstraintDataEnumsArray = const hkClassEnum[1];
static hkpPointToPathConstraintDataEnumsArray hkpPointToPathConstraintDataEnums{
	{"OrientationConstraintType", hkpPointToPathConstraintDataOrientationConstraintTypeEnumItems, 5, nullptr, {0}}};
// The OrientationConstraintType enum record, as the member table's angularConstrainedDOF row names it.
static const hkClassEnum* hkpPointToPathConstraintDataOrientationConstraintTypeEnum =
	&hkpPointToPathConstraintDataEnums[0];
// The angularConstrainedDOF row's enum is null in this table and is stored by an initialiser.
hkpPointToPathConstraintData::MembersArray hkpPointToPathConstraintData::Members{
	{"atoms", &hkpBridgeAtomsClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0, 0, 0xC,
		nullptr},
	{"path", &hkpParametricCurveClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x18,
		nullptr},
	{"maxFrictionForce", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x1C, nullptr},
	{"angularConstrainedDOF", nullptr, hkpPointToPathConstraintDataOrientationConstraintTypeEnum,
		{hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_INT8}, 0, 0, 0x20, nullptr},
	{"transform_OS_KS", nullptr, nullptr, {hkClassMember::TYPE_TRANSFORM}, {hkClassMember::TYPE_VOID}, 2, 0, 0x30,
		nullptr}};

hkClass hkpPointToPathConstraintDataClass("hkpPointToPathConstraintData", &hkpConstraintDataClass, 176, nullptr, 0,
	hkpPointToPathConstraintDataEnums, 1, hkpPointToPathConstraintData::Members, 5, nullptr, nullptr, 0, 0);
hkTypeInfo hkpPointToPathConstraintDataTypeInfo = {"hkpPointToPathConstraintData",
	finishLoadedObjecthkpPointToPathConstraintData, cleanupLoadedObjecthkpPointToPathConstraintData,
	getVtablehkpPointToPathConstraintData(), &typeid(hkpPointToPathConstraintData),
	sizeof(hkpPointToPathConstraintData)};

void cleanupLoadedObjecthkpPointToPathConstraintData(void* p) {
	static_cast<hkpPointToPathConstraintData*>(p)->~hkpPointToPathConstraintData();
}

// The type info's finish-loaded-object callback for hkpPointToPathConstraintData: runs its finish constructor.
void finishLoadedObjecthkpPointToPathConstraintData(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag aFlag;
		aFlag.m_finishing = finishing;
		::new (p) hkpPointToPathConstraintData(aFlag);
	}
}

const void* getVtablehkpPointToPathConstraintData() {
	return hkGetVtableOf<hkpPointToPathConstraintData>();
}

// The bridge atom has no constructor, so its type is set here. No path, no friction, identity pivot transforms; the
// bridge atom is tied to this data.
hkpPointToPathConstraintData::hkpPointToPathConstraintData() : m_path(nullptr), m_maxFrictionForce(0.0f) {
	m_atoms.m_bridgeAtom.m_type.m_storage = 1;
	m_angularConstrainedDOF.m_storage = 1;
	m_transform_OS_KS[0].m_rotation.m_col0.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[0].m_rotation.m_col1.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[0].m_rotation.m_col2.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[0].m_rotation.m_col0.m_quad[0] = 1.0f;
	m_transform_OS_KS[0].m_rotation.m_col1.m_quad[1] = 1.0f;
	m_transform_OS_KS[0].m_rotation.m_col2.m_quad[2] = 1.0f;
	m_transform_OS_KS[0].m_translation.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[1].m_rotation.m_col0.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[1].m_rotation.m_col1.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[1].m_rotation.m_col2.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[1].m_rotation.m_col0.m_quad[0] = 1.0f;
	m_transform_OS_KS[1].m_rotation.m_col1.m_quad[1] = 1.0f;
	m_transform_OS_KS[1].m_rotation.m_col2.m_quad[2] = 1.0f;
	m_transform_OS_KS[1].m_translation.m_quad = _mm_setzero_ps();
	m_atoms.m_bridgeAtom.init(this);
}

// Releases the path.
hkpPointToPathConstraintData::~hkpPointToPathConstraintData() {
	if (m_path) {
		m_path->removeReference();
	}
}

hkBool* hkpPointToPathConstraintData::isValid(hkBool* arResult) const {
	if (m_path && m_angularConstrainedDOF.m_storage) {
		arResult->m_bool = 1;
		return arResult;
	} else {
		arResult->m_bool = 0;
		return arResult;
	}
}

int32_t hkpPointToPathConstraintData::getType() const {
	return CONSTRAINT_TYPE_POINTTOPATH;
}

void hkpPointToPathConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
	(&infoOut)->m_numSolverResults = 7;
	(&infoOut)->m_sizeOfExternalRuntime = 60;
}

// Body A's pivot projected onto the path (in body B's frame) gives the parametric position kept in the runtime; then
// the friction along the tangent, two linear constraints across it, the angular constraints the orientation type asks
// for, and the limits at the path's ends when it is not a closed loop.
void hkpPointToPathConstraintData::buildJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) {
	hkBeginConstraints(&in, &out, static_cast<hkpSolverResults*>(in.m_constraintRuntime.m_storage), 8);
	hkTransform pathTransformWs;
	pathTransformWs.setMul(*in.m_transformB, m_transform_OS_KS[1]);
	hkTransform pivotTransformWs;
	pivotTransformWs.setMul(*in.m_transformA, m_transform_OS_KS[0]);
	Runtime* runtime = static_cast<Runtime*>(in.m_constraintRuntime.m_storage);

	// One path-space vector holds the point and then the tangent: the same vector is passed to getTangent.
	hkVector4 pathSpaceVector;
	pathSpaceVector.setTransformedInversePos(pathTransformWs, pivotTransformWs.m_translation);
	runtime->m_parametricPosition =
		m_path->getNearestPoint(runtime->m_parametricPosition, pathSpaceVector, pathSpaceVector);
	hkVector4 pointOnPathWs;
	pointOnPathWs.setTransformedPos(pathTransformWs, pathSpaceVector);
	m_path->getTangent(runtime->m_parametricPosition, pathSpaceVector);
	hkVector4 tangentWs;
	tangentWs.setRotatedDir(pathTransformWs.m_rotation, pathSpaceVector);

	// EXE-INLINED none: hkVector4Util::calculatePerpendicularVector
	float fMin = std::fabs(tangentWs(0));
	const float fAbsY = std::fabs(tangentWs(1));
	const float fAbsZ = std::fabs(tangentWs(2));
	int32_t iMin = 0;
	int32_t iFirst = 1;
	int32_t iSecond = 2;
	if (fMin > fAbsY) {
		fMin = fAbsY;
		iFirst = 0;
		iMin = 1;
	}
	if (fMin > fAbsZ) {
		iSecond = iMin;
	}
	hkVector4 perpWs;
	perpWs.m_quad = _mm_setzero_ps();
	perpWs(iFirst) = tangentWs(iSecond);
	perpWs(iSecond) = -tangentWs(iFirst);

	// EXE-INLINED 0x62AEA0
	const __m128 kPerpSquares = _mm_mul_ps(perpWs.m_quad, perpWs.m_quad);
	const __m128 kPerpLengthSquared = _mm_add_ps(_mm_shuffle_ps(kPerpSquares, kPerpSquares, 0xAA),
		_mm_add_ps(_mm_shuffle_ps(kPerpSquares, kPerpSquares, 0x55), kPerpSquares));
	const __m128 kEstimate = _mm_rsqrt_ss(kPerpLengthSquared);
	const __m128 kZeroMask = _mm_cmpeq_ss(kPerpLengthSquared, _mm_set_ss(0.0f));
	const __m128 kStep = _mm_sub_ps(_mm_set_ss(3.0f), _mm_mul_ps(_mm_mul_ps(kPerpLengthSquared, kEstimate), kEstimate));
	const __m128 kScale = _mm_andnot_ps(kZeroMask, _mm_mul_ps(_mm_mul_ps(_mm_set_ss(0.5f), kEstimate), kStep));
	perpWs.m_quad = _mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0), perpWs.m_quad);

	// EXE-INLINED 0x630DF0
	hkVector4 upWs;
	upWs.m_quad = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(tangentWs.m_quad, tangentWs.m_quad, 0xC9),
								 _mm_shuffle_ps(perpWs.m_quad, perpWs.m_quad, 0xD2)),
		_mm_mul_ps(_mm_shuffle_ps(tangentWs.m_quad, tangentWs.m_quad, 0xD2),
			_mm_shuffle_ps(perpWs.m_quad, perpWs.m_quad, 0xC9)));

	// Body A's pivot moved back along the tangent by the path length up to the parametric position.
	const float fNegLength = -m_path->getLengthFromStart(runtime->m_parametricPosition);
	hkVector4 pivotAlongPathWs;
	pivotAlongPathWs.m_quad =
		_mm_add_ps(_mm_mul_ps(_mm_set1_ps(fNegLength), tangentWs.m_quad), pivotTransformWs.m_translation.m_quad);

	if (m_maxFrictionForce > 0.0f) {
		hkp1dLinearFrictionInfo friction;
		friction.m_maxFrictionForce = m_maxFrictionForce;
		friction.m_constrainedDofW = tangentWs;
		friction.m_pivot = pivotAlongPathWs;
		friction.m_lastSolverResults = runtime->m_solverResults;
		hk1dLinearFrictionBuildJacobian(friction, in, out);
	}

	hkp1dLinearBilateralConstraintInfo linear;
	linear.m_pivotA = pivotTransformWs.m_translation;
	linear.m_pivotB = pointOnPathWs;
	linear.m_constrainedDofW = perpWs;
	hk1dLinearBilateralConstraintBuildJacobian(linear, in, out);
	linear.m_constrainedDofW = upWs;
	hk1dLinearBilateralConstraintBuildJacobian(linear, in, out);

	if (m_angularConstrainedDOF.m_storage > CONSTRAIN_ORIENTATION_NONE) {
		hkp1dAngularBilateralConstraintInfo angular;
		angular.m_zeroErrorAxisAinW = tangentWs;
		angular.m_constrainedDofW = pivotTransformWs.m_rotation.m_col2;
		angular.m_perpZeroErrorAxisBinW = pivotTransformWs.m_rotation.m_col1;
		hk1dAngularBilateralConstraintBuildJacobian(angular, in, out);
		angular.m_constrainedDofW = pivotTransformWs.m_rotation.m_col1;
		angular.m_perpZeroErrorAxisBinW.m_quad =
			_mm_xor_ps(pivotTransformWs.m_rotation.m_col2.m_quad, hkQuadRealNegateMask);
		hk1dAngularBilateralConstraintBuildJacobian(angular, in, out);
		if (m_angularConstrainedDOF.m_storage == CONSTRAIN_ORIENTATION_TO_PATH) {
			// The binormal goes into upWs and its world direction into perpWs, both of which are no longer needed at
			// this point.
			m_path->getBinormal(runtime->m_parametricPosition, upWs);
			perpWs.setRotatedDir(pathTransformWs.m_rotation, upWs);
			angular.m_zeroErrorAxisAinW = pivotTransformWs.m_rotation.m_col2;
			angular.m_constrainedDofW.m_quad = _mm_xor_ps(perpWs.m_quad, hkQuadRealNegateMask);
			angular.m_perpZeroErrorAxisBinW = pivotTransformWs.m_rotation.m_col0;
			hk1dAngularBilateralConstraintBuildJacobian(angular, in, out);
		}
	}

	hkBool closedLoop;
	if (!m_path->isClosedLoop(&closedLoop)->m_bool) {
		hkp1dLinearLimitInfo limit;
		limit.m_pivotA = pivotTransformWs.m_translation;
		limit.m_pivotB = pivotAlongPathWs;
		limit.m_min = m_path->getLengthFromStart(m_path->getStart());
		limit.m_max = m_path->getLengthFromStart(m_path->getEnd());
		limit.m_constrainedDofW = tangentWs;
		hk1dLinearLimitBuildJacobian(limit, in, out);
	}
}

void hkpPointToPathConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const {
	// The SDK's const_cast<hkpConstraintAtom*>(m_atoms.getAtoms()): the bridge atom heads the atoms.
	info.m_atoms = const_cast<hkpBridgeConstraintAtom*>(&m_atoms.m_bridgeAtom);
	info.m_sizeOfAllAtoms = sizeof(hkpBridgeAtoms);
	info.m_maxSizeOfSchema = 0;
	info.m_sizeOfSchemas = 384;
	info.m_numSolverResults = 7;
	info.m_numSolverElemTemps = 8;
}

void hkpPointToPathConstraintData::setPath(hkpParametricCurve* path) {
	if (path) {
		path->addReference();
	}
	hkReferencedObject* v3 = m_path;
	if (v3) {
		v3->removeReference();
	}
	m_path = path;
}

void hkpPointToPathConstraintData::setInWorldSpace(const hkTransform& ws_T_Aos, const hkTransform& ws_T_Ros,
	const hkVector4& pivotWs, hkpParametricCurve* path, const hkTransform& os_T_Rks) {
	hkVector4 v9;
	hkVector4 v10;
	hkVector4 v11;
	hkTransform v12;
	hkTransform v13;
	hkTransform a;
	hkTransform v15;
	if (path) {
		path->addReference();
	}
	hkpParametricCurve* m_path = this->m_path;
	if (m_path) {
		m_path->removeReference();
	}
	this->m_path = path;
	m_transform_OS_KS[1] = os_T_Rks;
	a.setMul(ws_T_Ros, os_T_Rks);
	v9.setTransformedInversePos(a, pivotWs);
	float v8 = this->m_path->getNearestPoint(0.0f, v9, v9);
	this->m_path->getTangent(v8, v11);
	this->m_path->getBinormal(v8, v10);
	v13.m_rotation.m_col1.m_quad = v10.m_quad;
	v13.m_rotation.m_col0.m_quad = v11.m_quad;
	v13.m_rotation.m_col2.m_quad =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v11.m_quad, v11.m_quad, 201), _mm_shuffle_ps(v10.m_quad, v10.m_quad, 210)),
			_mm_mul_ps(_mm_shuffle_ps(v11.m_quad, v11.m_quad, 210),
				_mm_shuffle_ps(v13.m_rotation.m_col1.m_quad, v13.m_rotation.m_col1.m_quad, 201)));
	v13.m_translation.m_quad = v9.m_quad;
	v15.setMul(os_T_Rks, v13);
	a.setMul(ws_T_Ros, v15);
	v12.setMulInverseMul(ws_T_Aos, a);
	m_transform_OS_KS[0] = v12;
}

void hkpPointToPathConstraintData::setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB,
	hkpParametricCurve* path) {
	if (path) {
		path->addReference();
	}
	hkpParametricCurve* m_path = this->m_path;
	if (m_path) {
		m_path->removeReference();
	}
	this->m_path = path;
	m_transform_OS_KS[0].m_rotation.m_col0.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[0].m_rotation.m_col1.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[0].m_rotation.m_col2.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[0].m_rotation.m_col0.m_quad[0] = 1.0f;
	m_transform_OS_KS[0].m_rotation.m_col1.m_quad[1] = 1.0f;
	m_transform_OS_KS[0].m_rotation.m_col2.m_quad[2] = 1.0f;
	m_transform_OS_KS[0].m_translation.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[1].m_rotation.m_col0.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[1].m_rotation.m_col1.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[1].m_rotation.m_col2.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[1].m_rotation.m_col0.m_quad[0] = 1.0f;
	m_transform_OS_KS[1].m_rotation.m_col1.m_quad[1] = 1.0f;
	m_transform_OS_KS[1].m_rotation.m_col2.m_quad[2] = 1.0f;
	m_transform_OS_KS[1].m_translation.m_quad = _mm_setzero_ps();
	m_transform_OS_KS[0].m_translation.m_quad = pivotA.m_quad;
	m_transform_OS_KS[1].m_translation.m_quad = pivotB.m_quad;
}

void hkpPointToPathConstraintData::calcPivot(const hkTransform& transformBodyA, hkVector4& pivotOut) const {
	pivotOut.setTransformedPos(transformBodyA, m_transform_OS_KS[0].m_translation);
}

const hkClass& hkpPointToPathConstraintData::staticClass() {
	return hkpPointToPathConstraintDataClass;
}
