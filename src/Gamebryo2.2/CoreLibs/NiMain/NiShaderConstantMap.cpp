#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMap.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureEffect.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cctype>

NiShaderConstantMap::PredefinedMapping::PredefinedMapping() : m_uiMappingID(0) {}

NiShaderConstantMap::PredefinedMapping::PredefinedMapping(uint32_t auiMappingID, const NiFixedString& akMappingName)
	: m_uiMappingID(auiMappingID) {
	m_kMappingName = akMappingName;
}

NiShaderConstantMap::NiShaderConstantMap(NiGPUProgram::ProgramType eType) : m_eProgramType(eType) {}

NiShaderConstantMap::~NiShaderConstantMap() {}

bool NiShaderConstantMap::LookUpObjectMapping(const char* pcMapping, uint32_t& uiMappingID) {
	NiShaderConstantMap::PredefinedMapping* v3;
	int32_t v2 = 0;
	if (!NiShaderConstantMap::ms_uiObjectMappingCount) {
		return 0;
	}
	while (1) {
		v3 = &NiShaderConstantMap::ms_pkObjectMappings[v2];
		if (!NiStricmp(v3->m_kMappingName.GetHandle(), pcMapping)) {
			break;
		}
		if (++v2 >= NiShaderConstantMap::ms_uiObjectMappingCount) {
			return 0;
		}
	}
	uiMappingID = v3->m_uiMappingID;
	return 1;
}

NiShaderAttributeDesc::AttributeType NiShaderConstantMap::LookUpObjectMappingType(uint32_t uiMappingID,
	uint32_t& uiRegCount, uint32_t& uiFloatCount) {
	int32_t result;
	uiRegCount = (uint32_t)1;
	uiFloatCount = (uint32_t)4;
	switch (uiMappingID) {
	case 1u:
	case 0x16u: {
		uiFloatCount = (uint32_t)1;
		result = 4;
		break;
	}
	case 2u:
	case 3u:
	case 4u:
	case 5u:
	case 6u:
	case 7u:
	case 8u:
	case 9u:
	case 0xAu:
	case 0xBu:
	case 0xEu:
	case 0xFu:
	case 0x12u:
	case 0x13u:
	case 0x14u:
	case 0x15u: {
		result = 7;
		break;
	}
	case 0xCu:
	case 0xDu:
	case 0x17u:
	case 0x18u: {
		uiRegCount = (uint32_t)4;
		uiFloatCount = (uint32_t)16;
		result = 9;
		break;
	}
	case 0x10u:
	case 0x11u: {
		uiRegCount = (uint32_t)3;
		uiFloatCount = (uint32_t)12;
		result = 8;
		break;
	}
	default: {
		result = 0;
		break;
	}
	}
	return static_cast<NiShaderAttributeDesc::AttributeType>(result);
}

bool NiShaderConstantMap::LookUpPredefinedMapping(const char* pszMapping, uint32_t& uiMappingID,
	uint32_t& uiNumRegisters) {
	char pszTrueName[260];
	bool result = NiShaderConstantMap::ParseKeyName(pszMapping, pszTrueName, 260u, uiNumRegisters);
	if (result) {
		int32_t v4 = 0;
		if (NiShaderConstantMap::ms_uiPredefinedMappingCount) {
			NiShaderConstantMap::PredefinedMapping* v5;
			while (1) {
				v5 = &NiShaderConstantMap::ms_pkPredefinedMappings[v4];
				if (!NiStricmp(v5->m_kMappingName.GetHandle(), pszTrueName)) {
					break;
				}
				if (++v4 >= NiShaderConstantMap::ms_uiPredefinedMappingCount) {
					return 0;
				}
			}
			uiMappingID = v5->m_uiMappingID;
			return 1;
		} else {
			return 0;
		}
	}
	return result;
}

NiShaderAttributeDesc::AttributeType NiShaderConstantMap::LookUpPredefinedMappingType(uint32_t auiMappingID,
	uint32_t auiNumRegisters) {
	int32_t result;
	switch (auiMappingID) {
	case 1u:
	case 2u:
	case 3u:
	case 4u:
	case 5u:
	case 6u:
	case 7u:
	case 8u:
	case 9u:
	case 0xAu:
	case 0xBu:
	case 0xCu:
	case 0xDu:
	case 0xEu:
	case 0xFu:
	case 0x10u:
	case 0x11u:
	case 0x12u:
	case 0x13u:
	case 0x14u:
	case 0x15u:
	case 0x16u:
	case 0x17u:
	case 0x18u:
	case 0x25u:
	case 0x26u:
	case 0x27u:
	case 0x28u:
	case 0x29u:
	case 0x2Au:
	case 0x2Bu:
	case 0x2Cu:
	case 0x2Du:
	case 0x2Eu:
	case 0x2Fu:
	case 0x30u:
	case 0x31u:
	case 0x32u:
	case 0x33u:
	case 0x34u:
	case 0x35u:
	case 0x36u:
	case 0x37u:
	case 0x38u:
	case 0x39u:
	case 0x3Au:
	case 0x3Bu:
	case 0x3Cu:
	case 0x3Du:
	case 0x3Eu:
	case 0x3Fu:
	case 0x40u:
	case 0x41u:
	case 0x42u:
	case 0x43u:
	case 0x44u:
	case 0x58u:
	case 0x59u:
	case 0x5Au:
	case 0x5Bu:
	case 0x5Cu:
	case 0x5Du:
	case 0x5Eu:
	case 0x5Fu: {
		result = ((uint8_t)(auiNumRegisters != 3)) + 8;
		break;
	}
	case 0x19u:
	case 0x1Au:
	case 0x1Bu:
	case 0x1Cu:
	case 0x1Du:
	case 0x1Eu:
	case 0x1Fu:
	case 0x20u:
	case 0x22u:
	case 0x24u:
	case 0x60u:
	case 0x61u:
	case 0x62u:
	case 0x63u: {
		result = 9;
		break;
	}
	case 0x21u:
	case 0x23u: {
		result = 8;
		break;
	}
	case 0x45u:
	case 0x46u:
	case 0x47u:
	case 0x48u:
	case 0x49u:
	case 0x4Au:
	case 0x4Bu:
	case 0x4Cu:
	case 0x4Du:
	case 0x4Eu:
	case 0x4Fu:
	case 0x50u:
	case 0x51u:
	case 0x52u:
	case 0x53u:
	case 0x54u:
	case 0x55u:
	case 0x56u:
	case 0x57u:
	case 0x64u:
	case 0x65u:
	case 0x66u:
	case 0x67u: {
		result = 7;
		break;
	}
	case 0x68u:
	case 0x6Au:
	case 0x6Bu:
	case 0x6Cu:
	case 0x6Du:
	case 0x6Eu:
	case 0x6Fu:
	case 0x70u:
	case 0x71u:
	case 0x72u:
	case 0x73u: {
		result = 5;
		break;
	}
	case 0x69u: {
		result = 4;
		break;
	}
	default: {
		result = 0;
		break;
	}
	}
	return static_cast<NiShaderAttributeDesc::AttributeType>(result);
}

// The exe also treats mapping ID 0 as no mapping.
NiShaderAttributeDesc::AttributeType NiShaderConstantMap::LookUpPredefinedMappingType(const char* apszMapping) {
	uint32_t uiMappingID;
	uint32_t uiNumRegisters;
	if (LookUpPredefinedMapping(apszMapping, uiMappingID, uiNumRegisters) && uiMappingID) {
		return LookUpPredefinedMappingType(uiMappingID, uiNumRegisters);
	}
	return NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED;
}

NiDynamicEffect* NiShaderConstantMap::GetDynamicEffectForObject(const NiDynamicEffectState* pkEffectState,
	NiShaderAttributeDesc::ObjectType eObjectType, uint32_t uiObjectIndex) {
	bool v11;
	NiDynamicEffect* v14;
	NiDynamicEffectState::ListItem* m_pProjLightList;
	int32_t v17;
	int32_t v3 = 0;
	NiDynamicEffect* v20 = 0;
	if (!pkEffectState) {
		return v20;
	}
	switch (eObjectType) {
	case NiShaderAttributeDesc::OT_EFFECT_GENERALLIGHT:
	case NiShaderAttributeDesc::OT_EFFECT_POINTLIGHT:
	case NiShaderAttributeDesc::OT_EFFECT_DIRECTIONALLIGHT:
	case NiShaderAttributeDesc::OT_EFFECT_SPOTLIGHT: {
		NiDynamicEffect* m_pItem;
		NiDynamicEffectState::ListItem* m_pLightList = pkEffectState->m_pLightList;
		if (!m_pLightList) {
			return v20;
		}
		while (1) {
			m_pItem = m_pLightList->m_pItem;
			m_pLightList = m_pLightList->m_pNext;
			uint32_t m_ucEffectType = m_pItem->m_ucEffectType;
			bool v7 = m_ucEffectType > eObjectType && eObjectType == NiShaderAttributeDesc::OT_EFFECT_GENERALLIGHT;
			if (v7 != (m_ucEffectType == eObjectType) && m_pItem->m_bOn) {
				int32_t v8 = v3++;
				if (v8 == uiObjectIndex) {
					break;
				}
			}
			if (!m_pLightList) {
				return 0;
			}
		}
		return m_pItem;
	}
	case NiShaderAttributeDesc::OT_EFFECT_SHADOWPOINTLIGHT: {
		NiTextureEffect* m_pEnvMap = pkEffectState->m_pEnvMap;
		v20 = m_pEnvMap;
		if (!m_pEnvMap) {
			return v20;
		}
		v11 = !m_pEnvMap->m_bOn;
		goto LABEL_31;
	}
	case NiShaderAttributeDesc::OT_EFFECT_SHADOWDIRECTIONALLIGHT: {
		NiDynamicEffectState::ListItem* m_pProjShadowList = pkEffectState->m_pProjShadowList;
		int32_t v13 = 0;
		if (!m_pProjShadowList) {
			return v20;
		}
		while (1) {
			v14 = m_pProjShadowList->m_pItem;
			m_pProjShadowList = m_pProjShadowList->m_pNext;
			if (v14) {
				if (v14->m_bOn) {
					int32_t v15 = v13++;
					if (v15 == uiObjectIndex) {
						break;
					}
				}
			}
			if (!m_pProjShadowList) {
				return 0;
			}
		}
		return v14;
	}
	case NiShaderAttributeDesc::OT_EFFECT_SHADOWSPOTLIGHT: {
		m_pProjLightList = pkEffectState->m_pProjLightList;
		v17 = 0;
		if (!m_pProjLightList) {
			return v20;
		}
		break;
	}
	case NiShaderAttributeDesc::OT_EFFECT_ENVIRONMENTMAP: {
		NiTextureEffect* m_pFogMap;
		m_pFogMap = pkEffectState->m_pFogMap;
		v20 = m_pFogMap;
		if (!m_pFogMap) {
			return v20;
		}
		v11 = !m_pFogMap->m_bOn;
	LABEL_31:;
		if (v11) {
			return 0;
		}
		return v20;
	}
	default: {
		return v20;
	}
	}
	while (1) {
		v14 = m_pProjLightList->m_pItem;
		m_pProjLightList = m_pProjLightList->m_pNext;
		if (v14) {
			if (v14->m_bOn) {
				int32_t v18 = v17++;
				if (v18 == uiObjectIndex) {
					break;
				}
			}
		}
		if (!m_pProjLightList) {
			return 0;
		}
	}
	return v14;
}

// The reference source's NiDelete[] of both tables, written out: the exe walks each array's elements backwards,
// dropping the reference on each name, then hands the block, its 4-byte element-count cookie included, to
// MemoryManager::Deallocate. The elements' destructor is written out so no C++ destructor runs.
void NiShaderConstantMap::_SDMShutdown() {
	PredefinedMapping* pkMappings = ms_pkObjectMappings;
	if (pkMappings != nullptr) {
		// The count cookie is the word before an MSVC array allocation; the loop index is signed and runs to 0.
		int32_t iCount = static_cast<int32_t>(fnvd::arrayCount(pkMappings));
		for (int32_t i = iCount - 1; i >= 0; --i) {
			NiGlobalStringTable::GlobalStringHandle kHandle = pkMappings[i].m_kMappingName.GetHandle();
			if (kHandle != NiGlobalStringTable::NULL_STRING) {
				// A string's block is untyped memory whose first word is the reference count.
				InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kHandle) - 8));
			}
		}
		MemoryManager_Singleton.Deallocate(fnvd::arrayBlock(pkMappings));
	}
	ms_pkObjectMappings = nullptr;
	pkMappings = ms_pkPredefinedMappings;
	if (pkMappings != nullptr) {
		int32_t iCount = static_cast<int32_t>(fnvd::arrayCount(pkMappings));
		for (int32_t i = iCount - 1; i >= 0; --i) {
			NiGlobalStringTable::GlobalStringHandle kHandle = pkMappings[i].m_kMappingName.GetHandle();
			if (kHandle != NiGlobalStringTable::NULL_STRING) {
				InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kHandle) - 8));
			}
		}
		MemoryManager_Singleton.Deallocate(fnvd::arrayBlock(pkMappings));
	}
	ms_pkPredefinedMappings = nullptr;
}

bool NiShaderConstantMap::ParseKeyName(const char* pszOrigName, char* pszTrueName, uint32_t uiMaxLen,
	uint32_t& uiRegisterCount) {
	char* Context;
	char Str[260];
	if (uiMaxLen + 1 < 260) {
		return 0;
	}
	strcpy_s(Str, 260, pszOrigName);
	strcpy_s(pszTrueName, 260, pszOrigName);
	char* v5 = std::strchr(Str, 95);
	if (v5) {
		if (std::isdigit(v5[1])) {
			if (!v5[2]) {
				char* v7 = strtok_s(Str, "_", &Context);
				char* v8 = strtok_s(0, "_", &Context);
				if (v8) {
					uiRegisterCount = std::atoi(v8);
					NiStrcpy(pszTrueName, uiMaxLen, v7);
				}
			}
		}
	}
	return 1;
}

// Gamebryo's _SDMInit: NiNew arrays of 136 predefined and 25 object mappings, each entry assigned a temporary
// PredefinedMapping(ID, NiFixedString(name)) whose name and string are then released. The exe inlines the
// temporary's constructor for the first 20 predefined entries and calls it (0xA9BDA0) for the rest; it builds the
// string inline (AddString's reference kept) up to the object table's 13th entry, then calls NiFixedString's
// constructor (0x438170). The cleanup-only EH frame is left out.
void NiShaderConstantMap::_SDMInit() {
	FNVD_INLINED_LOCAL(NiFixedString, kString, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, kConstructedString, std::nullopt, std::nullopt);
	// The exe realigns esp to 8 for the temporary mapping it passes to the constructor.
	alignas(8) FNVD_INLINED_LOCAL(PredefinedMapping, kTemp, std::nullopt, std::nullopt);

	ms_uiPredefinedMappingCount = SCM_DEF_COUNT + 20;
	size_t* puiBlock = static_cast<size_t*>(MemoryManager_Singleton.Allocate(sizeof(size_t) +
		(SCM_DEF_COUNT + 20) * sizeof(PredefinedMapping)));
	PredefinedMapping* pkMappings = nullptr;
	if (puiBlock) {
		// new[]'s cookie: the element count in the dword before the array, memory the exe writes untyped.
		puiBlock[0] = SCM_DEF_COUNT + 20;
		pkMappings = reinterpret_cast<PredefinedMapping*>(puiBlock + 1);
		rt::eh_vector_constructor_iterator(pkMappings, sizeof(PredefinedMapping), SCM_DEF_COUNT + 20,
			&fnvd::constructElement<NiShaderConstantMap::PredefinedMapping>);
	}
	ms_pkPredefinedMappings = pkMappings;

	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Invalid");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_INVALID;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[0] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Proj");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_PROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[1] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Projection");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_PROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[2] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvProj");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_INVPROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[3] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvProjection");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_INVPROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[4] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("View");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_VIEW;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[5] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvView");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_INVVIEW;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[6] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("World");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_WORLD;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[7] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvWorld");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_INVWORLD;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[8] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("WorldView");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_WORLDVIEW;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[9] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvWorldView");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_INVWORLDVIEW;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[10] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("ViewProj");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_VIEWPROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[11] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("ViewProjection");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_VIEWPROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[12] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvViewProj");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_INVVIEWPROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[13] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvViewProjection");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_INVVIEWPROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[14] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("WorldViewProj");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_WORLDVIEWPROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[15] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("WorldViewProjection");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_WORLDVIEWPROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[16] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvWorldViewProj");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_INVWORLDVIEWPROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[17] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvWorldViewProjection");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_INVWORLDVIEWPROJ;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[18] = kTemp;
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("ProjTranspose");
	// EXE-INLINED 0xA9BDA0
	kTemp.m_uiMappingID = SCM_DEF_PROJ_T;
	kTemp.m_kMappingName.m_kHandle = kString.GetHandle();
	NiGlobalStringTable::AcquireHandle(kString.GetHandle());
	ms_pkPredefinedMappings[19] = kTemp;
	// kTemp's release before each FNVD_CONSTRUCT: the exe's inlined destructor.
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("ProjectionTranspose");
	ms_pkPredefinedMappings[20] = *::new (&kTemp) PredefinedMapping(SCM_DEF_PROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvProjTranspose");
	ms_pkPredefinedMappings[21] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvProjectionTranspose");
	ms_pkPredefinedMappings[22] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("ViewTranspose");
	ms_pkPredefinedMappings[23] = *::new (&kTemp) PredefinedMapping(SCM_DEF_VIEW_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvViewTranspose");
	ms_pkPredefinedMappings[24] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVVIEW_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("WorldTranspose");
	ms_pkPredefinedMappings[25] = *::new (&kTemp) PredefinedMapping(SCM_DEF_WORLD_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvWorldTranspose");
	ms_pkPredefinedMappings[26] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVWORLD_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("WorldViewTranspose");
	ms_pkPredefinedMappings[27] = *::new (&kTemp) PredefinedMapping(SCM_DEF_WORLDVIEW_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvWorldViewTranspose");
	ms_pkPredefinedMappings[28] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVWORLDVIEW_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("ViewProjTranspose");
	ms_pkPredefinedMappings[29] = *::new (&kTemp) PredefinedMapping(SCM_DEF_VIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("ViewProjectionTranspose");
	ms_pkPredefinedMappings[30] = *::new (&kTemp) PredefinedMapping(SCM_DEF_VIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvViewProjTranspose");
	ms_pkPredefinedMappings[31] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVVIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvViewProjectionTranspose");
	ms_pkPredefinedMappings[32] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVVIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("WorldViewProjTranspose");
	ms_pkPredefinedMappings[33] = *::new (&kTemp) PredefinedMapping(SCM_DEF_WORLDVIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("WorldViewProjectionTranspose");
	ms_pkPredefinedMappings[34] = *::new (&kTemp) PredefinedMapping(SCM_DEF_WORLDVIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvWorldViewProjTranspose");
	ms_pkPredefinedMappings[35] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVWORLDVIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvWorldViewProjectionTranspose");
	ms_pkPredefinedMappings[36] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVWORLDVIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SkinWorldView");
	ms_pkPredefinedMappings[37] = *::new (&kTemp) PredefinedMapping(SCM_DEF_SKINWORLDVIEW, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvSkinWorldView");
	ms_pkPredefinedMappings[38] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVSKINWORLDVIEW, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SkinWorldViewTranspose");
	ms_pkPredefinedMappings[39] = *::new (&kTemp) PredefinedMapping(SCM_DEF_SKINWORLDVIEW_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvSkinWorldViewTranspose");
	ms_pkPredefinedMappings[40] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVSKINWORLDVIEW_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SkinWorldViewProj");
	ms_pkPredefinedMappings[41] = *::new (&kTemp) PredefinedMapping(SCM_DEF_SKINWORLDVIEWPROJ, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SkinWorldViewProjection");
	ms_pkPredefinedMappings[42] = *::new (&kTemp) PredefinedMapping(SCM_DEF_SKINWORLDVIEWPROJ, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvSkinWorldViewProj");
	ms_pkPredefinedMappings[43] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVSKINWORLDVIEWPROJ, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvSkinWorldViewProjection");
	ms_pkPredefinedMappings[44] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVSKINWORLDVIEWPROJ, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SkinWorldViewProjTranspose");
	ms_pkPredefinedMappings[45] = *::new (&kTemp) PredefinedMapping(SCM_DEF_SKINWORLDVIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SkinWorldViewProjectionTranspose");
	ms_pkPredefinedMappings[46] = *::new (&kTemp) PredefinedMapping(SCM_DEF_SKINWORLDVIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvSkinWorldViewProjTranspose");
	ms_pkPredefinedMappings[47] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVSKINWORLDVIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvSkinWorldViewProjectionTranspose");
	ms_pkPredefinedMappings[48] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVSKINWORLDVIEWPROJ_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("BoneMatrix3");
	ms_pkPredefinedMappings[49] = *::new (&kTemp) PredefinedMapping(SCM_DEF_BONE_MATRIX_3, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("BoneMatrix4");
	ms_pkPredefinedMappings[50] = *::new (&kTemp) PredefinedMapping(SCM_DEF_BONE_MATRIX_4, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SkinBoneMatrix3");
	ms_pkPredefinedMappings[51] = *::new (&kTemp) PredefinedMapping(SCM_DEF_SKINBONE_MATRIX_3, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SkinBoneMatrix4");
	ms_pkPredefinedMappings[52] = *::new (&kTemp) PredefinedMapping(SCM_DEF_SKINBONE_MATRIX_4, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformBase");
	ms_pkPredefinedMappings[53] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMBASE, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformBase");
	ms_pkPredefinedMappings[54] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMBASE, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformBaseTranspose");
	ms_pkPredefinedMappings[55] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMBASE_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformBaseTranspose");
	ms_pkPredefinedMappings[56] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMBASE_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformDark");
	ms_pkPredefinedMappings[57] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMDARK, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformDark");
	ms_pkPredefinedMappings[58] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDARK, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformDarkTranspose");
	ms_pkPredefinedMappings[59] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMDARK_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformDarkTranspose");
	ms_pkPredefinedMappings[60] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDARK_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformDetail");
	ms_pkPredefinedMappings[61] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMDETAIL, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformDetail");
	ms_pkPredefinedMappings[62] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDETAIL, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformDetailTranspose");
	ms_pkPredefinedMappings[63] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMDETAIL_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformDetailTranspose");
	ms_pkPredefinedMappings[64] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDETAIL_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformGloss");
	ms_pkPredefinedMappings[65] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMGLOSS, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformGloss");
	ms_pkPredefinedMappings[66] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMGLOSS, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformGlossTranspose");
	ms_pkPredefinedMappings[67] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMGLOSS_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformGlossTranspose");
	ms_pkPredefinedMappings[68] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMGLOSS_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformGlow");
	ms_pkPredefinedMappings[69] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMGLOW, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformGlow");
	ms_pkPredefinedMappings[70] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMGLOW, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformGlowTranspose");
	ms_pkPredefinedMappings[71] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMGLOW_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformGlowTranspose");
	ms_pkPredefinedMappings[72] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMGLOW_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformBump");
	ms_pkPredefinedMappings[73] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMBUMP, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformBump");
	ms_pkPredefinedMappings[74] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMBUMP, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformBumpTranspose");
	ms_pkPredefinedMappings[75] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMBUMP_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformBumpTranspose");
	ms_pkPredefinedMappings[76] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMBUMP_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformDecal");
	ms_pkPredefinedMappings[77] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMDECAL, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformDecal");
	ms_pkPredefinedMappings[78] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDECAL, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformDecalTranspose");
	ms_pkPredefinedMappings[79] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMDECAL_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformDecalTranspose");
	ms_pkPredefinedMappings[80] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMDECAL_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformShader");
	ms_pkPredefinedMappings[81] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMSHADER, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformShader");
	ms_pkPredefinedMappings[82] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMSHADER, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformShaderTranspose");
	ms_pkPredefinedMappings[83] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMSHADER_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformShaderTranspose");
	ms_pkPredefinedMappings[84] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMSHADER_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("LightPosWS");
	ms_pkPredefinedMappings[85] = *::new (&kTemp) PredefinedMapping(SCM_DEF_LIGHT_POS_WS, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("LightDirWS");
	ms_pkPredefinedMappings[86] = *::new (&kTemp) PredefinedMapping(SCM_DEF_LIGHT_DIR_WS, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("LightPosOS");
	ms_pkPredefinedMappings[87] = *::new (&kTemp) PredefinedMapping(SCM_DEF_LIGHT_POS_OS, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("LightDirOS");
	ms_pkPredefinedMappings[88] = *::new (&kTemp) PredefinedMapping(SCM_DEF_LIGHT_DIR_OS, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("MaterialDiffuse");
	ms_pkPredefinedMappings[89] = *::new (&kTemp) PredefinedMapping(SCM_DEF_MATERIAL_DIFFUSE, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Diffuse");
	ms_pkPredefinedMappings[90] = *::new (&kTemp) PredefinedMapping(SCM_DEF_MATERIAL_DIFFUSE, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("MaterialAmbient");
	ms_pkPredefinedMappings[91] = *::new (&kTemp) PredefinedMapping(SCM_DEF_MATERIAL_AMBIENT, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Ambient");
	ms_pkPredefinedMappings[92] = *::new (&kTemp) PredefinedMapping(SCM_DEF_MATERIAL_AMBIENT, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("MaterialSpecular");
	ms_pkPredefinedMappings[93] = *::new (&kTemp) PredefinedMapping(SCM_DEF_MATERIAL_SPECULAR, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Specular");
	ms_pkPredefinedMappings[94] = *::new (&kTemp) PredefinedMapping(SCM_DEF_MATERIAL_SPECULAR, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("MaterialEmissive");
	ms_pkPredefinedMappings[95] = *::new (&kTemp) PredefinedMapping(SCM_DEF_MATERIAL_EMISSIVE, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("MaterialPower");
	ms_pkPredefinedMappings[96] = *::new (&kTemp) PredefinedMapping(SCM_DEF_MATERIAL_POWER, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SpecularPower");
	ms_pkPredefinedMappings[97] = *::new (&kTemp) PredefinedMapping(SCM_DEF_MATERIAL_POWER, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("EyePos");
	ms_pkPredefinedMappings[98] = *::new (&kTemp) PredefinedMapping(SCM_DEF_EYE_POS, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("EyeDir");
	ms_pkPredefinedMappings[99] = *::new (&kTemp) PredefinedMapping(SCM_DEF_EYE_DIR, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TaylorSin");
	ms_pkPredefinedMappings[100] = *::new (&kTemp) PredefinedMapping(SCM_DEF_CONSTS_TAYLOR_SIN, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TaylorCos");
	ms_pkPredefinedMappings[101] = *::new (&kTemp) PredefinedMapping(SCM_DEF_CONSTS_TAYLOR_COS, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("time");
	ms_pkPredefinedMappings[102] = *::new (&kTemp) PredefinedMapping(SCM_DEF_CONSTS_TIME, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("sin_time");
	ms_pkPredefinedMappings[103] = *::new (&kTemp) PredefinedMapping(SCM_DEF_CONSTS_SINTIME, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("cos_time");
	ms_pkPredefinedMappings[104] = *::new (&kTemp) PredefinedMapping(SCM_DEF_CONSTS_COSTIME, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("tan_time");
	ms_pkPredefinedMappings[105] = *::new (&kTemp) PredefinedMapping(SCM_DEF_CONSTS_TANTIME, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("time_sin_cos_tan");
	ms_pkPredefinedMappings[106] =
		*::new (&kTemp) PredefinedMapping(SCM_DEF_CONSTS_TIME_SINTIME_COSTIME_TANTIME, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("AmbientLight");
	ms_pkPredefinedMappings[107] = *::new (&kTemp) PredefinedMapping(SCM_DEF_AMBIENTLIGHT, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformParallax");
	ms_pkPredefinedMappings[108] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMPARALLAX, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformParallax");
	ms_pkPredefinedMappings[109] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMPARALLAX, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformParallaxTranspose");
	ms_pkPredefinedMappings[110] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMPARALLAX_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformParallaxTranspose");
	ms_pkPredefinedMappings[111] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMPARALLAX_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformNormal");
	ms_pkPredefinedMappings[112] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMNORMAL, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformNormal");
	ms_pkPredefinedMappings[113] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMNORMAL, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexTransformNormalTranspose");
	ms_pkPredefinedMappings[114] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXTRANSFORMNORMAL_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvTexTransformNormalTranspose");
	ms_pkPredefinedMappings[115] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVTEXTRANSFORMNORMAL_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SkinWorld");
	ms_pkPredefinedMappings[116] = *::new (&kTemp) PredefinedMapping(SCM_DEF_SKINWORLD, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvSkinWorld");
	ms_pkPredefinedMappings[117] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVSKINWORLD, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("SkinWorldTranspose");
	ms_pkPredefinedMappings[118] = *::new (&kTemp) PredefinedMapping(SCM_DEF_SKINWORLD_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("InvSkinWorldTranspose");
	ms_pkPredefinedMappings[119] = *::new (&kTemp) PredefinedMapping(SCM_DEF_INVSKINWORLD_T, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("FogDensity");
	ms_pkPredefinedMappings[120] = *::new (&kTemp) PredefinedMapping(SCM_DEF_FOG_DENSITY, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("FogNearFar");
	ms_pkPredefinedMappings[121] = *::new (&kTemp) PredefinedMapping(SCM_DEF_FOG_NEARFAR, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("FogColor");
	ms_pkPredefinedMappings[122] = *::new (&kTemp) PredefinedMapping(SCM_DEF_FOG_COLOR, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("BumpMatrix");
	ms_pkPredefinedMappings[123] = *::new (&kTemp) PredefinedMapping(SCM_DEF_BUMP_MATRIX, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("BumpLumaAndScale");
	ms_pkPredefinedMappings[124] = *::new (&kTemp) PredefinedMapping(SCM_DEF_BUMP_LUMA_OFFSET_AND_SCALE, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("ParallaxOffset");
	ms_pkPredefinedMappings[125] = *::new (&kTemp) PredefinedMapping(SCM_DEF_PARALLAX_OFFSET, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexSizeBase");
	ms_pkPredefinedMappings[126] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXSIZEBASE, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexSizeDark");
	ms_pkPredefinedMappings[127] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXSIZEDARK, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexSizeDetail");
	ms_pkPredefinedMappings[128] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXSIZEDETAIL, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexSizeGloss");
	ms_pkPredefinedMappings[129] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXSIZEGLOSS, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexSizeGlow");
	ms_pkPredefinedMappings[130] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXSIZEGLOW, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexSizeBump");
	ms_pkPredefinedMappings[131] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXSIZEBUMP, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexSizeNormal");
	ms_pkPredefinedMappings[132] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXSIZEBUMP, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexSizeParallax");
	ms_pkPredefinedMappings[133] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXSIZEBUMP, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexSizeDecal");
	ms_pkPredefinedMappings[134] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXSIZEDECAL, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("TexSizeShader");
	ms_pkPredefinedMappings[135] = *::new (&kTemp) PredefinedMapping(SCM_DEF_TEXSIZESHADER, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());

	ms_uiObjectMappingCount = SCM_OBJ_COUNT;
	puiBlock = static_cast<size_t*>(MemoryManager_Singleton.Allocate(sizeof(size_t) +
		SCM_OBJ_COUNT * sizeof(PredefinedMapping)));
	pkMappings = nullptr;
	if (puiBlock) {
		// new[]'s cookie, as above.
		puiBlock[0] = SCM_OBJ_COUNT;
		pkMappings = reinterpret_cast<PredefinedMapping*>(puiBlock + 1);
		rt::eh_vector_constructor_iterator(pkMappings, sizeof(PredefinedMapping), SCM_OBJ_COUNT,
			&fnvd::constructElement<NiShaderConstantMap::PredefinedMapping>);
	}
	ms_pkObjectMappings = pkMappings;

	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Invalid");
	ms_pkObjectMappings[0] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_INVALID, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Dimmer");
	ms_pkObjectMappings[1] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_DIMMER, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("UndimmedAmbient");
	ms_pkObjectMappings[2] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_UNDIMMEDAMBIENT, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("UndimmedDiffuse");
	ms_pkObjectMappings[3] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_UNDIMMEDDIFFUSE, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("UndimmedSpecular");
	ms_pkObjectMappings[4] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_UNDIMMEDSPECULAR, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Ambient");
	ms_pkObjectMappings[5] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_AMBIENT, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Diffuse");
	ms_pkObjectMappings[6] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_DIFFUSE, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("Specular");
	ms_pkObjectMappings[7] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_SPECULAR, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("WorldPosition");
	ms_pkObjectMappings[8] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_WORLDPOSITION, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("ModelPosition");
	ms_pkObjectMappings[9] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_MODELPOSITION, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("WorldDirection");
	ms_pkObjectMappings[10] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_WORLDDIRECTION, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("ModelDirection");
	ms_pkObjectMappings[11] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_MODELDIRECTION, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	// EXE-INLINED 0x438170
	kString.m_kHandle = NiGlobalStringTable::AddString("WorldTransform");
	ms_pkObjectMappings[12] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_WORLDTRANSFORM, kString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// kString's last release: the exe's inlined destructor. Each earlier release
	// followed by a new handle's store is the assignment of the next name.
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("ModelTransform");
	ms_pkObjectMappings[13] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_MODELTRANSFORM, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("SpotAttenuation");
	ms_pkObjectMappings[14] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_SPOTATTENUATION, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("Attenuation");
	ms_pkObjectMappings[15] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_ATTENUATION, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("WorldProjectionMatrix");
	ms_pkObjectMappings[16] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_WORLDPROJECTIONMATRIX, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("ModelProjectionMatrix");
	ms_pkObjectMappings[17] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_MODELPROJECTIONMATRIX, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("WorldProjectionTranslation");
	ms_pkObjectMappings[18] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_WORLDPROJECTIONTRANSLATION, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("ModelProjectionTranslation");
	ms_pkObjectMappings[19] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_MODELPROJECTIONTRANSLATION, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("WorldClippingPlane");
	ms_pkObjectMappings[20] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_WORLDCLIPPINGPLANE, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("ModelClippingPlane");
	ms_pkObjectMappings[21] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_MODELCLIPPINGPLANE, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("TexCoordGen");
	ms_pkObjectMappings[22] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_TEXCOORDGEN, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("WorldProjectionTransform");
	ms_pkObjectMappings[23] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_WORLDPROJECTIONTRANSFORM, kConstructedString);
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
	::new (static_cast<void*>(&kConstructedString)) NiFixedString("ModelProjectionTransform");
	ms_pkObjectMappings[24] = *::new (&kTemp) PredefinedMapping(SCM_OBJ_MODELPROJECTIONTRANSFORM, kConstructedString);
	// The last releases: the exe's inlined destructors of kTemp and kConstructedString.
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kTemp.m_kMappingName.GetHandle());
	// EXE-INLINED 0x4381B0
	NiGlobalStringTable::ReleaseHandle(kConstructedString.GetHandle());
}
