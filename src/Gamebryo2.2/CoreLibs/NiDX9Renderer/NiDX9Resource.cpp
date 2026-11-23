#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Resource.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

NiDX9Resource::~NiDX9Resource() {
    if (m_pkD3DResource) {
        m_pkD3DResource->Release();
        m_pkD3DResource = nullptr;
    }
}
