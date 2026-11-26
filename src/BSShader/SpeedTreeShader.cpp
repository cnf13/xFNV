#include "BSShader/SpeedTreeShader.h"

#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"

void SpeedTreeShader::SetShaderConstants(NiD3DShaderConstantMap* apConstantMap, uint32_t auiCount) {
    apConstantMap->AddEntry("TreeData", 0x10000007u, 0, auiCount, 1u, "", 16u, 4u, pShaderData, false);
    apConstantMap->AddEntry(
        "WindMatrices",
        0x10000009u,
        0,
        auiCount + 1,
        16u,
        "",
        256u,
        4u,
        pWindMatrices,
        false
    );
}
