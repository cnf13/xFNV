#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"

NiRTTI NiGPUProgram::ms_RTTI("NiGPUProgram", nullptr);

// The exe has no out-of-line copy: NiD3DShaderProgram's destructor (0xE89840) inlines it down to NiRefObject's.
NiGPUProgram::~NiGPUProgram() {}
