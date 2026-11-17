#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"
#include "Reconstruction.hpp"

// A named input, output or uniform of a material's shader (type, semantic, label, variable, default value and
// where its value comes from).
class NiMaterialResource : public NiRefObject {
public:
	enum Source : int32_t {
		SOURCE_UNKNOWN = 0x0,
		SOURCE_OBJECT = 0x1,
		SOURCE_PREDEFINED = 0x2,
		SOURCE_CONSTANT = 0x3,
		SOURCE_ATTRIBUTE = 0x4,
		SOURCE_GLOBAL = 0x5,
		SOURCE_MAX = 0x6,
	};

	NiMaterialResource()
		: m_uiCount(1), m_eSource(SOURCE_UNKNOWN), m_uiExtraData(0),
		  m_eObjectType(NiShaderAttributeDesc::OT_UNDEFINED) {}

	~NiMaterialResource() override;

	// The PDB's string setters, which the exe keeps out of line (NiGPUProgramCache::LoadEntry, 0xEE9CC0, calls
	// them). SetType and SetVariable are folds of other classes' setters of a string at the same offset.
	void SetType(const NiFixedString& kType);
	void SetSemantic(const NiFixedString& kSemantic);
	void SetLabel(const NiFixedString& kLabel);
	void SetVariable(const NiFixedString& kVariable);
	void SetDefaultValue(const NiFixedString& kValue);

	NiFixedString m_kType;
	NiFixedString m_kSemantic;
	NiFixedString m_kLabel;
	NiFixedString m_kVariable;
	NiFixedString m_kDefaultValue;
	uint32_t m_uiCount;
	Source m_eSource;
	uint32_t m_uiExtraData;
	NiShaderAttributeDesc::ObjectType m_eObjectType;
};
inline constexpr fnvd::Vtable NiMaterialResource_vftable{};
