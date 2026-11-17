#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include <cstdint>

struct IDirect3DDevice9;
struct IDirect3DPixelShader9;
struct IDirect3DVertexDeclaration9;
struct IDirect3DVertexShader9;
class NiD3DPixelShader;
class NiD3DVertexShader;
class NiDX9Renderer;

// Compiles and creates the shader programs of one file format for NiD3DShaderProgramFactory. Only some of the members
// are declared.
class NiD3DShaderProgramCreator : public NiMemObject {
public:
	virtual ~NiD3DShaderProgramCreator();

	// The vertex declaration is the PC's NiD3DVertexDeclaration (an IDirect3DVertexDeclaration9*), not
	// D3DVertexDeclaration*.
	virtual uint32_t GetShaderCreationFlags() const = 0;
	virtual void SetShaderCreationFlags(uint32_t) = 0;
	// bSoftwareVP and bRecoverable are int32_t rather than bool so that the caller's whole 32-bit value is passed on; a
	// bool would be zero-extended.
	virtual NiD3DVertexShader* CreateVertexShaderFromFile(const char*, const char*, const char*, const char*,
		IDirect3DVertexDeclaration9*, uint32_t, int32_t bSoftwareVP, int32_t bRecoverable) = 0;
	// bSoftwareVP and bRecoverable are int32_t rather than bool so that the caller's whole 32-bit value is passed on; a
	// bool would be zero-extended.
	virtual NiD3DVertexShader* CreateVertexShaderFromBuffer(const void*, uint32_t, const char*, const char*,
		const char*, IDirect3DVertexDeclaration9*, uint32_t, int32_t bSoftwareVP, int32_t bRecoverable) = 0;
	// bRecoverable is an int32_t rather than a bool so that the caller's whole 32-bit value is passed on; a bool would
	// be zero-extended.
	virtual NiD3DPixelShader* CreatePixelShaderFromFile(const char*, const char*, const char*, const char*,
		int32_t bRecoverable) = 0;
	// bRecoverable is an int32_t rather than a bool so that the caller's whole 32-bit value is passed on; a bool would
	// be zero-extended.
	virtual NiD3DPixelShader* CreatePixelShaderFromBuffer(const void*, uint32_t, const char*, const char*, const char*,
		int32_t bRecoverable) = 0;
	virtual bool RecreateVertexShader(NiD3DVertexShader*) = 0;
	virtual bool RecreatePixelShader(NiD3DPixelShader*) = 0;

	// Creates the device's vertex shader from pvCode, null on failure; the arguments other than pvCode are ignored.
	// bSoftwareVP is not read. It is declared int32_t rather than bool so the caller's whole value is passed on; a bool
	// would be zero-extended.
	IDirect3DVertexShader9* CreateVertexShader(void* pvCode, IDirect3DVertexDeclaration9*& hDecl, uint32_t uiUsage,
		void* pvConstantData, uint32_t uiConstantSize, int32_t bSoftwareVP);
	// Creates the device's pixel shader from pvCode, null on failure.
	IDirect3DPixelShader9* CreatePixelShader(void* pvCode);

	// Holds the renderer and a reference on its device; null drops the device.
	static void SetD3DRenderer(NiDX9Renderer* pkD3DRenderer);

	static inline IDirect3DDevice9* ms_pkD3DDevice;
	static inline NiDX9Renderer* ms_pkD3DRenderer;
	// The flags every creator ORs into its own when it compiles or assembles shader code.
	static inline uint32_t ms_uiUniversalShaderCreationFlags;
};
