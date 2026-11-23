#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

class NiDX9Renderer;

// The D3D shader factory: shader libraries and shaders for the D3D renderer. Only some of its members are declared.
class NiD3DShaderFactory : public NiShaderFactory {
public:
    NiD3DShaderFactory();
    ~NiD3DShaderFactory() override;

    NiShader* RetrieveShader(const char* pcName, uint32_t uiImplementation, bool bReportNotFound) override;
    NiShader* FindShader(const char* pcName, uint32_t uiImplementation) override;
    bool ReleaseShaderFromLibrary(const char* pcName, uint32_t uiImplementation) override;
    bool ReleaseShaderFromLibrary(NiShader* pkShader) override;
    void InsertShader(NiShader* pkShader, uint32_t uiImplementation) override;
    void RemoveShader(const char* pcName, uint32_t uiImplementation) override;
    // Empties the shader map, if there is one.
    void RemoveAllShaders() override;
    bool IsDefaultImplementation(NiShader* pkShader) override;
    void InsertLibrary(NiShaderLibrary* pkLibrary) override;
    NiShaderLibrary* FindLibrary(const char* pcName) override;
    void RemoveLibrary(const char* pcName) override;
    void RemoveAllLibraries() override;
    NiShaderLibrary* GetFirstLibrary() override;
    NiShaderLibrary* GetNextLibrary() override;
    // The low byte of a D3D shader version.
    uint32_t GetMinorPixelShaderVersion(const uint32_t uiVersion) override;
    // The second byte of a D3D shader version.
    uint32_t GetMajorPixelShaderVersion(const uint32_t uiVersion) override;
    // D3DPS_VERSION(major, minor).
    uint32_t CreatePixelShaderVersion(const uint32_t uiMajorVersion, const uint32_t uiMinorVersion) override;
    uint32_t GetMinorVertexShaderVersion(const uint32_t uiVersion) override;
    uint32_t GetMajorVertexShaderVersion(const uint32_t uiVersion) override;
    // D3DVS_VERSION(major, minor).
    uint32_t CreateVertexShaderVersion(const uint32_t uiMajorVersion, const uint32_t uiMinorVersion) override;
    const char* GetRendererString() const override;

    virtual void SetRenderer(NiDX9Renderer* pkRenderer);
    virtual void PurgeAllShaderRendererData();
    virtual void RestoreAllShaderRendererData();

protected:
    const char* GetFirstProgramDirectory(NiTListIterator& kIter) override;
    const char* GetNextProgramDirectory(NiTListIterator& kIter) override;
    void AddProgramDirectory(const char* pacDirectory) override;
    void RemoveProgramDirectory(const char* pacDirectory) override;
    // Drops every program directory of the shader program factory, if there is one.
    void RemoveAllProgramDirectories() override;

public:
    // The active D3D factory, or null.
    static NiD3DShaderFactory* GetD3DShaderFactory();
    // Deletes the active factory, then clears it.
    static void _SDMShutdown();
    // Builds the factory and installs it in NiShaderFactory.
    static void _SDMInit();

    NiDX9Renderer* m_pkRenderer;
    NiTStringPointerMap<NiShader*>* m_pkShaderMap;
    // The position GetFirstLibrary and GetNextLibrary walk the library map from (Gamebryo: NiTMapIterator).
    NiTMapItem<const char*, NiPointer<NiShaderLibrary>>* m_kLibraryIter;
    NiTStringPointerMap<NiPointer<NiShaderLibrary>>* m_pkLibraryMap;
    // The lock the shader map is used under. The program directories are not held here: GetFirstProgramDirectory reads
    // NiD3DShaderProgramFactory::m_kProgramDirectories.
    NiCriticalSection m_kShaderMapLock;

    // The active D3D factory, or null.
    static inline NiD3DShaderFactory* ms_pkD3DShaderFactory;
    static inline uint32_t ms_uiNewShaderValue;
};
