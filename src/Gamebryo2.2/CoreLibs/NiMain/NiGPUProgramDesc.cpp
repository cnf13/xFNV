#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgramCache.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialResource.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPtrSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"

NiGPUProgramCache::NiGPUProgramDesc::NiGPUProgramDesc() : m_kUniforms(0) {}

NiGPUProgramCache::NiGPUProgramDesc::~NiGPUProgramDesc() {}
