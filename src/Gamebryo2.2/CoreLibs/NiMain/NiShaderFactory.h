#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalConstantEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTFixedStringMap.h"

class NiRenderer;
class NiShader;

// The position of a list walk (Gamebryo's NiTListIterator, a void*); the program directories are a list of strings.
using NiTListIterator = void*;

// The registry of shader libraries, shaders and global shader constants. Only some of its members are declared.
class NiShaderFactory : public NiRefObject {
public:
    // ReportError's error codes. This file passes NISHADERERR_UNKNOWN.
    enum NiShaderError : uint32_t {
        NISHADERERR_UNKNOWN = 1,
        // Gamebryo's value; NiD3DShaderConstantMap::SetupDefinedConstantValue returns it without the time extra data.
        NISHADERERR_ENTRYNOTFOUND = 0x10,
    };

    // Gamebryo's callback types (NiShaderFactory.h).
    using NISHADERFACTORY_CLASSCREATIONCALLBACK =
        bool (*)(const char*, NiRenderer*, int32_t, char*[], bool, NiShaderLibrary**);
    using NISHADERFACTORY_RUNPARSERCALLBACK = uint32_t (*)(const char*, NiRenderer*, const char*, bool);
    // The int32_t parameter is the recoverable flag. It is an int32_t rather than a bool so the caller's whole value is
    // passed on; a bool would be zero-extended.
    using NISHADERFACTORY_ERRORCALLBACK = uint32_t (*)(const char*, NiShaderError, int32_t);

    NiShaderFactory();

    // These virtual functions are stubs: they return 0 or do nothing.
    virtual NiShader* RetrieveShader(const char*, uint32_t, bool) {
        return nullptr;
    }
    virtual NiShader* FindShader(const char*, uint32_t) {
        return nullptr;
    }
    // The vtable lists these overloads in reverse declaration order: the NiShader* overload comes before the name
    // overload.
    virtual bool ReleaseShaderFromLibrary(const char*, uint32_t) {
        return false;
    }
    virtual bool ReleaseShaderFromLibrary(NiShader*) {
        return false;
    }
    virtual void InsertShader(NiShader*, uint32_t) {}
    virtual void RemoveShader(const char*, uint32_t) {}
    virtual void RemoveAllShaders() {}
    virtual bool IsDefaultImplementation(NiShader*) {
        return false;
    }
    virtual void InsertLibrary(NiShaderLibrary*) {}
    virtual NiShaderLibrary* FindLibrary(const char*) {
        return nullptr;
    }
    virtual void RemoveLibrary(const char*) {}
    virtual void RemoveAllLibraries() {}
    virtual NiShaderLibrary* GetFirstLibrary() {
        return nullptr;
    }
    virtual NiShaderLibrary* GetNextLibrary() {
        return nullptr;
    }
    virtual uint32_t GetMinorPixelShaderVersion(const uint32_t uiVersion) = 0;
    virtual uint32_t GetMajorPixelShaderVersion(const uint32_t uiVersion) = 0;
    virtual uint32_t CreatePixelShaderVersion(const uint32_t uiMajorVersion, const uint32_t uiMinorVersion) = 0;
    virtual uint32_t GetMinorVertexShaderVersion(const uint32_t uiVersion) = 0;
    virtual uint32_t GetMajorVertexShaderVersion(const uint32_t uiVersion) = 0;
    virtual uint32_t CreateVertexShaderVersion(const uint32_t uiMajorVersion, const uint32_t uiMinorVersion) = 0;
    virtual const char* GetRendererString() const = 0;
    ~NiShaderFactory() override;

protected:
    virtual const char* GetFirstProgramDirectory(NiTListIterator& kIter) = 0;
    virtual const char* GetNextProgramDirectory(NiTListIterator& kIter) = 0;
    virtual void AddProgramDirectory(const char* pacDirectory) = 0;
    virtual void RemoveProgramDirectory(const char* pacDirectory) = 0;
    virtual void RemoveAllProgramDirectories() = 0;

public:
    // Releases every shader of the active factory, if any.
    static void ReleaseAllShaders();

    // Adds (or updates) a global constant; false without a factory.
    static bool RegisterGlobalShaderConstant(
        const NiFixedString& kKey,
        NiShaderAttributeDesc::AttributeType eType,
        uint32_t uiDataSize,
        const void* pvInitialData
    );

    // Drops one reference to a global constant; false without a factory.
    static bool ReleaseGlobalShaderConstant(const NiFixedString& kKey);

    // Formats the message into a 2048-byte buffer and hands it to the factory's error callback; returns 0 without a
    // factory or callback. bRecoverable is int32_t because the caller's whole 32-bit value is passed on to the
    // callback; a bool would be zero-extended.
    static uint32_t ReportError(uint32_t eError, int32_t bRecoverable, const char* pacFmt, ...);
    static NiShaderFactory* GetInstance();

    // The map holds plain NiGlobalConstantEntry pointers: its DeleteItem releases no reference, and
    // RegisterGlobalShaderConstant takes the map's reference by hand.
    NiTFixedStringMap<NiGlobalConstantEntry*> m_kGlobalConstantMap;
    NISHADERFACTORY_CLASSCREATIONCALLBACK m_pfnClassCreate;
    NISHADERFACTORY_RUNPARSERCALLBACK m_pfnRunParser;
    NISHADERFACTORY_ERRORCALLBACK m_pfnErrorCallback;

    static inline NiShaderFactory* ms_pkShaderFactory;
};
