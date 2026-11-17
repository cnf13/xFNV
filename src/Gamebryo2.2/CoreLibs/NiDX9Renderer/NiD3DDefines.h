#pragma once

#include "Reconstruction.hpp"
#include "SDK/DirectX/d3d9.h"
#include "SDK/Windows/dinput.h"

// DirectInput8Create's thunk, which only calls the slot. IDA name; not in the PDB.
HRESULT DirectInput8Create_0(HINSTANCE__* hinst, uint32_t dwVersion, const GUID* riidltf, void** ppvOut,
	IUnknown* punkOuter);
// The exe's D3DX import thunks 0xEE6DF8 and 0xEE6E40, each a `jmp [slot]` to the slot of the same name (d3d9.h); the
// slots hold the PDB names, so the thunks carry a _Thunk suffix here. Defined and registered in NiD3DDefines.cpp.
D3DXVECTOR3* D3DXVec3TransformCoord_Thunk(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM);
HRESULT D3DXGetShaderConstantTable_Thunk(const uint32_t* pFunction, ID3DXConstantTable** ppConstantTable);
// The exe's D3DXGetPixelShaderProfile import thunk 0xEE6E28 (`jmp [0xFDF3F4]`); defined in NiD3DDefines.cpp.
const char* D3DXGetPixelShaderProfile_Thunk(IDirect3DDevice9* pDevice);
// The exe's D3DXCreateBuffer import thunk 0xEE6E3A (`jmp [0xFDF3E8]`); defined in NiD3DDefines.cpp.
HRESULT D3DXCreateBuffer_Thunk(uint32_t NumBytes, ID3DXBuffer** ppBuffer);
// The exe's D3DXGetVertexShaderProfile import thunk 0xEE6E2E (`jmp [0xFDF3F0]`); defined in NiD3DDefines.cpp.
const char* D3DXGetVertexShaderProfile_Thunk(IDirect3DDevice9* pDevice);
// The exe's D3DXAssembleShaderFromFileA import thunk 0xEE6E4C (`jmp [0xFDF3DC]`; IDA: D3DXAssembleShaderFromFileA_0);
// NiD3DShaderProgramCreatorAsm calls it. Defined in NiD3DDefines.cpp.
HRESULT D3DXAssembleShaderFromFileA_Thunk(const char* pSrcFile, const D3DXMACRO* pDefines, ID3DXInclude* pInclude,
	uint32_t Flags, ID3DXBuffer** ppShader, ID3DXBuffer** ppErrorMsgs);
// The exe's D3DXCompileShaderFromFileA import thunk 0xEE6E46 (`jmp [0xFDF3E0]`); NiD3DShaderProgramCreatorHLSL calls
// it. Defined in NiD3DDefines.cpp.
HRESULT D3DXCompileShaderFromFileA_Thunk(const char* pSrcFile, const D3DXMACRO* pDefines, ID3DXInclude* pInclude,
	const char* pFunctionName, const char* pProfile, uint32_t Flags, ID3DXBuffer** ppShader, ID3DXBuffer** ppErrorMsgs,
	ID3DXConstantTable** ppConstantTable);
// The exe's D3DXAssembleShader import thunk 0xEE6E52 (`jmp [0xFDF3D8]`); NiD3DShaderProgramCreatorAsm calls it.
// Defined in NiD3DDefines.cpp.
HRESULT D3DXAssembleShader_Thunk(const char* pSrcData, uint32_t SrcDataLen, const D3DXMACRO* pDefines,
	ID3DXInclude* pInclude, uint32_t Flags, ID3DXBuffer** ppShader, ID3DXBuffer** ppErrorMsgs);

// The exe's thunks of the Direct3D 9, D3DX and DirectInput imports (SDK/DirectX/d3d9.h, SDK/Windows/dinput.h).

// The exe's `jmp [slot]` thunks of the D3DX imports, for the callers that call the thunk rather than the slot
// (NiDX9Renderer::Do_SetCameraData 0xE6C780). Same stdcall signatures as the slots above. The PDB names each thunk
// after its import (D3DXVec4Transform, ...), which the slot overlays above already use, so these carry IDA's name
// or a suffix. D3DXVec3TransformCoord_Thunk (0xEE6DF8) is declared above.
// IDA name. The thunk of D3DXVec4Transform (`jmp [0xFDF3EC]`, 0xEE6E34).
inline D3DXVECTOR4* D3DXVec4Transform_0(D3DXVECTOR4* pOut, const D3DXVECTOR4* pV, const D3DXMATRIX* pM) {
	return D3DXVec4Transform(pOut, pV, pM);
}
// PDB name D3DXVec3Normalize, suffixed here. The thunk of D3DXVec3Normalize (`jmp [0xFDF41C]`, 0xEE6DEC).
inline D3DXVECTOR3* D3DXVec3Normalize_Thunk(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV) {
	return D3DXVec3Normalize(pOut, pV);
}
// PDB name D3DXVec3TransformNormal, suffixed here. The thunk of D3DXVec3TransformNormal (`jmp [0xFDF420]`, 0xEE6DF2).
inline D3DXVECTOR3* D3DXVec3TransformNormal_Thunk(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM) {
	return D3DXVec3TransformNormal(pOut, pV, pM);
}
// PDB name D3DXMatrixMultiply, suffixed here. The thunk (`jmp [0xFDF424]`, 0xEE6DE0).
inline D3DXMATRIX* D3DXMatrixMultiply_Thunk(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2) {
	return D3DXMatrixMultiply(pOut, pM1, pM2);
}
// PDB name D3DXMatrixMultiplyTranspose, suffixed here. The thunk (`jmp [0xFDF410]`, 0xEE6DFE).
inline D3DXMATRIX* D3DXMatrixMultiplyTranspose_Thunk(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2) {
	return D3DXMatrixMultiplyTranspose(pOut, pM1, pM2);
}
// PDB name D3DXMatrixInverse, suffixed here. The thunk (`jmp [0xFDF428]`, 0xEE6DDA).
inline D3DXMATRIX* D3DXMatrixInverse_Thunk(D3DXMATRIX* pOut, float* pDeterminant, const D3DXMATRIX* pM) {
	return D3DXMatrixInverse(pOut, pDeterminant, pM);
}
// PDB name D3DXMatrixRotationYawPitchRoll, suffixed here. The thunk (`jmp [0xFDF40C]`, 0xEE6E04).
inline D3DXMATRIX* D3DXMatrixRotationYawPitchRoll_Thunk(D3DXMATRIX* pOut, float Yaw, float Pitch, float Roll) {
	return D3DXMatrixRotationYawPitchRoll(pOut, Yaw, Pitch, Roll);
}
// PDB name D3DXMatrixTranspose, suffixed here. The thunk (`jmp [0xFDF42C]`, 0xEE6DD4).
inline D3DXMATRIX* D3DXMatrixTranspose_Thunk(D3DXMATRIX* pOut, const D3DXMATRIX* pM) {
	return D3DXMatrixTranspose(pOut, pM);
}
// PDB name D3DXPlaneNormalize, suffixed here. The thunk (`jmp [0xFDF430]`, 0xEE6DCE).
inline D3DXPLANE* D3DXPlaneNormalize_Thunk(D3DXPLANE* pOut, const D3DXPLANE* pP) {
	return D3DXPlaneNormalize(pOut, pP);
}
// PDB name D3DXPlaneTransform, suffixed here. The thunk (`jmp [0xFDF418]`, 0xEE6DC8).
inline D3DXPLANE* D3DXPlaneTransform_Thunk(D3DXPLANE* pOut, const D3DXPLANE* pP, const D3DXMATRIX* pM) {
	return D3DXPlaneTransform(pOut, pP, pM);
}
// PDB name D3DXCompileShader, suffixed here. The thunk (`jmp [0xFDF408]`, 0xEE6E0A).
inline HRESULT D3DXCompileShader_Thunk(const char* pSrcData, uint32_t SrcDataLen, const D3DXMACRO* pDefines,
	ID3DXInclude* pInclude, const char* pFunctionName, const char* pProfile, uint32_t Flags, ID3DXBuffer** ppShader,
	ID3DXBuffer** ppErrorMsgs, ID3DXConstantTable** ppConstantTable) {
	return D3DXCompileShader(pSrcData, SrcDataLen, pDefines, pInclude, pFunctionName, pProfile, Flags, ppShader,
		ppErrorMsgs, ppConstantTable);
}
// PDB name D3DXSaveTextureToFileA, suffixed here. The thunk (`jmp [0xFDF3D4]`, 0xEE6DC2).
inline HRESULT D3DXSaveTextureToFileA_Thunk(const char* pDestFile, uint32_t DestFormat,
	IDirect3DBaseTexture9* pSrcTexture, const PALETTEENTRY* pSrcPalette) {
	return D3DXSaveTextureToFileA(pDestFile, DestFormat, pSrcTexture, pSrcPalette);
}
// PDB name D3DXGetImageInfoFromFileInMemory, suffixed here. The thunk (`jmp [0xFDF3F8]`, 0xEE6E22).
inline HRESULT D3DXGetImageInfoFromFileInMemory_Thunk(const void* pSrcData, uint32_t SrcDataSize,
	D3DXIMAGE_INFO* pSrcInfo) {
	return D3DXGetImageInfoFromFileInMemory(pSrcData, SrcDataSize, pSrcInfo);
}
// PDB name D3DXCreateTextureFromFileInMemory, suffixed here. The thunk (`jmp [0xFDF3FC]`, 0xEE6E1C).
inline HRESULT D3DXCreateTextureFromFileInMemory_Thunk(IDirect3DDevice9* pDevice, const void* pSrcData,
	uint32_t SrcDataSize, IDirect3DTexture9** ppTexture) {
	return D3DXCreateTextureFromFileInMemory(pDevice, pSrcData, SrcDataSize, ppTexture);
}
// PDB name D3DXCreateCubeTextureFromFileInMemory, suffixed here. The thunk (`jmp [0xFDF400]`, 0xEE6E16).
inline HRESULT D3DXCreateCubeTextureFromFileInMemory_Thunk(IDirect3DDevice9* pDevice, const void* pSrcData,
	uint32_t SrcDataSize, IDirect3DCubeTexture9** ppCubeTexture) {
	return D3DXCreateCubeTextureFromFileInMemory(pDevice, pSrcData, SrcDataSize, ppCubeTexture);
}
// PDB name D3DXCreateVolumeTextureFromFileInMemory, suffixed here. The thunk (`jmp [0xFDF404]`, 0xEE6E10).
inline HRESULT D3DXCreateVolumeTextureFromFileInMemory_Thunk(IDirect3DDevice9* pDevice, const void* pSrcData,
	uint32_t SrcDataSize, IDirect3DVolumeTexture9** ppVolumeTexture) {
	return D3DXCreateVolumeTextureFromFileInMemory(pDevice, pSrcData, SrcDataSize, ppVolumeTexture);
}
