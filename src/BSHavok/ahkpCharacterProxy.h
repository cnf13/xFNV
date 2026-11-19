#pragma once

#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"

#include <xmmintrin.h>

class bhkCharacterProxy;
class bhkCharacterProxyCinfo;

// Bethesda's subclass of the Havok character proxy: it adds the pointer to the bhkCharacterProxy that owns it.
class ahkpCharacterProxy : public hkpCharacterProxy {
public:
    ahkpCharacterProxy(const bhkCharacterProxyCinfo& arInfo);

    hkArray<hkpRootCdPoint>* GetCdPoints();
    // The proxy's shape phantom.
    hkpShapePhantom* GetShapePhantom();
    void RemoveEntityListeners();

    bhkCharacterProxy* m_userData;
};
