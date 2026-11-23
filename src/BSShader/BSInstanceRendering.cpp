#include "BSShader/BSInstanceRendering.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"

// One previous-texture slot per texture blend stage, from the engine's heap.
BSInstanceRendering::BSInstanceRendering()
  : pPrevTextures(
        static_cast<NiTexture**>(
            MemoryManager::Instance().Allocate(sizeof(NiTexture*) * NiD3DPass::ms_uiMaxTextureBlendStages)
        )
    ) {}

BSInstanceRendering::~BSInstanceRendering() {
    MemoryManager::Instance().Deallocate(pPrevTextures);
}
