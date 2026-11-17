#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiDynamicEffect;
class NiDynamicEffectState;

// The shader constants one GPU program takes from the scene; the renderer's subclass does the work, this base
// rejects every entry (NiShaderError 0x30).
class NiShaderConstantMap : public NiRefObject {
public:
	// A predefined constant's ID and the name a shader refers to it by.
	class PredefinedMapping : public NiMemObject {
	public:
		PredefinedMapping();
		PredefinedMapping(uint32_t auiMappingID, const NiFixedString& akMappingName);

		// Copies the ID, then the name, taking a reference on the new handle and dropping the old one's unless they are
		// the same.
		PredefinedMapping& operator=(const PredefinedMapping& kOther) {
			m_uiMappingID = kOther.m_uiMappingID;
			NiGlobalStringTable::GlobalStringHandle kHandle = kOther.m_kMappingName.GetHandle();
			if (m_kMappingName.GetHandle() != kHandle) {
				NiGlobalStringTable::AcquireHandle(kHandle);
				NiGlobalStringTable::ReleaseHandle(m_kMappingName.GetHandle());
				m_kMappingName.m_kHandle = kHandle;
			}
			return *this;
		}

		uint32_t m_uiMappingID;
		NiFixedString m_kMappingName;
	};

	// The predefined ("defined") constants a shader can name; an entry's m_uiInternal for SCME_MAP_DEFINED.
	enum DefinedMappings {
		SCM_DEF_INVALID = 0x0,
		SCM_DEF_PROJ = 0x1,
		SCM_DEF_INVPROJ = 0x2,
		SCM_DEF_VIEW = 0x3,
		SCM_DEF_INVVIEW = 0x4,
		SCM_DEF_WORLD = 0x5,
		SCM_DEF_INVWORLD = 0x6,
		SCM_DEF_WORLDVIEW = 0x7,
		SCM_DEF_INVWORLDVIEW = 0x8,
		SCM_DEF_VIEWPROJ = 0x9,
		SCM_DEF_INVVIEWPROJ = 0xA,
		SCM_DEF_WORLDVIEWPROJ = 0xB,
		SCM_DEF_INVWORLDVIEWPROJ = 0xC,
		SCM_DEF_PROJ_T = 0xD,
		SCM_DEF_INVPROJ_T = 0xE,
		SCM_DEF_VIEW_T = 0xF,
		SCM_DEF_INVVIEW_T = 0x10,
		SCM_DEF_WORLD_T = 0x11,
		SCM_DEF_INVWORLD_T = 0x12,
		SCM_DEF_WORLDVIEW_T = 0x13,
		SCM_DEF_INVWORLDVIEW_T = 0x14,
		SCM_DEF_VIEWPROJ_T = 0x15,
		SCM_DEF_INVVIEWPROJ_T = 0x16,
		SCM_DEF_WORLDVIEWPROJ_T = 0x17,
		SCM_DEF_INVWORLDVIEWPROJ_T = 0x18,
		SCM_DEF_SKINWORLDVIEW = 0x19,
		SCM_DEF_INVSKINWORLDVIEW = 0x1A,
		SCM_DEF_SKINWORLDVIEW_T = 0x1B,
		SCM_DEF_INVSKINWORLDVIEW_T = 0x1C,
		SCM_DEF_SKINWORLDVIEWPROJ = 0x1D,
		SCM_DEF_INVSKINWORLDVIEWPROJ = 0x1E,
		SCM_DEF_SKINWORLDVIEWPROJ_T = 0x1F,
		SCM_DEF_INVSKINWORLDVIEWPROJ_T = 0x20,
		SCM_DEF_BONE_MATRIX_3 = 0x21,
		SCM_DEF_BONE_MATRIX_4 = 0x22,
		SCM_DEF_SKINBONE_MATRIX_3 = 0x23,
		SCM_DEF_SKINBONE_MATRIX_4 = 0x24,
		SCM_DEF_TEXTRANSFORMBASE = 0x25,
		SCM_DEF_INVTEXTRANSFORMBASE = 0x26,
		SCM_DEF_TEXTRANSFORMBASE_T = 0x27,
		SCM_DEF_INVTEXTRANSFORMBASE_T = 0x28,
		SCM_DEF_TEXTRANSFORMDARK = 0x29,
		SCM_DEF_INVTEXTRANSFORMDARK = 0x2A,
		SCM_DEF_TEXTRANSFORMDARK_T = 0x2B,
		SCM_DEF_INVTEXTRANSFORMDARK_T = 0x2C,
		SCM_DEF_TEXTRANSFORMDETAIL = 0x2D,
		SCM_DEF_INVTEXTRANSFORMDETAIL = 0x2E,
		SCM_DEF_TEXTRANSFORMDETAIL_T = 0x2F,
		SCM_DEF_INVTEXTRANSFORMDETAIL_T = 0x30,
		SCM_DEF_TEXTRANSFORMGLOSS = 0x31,
		SCM_DEF_INVTEXTRANSFORMGLOSS = 0x32,
		SCM_DEF_TEXTRANSFORMGLOSS_T = 0x33,
		SCM_DEF_INVTEXTRANSFORMGLOSS_T = 0x34,
		SCM_DEF_TEXTRANSFORMGLOW = 0x35,
		SCM_DEF_INVTEXTRANSFORMGLOW = 0x36,
		SCM_DEF_TEXTRANSFORMGLOW_T = 0x37,
		SCM_DEF_INVTEXTRANSFORMGLOW_T = 0x38,
		SCM_DEF_TEXTRANSFORMBUMP = 0x39,
		SCM_DEF_INVTEXTRANSFORMBUMP = 0x3A,
		SCM_DEF_TEXTRANSFORMBUMP_T = 0x3B,
		SCM_DEF_INVTEXTRANSFORMBUMP_T = 0x3C,
		SCM_DEF_TEXTRANSFORMDECAL = 0x3D,
		SCM_DEF_INVTEXTRANSFORMDECAL = 0x3E,
		SCM_DEF_TEXTRANSFORMDECAL_T = 0x3F,
		SCM_DEF_INVTEXTRANSFORMDECAL_T = 0x40,
		SCM_DEF_TEXTRANSFORMSHADER = 0x41,
		SCM_DEF_INVTEXTRANSFORMSHADER = 0x42,
		SCM_DEF_TEXTRANSFORMSHADER_T = 0x43,
		SCM_DEF_INVTEXTRANSFORMSHADER_T = 0x44,
		SCM_DEF_MATRIXTYPE_COUNT = 0x45,
		SCM_DEF_LIGHT_POS_WS = 0x45,
		SCM_DEF_LIGHT_DIR_WS = 0x46,
		SCM_DEF_LIGHT_POS_OS = 0x47,
		SCM_DEF_LIGHT_DIR_OS = 0x48,
		SCM_DEF_MATERIAL_DIFFUSE = 0x49,
		SCM_DEF_MATERIAL_AMBIENT = 0x4A,
		SCM_DEF_MATERIAL_SPECULAR = 0x4B,
		SCM_DEF_MATERIAL_EMISSIVE = 0x4C,
		SCM_DEF_MATERIAL_POWER = 0x4D,
		SCM_DEF_EYE_POS = 0x4E,
		SCM_DEF_EYE_DIR = 0x4F,
		SCM_DEF_CONSTS_TAYLOR_SIN = 0x50,
		SCM_DEF_CONSTS_TAYLOR_COS = 0x51,
		SCM_DEF_CONSTS_TIME = 0x52,
		SCM_DEF_CONSTS_SINTIME = 0x53,
		SCM_DEF_CONSTS_COSTIME = 0x54,
		SCM_DEF_CONSTS_TANTIME = 0x55,
		SCM_DEF_CONSTS_TIME_SINTIME_COSTIME_TANTIME = 0x56,
		SCM_DEF_AMBIENTLIGHT = 0x57,
		SCM_DEF_TEXTRANSFORMNORMAL = 0x58,
		SCM_DEF_INVTEXTRANSFORMNORMAL = 0x59,
		SCM_DEF_TEXTRANSFORMNORMAL_T = 0x5A,
		SCM_DEF_INVTEXTRANSFORMNORMAL_T = 0x5B,
		SCM_DEF_TEXTRANSFORMPARALLAX = 0x5C,
		SCM_DEF_INVTEXTRANSFORMPARALLAX = 0x5D,
		SCM_DEF_TEXTRANSFORMPARALLAX_T = 0x5E,
		SCM_DEF_INVTEXTRANSFORMPARALLAX_T = 0x5F,
		SCM_DEF_SKINWORLD = 0x60,
		SCM_DEF_INVSKINWORLD = 0x61,
		SCM_DEF_SKINWORLD_T = 0x62,
		SCM_DEF_INVSKINWORLD_T = 0x63,
		SCM_DEF_FOG_DENSITY = 0x64,
		SCM_DEF_FOG_NEARFAR = 0x65,
		SCM_DEF_FOG_COLOR = 0x66,
		SCM_DEF_BUMP_MATRIX = 0x67,
		SCM_DEF_BUMP_LUMA_OFFSET_AND_SCALE = 0x68,
		SCM_DEF_PARALLAX_OFFSET = 0x69,
		SCM_DEF_TEXSIZEBASE = 0x6A,
		SCM_DEF_TEXSIZEDARK = 0x6B,
		SCM_DEF_TEXSIZEDETAIL = 0x6C,
		SCM_DEF_TEXSIZEGLOSS = 0x6D,
		SCM_DEF_TEXSIZEGLOW = 0x6E,
		SCM_DEF_TEXSIZEBUMP = 0x6F,
		SCM_DEF_TEXSIZENORMAL = 0x70,
		SCM_DEF_TEXSIZEPARALLAX = 0x71,
		SCM_DEF_TEXSIZEDECAL = 0x72,
		SCM_DEF_TEXSIZESHADER = 0x73,
		SCM_DEF_COUNT = 0x74,
	};

	// The values an object (0x60000000) entry takes from its dynamic effect; the entry's m_uiInternal.
	enum ObjectMappings {
		SCM_OBJ_INVALID = 0x0,
		SCM_OBJ_DIMMER = 0x1,
		SCM_OBJ_UNDIMMEDAMBIENT = 0x2,
		SCM_OBJ_UNDIMMEDDIFFUSE = 0x3,
		SCM_OBJ_UNDIMMEDSPECULAR = 0x4,
		SCM_OBJ_AMBIENT = 0x5,
		SCM_OBJ_DIFFUSE = 0x6,
		SCM_OBJ_SPECULAR = 0x7,
		SCM_OBJ_WORLDPOSITION = 0x8,
		SCM_OBJ_MODELPOSITION = 0x9,
		SCM_OBJ_WORLDDIRECTION = 0xA,
		SCM_OBJ_MODELDIRECTION = 0xB,
		SCM_OBJ_WORLDTRANSFORM = 0xC,
		SCM_OBJ_MODELTRANSFORM = 0xD,
		SCM_OBJ_SPOTATTENUATION = 0xE,
		SCM_OBJ_ATTENUATION = 0xF,
		SCM_OBJ_WORLDPROJECTIONMATRIX = 0x10,
		SCM_OBJ_MODELPROJECTIONMATRIX = 0x11,
		SCM_OBJ_WORLDPROJECTIONTRANSLATION = 0x12,
		SCM_OBJ_MODELPROJECTIONTRANSLATION = 0x13,
		SCM_OBJ_WORLDCLIPPINGPLANE = 0x14,
		SCM_OBJ_MODELCLIPPINGPLANE = 0x15,
		SCM_OBJ_TEXCOORDGEN = 0x16,
		SCM_OBJ_WORLDPROJECTIONTRANSFORM = 0x17,
		SCM_OBJ_MODELPROJECTIONTRANSFORM = 0x18,
		SCM_OBJ_COUNT = 0x19,
	};

	NiShaderConstantMap(NiGPUProgram::ProgramType eType);
	~NiShaderConstantMap() override;

	virtual uint32_t AddEntry(const char*, uint32_t, uint32_t, uint32_t, uint32_t, const char*, uint32_t, uint32_t,
		const void*, bool) {
		return 0x30;
	}

	virtual uint32_t AddPredefinedEntry(const char*, uint32_t, uint32_t, const char*) { return 0x30; }

	virtual uint32_t AddAttributeEntry(const char*, uint32_t, uint32_t, uint32_t, uint32_t, const char*, uint32_t,
		uint32_t, const void*, bool) {
		return 0x30;
	}

	virtual uint32_t AddConstantEntry(const char*, uint32_t, uint32_t, uint32_t, uint32_t, const char*, uint32_t,
		uint32_t, const void*, bool) {
		return 0x30;
	}

	virtual uint32_t AddGlobalEntry(const char*, uint32_t, uint32_t, uint32_t, uint32_t, const char*, uint32_t,
		uint32_t, const void*, bool) {
		return 0x30;
	}

	virtual uint32_t AddOperatorEntry(const char*, uint32_t, uint32_t, uint32_t, uint32_t, const char*) { return 0x30; }

	virtual uint32_t AddObjectEntry(const char*, uint32_t, const char*, uint32_t, NiShaderAttributeDesc::ObjectType) {
		return 0x30;
	}

	virtual void SortByEnabled() {}

	// The object mapping ID of a key; false when it names none.
	static bool LookUpObjectMapping(const char* pcMapping, uint32_t& uiMappingID);

	// The attribute type of an object mapping, with its register and float counts (both always written).
	static NiShaderAttributeDesc::AttributeType LookUpObjectMappingType(uint32_t uiMappingID, uint32_t& uiRegCount,
		uint32_t& uiFloatCount);

	// The predefined mapping ID and register count of a key; false when it names none.
	static bool LookUpPredefinedMapping(const char* pszMapping, uint32_t& uiMappingID, uint32_t& uiNumRegisters);

	// The attribute type of a predefined mapping with that register count.
	static NiShaderAttributeDesc::AttributeType LookUpPredefinedMappingType(uint32_t auiMappingID,
		uint32_t auiNumRegisters);

	// The attribute type of a predefined mapping key.
	static NiShaderAttributeDesc::AttributeType LookUpPredefinedMappingType(const char* apszMapping);

	// The effect of that object type and index in the effect state, or null.
	static NiDynamicEffect* GetDynamicEffectForObject(const NiDynamicEffectState* pkEffectState,
		NiShaderAttributeDesc::ObjectType eObjectType, uint32_t uiObjectIndex);
	// Frees both mapping tables and clears them.
	static void _SDMShutdown();
	static bool ParseKeyName(const char* pszOrigName, char* pszTrueName, uint32_t uiMaxLen, uint32_t& uiRegisterCount);
	// Builds the predefined and object mapping tables.
	static void _SDMInit();

	NiGPUProgram::ProgramType m_eProgramType;

	// The predefined and object mapping tables: made by _SDMInit, freed and cleared by _SDMShutdown.
	static inline PredefinedMapping* ms_pkPredefinedMappings;
	static inline PredefinedMapping* ms_pkObjectMappings;
	static inline const char* ms_pcTimeExtraDataName = "NDL_UpdateTime";
	static inline uint32_t ms_uiPredefinedMappingCount;
	static inline uint32_t ms_uiObjectMappingCount;
};
