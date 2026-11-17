#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkStreamWriter;

// Sends the Havok version and the protocol versions to a visual debugger client. The exe's sendVersionInformation is
// 0xD61290 (it writes the command byte 0x90, the two protocol version globals and the version string);
// hkVisualDebugger::createClient calls it. Statics only: sizeof 1 in Havok's release hkvisualize.pdb.
class hkVersionReporter {
public:
	// The command byte it sends (the SDK's value).
	enum Command : int32_t {
		HK_VERSION_INFORMATION = 0x90,
	};

	// Writes the version packet to connection.
	static hkResult sendVersionInformation(hkStreamWriter* connection);

	// The protocol version the packet sends first (7100 in the exe), then the oldest one it is compatible with (2300).
	static inline int32_t m_protocolVersion = 7100;
	static inline int32_t m_protocolMinimumCompatible = 2300;
};
