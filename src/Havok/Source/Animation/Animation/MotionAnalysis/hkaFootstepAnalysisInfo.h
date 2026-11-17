#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

// The finish function hkaFootstepAnalysisInfoTypeInfo stores.
void finishLoadedObjecthkaFootstepAnalysisInfo(void* p, int32_t finishing);

// Stores sampled bone motion and the corresponding footstep annotation names.
class hkaFootstepAnalysisInfo : public hkReferencedObject {
public:
	// Implicit default constructor: every member's own default leaves it destructible.
	hkaFootstepAnalysisInfo() = default;
	hkaFootstepAnalysisInfo(int32_t n, float duration, float posTol, float velTol, const char* name,
		const char* nameStrike, const char* nameLift, const char* nameLock, const char* nameUnlock);
	hkaFootstepAnalysisInfo(hkFinishLoadedObjectFlag flag);
	~hkaFootstepAnalysisInfo() override;

	void updatePosition(int32_t frame, float value);
	void updateVelocity(int32_t frame, float value);
	void updateBones(int32_t frame, hkBool isDown);

	static float getMax(const hkArrayBase<float>& a1);
	static const hkClass& staticClass();
	static float getMin(const hkArrayBase<float>& arr);
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkArray<char, hkContainerHeapAllocator> m_name;
	hkArray<char, hkContainerHeapAllocator> m_nameStrike;
	hkArray<char, hkContainerHeapAllocator> m_nameLift;
	hkArray<char, hkContainerHeapAllocator> m_nameLock;
	hkArray<char, hkContainerHeapAllocator> m_nameUnlock;
	hkArray<float, hkContainerHeapAllocator> m_minPos;
	hkArray<float, hkContainerHeapAllocator> m_maxPos;
	hkArray<float, hkContainerHeapAllocator> m_minVel;
	hkArray<float, hkContainerHeapAllocator> m_maxVel;
	hkArray<float, hkContainerHeapAllocator> m_allBonesDown;
	hkArray<float, hkContainerHeapAllocator> m_anyBonesDown;
	float m_posTol;
	float m_velTol;
	float m_duration;
};

extern hkClass hkaFootstepAnalysisInfoClass;
extern hkTypeInfo hkaFootstepAnalysisInfoTypeInfo;
