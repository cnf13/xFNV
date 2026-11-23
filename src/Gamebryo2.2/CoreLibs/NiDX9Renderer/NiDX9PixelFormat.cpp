#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"

NiPixelFormat* NiDX9PixelFormat::InitFromD3DFormat(D3DFORMAT aeD3DFmt, NiPixelFormat* apkFormat) {
    NiPixelFormat* v2;
    if (aeD3DFmt > D3DFMT_DXT1) {
        if (aeD3DFmt == D3DFMT_DXT3) {
            v2 = &NiPixelFormat::DXT3;
        }
        else {
            if (aeD3DFmt == D3DFMT_DXT5) {
                v2 = &NiPixelFormat::DXT5;
            }
            else {
                v2 = &NiPixelFormat::RENDERERSPECIFICCOMPRESSED;
            }
        }
    }
    else {
        if (aeD3DFmt == D3DFMT_DXT1) {
            v2 = &NiPixelFormat::DXT1;
        }
        else {
            switch (aeD3DFmt) {
                case D3DFMT_R8G8B8: {
                    v2 = &NiPixelFormat::BGR888;
                    break;
                }
                case D3DFMT_A8R8G8B8: {
                    v2 = &NiPixelFormat::BGRA8888;
                    break;
                }
                case D3DFMT_X8R8G8B8: {
                    v2 = &NiPixelFormat::BGRX8888;
                    break;
                }
                case D3DFMT_R5G6B5: {
                    v2 = &NiPixelFormat::BGR565;
                    break;
                }
                case D3DFMT_X1R5G5B5: {
                    v2 = &NiPixelFormat::BGRX5551;
                    break;
                }
                case D3DFMT_A1R5G5B5: {
                    v2 = &NiPixelFormat::BGRA5551;
                    break;
                }
                case D3DFMT_A4R4G4B4: {
                    v2 = &NiPixelFormat::BGRA4444;
                    break;
                }
                case D3DFMT_R3G3B2: {
                    v2 = &NiPixelFormat::BGR233;
                    break;
                }
                case D3DFMT_A8: {
                    v2 = &NiPixelFormat::A8;
                    break;
                }
                case D3DFMT_A8R3G3B2: {
                    v2 = &NiPixelFormat::BGRA2338;
                    break;
                }
                case D3DFMT_X4R4G4B4: {
                    v2 = &NiPixelFormat::BGRX4444;
                    break;
                }
                case D3DFMT_A2B10G10R10: {
                    v2 = &NiPixelFormat::RGBA1010102;
                    break;
                }
                case D3DFMT_A8B8G8R8: {
                    v2 = &NiPixelFormat::RGBA32;
                    break;
                }
                case D3DFMT_X8B8G8R8: {
                    v2 = &NiPixelFormat::RGBX8888;
                    break;
                }
                case D3DFMT_G16R16: {
                    v2 = &NiPixelFormat::GR1616;
                    break;
                }
                case D3DFMT_A2R10G10B10: {
                    v2 = &NiPixelFormat::BGRA1010102;
                    break;
                }
                case D3DFMT_A16B16G16R16:
                case D3DFMT_A16B16G16R16F: {
                    v2 = &NiPixelFormat::RGBA64;
                    break;
                }
                case D3DFMT_A8P8: {
                    v2 = &NiPixelFormat::PA88;
                    break;
                }
                case D3DFMT_P8: {
                    v2 = &NiPixelFormat::PAL8;
                    break;
                }
                case D3DFMT_L8: {
                    v2 = &NiPixelFormat::L8;
                    break;
                }
                case D3DFMT_A8L8: {
                    v2 = &NiPixelFormat::LA88;
                    break;
                }
                case D3DFMT_A4L4: {
                    v2 = &NiPixelFormat::LA44;
                    break;
                }
                case D3DFMT_V8U8: {
                    v2 = &NiPixelFormat::BUMP16;
                    break;
                }
                case D3DFMT_L6V5U5: {
                    v2 = &NiPixelFormat::BUMPLUMA556;
                    break;
                }
                case D3DFMT_X8L8V8U8: {
                    v2 = &NiPixelFormat::BUMPLUMAX8888;
                    break;
                }
                case D3DFMT_Q8W8V8U8: {
                    v2 = &NiPixelFormat::BUMPWQ8888;
                    break;
                }
                case D3DFMT_V16U16: {
                    v2 = &NiPixelFormat::BUMP1616;
                    break;
                }
                case D3DFMT_A2W10V10U10: {
                    v2 = &NiPixelFormat::BUMPW101111;
                    break;
                }
                case D3DFMT_D16_LOCKABLE:
                case D3DFMT_D16: {
                    v2 = &NiPixelFormat::DEPTH16;
                    break;
                }
                case D3DFMT_D32: {
                    v2 = &NiPixelFormat::DEPTH32;
                    break;
                }
                case D3DFMT_D15S1: {
                    v2 = &NiPixelFormat::STENCILDEPTH115;
                    break;
                }
                case D3DFMT_D24S8: {
                    v2 = &NiPixelFormat::STENCILDEPTH824;
                    break;
                }
                case D3DFMT_D24X8: {
                    v2 = &NiPixelFormat::XDEPTH824;
                    break;
                }
                case D3DFMT_D24X4S4: {
                    v2 = &NiPixelFormat::STENCILXDEPTH4424;
                    break;
                }
                case D3DFMT_L16: {
                    v2 = &NiPixelFormat::L16;
                    break;
                }
                case D3DFMT_D32F_LOCKABLE:
                case D3DFMT_D24FS8: {
                    v2 = &NiPixelFormat::RENDERERSPECIFIC32;
                    break;
                }
                case D3DFMT_Q16W16V16U16: {
                    v2 = &NiPixelFormat::RENDERERSPECIFIC64;
                    break;
                }
                case D3DFMT_R16F: {
                    v2 = &NiPixelFormat::R16;
                    break;
                }
                case D3DFMT_G16R16F: {
                    v2 = &NiPixelFormat::RG32;
                    break;
                }
                case D3DFMT_R32F: {
                    v2 = &NiPixelFormat::R32;
                    break;
                }
                case D3DFMT_G32R32F: {
                    v2 = &NiPixelFormat::RG64;
                    break;
                }
                case D3DFMT_A32B32G32R32F: {
                    v2 = &NiPixelFormat::RGBA128;
                    break;
                }
                case D3DFMT_CxV8U8: {
                    v2 = &NiPixelFormat::RENDERERSPECIFIC16;
                    break;
                }
                default: {
                    v2 = &NiPixelFormat::RENDERERSPECIFICCOMPRESSED;
                    break;
                }
            }
        }
    }
    *apkFormat = *v2;
    apkFormat->m_uiRendererHint = aeD3DFmt;
    apkFormat->m_uiExtraData = 0;
    return apkFormat;
}

// Returns the renderer hint when the format carries one (anything but 0xFFFFFFFF, Gamebryo's invalid hint), else
// the D3D format its layout, depth and masks describe, or D3DFMT_UNKNOWN.
D3DFORMAT NiDX9PixelFormat::DetermineD3DFormat(const NiPixelFormat& akFormat) {
    _D3DFORMAT result = static_cast<_D3DFORMAT>(akFormat.m_uiRendererHint);
    uint8_t m_ucBitsPerPixel = akFormat.m_ucBitsPerPixel;
    if (akFormat.m_uiRendererHint == 0xFFFFFFFFu) {
        switch (akFormat.m_eFormat) {
            case NiPixelFormat::FORMAT_RGB: {
                switch (m_ucBitsPerPixel) {
                    case 8: {
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0xE0) {
                            result = D3DFMT_R3G3B2;
                            break;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED)) {
                            result = D3DFMT_UNKNOWN;
                            break;
                        }
                        return D3DFMT_L8;
                    }
                    case 16: {
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0xF800) {
                            return D3DFMT_R5G6B5;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0x7C00) {
                            return D3DFMT_X1R5G5B5;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0xF00) {
                            return D3DFMT_X4R4G4B4;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED)) {
                            result = D3DFMT_UNKNOWN;
                            break;
                        }
                        return D3DFMT_L16;
                    }
                    case 24: {
                        return D3DFMT_R8G8B8;
                    }
                    case 32: {
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0x00FF0000u) {
                            return D3DFMT_X8R8G8B8;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0xFF) {
                            return D3DFMT_X8B8G8R8;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) != 0xFFFF0000u) {
                            result = D3DFMT_UNKNOWN;
                            break;
                        }
                        result = D3DFMT_G16R16;
                        break;
                    }
                    default: {
                        result = D3DFMT_UNKNOWN;
                        break;
                    }
                }
                break;
            }
            case NiPixelFormat::FORMAT_RGBA: {
                switch (m_ucBitsPerPixel) {
                    case 8: {
                        if (!akFormat.GetMask(NiPixelFormat::COMP_LUMA)) {
                            return D3DFMT_A8;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_LUMA) != 0xF) {
                            result = D3DFMT_UNKNOWN;
                            break;
                        }
                        return D3DFMT_A4L4;
                    }
                    case 16: {
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0x7C00) {
                            return D3DFMT_A1R5G5B5;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0xF00) {
                            return D3DFMT_A4R4G4B4;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0xE0) {
                            return D3DFMT_A8R3G3B2;
                        }
                        if (!akFormat.GetMask(NiPixelFormat::COMP_RED)) {
                            result = D3DFMT_A8L8;
                            break;
                        }
                        result = D3DFMT_UNKNOWN;
                        break;
                    }
                    case 32: {
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0x00FF0000u) {
                            result = D3DFMT_A8R8G8B8;
                            break;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0x3FF) {
                            return D3DFMT_A2B10G10R10;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) == 0xFF) {
                            return D3DFMT_A8B8G8R8;
                        }
                        if (akFormat.GetMask(NiPixelFormat::COMP_RED) != 0x3FF00000u) {
                            result = D3DFMT_UNKNOWN;
                            break;
                        }
                        result = D3DFMT_A2R10G10B10;
                        break;
                    }
                    case 64: {
                        result = D3DFMT_A16B16G16R16F;
                        break;
                    }
                    case 128: {
                        result = D3DFMT_A32B32G32R32F;
                        break;
                    }
                    default: {
                        result = D3DFMT_UNKNOWN;
                        break;
                    }
                }
                break;
            }
            case NiPixelFormat::FORMAT_PAL: {
                result = D3DFMT_P8;
                break;
            }
            case NiPixelFormat::FORMAT_PALALPHA:
            case NiPixelFormat::FORMAT_RENDERERSPECIFIC:
            case NiPixelFormat::FORMAT_THREE_CHANNEL: {
                result = D3DFMT_UNKNOWN;
                break;
            }
            case NiPixelFormat::FORMAT_DXT1: {
                result = D3DFMT_DXT1;
                break;
            }
            case NiPixelFormat::FORMAT_DXT3: {
                result = D3DFMT_DXT3;
                break;
            }
            case NiPixelFormat::FORMAT_DXT5: {
                result = D3DFMT_DXT5;
                break;
            }
            case NiPixelFormat::FORMAT_BUMP: {
                result = D3DFMT_V8U8;
                break;
            }
            case NiPixelFormat::FORMAT_BUMPLUMA: {
                if (m_ucBitsPerPixel == 16) {
                    result = D3DFMT_L6V5U5;
                }
                else {
                    if (m_ucBitsPerPixel != 32) {
                        result = D3DFMT_UNKNOWN;
                        break;
                    }
                    result = D3DFMT_X8L8V8U8;
                }
                break;
            }
            case NiPixelFormat::FORMAT_ONE_CHANNEL: {
                switch (m_ucBitsPerPixel) {
                    case 8: {
                        result = static_cast<_D3DFORMAT>(
                            akFormat.GetMask(NiPixelFormat::COMP_ALPHA) != 0 ? D3DFMT_A8 : D3DFMT_L8
                        );
                        break;
                    }
                    case 16: {
                        result = D3DFMT_R16F;
                        break;
                    }
                    case 32: {
                        result = D3DFMT_R32F;
                        break;
                    }
                    default: {
                        result = D3DFMT_UNKNOWN;
                        break;
                    }
                }
                break;
            }
            case NiPixelFormat::FORMAT_TWO_CHANNEL: {
                switch (m_ucBitsPerPixel) {
                    case 16: {
                        result = D3DFMT_A8L8;
                        break;
                    }
                    case 32: {
                        result = D3DFMT_G16R16F;
                        break;
                    }
                    case 64: {
                        result = D3DFMT_G32R32F;
                        break;
                    }
                    default: {
                        result = D3DFMT_UNKNOWN;
                        break;
                    }
                }
                break;
            }
            case NiPixelFormat::FORMAT_FOUR_CHANNEL: {
                switch (m_ucBitsPerPixel) {
                    case 32: {
                        result = D3DFMT_A8R8G8B8;
                        break;
                    }
                    case 64: {
                        result = D3DFMT_A16B16G16R16F;
                        break;
                    }
                    case 128: {
                        result = D3DFMT_A32B32G32R32F;
                        break;
                    }
                    default: {
                        result = D3DFMT_UNKNOWN;
                        break;
                    }
                }
                break;
            }
            case NiPixelFormat::FORMAT_DEPTH_STENCIL: {
                if (m_ucBitsPerPixel == 16) {
                    result = static_cast<_D3DFORMAT>(
                        static_cast<uint8_t>(akFormat.GetBits(NiPixelFormat::COMP_STENCIL) - 1) != 0 ? D3DFMT_D16
                                                                                                     : D3DFMT_D15S1
                    );
                }
                else {
                    if (m_ucBitsPerPixel != 32) {
                        result = D3DFMT_UNKNOWN;
                        break;
                    }
                    uint8_t Bits = akFormat.GetBits(NiPixelFormat::COMP_STENCIL);
                    if (Bits == 4) {
                        result = D3DFMT_D24X4S4;
                    }
                    else {
                        if (Bits == 8) {
                            result = D3DFMT_D24S8;
                        }
                        else {
                            result = static_cast<_D3DFORMAT>(
                                static_cast<uint8_t>(akFormat.GetBits(NiPixelFormat::COMP_DEPTH) - 24) != 0
                                    ? D3DFMT_D32
                                    : D3DFMT_D24X8
                            );
                        }
                    }
                }
                break;
            }
            default: {
                result = D3DFMT_UNKNOWN;
                break;
            }
        }
    }
    return result;
}

NiPixelFormat* NiDX9PixelFormat::CreateFromD3DFormat(D3DFORMAT aeD3DFmt) {
    NiPixelFormat* pkFormat = ::new NiPixelFormat;
    InitFromD3DFormat(aeD3DFmt, pkFormat);
    return pkFormat;
}
