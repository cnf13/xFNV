#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

void cleanupLoadedObjecthkxMaterialShader(void* p);

// Shader source or filename plus the programmable-stage entry points.
class hkxMaterialShader : public hkReferencedObject {
public:
	enum ShaderType : int32_t {
		EFFECT_TYPE_INVALID = 0,
		EFFECT_TYPE_UNKNOWN = 1,
		EFFECT_TYPE_HLSL_INLINE = 2,
		EFFECT_TYPE_CG_INLINE = 3,
		EFFECT_TYPE_HLSL_FILENAME = 4,
		EFFECT_TYPE_CG_FILENAME = 5,
		EFFECT_TYPE_MAX_ID = 6,
	};

	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkxMaterialShader(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_name(aFlag), m_vertexEntryName(aFlag), m_geomEntryName(aFlag),
		  m_pixelEntryName(aFlag), m_data(aFlag) {}

	~hkxMaterialShader() override;

	static const hkClass& staticClass();

	hkStringPtr m_name;
	hkEnum<ShaderType, uint8_t> m_type;
	hkStringPtr m_vertexEntryName;
	hkStringPtr m_geomEntryName;
	hkStringPtr m_pixelEntryName;
	hkArray<uint8_t> m_data;
};

inline constexpr fnvd::Vtable hkxMaterialShader_vftable{};
extern hkClass hkxMaterialShaderClass;
extern hkTypeInfo hkxMaterialShaderTypeInfo;
