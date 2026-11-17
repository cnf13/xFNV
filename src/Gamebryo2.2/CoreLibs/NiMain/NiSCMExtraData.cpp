#include "Gamebryo2.2/CoreLibs/NiMain/NiSCMExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"

NiRTTI NiSCMExtraData::ms_RTTI("NiSCMExtraData", &NiExtraData::ms_RTTI);

// Only the vertex (0) and pixel (2) program types get entries.
NiSCMExtraData::NiSCMExtraData(const NiFixedString& kName, uint32_t uiNumVertexEntries, uint32_t uiNumPixelEntries) {
	SetName(kName);
	m_auiNumEntries[0] = uiNumVertexEntries;
	m_auiCurrentEntry[0] = 0;
	m_apkEntries[0] = uiNumVertexEntries
		? static_cast<SCMExtraDataEntry*>(_NiMalloc(sizeof(SCMExtraDataEntry) * uiNumVertexEntries))
		: nullptr;
	m_auiNumEntries[2] = uiNumPixelEntries;
	m_auiCurrentEntry[2] = 0;
	m_apkEntries[2] = uiNumPixelEntries
		? static_cast<SCMExtraDataEntry*>(_NiMalloc(sizeof(SCMExtraDataEntry) * uiNumPixelEntries))
		: nullptr;
}

NiExtraData* NiSCMExtraData::GetNextEntry(uint32_t uiConstantIndex, uint32_t uiPass, NiGPUProgram::ProgramType eType,
	bool bGlobal) {
	NiSCMExtraData::SCMExtraDataEntry* v5 = m_apkEntries[(int32_t)eType];
	uint32_t v6 = m_auiCurrentEntry[(int32_t)eType];
	uint8_t v7 = (uint8_t)(v5[v6].m_uiIndex == (uiConstantIndex | (uiPass << NiSCMExtraData::m_uiPassShift)));
	NiSCMExtraData::SCMExtraDataEntry* v8 = &v5[v6];
	if (!v7) {
		return nullptr;
	}
	NiExtraData* result = v8->m_pkExtraData;
	m_auiCurrentEntry[(int32_t)eType] = v6 + 1;
	return result;
}

void NiSCMExtraData::AddEntry(uint32_t uiConstantIndex, uint32_t uiPass, NiGPUProgram::ProgramType eType,
	NiExtraData* pkExtraData, bool bGlobal) {
	uint32_t v6 = 255;
	if (!bGlobal) {
		v6 = uiPass;
	}
	m_apkEntries[(int32_t)eType][m_auiCurrentEntry[(int32_t)eType]].m_uiIndex =
		uiConstantIndex | (v6 << NiSCMExtraData::m_uiPassShift);
	m_apkEntries[(int32_t)eType][m_auiCurrentEntry[(int32_t)eType]++].m_pkExtraData = pkExtraData;
}
