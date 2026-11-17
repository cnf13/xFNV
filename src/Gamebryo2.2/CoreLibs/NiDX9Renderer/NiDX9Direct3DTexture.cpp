#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

// Not named in the PDB or IDA; name chosen here. The RTTI record of the exe's NiDX9Direct3DTexture (a PC-only
// class, not in the Xbox PDB, with no vtable in the exe), derived from NiTexture.
static NiRTTI NiDX9Direct3DTexture_ms_RTTI("NiDX9Direct3DTexture", &NiTexture::ms_RTTI);
