// d3dx9's shader functions as the game calls them:
// - D3DXCreateBuffer and the ID3DXBuffer it returns (NiD3DGPUProgramCache).
// - D3DXGetShaderConstantTable: the CTAB comment of compiled shader code read into an ID3DXConstantTable with the
//   methods the game calls (GetDesc, GetConstant, GetConstantByName, GetConstantDesc on top-level constants, and the
//   ID3DXBuffer methods). Its Set* methods, GetSamplerIndex and GetConstantElement are not called by the game: they
//   return D3DERR_INVALIDCALL / nullptr.
// - D3DXGetVertexShaderProfile / D3DXGetPixelShaderProfile: the profile name from the device's caps.
// - D3DXCompileShader(FromFileA) and D3DXAssembleShader(FromFileA): no HLSL compiler or assembler here. They fail with
//   E_NOTIMPL, an error buffer saying so, and one line on stderr naming the shader. The game's shaders come compiled in
//   its shader packages; it compiles only a shader a package lacks.

#include "D3DX9Internal.h"

#include <cstdlib>
#include <cstring>
#include <new>
#include <string>

using namespace d3dx;

namespace {

constexpr GUID IID_IUnknown_ = {0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
constexpr GUID IID_ID3DXBuffer = {0x8BA5FB08, 0x5195, 0x40E2, {0xAC, 0x58, 0x0D, 0x98, 0x9C, 0x3A, 0x01, 0x02}};
constexpr GUID IID_ID3DXConstantTable = {0xAB3C758F, 0x093E, 0x4356, {0xB7, 0x62, 0x4D, 0xB1, 0x8F, 0x1B, 0x3A,
	0x01}};

bool SameGuid(const GUID& arA, const GUID& arB)
{
	return !std::memcmp(&arA, &arB, sizeof(GUID));
}

// The reference count and the bytes every buffer object has; the derived COM class gives the vtable.
template <class Interface>
struct BufferBase : Interface {
	uint32_t refs = 1;
	void* data = nullptr;
	uint32_t size = 0;

	virtual ~BufferBase() { std::free(data); }

	uint32_t STDMETHODCALLTYPE AddRef() override { return ++refs; }

	uint32_t STDMETHODCALLTYPE Release() override
	{
		const uint32_t uiRefs = --refs;
		if (!uiRefs) {
			delete this;
		}
		return uiRefs;
	}

	void* STDMETHODCALLTYPE GetBufferPointer() override { return data; }
	uint32_t STDMETHODCALLTYPE GetBufferSize() override { return size; }
};

struct Buffer final : BufferBase<ID3DXBuffer> {
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
	{
		if (!ppvObject) {
			return E_POINTER;
		}
		if (SameGuid(riid, IID_IUnknown_) || SameGuid(riid, IID_ID3DXBuffer)) {
			AddRef();
			*ppvObject = this;
			return S_OK;
		}
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}
};

// The CTAB comment's structures (d3dx9shader.h, D3DXSHADER_*), offsets counted from the start of the CTAB data.
struct CtabHeader {
	uint32_t Size;
	uint32_t Creator;
	uint32_t Version;
	uint32_t Constants;
	uint32_t ConstantInfo;
	uint32_t Flags;
	uint32_t Target;
};

struct CtabConstantInfo {
	uint32_t Name;
	uint16_t RegisterSet;
	uint16_t RegisterIndex;
	uint16_t RegisterCount;
	uint16_t Reserved;
	uint32_t TypeInfo;
	uint32_t DefaultValue;
};

struct CtabTypeInfo {
	uint16_t Class;
	uint16_t Type;
	uint16_t Rows;
	uint16_t Columns;
	uint16_t Elements;
	uint16_t StructMembers;
	uint32_t StructMemberInfo;
};

struct CtabMemberInfo {
	uint32_t Name;
	uint32_t TypeInfo;
};

// D3DXPARAMETER_CLASS and D3DXPARAMETER_TYPE values the size rule needs.
constexpr uint32_t D3DXPC_STRUCT = 5;
constexpr uint32_t D3DXPT_SAMPLER = 10;
constexpr uint32_t D3DXPT_SAMPLERCUBE = 14;

struct ConstantTable final : BufferBase<ID3DXConstantTable> {
	D3DXCONSTANTTABLE_DESC desc{};
	std::vector<D3DXCONSTANT_DESC> constants;

	const char* At(uint32_t auiOffset) const
	{
		return auiOffset && auiOffset < size ? static_cast<const char*>(data) + auiOffset : nullptr;
	}

	template <class T>
	bool Read(uint32_t auiOffset, T& arOut) const
	{
		if (auiOffset > size || size - auiOffset < sizeof(T)) {
			return false;
		}
		std::memcpy(&arOut, static_cast<const char*>(data) + auiOffset, sizeof(T));
		return true;
	}

	// The bytes of a constant of this type, as d3dx9 reports them: four per scalar of each element, four per
	// sampler, the members' sum for a struct.
	uint32_t TypeBytes(const CtabTypeInfo& arType, uint32_t auiDepth) const
	{
		const uint32_t uiElements = arType.Elements ? arType.Elements : 1;
		if (arType.Type >= D3DXPT_SAMPLER && arType.Type <= D3DXPT_SAMPLERCUBE) {
			return 4 * uiElements;
		}
		if (arType.Class != D3DXPC_STRUCT) {
			return 4 * uiElements * arType.Rows * arType.Columns;
		}
		uint32_t uiBytes = 0;
		for (uint32_t i = 0; i < arType.StructMembers && auiDepth < 16; ++i) {
			CtabMemberInfo kMember;
			CtabTypeInfo kType;
			if (Read(arType.StructMemberInfo + i * sizeof(CtabMemberInfo), kMember) && Read(kMember.TypeInfo, kType)) {
				uiBytes += TypeBytes(kType, auiDepth + 1);
			}
		}
		return uiBytes * uiElements;
	}

	bool Parse()
	{
		CtabHeader kHeader;
		if (!Read(0, kHeader) || kHeader.Size != sizeof(CtabHeader)) {
			return false;
		}
		desc.Creator = At(kHeader.Creator);
		desc.Version = kHeader.Version;
		desc.Constants = kHeader.Constants;
		constants.resize(kHeader.Constants);
		for (uint32_t i = 0; i < kHeader.Constants; ++i) {
			CtabConstantInfo kInfo;
			CtabTypeInfo kType;
			if (!Read(kHeader.ConstantInfo + i * sizeof(CtabConstantInfo), kInfo) || !Read(kInfo.TypeInfo, kType)) {
				return false;
			}
			D3DXCONSTANT_DESC& rDesc = constants[i];
			rDesc.Name = At(kInfo.Name);
			rDesc.RegisterSet = kInfo.RegisterSet;
			rDesc.RegisterIndex = kInfo.RegisterIndex;
			rDesc.RegisterCount = kInfo.RegisterCount;
			rDesc.Class = kType.Class;
			rDesc.Type = kType.Type;
			rDesc.Rows = kType.Rows;
			rDesc.Columns = kType.Columns;
			rDesc.Elements = kType.Elements;
			rDesc.StructMembers = kType.StructMembers;
			rDesc.Bytes = TypeBytes(kType, 0);
			rDesc.DefaultValue = At(kInfo.DefaultValue);
		}
		return true;
	}

	// A handle is a pointer to one of the constants' descriptions; anything else is read as a constant's name.
	const D3DXCONSTANT_DESC* Find(D3DXHANDLE ahConstant) const
	{
		if (!ahConstant) {
			return nullptr;
		}
		const auto* pDesc = reinterpret_cast<const D3DXCONSTANT_DESC*>(ahConstant);
		if (!constants.empty() && pDesc >= constants.data() && pDesc < constants.data() + constants.size()) {
			return pDesc;
		}
		for (const D3DXCONSTANT_DESC& rDesc : constants) {
			if (rDesc.Name && !std::strcmp(rDesc.Name, ahConstant)) {
				return &rDesc;
			}
		}
		return nullptr;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
	{
		if (!ppvObject) {
			return E_POINTER;
		}
		if (SameGuid(riid, IID_IUnknown_) || SameGuid(riid, IID_ID3DXBuffer) ||
			SameGuid(riid, IID_ID3DXConstantTable)) {
			AddRef();
			*ppvObject = this;
			return S_OK;
		}
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	HRESULT STDMETHODCALLTYPE GetDesc(D3DXCONSTANTTABLE_DESC* pDesc) override
	{
		if (!pDesc) {
			return D3DERR_INVALIDCALL;
		}
		*pDesc = desc;
		return D3D_OK;
	}

	HRESULT STDMETHODCALLTYPE GetConstantDesc(D3DXHANDLE hConstant, D3DXCONSTANT_DESC* pConstantDesc,
		uint32_t* pCount) override
	{
		const D3DXCONSTANT_DESC* pDesc = Find(hConstant);
		if (!pDesc) {
			return D3DERR_INVALIDCALL;
		}
		if (pConstantDesc) {
			*pConstantDesc = *pDesc;
		}
		if (pCount) {
			*pCount = 1;
		}
		return D3D_OK;
	}

	uint32_t STDMETHODCALLTYPE GetSamplerIndex(D3DXHANDLE) override { return static_cast<uint32_t>(-1); }

	// Top-level constants only: the game never asks for a struct's members.
	D3DXHANDLE STDMETHODCALLTYPE GetConstant(D3DXHANDLE hConstant, uint32_t Index) override
	{
		if (hConstant || Index >= constants.size()) {
			return nullptr;
		}
		return reinterpret_cast<D3DXHANDLE>(&constants[Index]);
	}

	D3DXHANDLE STDMETHODCALLTYPE GetConstantByName(D3DXHANDLE hConstant, const char* pName) override
	{
		if (hConstant || !pName) {
			return nullptr;
		}
		return reinterpret_cast<D3DXHANDLE>(Find(pName));
	}

	D3DXHANDLE STDMETHODCALLTYPE GetConstantElement(D3DXHANDLE, uint32_t) override { return nullptr; }

	HRESULT STDMETHODCALLTYPE SetDefaults(IDirect3DDevice9*) override { return D3DERR_INVALIDCALL; }
	HRESULT STDMETHODCALLTYPE SetValue(IDirect3DDevice9*, D3DXHANDLE, const void*, uint32_t) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetBool(IDirect3DDevice9*, D3DXHANDLE, int32_t) override { return D3DERR_INVALIDCALL; }
	HRESULT STDMETHODCALLTYPE SetBoolArray(IDirect3DDevice9*, D3DXHANDLE, const int32_t*, uint32_t) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetInt(IDirect3DDevice9*, D3DXHANDLE, int32_t) override { return D3DERR_INVALIDCALL; }
	HRESULT STDMETHODCALLTYPE SetIntArray(IDirect3DDevice9*, D3DXHANDLE, const int32_t*, uint32_t) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetFloat(IDirect3DDevice9*, D3DXHANDLE, float) override { return D3DERR_INVALIDCALL; }
	HRESULT STDMETHODCALLTYPE SetFloatArray(IDirect3DDevice9*, D3DXHANDLE, const float*, uint32_t) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetVector(IDirect3DDevice9*, D3DXHANDLE, const D3DXVECTOR4*) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetVectorArray(IDirect3DDevice9*, D3DXHANDLE, const D3DXVECTOR4*, uint32_t) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetMatrix(IDirect3DDevice9*, D3DXHANDLE, const D3DXMATRIX*) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetMatrixArray(IDirect3DDevice9*, D3DXHANDLE, const D3DXMATRIX*, uint32_t) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetMatrixPointerArray(IDirect3DDevice9*, D3DXHANDLE, const D3DXMATRIX**, uint32_t) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetMatrixTranspose(IDirect3DDevice9*, D3DXHANDLE, const D3DXMATRIX*) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetMatrixTransposeArray(IDirect3DDevice9*, D3DXHANDLE, const D3DXMATRIX*, uint32_t) override
	{
		return D3DERR_INVALIDCALL;
	}
	HRESULT STDMETHODCALLTYPE SetMatrixTransposePointerArray(IDirect3DDevice9*, D3DXHANDLE, const D3DXMATRIX**,
		uint32_t) override
	{
		return D3DERR_INVALIDCALL;
	}
};

constexpr uint32_t CTAB_FOURCC = 0x42415443;

// The CTAB comment of shader code, as its data (after the FourCC) and size in bytes; nullptr when there is none. The
// compiler writes it right after the version token; the walk also steps over SM2+ instructions by their length.
const uint8_t* FindCtab(const uint32_t* apFunction, uint32_t& arSize)
{
	const uint32_t uiVersion = apFunction[0];
	if ((uiVersion & 0xFFFE0000) != 0xFFFE0000) {
		return nullptr;
	}
	const uint32_t uiMajor = (uiVersion >> 8) & 0xFF;
	const uint32_t* pToken = apFunction + 1;
	// Bounded: shader code is never this long.
	for (uint32_t uiStep = 0; uiStep < 0x100000; ++uiStep) {
		const uint32_t uiToken = *pToken;
		if (uiToken == 0x0000FFFF) {
			return nullptr;
		}
		if ((uiToken & 0xFFFF) == 0xFFFE) {
			const uint32_t uiLength = (uiToken >> 16) & 0x7FFF;
			if (uiLength >= 1 && pToken[1] == CTAB_FOURCC) {
				arSize = (uiLength - 1) * 4;
				return reinterpret_cast<const uint8_t*>(pToken + 2);
			}
			pToken += 1 + uiLength;
		} else if (uiMajor >= 2) {
			pToken += 1 + ((uiToken >> 24) & 0xF);
		} else {
			return nullptr;
		}
	}
	return nullptr;
}

// The shader tools' failure: an error buffer and one line naming the shader.
HRESULT Unsupported(const char* apWhat, const std::string& arShader, ID3DXBuffer** appShader,
	ID3DXBuffer** appErrorMsgs, ID3DXConstantTable** appConstantTable)
{
	const std::string kMessage = std::string(apWhat) + " is not available (d3dx9 stand-in): " + arShader;
	Log("%s", kMessage.c_str());
	if (appShader) {
		*appShader = nullptr;
	}
	if (appConstantTable) {
		*appConstantTable = nullptr;
	}
	if (appErrorMsgs) {
		*appErrorMsgs = NewBuffer(static_cast<uint32_t>(kMessage.size() + 1), kMessage.c_str());
	}
	return E_NOTIMPL;
}

// A shader given as source text: its entry point and profile, its defines, and its first line.
std::string Describe(const char* apSrcData, uint32_t auiSrcDataLen, const D3DXMACRO* apDefines,
	const char* apFunctionName, const char* apProfile)
{
	std::string kText = std::string(apFunctionName ? apFunctionName : "") + " " + (apProfile ? apProfile : "");
	for (const D3DXMACRO* pDefine = apDefines; pDefine && pDefine->Name; ++pDefine) {
		kText += std::string(" /D") + pDefine->Name;
		if (pDefine->Definition && *pDefine->Definition) {
			kText += std::string("=") + pDefine->Definition;
		}
	}
	if (apSrcData) {
		uint32_t uiLength = 0;
		while (uiLength < auiSrcDataLen && uiLength < 80 && apSrcData[uiLength] != '\n' && apSrcData[uiLength] != '\r') {
			++uiLength;
		}
		kText += " (" + std::to_string(auiSrcDataLen) + " bytes: \"" + std::string(apSrcData, uiLength) + "\")";
	}
	return kText;
}

} // namespace

namespace d3dx {

ID3DXBuffer* NewBuffer(uint32_t auiBytes, const void* apData)
{
	Buffer* pBuffer = new (std::nothrow) Buffer;
	if (!pBuffer) {
		return nullptr;
	}
	pBuffer->data = std::calloc(auiBytes ? auiBytes : 1, 1);
	if (!pBuffer->data) {
		delete pBuffer;
		return nullptr;
	}
	pBuffer->size = auiBytes;
	if (apData) {
		std::memcpy(pBuffer->data, apData, auiBytes);
	}
	return pBuffer;
}

} // namespace d3dx

extern "C" HRESULT WINAPI D3DXCreateBuffer(uint32_t NumBytes, ID3DXBuffer** ppBuffer)
{
	if (!ppBuffer) {
		return D3DERR_INVALIDCALL;
	}
	*ppBuffer = NewBuffer(NumBytes);
	return *ppBuffer ? D3D_OK : E_OUTOFMEMORY;
}

extern "C" HRESULT WINAPI D3DXGetShaderConstantTable(const uint32_t* pFunction,
	ID3DXConstantTable** ppConstantTable)
{
	if (!pFunction || !ppConstantTable) {
		return D3DERR_INVALIDCALL;
	}
	*ppConstantTable = nullptr;
	uint32_t uiSize = 0;
	const uint8_t* pCtab = FindCtab(pFunction, uiSize);
	if (!pCtab) {
		return D3DXERR_INVALIDDATA;
	}
	ConstantTable* pTable = new (std::nothrow) ConstantTable;
	if (!pTable) {
		return E_OUTOFMEMORY;
	}
	pTable->data = std::malloc(uiSize ? uiSize : 1);
	if (!pTable->data) {
		delete pTable;
		return E_OUTOFMEMORY;
	}
	std::memcpy(pTable->data, pCtab, uiSize);
	pTable->size = uiSize;
	if (!pTable->Parse()) {
		pTable->Release();
		return D3DXERR_INVALIDDATA;
	}
	*ppConstantTable = pTable;
	return D3D_OK;
}

namespace {

constexpr uint32_t D3DVS20CAPS_PREDICATION = 0x1;
constexpr uint32_t D3DPS20CAPS_ARBITRARYSWIZZLE = 0x1;
constexpr uint32_t D3DPS20CAPS_GRADIENTINSTRUCTIONS = 0x2;
constexpr uint32_t D3DPS20CAPS_PREDICATION = 0x4;
constexpr uint32_t D3DPS20CAPS_NODEPENDENTREADLIMIT = 0x8;
constexpr uint32_t D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT = 0x10;

} // namespace

// The highest profile the caps allow: 3_0, else 2_a (the 2.x extras d3dx9 tests), else 2_0, else 1_1 for version 1.1
// exactly (d3dx9 names none for vertex shader versions 1.2 to 1.4, which do not exist).
extern "C" const char* WINAPI D3DXGetVertexShaderProfile(IDirect3DDevice9* pDevice)
{
	D3DCAPS9 kCaps;
	if (!pDevice || pDevice->GetDeviceCaps(&kCaps) < 0) {
		return nullptr;
	}
	const uint32_t uiVersion = kCaps.VertexShaderVersion & 0xFFFF;
	if (uiVersion >= 0x0300) {
		return "vs_3_0";
	}
	if (uiVersion >= 0x0200) {
		if (kCaps.VS20Caps.NumTemps >= 13 && kCaps.VS20Caps.DynamicFlowControlDepth == 24 &&
			(kCaps.VS20Caps.Caps & D3DVS20CAPS_PREDICATION)) {
			return "vs_2_a";
		}
		return "vs_2_0";
	}
	if (uiVersion == 0x0101) {
		return "vs_1_1";
	}
	return nullptr;
}

// As the vertex form, with 2_b (32 temporaries, no texture instruction limit) and the 1_x versions.
extern "C" const char* WINAPI D3DXGetPixelShaderProfile(IDirect3DDevice9* pDevice)
{
	D3DCAPS9 kCaps;
	if (!pDevice || pDevice->GetDeviceCaps(&kCaps) < 0) {
		return nullptr;
	}
	const uint32_t uiVersion = kCaps.PixelShaderVersion & 0xFFFF;
	const uint32_t uiCaps = kCaps.PS20Caps.Caps;
	if (uiVersion >= 0x0300) {
		return "ps_3_0";
	}
	if (uiVersion >= 0x0200) {
		if (kCaps.PS20Caps.NumTemps >= 22 && (uiCaps & D3DPS20CAPS_ARBITRARYSWIZZLE) &&
			(uiCaps & D3DPS20CAPS_GRADIENTINSTRUCTIONS) && (uiCaps & D3DPS20CAPS_PREDICATION) &&
			(uiCaps & D3DPS20CAPS_NODEPENDENTREADLIMIT) && (uiCaps & D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT)) {
			return "ps_2_a";
		}
		if (kCaps.PS20Caps.NumTemps >= 32 && (uiCaps & D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT)) {
			return "ps_2_b";
		}
		return "ps_2_0";
	}
	if (uiVersion >= 0x0104) {
		return "ps_1_4";
	}
	if (uiVersion >= 0x0103) {
		return "ps_1_3";
	}
	if (uiVersion >= 0x0102) {
		return "ps_1_2";
	}
	if (uiVersion >= 0x0101) {
		return "ps_1_1";
	}
	return nullptr;
}

extern "C" HRESULT WINAPI D3DXCompileShader(const char* pSrcData, uint32_t SrcDataLen,
	const D3DXMACRO* pDefines, ID3DXInclude* pInclude, const char* pFunctionName, const char* pProfile,
	uint32_t Flags, ID3DXBuffer** ppShader, ID3DXBuffer** ppErrorMsgs, ID3DXConstantTable** ppConstantTable)
{
	(void)pInclude;
	(void)Flags;
	return Unsupported("HLSL compile", Describe(pSrcData, SrcDataLen, pDefines, pFunctionName, pProfile), ppShader,
		ppErrorMsgs, ppConstantTable);
}

extern "C" HRESULT WINAPI D3DXCompileShaderFromFileA(const char* pSrcFile, const D3DXMACRO* pDefines,
	ID3DXInclude* pInclude, const char* pFunctionName, const char* pProfile, uint32_t Flags, ID3DXBuffer** ppShader,
	ID3DXBuffer** ppErrorMsgs, ID3DXConstantTable** ppConstantTable)
{
	(void)pInclude;
	(void)Flags;
	return Unsupported("HLSL compile",
		std::string(pSrcFile ? pSrcFile : "") + " " + Describe(nullptr, 0, pDefines, pFunctionName, pProfile),
		ppShader, ppErrorMsgs, ppConstantTable);
}

extern "C" HRESULT WINAPI D3DXAssembleShader(const char* pSrcData, uint32_t SrcDataLen,
	const D3DXMACRO* pDefines, ID3DXInclude* pInclude, uint32_t Flags, ID3DXBuffer** ppShader,
	ID3DXBuffer** ppErrorMsgs)
{
	(void)pInclude;
	(void)Flags;
	return Unsupported("shader assembly", Describe(pSrcData, SrcDataLen, pDefines, nullptr, nullptr), ppShader,
		ppErrorMsgs, nullptr);
}

extern "C" HRESULT WINAPI D3DXAssembleShaderFromFileA(const char* pSrcFile, const D3DXMACRO* pDefines,
	ID3DXInclude* pInclude, uint32_t Flags, ID3DXBuffer** ppShader, ID3DXBuffer** ppErrorMsgs)
{
	(void)pInclude;
	(void)Flags;
	return Unsupported("shader assembly",
		std::string(pSrcFile ? pSrcFile : "") + Describe(nullptr, 0, pDefines, nullptr, nullptr), ppShader,
		ppErrorMsgs, nullptr);
}
