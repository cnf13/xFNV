#pragma once

#include "Havok/Source/Common/Base/DebugUtil/Logger/hkLogger.h"

// A logger writing to an hkOstream. Only the virtual destructor is declared: hkBinaryTagfileReader's Reader deletes its
// log through it.
class hkOstreamLogger : public hkLogger {
public:
	~hkOstreamLogger() override;
};
