#include "Havok/Source/Physics/Utilities/Weapons/hkpProjectileGun.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Utilities/Weapons/hkpFirstPersonGun.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

#include <xmmintrin.h>

// Havok SDK name; not in the PDB.
// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
// Inferred type: apThis: hkpGunProjectile* from 0xe44d00 (this of hkpGunProjectile::_destroyBody), its members used at
// 0xe44cc4
static void removeFromWorld(hkpGunProjectile* apThis, [[maybe_unused]] void* apEdx);

// The reflection data of hkpProjectileGun's class record, as the Havok SDK's class file has it: per member its
// attribute objects (.data), the attribute list naming them and the custom attributes record over the list (.rdata),
// then the member table. Not named in the PDB or IDA; names chosen here.
using hkpProjectileGunAttributeArray2 = const hkCustomAttributes::Attribute[2];
static hkUiAttribute hkpProjectileGun_maxProjectiles_Ui{true, {hkUiAttribute::NONE}, "Max Projectiles in World", "", "",
	false, false, false};
static hkRangeInt32Attribute hkpProjectileGun_maxProjectiles_RangeInt32{0, 1000, 0, 1000};
static hkpProjectileGunAttributeArray2 hkpProjectileGun_maxProjectiles_AttributeList{
	{"hk.Ui", {&hkpProjectileGun_maxProjectiles_Ui, &hkUiAttributeClass}},
	{"hk.RangeInt32", {&hkpProjectileGun_maxProjectiles_RangeInt32, &hkRangeInt32AttributeClass}}};
static const hkCustomAttributes hkpProjectileGun_maxProjectiles_Attributes{
	hkpProjectileGun_maxProjectiles_AttributeList, 2};
static hkRangeRealAttribute hkpProjectileGun_reloadTime_RangeReal{0.0f, 100.0f, 0.0f, 100.0f};
static hkUiAttribute hkpProjectileGun_reloadTime_Ui{true, {hkUiAttribute::NONE}, "Reload delay", "", "", false, false,
	false};
static hkpProjectileGunAttributeArray2 hkpProjectileGun_reloadTime_AttributeList{
	{"hk.RangeReal", {&hkpProjectileGun_reloadTime_RangeReal, &hkRangeRealAttributeClass}},
	{"hk.Ui", {&hkpProjectileGun_reloadTime_Ui, &hkUiAttributeClass}}};
static const hkCustomAttributes hkpProjectileGun_reloadTime_Attributes{hkpProjectileGun_reloadTime_AttributeList, 2};
using hkpProjectileGunClass_MembersArray = const hkClassMember[6];
static hkpProjectileGunClass_MembersArray hkpProjectileGunClass_Members{
	{"maxProjectiles", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20,
		&hkpProjectileGun_maxProjectiles_Attributes},
	{"reloadTime", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x24,
		&hkpProjectileGun_reloadTime_Attributes},
	{"reload", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x28, nullptr},
	{"projectiles", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x2C, nullptr},
	{"world", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x38, nullptr},
	{"destructionWorld", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x3C, nullptr}};

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
static void removeFromWorld(hkpGunProjectile* apThis, [[maybe_unused]] void* apEdx) {
	hkBool result;
	if (apThis->m_body) {
		apThis->m_body->m_world->removeEntity(&result, apThis->m_body);
		if (apThis->m_flags.anyIsSet(hkpGunProjectile::HIT_LISTENING)) {
			apThis->m_body->removeContactListener(static_cast<hkpContactListener*>(apThis));
		}
	}
}

namespace {
// The PDB's `anonymous-namespace'::hkpProjectileGun_DefaultStruct: per member its offset into this struct (-1: no
// default), then the defaults.
struct hkpProjectileGun_DefaultStruct {
	int32_t s_defaultOffsets[6];
	int32_t m_maxProjectiles;
	float m_reloadTime;
};
} // namespace
// The defaults the class record names (.rdata). Not named in the PDB or IDA; name chosen here (the Havok SDK's).
static const hkpProjectileGun_DefaultStruct hkpProjectileGun_Default{{0x18, 0x1C, -1, -1, -1, -1}, 5, 0.3f};

hkClass hkpProjectileGunClass("hkpProjectileGun", &hkpFirstPersonGunClass, 64, nullptr, 0, nullptr, 0,
	hkpProjectileGunClass_Members, 6, &hkpProjectileGun_Default, nullptr, 0, 0);
hkTypeInfo hkpProjectileGunTypeInfo = {"hkpProjectileGun", finishLoadedObjecthkpProjectileGun,
	cleanupLoadedObjecthkpProjectileGun, getVtablehkpProjectileGun(), &typeid(hkpProjectileGun),
	sizeof(hkpProjectileGun)};

hkpGunProjectile::hkpGunProjectile(hkpProjectileGun* gun, hkpRigidBody* body)
	: m_flags(0), m_body(body), m_existanceTime(0.0f), m_gun(gun) {
	body->addReference();
}

hkpGunProjectile::~hkpGunProjectile() {
	_destroyBody();
}

// The exe's slot is a jump to _destroyBody (IDA j_hkpGunProjectile::Func0003).
void hkpGunProjectile::onDestroy() {
	_destroyBody();
}

void hkpGunProjectile::_destroyBody() {
	hkpRigidBody* m_body = this->m_body;
	if (m_body) {
		if (m_body->m_world) {
			removeFromWorld(this, nullptr);
		}
		this->m_body->removeReference();
		this->m_body = 0;
	}
}

void hkpGunProjectile::setHitListening(bool enable) {
	if (enable != m_flags.anyIsSet(HIT_LISTENING)) {
		hkpRigidBody* m_body = this->m_body;
		if (m_body && m_body->m_world) {
			hkpContactListener* v4;
			v4 = static_cast<hkpContactListener*>(this);
			if (!enable) {
				m_body->removeContactListener(v4);
			LABEL_9:;
				m_flags.andWith(static_cast<uint8_t>(~HIT_LISTENING));
				return;
			}
			m_body->addContactListener(v4);
		}
		if (enable) {
			m_flags.orWith(HIT_LISTENING);
			return;
		}
		goto LABEL_9;
	}
}

void hkpGunProjectile::destroy() {
	if ((m_flags.get() & 1) == 0) {
		onDestroy();
		m_flags.m_storage |= 1u;
	}
}

void hkpGunProjectile::addToWorld(hkpWorld* world) {
	if (m_flags.anyIsSet(HIT_LISTENING)) {
		m_body->addContactListener(this);
	}
	world->addEntity(m_body, HK_ENTITY_ACTIVATION_DO_ACTIVATE);
}

void hkpGunProjectile::update(float timeStep) {
	if (!m_flags.anyIsSet(DESTROYED)) {
		m_existanceTime = m_existanceTime + timeStep;
		onUpdate(timeStep);
	}
}

void hkpGunProjectile::flyTrue(hkpRigidBody* body, float minVelocity, float timeStep) {
	hkVector4 v3;
	hkTransform arB;
	v3.m_quad = body->m_motion.m_linearVelocity.m_quad;
	__m128 m_quad = body->m_motion.m_motionState.m_transform.m_rotation.m_col0.m_quad;
	arB.m_rotation.m_col1 = body->m_motion.m_motionState.m_transform.m_rotation.m_col1;
	arB.m_rotation.m_col2 = body->m_motion.m_motionState.m_transform.m_rotation.m_col2;
	arB.m_translation = body->m_motion.m_motionState.m_transform.m_translation;
	__m128 v5 = _mm_mul_ps(v3.m_quad, v3.m_quad);
	arB.m_rotation.m_col0.m_quad = m_quad;
	if (minVelocity <
		static_cast<double>(_mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(static_cast<float>((_mm_shuffle_ps(v5, v5, 170)[0] +
			static_cast<float>(_mm_shuffle_ps(v5, v5, 85)[0] + v5[0])))))))) {
		hkVector4 q;
		__m128 v6 = _mm_mul_ps(v3.m_quad, v3.m_quad);
		float v7 = _mm_shuffle_ps(v6, v6, 85)[0] + v6[0];
		__m128 v8 = _mm_shuffle_ps(v6, v6, 170);
		v8[0] = v8[0] + v7;
		float v9 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v8[0])));
		__m128 v10 = _mm_setzero_ps();
		v10[0] = 0.0f;
		__m128 v11 = _mm_setzero_ps();
		v11[0] = 0.5f;
		__m128 v12 = _mm_cmpeq_ss(v8, v10);
		q.m_quad = v10;
		v10[0] = 3.0f - static_cast<float>(static_cast<float>(v8[0] * v9) * v9);
		v11[0] = static_cast<float>(0.5f * v9) * v10[0];
		__m128 v14 = _mm_andnot_ps(v12, v11);
		__m128 v15 = _mm_mul_ps(_mm_shuffle_ps(v14, v14, 0), v3.m_quad);
		__m128 v16 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 210), _mm_shuffle_ps(v15, v15, 201)),
			_mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 201), _mm_shuffle_ps(v15, v15, 210)));
		__m128 v17 = _mm_mul_ps(v16, v16);
		float v18 = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(static_cast<float>((_mm_shuffle_ps(v17, v17, 170)[0] +
			static_cast<float>(_mm_shuffle_ps(v17, v17, 85)[0] + v17[0]))))));
		if (v18 > 1e-05f) {
			hkRotation v26;
			hkRotation arA;
			float v19 = _mm_shuffle_ps(v17, v17, 85)[0] + v17[0];
			__m128 v20 = _mm_shuffle_ps(v17, v17, 170);
			v20[0] = v20[0] + v19;
			float v21 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v20[0])));
			float afAngle = v18 * timeStep * -5.0;
			v11[0] = static_cast<float>(0.5f * v21) *
				static_cast<float>(3.0f - static_cast<float>(static_cast<float>(v20[0] * v21) * v21));
			__m128 v22 = _mm_andnot_ps(_mm_cmpeq_ss(v20, q.m_quad), v11);
			q.m_quad = _mm_mul_ps(_mm_shuffle_ps(v22, v22, 0), v16);
			arA.setAxisAngle(q, afAngle);
			v26.setMul(arA, arB.m_rotation);
			arB.m_rotation = v26;
			body->setTransform(arB);
		}
	}
}

hkpProjectileGun::hkpProjectileGun(hkpWorld* world, hkdWorld* destructionWorld)
	: m_maxProjectiles(5), m_reloadTime(0.3f), m_reload(0.0f), m_world(world), m_destructionWorld(destructionWorld) {}

hkpProjectileGun::hkpProjectileGun(hkFinishLoadedObjectFlag aFlag) : hkpFirstPersonGun(aFlag) {}

// The projectiles array frees itself.
hkpProjectileGun::~hkpProjectileGun() {
	clearProjectiles();
}

void hkpProjectileGun::fireGun(hkpWorld* world, const hkTransform& viewTransform) {
	if (!(m_reload > 0.0f)) {
		// The exe copies the reload time with fld/fstp.
		float reloadTime = m_reloadTime;
		m_reload = reloadTime;
		onGunFired(world, viewTransform);
	}
}

void hkpProjectileGun::stepGun(float timeStep, hkpWorld* world, const hkpRigidBody* characterBody,
	const hkTransform& viewTransform, bool fire, bool fireRmb) {
	if (m_reload > 0.0) {
		m_reload = m_reload - timeStep;
	}
	onUpdate(timeStep, world, characterBody, viewTransform, fire, fireRmb);
}

void hkpProjectileGun::reset(hkpWorld* world) {
	clearProjectiles();
}

// The exe calls removeDestroyedProjectiles with the ecx clearHitProjectiles leaves, which is this: that callee never
// writes ecx.
void hkpProjectileGun::onUpdate(float timeStep, hkpWorld* world, const hkpRigidBody* characterBody,
	const hkTransform& viewTransform, bool fire, bool fireRmb) {
	updateProjectiles(timeStep);
	clearHitProjectiles();
	removeDestroyedProjectiles();
}

void hkpProjectileGun::clearProjectiles() {
	int32_t m_size;
	hkpGunProjectile* v4;
	m_size = (int32_t)m_projectiles.m_size;
	int32_t v3 = 0;
	if (m_size <= 0) {
		m_projectiles.m_size = (uint32_t)0;
	} else {
		do {
			v4 = m_projectiles.m_data[v3];
			v4->destroy();
			v4->removeReference();
			++v3;
		} while (v3 < m_size);
		m_projectiles.m_size = (uint32_t)0;
	}
}

void hkpProjectileGun::updateProjectiles(float timeStep) {
	int32_t m_size = m_projectiles.m_size;
	for (int32_t i = 0; i < m_size; ++i) {
		m_projectiles.m_data[i]->update(timeStep);
	}
}

// Kept unregistered: its one exe caller, onUpdate (0xE44F30), calls removeDestroyedProjectiles at 0xE44F47 with the
// ecx this body leaves intact (the exe's body never writes ecx), which clang does not guarantee.
void hkpProjectileGun::clearHitProjectiles() {
	int32_t m_size = m_projectiles.m_size;
	for (int32_t i = 0; i < m_size; ++i) {
		hkpGunProjectile* v3 = m_projectiles.m_data[i];
		v3->m_flags.m_storage &= ~2u;
	}
}

void hkpProjectileGun::removeDestroyedProjectiles() {
	int32_t m_size = m_projectiles.m_size;
	for (int32_t i = 0; i < m_size; ++i) {
		hkpGunProjectile* v4 = m_projectiles.m_data[i];
		if (v4->m_flags.anyIsSet(hkpGunProjectile::DESTROYED)) {
			v4->removeReference();
			uint32_t v5 = --m_projectiles.m_size;
			if (v5 != i) {
				m_projectiles.m_data[i] = m_projectiles.m_data[v5];
			}
			--m_size;
			--i;
		}
	}
}

void hkpProjectileGun::destroyAgedProjectiles(float time) {
	int32_t m_size = m_projectiles.m_size;
	for (int32_t i = 0; i < m_size; ++i) {
		hkpGunProjectile* v5 = m_projectiles.m_data[i];
		if (time < static_cast<double>(v5->m_existanceTime)) {
			v5->destroy();
		}
	}
}

hkpGunProjectile* hkpProjectileGun::getFirstActiveProjectile() {
	hkpGunProjectile* result;
	int32_t m_size = m_projectiles.m_size;
	int32_t v2 = 0;
	if (m_size <= 0) {
		return 0;
	}
	for (hkpGunProjectile** i = static_cast<hkArrayBase<hkpGunProjectile*>&>(m_projectiles).begin();; ++i) {
		result = *i;
		if (!(*i)->m_flags.anyIsSet(hkpGunProjectile::DESTROYED)) {
			break;
		}
		if (++v2 >= m_size) {
			return 0;
		}
	}
	return result;
}

void hkpProjectileGun::addProjectile(hkpGunProjectile* proj) {
	proj->addReference();
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	if (m_projectiles.m_size == (m_projectiles.m_capacityAndFlags & hkArrayBase<hkpGunProjectile*>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*Instance->m_heap, &m_projectiles, sizeof(hkpGunProjectile*));
	}
	m_projectiles.m_data[m_projectiles.m_size++] = proj;
	proj->addToWorld(m_world);
}

const hkClass& hkpProjectileGun::staticClass() {
	return hkpProjectileGunClass;
}

const hkClass& hkpProjectileGun::staticClass_00DFCE30() {
	return hkpBallSocketChainDataClass;
}
