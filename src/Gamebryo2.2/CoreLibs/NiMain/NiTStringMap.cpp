#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Tile.h"
#include "BSShader/ShaderBuffer.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/IDLE_ANIM_ROOT.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreator.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/string.h"

#include <stdint.h>

using NiTStringTemplateMapBuildStorage =
	NiTStringTemplateMap<NiTPointerMap<const char*, Tile::BuildStorage*>, Tile::BuildStorage*>;
// NiTStringTemplateMap<NiTPointerMap<const char*, XMLStorage*>, XMLStorage*>::KeyToHashIndex and the int
// instantiation's (and their NiTStringPointerMap slots):

// The other pointer string maps (NiControllerSequence*, NiShader*, the create functions, ...) share the optimised
// body; the controller sequence map is their owner by name order.
using NiTStringTemplateMapControllerSequence =
	NiTStringTemplateMap<NiTPointerMap<const char*, NiControllerSequence*>, NiControllerSequence*>;

// The string map of strings (NiTStringMap<char*>, built on NiTMap) has its own copy of the debug-compiled body.
using NiTStringTemplateMapString = NiTStringTemplateMap<NiTMap<const char*, char*>, char*>;

using NiTStringTemplateMap_lNiTPointerMap_lconst_schar_p_cunsigned_sshort_g_cunsigned_sshort =
	NiTStringTemplateMap<NiTPointerMap<const char*, uint16_t>, uint16_t>;
