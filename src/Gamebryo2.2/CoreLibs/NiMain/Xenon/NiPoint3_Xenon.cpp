#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

void NiPoint3::_SDMShutdown() {
	_NiFree(NiPoint3::ms_pSqrtTable);
	NiPoint3::ms_pSqrtTable = nullptr;
}

void NiPoint3::_SDMInit() {
	NiPoint3::ms_pSqrtTable = NiPoint3::InitSqrtTable();
}
