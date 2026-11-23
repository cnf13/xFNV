#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

// The DirectShow SDK's vfwmsgs.h codes the exe's DirectShow base classes return, spelled and valued as the SDK has
// them. VFW_E_ENUM_OUT_OF_SYNC is declared in SDK/DirectShow/CUnknown.h.
constexpr HRESULT VFW_E_NOT_CONNECTED = static_cast<HRESULT>(0x80040209);
constexpr HRESULT VFW_E_BUFFER_OVERFLOW = static_cast<HRESULT>(0x8004020D);
constexpr HRESULT VFW_E_BADALIGN = static_cast<HRESULT>(0x8004020E);
constexpr HRESULT VFW_E_ALREADY_COMMITTED = static_cast<HRESULT>(0x8004020F);
constexpr HRESULT VFW_E_BUFFERS_OUTSTANDING = static_cast<HRESULT>(0x80040210);
constexpr HRESULT VFW_E_NOT_COMMITTED = static_cast<HRESULT>(0x80040211);
constexpr HRESULT VFW_E_SIZENOTSET = static_cast<HRESULT>(0x80040212);
constexpr HRESULT VFW_E_NOT_STOPPED = static_cast<HRESULT>(0x80040224);
constexpr HRESULT VFW_E_WRONG_STATE = static_cast<HRESULT>(0x80040227);
constexpr HRESULT VFW_E_TIMEOUT = static_cast<HRESULT>(0x8004022E);
constexpr HRESULT VFW_E_SAMPLE_TIME_NOT_SET = static_cast<HRESULT>(0x80040249);
constexpr HRESULT VFW_E_MEDIA_TIME_NOT_SET = static_cast<HRESULT>(0x80040251);
constexpr HRESULT VFW_S_NO_MORE_ITEMS = static_cast<HRESULT>(0x00040103);
constexpr HRESULT VFW_S_NO_STOP_TIME = static_cast<HRESULT>(0x00040270);
