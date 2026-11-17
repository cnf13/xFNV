#include "BSShader/BSShaderSDM.h"

#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderUtil.h"
#include "Fallout/Misc/Main.h"

#include <cstdint>

void BSShaderSDM::InitSDM() {
    if (!bShaderInitialized) {
        BSFadeNode::InitSDM();
        BSShaderUtil::InitSDM();
        bShaderInitialized = 1;
    }
}

void BSShaderSDM::KillSDM() {
    if (bShaderInitialized) {
        BSFadeNode::KillSDM();
        BSShaderUtil::KillSDM();
        bShaderInitialized = 0;
    }
}
