#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9IndexBufferManager.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Resource.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

// NiDX9Resource's fields, which the exe sets inline here.
NiDX9IndexBufferManager::NiDX9IBResource::NiDX9IBResource() {
	m_uiResID = 0xFFFFFFFF;
	m_eType = NiDX9Resource::TYPE_INVALID;
	m_ePool = 0;
	m_pkD3DResource = nullptr;
}
