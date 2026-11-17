#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DHLSLPixelShader.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"

NiRTTI NiD3DHLSLPixelShader::ms_RTTI("NiD3DHLSLPixelShader", &NiD3DPixelShader::ms_RTTI);

NiD3DHLSLPixelShader::~NiD3DHLSLPixelShader() {
	_NiFree(m_pcEntryPoint);
	_NiFree(m_pcShaderTarget);
	if (m_pkConstantTable) {
		m_pkConstantTable->Release();
		m_pkConstantTable = nullptr;
	}
}

const NiRTTI* NiD3DHLSLPixelShader::GetRTTI() const {
	return &NiD3DHLSLPixelShader::ms_RTTI;
}

// The number of constants in the D3DX constant table; 0 without a table or when GetDesc fails.
// Left unregistered: an uninitialised read. The exe loads kDesc.Constants after GetDesc whatever its result (the
// sign only masks the loaded dword to 0), so when GetDesc leaves kDesc unwritten the return is stack garbage; the
// strict check's GetDesc stub (slot 5) returns success without filling kDesc, so the two sides' garbage differs
// (checkbody 92cd080271eb: exe 0x50D87938, src 0x7CC7F60A).
uint32_t NiD3DHLSLPixelShader::GetVariableCount() {
	if (!m_pkConstantTable) {
		return 0;
	}
	D3DXCONSTANTTABLE_DESC kDesc;
	if (m_pkConstantTable->GetDesc(&kDesc) < 0) {
		return 0;
	}
	return kDesc.Constants;
}

const char* NiD3DHLSLPixelShader::GetVariableName(uint32_t uiIndex) {
	ID3DXConstantTable* m_pkConstantTable;
	_D3DXCONSTANT_DESC kDesc;
	D3DXHANDLE v3 = this->m_pkConstantTable->GetConstant(0, uiIndex);
	if (v3 &&
		((m_pkConstantTable = this->m_pkConstantTable,
			 (uiIndex = 1, m_pkConstantTable->GetConstantDesc(v3, &kDesc, &uiIndex) >= 0)) &&
			uiIndex)) {
		return kDesc.Name;
	} else {
		return 0;
	}
}

// Hooks the entry up to its constant table variable on first use, then sets the constant by attribute type. The
// exe inlines the variable name's copy constructor and destructor (with the import calls), and for matrices
// NiDX9RenderState::SetPixelShaderConstantF. The warning text is the reference source's (NiRenderer::Error, the
// exe's empty 0xA29680, stands for NiD3DRenderer::Warning).
// Unregistered: the exe ignores GetConstantDesc's result and reads kDesc even when the call fails without writing
// it (an uninitialised read on that path).
bool NiD3DHLSLPixelShader::SetShaderConstant(NiShaderConstantMapEntry* pkEntry, const void* pvDataSource,
	uint32_t __formal) {
	if (!m_pkConstantTable || !m_pkD3DDevice) {
		return false;
	}
	if (!pvDataSource) {
		pvDataSource = pkEntry->m_pvDataSource;
	}
	if (!pkEntry->m_bVariableHookupValid) {
		// EXE-INLINED 0x48AF40
		// The reference count is the untyped word 8 bytes before the characters (GetRealBufferStart).
		NiGlobalStringTable::GlobalStringHandle kHandle = pkEntry->m_kVariableName.GetHandle();
		if (kHandle != NiGlobalStringTable::NULL_STRING) {
			InterlockedIncrementUInt32(reinterpret_cast<
				volatile uint32_t*>(const_cast<char*>(NiGlobalStringTable::GetRealBufferStart(kHandle))));
		}
		NiFixedString kVariableName;
		kVariableName.SetHandle(pkEntry->m_kVariableName.GetHandle());
		if (kVariableName.GetHandle() == NiGlobalStringTable::NULL_STRING) {
			kVariableName.Assign(pkEntry->m_kKey);
			pkEntry->SetVariableName(kVariableName);
		}
		D3DXHANDLE pkVariable = m_pkConstantTable->GetConstantByName(nullptr, kVariableName.GetHandle());
		kHandle = kVariableName.GetHandle();
		if (!pkVariable) {
			NiRenderer::Error("NiD3DHLSLPixelShader::SetShaderConstant - %s - variable %s not found\n", m_pszName,
				kHandle);
			// EXE-INLINED 0x4381B0
			if (kHandle != NiGlobalStringTable::NULL_STRING) {
				InterlockedDecrementUInt32(reinterpret_cast<
					volatile uint32_t*>(const_cast<char*>(NiGlobalStringTable::GetRealBufferStart(kHandle))));
			}
			return false;
		}
		D3DXCONSTANT_DESC kDesc;
		uint32_t uiCount = 1;
		m_pkConstantTable->GetConstantDesc(pkVariable, &kDesc, &uiCount);
		pkEntry->m_uiShaderRegister = kDesc.RegisterIndex;
		pkEntry->m_uiRegisterCount = kDesc.RegisterCount;
		pkEntry->m_bVariableHookupValid = true;
		// EXE-INLINED 0x4381B0
		if (kHandle != NiGlobalStringTable::NULL_STRING) {
			InterlockedDecrementUInt32(reinterpret_cast<
				volatile uint32_t*>(const_cast<char*>(NiGlobalStringTable::GetRealBufferStart(kHandle))));
		}
	}
	NiDX9RenderState* pkRenderState = static_cast<NiDX9RenderState*>(m_pkD3DRenderState);
	bool bReturn = false;
	switch (NiShaderConstantMapEntry::ms_aeAttribType[pkEntry->m_uiFlags & 0xFF]) {
	case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3:
	case NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4: {
		alignas(16) D3DXMATRIX kMat;
		D3DXMatrixTranspose(&kMat, static_cast<const D3DXMATRIX*>(pvDataSource));
		if (m_pkD3DRenderState->m_pkD3DDevice->SetPixelShaderConstantF(pkEntry->m_uiShaderRegister,
				reinterpret_cast<const float*>(&kMat), pkEntry->m_uiRegisterCount) >= 0) {
			bReturn = true;
		}
		break;
	}
	case NiShaderAttributeDesc::ATTRIB_TYPE_POINT2:
	case NiShaderAttributeDesc::ATTRIB_TYPE_POINT3:
	case NiShaderAttributeDesc::ATTRIB_TYPE_POINT4:
	case NiShaderAttributeDesc::ATTRIB_TYPE_COLOR:
	case NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT:
		if (pkRenderState->SetPixelShaderConstantF(pkEntry->m_uiShaderRegister, static_cast<const float*>(pvDataSource),
				pkEntry->m_uiRegisterCount, false)) {
			bReturn = true;
		}
		break;
	case NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT:
		if (pkRenderState->SetPixelShaderConstantI(pkEntry->m_uiShaderRegister,
				static_cast<const int32_t*>(pvDataSource), pkEntry->m_uiRegisterCount, false)) {
			bReturn = true;
		}
		break;
	case NiShaderAttributeDesc::ATTRIB_TYPE_BOOL:
		if (pkRenderState->SetPixelShaderConstantB(pkEntry->m_uiShaderRegister,
				static_cast<const int32_t*>(pvDataSource), pkEntry->m_uiRegisterCount, false)) {
			bReturn = true;
		}
		break;
	default:
		break;
	}
	return bReturn;
}

// Hooks the entry up to its constant table variable on first use, then sets each matrix of the array transposed,
// in pusReorderArray's order when there is one. The exe inlines the variable name's copy constructor and destructor
// (with the import calls) and NiDX9RenderState::SetPixelShaderConstantF. The warning goes to NiRenderer::Error, the
// exe's empty 0xA29680.
// GetConstantDesc's result is not checked: for the handle GetConstantByName returned, D3DX fills kDesc.
bool NiD3DHLSLPixelShader::SetShaderConstantArray(NiShaderConstantMapEntry* pkEntry, const void* pvDataSource,
	uint32_t uiNumEntries, uint32_t uiRegistersPerEntry, uint16_t* pusReorderArray) {
	FNVD_INLINED_LOCAL(NiFixedString, kVariableName, std::nullopt, std::nullopt);
	if (!m_pkConstantTable || !m_pkD3DDevice) {
		return false;
	}
	NiShaderAttributeDesc::AttributeType eType =
		NiShaderConstantMapEntry::ms_aeAttribType[static_cast<uint8_t>(pkEntry->m_uiFlags)];
	if (eType != NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3 && eType != NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4) {
		return false;
	}
	if (!pvDataSource) {
		pvDataSource = pkEntry->m_pvDataSource;
	}
	if (!pkEntry->m_bVariableHookupValid) {
		NiGlobalStringTable::AcquireHandle(pkEntry->m_kVariableName.GetHandle());
		const char* kHandle = pkEntry->m_kVariableName.GetHandle();
		kVariableName.m_kHandle = kHandle;
		if (kHandle == NiGlobalStringTable::NULL_STRING) {
			kVariableName.Assign(pkEntry->m_kKey);
			pkEntry->SetVariableName(kVariableName);
			kHandle = kVariableName.GetHandle();
		}
		D3DXHANDLE pkVariable = m_pkConstantTable->GetConstantByName(nullptr, kHandle);
		if (!pkVariable) {
			NiRenderer::Error("NiD3DHLSLPixelShader::SetShaderConstantArray - %s - variable %s not found\n", m_pszName,
				kHandle);
			// The exe's inlined destructor of kVariableName.
			NiGlobalStringTable::ReleaseHandle(kHandle);
			return false;
		}
		D3DXCONSTANT_DESC kDesc;
		uint32_t uiCount = 1;
		m_pkConstantTable->GetConstantDesc(pkVariable, &kDesc, &uiCount);
		pkEntry->m_uiShaderRegister = kDesc.RegisterIndex;
		pkEntry->m_uiRegisterCount = kDesc.RegisterCount;
		pkEntry->m_bVariableHookupValid = true;
		// The exe's inlined destructor of kVariableName.
		NiGlobalStringTable::ReleaseHandle(kHandle);
	}
	// Each entry is uiRegistersPerEntry four-float registers of the source.
	const float* pfData = static_cast<const float*>(pvDataSource);
	alignas(16) D3DXMATRIX kMat;
	bool bReturn = false;
	if (pusReorderArray) {
		uint32_t uiRegister = pkEntry->m_uiShaderRegister;
		for (uint32_t i = 0; i < uiNumEntries; i++) {
			bReturn = false;
			const float* pfEntry = pfData + 4 * uiRegistersPerEntry * pusReorderArray[i];
			D3DXMatrixTranspose_Thunk(&kMat, static_cast<const D3DXMATRIX*>(static_cast<const void*>(pfEntry)));
			if (m_pkD3DRenderState->m_pkD3DDevice->SetPixelShaderConstantF(uiRegister, &kMat._11, uiRegistersPerEntry) <
				0) {
				break;
			}
			uiRegister += uiRegistersPerEntry;
			bReturn = true;
		}
	} else {
		uint32_t uiRegister = pkEntry->m_uiShaderRegister;
		for (uint32_t i = 0; i < uiNumEntries; i++) {
			D3DXMatrixTranspose_Thunk(&kMat, static_cast<const D3DXMATRIX*>(static_cast<const void*>(pfData)));
			bReturn = false;
			if (m_pkD3DRenderState->m_pkD3DDevice->SetPixelShaderConstantF(uiRegister, &kMat._11, uiRegistersPerEntry) <
				0) {
				break;
			}
			pfData += 4 * uiRegistersPerEntry;
			uiRegister += uiRegistersPerEntry;
			bReturn = true;
		}
	}
	return bReturn;
}

void NiD3DHLSLPixelShader::DestroyRendererData() {
	if (m_pkConstantTable) {
		m_pkConstantTable->Release();
		m_pkConstantTable = nullptr;
	}
	NiD3DPixelShader::DestroyRendererData();
}

// The exe's body is a jump to NiD3DPixelShader's (0xE898D0).
void NiD3DHLSLPixelShader::RecreateRendererData() {
	NiD3DPixelShader::RecreateRendererData();
}

// Replaces the entry point name with a copy of pcEntryPoint; an empty name clears it. The exe inlines strlen.
void NiD3DHLSLPixelShader::SetEntryPoint(const char* pcEntryPoint) {
	if (m_pcEntryPoint == pcEntryPoint) {
		return;
	}
	_NiFree(m_pcEntryPoint);
	if (pcEntryPoint && *pcEntryPoint) {
		// EXE-INLINED 0xEC6130
		size_t uiLength = 0;
		while (pcEntryPoint[uiLength]) {
			++uiLength;
		}
		size_t uiSize = uiLength + 1;
		char* pcCopy = static_cast<char*>(_NiMalloc(uiSize));
		strcpy_s(pcCopy, uiSize, pcEntryPoint);
		m_pcEntryPoint = pcCopy;
	} else {
		m_pcEntryPoint = nullptr;
	}
}

// Replaces the shader target with a copy of pcShaderTarget. Unlike SetEntryPoint, an empty target leaves the freed
// pointer in place, as the exe does. The exe inlines strlen.
void NiD3DHLSLPixelShader::SetShaderTarget(const char* pcShaderTarget) {
	if (m_pcShaderTarget == pcShaderTarget) {
		return;
	}
	_NiFree(m_pcShaderTarget);
	if (pcShaderTarget && *pcShaderTarget) {
		// EXE-INLINED 0xEC6130
		size_t uiLength = 0;
		while (pcShaderTarget[uiLength]) {
			++uiLength;
		}
		size_t uiSize = uiLength + 1;
		char* pcCopy = static_cast<char*>(_NiMalloc(uiSize));
		strcpy_s(pcCopy, uiSize, pcShaderTarget);
		m_pcShaderTarget = pcCopy;
	}
}

// Swaps in pkTable: releases the old table and holds a reference to the new one.
void NiD3DHLSLPixelShader::SetConstantTable(ID3DXConstantTable* pkTable) {
	if (m_pkConstantTable == pkTable) {
		return;
	}
	if (m_pkConstantTable) {
		m_pkConstantTable->Release();
	}
	m_pkConstantTable = pkTable;
	if (pkTable) {
		pkTable->AddRef();
	}
}
