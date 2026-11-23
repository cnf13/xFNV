#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectGroup.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiObjectGroup::NiObjectGroup(uint32_t uiBytes)
  : m_uiSize(uiBytes), m_pvBuffer(uiBytes ? _NiMalloc(uiBytes) : nullptr), m_pvFree(m_pvBuffer), m_uiRefCount(0) {}

void NiObjectGroup::FreeVertexBuffer() {
    _NiFree(m_pvBuffer);
}
