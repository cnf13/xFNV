#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

GUID CLSID_IAsyncReader{0x56A868AA, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
GUID CLSID_IAMovieSetup{0xA3D8CEC0, 32346, 4559, {187, 197, 0, 128, 95, 108, 239, 32}};
GUID CLSID_IMemAllocatorCallbackTemp{0x379A0CF0, 49630, 4562, {171, 245, 0, 160, 201, 5, 243, 117}};
GUID CLSID_IMemAllocator{0x56A8689C, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
GUID CLSID_IBaseFilter{0x56A86895, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
GUID CLSID_IMediaFilter{0x56A86899, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
GUID CLSID_IEnumMediaTypes{0x89C31040, 33899, 4558, {151, 211, 0, 170, 0, 85, 89, 90}};
GUID CLSID_IEnumPins{0x56A86892, 2772, 4558, {176, 58, 0, 32, 175, 11, 167, 112}};
GUID GUID_NULL{0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
GUID CLSID_IUnknown{0, 0, 0, {192, 0, 0, 0, 0, 0, 0, 70}};
GUID CLSID_IPersist{268, 0, 0, {192, 0, 0, 0, 0, 0, 0, 70}};
GUID CLSID_IDirect3DTexture9{0x85C31227, 15845, 20224, {155, 58, 241, 26, 195, 140, 24, 181}};

// Wrappers for the Direct3D / D3DX imports: each calls its dinput8.dll or d3dx9_38.dll import and returns its result.
HRESULT
DirectInput8Create_0(HINSTANCE__* hinst, uint32_t dwVersion, const GUID* riidltf, void** ppvOut, IUnknown* punkOuter) {
    return imp_DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

D3DXVECTOR3* D3DXVec3TransformCoord_Thunk(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM) {
    return D3DXVec3TransformCoord(pOut, pV, pM);
}

HRESULT D3DXGetShaderConstantTable_Thunk(const uint32_t* pFunction, ID3DXConstantTable** ppConstantTable) {
    return D3DXGetShaderConstantTable(pFunction, ppConstantTable);
}

const char* D3DXGetPixelShaderProfile_Thunk(IDirect3DDevice9* pDevice) {
    return D3DXGetPixelShaderProfile(pDevice);
}

HRESULT D3DXCreateBuffer_Thunk(uint32_t NumBytes, ID3DXBuffer** ppBuffer) {
    return D3DXCreateBuffer(NumBytes, ppBuffer);
}

const char* D3DXGetVertexShaderProfile_Thunk(IDirect3DDevice9* pDevice) {
    return D3DXGetVertexShaderProfile(pDevice);
}

HRESULT D3DXAssembleShaderFromFileA_Thunk(
    const char* pSrcFile,
    const D3DXMACRO* pDefines,
    ID3DXInclude* pInclude,
    uint32_t Flags,
    ID3DXBuffer** ppShader,
    ID3DXBuffer** ppErrorMsgs
) {
    return D3DXAssembleShaderFromFileA(pSrcFile, pDefines, pInclude, Flags, ppShader, ppErrorMsgs);
}

HRESULT D3DXCompileShaderFromFileA_Thunk(
    const char* pSrcFile,
    const D3DXMACRO* pDefines,
    ID3DXInclude* pInclude,
    const char* pFunctionName,
    const char* pProfile,
    uint32_t Flags,
    ID3DXBuffer** ppShader,
    ID3DXBuffer** ppErrorMsgs,
    ID3DXConstantTable** ppConstantTable
) {
    return D3DXCompileShaderFromFileA(
        pSrcFile,
        pDefines,
        pInclude,
        pFunctionName,
        pProfile,
        Flags,
        ppShader,
        ppErrorMsgs,
        ppConstantTable
    );
}

HRESULT D3DXAssembleShader_Thunk(
    const char* pSrcData,
    uint32_t SrcDataLen,
    const D3DXMACRO* pDefines,
    ID3DXInclude* pInclude,
    uint32_t Flags,
    ID3DXBuffer** ppShader,
    ID3DXBuffer** ppErrorMsgs
) {
    return D3DXAssembleShader(pSrcData, SrcDataLen, pDefines, pInclude, Flags, ppShader, ppErrorMsgs);
}

D3DXPLANE::D3DXPLANE() {}

D3DXPLANE::D3DXPLANE(float fa, float fb, float fc, float fd) : a(fa), b(fb), c(fc), d(fd) {}
