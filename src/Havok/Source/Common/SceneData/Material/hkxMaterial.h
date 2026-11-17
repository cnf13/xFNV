#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"

// A scene material with texture stages, colors and sub-materials.
class hkxMaterial : public hkxAttributeHolder {
public:
	enum TextureType : int32_t {
		TEX_UNKNOWN = 0,
		TEX_DIFFUSE = 1,
		TEX_REFLECTION = 2,
		TEX_BUMP = 3,
		TEX_NORMAL = 4,
		TEX_DISPLACEMENT = 5,
		TEX_SPECULAR = 6,
		TEX_SPECULARANDGLOSS = 7,
		TEX_OPACITY = 8,
		TEX_EMISSIVE = 9,
		TEX_REFRACTION = 10,
		TEX_GLOSS = 11,
		TEX_NOTEXPORTED = 12,
	};

	struct TextureStage {
		static const hkClass& staticClass();

		hkRefVariant m_texture;
		hkEnum<TextureType, int32_t> m_usageHint;
		int32_t m_tcoordChannel;
	};

	hkxMaterial() = default;

	// The SDK's finish constructor: the loaded material keeps its fields; the name and the arrays are finished.
	hkxMaterial(hkFinishLoadedObjectFlag aFlag)
		: hkxAttributeHolder(aFlag), m_name(aFlag), m_stages(aFlag), m_subMaterials(aFlag), m_extraData(aFlag) {}

	~hkxMaterial() override;

	void sortTextureStageOrder();

	static const hkClass& staticClass();

	hkStringPtr m_name;
	hkArray<TextureStage> m_stages;
	hkVector4 m_diffuseColor;
	hkVector4 m_ambientColor;
	hkVector4 m_specularColor;
	hkVector4 m_emissiveColor;
	hkArray<hkRefPtr<hkxMaterial>> m_subMaterials;
	hkRefVariant m_extraData;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkxMaterialTextureStageTypeInfo;
