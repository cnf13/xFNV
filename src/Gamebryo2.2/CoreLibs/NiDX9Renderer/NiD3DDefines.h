#pragma once

#include "SDK/DirectX/d3d9.h"
#include "SDK/Windows/dinput.h"

// DirectInput8Create's thunk, which only calls the slot.
HRESULT
DirectInput8Create_0(HINSTANCE__* hinst, uint32_t dwVersion, const GUID* riidltf, void** ppvOut, IUnknown* punkOuter);
D3DXVECTOR3* D3DXVec3TransformCoord_Thunk(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM);
HRESULT D3DXGetShaderConstantTable_Thunk(const uint32_t* pFunction, ID3DXConstantTable** ppConstantTable);
const char* D3DXGetPixelShaderProfile_Thunk(IDirect3DDevice9* pDevice);
// Defined in NiD3DDefines.cpp.
HRESULT D3DXCreateBuffer_Thunk(uint32_t NumBytes, ID3DXBuffer** ppBuffer);
const char* D3DXGetVertexShaderProfile_Thunk(IDirect3DDevice9* pDevice);
// NiD3DShaderProgramCreatorAsm calls it. Defined in NiD3DDefines.cpp.
HRESULT D3DXAssembleShaderFromFileA_Thunk(
    const char* pSrcFile,
    const D3DXMACRO* pDefines,
    ID3DXInclude* pInclude,
    uint32_t Flags,
    ID3DXBuffer** ppShader,
    ID3DXBuffer** ppErrorMsgs
);
// Wrapper for D3DXCompileShaderFromFileA; NiD3DShaderProgramCreatorHLSL calls it. Defined in NiD3DDefines.cpp.
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
);
// Calls D3DXAssembleShader; NiD3DShaderProgramCreatorAsm calls it. Defined in NiD3DDefines.cpp.
HRESULT D3DXAssembleShader_Thunk(
    const char* pSrcData,
    uint32_t SrcDataLen,
    const D3DXMACRO* pDefines,
    ID3DXInclude* pInclude,
    uint32_t Flags,
    ID3DXBuffer** ppShader,
    ID3DXBuffer** ppErrorMsgs
);

inline D3DXVECTOR4* D3DXVec4Transform_0(D3DXVECTOR4* pOut, const D3DXVECTOR4* pV, const D3DXMATRIX* pM) {
    return D3DXVec4Transform(pOut, pV, pM);
}
inline D3DXVECTOR3* D3DXVec3Normalize_Thunk(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV) {
    return D3DXVec3Normalize(pOut, pV);
}
inline D3DXVECTOR3* D3DXVec3TransformNormal_Thunk(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM) {
    return D3DXVec3TransformNormal(pOut, pV, pM);
}
inline D3DXMATRIX* D3DXMatrixMultiply_Thunk(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2) {
    return D3DXMatrixMultiply(pOut, pM1, pM2);
}
inline D3DXMATRIX* D3DXMatrixMultiplyTranspose_Thunk(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2) {
    return D3DXMatrixMultiplyTranspose(pOut, pM1, pM2);
}
inline D3DXMATRIX* D3DXMatrixInverse_Thunk(D3DXMATRIX* pOut, float* pDeterminant, const D3DXMATRIX* pM) {
    return D3DXMatrixInverse(pOut, pDeterminant, pM);
}
inline D3DXMATRIX* D3DXMatrixRotationYawPitchRoll_Thunk(D3DXMATRIX* pOut, float Yaw, float Pitch, float Roll) {
    return D3DXMatrixRotationYawPitchRoll(pOut, Yaw, Pitch, Roll);
}
inline D3DXMATRIX* D3DXMatrixTranspose_Thunk(D3DXMATRIX* pOut, const D3DXMATRIX* pM) {
    return D3DXMatrixTranspose(pOut, pM);
}
inline D3DXPLANE* D3DXPlaneNormalize_Thunk(D3DXPLANE* pOut, const D3DXPLANE* pP) {
    return D3DXPlaneNormalize(pOut, pP);
}
inline D3DXPLANE* D3DXPlaneTransform_Thunk(D3DXPLANE* pOut, const D3DXPLANE* pP, const D3DXMATRIX* pM) {
    return D3DXPlaneTransform(pOut, pP, pM);
}
inline HRESULT D3DXCompileShader_Thunk(
    const char* pSrcData,
    uint32_t SrcDataLen,
    const D3DXMACRO* pDefines,
    ID3DXInclude* pInclude,
    const char* pFunctionName,
    const char* pProfile,
    uint32_t Flags,
    ID3DXBuffer** ppShader,
    ID3DXBuffer** ppErrorMsgs,
    ID3DXConstantTable** ppConstantTable
) {
    return D3DXCompileShader(
        pSrcData, SrcDataLen, pDefines, pInclude, pFunctionName, pProfile, Flags, ppShader, ppErrorMsgs, ppConstantTable
    );
}
inline HRESULT D3DXSaveTextureToFileA_Thunk(
    const char* pDestFile,
    uint32_t DestFormat,
    IDirect3DBaseTexture9* pSrcTexture,
    const PALETTEENTRY* pSrcPalette
) {
    return D3DXSaveTextureToFileA(pDestFile, DestFormat, pSrcTexture, pSrcPalette);
}
inline HRESULT
D3DXGetImageInfoFromFileInMemory_Thunk(const void* pSrcData, uint32_t SrcDataSize, D3DXIMAGE_INFO* pSrcInfo) {
    return D3DXGetImageInfoFromFileInMemory(pSrcData, SrcDataSize, pSrcInfo);
}
inline HRESULT D3DXCreateTextureFromFileInMemory_Thunk(
    IDirect3DDevice9* pDevice,
    const void* pSrcData,
    uint32_t SrcDataSize,
    IDirect3DTexture9** ppTexture
) {
    return D3DXCreateTextureFromFileInMemory(pDevice, pSrcData, SrcDataSize, ppTexture);
}
inline HRESULT D3DXCreateCubeTextureFromFileInMemory_Thunk(
    IDirect3DDevice9* pDevice,
    const void* pSrcData,
    uint32_t SrcDataSize,
    IDirect3DCubeTexture9** ppCubeTexture
) {
    return D3DXCreateCubeTextureFromFileInMemory(pDevice, pSrcData, SrcDataSize, ppCubeTexture);
}
inline HRESULT D3DXCreateVolumeTextureFromFileInMemory_Thunk(
    IDirect3DDevice9* pDevice,
    const void* pSrcData,
    uint32_t SrcDataSize,
    IDirect3DVolumeTexture9** ppVolumeTexture
) {
    return D3DXCreateVolumeTextureFromFileInMemory(pDevice, pSrcData, SrcDataSize, ppVolumeTexture);
}
