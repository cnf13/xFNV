#include "Havok/Source/Physics/Vehicle/TyreMarks/hkpTyremarksInfo.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"
#include "SDK/MSVC/string.h"

#include <xmmintrin.h>

static void cleanupLoadedObjecthkpTyremarkPoint(void* p);
static void finishLoadedObjecthkpTyremarkPoint(void* p, int32_t finishing);
static void finishLoadedObjecthkpTyremarksWheel(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpTyremarksWheel();
static void finishLoadedObjecthkpTyremarksInfo(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpTyremarksInfo();

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpTyremarkPointTypeInfo = {"hkpTyremarkPoint", finishLoadedObjecthkpTyremarkPoint,
	cleanupLoadedObjecthkpTyremarkPoint, nullptr, &typeid(hkpTyremarkPoint), sizeof(hkpTyremarkPoint)};

// The reflected members of hkpTyremarkPoint.
using hkpTyremarkPointClass_MembersArray = const hkClassMember[2];
static hkpTyremarkPointClass_MembersArray hkpTyremarkPointClass_Members{
	{"pointLeft", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
	{"pointRight", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr}};
hkClass hkpTyremarkPointClass("hkpTyremarkPoint", nullptr, 32, nullptr, 0, nullptr, 0, hkpTyremarkPointClass_Members, 2,
	nullptr, nullptr, 0, 0);

// The reflected members of hkpTyremarksWheel.
using hkpTyremarksWheelClass_MembersArray = const hkClassMember[3];
static hkpTyremarksWheelClass_MembersArray hkpTyremarksWheelClass_Members{
	{"currentPosition", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8,
		nullptr},
	{"numPoints", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
	{"tyremarkPoints", &hkpTyremarkPointClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT}, 0,
		{0}, 0x10, nullptr}};
hkClass hkpTyremarksWheelClass("hkpTyremarksWheel", &hkReferencedObjectClass, 28, nullptr, 0, nullptr, 0,
	hkpTyremarksWheelClass_Members, 3, nullptr, nullptr, 0, 0);
hkTypeInfo hkpTyremarksWheelTypeInfo = {"hkpTyremarksWheel", finishLoadedObjecthkpTyremarksWheel,
	cleanupLoadedObjecthkpTyremarksWheel, getVtablehkpTyremarksWheel(), &typeid(hkpTyremarksWheel),
	sizeof(hkpTyremarksWheel)};

// The reflected members of hkpTyremarksInfo.
using hkpTyremarksInfoClass_MembersArray = const hkClassMember[3];
static hkpTyremarksInfoClass_MembersArray hkpTyremarksInfoClass_Members{
	{"minTyremarkEnergy", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8,
		nullptr},
	{"maxTyremarkEnergy", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC,
		nullptr},
	{"tyremarksWheel", &hkpTyremarksWheelClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0,
		{0}, 0x10, nullptr}};
hkClass hkpTyremarksInfoClass("hkpTyremarksInfo", &hkReferencedObjectClass, 28, nullptr, 0, nullptr, 0,
	hkpTyremarksInfoClass_Members, 3, nullptr, nullptr, 0, 0);
hkTypeInfo hkpTyremarksInfoTypeInfo = {"hkpTyremarksInfo", finishLoadedObjecthkpTyremarksInfo,
	cleanupLoadedObjecthkpTyremarksInfo, getVtablehkpTyremarksInfo(), &typeid(hkpTyremarksInfo),
	sizeof(hkpTyremarksInfo)};

static void cleanupLoadedObjecthkpTyremarkPoint(void* p) {
	;
}

static void finishLoadedObjecthkpTyremarkPoint(void* p, int32_t finishing) {
	;
}

// The type info's finish-loaded-object callback for hkpTyremarksWheel: runs its finish constructor.
static void finishLoadedObjecthkpTyremarksWheel(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpTyremarksWheel(flag);
	}
}

static const void* getVtablehkpTyremarksWheel() {
	return hkGetVtableOf<hkpTyremarksWheel>();
}

// The type info's finish-loaded-object callback for hkpTyremarksInfo: runs its finish constructor.
static void finishLoadedObjecthkpTyremarksInfo(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpTyremarksInfo(flag);
	}
}

static const void* getVtablehkpTyremarksInfo() {
	return hkGetVtableOf<hkpTyremarksInfo>();
}

void cleanupLoadedObjecthkpTyremarksWheel(void* p) {
	static_cast<hkpTyremarksWheel*>(p)->~hkpTyremarksWheel();
}

void cleanupLoadedObjecthkpTyremarksInfo(void* p) {
	static_cast<hkpTyremarksInfo*>(p)->~hkpTyremarksInfo();
}

hkpTyremarkPoint::hkpTyremarkPoint() {
	m_pointLeft.setZero4();
	m_pointRight.setZero4();
}

float hkpTyremarkPoint::getTyremarkStrength() {
	return m_pointRight.m_quad[3];
}

const hkClass& hkpTyremarkPoint::staticClass() {
	return hkpTyremarkPointClass;
}

hkpTyremarksWheel::hkpTyremarksWheel() : m_currentPosition(0), m_numPoints(0) {}

// The points array is freed by its own destructor.
hkpTyremarksWheel::~hkpTyremarksWheel() {}

const hkpTyremarkPoint& hkpTyremarksWheel::getTyremarkPoint(int32_t point) const {
	return m_tyremarkPoints.m_data[(point + m_currentPosition) % m_numPoints];
}

void hkpTyremarksWheel::addTyremarkPoint(hkpTyremarkPoint* a1) {
	if (0.0f != a1->getTyremarkStrength() ||
		m_tyremarkPoints.m_data[(m_currentPosition + m_numPoints - 1) % m_numPoints].getTyremarkStrength() != 0.0f) {
		m_tyremarkPoints.m_data[m_currentPosition] = *a1;
		m_currentPosition = (m_currentPosition + 1) % m_numPoints;
	}
}

void hkpTyremarksWheel::setNumPoints(int32_t num_points) {
	hkpTyremarkPoint v8;
	hkpTyremarksWheel* v2 = this;
	m_numPoints = num_points;
	m_currentPosition = 0;
	int32_t v6 = 0;
	if (num_points > 0) {
		hkArray<hkpTyremarkPoint, hkContainerHeapAllocator>* p_m_tyremarkPoints = &m_tyremarkPoints;
		do {
			v8.~hkpTyremarkPoint();
			::new (&v8) hkpTyremarkPoint;
			hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
			if (p_m_tyremarkPoints->m_size ==
				(p_m_tyremarkPoints->m_capacityAndFlags & hkArrayBase<hkpTyremarkPoint>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_tyremarkPoints, sizeof(hkpTyremarkPoint));
			}
			hkpTyremarkPoint* v5 = &p_m_tyremarkPoints->m_data[p_m_tyremarkPoints->m_size];
			if (v5) {
				rt::memcpy(v5, &v8, sizeof(hkpTyremarkPoint));
				v2 = this;
			}
			++p_m_tyremarkPoints->m_size;
			++v6;
		} while (v6 < v2->m_numPoints);
	}
}

const hkClass& hkpTyremarksWheel::staticClass() {
	return hkpTyremarksWheelClass;
}

// One wheel history of num_points points per wheel of data.
hkpTyremarksInfo::hkpTyremarksInfo(const hkpVehicleData& data, int32_t num_points)
	: m_minTyremarkEnergy(0.0f), m_maxTyremarkEnergy(0.0f) {
	m_tyremarksWheel.setSize(data.m_numWheels);
	for (int32_t i = 0; i < m_tyremarksWheel.m_size; ++i) {
		hkpTyremarksWheel* pWheel = new (hkReferencedObject::newObject(sizeof(hkpTyremarksWheel))) hkpTyremarksWheel();
		pWheel->setNumPoints(num_points);
		m_tyremarksWheel.m_data[i] = pWheel;
	}
}

// Drops this object's reference to each wheel's history; the array is freed by its own destructor.
hkpTyremarksInfo::~hkpTyremarksInfo() {
	for (int32_t i = 0; i < m_tyremarksWheel.m_size; ++i) {
		m_tyremarksWheel.m_data[i]->removeReference();
	}
}

void hkpTyremarksInfo::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	for (int32_t i = 0; i < m_tyremarksWheel.m_size; ++i) {
		hkpTyremarksWheel* v5 = m_tyremarksWheel.m_data[i];
		int32_t m_capacityAndFlags = v5->m_tyremarkPoints.m_capacityAndFlags;
		hkArray<hkpTyremarkPoint, hkContainerHeapAllocator>* p_m_tyremarkPoints = &v5->m_tyremarkPoints;
		if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
			int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(p_m_tyremarkPoints->begin(),
				static_cast<int32_t>(sizeof(*p_m_tyremarkPoints->begin())) * m_capacityAndFlags);
			collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "Points", p_m_tyremarkPoints->begin(),
				static_cast<int32_t>(sizeof(*p_m_tyremarkPoints->begin())) * p_m_tyremarkPoints->m_size, AllocatedSize);
		}
	}
}

// One tyre mark point per wheel from its contact: the contact point lifted 0.05 along the separating normal, moved half
// the wheel's width either way along the chassis's rotated m_chassisOrientation column 2 and by the chassis's linear
// velocity times the time step; w is the mark's strength, the skid energy density above m_minTyremarkEnergy scaled to
// 0..255.
void hkpTyremarksInfo::updateTyremarksInfo(float timestep, const hkpVehicleInstance* vehicle) {
	hkpEntity* pChassis = vehicle->m_entity;
	const __m128 vStep = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(timestep), _mm_set_ss(timestep), 0),
		pChassis->m_motion.m_linearVelocity.m_quad);
	hkVector4 vRight;
	vRight.setRotatedDir(pChassis->m_motion.m_motionState.m_transform.m_rotation,
		vehicle->m_data->m_chassisOrientation.m_col2);
	const __m128 vStepY = _mm_shuffle_ps(vStep, vStep, 0x55);
	const __m128 vStepZ = _mm_shuffle_ps(vStep, vStep, 0xAA);
	const __m128 vLift = _mm_set_ss(0.05f);
	for (int32_t i = 0; i < vehicle->m_data->m_numWheels; ++i) {
		const hkpVehicleInstance::WheelInfo& wheelInfo = vehicle->m_wheelsInfo.m_data[i];
		const float fEnergy = wheelInfo.m_skidEnergyDensity - m_minTyremarkEnergy;
		float fStrength;
		if (fEnergy > 0.0f) {
			fStrength = fEnergy * (255.0 / (m_maxTyremarkEnergy - m_minTyremarkEnergy));
			if (fStrength > 255.0) {
				fStrength = 255.0f;
			}
		} else {
			fStrength = 0.0f;
		}
		const float fWidth = vehicle->m_data->m_wheelParams.m_data[i].m_width;
		const float fLeftOffset = -0.5 * fWidth;
		const float fRightOffset = fWidth * 0.5;
		const __m128 vCenter =
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(vLift, vLift, 0), wheelInfo.m_contactPoint.m_separatingNormal.m_quad),
				wheelInfo.m_contactPoint.m_position.m_quad);
		hkVector4 vRightEdge;
		vRightEdge.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fRightOffset), _mm_set_ss(fRightOffset), 0), vRight.m_quad),
				vCenter);
		vRightEdge.m_quad[0] = vRightEdge.m_quad[0] + vStep[0];
		vRightEdge.m_quad[3] = vRightEdge.m_quad[3] + vStep[3];
		hkpTyremarkPoint point;
		point.~hkpTyremarkPoint();
		::new (&point) hkpTyremarkPoint;
		point.m_pointLeft.m_quad =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fLeftOffset), _mm_set_ss(fLeftOffset), 0),
									  vRight.m_quad),
						   vCenter),
				vStep);
		point.m_pointLeft.m_quad[3] = fStrength;
		point.m_pointRight.m_quad = vRightEdge.m_quad;
		point.m_pointRight.m_quad[1] =
			_mm_add_ss(_mm_shuffle_ps(vRightEdge.m_quad, vRightEdge.m_quad, 0x55), vStepY)[0];
		point.m_pointRight.m_quad[2] =
			_mm_add_ss(_mm_shuffle_ps(vRightEdge.m_quad, vRightEdge.m_quad, 0xAA), vStepZ)[0];
		point.m_pointRight.m_quad[3] = fStrength;
		m_tyremarksWheel.m_data[i]->addTyremarkPoint(&point);
	}
}

void hkpTyremarksInfo::getWheelTyremarksStrips(const hkpVehicleInstance* vehicle, int32_t wheel,
	hkVector4* strips_out) const {
	hkpTyremarksWheel* v4 = m_tyremarksWheel.m_data[wheel];
	int32_t m_numPoints = v4->m_numPoints;
	for (int32_t i = 0; i < m_numPoints; strips_out += 2) {
		const hkpTyremarkPoint& tyremarkPoint = v4->getTyremarkPoint(i++);
		strips_out[0] = tyremarkPoint.m_pointLeft;
		strips_out[1] = tyremarkPoint.m_pointRight;
	}
}

const hkClass& hkpTyremarksInfo::staticClass() {
	return hkpTyremarksInfoClass;
}
