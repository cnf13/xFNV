#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"

NiVBChip::NiVBChip()
  : m_uiIndex(0), m_pkBlock(nullptr), m_pkVB(nullptr), m_uiOffset(0), m_uiLockFlags(0), m_uiSize(0),
    m_pkNextChip(nullptr), m_pkPrevChip(nullptr) {}
