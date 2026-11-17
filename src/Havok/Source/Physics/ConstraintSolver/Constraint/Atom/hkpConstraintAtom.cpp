#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/ConstraintSolver/SimpleConstraints/hkpSimpleConstraintUtil.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"

#include <xmmintrin.h>

static void cleanupLoadedObjecthkpConstraintAtom(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpConstraintAtomTypeInfo = {"hkpConstraintAtom", finishLoadedObjecthkpConstraintAtom,
	cleanupLoadedObjecthkpConstraintAtom, nullptr, &typeid(hkpConstraintAtom), sizeof(hkpConstraintAtom)};
hkTypeInfo hkpSimpleContactConstraintAtomTypeInfo = {"hkpSimpleContactConstraintAtom",
	finishLoadedObjecthkpSimpleContactConstraintAtom, cleanupLoadedObjecthkpSimpleContactConstraintAtom, nullptr,
	&typeid(hkpSimpleContactConstraintAtom), sizeof(hkpSimpleContactConstraintAtom)};
hkTypeInfo hkpModifierConstraintAtomTypeInfo = {"hkpModifierConstraintAtom",
	finishLoadedObjecthkpModifierConstraintAtom, cleanupLoadedObjecthkpModifierConstraintAtom, nullptr,
	&typeid(hkpModifierConstraintAtom), sizeof(hkpModifierConstraintAtom)};
hkTypeInfo hkpSoftContactModifierConstraintAtomTypeInfo = {"hkpSoftContactModifierConstraintAtom",
	finishLoadedObjecthkpSoftContactModifierConstraintAtom, cleanupLoadedObjecthkpSoftContactModifierConstraintAtom,
	nullptr, &typeid(hkpSoftContactModifierConstraintAtom), sizeof(hkpSoftContactModifierConstraintAtom)};
hkTypeInfo hkpMassChangerModifierConstraintAtomTypeInfo = {"hkpMassChangerModifierConstraintAtom",
	finishLoadedObjecthkpMassChangerModifierConstraintAtom, cleanupLoadedObjecthkpMassChangerModifierConstraintAtom,
	nullptr, &typeid(hkpMassChangerModifierConstraintAtom), sizeof(hkpMassChangerModifierConstraintAtom)};
hkTypeInfo hkpCenterOfMassChangerModifierConstraintAtomTypeInfo = {"hkpCenterOfMassChangerModifierConstraintAtom",
	finishLoadedObjecthkpCenterOfMassChangerModifierConstraintAtom,
	cleanupLoadedObjecthkpCenterOfMassChangerModifierConstraintAtom, nullptr,
	&typeid(hkpCenterOfMassChangerModifierConstraintAtom), sizeof(hkpCenterOfMassChangerModifierConstraintAtom)};
hkTypeInfo hkpViscousSurfaceModifierConstraintAtomTypeInfo = {"hkpViscousSurfaceModifierConstraintAtom",
	finishLoadedObjecthkpViscousSurfaceModifierConstraintAtom,
	cleanupLoadedObjecthkpViscousSurfaceModifierConstraintAtom, nullptr,
	&typeid(hkpViscousSurfaceModifierConstraintAtom), sizeof(hkpViscousSurfaceModifierConstraintAtom)};
hkTypeInfo hkpMovingSurfaceModifierConstraintAtomTypeInfo = {"hkpMovingSurfaceModifierConstraintAtom",
	finishLoadedObjecthkpMovingSurfaceModifierConstraintAtom, cleanupLoadedObjecthkpMovingSurfaceModifierConstraintAtom,
	nullptr, &typeid(hkpMovingSurfaceModifierConstraintAtom), sizeof(hkpMovingSurfaceModifierConstraintAtom)};
hkTypeInfo hkpIgnoreModifierConstraintAtomTypeInfo = {"hkpIgnoreModifierConstraintAtom",
	finishLoadedObjecthkpIgnoreModifierConstraintAtom, cleanupLoadedObjecthkpIgnoreModifierConstraintAtom, nullptr,
	&typeid(hkpIgnoreModifierConstraintAtom), sizeof(hkpIgnoreModifierConstraintAtom)};

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected enums of
// hkpConstraintAtom, as the exe's .rdata holds them.
using hkpConstraintAtomAtomTypeEnumItemsArray = const hkClassEnum::Item[30];
static hkpConstraintAtomAtomTypeEnumItemsArray hkpConstraintAtomAtomTypeEnumItems{
	{hkpConstraintAtom::TYPE_INVALID, "TYPE_INVALID"}, {hkpConstraintAtom::TYPE_BRIDGE, "TYPE_BRIDGE"},
	{hkpConstraintAtom::TYPE_SET_LOCAL_TRANSFORMS, "TYPE_SET_LOCAL_TRANSFORMS"},
	{hkpConstraintAtom::TYPE_SET_LOCAL_TRANSLATIONS, "TYPE_SET_LOCAL_TRANSLATIONS"},
	{hkpConstraintAtom::TYPE_SET_LOCAL_ROTATIONS, "TYPE_SET_LOCAL_ROTATIONS"},
	{hkpConstraintAtom::TYPE_BALL_SOCKET, "TYPE_BALL_SOCKET"},
	{hkpConstraintAtom::TYPE_STIFF_SPRING, "TYPE_STIFF_SPRING"}, {hkpConstraintAtom::TYPE_LIN, "TYPE_LIN"},
	{hkpConstraintAtom::TYPE_LIN_SOFT, "TYPE_LIN_SOFT"}, {hkpConstraintAtom::TYPE_LIN_LIMIT, "TYPE_LIN_LIMIT"},
	{hkpConstraintAtom::TYPE_LIN_FRICTION, "TYPE_LIN_FRICTION"}, {hkpConstraintAtom::TYPE_LIN_MOTOR, "TYPE_LIN_MOTOR"},
	{hkpConstraintAtom::TYPE_2D_ANG, "TYPE_2D_ANG"}, {hkpConstraintAtom::TYPE_ANG, "TYPE_ANG"},
	{hkpConstraintAtom::TYPE_ANG_LIMIT, "TYPE_ANG_LIMIT"}, {hkpConstraintAtom::TYPE_TWIST_LIMIT, "TYPE_TWIST_LIMIT"},
	{hkpConstraintAtom::TYPE_CONE_LIMIT, "TYPE_CONE_LIMIT"},
	{hkpConstraintAtom::TYPE_ANG_FRICTION, "TYPE_ANG_FRICTION"}, {hkpConstraintAtom::TYPE_ANG_MOTOR, "TYPE_ANG_MOTOR"},
	{hkpConstraintAtom::TYPE_RAGDOLL_MOTOR, "TYPE_RAGDOLL_MOTOR"}, {hkpConstraintAtom::TYPE_PULLEY, "TYPE_PULLEY"},
	{hkpConstraintAtom::TYPE_OVERWRITE_PIVOT, "TYPE_OVERWRITE_PIVOT"},
	{hkpConstraintAtom::TYPE_CONTACT, "TYPE_CONTACT"},
	{hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT, "TYPE_MODIFIER_SOFT_CONTACT"},
	{hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER, "TYPE_MODIFIER_MASS_CHANGER"},
	{hkpConstraintAtom::TYPE_MODIFIER_VISCOUS_SURFACE, "TYPE_MODIFIER_VISCOUS_SURFACE"},
	{hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE, "TYPE_MODIFIER_MOVING_SURFACE"},
	{hkpConstraintAtom::TYPE_MODIFIER_IGNORE_CONSTRAINT, "TYPE_MODIFIER_IGNORE_CONSTRAINT"},
	{hkpConstraintAtom::TYPE_MODIFIER_CENTER_OF_MASS_CHANGER, "TYPE_MODIFIER_CENTER_OF_MASS_CHANGER"},
	{hkpConstraintAtom::TYPE_MAX, "TYPE_MAX"}};
using hkpConstraintAtomCallbackRequestEnumItemsArray = const hkClassEnum::Item[5];
static hkpConstraintAtomCallbackRequestEnumItemsArray hkpConstraintAtomCallbackRequestEnumItems{
	{hkpConstraintAtom::CALLBACK_REQUEST_NONE, "CALLBACK_REQUEST_NONE"},
	{hkpConstraintAtom::CALLBACK_REQUEST_NEW_CONTACT_POINT, "CALLBACK_REQUEST_NEW_CONTACT_POINT"},
	{hkpConstraintAtom::CALLBACK_REQUEST_SETUP_PPU_ONLY, "CALLBACK_REQUEST_SETUP_PPU_ONLY"},
	{hkpConstraintAtom::CALLBACK_REQUEST_SETUP_CALLBACK, "CALLBACK_REQUEST_SETUP_CALLBACK"},
	{hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK, "CALLBACK_REQUEST_CONTACT_POINT_CALLBACK"}};
using hkpConstraintAtomEnumsArray = const hkClassEnum[2];
static hkpConstraintAtomEnumsArray hkpConstraintAtomEnums{
	{"AtomType", hkpConstraintAtomAtomTypeEnumItems, 30, nullptr, {0}},
	{"CallbackRequest", hkpConstraintAtomCallbackRequestEnumItems, 5, nullptr, {0}}};

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected members of
// hkpSimpleContactConstraintAtom and of the modifier atoms, as the exe's .rdata holds them.
using hkpSimpleContactConstraintAtomClass_MembersArray = const hkClassMember[8];
static hkpSimpleContactConstraintAtomClass_MembersArray hkpSimpleContactConstraintAtomClass_Members{
	{"sizeOfAllAtoms", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
	{"numContactPoints", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
	{"numReservedContactPoints", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x6,
		nullptr},
	{"numUserDatasForBodyA", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
	{"numUserDatasForBodyB", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x9, nullptr},
	{"contactPointPropertiesStriding", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0xA,
		nullptr},
	{"maxNumContactPoints", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
	{"info", &hkpSimpleContactConstraintDataInfoClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0,
		{hkClassMember::ALIGN_16}, 0x10, nullptr}};
using hkpModifierConstraintAtomClass_MembersArray = const hkClassMember[4];
static hkpModifierConstraintAtomClass_MembersArray hkpModifierConstraintAtomClass_Members{
	{"modifierAtomSize", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0,
		{hkClassMember::ALIGN_16}, 0x10, nullptr},
	{"childSize", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x12, nullptr},
	{"child", &hkpConstraintAtomClass, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0, 0, 0x14,
		nullptr},
	{"pad", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 2, 0, 0x18, nullptr}};
using hkpSoftContactModifierConstraintAtomClass_MembersArray = const hkClassMember[2];
static hkpSoftContactModifierConstraintAtomClass_MembersArray hkpSoftContactModifierConstraintAtomClass_Members{
	{"tau", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x20, nullptr},
	{"maxAcceleration", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x24, nullptr}};
using hkpMassChangerModifierConstraintAtomClass_MembersArray = const hkClassMember[2];
static hkpMassChangerModifierConstraintAtomClass_MembersArray hkpMassChangerModifierConstraintAtomClass_Members{
	{"factorA", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x20, nullptr},
	{"factorB", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x30, nullptr}};
using hkpCenterOfMassChangerModifierConstraintAtomClass_MembersArray = const hkClassMember[2];
static hkpCenterOfMassChangerModifierConstraintAtomClass_MembersArray
	hkpCenterOfMassChangerModifierConstraintAtomClass_Members{
		{"displacementA", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x20, nullptr},
		{"displacementB", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x30,
			nullptr}};
using hkpMovingSurfaceModifierConstraintAtomClass_MembersArray = const hkClassMember[1];
static hkpMovingSurfaceModifierConstraintAtomClass_MembersArray hkpMovingSurfaceModifierConstraintAtomClass_Members{
	{"velocity", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x20, nullptr}};

// The PDB's names: the enums of hkpConstraintAtom, by pointer.
static const hkClassEnum* hkpConstraintAtomAtomTypeEnum = &hkpConstraintAtomEnums[0];
static const hkClassEnum* hkpConstraintAtomCallbackRequestEnum = &hkpConstraintAtomEnums[1];

// The PDB's name. The reflected members of hkpConstraintAtom (.data); the type member's enum is
// hkpConstraintAtomAtomTypeEnum, null in the exe's image and stored by the array's dynamic initializer.
using hkpConstraintAtomClass_MembersArray = hkClassMember[1];
static hkpConstraintAtomClass_MembersArray hkpConstraintAtomClass_Members{{"type", nullptr,
	hkpConstraintAtomAtomTypeEnum, hkClassMember::TYPE_ENUM, hkClassMember::TYPE_UINT16, 0, 0, 0x0, nullptr}};

// The SDK's name (release hkpDynamics.lib, hkpConstraintAtom.obj: the file static at .bss +0x40, after the 0x40 bytes
// of s_bodyVelocities, which s_savedLinearVelocityA..s_savedAngularVelocityB are). Nothing in the exe reads or writes it.
static hkBool s_bodyVelocitiesInitialized{false};
hkClass hkpConstraintAtomClass("hkpConstraintAtom", nullptr, 2, nullptr, 0, hkpConstraintAtomEnums, 2,
	hkpConstraintAtomClass_Members, 1, nullptr, nullptr, 0, 0);
hkClass hkpSimpleContactConstraintAtomClass("hkpSimpleContactConstraintAtom", &hkpConstraintAtomClass, 48, nullptr, 0,
	nullptr, 0, hkpSimpleContactConstraintAtomClass_Members, 8, nullptr, nullptr, 0, 0);
hkClass hkpModifierConstraintAtomClass("hkpModifierConstraintAtom", &hkpConstraintAtomClass, 32, nullptr, 0, nullptr, 0,
	hkpModifierConstraintAtomClass_Members, 4, nullptr, nullptr, 0, 0);
hkClass hkpSoftContactModifierConstraintAtomClass("hkpSoftContactModifierConstraintAtom",
	&hkpModifierConstraintAtomClass, 48, nullptr, 0, nullptr, 0, hkpSoftContactModifierConstraintAtomClass_Members, 2,
	nullptr, nullptr, 0, 0);
hkClass hkpMassChangerModifierConstraintAtomClass("hkpMassChangerModifierConstraintAtom",
	&hkpModifierConstraintAtomClass, 64, nullptr, 0, nullptr, 0, hkpMassChangerModifierConstraintAtomClass_Members, 2,
	nullptr, nullptr, 0, 1);
hkClass hkpCenterOfMassChangerModifierConstraintAtomClass("hkpCenterOfMassChangerModifierConstraintAtom",
	&hkpModifierConstraintAtomClass, 64, nullptr, 0, nullptr, 0,
	hkpCenterOfMassChangerModifierConstraintAtomClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkpViscousSurfaceModifierConstraintAtomClass("hkpViscousSurfaceModifierConstraintAtom",
	&hkpModifierConstraintAtomClass, 32, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);
hkClass hkpMovingSurfaceModifierConstraintAtomClass("hkpMovingSurfaceModifierConstraintAtom",
	&hkpModifierConstraintAtomClass, 48, nullptr, 0, nullptr, 0, hkpMovingSurfaceModifierConstraintAtomClass_Members, 1,
	nullptr, nullptr, 0, 0);
hkClass hkpIgnoreModifierConstraintAtomClass("hkpIgnoreModifierConstraintAtom", &hkpModifierConstraintAtomClass, 32,
	nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);

static void cleanupLoadedObjecthkpConstraintAtom(void* p) {
	;
}

void hkBridgeConstraintAtom_callData(hkpConstraintData* m_constraintData, const hkpConstraintQueryIn& in,
	hkpConstraintQueryOut& out) {
	m_constraintData->buildJacobian(in, out);
}

const hkClass& hkpConstraintAtom::staticClass() {
	return hkpConstraintAtomClass;
}

// Adds the modifier's solver sizes to cinfo->m_sizeOfSchemas and ORs its callback flags into usedFlagsOut, returning
// the callback request: a switch on the atom type whose cases add 0x50, 0x50, 0xD0 or nothing and OR 1, 4, 2 or 0x10,
// and whose ignore-modifier case calls getConstraintInfo.
int32_t hkpModifierConstraintAtom::addModifierDataToConstraintInfo(hkpConstraintInfo& cinfo,
	uint8_t& usedFlagsOut) const {
	int32_t result = 0;
	switch (m_type.m_storage) {
	case 23: {
		usedFlagsOut |= 8u;
		break;
	}
	case 24: {
		// Two set-mass schemas and a header schema (80 bytes in the exe).
		cinfo.m_sizeOfSchemas += 2 * HK_SIZE_OF_JACOBIAN_SET_MASS_SCHEMA + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
		usedFlagsOut |= 1u;
		break;
	}
	case 25: {
		usedFlagsOut |= 16u;
		break;
	}
	case 26: {
		// Two add-velocity schemas and a header schema (80 bytes in the exe).
		cinfo.m_sizeOfSchemas += 2 * HK_SIZE_OF_JACOBIAN_ADD_VELOCITY_SCHEMA + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
		usedFlagsOut |= 4u;
		break;
	}
	case 27: {
		result = static_cast<const hkpIgnoreModifierConstraintAtom*>(this)->getConstraintInfo(cinfo);
		break;
	}
	case 28: {
		// Two set-centre-of-mass schemas and a header schema (208 bytes in the exe).
		cinfo.m_sizeOfSchemas += 2 * HK_SIZE_OF_JACOBIAN_SET_CENTER_OF_MASS_SCHEMA + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
		usedFlagsOut |= 2u;
		break;
	}
	default: {
		return result;
	}
	}
	return result;
}

int32_t hkpModifierConstraintAtom::addAllModifierDataToConstraintInfo(hkpModifierConstraintAtom* modifier,
	hkpConstraintInfo& a2, uint8_t& a3) {
	int32_t i;
	int32_t v5;
	hkpModifierConstraintAtom* v3 = modifier;
	for (i = 0; v3->m_type.m_storage >= 23u; i |= v5) {
		v5 = v3->addModifierDataToConstraintInfo(a2, a3);
		v3 = static_cast<hkpModifierConstraintAtom*>(v3->m_child);
	}
	return i;
}

const hkClass& hkpModifierConstraintAtom::staticClass() {
	return hkpModifierConstraintAtomClass;
}

int32_t hkpIgnoreModifierConstraintAtom::getConstraintInfo(hkpConstraintInfo& info) const {
	const hkpIgnoreModifierConstraintAtom* pThis = this;
	int32_t i;
	for (i = 0; pThis->m_type.m_storage >= 23u; pThis = static_cast<hkpIgnoreModifierConstraintAtom*>(pThis->m_child)) {
		i = pThis->m_childSize;
	}
	if (pThis->m_type.m_storage != 22 && pThis->m_type.m_storage != 1) {
		hkpConstraintData::ConstraintInfo a3;
		hkpConstraintData::getConstraintInfoUtil(pThis, i, a3);
		info.m_numSolverElemTemps -= a3.m_numSolverElemTemps;
		int32_t m_sizeOfSchemas = a3.m_sizeOfSchemas;
		info.m_numSolverResults -= a3.m_numSolverResults;
		info.m_sizeOfSchemas -= m_sizeOfSchemas;
	}
	return 0;
}

const hkClass& hkpIgnoreModifierConstraintAtom::staticClass() {
	return hkpIgnoreModifierConstraintAtomClass;
}

void hkpSoftContactModifierConstraintAtom::collisionResponseEndCallback(const hkContactPoint& cp, float impulseApplied,
	hkpSimpleConstraintInfoInitInput& inA, hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB,
	hkpBodyVelocity& velB) {
	__m128 v7 = _mm_undefined_ps();
	float m_tau = this->m_tau;
	v7[0] = m_tau;
	__m128 v8 = _mm_shuffle_ps(v7, v7, 0);
	velA.m_linear.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v8), s_savedLinearVelocityA),
		_mm_mul_ps(velA.m_linear.m_quad, v8));
	v8[0] = m_tau;
	__m128 v9 = _mm_shuffle_ps(v8, v8, 0);
	velA.m_angular.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v9), s_savedAngularVelocityA),
		_mm_mul_ps(velA.m_angular.m_quad, v9));
	v9[0] = m_tau;
	__m128 v10 = _mm_shuffle_ps(v9, v9, 0);
	velB.m_linear.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v10), s_savedLinearVelocityB),
		_mm_mul_ps(velB.m_linear.m_quad, v10));
	__m128 v11 = _mm_shuffle_ps(_mm_set_ss(m_tau), _mm_set_ss(m_tau), 0);
	velB.m_angular.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v11), s_savedAngularVelocityB),
		_mm_mul_ps(velB.m_angular.m_quad, v11));
}

void hkpSoftContactModifierConstraintAtom::collisionResponseBeginCallback(const hkContactPoint& cp,
	hkpSimpleConstraintInfoInitInput& inA, hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB,
	hkpBodyVelocity& velB) {
	s_savedLinearVelocityA = velA.m_linear.m_quad;
	s_savedAngularVelocityA = velA.m_angular.m_quad;
	s_savedLinearVelocityB = velB.m_linear.m_quad;
	s_savedAngularVelocityB = velB.m_angular.m_quad;
}

const hkClass& hkpSoftContactModifierConstraintAtom::staticClass() {
	return hkpSoftContactModifierConstraintAtomClass;
}

const hkClass& hkpViscousSurfaceModifierConstraintAtom::staticClass() {
	return hkpViscousSurfaceModifierConstraintAtomClass;
}

void hkpMassChangerModifierConstraintAtom::collisionResponseEndCallback(const hkContactPoint& cp, float impulseApplied,
	hkpSimpleConstraintInfoInitInput& inA, hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB,
	hkpBodyVelocity& velB) {
	;
}

void hkpMassChangerModifierConstraintAtom::collisionResponseBeginCallback(const hkContactPoint& cp,
	hkpSimpleConstraintInfoInitInput& inA, hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB,
	hkpBodyVelocity& velB) {
	hkpSimpleConstraintInfoInitInput* pInA = &inA;
	bool v11;
	hkVector4 v12;
	v12.m_quad = m_factorA.m_quad;
	__m128 m_quad = v12.m_quad;
	int32_t v13 = 2;
	do {
		pInA->m_invMasses.m_quad = _mm_mul_ps(pInA->m_invMasses.m_quad, m_quad);
		if (v12.m_quad[1] == v12.m_quad[0] && v12.m_quad[2] == v12.m_quad[1]) {
			hkSimdReal v15;
			v15.m_real = m_quad;
			pInA->m_invInertia.mul(v15);
		} else {
			hkMatrix3 v14;
			hkMatrix3 arA;
			const hkTransform* m_transform = pInA->m_transform;
			hkMatrix3* p_m_invInertia = &pInA->m_invInertia;
			arA.setMulInverseMul(m_transform->m_rotation, pInA->m_invInertia);
			v14.setMul(arA, m_transform->m_rotation);
			v14.m_col0.m_quad[0] = v14.m_col0.m_quad[0] * v12.m_quad[0];
			v14.m_col1.m_quad[1] = v14.m_col1.m_quad[1] * v12.m_quad[1];
			v14.m_col2.m_quad[2] = v14.m_col2.m_quad[2] * v12.m_quad[2];
			arA.setMulInverse(v14, m_transform->m_rotation);
			p_m_invInertia->setMul(m_transform->m_rotation, arA);
		}
		v11 = v13-- == 1;
		m_quad = m_factorB.m_quad;
		pInA = &inB;
		v12.m_quad = m_quad;
	} while (!v11);
}

const hkClass& hkpMassChangerModifierConstraintAtom::staticClass() {
	return hkpMassChangerModifierConstraintAtomClass;
}

void hkpMovingSurfaceModifierConstraintAtom::collisionResponseEndCallback(const hkContactPoint& cp,
	float impulseApplied, hkpSimpleConstraintInfoInitInput& inA, hkpBodyVelocity& velA,
	hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB) {
	hkVector4 v10;
	v10.m_quad = m_velocity.m_quad;
	__m128 v7 = _mm_mul_ps(cp.m_separatingNormal.m_quad, v10.m_quad);
	float v8 = _mm_shuffle_ps(v7, v7, 85)[0] + v7[0];
	__m128 v9 = _mm_shuffle_ps(v7, v7, 170);
	v9[0] = v9[0] + v8;
	velB.m_linear.m_quad =
		_mm_sub_ps(velB.m_linear.m_quad, _mm_sub_ps(v10.m_quad, _mm_mul_ps(_mm_shuffle_ps(v9, v9, 0), v10.m_quad)));
}

void hkpMovingSurfaceModifierConstraintAtom::collisionResponseBeginCallback(const hkContactPoint& cp,
	hkpSimpleConstraintInfoInitInput& inA, hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB,
	hkpBodyVelocity& velB) {
	hkVector4 v10;
	v10.m_quad = m_velocity.m_quad;
	__m128 v7 = _mm_mul_ps(cp.m_separatingNormal.m_quad, v10.m_quad);
	float v8 = _mm_shuffle_ps(v7, v7, 85)[0] + v7[0];
	__m128 v9 = _mm_shuffle_ps(v7, v7, 170);
	v9[0] = v9[0] + v8;
	velB.m_linear.m_quad =
		_mm_add_ps(_mm_sub_ps(v10.m_quad, _mm_mul_ps(_mm_shuffle_ps(v9, v9, 0), v10.m_quad)), velB.m_linear.m_quad);
}

const hkClass& hkpMovingSurfaceModifierConstraintAtom::staticClass() {
	return hkpMovingSurfaceModifierConstraintAtomClass;
}

void hkpCenterOfMassChangerModifierConstraintAtom::collisionResponseEndCallback(const hkContactPoint& cp,
	float impulseApplied, hkpSimpleConstraintInfoInitInput& inA, hkpBodyVelocity& velA,
	hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB) {
	hkVector4 v7;
	hkVector4 v14;
	hkVector4 v17;
	v7.m_quad = m_displacementA.m_quad;
	const hkTransform* m_transform = inA.m_transform;
	__m128 v9 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v7.m_quad, v7.m_quad, 85), m_transform->m_rotation.m_col1.m_quad),
		_mm_mul_ps(_mm_shuffle_ps(v7.m_quad, v7.m_quad, 0), m_transform->m_rotation.m_col0.m_quad));
	__m128 v10 = _mm_shuffle_ps(v7.m_quad, v7.m_quad, 170);
	__m128 m_quad = m_transform->m_rotation.m_col2.m_quad;
	const hkTransform* v12 = inB.m_transform;
	__m128 v13 = _mm_add_ps(v9, _mm_mul_ps(v10, m_quad));
	v14.m_quad = m_displacementB.m_quad;
	__m128 v15 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v14.m_quad, v14.m_quad, 85), v12->m_rotation.m_col1.m_quad),
		_mm_mul_ps(_mm_shuffle_ps(v14.m_quad, v14.m_quad, 0), v12->m_rotation.m_col0.m_quad));
	__m128 v16 = _mm_mul_ps(_mm_shuffle_ps(v14.m_quad, v14.m_quad, 170), v12->m_rotation.m_col2.m_quad);
	v17.m_quad = velB.m_angular.m_quad;
	__m128 v18 = _mm_add_ps(v15, v16);
	velA.m_linear.m_quad = _mm_add_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v13, v13, 201),
													 _mm_shuffle_ps(velA.m_angular.m_quad, velA.m_angular.m_quad, 210)),
										  _mm_mul_ps(_mm_shuffle_ps(v13, v13, 210),
											  _mm_shuffle_ps(velA.m_angular.m_quad, velA.m_angular.m_quad, 201))),
		velA.m_linear.m_quad);
	velB.m_linear.m_quad =
		_mm_add_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v18, v18, 201), _mm_shuffle_ps(v17.m_quad, v17.m_quad, 210)),
					   _mm_mul_ps(_mm_shuffle_ps(v18, v18, 210), _mm_shuffle_ps(v17.m_quad, v17.m_quad, 201))),
			velB.m_linear.m_quad);
}

void hkpCenterOfMassChangerModifierConstraintAtom::collisionResponseBeginCallback(const hkContactPoint& cp,
	hkpSimpleConstraintInfoInitInput& inA, hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB,
	hkpBodyVelocity& velB) {
	hkVector4 v7;
	hkVector4 v13;
	__m128 v6 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(m_displacementA.m_quad, m_displacementA.m_quad, 85),
										  inA.m_transform->m_rotation.m_col1.m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(m_displacementA.m_quad, m_displacementA.m_quad, 0),
								   inA.m_transform->m_rotation.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(m_displacementA.m_quad, m_displacementA.m_quad, 170),
			inA.m_transform->m_rotation.m_col2.m_quad));
	v7.m_quad = m_displacementB.m_quad;
	const hkTransform* m_transform = inB.m_transform;
	__m128 v9 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v7.m_quad, v7.m_quad, 85), m_transform->m_rotation.m_col1.m_quad),
		_mm_mul_ps(_mm_shuffle_ps(v7.m_quad, v7.m_quad, 0), m_transform->m_rotation.m_col0.m_quad));
	__m128 v10 = _mm_shuffle_ps(v7.m_quad, v7.m_quad, 170);
	__m128 m_quad = m_transform->m_rotation.m_col2.m_quad;
	inA.m_massRelPos.m_quad = _mm_sub_ps(inA.m_massRelPos.m_quad, v6);
	__m128 v12 = _mm_add_ps(v9, _mm_mul_ps(v10, m_quad));
	inB.m_massRelPos.m_quad = _mm_sub_ps(inB.m_massRelPos.m_quad, v12);
	v13.m_quad = velB.m_angular.m_quad;
	velA.m_linear.m_quad = _mm_sub_ps(velA.m_linear.m_quad,
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v6, v6, 201),
					   _mm_shuffle_ps(velA.m_angular.m_quad, velA.m_angular.m_quad, 210)),
			_mm_mul_ps(_mm_shuffle_ps(v6, v6, 210),
				_mm_shuffle_ps(velA.m_angular.m_quad, velA.m_angular.m_quad, 201))));
	velB.m_linear.m_quad = _mm_sub_ps(velB.m_linear.m_quad,
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v12, v12, 201), _mm_shuffle_ps(v13.m_quad, v13.m_quad, 210)),
			_mm_mul_ps(_mm_shuffle_ps(v12, v12, 210), _mm_shuffle_ps(v13.m_quad, v13.m_quad, 201))));
}

const hkClass& hkpCenterOfMassChangerModifierConstraintAtom::staticClass() {
	return hkpCenterOfMassChangerModifierConstraintAtomClass;
}

// The loaded counts stay; when finishing, the properties' striding follows from them (as allocateAtom's).
hkpSimpleContactConstraintAtom::hkpSimpleContactConstraintAtom(hkFinishLoadedObjectFlag f) {
	if (f.m_finishing) {
		m_contactPointPropertiesStriding = static_cast<uint8_t>(sizeof(hkpContactPointProperties::UserData) *
				(m_numUserDatasForBodyA + m_numUserDatasForBodyB) +
			sizeof(hkpContactPointProperties));
	}
}

const hkClass& hkpSimpleContactConstraintAtom::staticClass() {
	return hkpSimpleContactConstraintAtomClass;
}

void hkpBridgeConstraintAtom::init(hkpConstraintData* constraintData) {
	m_constraintData = constraintData;
	m_buildJacobianFunc = &hkBridgeConstraintAtom_callData;
}

const hkClass& hkpBridgeConstraintAtom::staticClass() {
	return hkpBridgeConstraintAtomClass;
}

hkpSimpleContactConstraintDataInfo* clearContactInfo(hkpSimpleContactConstraintDataInfo* apInfo) {
	apInfo->m_data[2] = 0;
	apInfo->m_data[4] = 0;
	apInfo->m_data[6] = 0;
	return apInfo;
}
