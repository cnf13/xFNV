#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystemSDM.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystemDesc.h"

void NiSystemSDM::Init() {
    if (!NiSystemSDM::ms_bInitialized) {
        NiSystemSDM::ms_bInitialized = 1;
        NiMemManager::_SDMInit();
        NiSystemDesc::InitSystemDesc();
        OEI::IO::initialize();
    }
}

void NiSystemSDM::Shutdown() {
    if (NiSystemSDM::ms_bInitialized) {
        NiSystemSDM::ms_bInitialized = 0;
        OEI::IO::terminate();
        NiSystemDesc::ShutdownSystemDesc();
    }
}
