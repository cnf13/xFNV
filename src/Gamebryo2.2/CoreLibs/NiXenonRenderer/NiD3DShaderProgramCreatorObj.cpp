#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorObj.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"

#include <cstdio>
#include <filesystem>
#include <system_error>

NiD3DShaderProgramCreatorObj::NiD3DShaderProgramCreatorObj() {}

NiD3DShaderProgramCreatorObj::~NiD3DShaderProgramCreatorObj() {}

// Opens the file read-only with OPEN_EXISTING ("rb"), and deliberately leaves it open when the allocation fails.
bool NiD3DShaderProgramCreatorObj::LoadShaderCodeFromFile(
    const char* pcFileName,
    void*& pvCode,
    uint32_t& uiCodeSize,
    void*& pvConstantData,
    uint32_t& uiConstantSize,
    int32_t bRecoverable
) {
    pvCode = nullptr;
    uiCodeSize = 0;
    pvConstantData = nullptr;
    uiConstantSize = 0;
    if (pcFileName && *pcFileName) {
        char FileName[260];
        if (!NiD3DShaderProgramFactory::ResolveShaderFileName(pcFileName, FileName, 260u)) {
            NiShaderFactory::ReportError(1u, bRecoverable, "Failed to find shader program file %s\n", pcFileName);
            return false;
        }
        const std::filesystem::path kPath = platform::NativePath(FileName);
        std::FILE* pFile = std::fopen(kPath.string().c_str(), "rb");
        if (!pFile) {
            NiShaderFactory::ReportError(1u, bRecoverable, "Invalid shader file %s\n", FileName);
            return false;
        }
        // GetFileSize's INVALID_FILE_SIZE (0xFFFFFFFF) when the size cannot be read.
        std::error_code kError;
        uint32_t dwSize = static_cast<uint32_t>(std::filesystem::file_size(kPath, kError));
        if (kError) {
            dwSize = 0xFFFFFFFF;
        }
        uint8_t* pbyData = static_cast<uint8_t*>(_NiMalloc(dwSize + 4));
        if (!pbyData) {
            return false;
        }
        std::memset(pbyData, 0, dwSize + 4);
        const uint32_t dwBytesRead = static_cast<uint32_t>(std::fread(pbyData, 1, dwSize, pFile));
        std::fclose(pFile);
        if (dwBytesRead != dwSize) {
            _NiFree(pbyData);
            return false;
        }
        uiCodeSize = dwSize;
        pvCode = pbyData;
        return true;
    }

    NiShaderFactory::ReportError(1u, bRecoverable, "Invalid shader file name\n");
    return false;
}

bool NiD3DShaderProgramCreatorObj::LoadShaderCodeFromBuffer(
    const void* pvBuffer,
    uint32_t uiBufferSize,
    void*& pvCode,
    uint32_t& uiCodeSize,
    void*& pvConstantData,
    uint32_t& uiConstantSize,
    int32_t bRecoverable
) {
    pvCode = nullptr;
    uiCodeSize = 0;
    pvConstantData = nullptr;
    uiConstantSize = 0;
    if (pvBuffer && uiBufferSize) {
        void* v8 = _NiMalloc(uiBufferSize);
        pvCode = v8;
        rt::memcpy(v8, pvBuffer, uiBufferSize);
        uiCodeSize = uiBufferSize;
        return true;
    }

    NiShaderFactory::ReportError(1u, bRecoverable, "Invalid shader buffer\n");
    return false;
}

// Creates the creator into ms_pkCreator and registers it for both extensions; ms_pkCreator is reloaded for the second
// registration.
void NiD3DShaderProgramCreatorObj::_SDMInit() {
    NiD3DShaderProgramCreatorObj* pkCreator = new NiD3DShaderProgramCreatorObj;
    ms_pkCreator = pkCreator;
    NiD3DShaderProgramFactory::RegisterShaderCreator("vso", pkCreator);
    NiD3DShaderProgramFactory::RegisterShaderCreator("pso", ms_pkCreator);
}
