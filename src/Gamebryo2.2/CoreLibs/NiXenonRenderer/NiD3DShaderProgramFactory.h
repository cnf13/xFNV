#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"

class NiD3DPixelShader;
class NiD3DShaderProgramCreator;
class NiD3DVertexShader;
class NiDX9Renderer;
class NiDX9RenderState;

// The pointer list's GetNext: returns the element of the node arIterator holds and moves arIterator to the next node.
void* GetNext(NiTListIterator& arIterator);

class NiD3DShaderProgramFactory : public NiRefObject {
public:
    NiD3DShaderProgramFactory();
    ~NiD3DShaderProgramFactory() override;

    // Adds pacDirectory to the shader search path ('/' becomes '\\'), unless it is there already (ignoring case).
    void AddProgramDirectory(const char* pacDirectory);
    // Removes pacDirectory from the shader search path.
    void RemoveProgramDirectory(const char* pcDirectory);
    // Clears the shader search path.
    void RemoveAllProgramDirectories();
    // Starts kIter at the second program directory and returns the first, or null when there
    // is none.
    const char* GetFirstProgramDirectory(NiTListIterator& kIter);
    void InsertVertexShaderIntoMap(NiD3DVertexShader* pkShader);
    void InsertPixelShaderIntoMap(NiD3DPixelShader* pkShader);
    NiD3DVertexShader* GetVertexShader(const char* pcShaderName);
    NiD3DPixelShader* GetPixelShader(const char* pcShaderName);

    // Drops pkShader from the factory's vertex shader map, keyed by its name.
    static void ReleaseVertexShader(NiD3DVertexShader* pkShader);
    // Drops pkShader from the factory's pixel shader map, keyed by its name.
    static void ReleasePixelShader(NiD3DPixelShader* pkShader);
    // Copies pcOriginalName to pcTrueName when the file exists, else the first program directory path that does.
    static bool ResolveShaderFileName(const char* pcOriginalName, char* pcTrueName, uint32_t uiStringLen);
    // Recreates pkPixelShader's handle through its creator, unless it still has one.
    static bool RecreatePixelShader(NiD3DPixelShader* pkPixelShader);
    // Recreates pkVertexShader's handle through its creator, unless it still has one.
    static bool RecreateVertexShader(NiD3DVertexShader* pkVertexShader);
    // Finds the creator registered for pcExt and builds the vertex shader from pvBuffer. bSoftwareVP and bRecoverable
    // are int32_t rather than bool so the caller's whole value is passed on; a bool would be zero-extended.
    static NiD3DVertexShader* CreateVertexShaderFromBuffer(
        const void* pvBuffer,
        uint32_t uiBufferSize,
        const char* pcExt,
        const char* pcShaderName,
        const char* pcEntryPoint,
        const char* pcShaderTarget,
        IDirect3DVertexDeclaration9* hDecl,
        uint32_t uiUsage,
        int32_t bSoftwareVP,
        int32_t bRecoverable
    );
    // Finds the creator registered for pcExt and builds the pixel shader from pvBuffer. bRecoverable is an int32_t
    // rather than a bool so that the caller's whole 32-bit value is passed on; a bool would be zero-extended.
    static NiD3DPixelShader* CreatePixelShaderFromBuffer(
        const void* pvBuffer,
        uint32_t uiBufferSize,
        const char* pcExt,
        const char* pcShaderName,
        const char* pcEntryPoint,
        const char* pcShaderTarget,
        int32_t bRecoverable
    );
    // Creates the shader creator map when the factory has none.
    static void _SDMInit();
    // Empties and deletes the shader creator map.
    static void _SDMShutdown();
    static void CreateShaderCreatorMap();
    static void SetD3DRenderer(NiDX9Renderer* pkD3DRenderer);

    // Deletes the factory instance.
    static void Shutdown();
    static NiD3DShaderProgramFactory* GetInstance();
    static void UnregisterShaderCreator(NiD3DShaderProgramCreator* pkCreator);
    static NiD3DShaderProgramCreator* GetShaderCreator(const char* pcFilename);
    // KEEP: see the body.
    static bool RegisterShaderCreator(const char* pcExt, NiD3DShaderProgramCreator* pkCreator);

    NiTPointerList<char*> m_kProgramDirectories;
    NiTStringPointerMap<NiD3DVertexShader*>* m_pkVertexShaderMap;
    NiTStringPointerMap<NiD3DPixelShader*>* m_pkPixelShaderMap;

    // The factory instance, created by NiD3DShaderInterface::InitializeShaderSystem.
    static inline NiD3DShaderProgramFactory* ms_pkFactory;
    // The shader creator map, keyed by creator name.
    static inline NiTStringPointerMap<NiD3DShaderProgramCreator*>* ms_pkShaderCreators;
    // The renderer's device (with a reference held), renderer and render state, as SetD3DRenderer sets them.
    static inline IDirect3DDevice9* ms_pkD3DDevice;
    static inline NiDX9Renderer* ms_pkD3DRenderer;
    static inline NiDX9RenderState* ms_pkD3DRenderState;
};
