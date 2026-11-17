#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class hkMemoryExceptionTestingUtil;

// Tracks frame-local memory exception injection checks.
class hkMemoryExceptionTestingUtil {
public:
	enum : int32_t {
		MAX_CHECK_ID = 0x32,
	};

	hkMemoryExceptionTestingUtil();
	~hkMemoryExceptionTestingUtil();

	void startFrameImpl();
	void startNewDemoImpl();
	void endFrameImpl();
	void allowMemoryExceptionsImpl(bool allowMemExceptions);
	bool isMemoryAvailableImpl(int32_t id);

	static void destroy();
	static hkMemoryExceptionTestingUtil& getInstance();
	static void create();

	bool m_outOfMemory;
	uint8_t padding01[3];
	int32_t m_frameCounter;
	bool m_allowMemoryExceptions;
	bool m_wasCheckIdThrown[50];
	uint8_t padding3B;

	// IDA name; the PDB's is hkMemoryExceptionTestingUtil::s_instance. create (0xD88150) stores the object it
	// constructs here and getInstance (0xD880A0) returns it.
	static inline hkMemoryExceptionTestingUtil* s_instance;
};
