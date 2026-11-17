#pragma once

#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"
#include "Reconstruction.hpp"

#include <xmmintrin.h>

class bhkCharacterProxyCinfo;

// Bethesda's subclass of the Havok character proxy: it adds the pointer to the bhkCharacterProxy that owns it (the
// PDB puts it in bhkCharacterProxy.cpp). The exe allocates 0xD0 bytes for it.
class ahkpCharacterProxy : public hkpCharacterProxy {
public:
	ahkpCharacterProxy(const bhkCharacterProxyCinfo& arInfo);

	hkArray<hkpRootCdPoint>* GetCdPoints();
	// The proxy's shape phantom.
	hkpShapePhantom* GetShapePhantom();
	void RemoveEntityListeners();

	void* m_userData;
};
inline constexpr fnvd::Vtable ahkpCharacterProxy_vftable_hkpPhantomListener{};
inline constexpr fnvd::Vtable ahkpCharacterProxy_vftable_hkpEntityListener{};
inline constexpr fnvd::Vtable ahkpCharacterProxy_vftable{};
