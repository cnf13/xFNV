#pragma once

#include "Havok/Source/Common/Base/DebugUtil/Logger/hkLogger.h"
#include "Reconstruction.hpp"

// A logger writing to an hkOstream (the PDB's hkOstreamLogger : hkLogger). Only what the reconstructed code uses is
// declared: hkBinaryTagfileReader's Reader deletes its log through the virtual destructor (slot 0).
class hkOstreamLogger : public hkLogger {
public:
	~hkOstreamLogger() override;
};
