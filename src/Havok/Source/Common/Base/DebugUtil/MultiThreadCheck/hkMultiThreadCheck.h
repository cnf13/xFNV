#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Reconstruction.hpp"

// The marks a world object's methods leave while they read or write it, so that the engine can check the
// object is not touched from two threads at once.
class hkMultiThreadCheck {
public:
	// How a method touches the checked object.
	enum AccessType : int32_t {
		HK_ACCESS_IGNORE = 0,
		HK_ACCESS_RO = 1,
		HK_ACCESS_RW = 2,
	};

	// Whether a read mark covers the object only or what it holds too.
	enum ReadMode : int32_t {
		THIS_OBJECT_ONLY = 0,
		RECURSIVE = 1,
	};

	// The SDK's m_threadId values.
	enum : uint32_t {
		MARKED_RO = 0xFFFFFFE1,
		MARKED_RO_SELF_ONLY = 0xFFFFFFC1,
		UNMARKED = 0xFFFFFFF1,
	};

	// Sets the top bit of the word at +4 (m_markCount here), which turns the checks off.
	void disableChecks();
	void markForReadImpl(ReadMode mode);
	void markForWriteImpl();
	bool isMarkedForWriteImpl();
	void unmarkForReadImpl();
	void unmarkForWriteImpl();
	// Turns the checks back on where they are off, unmarking the object.
	void enableChecks();
	void reenableChecks();
	void accessCheck(hkMultiThreadCheck::AccessType type);

	static void staticInit();
	static void staticQuit();
	static void accessCheckWithParent(const hkMultiThreadCheck* parentLock, hkMultiThreadCheck::AccessType parentType,
		const hkMultiThreadCheck& lock, hkMultiThreadCheck::AccessType type);
	// Havok SDK signature (hkBase.lib, tied by target).
	static const hkClass& staticClass();

	uint32_t m_threadId;
	uint16_t m_markCount;
	uint16_t m_markBitStack;
};

inline _RTL_CRITICAL_SECTION* s_criticalSection;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkMultiThreadCheckTypeInfo;
extern hkClass hkMultiThreadCheckClass;
