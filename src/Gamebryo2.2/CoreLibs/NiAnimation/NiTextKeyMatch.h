#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

// Gamebryo's text-key matcher: the key a sequence callback waits for and the last key it matched. It is held through
// NiActorManager::TimelineData::m_spMatchObject.
class NiTextKeyMatch : public NiRefObject {
public:
    virtual bool IsKeyMatch(const NiFixedString& kTextKey);

    NiFixedString m_kLastKeyMatched;
    NiFixedString m_kMatchKey;
};
