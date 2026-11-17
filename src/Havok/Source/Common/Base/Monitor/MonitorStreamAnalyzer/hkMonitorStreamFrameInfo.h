#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// Identifies the counters and byte range of one captured monitor frame.
class hkMonitorStreamFrameInfo {
public:
	enum AbsoluteTimeCounter : int32_t {
		ABSOLUTE_TIME_TIMER_0 = 0x0,
		ABSOLUTE_TIME_TIMER_1 = 0x1,
		ABSOLUTE_TIME_NOT_TIMED = -1,
	};

	hkMonitorStreamFrameInfo();

	static const hkClass& staticClass();

	const char* m_heading;
	int32_t m_indexOfTimer0;
	int32_t m_indexOfTimer1;
	hkEnum<AbsoluteTimeCounter, uint32_t> m_absoluteTimeCounter;
	float m_timerFactor0;
	float m_timerFactor1;
	int32_t m_threadId;
	int32_t m_frameStreamStart;
	int32_t m_frameStreamEnd;
};

// Guessed bound: the database gives none; the next data item starts 0x30 bytes on.
extern hkClass hkMonitorStreamFrameInfoClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkMonitorStreamFrameInfoTypeInfo;
