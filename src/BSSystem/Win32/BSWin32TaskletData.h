#pragma once

#include "BSSystem/BSTaskletData.h"
#include "Reconstruction.hpp"

class BSWin32TaskletGroupData;

// IDA name; not in the PDB. The PC tasklet data, the counterpart of the PDB's BSXenonTaskletData. Only the
// destructor is declared.
class BSWin32TaskletData : public BSTaskletData {
public:
	BSWin32TaskletData();
	~BSWin32TaskletData() override;

	// The PC fields, in the PC order (the Xbox class has pLink, pGroupData, bRunOnStartup and TaskDataLock).
	BSWin32TaskletGroupData* pGroupData;
	bool bRunOnStartup;
	uint8_t padding0D[3];
	// IDA: UInt32 TaskDataLock. The PDB types it BSNonReentrantSpinLock, a 4-byte lock class src does not have;
	// the exe initialises it (0x6C7850) and locks it (0xB00EC0) as a dword.
	uint32_t TaskDataLock;
	BSWin32TaskletData* pLink;
};
inline constexpr fnvd::Vtable BSWin32TaskletData_vftable{};
