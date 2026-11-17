#include "BSShader/BSFogProperty.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/SpeedTreeBranchShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

void SpeedTreeBranchShader::UpdateTogglesConstant(NiGeometry* a1, BSFogProperty* a2, NiMaterialProperty* a3) {
	double m_fShine;
	float g = NiColorA_BLACK.g;
	if (a2) {
		g = 1.0f;
	}
	if (a3) {
		m_fShine = a3->m_fShine;
	} else {
		m_fShine = 30.0f;
	}
	float fEndWaterDistance = m_fShine;
	BSShaderLightingProperty::pShaderLightConstants[33].y = g;
	BSShaderLightingProperty::pShaderLightConstants[33].x = 1.0f;
	BSShaderLightingProperty::pShaderLightConstants[33].z = fEndWaterDistance;
	BSShaderLightingProperty::pShaderLightConstants[33].w = NiColorA_BLACK.a;
}

// The exe zero-extends the pass and enables the constants with no alpha pass.
void SpeedTreeBranchShader::EnableConstantsForPass(uint16_t usPassEnum) {
	ShadowLightShader::EnableConstantsForPass(usPassEnum, false);
}
