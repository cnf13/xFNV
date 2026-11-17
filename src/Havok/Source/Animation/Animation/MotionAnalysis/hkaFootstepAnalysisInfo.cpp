#include "Havok/Source/Animation/Animation/MotionAnalysis/hkaFootstepAnalysisInfo.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/MotionAnalysis/hkaFootstepAnalysisInfoContainer.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

#include <cmath>

// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
static const void* getVtablehkaFootstepAnalysisInfo();
static void cleanupLoadedObjecthkaFootstepAnalysisInfo(void* p);

// The class record's member table.
using hkaFootstepAnalysisInfoClassMemberArray = const hkClassMember[14];
static hkaFootstepAnalysisInfoClassMemberArray hkaFootstepAnalysisInfoClass_Members{
	{"name", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_CHAR}, 0, 0, 0x8, nullptr},
	{"nameStrike", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_CHAR}, 0, 0, 0x14, nullptr},
	{"nameLift", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_CHAR}, 0, 0, 0x20, nullptr},
	{"nameLock", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_CHAR}, 0, 0, 0x2C, nullptr},
	{"nameUnlock", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_CHAR}, 0, 0, 0x38, nullptr},
	{"minPos", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, 0, 0x44, nullptr},
	{"maxPos", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, 0, 0x50, nullptr},
	{"minVel", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, 0, 0x5C, nullptr},
	{"maxVel", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, 0, 0x68, nullptr},
	{"allBonesDown", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, 0, 0x74, nullptr},
	{"anyBonesDown", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, 0, 0x80, nullptr},
	{"posTol", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8C, nullptr},
	{"velTol", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x90, nullptr},
	{"duration", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x94, nullptr}};
hkClass hkaFootstepAnalysisInfoClass("hkaFootstepAnalysisInfo", &hkReferencedObjectClass, 152, nullptr, 0, nullptr, 0,
	hkaFootstepAnalysisInfoClass_Members, 14, nullptr, nullptr, 0, 1);
hkTypeInfo hkaFootstepAnalysisInfoTypeInfo = {"hkaFootstepAnalysisInfo", finishLoadedObjecthkaFootstepAnalysisInfo,
	cleanupLoadedObjecthkaFootstepAnalysisInfo, getVtablehkaFootstepAnalysisInfo(), &typeid(hkaFootstepAnalysisInfo),
	sizeof(hkaFootstepAnalysisInfo)};

// The type info's vtable getter for hkaFootstepAnalysisInfo: finish-constructs one on a 16-aligned stack object, not
// finishing, and returns its vtable.
static const void* getVtablehkaFootstepAnalysisInfo() {
	return hkGetVtableOf<hkaFootstepAnalysisInfo>();
}

static void cleanupLoadedObjecthkaFootstepAnalysisInfo(void* p) {
	static_cast<hkaFootstepAnalysisInfo*>(p)->~hkaFootstepAnalysisInfo();
}

// n frames of statistics: the position and velocity bounds at the opposite extremes (Havok's HK_REAL_MAX), no bone
// down at all frames and some bone down at every frame; then the five names copied with their terminators.
hkaFootstepAnalysisInfo::hkaFootstepAnalysisInfo(int32_t n, float duration, float posTol, float velTol,
	const char* name, const char* nameStrike, const char* nameLift, const char* nameLock, const char* nameUnlock)
	: m_name(), m_nameStrike(), m_nameLift(), m_nameLock(), m_nameUnlock(), m_minPos(), m_maxPos(), m_minVel(),
	  m_maxVel(), m_allBonesDown(), m_anyBonesDown(), m_posTol(posTol), m_velTol(velTol), m_duration(duration) {
	m_minPos.setSize(n);
	m_maxPos.setSize(n);
	m_minVel.setSize(n);
	m_maxVel.setSize(n);
	m_allBonesDown.setSize(n);
	m_anyBonesDown.setSize(n);
	for (int32_t i = 0; i < n; ++i) {
		m_minPos.m_data[i] = 3.40282e+38f;
		m_maxPos.m_data[i] = -3.40282e+38f;
		m_minVel.m_data[i] = 3.40282e+38f;
		m_maxVel.m_data[i] = -3.40282e+38f;
		m_allBonesDown.m_data[i] = 0.0f;
		m_anyBonesDown.m_data[i] = 1.0f;
	}
	m_name.setSize(hkString::strLen(name) + 1);
	hkString::strCpy(m_name.begin(), name);
	m_nameStrike.setSize(hkString::strLen(nameStrike) + 1);
	hkString::strCpy(m_nameStrike.begin(), nameStrike);
	m_nameLift.setSize(hkString::strLen(nameLift) + 1);
	hkString::strCpy(m_nameLift.begin(), nameLift);
	m_nameLock.setSize(hkString::strLen(nameLock) + 1);
	hkString::strCpy(m_nameLock.begin(), nameLock);
	m_nameUnlock.setSize(hkString::strLen(nameUnlock) + 1);
	hkString::strCpy(m_nameUnlock.begin(), nameUnlock);
}

// The name and statistics arrays go with their members.
hkaFootstepAnalysisInfo::~hkaFootstepAnalysisInfo() {}

void hkaFootstepAnalysisInfo::updatePosition(int32_t frame, float value) {
	double v6;
	int32_t framea;
	float* v4 = &m_minPos.m_data[frame];
	framea = *reinterpret_cast<int32_t*>(v4);
	double v5 = value;
	if (!(value > static_cast<double>(*reinterpret_cast<float*>(&framea)))) {
		v6 = value;
	} else {
		v5 = *reinterpret_cast<float*>(&framea);
		v6 = value;
	}
	float framec = v5;
	float* m_data = static_cast<hkArrayBase<float>&>(m_maxPos).begin();
	*v4 = framec;
	float* v8 = &m_data[frame];
	float frameb = *v8;
	if (!(frameb > v6)) {
		frameb = v6;
	}
	*v8 = frameb;
}

void hkaFootstepAnalysisInfo::updateVelocity(int32_t frame, float value) {
	float valuea = std::fabs((double)value);
	double v3 = valuea;
	float* v4 = &m_minVel.m_data[frame];
	double v5 = *v4;
	if (!(valuea > v5)) {
		v5 = valuea;
	}
	float valuec = v5;
	float* v6 = &m_maxVel.m_data[frame];
	*v4 = valuec;
	float framea = v3;
	float valueb = *v6;
	if (!(framea < static_cast<double>(valueb))) {
		valueb = v3;
	}
	*v6 = valueb;
}

void hkaFootstepAnalysisInfo::updateBones(int32_t frame, hkBool isDown) {
	if (isDown.m_bool) {
		m_anyBonesDown.m_data[frame] = 0.0;
	} else {
		m_allBonesDown.m_data[frame] = 1.0;
	}
}

float hkaFootstepAnalysisInfo::getMax(const hkArrayBase<float>& a1) {
	int32_t m_size;
	float* v4;
	uint32_t v5;
	float* v6;
	int32_t v7;
	float v10;
	float v11;
	float v12;
	float v13;
	float v14;
	float v9 = (-0x1.ffffdc0000000p+127f);
	m_size = (int32_t)(&a1)->m_size;
	int32_t v3 = 0;
	if (m_size >= 4) {
		v4 = (&a1)->m_data + 2;
		v5 = (((uint32_t)(m_size - 4)) >> 2) + 1;
		v3 = (int32_t)(4 * v5);
		do {
			v10 = *(v4 - 2);
			if (v10 >= ((double)v9)) {
				v9 = v10;
			}
			v11 = *(v4 - 1);
			if (v11 >= ((double)v9)) {
				v9 = v11;
			}
			v12 = *v4;
			if (v12 >= ((double)v9)) {
				v9 = v12;
			}
			v13 = v4[1];
			if (v13 >= ((double)v9)) {
				v9 = v13;
			}
			v4 += 4;
			--v5;
		} while (v5);
	}
	if (v3 < m_size) {
		v6 = &(&a1)->m_data[v3];
		v7 = m_size - v3;
		do {
			v14 = *v6;
			if (v14 >= ((double)v9)) {
				v9 = v14;
			}
			++v6;
			--v7;
		} while (v7);
	}
	return v9;
}

const hkClass& hkaFootstepAnalysisInfo::staticClass() {
	return hkaFootstepAnalysisInfoClass;
}

// The least value of arr, HK_REAL_MAX for an empty array; a NaN replaces the minimum, since the running minimum is kept
// only when a value compares above it.
float hkaFootstepAnalysisInfo::getMin(const hkArrayBase<float>& arr) {
	float minValue = 3.40282e+38f;
	for (int32_t i = 0; i < arr.m_size; ++i) {
		const float value = arr.m_data[i];
		if (!(value > minValue)) {
			minValue = value;
		}
	}
	return minValue;
}

hkaFootstepAnalysisInfo* hkaFootstepAnalysisInfoContainer::newPreviewInfo(int32_t numFrames, float duration,
	float posTol, float velTol, const char* name, const char* nameStrike, const char* nameLift, const char* nameLock,
	const char* nameUnlock) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkaFootstepAnalysisInfo* v12 =
		static_cast<hkaFootstepAnalysisInfo*>(Instance->m_heap->blockAlloc(sizeof(hkaFootstepAnalysisInfo)));
	v12->m_memSizeAndFlags = sizeof(hkaFootstepAnalysisInfo);
	hkaFootstepAnalysisInfo* v13 = ::new (v12)
		hkaFootstepAnalysisInfo(numFrames, duration, posTol, velTol, name, nameStrike, nameLift, nameLock, nameUnlock);
	hkReferencedObject* v14 = v13;
	if (v13) {
		v13->addReference();
	}
	hkArray<hkRefPtr<hkaFootstepAnalysisInfo>, hkContainerHeapAllocator>* p_m_previewInfo = &m_previewInfo;
	hkMemoryRouter* v16 = &hkMemoryRouter::getInstance();
	if (m_previewInfo.m_size ==
		(m_previewInfo.m_capacityAndFlags & hkArrayBase<hkRefPtr<hkaFootstepAnalysisInfo>>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*v16->m_heap, &m_previewInfo, sizeof(hkRefPtr<hkaFootstepAnalysisInfo>));
	}
	hkRefPtr<hkaFootstepAnalysisInfo>* v17 = &p_m_previewInfo->m_data[m_previewInfo.m_size];
	if (v17) {
		if (v14) {
			v14->addReference();
		}
		v17->m_pntr = static_cast<hkaFootstepAnalysisInfo*>(v14);
	}
	++m_previewInfo.m_size;
	v14->removeReference();
	hkaFootstepAnalysisInfo* m_pntr = p_m_previewInfo->m_data[m_previewInfo.m_size - 1].val();
	if (v14) {
		v14->removeReference();
	}
	return m_pntr;
}
