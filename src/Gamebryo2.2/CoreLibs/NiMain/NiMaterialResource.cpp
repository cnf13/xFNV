#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialResource.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "SDK/Windows/winbase.h"

NiMaterialResource::~NiMaterialResource() {}

void NiMaterialResource::SetType(const NiFixedString& kType) {
	NiGlobalStringTable::GlobalStringHandle kHandle = kType.GetHandle();
	if (m_kType.GetHandle() != kHandle) {
		// EXE-INLINED 0x43BA10
		NiGlobalStringTable::AcquireHandle(kHandle);
		NiGlobalStringTable::ReleaseHandle(m_kType.GetHandle());
		m_kType.m_kHandle = kHandle;
	}
}

void NiMaterialResource::SetSemantic(const NiFixedString& kSemantic) {
	NiGlobalStringTable::GlobalStringHandle kHandle = kSemantic.GetHandle();
	if (m_kSemantic.GetHandle() != kHandle) {
		// EXE-INLINED 0x43BA10
		NiGlobalStringTable::AcquireHandle(kHandle);
		NiGlobalStringTable::ReleaseHandle(m_kSemantic.GetHandle());
		m_kSemantic.m_kHandle = kHandle;
	}
}

void NiMaterialResource::SetLabel(const NiFixedString& kLabel) {
	NiGlobalStringTable::GlobalStringHandle kHandle = kLabel.GetHandle();
	if (m_kLabel.GetHandle() != kHandle) {
		// EXE-INLINED 0x43BA10
		NiGlobalStringTable::AcquireHandle(kHandle);
		NiGlobalStringTable::ReleaseHandle(m_kLabel.GetHandle());
		m_kLabel.m_kHandle = kHandle;
	}
}

void NiMaterialResource::SetVariable(const NiFixedString& kVariable) {
	NiGlobalStringTable::GlobalStringHandle kHandle = kVariable.GetHandle();
	if (m_kVariable.GetHandle() != kHandle) {
		// EXE-INLINED 0x43BA10
		NiGlobalStringTable::AcquireHandle(kHandle);
		NiGlobalStringTable::ReleaseHandle(m_kVariable.GetHandle());
		m_kVariable.m_kHandle = kHandle;
	}
}

void NiMaterialResource::SetDefaultValue(const NiFixedString& kValue) {
	NiGlobalStringTable::GlobalStringHandle kHandle = kValue.GetHandle();
	if (m_kDefaultValue.GetHandle() != kHandle) {
		// EXE-INLINED 0x43BA10
		NiGlobalStringTable::AcquireHandle(kHandle);
		NiGlobalStringTable::ReleaseHandle(m_kDefaultValue.GetHandle());
		m_kDefaultValue.m_kHandle = kHandle;
	}
}
