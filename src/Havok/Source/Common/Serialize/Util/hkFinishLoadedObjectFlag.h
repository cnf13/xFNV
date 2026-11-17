#pragma once

#include <cstdint>

// The flag a loaded object's finish constructor reads: whether the object is being finished after a load.
class hkFinishLoadedObjectFlag {
public:
	int32_t m_finishing;
};
