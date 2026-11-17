#pragma once

#include "BSSystem/Win32/BSWin32TaskletData.h"

class NiAVObject;
struct NiUpdateData;

// Updates a master particle system with the tasklet's update data.
class UpdateMasterPsysTaskletData : public BSWin32TaskletData {
public:
	UpdateMasterPsysTaskletData();
	~UpdateMasterPsysTaskletData() override;

	void Process() override;

	static void operator delete(void* apMemory);

	NiAVObject* pObj;
	NiUpdateData* pUpdateData;
};
