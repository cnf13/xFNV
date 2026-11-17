#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkpCdBody;

// Collects the body pairs a collision query finds; m_earlyOut tells the query to stop. IDA's PC struct adds an
// hkArray of pairs at 0x8, but that belongs to hkpAllCdBodyPairCollector: its constructor (0x62D350) builds it,
// while this class's constructor (0x62D3C0) sets only the vtable and m_earlyOut.
class hkpCdBodyPairCollector {
public:
	hkpCdBodyPairCollector();
	virtual ~hkpCdBodyPairCollector();

	// slot 1; the exe's entry is the unimplemented-virtual stub (0xEC60FA)
	virtual void addCdBodyPair(const hkpCdBody& aBodyA, const hkpCdBody& aBodyB) = 0;
	// Clears the early-out flag. The exe's out-of-line copy (0x62D330) is registered in TESTrapListener.cpp.
	virtual void reset() { m_earlyOut = false; }

	// DeletingDestruct's free (hkpCdPointCollector::operator delete, 0x6210D0).
	static void operator delete(void* apMemory);

	hkBool m_earlyOut;
};
inline constexpr fnvd::Vtable hkpCdBodyPairCollector_vftable{};
