#pragma once

#include "Reconstruction.hpp"
#include "SpeedTree/SpeedTreeRT.h"

// speedtree-2 LightingEngine.h: the light and material array dimensions.
inline constexpr int32_t c_nMaxLights = 8;

inline constexpr int32_t c_nLightPos = 0;
inline constexpr int32_t c_nLightDiffuse = c_nLightPos + 3;
inline constexpr int32_t c_nLightAmbient = c_nLightDiffuse + 3;
inline constexpr int32_t c_nLightSpecular = c_nLightAmbient + 3;
inline constexpr int32_t c_nLightDirectional = c_nLightSpecular + 3;
inline constexpr int32_t c_nLightAttenuation = c_nLightDirectional + 1;
inline constexpr int32_t c_nLightParamSize = c_nLightAttenuation + 3;

inline constexpr int32_t c_nMaterialDiffuse = 0;
inline constexpr int32_t c_nMaterialAmbient = c_nMaterialDiffuse + 3;
inline constexpr int32_t c_nMaterialSpecular = c_nMaterialAmbient + 3;
inline constexpr int32_t c_nMaterialEmissive = c_nMaterialSpecular + 3;
inline constexpr int32_t c_nMaterialShininess = c_nMaterialEmissive + 3;
inline constexpr int32_t c_nMaterialParamSize = c_nMaterialShininess + 1;

// The lights and materials a tree is lit with when its lighting is static (the lighting method comes from the tree's
// SpeedTreeRT settings). Members are public here so that their offsets can be pinned; speedtree-2 makes them private.
class CLightingEngine {
public:
	enum EMaterial : int32_t {
		MATERIAL_BRANCH = 0,
		MATERIAL_LEAF = 1,
		MATERIAL_FROND = 2,
	};

	using LightEnabledArray = bool[c_nMaxLights];
	using LightAttributeArray = float[c_nMaxLights][c_nLightParamSize];

	CLightingEngine();

	// Lights the nNumLeafLods leaf LODs pLeafLods around cTreeCenter.
	void ComputeLeafStaticLighting(const CVec3& cTreeCenter,
		std::vector<class CBillboardLeaf*, std::allocator<class CBillboardLeaf*>>* pLeafLods, int32_t nNumLeafLods);
	// Reads the lighting block from cFile.
	void Parse(class CTreeFileAccess& cFile);

	CSpeedTreeRT::ELightingMethod m_eBranchLightingMethod;
	float m_afBranchMaterial[c_nMaterialParamSize];
	CSpeedTreeRT::ELightingMethod m_eLeafLightingMethod;
	float m_afLeafMaterial[c_nMaterialParamSize];
	float m_fLeafLightingAdjustmentScalar;
	CSpeedTreeRT::EStaticLightingStyle m_eStaticLightingStyle;
	CSpeedTreeRT::ELightingMethod m_eFrondLightingMethod;
	float m_afFrondMaterial[c_nMaterialParamSize];

	static inline LightEnabledArray m_abLightEnabled{};
	// speedtree-2 LightingEngine.cpp's statics: eight default lights (at the origin, white diffuse and specular, no
	// ambient, positional, attenuation 1, 0, 0), none enabled.
	static inline LightAttributeArray m_afLightAttributes{
		{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f}};
};
