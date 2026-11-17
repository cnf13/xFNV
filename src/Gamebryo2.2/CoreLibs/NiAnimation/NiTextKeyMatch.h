#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

// Gamebryo's text-key matcher (Gamebryo2.2\CoreLibs\NiAnimation\NiTextKeyMatch.h): the key a sequence callback waits
// for and the last key it matched. The exe holds it only through NiActorManager::TimelineData::m_spMatchObject; IDA
// knows no NiTextKeyMatch vtable or struct, so the members are the PDB's (Xbox offsets 0x8 and 0xC, no pointers
// before them) and IsKeyMatch has no exe body.
class NiTextKeyMatch : public NiRefObject {
public:
	virtual bool IsKeyMatch(const NiFixedString& kTextKey);

	NiFixedString m_kLastKeyMatched;
	NiFixedString m_kMatchKey;
};
