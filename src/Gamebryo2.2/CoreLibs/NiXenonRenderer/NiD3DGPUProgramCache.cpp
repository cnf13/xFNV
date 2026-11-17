#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DGPUProgramCache.h"

#include "BSShader/BSShaderManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgramCache.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialResource.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPtrSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStreamMacro.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DHLSLPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DHLSLVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorHLSL.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"
#include "SDK/MSVC/string.h"

// Gamebryo's constructor: the base cache, the type, material identifier and cache filename, the working directory,
// then the shader profile and the compile targets it allows, loading the cache when asked.
NiD3DGPUProgramCache::NiD3DGPUProgramCache(
    uint32_t auiVersion,
    const char* apcWorkingDir,
    NiGPUProgram::ProgramType aeType,
    const NiFixedString& akShaderProfile,
    const char* apcMaterialIdentifier,
    int32_t abAutoWriteToDisk,
    int32_t abWriteDebugFile,
    int32_t abLocked,
    int32_t abLoad
)
  : NiGPUProgramCache(auiVersion, abAutoWriteToDisk, abLocked), m_eType(aeType),
    m_bWriteDebugHLSLFile(static_cast<uint8_t>(abWriteDebugFile)) {
    m_kMaterialIdentifier = apcMaterialIdentifier;
    // NI_MAX_PATH.
    char acFilename[0x104];
    NiSprintf(acFilename, 0x104, "%s_%s_%s.cache", apcMaterialIdentifier, akShaderProfile.GetHandle(), "DX9");
    m_kFilename = acFilename;
    SetWorkingDirectory(NiFixedString(apcWorkingDir));
    m_kShaderProfile = akShaderProfile;

    auto AddTarget = [this](const char* apcTarget) { m_kValidTargets.Add(NiFixedString(apcTarget)); };
    if (aeType == NiGPUProgram::PROGRAM_VERTEX) {
        if (m_kShaderProfile.EqualsNoCase("vs_1_1")) {
            AddTarget("vs_1_1");
        }
        else if (m_kShaderProfile.EqualsNoCase("vs_2_0")) {
            AddTarget("vs_2_0");
            AddTarget("vs_1_1");
        }
        else if (m_kShaderProfile.EqualsNoCase("vs_2_a")) {
            AddTarget("vs_2_a");
            AddTarget("vs_2_0");
            AddTarget("vs_1_1");
        }
        else if (m_kShaderProfile.EqualsNoCase("vs_3_0")) {
            AddTarget("vs_3_0");
            AddTarget("vs_2_a");
            AddTarget("vs_2_0");
            AddTarget("vs_1_1");
        }
    }
    else if (aeType == NiGPUProgram::PROGRAM_PIXEL) {
        if (m_kShaderProfile.EqualsNoCase("ps_1_1")) {
            AddTarget("ps_1_1");
        }
        else if (m_kShaderProfile.EqualsNoCase("ps_1_2")) {
            AddTarget("ps_1_2");
            AddTarget("ps_1_1");
        }
        else if (m_kShaderProfile.EqualsNoCase("ps_1_3")) {
            AddTarget("ps_1_3");
            AddTarget("ps_1_2");
            AddTarget("ps_1_1");
        }
        else if (m_kShaderProfile.EqualsNoCase("ps_1_4")) {
            AddTarget("ps_1_4");
            AddTarget("ps_1_3");
            AddTarget("ps_1_2");
            AddTarget("ps_1_1");
        }
        else if (m_kShaderProfile.EqualsNoCase("ps_2_0")) {
            AddTarget("ps_2_0");
            AddTarget("ps_1_4");
            AddTarget("ps_1_3");
            AddTarget("ps_1_2");
            AddTarget("ps_1_1");
        }
        else if (m_kShaderProfile.EqualsNoCase("ps_2_a")) {
            AddTarget("ps_2_a");
            AddTarget("ps_2_0");
            AddTarget("ps_1_4");
            AddTarget("ps_1_3");
            AddTarget("ps_1_2");
            AddTarget("ps_1_1");
        }
        else if (m_kShaderProfile.EqualsNoCase("ps_2_b")) {
            AddTarget("ps_2_b");
            AddTarget("ps_2_0");
            AddTarget("ps_1_4");
            AddTarget("ps_1_3");
            AddTarget("ps_1_2");
            AddTarget("ps_1_1");
        }
        else if (m_kShaderProfile.EqualsNoCase("ps_3_0")) {
            AddTarget("ps_3_0");
            AddTarget("ps_2_a");
            AddTarget("ps_2_0");
            AddTarget("ps_1_4");
            AddTarget("ps_1_3");
            AddTarget("ps_1_2");
            AddTarget("ps_1_1");
        }
    }

    if (static_cast<uint8_t>(abLoad) && apcWorkingDir) {
        Load();
    }
}

NiD3DGPUProgramCache::~NiD3DGPUProgramCache() {}

// Builds the program's HLSL source with a #line directive naming the cache file, has the factory's HLSL creator compile
// it, then caches the program with its uniforms. Null when the cache is locked or the compile fails; a failed compile
// also clears the uniform set.
NiGPUProgram* NiD3DGPUProgramCache::GenerateProgram(
    const char* pcName,
    const char* pcProgramText,
    NiTObjectPtrSet<NiPointer<NiMaterialResource>>& kUniforms
) {
    if (m_bLocked) {
        return nullptr;
    }

    NiPointer<NiGPUProgram> spProgram;
    // EXE-INLINED none: strlen
    uint32_t uiProgramLength = static_cast<uint32_t>(__builtin_strlen(pcProgramText));
    uint32_t uiNewProgramLength = uiProgramLength + 0x208;
    char* pcNewProgramText = static_cast<char*>(_NiMalloc(uiNewProgramLength));
    // NI_MAX_PATH.
    char acFilename[0x104];

    // GetWorkingDirectory and GetPlatformSpecificCodeID are called through the vtable.
    NiGPUProgramCache* pkCache = this;
    if (m_eType == NiGPUProgram::PROGRAM_VERTEX) {
        const char* pcWorkingDir = pkCache->GetWorkingDirectory().GetHandle();
        const char* pcCodeID = pkCache->GetPlatformSpecificCodeID();
        NiSprintf(
            acFilename, 0x104, "%s\\%s\\%s-V.%s", pcWorkingDir, m_kMaterialIdentifier.GetHandle(), pcName, pcCodeID
        );
        NiSprintf(pcNewProgramText, uiNewProgramLength, "#line 2 \"%s\"\n%s", acFilename, pcProgramText);
        // EXE-INLINED none: strlen
        spProgram.SetNew(
            NiD3DShaderProgramFactory::CreateVertexShaderFromBuffer(
                pcNewProgramText, static_cast<uint32_t>(__builtin_strlen(pcNewProgramText)), acFilename, acFilename,
                "Main", m_kShaderProfile.GetHandle(), nullptr, 0, false, true
            )
        );
    }
    else if (m_eType == NiGPUProgram::PROGRAM_PIXEL) {
        const char* pcWorkingDir = pkCache->GetWorkingDirectory().GetHandle();
        const char* pcCodeID = pkCache->GetPlatformSpecificCodeID();
        NiSprintf(
            acFilename, 0x104, "%s\\%s\\%s-P.%s", pcWorkingDir, m_kMaterialIdentifier.GetHandle(), pcName, pcCodeID
        );
        NiSprintf(pcNewProgramText, uiNewProgramLength, "#line 2 \"%s\"\n%s", acFilename, pcProgramText);
        // EXE-INLINED none: strlen
        spProgram.SetNew(
            NiD3DShaderProgramFactory::CreatePixelShaderFromBuffer(
                pcNewProgramText, static_cast<uint32_t>(__builtin_strlen(pcNewProgramText)), acFilename, acFilename,
                "Main", m_kShaderProfile.GetHandle(), true
            )
        );
    }

    if (m_bWriteDebugHLSLFile) {
        SaveDebugHLSLFile(acFilename, pcNewProgramText);
    }

    if (spProgram && PruneUnusedConstants(spProgram, kUniforms)) {
        NiGPUProgramCache::NiGPUProgramDesc* pkDesc = nullptr;
        void* pvMemory = NiMemObject::operator new(sizeof(NiGPUProgramCache::NiGPUProgramDesc));
        if (pvMemory) {
            pkDesc = ::new (pvMemory) NiGPUProgramCache::NiGPUProgramDesc;
        }
        for (uint32_t ui = 0; ui < kUniforms.m_uiUsed; ++ui) {
            pkDesc->m_kUniforms.Add(kUniforms.m_pBase[ui]);
        }

        // EXE-INLINED none: NiPointer<NiGPUProgram>::operator=
        NiGPUProgram* pkOldProgram = pkDesc->m_spProgram;
        NiGPUProgram* pkNewProgram = spProgram;
        if (pkOldProgram != pkNewProgram) {
            if (pkOldProgram) {
                if (!InterlockedDecrementUInt32(&pkOldProgram->m_uiRefCount)) {
                    pkOldProgram->DeleteThis();
                }
            }
            pkDesc->m_spProgram.m_pObject = pkNewProgram;
            if (pkNewProgram) {
                InterlockedIncrementUInt32(&pkNewProgram->m_uiRefCount);
            }
        }

        // EXE-INLINED 0x438170: NiFixedString::NiFixedString(const char*)
        NiFixedString kName;
        kName.SetHandle(pcName ? NiGlobalStringTable::AddString(pcName) : NiGlobalStringTable::NULL_STRING);

        if (m_bAutoWriteCacheToDisk) {
            AppendEntry(pkDesc, kName);
        }

        // The map's SetAt takes the value by value, so building the temporary NiPointer increments the desc's reference
        // count.
        m_kCachedPrograms.SetAt(kName, pkDesc);
    }

    // Reads the local's object, frees the text, then runs the local NiPointer's destructor where it returns.
    NiGPUProgram* pkProgram = spProgram;
    spProgram.m_pObject = nullptr;
    _NiFree(pcNewProgramText);
    if (pkProgram) {
        pkProgram->DecRefCount();
        return pkProgram;
    }
    kUniforms.RemoveAll();
    return nullptr;
}

const char* NiD3DGPUProgramCache::GetPlatformSpecificCodeID() const {
    return "hlsl";
}

const char* NiD3DGPUProgramCache::GetPlatform() const {
    return "DX9";
}

// Writes the program's shader type (its RTTI name), the code size and the compiled code.
bool NiD3DGPUProgramCache::SaveGPUProgram(NiBinaryStream& kStream, NiGPUProgram* pkProgram) {
    // The source's downcast: this cache's programs are D3D shader programs.
    NiD3DShaderProgram* pkShader = static_cast<NiD3DShaderProgram*>(pkProgram);
    if (!pkShader) {
        return false;
    }
    const NiRTTI* pkRTTI = pkShader->GetRTTI();
    SaveString(kStream, pkRTTI->GetName());
    uint32_t uiCodeSize = pkShader->GetCodeSize();
    const uint8_t* pbCode = static_cast<const uint8_t*>(pkShader->GetCode());
    NiStreamSaveBinary(kStream, uiCodeSize);
    NiStreamSaveBinary(kStream, pbCode, uiCodeSize);
    return true;
}

// Reads one program from the cache stream: the shader type name, the code size and the compiled code, then wraps the
// code in a vertex or pixel shader through the HLSL creator and inserts it into the factory's map. bSkip jumps over
// the code instead of reading it. False for a shader type that is neither, or when the buffer cannot be created.
bool NiD3DGPUProgramCache::LoadGPUProgram(
    NiBinaryStream& kStream,
    const NiFixedString& kName,
    NiGPUProgram*& pkProgram,
    bool bSkip
) {
    pkProgram = nullptr;
    uint32_t uiCodeSize = 0;

    char acShaderType[256];
    LoadString(kStream, acShaderType, 256);

    bool bHLSLVertex;
    if (NiStricmp(acShaderType, NiD3DHLSLVertexShader::ms_RTTI.m_pcName) == 0) {
        bHLSLVertex = true;
    }
    else if (NiStricmp(acShaderType, NiD3DHLSLPixelShader::ms_RTTI.m_pcName) == 0) {
        bHLSLVertex = false;
    }
    else {
        return false;
    }

    // EXE-INLINED none: NiBinaryStream::BinaryRead
    uint32_t uiComponentSize = 4;
    kStream.BinaryRead(&uiCodeSize, 4, &uiComponentSize, 1);

    if (bSkip) {
        kStream.Seek(static_cast<int32_t>(uiCodeSize));
        return true;
    }

    ID3DXBuffer* pkBuffer = nullptr;
    if (D3DXCreateBuffer_Thunk(uiCodeSize, &pkBuffer) < 0) {
        return false;
    }

    // EXE-INLINED none: NiBinaryStream::BinaryRead
    uint32_t uiBufferComponentSize = 1;
    kStream.BinaryRead(pkBuffer->GetBufferPointer(), uiCodeSize, &uiBufferComponentSize, 1);

    // Loads the creator, the profile and the name before branching on the shader type.
    NiD3DShaderProgramCreatorHLSL* pkCreator = NiD3DShaderProgramCreatorHLSL::ms_pkCreator;
    NiGPUProgramCache* pkCache = this;
    const char* pcShaderProfile = pkCache->GetShaderProfile().GetHandle();
    const char* pcShaderName = kName.GetHandle();
    if (bHLSLVertex) {
        NiD3DVertexShader* pkVertexShader = pkCreator->CreateVertexShaderFromD3DXBuffer(
            pkBuffer, uiCodeSize, pcShaderName, "Main", pcShaderProfile, nullptr, 0, false, false
        );
        pkProgram = pkVertexShader;
        if (pkVertexShader) {
            NiD3DShaderProgramFactory::GetInstance()->InsertVertexShaderIntoMap(pkVertexShader);
            pkBuffer->Release();
            return true;
        }
    }
    else {
        NiD3DPixelShader* pkPixelShader = pkCreator->CreatePixelShaderFromD3DXBuffer(
            pkBuffer, uiCodeSize, pcShaderName, "Main", pcShaderProfile, false
        );
        pkProgram = pkPixelShader;
        if (pkPixelShader) {
            NiD3DShaderProgramFactory::GetInstance()->InsertPixelShaderIntoMap(pkPixelShader);
        }
    }
    pkBuffer->Release();
    return true;
}

// Writes pcProgramText to pcFilename, creating its directory first.
bool NiD3DGPUProgramCache::SaveDebugHLSLFile(const char* pcFilename, const char* pcProgramText) {
    if (!ValidateDirectory()) {
        return false;
    }
    char acPath[260];
    strcpy_s(acPath, 260, pcFilename);
    NiPath::Standardize(acPath);
    char* pcPathEnd = const_cast<char*>(std::strrchr(static_cast<const char*>(acPath), '/'));
    if (pcPathEnd || (pcPathEnd = const_cast<char*>(std::strrchr(static_cast<const char*>(acPath), '\\')))) {
        *pcPathEnd = '\0';
    }
    if (!NiFile::DirectoryExists(acPath) && !NiFile::CreateDirectoryRecursive(acPath)) {
        return false;
    }
    // The release build has no NiOutputDebugString calls here.
    NiFile* pkFile = NiFile::GetFile(pcFilename, NiFile::WRITE_ONLY, 0x8000);
    if (!pkFile) {
        return false;
    }
    // EXE-INLINED none: strlen
    pkFile->Write(pcProgramText, static_cast<uint32_t>(__builtin_strlen(pcProgramText)));
    delete pkFile;
    return true;
}

const NiRTTI* NiGPUProgramCache::GetRTTI() const {
    return &ms_RTTI;
}
