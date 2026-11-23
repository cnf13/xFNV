#include "BSShader/BSFogProperty.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/SpeedTreeBranchShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

void SpeedTreeBranchShader::UpdateTogglesConstant(
    NiGeometry* apGeometry,
    BSFogProperty* apFog,
    NiMaterialProperty* apMat
) {
    double m_fShine;
    float g = NiColorA::BLACK.g;
    if (apFog) {
        g = 1.0f;
    }
    if (apMat) {
        m_fShine = apMat->m_fShine;
    }
    else {
        m_fShine = 30.0f;
    }
    float fEndWaterDistance = m_fShine;
    BSShaderLightingProperty::pShaderLightConstants[33].y = g;
    BSShaderLightingProperty::pShaderLightConstants[33].x = 1.0f;
    BSShaderLightingProperty::pShaderLightConstants[33].z = fEndWaterDistance;
    BSShaderLightingProperty::pShaderLightConstants[33].w = NiColorA::BLACK.a;
}

// Enables the constants with no alpha pass.
void SpeedTreeBranchShader::EnableConstantsForPass(uint16_t usPassEnum) {
    ShadowLightShader::EnableConstantsForPass(usPassEnum, false);
}
