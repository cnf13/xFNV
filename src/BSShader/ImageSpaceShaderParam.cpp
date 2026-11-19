#include "BSShader/ImageSpaceShaderParam.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"

ImageSpaceShaderParam::ImageSpaceShaderParam()
  : iFiltermode(NiTexturingProperty::FILTER_NEAREST), iClearFlags(NiRenderer::CLEAR_NONE),
    fClearColor(0.0f, 0.0f, 0.0f, 0.0f), pfVertexConstantGroup(nullptr), iVSize(0), pfPixelConstantGroup(nullptr),
    iPSize(0), spVertexConstantMap(nullptr), spPixelConstantMap(nullptr), bUseTexOffset{false, false, false},
    pInputTextures(0, 1), pInputFilterModes(0, 1), pRenderStates(37) {}

// The arrays, the render-state map and the constant maps are members the compiler destroys.
ImageSpaceShaderParam::~ImageSpaceShaderParam() {
    ClearConstantGroups();
}

void ImageSpaceShaderParam::InitConstantMap(int32_t aiVSize, int32_t aiPSize) {
    ClearConstantGroups();
    NiD3DShaderConstantMap* pMap =
        new NiD3DShaderConstantMap(BSShaderManager::pRenderer.m_pObject, NiGPUProgram::PROGRAM_VERTEX);
    spVertexConstantMap = pMap;
    pMap = new NiD3DShaderConstantMap(BSShaderManager::pRenderer.m_pObject, NiGPUProgram::PROGRAM_PIXEL);
    spPixelConstantMap = pMap;

    iVSize = aiVSize + 3;
    if (iVSize > 0) {
        if (pfVertexConstantGroup) {
            MemoryManager_Singleton.Deallocate(pfVertexConstantGroup);
        }
        // EXE-INLINED none: operator new[] (the overflow-checked float count, iVSize * 4)
        uint64_t ullBytes = uint64_t(uint32_t(iVSize * 4)) * sizeof(float);
        pfVertexConstantGroup = static_cast<float*>(
            MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
        );
        uint32_t uiRegisters = iVSize;
        spVertexConstantMap.m_pObject->AddEntry(
            "Constant Group", 0x10000000, 0, 0, uiRegisters, "", uiRegisters << 4, 4, pfVertexConstantGroup, false
        );
    }
    iPSize = aiPSize;
    if (iPSize > 0) {
        if (pfPixelConstantGroup) {
            MemoryManager_Singleton.Deallocate(pfPixelConstantGroup);
        }
        // EXE-INLINED none: operator new[] (the overflow-checked float count, iPSize * 4)
        uint64_t ullBytes = uint64_t(uint32_t(iPSize * 4)) * sizeof(float);
        pfPixelConstantGroup = static_cast<float*>(
            MemoryManager_Singleton.Allocate(ullBytes > UINT32_MAX ? size_t(-1) : size_t(ullBytes))
        );
        uint32_t uiRegisters = iPSize;
        spPixelConstantMap.m_pObject->AddEntry(
            "Constant Group", 0x10000000, 0, 0, uiRegisters, "", uiRegisters << 4, 4, pfPixelConstantGroup, false
        );
    }
}

void ImageSpaceShaderParam::ClearConstantGroups() {
    NiD3DShaderConstantMap* m_pObject;
    m_pObject = spVertexConstantMap.m_pObject;
    if (m_pObject) {
        m_pObject->DecRefCount();
        spVertexConstantMap.m_pObject = nullptr;
    }
    spPixelConstantMap = nullptr;
    if (pfVertexConstantGroup) {
        (&MemoryManager_Singleton)->Deallocate(pfVertexConstantGroup);
        pfVertexConstantGroup = nullptr;
    }
    if (pfPixelConstantGroup) {
        (&MemoryManager_Singleton)->Deallocate(pfPixelConstantGroup);
        pfPixelConstantGroup = nullptr;
    }
}

void ImageSpaceShaderParam::SetRenderState(uint32_t auiState, uint32_t auiValue) {
    pRenderStates.SetAt(auiState, auiValue);
}

void ImageSpaceShaderParam::CopyConstantGroup(int32_t a2, void* pvSrc, uint32_t stCount) {
    rt::memcpy(&pfPixelConstantGroup[4 * a2], pvSrc, stCount);
}

void ImageSpaceShaderParam::SetPixelConstant(int32_t aiIndex, float afVal0, float afVal1, float afVal2, float afVal3) {
    float* pfPixelConstantGroup;
    pfPixelConstantGroup = this->pfPixelConstantGroup;
    int32_t v7 = 4 * aiIndex;
    pfPixelConstantGroup[v7] = afVal0;
    pfPixelConstantGroup[v7 + 1] = afVal1;
    pfPixelConstantGroup[v7 + 2] = afVal2;
    pfPixelConstantGroup[v7 + 3] = afVal3;
}

void ImageSpaceShaderParam::SetVertexConstant(int32_t aiIndex, float afVal0, float afVal1, float afVal2, float afVal3) {
    float* pfConstant = pfVertexConstantGroup + (aiIndex * 4 + 12);
    pfConstant[0] = afVal0;
    pfConstant[1] = afVal1;
    pfConstant[2] = afVal2;
    pfConstant[3] = afVal3;
}

void ImageSpaceShaderParam::SetTextureOffsets(int32_t auiTexture, int32_t auiWidth, int32_t auiHeight) {
    float* pfVertexConstantGroup;
    int32_t v6;
    float auiTexturea;
    float auiWidtha;
    if (auiTexture < 3) {
        if ((!auiTexture) || bUseTexOffset[auiTexture]) {
            auiTexturea = (float)(0.5 / ((double)((uint32_t)auiWidth)));
            auiWidtha = (float)(0.5 / ((double)((uint32_t)auiHeight)));
        }
        else {
            auiTexturea = (float)0.0;
            auiWidtha = (float)0.0;
        }
        pfVertexConstantGroup = this->pfVertexConstantGroup;
        v6 = 4 * auiTexture;
        pfVertexConstantGroup[v6] = auiTexturea;
        pfVertexConstantGroup[v6 + 1] = auiWidtha;
        pfVertexConstantGroup[v6 + 2] = (float)0.0;
        pfVertexConstantGroup[v6 + 3] = (float)0.0;
    }
}

void ImageSpaceShaderParam::SetBlurClearMode() {
    iClearFlags = static_cast<NiRenderer::ClearFlags>(BSShaderManager::bSLIMode ? 7 : 0);
}

void ImageSpaceShaderParam::SetPixelConstants(int32_t registerIndex, NiVector4* a3) {
    float* pfPixelConstantGroup;
    float z;
    float w;
    pfPixelConstantGroup = this->pfPixelConstantGroup;
    float a3a = a3->y;
    z = a3->z;
    int32_t v5 = 4 * registerIndex;
    w = a3->w;
    pfPixelConstantGroup[v5] = a3->x;
    pfPixelConstantGroup[v5 + 1] = a3a;
    pfPixelConstantGroup[v5 + 2] = z;
    pfPixelConstantGroup[v5 + 3] = w;
}

void ImageSpaceShaderParam::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
