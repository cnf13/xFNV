#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgramCache.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiDDSReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialResource.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStreamMacro.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"
#include "SDK/Windows/winbase.h"

using NiGPUProgramCache_ProgramMap = NiTFixedStringMapBase<
    DFALL<NiTMapItem<NiFixedString, NiPointer<NiGPUProgramCache::NiGPUProgramDesc>>>,
    NiPointer<NiGPUProgramCache::NiGPUProgramDesc>>;

NiRTTI NiGPUProgramCache::ms_RTTI("NiGPUProgramCache", nullptr);

// The destructor only runs the members' destructors (the target set, the four strings, the map).
NiGPUProgramCache::~NiGPUProgramCache() {}

// Gamebryo's source: the cached entry's uniforms are appended to kUniforms and its program returned. The program
// is read before the entry's release; the key's handle is released last.
NiGPUProgram*
NiGPUProgramCache::FindCachedProgram(const char* pcName, NiTObjectPtrSet<NiPointer<NiMaterialResource>>& kUniforms) {
    FNVD_INLINED_LOCAL(NiFixedString, kKey, std::nullopt, std::nullopt);
    // EXE-INLINED 0x438170
    if (pcName) {
        kKey.m_kHandle = NiGlobalStringTable::AddString(pcName);
    }
    else {
        kKey.m_kHandle = NiGlobalStringTable::NULL_STRING;
    }
    NiGPUProgram* pkProgram = nullptr;
    {
        NiPointer<NiGPUProgramDesc> spDesc;
        if (m_kCachedPrograms.GetAt(kKey, spDesc)) {
            NiGPUProgramDesc* pkDesc = spDesc.m_pObject;
            for (uint32_t ui = 0; ui < pkDesc->m_kUniforms.m_uiUsed; ++ui) {
                kUniforms.Add(pkDesc->m_kUniforms.m_pBase[ui]);
            }
            pkProgram = pkDesc->m_spProgram.m_pObject;
        }
    }
    // The destructor of kKey.
    // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
    NiGlobalStringTable::ReleaseHandle(kKey.GetHandle());
    return pkProgram;
}

NiGPUProgram* NiGPUProgramCache::GenerateProgram(
    const char* pcName,
    const char* pcProgramText,
    NiTObjectPtrSet<NiPointer<NiMaterialResource>>& kUniforms
) {
    return nullptr;
}

const NiTObjectPtrSet<NiFixedString>* NiGPUProgramCache::GetValidCompileTargetsForPlatform() {
    return &m_kValidTargets;
}

const NiFixedString& NiGPUProgramCache::GetShaderProfile() {
    return m_kShaderProfile;
}

const NiFixedString& NiGPUProgramCache::GetWorkingDirectory() {
    return m_kWorkingDir;
}

// A relative kFilename is made absolute against the current directory. The old directory is held as a raw handle, with
// its reference taken and dropped where a NiFixedString copy and destructor would do it.
bool NiGPUProgramCache::SetWorkingDirectory(const NiFixedString& kFilename) {
    char acCWD[0x104];
    char acString[0x104];
    NiGlobalStringTable::GlobalStringHandle kOldWorkingDir = m_kWorkingDir.m_kHandle;
    NiGlobalStringTable::AcquireHandle(kOldWorkingDir);
    if (kFilename.m_kHandle != NiGlobalStringTable::NULL_STRING && NiPath::IsRelative(kFilename.m_kHandle)) {
        if (!NiPath::GetCurrentWorkingDirectory(acCWD, 0x104) ||
            NiPath::ConvertToAbsolute(acString, 0x104, kFilename.m_kHandle, acCWD) == 0) {
            NiGlobalStringTable::ReleaseHandle(kOldWorkingDir);
            return false;
        }
        m_kWorkingDir.Set(acString);
    }
    else {
        m_kWorkingDir = kFilename;
    }
    if (kOldWorkingDir != m_kWorkingDir.m_kHandle) {
        ComputeFullFilename();
    }
    NiGlobalStringTable::ReleaseHandle(kOldWorkingDir);
    return true;
}

// KEEP: uiCacheFileVersion is read uninitialised when the file ends before it (Gamebryo's source leaves it so).
// Gamebryo's source without its debug output; ValidateDirectory is inlined, and the endian flag is read and ignored
// (the PC is little-endian).
bool NiGPUProgramCache::Load() {
    // EXE-INLINED 0xEE8D20
    if (m_kWorkingDir.m_kHandle == NiGlobalStringTable::NULL_STRING ||
        (!NiFile::DirectoryExists(m_kWorkingDir.m_kHandle) &&
         !NiFile::CreateDirectoryRecursive(m_kWorkingDir.m_kHandle))) {
        return false;
    }
    if (m_bLoaded) {
        return true;
    }
    NiFile* pkFile = NiFile::GetFile(m_kPathAndFilename.m_kHandle, NiFile::READ_ONLY, 0x8000);
    if (!pkFile) {
        return false;
    }
    NiBinaryStream* pkStream = pkFile;
    if (!*pkStream) {
        delete pkStream;
        return false;
    }
    const char* pcHeader = "NiGPUProgram Cache File";
    char acBuffer[512];
    // Reads the header through the unsigned char array instance, as raw bytes.
    NiStreamLoadBinary<uint8_t>(
        *pkFile, reinterpret_cast<uint8_t*>(acBuffer), static_cast<uint32_t>(__builtin_strlen(pcHeader)) + 1
    );
    if (NiStricmp(acBuffer, pcHeader) != 0) {
        delete pkStream;
        return false;
    }
    uint8_t ucLittleEndian;
    NiStreamLoadBinary<uint8_t>(*pkFile, ucLittleEndian);
    uint32_t uiCacheFileVersion;
    NiStreamLoadBinary(pkFile, &uiCacheFileVersion);
    if (uiCacheFileVersion < 5) {
        delete pkStream;
        if (ms_bOverwriteDeprecatedCacheFiles) {
            Save();
        }
        return false;
    }
    LoadString(*pkFile, acBuffer, 512);
    if (NiStricmp(acBuffer, GetPlatform()) != 0) {
        delete pkStream;
        return false;
    }
    LoadString(*pkFile, acBuffer, 512);
    if (NiStricmp(acBuffer, GetPlatformSpecificCodeID()) != 0) {
        delete pkStream;
        return false;
    }
    LoadString(*pkFile, acBuffer, 512);
    if (NiStricmp(acBuffer, GetShaderProfile().m_kHandle) != 0) {
        delete pkStream;
        return false;
    }
    bool bAnyDeprecated = false;
    for (;;) {
        // Asks for the size before the position.
        uint32_t uiFileSize = pkFile->GetFileSize();
        if (pkFile->GetPosition() >= uiFileSize) {
            break;
        }
        bool bDeprecated = false;
        if (!LoadEntry(*pkFile, bDeprecated)) {
            delete pkStream;
            return false;
        }
        if (bDeprecated) {
            bAnyDeprecated = true;
        }
    }
    delete pkStream;
    m_bLoaded = true;
    if (bAnyDeprecated && ms_bOverwriteDeprecatedCacheFiles) {
        Save();
    }
    return true;
}

// A file that opened but is not good is not deleted, as in Gamebryo's source. SaveEntry's result is ignored.
bool NiGPUProgramCache::Save() {
    // EXE-INLINED 0xEE8D20
    if (m_kWorkingDir.m_kHandle == NiGlobalStringTable::NULL_STRING ||
        (!NiFile::DirectoryExists(m_kWorkingDir.m_kHandle) &&
         !NiFile::CreateDirectoryRecursive(m_kWorkingDir.m_kHandle))) {
        return false;
    }
    NiFile* pkFile = NiFile::GetFile(m_kPathAndFilename.m_kHandle, NiFile::WRITE_ONLY, 0x8000);
    if (!pkFile) {
        return false;
    }
    NiBinaryStream* pkStream = pkFile;
    if (!*pkStream) {
        return false;
    }
    const char* pcHeader = "NiGPUProgram Cache File";
    WriteBytes(pkFile, pcHeader, static_cast<uint32_t>(__builtin_strlen(pcHeader)) + 1);
    uint8_t ucLittleEndian = 1;
    uint32_t uiSize = 1;
    pkFile->BinaryWrite(&ucLittleEndian, 1, &uiSize, 1);
    uint32_t uiCacheFileVersion = 5;
    uiSize = 4;
    pkFile->BinaryWrite(&uiCacheFileVersion, 4, &uiSize, 1);
    SaveString(*pkFile, GetPlatform());
    SaveString(*pkFile, GetPlatformSpecificCodeID());
    SaveString(*pkFile, GetShaderProfile().m_kHandle);
    NiTMapItem<NiFixedString, NiPointer<NiGPUProgramDesc>>* pkPos = m_kCachedPrograms.GetFirstPos();
    while (pkPos) {
        NiPointer<NiGPUProgramDesc> spDesc;
        FNVD_INLINED_LOCAL(NiFixedString, kName, std::nullopt, std::nullopt);
        kName.m_kHandle = NiGlobalStringTable::NULL_STRING;
        m_kCachedPrograms.GetNext(pkPos, kName, spDesc);
        SaveEntry(*pkFile, spDesc.m_pObject, kName);
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(kName.GetHandle());
    }
    delete pkStream;
    return true;
}

bool NiGPUProgramCache::Clear() {
    m_kCachedPrograms.RemoveAll();
    m_bLoaded = false;
    return true;
}

bool NiGPUProgramCache::IsLoaded() const {
    return m_bLoaded;
}

bool NiGPUProgramCache::IsLocked() const {
    return m_bLocked;
}

// Keeps the uniforms whose variable some program variable names, removing the others from the end.
bool NiGPUProgramCache::PruneUnusedConstants(
    NiGPUProgram* pkProgram,
    NiTObjectPtrSet<NiPointer<NiMaterialResource>>& kUniforms
) {
    uint32_t uiUniformSize = kUniforms.m_uiUsed;
    bool abKeepEntries[1024];
    std::memset(abKeepEntries, 0, sizeof(abKeepEntries));
    for (uint32_t ui = 0; ui < pkProgram->GetVariableCount(); ++ui) {
        const char* pcVarName = pkProgram->GetVariableName(ui);
        for (uint32_t uiUniformIdx = 0; uiUniformIdx < uiUniformSize; ++uiUniformIdx) {
            NiMaterialResource* pkRes = kUniforms.m_pBase[uiUniformIdx].m_pObject;
            if (!pkRes) {
                continue;
            }
            const char* pcVariable = pkRes->m_kVariable.GetHandle();
            bool bEqual = pcVariable == pcVarName;
            if (!bEqual && pcVarName && pcVariable) {
                // EXE-INLINED none: strcmp
                const char* pc1 = pcVariable;
                const char* pc2 = pcVarName;
                while (*pc1 == *pc2 && *pc1) {
                    ++pc1;
                    ++pc2;
                }
                bEqual = *pc1 == *pc2;
            }
            if (bEqual) {
                abKeepEntries[uiUniformIdx] = true;
                break;
            }
        }
    }
    for (int32_t iIdx = static_cast<int32_t>(uiUniformSize) - 1; iIdx >= 0; --iIdx) {
        if (!abKeepEntries[iIdx]) {
            kUniforms.OrderedRemoveAt(static_cast<uint32_t>(iIdx));
        }
    }
    return true;
}

// The path and file name from the working directory and the file name; the cache is cleared when it changes. The old
// path is held as a raw handle, its reference taken and dropped as a NiFixedString copy does.
void NiGPUProgramCache::ComputeFullFilename() {
    if (m_kWorkingDir.m_kHandle == NiGlobalStringTable::NULL_STRING ||
        m_kFilename.m_kHandle == NiGlobalStringTable::NULL_STRING) {
        // Assignment of null to m_kPathAndFilename: tests the old handle against null first.
        NiGlobalStringTable::GlobalStringHandle kOld = m_kPathAndFilename.m_kHandle;
        if (kOld) {
            m_kPathAndFilename.m_kHandle = NiGlobalStringTable::AddString(nullptr);
            NiGlobalStringTable::ReleaseHandle(kOld);
        }
        m_bLoaded = false;
        return;
    }
    NiGlobalStringTable::GlobalStringHandle kCacheFile = m_kPathAndFilename.m_kHandle;
    NiGlobalStringTable::AcquireHandle(kCacheFile);
    char acString[0x104];
    NiPath::ConvertToAbsolute(acString, 0x104, m_kFilename.m_kHandle, m_kWorkingDir.m_kHandle);
    // The NiFixedString assignment of acString.
    NiGlobalStringTable::GlobalStringHandle kOld = m_kPathAndFilename.m_kHandle;
    if (kOld != acString) {
        m_kPathAndFilename.m_kHandle = NiGlobalStringTable::AddString(acString);
        NiGlobalStringTable::ReleaseHandle(kOld);
    }
    if (kCacheFile != m_kPathAndFilename.m_kHandle) {
        Clear();
    }
    NiGlobalStringTable::ReleaseHandle(kCacheFile);
}

// Appends one entry to the cache file, saving the whole cache first when the file is missing but writable.
bool NiGPUProgramCache::AppendEntry(NiGPUProgramCache::NiGPUProgramDesc* pkDesc, const NiFixedString& kName) {
    if (m_kPathAndFilename.m_kHandle == NiGlobalStringTable::NULL_STRING) {
        return false;
    }
    if ((!NiFile::Access(m_kPathAndFilename.m_kHandle, NiFile::READ_ONLY)) &&
        (!NiFile::Access(m_kPathAndFilename.m_kHandle, NiFile::WRITE_ONLY) || !Save())) {
        return false;
    }

    NiFile* pkFile = NiFile::GetFile(m_kPathAndFilename.m_kHandle, NiFile::APPEND_ONLY, 0x8000);
    if (!pkFile) {
        return false;
    }
    NiBinaryStream* pkStream = pkFile;
    if (*pkStream) {
        bool bSuccess = SaveEntry(*pkFile, pkDesc, kName);
        delete pkStream;
        return bSuccess;
    }
    delete pkStream;
    return false;
}

// Writes the version, the name, the program, then each uniform's strings and values.
bool NiGPUProgramCache::SaveEntry(
    NiBinaryStream& kStream,
    NiGPUProgramCache::NiGPUProgramDesc* pkDesc,
    const NiFixedString& kName
) {
    uint32_t uiSize = 4;
    kStream.BinaryWrite(&m_uiVersion, 4, &uiSize, 1);
    SaveString(kStream, kName.GetHandle());
    if (!SaveGPUProgram(kStream, pkDesc->m_spProgram.m_pObject)) {
        return false;
    }
    uint32_t uiResCount = pkDesc->m_kUniforms.m_uiUsed;
    uiSize = 4;
    kStream.BinaryWrite(&uiResCount, 4, &uiSize, 1);
    for (uint32_t ui = 0; ui < uiResCount; ++ui) {
        NiMaterialResource* pkRes = pkDesc->m_kUniforms.m_pBase[ui].m_pObject;
        SaveString(kStream, pkRes->m_kType.GetHandle());
        SaveString(kStream, pkRes->m_kSemantic.GetHandle());
        SaveString(kStream, pkRes->m_kLabel.GetHandle());
        SaveString(kStream, pkRes->m_kVariable.GetHandle());
        SaveString(kStream, pkRes->m_kDefaultValue.GetHandle());
        uint32_t uiCount = pkRes->m_uiCount;
        uiSize = 4;
        kStream.BinaryWrite(&uiCount, 4, &uiSize, 1);
        uint32_t uiSource = static_cast<uint32_t>(pkRes->m_eSource);
        uiSize = 4;
        kStream.BinaryWrite(&uiSource, 4, &uiSize, 1);
        uint32_t uiExtraData = pkRes->m_uiExtraData;
        uiSize = 4;
        kStream.BinaryWrite(&uiExtraData, 4, &uiSize, 1);
        uint32_t uiObjectType = static_cast<uint32_t>(pkRes->m_eObjectType);
        uiSize = 4;
        kStream.BinaryWrite(&uiObjectType, 4, &uiSize, 1);
    }
    return true;
}

// KEEP: uiVersion and the uniforms' values are read uninitialised when the file ends before them (Gamebryo's
// source leaves them so).
// Gamebryo's source without its debug output. The exe inlines LoadString (0xEE92D0) for every string: the name's
// characters go through the unsigned char array instance (0xA8D8B0), the uniforms' through BinaryRead inlined. A
// new entry's NiGPUProgramDesc constructor (0xE8EAD0) is inlined too.
bool NiGPUProgramCache::LoadEntry(NiBinaryStream& kStream, bool& bDeprecated) {
    uint32_t uiVersion;
    uint32_t uiSize = 4;
    kStream.BinaryRead(&uiVersion, 4, &uiSize, 1);
    char acBuffer[512];
    uint32_t uiLength = 0;
    uiSize = 4;
    kStream.BinaryRead(&uiLength, 4, &uiSize, 1);
    if (uiLength == 0) {
        acBuffer[0] = '\0';
    }
    else {
        NiStreamLoadBinary<uint8_t>(kStream, reinterpret_cast<uint8_t*>(acBuffer), uiLength);
        acBuffer[uiLength + 1] = '\0';
    }
    FNVD_INLINED_LOCAL(NiFixedString, kName, std::nullopt, std::nullopt);
    // EXE-INLINED 0x438170
    kName.m_kHandle = NiGlobalStringTable::AddString(acBuffer);

    bool bSkip = false;
    if (uiVersion != m_uiVersion) {
        bDeprecated = true;
        bSkip = true;
    }
    {
        NiPointer<NiGPUProgramDesc> spOtherDesc;
        if (m_kCachedPrograms.GetAt(kName, spOtherDesc)) {
            bSkip = true;
        }
    }

    NiGPUProgramDesc* pkDesc = nullptr;
    if (!bSkip) {
        pkDesc = new NiGPUProgramDesc;
    }

    NiGPUProgram* pkProgram = nullptr;
    if (!LoadGPUProgram(kStream, kName, pkProgram, bSkip)) {
        // The destructor of kName, run by hand on each return.
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(kName.GetHandle());
        return false;
    }
    if (!bSkip) {
        pkDesc->m_spProgram = pkProgram;
    }

    // EXE-INLINED 0xEE92D0
    auto ReadString = [&] __attribute__((always_inline)) {
        uiLength = 0;
        uiSize = 4;
        kStream.BinaryRead(&uiLength, 4, &uiSize, 1);
        if (uiLength == 0) {
            acBuffer[0] = '\0';
        }
        else {
            uiSize = 1;
            kStream.BinaryRead(acBuffer, uiLength, &uiSize, 1);
            acBuffer[uiLength + 1] = '\0';
        }
    };
    uint32_t uiResCount = 0;
    uiSize = 4;
    kStream.BinaryRead(&uiResCount, 4, &uiSize, 1);
    for (uint32_t ui = 0; ui < uiResCount; ++ui) {
        uint32_t uiValue;
        if (!bSkip) {
            void* pvMemory = NiMemObject::operator new(sizeof(NiMaterialResource));
            NiMaterialResource* pkRes = pvMemory ? ::new (pvMemory) NiMaterialResource : nullptr;

            ReadString();
            FNVD_INLINED_LOCAL(NiFixedString, kType, std::nullopt, std::nullopt);
            // EXE-INLINED 0x438170
            kType.m_kHandle = NiGlobalStringTable::AddString(acBuffer);
            pkRes->SetType(kType);
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(kType.GetHandle());

            ReadString();
            FNVD_INLINED_LOCAL(NiFixedString, kSemantic, std::nullopt, std::nullopt);
            // EXE-INLINED 0x438170
            kSemantic.m_kHandle = NiGlobalStringTable::AddString(acBuffer);
            pkRes->SetSemantic(kSemantic);
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(kSemantic.GetHandle());

            ReadString();
            FNVD_INLINED_LOCAL(NiFixedString, kLabel, std::nullopt, std::nullopt);
            // EXE-INLINED 0x438170
            kLabel.m_kHandle = NiGlobalStringTable::AddString(acBuffer);
            pkRes->SetLabel(kLabel);
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(kLabel.GetHandle());

            ReadString();
            FNVD_INLINED_LOCAL(NiFixedString, kVariable, std::nullopt, std::nullopt);
            // EXE-INLINED 0x438170
            kVariable.m_kHandle = NiGlobalStringTable::AddString(acBuffer);
            pkRes->SetVariable(kVariable);
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(kVariable.GetHandle());

            ReadString();
            FNVD_INLINED_LOCAL(NiFixedString, kDefaultValue, std::nullopt, std::nullopt);
            // EXE-INLINED 0x438170
            kDefaultValue.m_kHandle = NiGlobalStringTable::AddString(acBuffer);
            pkRes->SetDefaultValue(kDefaultValue);
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(kDefaultValue.GetHandle());

            uiSize = 4;
            kStream.BinaryRead(&uiValue, 4, &uiSize, 1);
            pkRes->m_uiCount = uiValue;
            uiSize = 4;
            kStream.BinaryRead(&uiValue, 4, &uiSize, 1);
            pkRes->m_eSource = static_cast<NiMaterialResource::Source>(uiValue);
            uiSize = 4;
            kStream.BinaryRead(&uiValue, 4, &uiSize, 1);
            pkRes->m_uiExtraData = uiValue;
            uiSize = 4;
            kStream.BinaryRead(&uiValue, 4, &uiSize, 1);
            pkRes->m_eObjectType = static_cast<NiShaderAttributeDesc::ObjectType>(uiValue);

            NiPointer<NiMaterialResource> spRes = pkRes;
            pkDesc->m_kUniforms.Add(spRes);
        }
        else {
            ReadString();
            ReadString();
            ReadString();
            ReadString();
            ReadString();
            uiSize = 4;
            kStream.BinaryRead(&uiValue, 4, &uiSize, 1);
            uiSize = 4;
            kStream.BinaryRead(&uiValue, 4, &uiSize, 1);
            uiSize = 4;
            kStream.BinaryRead(&uiValue, 4, &uiSize, 1);
            uiSize = 4;
            kStream.BinaryRead(&uiValue, 4, &uiSize, 1);
        }
    }

    if (!bSkip && pkDesc->m_spProgram.m_pObject) {
        m_kCachedPrograms.SetAt(kName, pkDesc);
    }
    // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
    NiGlobalStringTable::ReleaseHandle(kName.GetHandle());
    return true;
}

bool NiGPUProgramCache::ValidateDirectory() {
    if (m_kWorkingDir.m_kHandle != NiGlobalStringTable::NULL_STRING) {
        if (NiFile::DirectoryExists(m_kWorkingDir.m_kHandle)) {
            return true;
        }
        return NiFile::CreateDirectoryRecursive(m_kWorkingDir.m_kHandle);
    }
    return false;
}

// The length with its terminator (0 for null), then the characters.
bool NiGPUProgramCache::SaveString(NiBinaryStream& kStream, const char* pcString) {
    if (!pcString) {
        uint32_t uiLength = 0;
        uint32_t uiSize = 4;
        kStream.BinaryWrite(&uiLength, 4, &uiSize, 1);
        return true;
    }
    uint32_t uiLength = static_cast<uint32_t>(__builtin_strlen(pcString)) + 1;
    uint32_t uiSize = 4;
    kStream.BinaryWrite(&uiLength, 4, &uiSize, 1);
    uiSize = 1;
    kStream.BinaryWrite(pcString, uiLength, &uiSize, 1);
    return true;
}

// The terminator goes one byte past the string's end, as in Gamebryo's source.
bool NiGPUProgramCache::LoadString(NiBinaryStream& kStream, char* pcBuffer, uint32_t stBufferSize) {
    if (stBufferSize == 0 || !pcBuffer) {
        return false;
    }
    uint32_t uiLength = 0;
    uint32_t uiSize = 4;
    kStream.BinaryRead(&uiLength, 4, &uiSize, 1);
    if (uiLength == 0) {
        pcBuffer[0] = '\0';
        return true;
    }
    // Reads the characters through the unsigned char instance, as raw bytes.
    NiStreamLoadBinary<uint8_t>(kStream, reinterpret_cast<uint8_t*>(pcBuffer), uiLength);
    pcBuffer[uiLength + 1] = '\0';
    return true;
}
