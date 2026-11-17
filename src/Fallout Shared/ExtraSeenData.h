#pragma once

#include "Fallout Shared/ExtraDataList.h"
#include "Reconstruction.hpp"

class SeenData;

// The extra data that carries a SeenData pointer. The PDB places it in ExtraDataCell.h.
class ExtraSeenData : public BSExtraData {
public:
	ExtraSeenData();
	~ExtraSeenData() override;

	// The exe's deleting destructor frees through ourFree (BSCore/MemoryManager.h).
	static void operator delete(void* apMemory);

	SeenData* pSeenData;
};
inline constexpr fnvd::Vtable ExtraSeenData_vftable{};
