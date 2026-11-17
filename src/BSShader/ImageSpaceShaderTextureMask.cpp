#include "BSShader/ImageSpaceShaderTextureMask.h"

#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

ImageSpaceShaderTextureMask::ImageSpaceShaderTextureMask() : pWindowTexture(nullptr) {
    pTextures.SetSize(2u);
    pWindowTexture = static_cast<NiSourceTexture*>(BSShaderManager::spWindowEnvironmentMap01.m_pObject);
}

// The pWindowTexture member's destructor releases the window texture.
ImageSpaceShaderTextureMask::~ImageSpaceShaderTextureMask() {}
