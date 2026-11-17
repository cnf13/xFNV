#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"

NiGeometryGroup::NiGeometryGroup() : m_uiRefCount(0), m_pkD3DDevice(nullptr) {}

// Releases the device.
NiGeometryGroup::~NiGeometryGroup() {
	if (m_pkD3DDevice) {
		m_pkD3DDevice->Release();
	}
}

void NiGeometryGroup::AddObject(NiGeometryBufferData* apBuffData) {
	InterlockedIncrementUInt32(&m_uiRefCount);
	apBuffData->m_pkGeometryGroup = this;
}

void NiGeometryGroup::RemoveObject(NiGeometryBufferData* apBuffData) {
	InterlockedDecrementUInt32(&m_uiRefCount);
	uint32_t uiStreamCount = apBuffData->m_uiStreamCount;
	apBuffData->m_pkGeometryGroup = nullptr;
	for (uint32_t i = 0; i < uiStreamCount; ++i) {
		ReleaseChip(apBuffData, i);
	}
	apBuffData->RemoveIB();
}
