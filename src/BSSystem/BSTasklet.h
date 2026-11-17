#pragma once

#include "Reconstruction.hpp"

class BSTaskletData;

// A handle a tasklet group runs: the data whose Process the tasklet thread calls.
class BSTasklet {
public:
	BSTasklet();
	virtual ~BSTasklet();

	// Sets pData; the exe's shared one-field setter 0x6ECD40 (thiscall, ret 4).
	void SetTaskData(BSTaskletData* apData);

	// The exe's deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	BSTaskletData* pData;
};
inline constexpr fnvd::Vtable BSTasklet_vftable{};
