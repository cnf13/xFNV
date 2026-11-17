#pragma once

#include "Reconstruction.hpp"
#include "SDK/Windows/objbase.h"
#include "SDK/Windows/unknwn.h"
#include "SDK/Windows/windef.h"
#include "SDK/Windows/winnt.h"

// The Direct3D 9 and D3DX (d3dx9_38) SDK declarations the renderer uses, under the SDK's names and with its 32-bit
// layouts, so no Direct3D header is needed:
//   - COM interfaces in the SDK's vtable order (IUnknown's three methods first). Every method is stdcall with `this`
//     pushed last, which STDMETHODCALLTYPE gives a virtual member (clang-cl would otherwise make it thiscall).
//   - Enums, DWORD/UINT/ULONG are uint32_t; BOOL/INT are int32_t; HRESULT is winnt.h's; handles are pointers as
//     windows.h declares them (HWND__*, HMONITOR__*, HDC__*, HANDLE), so they are 8 bytes on x64 as dxvk-native has
//     them.
//   - The exe's d3d9.dll / d3dx9_38.dll imports, at the end of this file.

typedef struct _LUID {
    uint32_t LowPart;
    int32_t HighPart;
} LUID;

// WM_WINDOWPOSCHANGED's lParam (the same as windows.h declares it).
typedef struct tagWINDOWPOS {
    HWND__* hwnd;
    HWND__* hwndInsertAfter;
    int32_t x;
    int32_t y;
    int32_t cx;
    int32_t cy;
    uint32_t flags;
} WINDOWPOS;

typedef struct tagPALETTEENTRY {
    uint8_t peRed;
    uint8_t peGreen;
    uint8_t peBlue;
    uint8_t peFlags;
} PALETTEENTRY;

// Only passed by pointer.
typedef struct _RGNDATA RGNDATA;
typedef struct _D3DRECTPATCH_INFO D3DRECTPATCH_INFO;
typedef struct _D3DTRIPATCH_INFO D3DTRIPATCH_INFO;

struct IUnknown;
struct IDirect3D9;
struct IDirect3D9Ex;
struct IDirect3DDevice9;
struct IDirect3DDevice9Ex;
struct IDirect3DSwapChain9;
struct IDirect3DResource9;
struct IDirect3DBaseTexture9;
struct IDirect3DTexture9;
struct IDirect3DCubeTexture9;
struct IDirect3DVolumeTexture9;
struct IDirect3DSurface9;
struct IDirect3DVolume9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;
struct IDirect3DVertexDeclaration9;
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;
struct IDirect3DQuery9;
struct IDirect3DStateBlock9;
struct ID3DXBuffer;
struct ID3DXConstantTable;
struct ID3DXInclude;

typedef struct _D3DMATRIX {
    float _11;
    float _12;
    float _13;
    float _14;
    float _21;
    float _22;
    float _23;
    float _24;
    float _31;
    float _32;
    float _33;
    float _34;
    float _41;
    float _42;
    float _43;
    float _44;

public:
    // The copy is the exe's 0x40-byte memcpy; the default constructor, which the copy constructor would suppress,
    // leaves the matrix uninitialised, as the SDK's does.
    _D3DMATRIX() = default;
    _D3DMATRIX(const _D3DMATRIX& a1) = default;
    // D3DXMATRIX::operator*(const D3DXMATRIX&): this times mat (thiscall, ret 8, 0x4ED140;
    // the result arrives through the hidden pointer).
    _D3DMATRIX operator*(const _D3DMATRIX& mat) const;
} D3DMATRIX;

typedef struct _D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
} D3DCOLORVALUE;

typedef struct _D3DMATERIAL9 {
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Ambient;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Emissive;
    float Power;
} D3DMATERIAL9;

typedef struct _D3DVIEWPORT9 {
    uint32_t X;
    uint32_t Y;
    uint32_t Width;
    uint32_t Height;
    float MinZ;
    float MaxZ;
} D3DVIEWPORT9;

typedef struct _D3DVERTEXBUFFER_DESC {
    uint32_t Format;
    uint32_t Type;
    uint32_t Usage;
    uint32_t Pool;
    uint32_t Size;
    uint32_t FVF;
} D3DVERTEXBUFFER_DESC;

// d3d9types.h and d3d9caps.h pack to 4 bytes on x86 only (D3DADAPTER_IDENTIFIER9's LARGE_INTEGER sits at 0x420
// unpadded); on x64 they keep natural alignment (D3DPRESENT_PARAMETERS's window handle at 0x20).
#if !FNVD_64BIT
#pragma pack(push, 4)
#endif

typedef struct _D3DVECTOR {
    float x;
    float y;
    float z;
} D3DVECTOR;

typedef struct _D3DVSHADERCAPS2_0 {
    uint32_t Caps;
    int32_t DynamicFlowControlDepth;
    int32_t NumTemps;
    int32_t StaticFlowControlDepth;
} D3DVSHADERCAPS2_0;

typedef struct _D3DPSHADERCAPS2_0 {
    uint32_t Caps;
    int32_t DynamicFlowControlDepth;
    int32_t NumTemps;
    int32_t StaticFlowControlDepth;
    int32_t NumInstructionSlots;
} D3DPSHADERCAPS2_0;

typedef struct _D3DCAPS9 {
    uint32_t DeviceType;
    uint32_t AdapterOrdinal;
    uint32_t Caps;
    uint32_t Caps2;
    uint32_t Caps3;
    uint32_t PresentationIntervals;
    uint32_t CursorCaps;
    uint32_t DevCaps;
    uint32_t PrimitiveMiscCaps;
    uint32_t RasterCaps;
    uint32_t ZCmpCaps;
    uint32_t SrcBlendCaps;
    uint32_t DestBlendCaps;
    uint32_t AlphaCmpCaps;
    uint32_t ShadeCaps;
    uint32_t TextureCaps;
    uint32_t TextureFilterCaps;
    uint32_t CubeTextureFilterCaps;
    uint32_t VolumeTextureFilterCaps;
    uint32_t TextureAddressCaps;
    uint32_t VolumeTextureAddressCaps;
    uint32_t LineCaps;
    uint32_t MaxTextureWidth;
    uint32_t MaxTextureHeight;
    uint32_t MaxVolumeExtent;
    uint32_t MaxTextureRepeat;
    uint32_t MaxTextureAspectRatio;
    uint32_t MaxAnisotropy;
    float MaxVertexW;
    float GuardBandLeft;
    float GuardBandTop;
    float GuardBandRight;
    float GuardBandBottom;
    float ExtentsAdjust;
    uint32_t StencilCaps;
    uint32_t FVFCaps;
    uint32_t TextureOpCaps;
    uint32_t MaxTextureBlendStages;
    uint32_t MaxSimultaneousTextures;
    uint32_t VertexProcessingCaps;
    uint32_t MaxActiveLights;
    uint32_t MaxUserClipPlanes;
    uint32_t MaxVertexBlendMatrices;
    uint32_t MaxVertexBlendMatrixIndex;
    float MaxPointSize;
    uint32_t MaxPrimitiveCount;
    uint32_t MaxVertexIndex;
    uint32_t MaxStreams;
    uint32_t MaxStreamStride;
    uint32_t VertexShaderVersion;
    uint32_t MaxVertexShaderConst;
    uint32_t PixelShaderVersion;
    float PixelShader1xMaxValue;
    uint32_t DevCaps2;
    float MaxNpatchTessellationLevel;
    uint32_t Reserved5;
    uint32_t MasterAdapterOrdinal;
    uint32_t AdapterOrdinalInGroup;
    uint32_t NumberOfAdaptersInGroup;
    uint32_t DeclTypes;
    uint32_t NumSimultaneousRTs;
    uint32_t StretchRectFilterCaps;
    D3DVSHADERCAPS2_0 VS20Caps;
    D3DPSHADERCAPS2_0 PS20Caps;
    uint32_t VertexTextureFilterCaps;
    uint32_t MaxVShaderInstructionsExecuted;
    uint32_t MaxPShaderInstructionsExecuted;
    uint32_t MaxVertexShader30InstructionSlots;
    uint32_t MaxPixelShader30InstructionSlots;
} D3DCAPS9;

typedef struct _D3DPRESENT_PARAMETERS_ {
    uint32_t BackBufferWidth;
    uint32_t BackBufferHeight;
    uint32_t BackBufferFormat;
    uint32_t BackBufferCount;
    uint32_t MultiSampleType;
    uint32_t MultiSampleQuality;
    uint32_t SwapEffect;
    HWND__* hDeviceWindow;
    int32_t Windowed;
    int32_t EnableAutoDepthStencil;
    uint32_t AutoDepthStencilFormat;
    uint32_t Flags;
    uint32_t FullScreen_RefreshRateInHz;
    uint32_t PresentationInterval;
} D3DPRESENT_PARAMETERS;

enum _D3DFORMAT : uint32_t;

typedef struct _D3DDISPLAYMODE {
    uint32_t Width;
    uint32_t Height;
    uint32_t RefreshRate;
    _D3DFORMAT Format;
} D3DDISPLAYMODE;

typedef struct D3DDISPLAYMODEEX {
    uint32_t Size;
    uint32_t Width;
    uint32_t Height;
    uint32_t RefreshRate;
    uint32_t Format;
    uint32_t ScanLineOrdering;
} D3DDISPLAYMODEEX;

typedef struct D3DDISPLAYMODEFILTER {
    uint32_t Size;
    uint32_t Format;
    uint32_t ScanLineOrdering;
} D3DDISPLAYMODEFILTER;

enum _D3DFORMAT : uint32_t;

typedef struct _D3DSURFACE_DESC {
    _D3DFORMAT Format;
    uint32_t Type;
    uint32_t Usage;
    uint32_t Pool;
    uint32_t MultiSampleType;
    uint32_t MultiSampleQuality;
    uint32_t Width;
    uint32_t Height;
} D3DSURFACE_DESC;

typedef struct _D3DVOLUME_DESC {
    _D3DFORMAT Format;
    uint32_t Type;
    uint32_t Usage;
    uint32_t Pool;
    uint32_t Width;
    uint32_t Height;
    uint32_t Depth;
} D3DVOLUME_DESC;

typedef struct _D3DINDEXBUFFER_DESC {
    uint32_t Format;
    uint32_t Type;
    uint32_t Usage;
    uint32_t Pool;
    uint32_t Size;
} D3DINDEXBUFFER_DESC;

// The faces of a cube texture, in the SDK's order.
enum D3DCUBEMAP_FACES : uint32_t {
    D3DCUBEMAP_FACE_POSITIVE_X = 0,
    D3DCUBEMAP_FACE_NEGATIVE_X = 1,
    D3DCUBEMAP_FACE_POSITIVE_Y = 2,
    D3DCUBEMAP_FACE_NEGATIVE_Y = 3,
    D3DCUBEMAP_FACE_POSITIVE_Z = 4,
    D3DCUBEMAP_FACE_NEGATIVE_Z = 5,
};

typedef struct _D3DLOCKED_RECT {
    int32_t Pitch;
    void* pBits;
} D3DLOCKED_RECT;

typedef struct _D3DLOCKED_BOX {
    int32_t RowPitch;
    int32_t SlicePitch;
    void* pBits;
} D3DLOCKED_BOX;

typedef struct _D3DBOX {
    uint32_t Left;
    uint32_t Top;
    uint32_t Right;
    uint32_t Bottom;
    uint32_t Front;
    uint32_t Back;
} D3DBOX;

typedef struct _D3DRECT {
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;
} D3DRECT;

typedef struct _D3DVERTEXELEMENT9 {
    uint16_t Stream;
    uint16_t Offset;
    uint8_t Type;
    uint8_t Method;
    uint8_t Usage;
    uint8_t UsageIndex;
} D3DVERTEXELEMENT9;

// D3D9's light types, the values of D3DLIGHT9::Type.
enum D3DLIGHTTYPE : uint32_t {
    D3DLIGHT_POINT = 1,
    D3DLIGHT_SPOT = 2,
    D3DLIGHT_DIRECTIONAL = 3,
};

typedef struct _D3DLIGHT9 {
    uint32_t Type;
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Ambient;
    D3DVECTOR Position;
    D3DVECTOR Direction;
    float Range;
    float Falloff;
    float Attenuation0;
    float Attenuation1;
    float Attenuation2;
    float Theta;
    float Phi;
} D3DLIGHT9;

typedef struct _D3DCLIPSTATUS9 {
    uint32_t ClipUnion;
    uint32_t ClipIntersection;
} D3DCLIPSTATUS9;

typedef struct _D3DGAMMARAMP {
    uint16_t red[256];
    uint16_t green[256];
    uint16_t blue[256];
} D3DGAMMARAMP;

typedef struct _D3DRASTER_STATUS {
    int32_t InVBlank;
    uint32_t ScanLine;
} D3DRASTER_STATUS;

typedef struct _D3DDEVICE_CREATION_PARAMETERS {
    uint32_t AdapterOrdinal;
    uint32_t DeviceType;
    HWND__* hFocusWindow;
    uint32_t BehaviorFlags;
} D3DDEVICE_CREATION_PARAMETERS;

typedef struct _D3DADAPTER_IDENTIFIER9 {
    char Driver[512];
    char Description[512];
    char DeviceName[32];
    LARGE_INTEGER DriverVersion;
    uint32_t VendorId;
    uint32_t DeviceId;
    uint32_t SubSysId;
    uint32_t Revision;
    GUID DeviceIdentifier;
    uint32_t WHQLLevel;
} D3DADAPTER_IDENTIFIER9;

#if !FNVD_64BIT
#pragma pack(pop)
#endif

// The DirectShow base classes' interface ids the movie filters compare queried ids against. IDA names (CLSID_ prefix,
// though they are interface ids); not in the PDB.
extern GUID CLSID_IAMovieSetup;
extern GUID CLSID_IBaseFilter;
extern GUID CLSID_IMediaFilter;
extern GUID CLSID_IEnumMediaTypes;
extern GUID CLSID_IEnumPins;
// The 3D algorithm a buffer description names by default (DS3DALG_DEFAULT) and the null type DirectShow compares
// media types with: the exe reads its own copy rather than writing zeros (the PDB's name; the exe's map leaves the
// address unnamed).
extern GUID GUID_NULL;
extern GUID CLSID_IUnknown;
extern GUID CLSID_IPersist;
// The texture interface id NiDX9RenderedTextureData::Create queries its base texture for.
extern GUID CLSID_IDirect3DTexture9;

// D3DX value types (d3dx9math.h). D3DXMATRIX and D3DXVECTOR3 add only operators to D3DMATRIX and D3DVECTOR.
typedef D3DMATRIX D3DXMATRIX;
typedef D3DVECTOR D3DXVECTOR3;

typedef struct D3DXVECTOR4 {
    float x;
    float y;
    float z;
    float w;
} D3DXVECTOR4;

typedef struct D3DXPLANE {
    float a;
    float b;
    float c;
    float d;
    D3DXPLANE();
    D3DXPLANE(float fa, float fb, float fc, float fd);
} D3DXPLANE;

// D3DX shader and texture types (d3dx9shader.h, d3dx9tex.h). D3DXHANDLE is the SDK's LPCSTR.
typedef const char* D3DXHANDLE;

typedef struct _D3DXMACRO {
    const char* Name;
    const char* Definition;
} D3DXMACRO;

typedef struct _D3DXCONSTANTTABLE_DESC {
    const char* Creator;
    uint32_t Version;
    uint32_t Constants;
} D3DXCONSTANTTABLE_DESC;

typedef struct _D3DXCONSTANT_DESC {
    const char* Name;
    uint32_t RegisterSet;
    uint32_t RegisterIndex;
    uint32_t RegisterCount;
    uint32_t Class;
    uint32_t Type;
    uint32_t Rows;
    uint32_t Columns;
    uint32_t Elements;
    uint32_t StructMembers;
    uint32_t Bytes;
    const void* DefaultValue;
} D3DXCONSTANT_DESC;

typedef struct _D3DXIMAGE_INFO {
    uint32_t Width;
    uint32_t Height;
    uint32_t Depth;
    uint32_t MipLevels;
    uint32_t Format;
    uint32_t ResourceType;
    uint32_t ImageFileFormat;
} D3DXIMAGE_INFO;

// D3DX's image file formats (d3dx9tex.h), the values of D3DXIMAGE_INFO::ImageFileFormat.
typedef enum _D3DXIMAGE_FILEFORMAT : uint32_t {
    D3DXIFF_BMP = 0,
    D3DXIFF_JPG = 1,
    D3DXIFF_TGA = 2,
    D3DXIFF_PNG = 3,
    D3DXIFF_DDS = 4,
    D3DXIFF_PPM = 5,
    D3DXIFF_DIB = 6,
    D3DXIFF_HDR = 7,
    D3DXIFF_PFM = 8,
} D3DXIMAGE_FILEFORMAT;

// A DDS file's pixel format and header (the DirectX SDK's dds.h): the header follows the file's 4-byte magic "DDS ".
// NiDX9SourceTextureData::LoadTextureFile reads the flags at file offset 0x50, the FourCC at 0x54, the
// mip count at 0x1C and rewrites the height (0xC), width (0x10) and linear size (0x14).
enum : uint32_t {
    DDPF_FOURCC = 0x4,
};

struct DDS_PIXELFORMAT {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};

struct DDS_HEADER {
    uint32_t dwSize;
    uint32_t dwHeaderFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t dwSurfaceFlags;
    uint32_t dwCubemapFlags;
    uint32_t dwReserved2[3];
};

// SDK constants. Enumerators convert to the uint32_t parameters.

enum D3DRENDERSTATETYPE : uint32_t {
    D3DRS_ZENABLE = 0x7,
    D3DRS_FILLMODE = 0x8,
    D3DRS_SHADEMODE = 0x9,
    D3DRS_ZWRITEENABLE = 0xE,
    D3DRS_ALPHATESTENABLE = 0xF,
    D3DRS_LASTPIXEL = 0x10,
    D3DRS_SRCBLEND = 0x13,
    D3DRS_DESTBLEND = 0x14,
    D3DRS_CULLMODE = 0x16,
    D3DRS_ZFUNC = 0x17,
    D3DRS_ALPHAREF = 0x18,
    D3DRS_ALPHAFUNC = 0x19,
    D3DRS_DITHERENABLE = 0x1A,
    D3DRS_ALPHABLENDENABLE = 0x1B,
    D3DRS_FOGENABLE = 0x1C,
    D3DRS_SPECULARENABLE = 0x1D,
    D3DRS_FOGCOLOR = 0x22,
    D3DRS_FOGTABLEMODE = 0x23,
    D3DRS_FOGSTART = 0x24,
    D3DRS_FOGEND = 0x25,
    D3DRS_FOGDENSITY = 0x26,
    D3DRS_RANGEFOGENABLE = 0x30,
    D3DRS_STENCILENABLE = 0x34,
    D3DRS_STENCILFAIL = 0x35,
    D3DRS_STENCILZFAIL = 0x36,
    D3DRS_STENCILPASS = 0x37,
    D3DRS_STENCILFUNC = 0x38,
    D3DRS_STENCILREF = 0x39,
    D3DRS_STENCILMASK = 0x3A,
    D3DRS_STENCILWRITEMASK = 0x3B,
    D3DRS_TEXTUREFACTOR = 0x3C,
    D3DRS_WRAP0 = 0x80,
    D3DRS_WRAP1 = 0x81,
    D3DRS_WRAP2 = 0x82,
    D3DRS_WRAP3 = 0x83,
    D3DRS_WRAP4 = 0x84,
    D3DRS_WRAP5 = 0x85,
    D3DRS_WRAP6 = 0x86,
    D3DRS_WRAP7 = 0x87,
    D3DRS_CLIPPING = 0x88,
    D3DRS_LIGHTING = 0x89,
    D3DRS_AMBIENT = 0x8B,
    D3DRS_FOGVERTEXMODE = 0x8C,
    D3DRS_COLORVERTEX = 0x8D,
    D3DRS_LOCALVIEWER = 0x8E,
    D3DRS_NORMALIZENORMALS = 0x8F,
    D3DRS_DIFFUSEMATERIALSOURCE = 0x91,
    D3DRS_SPECULARMATERIALSOURCE = 0x92,
    D3DRS_AMBIENTMATERIALSOURCE = 0x93,
    D3DRS_EMISSIVEMATERIALSOURCE = 0x94,
    D3DRS_VERTEXBLEND = 0x97,
    D3DRS_CLIPPLANEENABLE = 0x98,
    D3DRS_POINTSIZE = 0x9A,
    D3DRS_POINTSIZE_MIN = 0x9B,
    D3DRS_POINTSPRITEENABLE = 0x9C,
    D3DRS_POINTSCALEENABLE = 0x9D,
    D3DRS_POINTSCALE_A = 0x9E,
    D3DRS_POINTSCALE_B = 0x9F,
    D3DRS_POINTSCALE_C = 0xA0,
    D3DRS_MULTISAMPLEANTIALIAS = 0xA1,
    D3DRS_MULTISAMPLEMASK = 0xA2,
    D3DRS_PATCHEDGESTYLE = 0xA3,
    D3DRS_DEBUGMONITORTOKEN = 0xA5,
    D3DRS_POINTSIZE_MAX = 0xA6,
    D3DRS_INDEXEDVERTEXBLENDENABLE = 0xA7,
    D3DRS_COLORWRITEENABLE = 0xA8,
    D3DRS_TWEENFACTOR = 0xAA,
    D3DRS_BLENDOP = 0xAB,
    D3DRS_POSITIONDEGREE = 0xAC,
    D3DRS_NORMALDEGREE = 0xAD,
    D3DRS_SCISSORTESTENABLE = 0xAE,
    D3DRS_SLOPESCALEDEPTHBIAS = 0xAF,
    D3DRS_ANTIALIASEDLINEENABLE = 0xB0,
    D3DRS_MINTESSELLATIONLEVEL = 0xB2,
    D3DRS_MAXTESSELLATIONLEVEL = 0xB3,
    D3DRS_ADAPTIVETESS_X = 0xB4,
    D3DRS_ADAPTIVETESS_Y = 0xB5,
    D3DRS_ADAPTIVETESS_Z = 0xB6,
    D3DRS_ADAPTIVETESS_W = 0xB7,
    D3DRS_ENABLEADAPTIVETESSELLATION = 0xB8,
    D3DRS_TWOSIDEDSTENCILMODE = 0xB9,
    D3DRS_CCW_STENCILFAIL = 0xBA,
    D3DRS_CCW_STENCILZFAIL = 0xBB,
    D3DRS_CCW_STENCILPASS = 0xBC,
    D3DRS_CCW_STENCILFUNC = 0xBD,
    D3DRS_COLORWRITEENABLE1 = 0xBE,
    D3DRS_COLORWRITEENABLE2 = 0xBF,
    D3DRS_COLORWRITEENABLE3 = 0xC0,
    D3DRS_BLENDFACTOR = 0xC1,
    D3DRS_SRGBWRITEENABLE = 0xC2,
    D3DRS_DEPTHBIAS = 0xC3,
    D3DRS_WRAP8 = 0xC6,
    D3DRS_WRAP9 = 0xC7,
    D3DRS_WRAP10 = 0xC8,
    D3DRS_WRAP11 = 0xC9,
    D3DRS_WRAP12 = 0xCA,
    D3DRS_WRAP13 = 0xCB,
    D3DRS_WRAP14 = 0xCC,
    D3DRS_WRAP15 = 0xCD,
    D3DRS_SEPARATEALPHABLENDENABLE = 0xCE,
    D3DRS_SRCBLENDALPHA = 0xCF,
    D3DRS_DESTBLENDALPHA = 0xD0,
    D3DRS_BLENDOPALPHA = 0xD1,
    // Not in the SDK; name chosen here. Gamebryo's (D3DRENDERSTATETYPE)0xFFFFFFFF, the end marker of
    // NiDX9RenderState::ms_akInitialRenderStateSettings that NiDX9RenderState::Create tests for.
    D3DRS_NI_END_OF_LIST = 0xFFFFFFFF,
};

enum D3DTEXTURESTAGESTATETYPE : uint32_t {
    D3DTSS_COLOROP = 0x1,
    D3DTSS_COLORARG1 = 0x2,
    D3DTSS_COLORARG2 = 0x3,
    D3DTSS_ALPHAOP = 0x4,
    D3DTSS_ALPHAARG1 = 0x5,
    D3DTSS_ALPHAARG2 = 0x6,
    D3DTSS_BUMPENVMAT00 = 0x7,
    D3DTSS_BUMPENVMAT01 = 0x8,
    D3DTSS_BUMPENVMAT10 = 0x9,
    D3DTSS_BUMPENVMAT11 = 0xA,
    D3DTSS_TEXCOORDINDEX = 0xB,
    D3DTSS_BUMPENVLSCALE = 0x16,
    D3DTSS_BUMPENVLOFFSET = 0x17,
    D3DTSS_TEXTURETRANSFORMFLAGS = 0x18,
    D3DTSS_COLORARG0 = 0x1A,
    D3DTSS_ALPHAARG0 = 0x1B,
    D3DTSS_RESULTARG = 0x1C,
    D3DTSS_CONSTANT = 0x20,
};

enum D3DSAMPLERSTATETYPE : uint32_t {
    D3DSAMP_ADDRESSU = 0x1,
    D3DSAMP_ADDRESSV = 0x2,
    D3DSAMP_ADDRESSW = 0x3,
    D3DSAMP_BORDERCOLOR = 0x4,
    D3DSAMP_MAGFILTER = 0x5,
    D3DSAMP_MINFILTER = 0x6,
    D3DSAMP_MIPFILTER = 0x7,
    D3DSAMP_MIPMAPLODBIAS = 0x8,
    D3DSAMP_MAXMIPLEVEL = 0x9,
    D3DSAMP_MAXANISOTROPY = 0xA,
    D3DSAMP_SRGBTEXTURE = 0xB,
    D3DSAMP_ELEMENTINDEX = 0xC,
    D3DSAMP_DMAPOFFSET = 0xD,
};

enum D3DBLEND : uint32_t {
    D3DBLEND_ZERO = 0x1,
    D3DBLEND_ONE = 0x2,
    D3DBLEND_SRCCOLOR = 0x3,
    D3DBLEND_INVSRCCOLOR = 0x4,
    D3DBLEND_SRCALPHA = 0x5,
    D3DBLEND_INVSRCALPHA = 0x6,
    D3DBLEND_DESTALPHA = 0x7,
    D3DBLEND_INVDESTALPHA = 0x8,
    D3DBLEND_DESTCOLOR = 0x9,
    D3DBLEND_INVDESTCOLOR = 0xA,
    D3DBLEND_SRCALPHASAT = 0xB,
    D3DBLEND_BOTHSRCALPHA = 0xC,
    D3DBLEND_BOTHINVSRCALPHA = 0xD,
    D3DBLEND_BLENDFACTOR = 0xE,
    D3DBLEND_INVBLENDFACTOR = 0xF,
};

enum D3DBLENDOP : uint32_t {
    D3DBLENDOP_ADD = 0x1,
    D3DBLENDOP_SUBTRACT = 0x2,
    D3DBLENDOP_REVSUBTRACT = 0x3,
    D3DBLENDOP_MIN = 0x4,
    D3DBLENDOP_MAX = 0x5,
};

enum D3DTEXTUREOP : uint32_t {
    D3DTOP_DISABLE = 0x1,
    D3DTOP_SELECTARG1 = 0x2,
    D3DTOP_SELECTARG2 = 0x3,
    D3DTOP_MODULATE = 0x4,
};

// Texture argument selectors (d3d9types.h D3DTA_*), as a uint32_t-valued enum.
enum : uint32_t {
    D3DTA_DIFFUSE = 0x0,
    D3DTA_CURRENT = 0x1,
    D3DTA_TEXTURE = 0x2,
    D3DTA_TFACTOR = 0x3,
};

enum D3DTEXTURETRANSFORMFLAGS : uint32_t {
    D3DTTFF_DISABLE = 0x0,
    D3DTTFF_COUNT2 = 0x2,
    D3DTTFF_COUNT3 = 0x3,
    D3DTTFF_PROJECTED = 0x100,
};

// Texture coordinate generation for D3DTSS_TEXCOORDINDEX (d3d9types.h D3DTSS_TCI_*).
enum : uint32_t {
    D3DTSS_TCI_CAMERASPACENORMAL = 0x10000,
    D3DTSS_TCI_CAMERASPACEPOSITION = 0x20000,
    D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR = 0x30000,
};

// The SDK's tag and typedef: the exe's mangled names and RTTI spell the tag (`NiTMapBase<...,_D3DFORMAT,...>`), so
// src's class-template instances must too.
enum _D3DFORMAT : uint32_t {
    D3DFMT_UNKNOWN = 0x0,
    D3DFMT_R8G8B8 = 0x14,
    D3DFMT_A8R8G8B8 = 0x15,
    D3DFMT_X8R8G8B8 = 0x16,
    D3DFMT_R5G6B5 = 0x17,
    D3DFMT_X1R5G5B5 = 0x18,
    D3DFMT_A1R5G5B5 = 0x19,
    D3DFMT_A4R4G4B4 = 0x1A,
    D3DFMT_R3G3B2 = 0x1B,
    D3DFMT_A8 = 0x1C,
    D3DFMT_A8R3G3B2 = 0x1D,
    D3DFMT_X4R4G4B4 = 0x1E,
    D3DFMT_A2B10G10R10 = 0x1F,
    D3DFMT_A8B8G8R8 = 0x20,
    D3DFMT_X8B8G8R8 = 0x21,
    D3DFMT_G16R16 = 0x22,
    D3DFMT_A2R10G10B10 = 0x23,
    D3DFMT_A16B16G16R16 = 0x24,
    D3DFMT_A8P8 = 0x28,
    D3DFMT_P8 = 0x29,
    D3DFMT_L8 = 0x32,
    D3DFMT_A8L8 = 0x33,
    D3DFMT_A4L4 = 0x34,
    D3DFMT_V8U8 = 0x3C,
    D3DFMT_L6V5U5 = 0x3D,
    D3DFMT_X8L8V8U8 = 0x3E,
    D3DFMT_Q8W8V8U8 = 0x3F,
    D3DFMT_V16U16 = 0x40,
    D3DFMT_A2W10V10U10 = 0x43,
    D3DFMT_UYVY = 0x59565955,
    D3DFMT_YUY2 = 0x32595559,
    D3DFMT_DXT1 = 0x31545844,
    D3DFMT_DXT2 = 0x32545844,
    D3DFMT_DXT3 = 0x33545844,
    D3DFMT_DXT4 = 0x34545844,
    D3DFMT_DXT5 = 0x35545844,
    D3DFMT_MULTI2_ARGB8 = 0x3154454D,
    D3DFMT_G8R8_G8B8 = 0x42475247,
    D3DFMT_R8G8_B8G8 = 0x47424752,
    D3DFMT_D16_LOCKABLE = 0x46,
    D3DFMT_D32 = 0x47,
    D3DFMT_D15S1 = 0x49,
    D3DFMT_D24S8 = 0x4B,
    D3DFMT_D24X8 = 0x4D,
    D3DFMT_D24X4S4 = 0x4F,
    D3DFMT_D16 = 0x50,
    D3DFMT_L16 = 0x51,
    D3DFMT_D32F_LOCKABLE = 0x52,
    D3DFMT_D24FS8 = 0x53,
    D3DFMT_D32_LOCKABLE = 0x54,
    D3DFMT_S8_LOCKABLE = 0x55,
    D3DFMT_VERTEXDATA = 0x64,
    D3DFMT_INDEX16 = 0x65,
    D3DFMT_INDEX32 = 0x66,
    D3DFMT_Q16W16V16U16 = 0x6E,
    D3DFMT_R16F = 0x6F,
    D3DFMT_G16R16F = 0x70,
    D3DFMT_A16B16G16R16F = 0x71,
    D3DFMT_R32F = 0x72,
    D3DFMT_G32R32F = 0x73,
    D3DFMT_A32B32G32R32F = 0x74,
    D3DFMT_CxV8U8 = 0x75,
    D3DFMT_A1 = 0x76,
    D3DFMT_A2B10G10R10_XR_BIAS = 0x77,
    D3DFMT_BINARYBUFFER = 0xC7,
};
typedef _D3DFORMAT D3DFORMAT;

enum D3DPOOL : uint32_t {
    D3DPOOL_DEFAULT = 0x0,
    D3DPOOL_MANAGED = 0x1,
    D3DPOOL_SYSTEMMEM = 0x2,
    D3DPOOL_SCRATCH = 0x3,
};

enum D3DPRIMITIVETYPE : uint32_t {
    D3DPT_POINTLIST = 0x1,
    D3DPT_LINELIST = 0x2,
    D3DPT_LINESTRIP = 0x3,
    D3DPT_TRIANGLELIST = 0x4,
    D3DPT_TRIANGLESTRIP = 0x5,
    D3DPT_TRIANGLEFAN = 0x6,
};

// Flexible vertex format bits (d3d9types.h D3DFVF_*), as a uint32_t-valued enum.
enum : uint32_t {
    D3DFVF_XYZ = 0x2,
    D3DFVF_DIFFUSE = 0x40,
    D3DFVF_TEX1 = 0x100,
};

// More D3DFVF_* bits (d3d9types.h).
enum : uint32_t {
    D3DFVF_XYZRHW = 0x4,
    D3DFVF_XYZB3 = 0xA,
    D3DFVF_NORMAL = 0x10,
    D3DFVF_TEXCOUNT_SHIFT = 8,
};

// D3DSHADEMODE (d3d9types.h).
enum : uint32_t {
    D3DSHADE_FLAT = 0x1,
    D3DSHADE_GOURAUD = 0x2,
};

enum D3DDECLTYPE : uint32_t {
    D3DDECLTYPE_FLOAT1 = 0x0,
    D3DDECLTYPE_FLOAT2 = 0x1,
    D3DDECLTYPE_FLOAT3 = 0x2,
    D3DDECLTYPE_FLOAT4 = 0x3,
    D3DDECLTYPE_D3DCOLOR = 0x4,
    D3DDECLTYPE_UBYTE4 = 0x5,
    D3DDECLTYPE_SHORT2 = 0x6,
    D3DDECLTYPE_SHORT4 = 0x7,
    D3DDECLTYPE_UBYTE4N = 0x8,
    D3DDECLTYPE_SHORT2N = 0x9,
    D3DDECLTYPE_SHORT4N = 0xA,
    D3DDECLTYPE_USHORT2N = 0xB,
    D3DDECLTYPE_USHORT4N = 0xC,
    D3DDECLTYPE_UDEC3 = 0xD,
    D3DDECLTYPE_DEC3N = 0xE,
    D3DDECLTYPE_FLOAT16_2 = 0xF,
    D3DDECLTYPE_FLOAT16_4 = 0x10,
    D3DDECLTYPE_UNUSED = 0x11,
};

enum D3DDECLUSAGE : uint32_t {
    D3DDECLUSAGE_POSITION = 0x0,
    D3DDECLUSAGE_BLENDWEIGHT = 0x1,
    D3DDECLUSAGE_BLENDINDICES = 0x2,
    D3DDECLUSAGE_NORMAL = 0x3,
    D3DDECLUSAGE_PSIZE = 0x4,
    D3DDECLUSAGE_TEXCOORD = 0x5,
    D3DDECLUSAGE_TANGENT = 0x6,
    D3DDECLUSAGE_BINORMAL = 0x7,
    D3DDECLUSAGE_TESSFACTOR = 0x8,
    D3DDECLUSAGE_POSITIONT = 0x9,
    D3DDECLUSAGE_COLOR = 0xA,
    D3DDECLUSAGE_FOG = 0xB,
    D3DDECLUSAGE_DEPTH = 0xC,
    D3DDECLUSAGE_SAMPLE = 0xD,
};

enum D3DDECLMETHOD : uint32_t {
    D3DDECLMETHOD_DEFAULT = 0x0,
    D3DDECLMETHOD_PARTIALU = 0x1,
    D3DDECLMETHOD_PARTIALV = 0x2,
    D3DDECLMETHOD_CROSSUV = 0x3,
    D3DDECLMETHOD_UV = 0x4,
    D3DDECLMETHOD_LOOKUP = 0x5,
    D3DDECLMETHOD_LOOKUPPRESAMPLED = 0x6,
};

enum D3DMULTISAMPLE_TYPE : uint32_t {
    D3DMULTISAMPLE_NONE = 0x0,
    D3DMULTISAMPLE_NONMASKABLE = 0x1,
    D3DMULTISAMPLE_2_SAMPLES = 0x2,
    D3DMULTISAMPLE_3_SAMPLES = 0x3,
    D3DMULTISAMPLE_4_SAMPLES = 0x4,
    D3DMULTISAMPLE_5_SAMPLES = 0x5,
    D3DMULTISAMPLE_6_SAMPLES = 0x6,
    D3DMULTISAMPLE_7_SAMPLES = 0x7,
    D3DMULTISAMPLE_8_SAMPLES = 0x8,
    D3DMULTISAMPLE_9_SAMPLES = 0x9,
    D3DMULTISAMPLE_10_SAMPLES = 0xA,
    D3DMULTISAMPLE_11_SAMPLES = 0xB,
    D3DMULTISAMPLE_12_SAMPLES = 0xC,
    D3DMULTISAMPLE_13_SAMPLES = 0xD,
    D3DMULTISAMPLE_14_SAMPLES = 0xE,
    D3DMULTISAMPLE_15_SAMPLES = 0xF,
    D3DMULTISAMPLE_16_SAMPLES = 0x10,
};

enum D3DRESOURCETYPE : uint32_t {
    D3DRTYPE_SURFACE = 0x1,
    D3DRTYPE_VOLUME = 0x2,
    D3DRTYPE_TEXTURE = 0x3,
    D3DRTYPE_VOLUMETEXTURE = 0x4,
    D3DRTYPE_CUBETEXTURE = 0x5,
    D3DRTYPE_VERTEXBUFFER = 0x6,
    D3DRTYPE_INDEXBUFFER = 0x7,
};

enum D3DDEVTYPE : uint32_t {
    D3DDEVTYPE_HAL = 0x1,
    D3DDEVTYPE_REF = 0x2,
    D3DDEVTYPE_SW = 0x3,
    D3DDEVTYPE_NULLREF = 0x4,
};

enum D3DTEXTUREFILTERTYPE : uint32_t {
    D3DTEXF_NONE = 0x0,
    D3DTEXF_POINT = 0x1,
    D3DTEXF_LINEAR = 0x2,
    D3DTEXF_ANISOTROPIC = 0x3,
    D3DTEXF_PYRAMIDALQUAD = 0x6,
    D3DTEXF_GAUSSIANQUAD = 0x7,
    D3DTEXF_CONVOLUTIONMONO = 0x8,
};

enum D3DCMPFUNC : uint32_t {
    D3DCMP_NEVER = 0x1,
    D3DCMP_LESS = 0x2,
    D3DCMP_EQUAL = 0x3,
    D3DCMP_LESSEQUAL = 0x4,
    D3DCMP_GREATER = 0x5,
    D3DCMP_NOTEQUAL = 0x6,
    D3DCMP_GREATEREQUAL = 0x7,
    D3DCMP_ALWAYS = 0x8,
};

enum D3DCULL : uint32_t {
    D3DCULL_NONE = 0x1,
    D3DCULL_CW = 0x2,
    D3DCULL_CCW = 0x3,
};

enum D3DTRANSFORMSTATETYPE : uint32_t {
    D3DTS_VIEW = 0x2,
    D3DTS_PROJECTION = 0x3,
    D3DTS_TEXTURE0 = 0x10,
    D3DTS_TEXTURE1 = 0x11,
    D3DTS_TEXTURE2 = 0x12,
    D3DTS_TEXTURE3 = 0x13,
    D3DTS_TEXTURE4 = 0x14,
    D3DTS_TEXTURE5 = 0x15,
    D3DTS_TEXTURE6 = 0x16,
    D3DTS_TEXTURE7 = 0x17,
    // D3DTS_WORLDMATRIX(0)
    D3DTS_WORLD = 0x100,
};

enum D3DSWAPEFFECT : uint32_t {
    D3DSWAPEFFECT_DISCARD = 0x1,
    D3DSWAPEFFECT_FLIP = 0x2,
    D3DSWAPEFFECT_COPY = 0x3,
    D3DSWAPEFFECT_OVERLAY = 0x4,
    D3DSWAPEFFECT_FLIPEX = 0x5,
};

enum D3DTEXTUREADDRESS : uint32_t {
    D3DTADDRESS_WRAP = 0x1,
    D3DTADDRESS_MIRROR = 0x2,
    D3DTADDRESS_CLAMP = 0x3,
    D3DTADDRESS_BORDER = 0x4,
    D3DTADDRESS_MIRRORONCE = 0x5,
};

enum D3DSCANLINEORDERING : uint32_t {
    D3DSCANLINEORDERING_UNKNOWN = 0x0,
    D3DSCANLINEORDERING_PROGRESSIVE = 0x1,
    D3DSCANLINEORDERING_INTERLACED = 0x2,
};

constexpr uint32_t D3DPRESENTFLAG_LOCKABLE_BACKBUFFER = 0x1;

// d3dx9tex.h: the default value of a D3DX filter or size argument.
constexpr uint32_t D3DX_DEFAULT = 0xFFFFFFFF;

constexpr uint32_t D3DPRESENT_INTERVAL_DEFAULT = 0x0;
constexpr uint32_t D3DPRESENT_INTERVAL_ONE = 0x1;
constexpr uint32_t D3DPRESENT_INTERVAL_IMMEDIATE = 0x80000000;
constexpr uint32_t D3DPRESENT_INTERVAL_TWO = 0x2;
constexpr uint32_t D3DPRESENT_INTERVAL_THREE = 0x4;
constexpr uint32_t D3DPRESENT_INTERVAL_FOUR = 0x8;

constexpr uint32_t D3DCAPS2_FULLSCREENGAMMA = 0x20000;
constexpr uint32_t D3DCAPS2_DYNAMICTEXTURES = 0x20000000;
constexpr uint32_t D3DDEVCAPS_HWTRANSFORMANDLIGHT = 0x10000;
constexpr uint32_t D3DDEVCAPS_NPATCHES = 0x1000000;
constexpr uint32_t D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS = 0x40000;
constexpr uint32_t D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING = 0x80000;
constexpr uint32_t D3DPTEXTURECAPS_POW2 = 0x2;
constexpr uint32_t D3DPTEXTURECAPS_NONPOW2CONDITIONAL = 0x100;
constexpr uint32_t D3DPTEXTURECAPS_PROJECTED = 0x400;
constexpr uint32_t D3DPTEXTURECAPS_MIPCUBEMAP = 0x10000;
constexpr uint32_t D3DPTFILTERCAPS_MINFANISOTROPIC = 0x400;
constexpr uint32_t D3DPTFILTERCAPS_MAGFANISOTROPIC = 0x4000000;

constexpr uint32_t D3DUSAGE_RENDERTARGET = 0x1;
constexpr uint32_t D3DUSAGE_DEPTHSTENCIL = 0x2;
constexpr uint32_t D3DUSAGE_WRITEONLY = 0x8;
constexpr uint32_t D3DUSAGE_SOFTWAREPROCESSING = 0x10;
constexpr uint32_t D3DUSAGE_DONOTCLIP = 0x20;
constexpr uint32_t D3DUSAGE_POINTS = 0x40;
constexpr uint32_t D3DUSAGE_RTPATCHES = 0x80;
constexpr uint32_t D3DUSAGE_NPATCHES = 0x100;
constexpr uint32_t D3DUSAGE_DYNAMIC = 0x200;
constexpr uint32_t D3DUSAGE_AUTOGENMIPMAP = 0x400;
constexpr uint32_t D3DUSAGE_DMAP = 0x4000;
constexpr uint32_t D3DUSAGE_RESTRICTED_CONTENT = 0x800;
constexpr uint32_t D3DUSAGE_RESTRICT_SHARED_RESOURCE_DRIVER = 0x1000;
constexpr uint32_t D3DUSAGE_RESTRICT_SHARED_RESOURCE = 0x2000;
constexpr uint32_t D3DUSAGE_NONSECURE = 0x800000;

// D3DRS_COLORWRITEENABLE channel bits (d3d9types.h).
constexpr uint32_t D3DCOLORWRITEENABLE_RED = 0x1;
constexpr uint32_t D3DCOLORWRITEENABLE_GREEN = 0x2;
constexpr uint32_t D3DCOLORWRITEENABLE_BLUE = 0x4;
constexpr uint32_t D3DCOLORWRITEENABLE_ALPHA = 0x8;
constexpr uint32_t D3DUSAGE_TEXTAPI = 0x10000000;
constexpr uint32_t D3DUSAGE_QUERY_LEGACYBUMPMAP = 0x8000;
constexpr uint32_t D3DUSAGE_QUERY_SRGBREAD = 0x10000;
constexpr uint32_t D3DUSAGE_QUERY_FILTER = 0x20000;
constexpr uint32_t D3DUSAGE_QUERY_SRGBWRITE = 0x40000;
constexpr uint32_t D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING = 0x80000;
constexpr uint32_t D3DUSAGE_QUERY_VERTEXTEXTURE = 0x100000;
constexpr uint32_t D3DUSAGE_QUERY_WRAPANDMIP = 0x200000;

constexpr uint32_t D3DLOCK_READONLY = 0x10;
constexpr uint32_t D3DLOCK_NOSYSLOCK = 0x800;
constexpr uint32_t D3DLOCK_NOOVERWRITE = 0x1000;
constexpr uint32_t D3DLOCK_DISCARD = 0x2000;
constexpr uint32_t D3DLOCK_DONOTWAIT = 0x4000;
constexpr uint32_t D3DLOCK_NO_DIRTY_UPDATE = 0x8000;

constexpr uint32_t D3DCLEAR_TARGET = 0x1;
constexpr uint32_t D3DCLEAR_ZBUFFER = 0x2;
constexpr uint32_t D3DCLEAR_STENCIL = 0x4;

constexpr uint32_t D3DCREATE_FPU_PRESERVE = 0x2;
constexpr uint32_t D3DCREATE_MULTITHREADED = 0x4;
constexpr uint32_t D3DCREATE_PUREDEVICE = 0x10;
constexpr uint32_t D3DCREATE_SOFTWARE_VERTEXPROCESSING = 0x20;
constexpr uint32_t D3DCREATE_HARDWARE_VERTEXPROCESSING = 0x40;
constexpr uint32_t D3DCREATE_MIXED_VERTEXPROCESSING = 0x80;
constexpr uint32_t D3DCREATE_DISABLE_DRIVER_MANAGEMENT = 0x100;
constexpr uint32_t D3DCREATE_ADAPTERGROUP_DEVICE = 0x200;
constexpr uint32_t D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX = 0x400;
constexpr uint32_t D3DCREATE_NOWINDOWCHANGES = 0x800;
constexpr uint32_t D3DCREATE_DISABLE_PSGP_THREADING = 0x2000;
constexpr uint32_t D3DCREATE_ENABLE_PRESENTSTATS = 0x4000;
constexpr uint32_t D3DCREATE_DISABLE_PRINTSCREEN = 0x8000;
constexpr uint32_t D3DCREATE_SCREENSAVER = 0x10000000;

constexpr HRESULT D3D_OK = 0;
constexpr HRESULT D3DOK_NOAUTOGEN = static_cast<HRESULT>(0x0876086F);
constexpr HRESULT D3DERR_WRONGTEXTUREFORMAT = static_cast<HRESULT>(0x88760818);
constexpr HRESULT D3DERR_UNSUPPORTEDCOLOROPERATION = static_cast<HRESULT>(0x88760819);
constexpr HRESULT D3DERR_UNSUPPORTEDCOLORARG = static_cast<HRESULT>(0x8876081A);
constexpr HRESULT D3DERR_UNSUPPORTEDALPHAOPERATION = static_cast<HRESULT>(0x8876081B);
constexpr HRESULT D3DERR_UNSUPPORTEDALPHAARG = static_cast<HRESULT>(0x8876081C);
constexpr HRESULT D3DERR_TOOMANYOPERATIONS = static_cast<HRESULT>(0x8876081D);
constexpr HRESULT D3DERR_CONFLICTINGTEXTUREFILTER = static_cast<HRESULT>(0x8876081E);
constexpr HRESULT D3DERR_UNSUPPORTEDFACTORVALUE = static_cast<HRESULT>(0x8876081F);
constexpr HRESULT D3DERR_CONFLICTINGRENDERSTATE = static_cast<HRESULT>(0x88760821);
constexpr HRESULT D3DERR_UNSUPPORTEDTEXTUREFILTER = static_cast<HRESULT>(0x88760822);
constexpr HRESULT D3DERR_CONFLICTINGTEXTUREPALETTE = static_cast<HRESULT>(0x88760826);
constexpr HRESULT D3DERR_DRIVERINTERNALERROR = static_cast<HRESULT>(0x88760827);
constexpr HRESULT D3DERR_NOTFOUND = static_cast<HRESULT>(0x88760866);
constexpr HRESULT D3DERR_MOREDATA = static_cast<HRESULT>(0x88760867);
constexpr HRESULT D3DERR_DEVICELOST = static_cast<HRESULT>(0x88760868);
constexpr HRESULT D3DERR_DEVICENOTRESET = static_cast<HRESULT>(0x88760869);
constexpr HRESULT D3DERR_NOTAVAILABLE = static_cast<HRESULT>(0x8876086A);
constexpr HRESULT D3DERR_OUTOFVIDEOMEMORY = static_cast<HRESULT>(0x8876017C);
constexpr HRESULT D3DERR_INVALIDDEVICE = static_cast<HRESULT>(0x8876086B);
constexpr HRESULT D3DERR_INVALIDCALL = static_cast<HRESULT>(0x8876086C);
constexpr HRESULT D3DERR_DRIVERINVALIDCALL = static_cast<HRESULT>(0x8876086D);
constexpr HRESULT D3DERR_WASSTILLDRAWING = static_cast<HRESULT>(0x8876021C);
constexpr HRESULT D3DERR_DEVICEREMOVED = static_cast<HRESULT>(0x88760870);
constexpr HRESULT D3DERR_DEVICEHUNG = static_cast<HRESULT>(0x88760874);
constexpr HRESULT D3DERR_UNSUPPORTEDOVERLAY = static_cast<HRESULT>(0x8876087B);
constexpr HRESULT D3DERR_UNSUPPORTEDOVERLAYFORMAT = static_cast<HRESULT>(0x8876087C);
constexpr HRESULT D3DERR_CANNOTPROTECTCONTENT = static_cast<HRESULT>(0x8876087D);
constexpr HRESULT D3DERR_UNSUPPORTEDCRYPTO = static_cast<HRESULT>(0x8876087E);
constexpr HRESULT D3DERR_PRESENT_STATISTICS_DISJOINT = static_cast<HRESULT>(0x88760884);

// COM interfaces. A call is `mov eax,[obj]; push args; push obj; call [eax + 4*index]`, index counting IUnknown's 3.

struct IDirect3D9 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(void* pInitializeFunction) = 0;
    virtual uint32_t STDMETHODCALLTYPE GetAdapterCount() = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetAdapterIdentifier(uint32_t Adapter, uint32_t Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) = 0;
    virtual uint32_t STDMETHODCALLTYPE GetAdapterModeCount(uint32_t Adapter, uint32_t Format) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    EnumAdapterModes(uint32_t Adapter, uint32_t Format, uint32_t Mode, D3DDISPLAYMODE* pMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(uint32_t Adapter, D3DDISPLAYMODE* pMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDeviceType(
        uint32_t Adapter,
        uint32_t DevType,
        uint32_t AdapterFormat,
        uint32_t BackBufferFormat,
        int32_t bWindowed
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDeviceFormat(
        uint32_t Adapter,
        uint32_t DeviceType,
        uint32_t AdapterFormat,
        uint32_t Usage,
        uint32_t RType,
        uint32_t CheckFormat
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(
        uint32_t Adapter,
        uint32_t DeviceType,
        uint32_t SurfaceFormat,
        int32_t Windowed,
        uint32_t MultiSampleType,
        uint32_t* pQualityLevels
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(
        uint32_t Adapter,
        uint32_t DeviceType,
        uint32_t AdapterFormat,
        uint32_t RenderTargetFormat,
        uint32_t DepthStencilFormat
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDeviceFormatConversion(
        uint32_t Adapter,
        uint32_t DeviceType,
        uint32_t SourceFormat,
        uint32_t TargetFormat
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceCaps(uint32_t Adapter, uint32_t DeviceType, D3DCAPS9* pCaps) = 0;
    virtual HMONITOR__* STDMETHODCALLTYPE GetAdapterMonitor(uint32_t Adapter) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDevice(
        uint32_t Adapter,
        uint32_t DeviceType,
        HWND__* hFocusWindow,
        uint32_t BehaviorFlags,
        D3DPRESENT_PARAMETERS* pPresentationParameters,
        IDirect3DDevice9** ppReturnedDeviceInterface
    ) = 0;
};

struct IDirect3D9Ex : IDirect3D9 {
    virtual uint32_t STDMETHODCALLTYPE GetAdapterModeCountEx(uint32_t Adapter, const D3DDISPLAYMODEFILTER* pFilter) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumAdapterModesEx(
        uint32_t Adapter,
        const D3DDISPLAYMODEFILTER* pFilter,
        uint32_t Mode,
        D3DDISPLAYMODEEX* pMode
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetAdapterDisplayModeEx(uint32_t Adapter, D3DDISPLAYMODEEX* pMode, uint32_t* pRotation) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDeviceEx(
        uint32_t Adapter,
        uint32_t DeviceType,
        HWND__* hFocusWindow,
        uint32_t BehaviorFlags,
        D3DPRESENT_PARAMETERS* pPresentationParameters,
        D3DDISPLAYMODEEX* pFullscreenDisplayMode,
        IDirect3DDevice9Ex** ppReturnedDeviceInterface
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdapterLUID(uint32_t Adapter, LUID* pLUID) = 0;
};

struct IDirect3DResource9 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetPrivateData(REFGUID refguid, const void* pData, uint32_t SizeOfData, uint32_t Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, uint32_t* pSizeOfData) = 0;
    virtual HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) = 0;
    virtual uint32_t STDMETHODCALLTYPE SetPriority(uint32_t PriorityNew) = 0;
    virtual uint32_t STDMETHODCALLTYPE GetPriority() = 0;
    virtual void STDMETHODCALLTYPE PreLoad() = 0;
    virtual uint32_t STDMETHODCALLTYPE GetType() = 0;
};

struct IDirect3DBaseTexture9 : IDirect3DResource9 {
    virtual uint32_t STDMETHODCALLTYPE SetLOD(uint32_t LODNew) = 0;
    virtual uint32_t STDMETHODCALLTYPE GetLOD() = 0;
    virtual uint32_t STDMETHODCALLTYPE GetLevelCount() = 0;
    virtual HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(uint32_t FilterType) = 0;
    virtual uint32_t STDMETHODCALLTYPE GetAutoGenFilterType() = 0;
    virtual void STDMETHODCALLTYPE GenerateMipSubLevels() = 0;
};

struct IDirect3DTexture9 : IDirect3DBaseTexture9 {
    virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(uint32_t Level, D3DSURFACE_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSurfaceLevel(uint32_t Level, IDirect3DSurface9** ppSurfaceLevel) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    LockRect(uint32_t Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, uint32_t Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockRect(uint32_t Level) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddDirtyRect(const RECT* pDirtyRect) = 0;
};

struct IDirect3DCubeTexture9 : IDirect3DBaseTexture9 {
    virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(uint32_t Level, D3DSURFACE_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetCubeMapSurface(uint32_t FaceType, uint32_t Level, IDirect3DSurface9** ppCubeMapSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    LockRect(uint32_t FaceType, uint32_t Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, uint32_t Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockRect(uint32_t FaceType, uint32_t Level) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddDirtyRect(uint32_t FaceType, const RECT* pDirtyRect) = 0;
};

struct IDirect3DVolumeTexture9 : IDirect3DBaseTexture9 {
    virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(uint32_t Level, D3DVOLUME_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVolumeLevel(uint32_t Level, IDirect3DVolume9** ppVolumeLevel) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    LockBox(uint32_t Level, D3DLOCKED_BOX* pLockedVolume, const D3DBOX* pBox, uint32_t Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockBox(uint32_t Level) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddDirtyBox(const D3DBOX* pDirtyBox) = 0;
};

struct IDirect3DSurface9 : IDirect3DResource9 {
    virtual HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT* pLockedRect, const RECT* pRect, uint32_t Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockRect() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDC(HDC__** phdc) = 0;
    virtual HRESULT STDMETHODCALLTYPE ReleaseDC(HDC__* hdc) = 0;
};

// Not a resource: the SDK derives it from IUnknown directly.
struct IDirect3DVolume9 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetPrivateData(REFGUID refguid, const void* pData, uint32_t SizeOfData, uint32_t Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, uint32_t* pSizeOfData) = 0;
    virtual HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DVOLUME_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockBox(D3DLOCKED_BOX* pLockedVolume, const D3DBOX* pBox, uint32_t Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockBox() = 0;
};

struct IDirect3DVertexBuffer9 : IDirect3DResource9 {
    virtual HRESULT STDMETHODCALLTYPE
    Lock(uint32_t OffsetToLock, uint32_t SizeToLock, void** ppbData, uint32_t Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DVERTEXBUFFER_DESC* pDesc) = 0;
};

struct IDirect3DIndexBuffer9 : IDirect3DResource9 {
    virtual HRESULT STDMETHODCALLTYPE
    Lock(uint32_t OffsetToLock, uint32_t SizeToLock, void** ppbData, uint32_t Flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DINDEXBUFFER_DESC* pDesc) = 0;
};

struct IDirect3DVertexDeclaration9 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeclaration(D3DVERTEXELEMENT9* pElement, uint32_t* pNumElements) = 0;
};

struct IDirect3DVertexShader9 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFunction(void* pData, uint32_t* pSizeOfData) = 0;
};

struct IDirect3DPixelShader9 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFunction(void* pData, uint32_t* pSizeOfData) = 0;
};

struct IDirect3DQuery9 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) = 0;
    virtual uint32_t STDMETHODCALLTYPE GetType() = 0;
    virtual uint32_t STDMETHODCALLTYPE GetDataSize() = 0;
    virtual HRESULT STDMETHODCALLTYPE Issue(uint32_t dwIssueFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetData(void* pData, uint32_t dwSize, uint32_t dwGetDataFlags) = 0;
};

struct IDirect3DStateBlock9 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE Capture() = 0;
    virtual HRESULT STDMETHODCALLTYPE Apply() = 0;
};

struct IDirect3DSwapChain9 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Present(
        const RECT* pSourceRect,
        const RECT* pDestRect,
        HWND__* hDestWindowOverride,
        const RGNDATA* pDirtyRegion,
        uint32_t dwFlags
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrontBufferData(IDirect3DSurface9* pDestSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetBackBuffer(uint32_t iBackBuffer, uint32_t Type, IDirect3DSurface9** ppBackBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRasterStatus(D3DRASTER_STATUS* pRasterStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDisplayMode(D3DDISPLAYMODE* pMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPresentParameters(D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
};

struct IDirect3DDevice9 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE TestCooperativeLevel() = 0;
    virtual uint32_t STDMETHODCALLTYPE GetAvailableTextureMem() = 0;
    virtual HRESULT STDMETHODCALLTYPE EvictManagedResources() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D9** ppD3D9) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS9* pCaps) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDisplayMode(uint32_t iSwapChain, D3DDISPLAYMODE* pMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetCursorProperties(uint32_t XHotSpot, uint32_t YHotSpot, IDirect3DSurface9* pCursorBitmap) = 0;
    virtual void STDMETHODCALLTYPE SetCursorPosition(int32_t X, int32_t Y, uint32_t Flags) = 0;
    virtual int32_t STDMETHODCALLTYPE ShowCursor(int32_t bShow) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSwapChain(uint32_t iSwapChain, IDirect3DSwapChain9** pSwapChain) = 0;
    virtual uint32_t STDMETHODCALLTYPE GetNumberOfSwapChains() = 0;
    virtual HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
    virtual HRESULT STDMETHODCALLTYPE Present(
        const RECT* pSourceRect,
        const RECT* pDestRect,
        HWND__* hDestWindowOverride,
        const RGNDATA* pDirtyRegion
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetBackBuffer(uint32_t iSwapChain, uint32_t iBackBuffer, uint32_t Type, IDirect3DSurface9** ppBackBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRasterStatus(uint32_t iSwapChain, D3DRASTER_STATUS* pRasterStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDialogBoxMode(int32_t bEnableDialogs) = 0;
    virtual void STDMETHODCALLTYPE SetGammaRamp(uint32_t iSwapChain, uint32_t Flags, const D3DGAMMARAMP* pRamp) = 0;
    virtual void STDMETHODCALLTYPE GetGammaRamp(uint32_t iSwapChain, D3DGAMMARAMP* pRamp) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateTexture(
        uint32_t Width,
        uint32_t Height,
        uint32_t Levels,
        uint32_t Usage,
        uint32_t Format,
        uint32_t Pool,
        IDirect3DTexture9** ppTexture,
        HANDLE* pSharedHandle
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVolumeTexture(
        uint32_t Width,
        uint32_t Height,
        uint32_t Depth,
        uint32_t Levels,
        uint32_t Usage,
        uint32_t Format,
        uint32_t Pool,
        IDirect3DVolumeTexture9** ppVolumeTexture,
        HANDLE* pSharedHandle
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateCubeTexture(
        uint32_t EdgeLength,
        uint32_t Levels,
        uint32_t Usage,
        uint32_t Format,
        uint32_t Pool,
        IDirect3DCubeTexture9** ppCubeTexture,
        HANDLE* pSharedHandle
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVertexBuffer(
        uint32_t Length,
        uint32_t Usage,
        uint32_t FVF,
        uint32_t Pool,
        IDirect3DVertexBuffer9** ppVertexBuffer,
        HANDLE* pSharedHandle
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateIndexBuffer(
        uint32_t Length,
        uint32_t Usage,
        uint32_t Format,
        uint32_t Pool,
        IDirect3DIndexBuffer9** ppIndexBuffer,
        HANDLE* pSharedHandle
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateRenderTarget(
        uint32_t Width,
        uint32_t Height,
        uint32_t Format,
        uint32_t MultiSample,
        uint32_t MultisampleQuality,
        int32_t Lockable,
        IDirect3DSurface9** ppSurface,
        HANDLE* pSharedHandle
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(
        uint32_t Width,
        uint32_t Height,
        uint32_t Format,
        uint32_t MultiSample,
        uint32_t MultisampleQuality,
        int32_t Discard,
        IDirect3DSurface9** ppSurface,
        HANDLE* pSharedHandle
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateSurface(
        IDirect3DSurface9* pSourceSurface,
        const RECT* pSourceRect,
        IDirect3DSurface9* pDestinationSurface,
        const POINT* pDestPoint
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrontBufferData(uint32_t iSwapChain, IDirect3DSurface9* pDestSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE StretchRect(
        IDirect3DSurface9* pSourceSurface,
        const RECT* pSourceRect,
        IDirect3DSurface9* pDestSurface,
        const RECT* pDestRect,
        uint32_t Filter
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE ColorFill(IDirect3DSurface9* pSurface, const RECT* pRect, uint32_t color) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurface(
        uint32_t Width,
        uint32_t Height,
        uint32_t Format,
        uint32_t Pool,
        IDirect3DSurface9** ppSurface,
        HANDLE* pSharedHandle
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderTarget(uint32_t RenderTargetIndex, IDirect3DSurface9* pRenderTarget) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetRenderTarget(uint32_t RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE BeginScene() = 0;
    virtual HRESULT STDMETHODCALLTYPE EndScene() = 0;
    virtual HRESULT STDMETHODCALLTYPE
    Clear(uint32_t Count, const D3DRECT* pRects, uint32_t Flags, uint32_t Color, float Z, uint32_t Stencil) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTransform(uint32_t State, const D3DMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTransform(uint32_t State, D3DMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE MultiplyTransform(uint32_t State, const D3DMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT9* pViewport) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT9* pViewport) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL9* pMaterial) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL9* pMaterial) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetLight(uint32_t Index, const D3DLIGHT9* pLight) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLight(uint32_t Index, D3DLIGHT9* pLight) = 0;
    virtual HRESULT STDMETHODCALLTYPE LightEnable(uint32_t Index, int32_t Enable) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLightEnable(uint32_t Index, int32_t* pEnable) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetClipPlane(uint32_t Index, const float* pPlane) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetClipPlane(uint32_t Index, float* pPlane) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderState(uint32_t State, uint32_t Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderState(uint32_t State, uint32_t* pValue) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateStateBlock(uint32_t Type, IDirect3DStateBlock9** ppSB) = 0;
    virtual HRESULT STDMETHODCALLTYPE BeginStateBlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE EndStateBlock(IDirect3DStateBlock9** ppSB) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetClipStatus(const D3DCLIPSTATUS9* pClipStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetClipStatus(D3DCLIPSTATUS9* pClipStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTexture(uint32_t Stage, IDirect3DBaseTexture9** ppTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTexture(uint32_t Stage, IDirect3DBaseTexture9* pTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTextureStageState(uint32_t Stage, uint32_t Type, uint32_t* pValue) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTextureStageState(uint32_t Stage, uint32_t Type, uint32_t Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSamplerState(uint32_t Sampler, uint32_t Type, uint32_t* pValue) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetSamplerState(uint32_t Sampler, uint32_t Type, uint32_t Value) = 0;
    virtual HRESULT STDMETHODCALLTYPE ValidateDevice(uint32_t* pNumPasses) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPaletteEntries(uint32_t PaletteNumber, const PALETTEENTRY* pEntries) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPaletteEntries(uint32_t PaletteNumber, PALETTEENTRY* pEntries) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(uint32_t PaletteNumber) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(uint32_t* PaletteNumber) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetScissorRect(const RECT* pRect) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetScissorRect(RECT* pRect) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetSoftwareVertexProcessing(int32_t bSoftware) = 0;
    virtual int32_t STDMETHODCALLTYPE GetSoftwareVertexProcessing() = 0;
    virtual HRESULT STDMETHODCALLTYPE SetNPatchMode(float nSegments) = 0;
    virtual float STDMETHODCALLTYPE GetNPatchMode() = 0;
    virtual HRESULT STDMETHODCALLTYPE
    DrawPrimitive(uint32_t PrimitiveType, uint32_t StartVertex, uint32_t PrimitiveCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(
        uint32_t PrimitiveType,
        int32_t BaseVertexIndex,
        uint32_t MinVertexIndex,
        uint32_t NumVertices,
        uint32_t startIndex,
        uint32_t primCount
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(
        uint32_t PrimitiveType,
        uint32_t PrimitiveCount,
        const void* pVertexStreamZeroData,
        uint32_t VertexStreamZeroStride
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(
        uint32_t PrimitiveType,
        uint32_t MinVertexIndex,
        uint32_t NumVertices,
        uint32_t PrimitiveCount,
        const void* pIndexData,
        uint32_t IndexDataFormat,
        const void* pVertexStreamZeroData,
        uint32_t VertexStreamZeroStride
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE ProcessVertices(
        uint32_t SrcStartIndex,
        uint32_t DestIndex,
        uint32_t VertexCount,
        IDirect3DVertexBuffer9* pDestBuffer,
        IDirect3DVertexDeclaration9* pVertexDecl,
        uint32_t Flags
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    CreateVertexDeclaration(const D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFVF(uint32_t FVF) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFVF(uint32_t* pFVF) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    CreateVertexShader(const uint32_t* pFunction, IDirect3DVertexShader9** ppShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexShader(IDirect3DVertexShader9* pShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShader(IDirect3DVertexShader9** ppShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetVertexShaderConstantF(uint32_t StartRegister, const float* pConstantData, uint32_t Vector4fCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetVertexShaderConstantF(uint32_t StartRegister, float* pConstantData, uint32_t Vector4fCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetVertexShaderConstantI(uint32_t StartRegister, const int32_t* pConstantData, uint32_t Vector4iCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetVertexShaderConstantI(uint32_t StartRegister, int32_t* pConstantData, uint32_t Vector4iCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetVertexShaderConstantB(uint32_t StartRegister, const int32_t* pConstantData, uint32_t BoolCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetVertexShaderConstantB(uint32_t StartRegister, int32_t* pConstantData, uint32_t BoolCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetStreamSource(
        uint32_t StreamNumber,
        IDirect3DVertexBuffer9* pStreamData,
        uint32_t OffsetInBytes,
        uint32_t Stride
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStreamSource(
        uint32_t StreamNumber,
        IDirect3DVertexBuffer9** ppStreamData,
        uint32_t* pOffsetInBytes,
        uint32_t* pStride
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetStreamSourceFreq(uint32_t StreamNumber, uint32_t Setting) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStreamSourceFreq(uint32_t StreamNumber, uint32_t* pSetting) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer9* pIndexData) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer9** ppIndexData) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    CreatePixelShader(const uint32_t* pFunction, IDirect3DPixelShader9** ppShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPixelShader(IDirect3DPixelShader9* pShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShader(IDirect3DPixelShader9** ppShader) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetPixelShaderConstantF(uint32_t StartRegister, const float* pConstantData, uint32_t Vector4fCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetPixelShaderConstantF(uint32_t StartRegister, float* pConstantData, uint32_t Vector4fCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetPixelShaderConstantI(uint32_t StartRegister, const int32_t* pConstantData, uint32_t Vector4iCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetPixelShaderConstantI(uint32_t StartRegister, int32_t* pConstantData, uint32_t Vector4iCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetPixelShaderConstantB(uint32_t StartRegister, const int32_t* pConstantData, uint32_t BoolCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetPixelShaderConstantB(uint32_t StartRegister, int32_t* pConstantData, uint32_t BoolCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    DrawRectPatch(uint32_t Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    DrawTriPatch(uint32_t Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeletePatch(uint32_t Handle) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateQuery(uint32_t Type, IDirect3DQuery9** ppQuery) = 0;
};

// The exe calls CheckDeviceState (index 128) on its device when it created it through Direct3DCreate9Ex.
struct IDirect3DDevice9Ex : IDirect3DDevice9 {
    virtual HRESULT STDMETHODCALLTYPE
    SetConvolutionMonoKernel(uint32_t width, uint32_t height, float* rows, float* columns) = 0;
    virtual HRESULT STDMETHODCALLTYPE ComposeRects(
        IDirect3DSurface9* pSrc,
        IDirect3DSurface9* pDst,
        IDirect3DVertexBuffer9* pSrcRectDescs,
        uint32_t NumRects,
        IDirect3DVertexBuffer9* pDstRectDescs,
        uint32_t Operation,
        int32_t Xoffset,
        int32_t Yoffset
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE PresentEx(
        const RECT* pSourceRect,
        const RECT* pDestRect,
        HWND__* hDestWindowOverride,
        const RGNDATA* pDirtyRegion,
        uint32_t dwFlags
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetGPUThreadPriority(int32_t* pPriority) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetGPUThreadPriority(int32_t Priority) = 0;
    virtual HRESULT STDMETHODCALLTYPE WaitForVBlank(uint32_t iSwapChain) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    CheckResourceResidency(IDirect3DResource9** pResourceArray, uint32_t NumResources) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(uint32_t MaxLatency) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(uint32_t* pMaxLatency) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDeviceState(HWND__* hDestinationWindow) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetEx(
        uint32_t Width,
        uint32_t Height,
        uint32_t Format,
        uint32_t MultiSample,
        uint32_t MultisampleQuality,
        int32_t Lockable,
        IDirect3DSurface9** ppSurface,
        HANDLE* pSharedHandle,
        uint32_t Usage
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurfaceEx(
        uint32_t Width,
        uint32_t Height,
        uint32_t Format,
        uint32_t Pool,
        IDirect3DSurface9** ppSurface,
        HANDLE* pSharedHandle,
        uint32_t Usage
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilSurfaceEx(
        uint32_t Width,
        uint32_t Height,
        uint32_t Format,
        uint32_t MultiSample,
        uint32_t MultisampleQuality,
        int32_t Discard,
        IDirect3DSurface9** ppSurface,
        HANDLE* pSharedHandle,
        uint32_t Usage
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    ResetEx(D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetDisplayModeEx(uint32_t iSwapChain, D3DDISPLAYMODEEX* pMode, uint32_t* pRotation) = 0;
};

struct ID3DXBuffer : IUnknown {
    virtual void* STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual uint32_t STDMETHODCALLTYPE GetBufferSize() = 0;
};

// d3dx9_38's constant table derives from ID3DXBuffer, not IUnknown.
struct ID3DXConstantTable : ID3DXBuffer {
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DXCONSTANTTABLE_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    GetConstantDesc(D3DXHANDLE hConstant, D3DXCONSTANT_DESC* pConstantDesc, uint32_t* pCount) = 0;
    virtual uint32_t STDMETHODCALLTYPE GetSamplerIndex(D3DXHANDLE hConstant) = 0;
    virtual D3DXHANDLE STDMETHODCALLTYPE GetConstant(D3DXHANDLE hConstant, uint32_t Index) = 0;
    virtual D3DXHANDLE STDMETHODCALLTYPE GetConstantByName(D3DXHANDLE hConstant, const char* pName) = 0;
    virtual D3DXHANDLE STDMETHODCALLTYPE GetConstantElement(D3DXHANDLE hConstant, uint32_t Index) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaults(IDirect3DDevice9* pDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetValue(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, const void* pData, uint32_t Bytes) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBool(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, int32_t b) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetBoolArray(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, const int32_t* pb, uint32_t Count) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetInt(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, int32_t n) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetIntArray(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, const int32_t* pn, uint32_t Count) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFloat(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, float f) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetFloatArray(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, const float* pf, uint32_t Count) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetVector(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, const D3DXVECTOR4* pVector) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetVectorArray(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, const D3DXVECTOR4* pVector, uint32_t Count) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetMatrix(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, const D3DXMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetMatrixArray(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, const D3DXMATRIX* pMatrix, uint32_t Count) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMatrixPointerArray(
        IDirect3DDevice9* pDevice,
        D3DXHANDLE hConstant,
        const D3DXMATRIX** ppMatrix,
        uint32_t Count
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE
    SetMatrixTranspose(IDirect3DDevice9* pDevice, D3DXHANDLE hConstant, const D3DXMATRIX* pMatrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMatrixTransposeArray(
        IDirect3DDevice9* pDevice,
        D3DXHANDLE hConstant,
        const D3DXMATRIX* pMatrix,
        uint32_t Count
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMatrixTransposePointerArray(
        IDirect3DDevice9* pDevice,
        D3DXHANDLE hConstant,
        const D3DXMATRIX** ppMatrix,
        uint32_t Count
    ) = 0;
};

// Not COM: no IUnknown. Open's IncludeType is a D3DXINCLUDE_TYPE.
struct ID3DXInclude {
    virtual HRESULT STDMETHODCALLTYPE Open(
        uint32_t IncludeType,
        const char* pFileName,
        const void* pParentData,
        const void** ppData,
        uint32_t* pBytes
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE Close(const void* pData) = 0;
};

// d3d9.dll's Direct3DCreate9 and Direct3DCreate9Ex, as GetProcAddress finds them.
using Direct3DCreate9_t = IDirect3D9*(WINAPI*)(uint32_t SDKVersion);
using Direct3DCreate9Ex_t = HRESULT(WINAPI*)(uint32_t SDKVersion, IDirect3D9Ex** ppD3D9Ex);

// The exe's d3d9.dll and d3dx9_38.dll imports. Each is FNVD_GLOBAL(<pointer to the SDK prototype>, <SDK name>, <import
// slot>): the hybrid build reads the exe's import slot, so `D3DXCompileShader(...)` compiles to `call [0xFDF408]` as
// the exe's calls do (the exe also reaches some through a `jmp [slot]` thunk, e.g. D3DXMatrixInverse 0xEE6DDA; the
// effect is the same). Every one is WINAPI (stdcall).
using D3DPERF_SetOptions_t = void(WINAPI*)(uint32_t dwOptions);
using D3DXLoadSurfaceFromSurface_t = HRESULT(WINAPI*)(
    IDirect3DSurface9* pDestSurface,
    const PALETTEENTRY* pDestPalette,
    const RECT* pDestRect,
    IDirect3DSurface9* pSrcSurface,
    const PALETTEENTRY* pSrcPalette,
    const RECT* pSrcRect,
    uint32_t Filter,
    uint32_t ColorKey
);
using D3DXSaveTextureToFileA_t = HRESULT(WINAPI*)(
    const char* pDestFile,
    uint32_t DestFormat,
    IDirect3DBaseTexture9* pSrcTexture,
    const PALETTEENTRY* pSrcPalette
);
using D3DXAssembleShader_t = HRESULT(WINAPI*)(
    const char* pSrcData,
    uint32_t SrcDataLen,
    const D3DXMACRO* pDefines,
    ID3DXInclude* pInclude,
    uint32_t Flags,
    ID3DXBuffer** ppShader,
    ID3DXBuffer** ppErrorMsgs
);
using D3DXAssembleShaderFromFileA_t = HRESULT(WINAPI*)(
    const char* pSrcFile,
    const D3DXMACRO* pDefines,
    ID3DXInclude* pInclude,
    uint32_t Flags,
    ID3DXBuffer** ppShader,
    ID3DXBuffer** ppErrorMsgs
);
using D3DXCompileShaderFromFileA_t = HRESULT(WINAPI*)(
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
using D3DXGetShaderConstantTable_t = HRESULT(WINAPI*)(const uint32_t* pFunction, ID3DXConstantTable** ppConstantTable);
using D3DXCreateBuffer_t = HRESULT(WINAPI*)(uint32_t NumBytes, ID3DXBuffer** ppBuffer);
using D3DXVec4Transform_t = D3DXVECTOR4*(WINAPI*)(D3DXVECTOR4 * pOut, const D3DXVECTOR4* pV, const D3DXMATRIX* pM);
using D3DXGetVertexShaderProfile_t = const char*(WINAPI*)(IDirect3DDevice9 * pDevice);
using D3DXGetPixelShaderProfile_t = const char*(WINAPI*)(IDirect3DDevice9 * pDevice);
using D3DXGetImageInfoFromFileInMemory_t =
    HRESULT(WINAPI*)(const void* pSrcData, uint32_t SrcDataSize, D3DXIMAGE_INFO* pSrcInfo);
using D3DXCreateTextureFromFileInMemory_t = HRESULT(WINAPI*)(
    IDirect3DDevice9* pDevice,
    const void* pSrcData,
    uint32_t SrcDataSize,
    IDirect3DTexture9** ppTexture
);
using D3DXCreateCubeTextureFromFileInMemory_t = HRESULT(WINAPI*)(
    IDirect3DDevice9* pDevice,
    const void* pSrcData,
    uint32_t SrcDataSize,
    IDirect3DCubeTexture9** ppCubeTexture
);
using D3DXCreateVolumeTextureFromFileInMemory_t = HRESULT(WINAPI*)(
    IDirect3DDevice9* pDevice,
    const void* pSrcData,
    uint32_t SrcDataSize,
    IDirect3DVolumeTexture9** ppVolumeTexture
);
using D3DXCompileShader_t = HRESULT(WINAPI*)(
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
);
using D3DXMatrixRotationYawPitchRoll_t = D3DXMATRIX*(WINAPI*)(D3DXMATRIX * pOut, float Yaw, float Pitch, float Roll);
using D3DXMatrixMultiplyTranspose_t = D3DXMATRIX*(WINAPI*)(D3DXMATRIX * pOut,
                                                           const D3DXMATRIX* pM1,
                                                           const D3DXMATRIX* pM2);
using D3DXVec3TransformCoord_t = D3DXVECTOR3*(WINAPI*)(D3DXVECTOR3 * pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM);
using D3DXPlaneTransform_t = D3DXPLANE*(WINAPI*)(D3DXPLANE * pOut, const D3DXPLANE* pP, const D3DXMATRIX* pM);
using D3DXVec3Normalize_t = D3DXVECTOR3*(WINAPI*)(D3DXVECTOR3 * pOut, const D3DXVECTOR3* pV);
using D3DXVec3TransformNormal_t = D3DXVECTOR3*(WINAPI*)(D3DXVECTOR3 * pOut,
                                                        const D3DXVECTOR3* pV,
                                                        const D3DXMATRIX* pM);
using D3DXMatrixMultiply_t = D3DXMATRIX*(WINAPI*)(D3DXMATRIX * pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2);
using D3DXMatrixInverse_t = D3DXMATRIX*(WINAPI*)(D3DXMATRIX * pOut, float* pDeterminant, const D3DXMATRIX* pM);
using D3DXMatrixTranspose_t = D3DXMATRIX*(WINAPI*)(D3DXMATRIX * pOut, const D3DXMATRIX* pM);
using D3DXPlaneNormalize_t = D3DXPLANE*(WINAPI*)(D3DXPLANE * pOut, const D3DXPLANE* pP);

FNVD_IMPORT(D3DPERF_SetOptions_t, D3DPERF_SetOptions, 0xFDF3C8);
FNVD_IMPORT(D3DXLoadSurfaceFromSurface_t, D3DXLoadSurfaceFromSurface, 0xFDF3D0);
FNVD_IMPORT(D3DXSaveTextureToFileA_t, D3DXSaveTextureToFileA, 0xFDF3D4);
FNVD_IMPORT(D3DXAssembleShader_t, D3DXAssembleShader, 0xFDF3D8);
FNVD_IMPORT(D3DXAssembleShaderFromFileA_t, D3DXAssembleShaderFromFileA, 0xFDF3DC);
FNVD_IMPORT(D3DXCompileShaderFromFileA_t, D3DXCompileShaderFromFileA, 0xFDF3E0);
FNVD_IMPORT(D3DXGetShaderConstantTable_t, D3DXGetShaderConstantTable, 0xFDF3E4);
FNVD_IMPORT(D3DXCreateBuffer_t, D3DXCreateBuffer, 0xFDF3E8);
FNVD_IMPORT(D3DXVec4Transform_t, D3DXVec4Transform, 0xFDF3EC);
FNVD_IMPORT(D3DXGetVertexShaderProfile_t, D3DXGetVertexShaderProfile, 0xFDF3F0);
FNVD_IMPORT(D3DXGetPixelShaderProfile_t, D3DXGetPixelShaderProfile, 0xFDF3F4);
FNVD_IMPORT(D3DXGetImageInfoFromFileInMemory_t, D3DXGetImageInfoFromFileInMemory, 0xFDF3F8);
FNVD_IMPORT(D3DXCreateTextureFromFileInMemory_t, D3DXCreateTextureFromFileInMemory, 0xFDF3FC);
FNVD_IMPORT(D3DXCreateCubeTextureFromFileInMemory_t, D3DXCreateCubeTextureFromFileInMemory, 0xFDF400);
FNVD_IMPORT(D3DXCreateVolumeTextureFromFileInMemory_t, D3DXCreateVolumeTextureFromFileInMemory, 0xFDF404);
FNVD_IMPORT(D3DXCompileShader_t, D3DXCompileShader, 0xFDF408);
FNVD_IMPORT(D3DXMatrixRotationYawPitchRoll_t, D3DXMatrixRotationYawPitchRoll, 0xFDF40C);
FNVD_IMPORT(D3DXMatrixMultiplyTranspose_t, D3DXMatrixMultiplyTranspose, 0xFDF410);
FNVD_IMPORT(D3DXVec3TransformCoord_t, D3DXVec3TransformCoord, 0xFDF414);
FNVD_IMPORT(D3DXPlaneTransform_t, D3DXPlaneTransform, 0xFDF418);
FNVD_IMPORT(D3DXVec3Normalize_t, D3DXVec3Normalize, 0xFDF41C);
FNVD_IMPORT(D3DXVec3TransformNormal_t, D3DXVec3TransformNormal, 0xFDF420);
FNVD_IMPORT(D3DXMatrixMultiply_t, D3DXMatrixMultiply, 0xFDF424);
FNVD_IMPORT(D3DXMatrixInverse_t, D3DXMatrixInverse, 0xFDF428);
FNVD_IMPORT(D3DXMatrixTranspose_t, D3DXMatrixTranspose, 0xFDF42C);
FNVD_IMPORT(D3DXPlaneNormalize_t, D3DXPlaneNormalize, 0xFDF430);

// IDA names; not in the PDB.
extern GUID CLSID_IAsyncReader;
extern GUID CLSID_IMemAllocatorCallbackTemp;
extern GUID CLSID_IMemAllocator;
