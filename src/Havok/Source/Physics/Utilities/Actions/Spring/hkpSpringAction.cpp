#include "Havok/Source/Physics/Utilities/Actions/Spring/hkpSpringAction.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpBinaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

#include <xmmintrin.h>

// The reflection data of hkpSpringAction's class record, as the Havok SDK's class file has it: per member its range,
// user-interface and semantics attributes (.data), the attribute list naming them and the custom attributes record
// over the list (.rdata), then the member table and the defaults. Not named in the PDB or IDA; names chosen here.
using hkpSpringActionAttributeArray3 = const hkCustomAttributes::Attribute[3];
using hkpSpringActionAttributeArray2 = const hkCustomAttributes::Attribute[2];
namespace {
// The SDK's hkpSpringAction_DefaultStruct: each member's offset in this struct (-1: no default for lastForce and the
// two positions), then the defaults of restLength, strength, damping and the two flags (the SDK's _hkBool, an 8-bit
// integer).
struct hkpSpringAction_DefaultStruct {
	int32_t s_defaultOffsets[8];
	float m_restLength;
	float m_strength;
	float m_damping;
	int8_t m_onCompression;
	int8_t m_onExtension;
};
} // namespace

// Pushes the two bodies apart or together along the line between the attachment points: a spring force
// (length - rest length) * strength plus the damping times the relative velocity along the line. The exe inlines
// the SDK's hkVector4 setSub4/length3/mul4/dot3/setMul4 (SSE, written as intrinsics) and hkpRigidBody::applyForce
// (activate, then the motion's applyForce slot 22); the scalar arithmetic between them is rounded at the float
// locals where the exe stores. The exe's frame has no EH and no /GS cookie.

using hkpSpringActionAttributeArray1 = const hkCustomAttributes::Attribute[1];
static hkRangeRealAttribute hkpSpringAction_positionAinA_RangeReal{-1000.0f, 1000.0f, -10.0f, 10.0f};
static hkUiAttribute hkpSpringAction_positionAinA_Ui{true, {hkUiAttribute::NONE}, "Pivot", "Pivot", "", false, false,
	false};
static hkSemanticsAttribute hkpSpringAction_positionAinA_Semantics{{hkSemanticsAttribute::POSITION}};
static hkpSpringActionAttributeArray3 hkpSpringAction_positionAinA_AttributeList{
	{"hk.RangeReal", {&hkpSpringAction_positionAinA_RangeReal, &hkRangeRealAttributeClass}},
	{"hk.Ui", {&hkpSpringAction_positionAinA_Ui, &hkUiAttributeClass}},
	{"hk.Semantics", {&hkpSpringAction_positionAinA_Semantics, &hkSemanticsAttributeClass}}};
static const hkCustomAttributes hkpSpringAction_positionAinA_Attributes{hkpSpringAction_positionAinA_AttributeList, 3};
static hkRangeRealAttribute hkpSpringAction_positionBinB_RangeReal{-1000.0f, 1000.0f, -10.0f, 10.0f};
static hkUiAttribute hkpSpringAction_positionBinB_Ui{true, {hkUiAttribute::NONE}, "Pivot On Other Body", "", "", true,
	false, false};
static hkSemanticsAttribute hkpSpringAction_positionBinB_Semantics{{hkSemanticsAttribute::POSITION}};
static hkpSpringActionAttributeArray3 hkpSpringAction_positionBinB_AttributeList{
	{"hk.RangeReal", {&hkpSpringAction_positionBinB_RangeReal, &hkRangeRealAttributeClass}},
	{"hk.Ui", {&hkpSpringAction_positionBinB_Ui, &hkUiAttributeClass}},
	{"hk.Semantics", {&hkpSpringAction_positionBinB_Semantics, &hkSemanticsAttributeClass}}};
static const hkCustomAttributes hkpSpringAction_positionBinB_Attributes{hkpSpringAction_positionBinB_AttributeList, 3};
static hkRangeRealAttribute hkpSpringAction_restLength_RangeReal{0.0f, 1000.0f, 0.1f, 10.0f};
static hkUiAttribute hkpSpringAction_restLength_Ui{true, {hkUiAttribute::NONE}, "Resting Length", "Constants", "",
	false, false, false};
static hkSemanticsAttribute hkpSpringAction_restLength_Semantics{{hkSemanticsAttribute::DISTANCE}};
static hkpSpringActionAttributeArray3 hkpSpringAction_restLength_AttributeList{
	{"hk.RangeReal", {&hkpSpringAction_restLength_RangeReal, &hkRangeRealAttributeClass}},
	{"hk.Ui", {&hkpSpringAction_restLength_Ui, &hkUiAttributeClass}},
	{"hk.Semantics", {&hkpSpringAction_restLength_Semantics, &hkSemanticsAttributeClass}}};
static const hkCustomAttributes hkpSpringAction_restLength_Attributes{hkpSpringAction_restLength_AttributeList, 3};
static hkRangeRealAttribute hkpSpringAction_strength_RangeReal{0.1f, 1000.0f, 0.1f, 1000.0f};
static hkUiAttribute hkpSpringAction_strength_Ui{true, {hkUiAttribute::NONE}, "", "", "", false, false, false};
static hkpSpringActionAttributeArray2 hkpSpringAction_strength_AttributeList{
	{"hk.RangeReal", {&hkpSpringAction_strength_RangeReal, &hkRangeRealAttributeClass}},
	{"hk.Ui", {&hkpSpringAction_strength_Ui, &hkUiAttributeClass}}};
static const hkCustomAttributes hkpSpringAction_strength_Attributes{hkpSpringAction_strength_AttributeList, 2};
static hkRangeRealAttribute hkpSpringAction_damping_RangeReal{0.1f, 1000.0f, 0.1f, 1000.0f};
static hkUiAttribute hkpSpringAction_damping_Ui{true, {hkUiAttribute::NONE}, "", "", "", false, false, false};
static hkpSpringActionAttributeArray2 hkpSpringAction_damping_AttributeList{
	{"hk.RangeReal", {&hkpSpringAction_damping_RangeReal, &hkRangeRealAttributeClass}},
	{"hk.Ui", {&hkpSpringAction_damping_Ui, &hkUiAttributeClass}}};
static const hkCustomAttributes hkpSpringAction_damping_Attributes{hkpSpringAction_damping_AttributeList, 2};
static hkUiAttribute hkpSpringAction_onCompression_Ui{true, {hkUiAttribute::NONE}, "", "", "", false, false, false};
static hkpSpringActionAttributeArray1 hkpSpringAction_onCompression_AttributeList{
	{"hk.Ui", {&hkpSpringAction_onCompression_Ui, &hkUiAttributeClass}}};
static const hkCustomAttributes hkpSpringAction_onCompression_Attributes{hkpSpringAction_onCompression_AttributeList,
	1};
static hkUiAttribute hkpSpringAction_onExtension_Ui{true, {hkUiAttribute::NONE}, "", "", "", true, false, false};
static hkpSpringActionAttributeArray1 hkpSpringAction_onExtension_AttributeList{
	{"hk.Ui", {&hkpSpringAction_onExtension_Ui, &hkUiAttributeClass}}};
static const hkCustomAttributes hkpSpringAction_onExtension_Attributes{hkpSpringAction_onExtension_AttributeList, 1};
using hkpSpringActionClass_MembersArray = const hkClassMember[8];
static hkpSpringActionClass_MembersArray hkpSpringActionClass_Members{
	{"lastForce", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
	{"positionAinA", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30,
		&hkpSpringAction_positionAinA_Attributes},
	{"positionBinB", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x40,
		&hkpSpringAction_positionBinB_Attributes},
	{"restLength", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x50,
		&hkpSpringAction_restLength_Attributes},
	{"strength", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x54,
		&hkpSpringAction_strength_Attributes},
	{"damping", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x58,
		&hkpSpringAction_damping_Attributes},
	{"onCompression", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x5C,
		&hkpSpringAction_onCompression_Attributes},
	{"onExtension", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x5D,
		&hkpSpringAction_onExtension_Attributes}};

// The defaults the class record names (.rdata). Not named in the PDB or IDA; name chosen here (the Havok SDK's).
static const hkpSpringAction_DefaultStruct hkpSpringAction_Default{{-1, -1, -1, 0x20, 0x24, 0x28, 0x2C, 0x2D}, 1.0f,
	1000.0f, 0.1f, 1, 1};
hkClass hkpSpringActionClass("hkpSpringAction", &hkpBinaryActionClass, 96, nullptr, 0, nullptr, 0,
	hkpSpringActionClass_Members, 8, &hkpSpringAction_Default, nullptr, 0, 0);
hkTypeInfo hkpSpringActionTypeInfo = {"hkpSpringAction", finishLoadedObjecthkpSpringAction,
	cleanupLoadedObjecthkpSpringAction, getVtablehkpSpringAction(), &typeid(hkpSpringAction), sizeof(hkpSpringAction)};

// The positions and the last force are left unwritten, as in the exe.
hkpSpringAction::hkpSpringAction(hkpRigidBody* entityA, hkpRigidBody* entityB, hkUlong userData)
	: hkpBinaryAction(entityA, entityB, userData), m_restLength(1.0f), m_strength(1000.0f), m_damping(0.1f) {
	m_onCompression.m_bool = 1;
	m_onExtension.m_bool = 1;
}

hkpSpringAction::~hkpSpringAction() {}

FNVD_NO_STACK_COOKIE void hkpSpringAction::applyAction(const hkStepInfo& stepInfo) {
	hkpRigidBody* ra = static_cast<hkpRigidBody*>(m_entityA);
	hkpRigidBody* rb = static_cast<hkpRigidBody*>(m_entityB);

	// The exe's frame holds posB, posA, the direction, velB and velA back to back (16 bytes each, [esp+0x20] to
	// [esp+0x60]); the callees that take posA, posB or a velocity see the vectors after it, so src keeps that order.
	alignas(16) hkVector4 frameVectors[5];
	hkVector4& posB = frameVectors[0];
	hkVector4& posA = frameVectors[1];
	hkVector4& direction = frameVectors[2];
	hkVector4& velB = frameVectors[3];
	hkVector4& velA = frameVectors[4];
	posA.setTransformedPos(ra->m_motion.m_motionState.m_transform, m_positionAinA);
	posB.setTransformedPos(rb->m_motion.m_motionState.m_transform, m_positionBinB);

	// EXE-INLINED none: hkVector4::setSub4
	__m128 dirAtoB = _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&posB.m_quad)[0]),
		_mm_load_ps(&reinterpret_cast<float*>(&posA.m_quad)[0]));
	// EXE-INLINED 0x630B40
	__m128 sq = _mm_mul_ps(dirAtoB, dirAtoB);
	__m128 sum = _mm_add_ss(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq), _mm_shuffle_ps(sq, sq, 0xAA));
	float length = _mm_cvtss_f32(_mm_sqrt_ss(sum));
	if (length < 0.001f) {
		return;
	}

	// The reciprocal is x87 (fld1; fdivrp), stored as a float before the SSE scale.
	float invLength = static_cast<float>(1.0 / static_cast<double>(length));
	// EXE-INLINED 0x627920
	dirAtoB = _mm_mul_ps(_mm_set1_ps(invLength), dirAtoB);
	// movaps [esp+0x40]: the exe keeps the direction in its frame slot.
	_mm_store_ps(&reinterpret_cast<float*>(&direction.m_quad)[0], dirAtoB);

	// EXE-INLINED 0x5609F0
	if (m_onCompression.m_bool == 0 && length < m_restLength) {
		return;
	}
	// EXE-INLINED 0x5609F0
	if (m_onExtension.m_bool == 0 && length > m_restLength) {
		return;
	}

	ra->getPointVelocity(posA, velA);
	rb->getPointVelocity(posB, velB);

	// EXE-INLINED 0x62B0A0
	__m128 relVel = _mm_mul_ps(_mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&velB.m_quad)[0]),
								   _mm_load_ps(&reinterpret_cast<float*>(&velA.m_quad)[0])),
		dirAtoB);
	float relVelDot = _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(_mm_shuffle_ps(relVel, relVel, 0x55), relVel),
		_mm_shuffle_ps(relVel, relVel, 0xAA)));
	// The x87 chain: (length - restLength) * strength + damping * relVel, stored as a float.
	float force = static_cast<float>((static_cast<double>(length) - m_restLength) * m_strength +
		static_cast<double>(m_damping) * relVelDot);

	float negForce = -force;
	// EXE-INLINED 0x630AE0
	_mm_store_ps(&reinterpret_cast<float*>(&m_lastForce.m_quad)[0], _mm_mul_ps(_mm_set1_ps(negForce), dirAtoB));
	float deltaTimeB = stepInfo.m_deltaTime;
	// EXE-INLINED 0x631140
	rb->activate();
	rb->getRigidMotion()->applyForce(deltaTimeB, m_lastForce, posB);

	// EXE-INLINED 0x630AE0
	_mm_store_ps(&reinterpret_cast<float*>(&m_lastForce.m_quad)[0], _mm_mul_ps(_mm_set1_ps(force), dirAtoB));
	float deltaTimeA = stepInfo.m_deltaTime;
	// EXE-INLINED 0x631140
	ra->activate();
	ra->getRigidMotion()->applyForce(deltaTimeA, m_lastForce, posA);
}

// The SDK's clone: a new spring between the two new entities (downcast to hkpRigidBody as the source does), with
// this spring's pivots, lengths and flags. The float fields are x87 copies (fld/fstp at 0xD4D507-0xD4D517).
hkpAction* hkpSpringAction::clone(const hkArray<hkpEntity*, hkContainerHeapAllocator>& newEntities,
	const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms) const {
	if (newEntities.m_size != 2 || newPhantoms.m_size) {
		return 0;
	}
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpSpringAction* v6 = static_cast<hkpSpringAction*>(Instance->m_heap->blockAlloc(sizeof(hkpSpringAction)));
	v6->m_memSizeAndFlags = sizeof(hkpSpringAction);
	hkpSpringAction* pClone = ::new (v6) hkpSpringAction(static_cast<hkpRigidBody*>(newEntities.m_data[0]),
		static_cast<hkpRigidBody*>(newEntities.m_data[1]), m_userData);
	pClone->m_positionAinA = m_positionAinA;
	pClone->m_positionBinB = m_positionBinB;
	pClone->m_restLength = m_restLength;
	pClone->m_strength = m_strength;
	pClone->m_damping = m_damping;
	pClone->m_onCompression.m_bool = m_onCompression.m_bool;
	pClone->m_onExtension.m_bool = m_onExtension.m_bool;
	return pClone;
}

void hkpSpringAction::setPositionsInWorldSpace(const hkVector4& pivotA, const hkVector4& pivotB) {
	hkpEntity* m_entityB = this->getEntityB();
	m_positionAinA.setTransformedInversePos(this->getEntityA()->m_motion.m_motionState.m_transform, pivotA);
	m_positionBinB.setTransformedInversePos(m_entityB->m_motion.m_motionState.m_transform, pivotB);
	__m128 v5 = _mm_sub_ps(pivotA.m_quad, pivotB.m_quad);
	__m128 v6 = _mm_mul_ps(v5, v5);
	m_restLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(static_cast<float>((_mm_shuffle_ps(v6, v6, 170)[0] +
		static_cast<float>(_mm_shuffle_ps(v6, v6, 85)[0] + v6[0]))))));
}

void hkpSpringAction::setPositionsInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB) {
	hkVector4 v8;
	hkVector4 v9;
	hkpEntity* m_entityB = this->getEntityB();
	hkMotionState* p_m_motionState = this->getEntityA()->m_motion.getMotionState();
	m_positionAinA.m_quad = pivotA.m_quad;
	m_positionBinB.m_quad = pivotB.m_quad;
	v8.setTransformedPos(p_m_motionState->m_transform, pivotA);
	v9.setTransformedPos(m_entityB->m_motion.m_motionState.m_transform, pivotB);
	__m128 v6 = _mm_sub_ps(v8.m_quad, v9.m_quad);
	__m128 v7 = _mm_mul_ps(v6, v6);
	m_restLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(static_cast<float>((_mm_shuffle_ps(v7, v7, 170)[0] +
		static_cast<float>(_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]))))));
}

const hkClass& hkpSpringAction::staticClass() {
	return hkpSpringActionClass;
}

const hkClass* hkpBinaryAction::getClassType() const {
	return &hkpBinaryActionClass;
}
