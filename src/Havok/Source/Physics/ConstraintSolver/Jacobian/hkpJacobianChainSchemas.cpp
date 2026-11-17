#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianElement.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianPoweredChainSchema.h"

// The schema stream is raw memory the exe lays out by byte offsets, hence the casts.
hkp1Lin2AngJacobian* hkJacobianPoweredChainSchema_getLinearJacobians(hkpJacobianSchema* schema) {
	return reinterpret_cast<hkp1Lin2AngJacobian*>(reinterpret_cast<char*>(schema) +
		sizeof(hkpJacobianPoweredChainSchema));
}

hkp2AngJacobian* hkJacobianPoweredChainSchema_getAngularJacobians(hkpJacobianSchema* schema, int32_t numConstraints) {
	return reinterpret_cast<hkp2AngJacobian*>(hkJacobianPoweredChainSchema_getLinearJacobians(schema) +
		3 * numConstraints);
}

int32_t hkJacobianStiffSpringChainSchema_getJacobians(int32_t a1) {
	return a1 + 16;
}

int32_t hkJacobianBallSocketChainSchema_getJacobians(int32_t a1) {
	return a1 + 16;
}
