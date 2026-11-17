#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/Lighting30Shader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"

#include <bit>

// Sets up every pass's stages, shaders, render states and constant flags, then builds the passes' constant maps. The
// depth bias is fDepthBiasUnit, read once into a float local; passes 5 and 6 take twice that, also stored as a float.
// The D3D values FALSE and TRUE are written as 0 and 1, the colour write masks as numbers. Where the pass was
// dereferenced before (the assignments of passes 1 to 7, 89 to 92, 70 and 71, and the end), the pass pointer is
// released without a null check, as the game does, so those releases are written out.
void Lighting30Shader::PresetStages() {
    float fDepthBias = BSShaderProperty::fDepthBiasUnit;
    // EXE-INLINED none: NiPointer<NiD3DPass>::NiPointer
    FNVD_INLINED_LOCAL(NiPointer<NiD3DPass>, spD3DPass, std::nullopt, std::nullopt);
    spD3DPass.m_pObject = spPass[0].m_pObject;
    if (spD3DPass.m_pObject) {
        spD3DPass.m_pObject->IncRefCount();
    }
    CreateDefaultStages(spD3DPass, 0);
    spD3DPass->m_spVertexShader = spVertexShaders[49];
    spD3DPass->m_spPixelShader = spPixelShaders[44];
    spD3DPass->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    ulVertexConstantFlags[0] = 138;
    ulPixelConstantFlags[0] = 0;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[1].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[1].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->m_spVertexShader = spVertexShaders[49];
    spD3DPass->m_spPixelShader = spPixelShaders[44];
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    ulVertexConstantFlags[1] = 2;
    ulPixelConstantFlags[1] = 0;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[2].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[2].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->m_spVertexShader = spVertexShaders[50];
    spD3DPass->m_spPixelShader = spPixelShaders[45];
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    ulVertexConstantFlags[2] = 36;
    ulPixelConstantFlags[2] = 0;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[3].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[3].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->m_spVertexShader = spVertexShaders[51];
    spD3DPass->m_spPixelShader = spPixelShaders[44];
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    ulVertexConstantFlags[3] = 2;
    ulPixelConstantFlags[3] = 0;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[4].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[4].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->m_spVertexShader = spVertexShaders[52];
    spD3DPass->m_spPixelShader = spPixelShaders[45];
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    ulVertexConstantFlags[4] = 36;
    ulPixelConstantFlags[4] = 0;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[5].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[5].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->m_spVertexShader = spVertexShaders[53];
    spD3DPass->m_spPixelShader = spPixelShaders[46];
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ALPHATESTENABLE, 1);
    spD3DPass->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    spD3DPass->SetRenderState(D3DRS_ALPHAREF, 0);
    float fDoubleDepthBias = fDepthBias + fDepthBias;
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDoubleDepthBias));
    ulVertexConstantFlags[5] = 2;
    ulPixelConstantFlags[5] = 0;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[6].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[6].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->m_spVertexShader = spVertexShaders[54];
    spD3DPass->m_spPixelShader = spPixelShaders[47];
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ALPHATESTENABLE, 1);
    spD3DPass->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    spD3DPass->SetRenderState(D3DRS_ALPHAREF, 0);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDoubleDepthBias));
    ulVertexConstantFlags[6] = 36;
    ulPixelConstantFlags[6] = 0;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[89].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[89].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->m_spVertexShader = spVertexShaders[55];
    spD3DPass->m_spPixelShader = spPixelShaders[48];
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    ulVertexConstantFlags[89] = 2;
    ulPixelConstantFlags[89] = 18;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[90].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[90].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->m_spVertexShader = spVertexShaders[51];
    spD3DPass->m_spPixelShader = spPixelShaders[48];
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    ulVertexConstantFlags[90] = 2;
    ulPixelConstantFlags[90] = 18;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[91].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[91].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->m_spVertexShader = spVertexShaders[56];
    spD3DPass->m_spPixelShader = spPixelShaders[49];
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    ulVertexConstantFlags[91] = 36;
    ulPixelConstantFlags[91] = 18;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[92].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[92].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->m_spVertexShader = spVertexShaders[52];
    spD3DPass->m_spPixelShader = spPixelShaders[49];
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    ulVertexConstantFlags[92] = 36;
    ulPixelConstantFlags[92] = 18;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[7].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[7].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 3);
    spD3DPass->m_spVertexShader = spVertexShaders[0];
    spD3DPass->m_spPixelShader = spPixelShaders[0];
    ulVertexConstantFlags[7] = 0x10002;
    ulPixelConstantFlags[7] = 1042;
    spD3DPass = spPass[8];
    CreateDefaultStages(spD3DPass, 3);
    spD3DPass->m_spVertexShader = spVertexShaders[2];
    spD3DPass->m_spPixelShader = spPixelShaders[0];
    ulVertexConstantFlags[8] = 0x10024;
    ulPixelConstantFlags[8] = 1042;
    spD3DPass = spPass[9];
    CreateDefaultStages(spD3DPass, 51);
    spD3DPass->m_spVertexShader = spVertexShaders[3];
    spD3DPass->m_spPixelShader = spPixelShaders[2];
    ulVertexConstantFlags[9] = 0x10002;
    ulPixelConstantFlags[9] = 1054;
    spD3DPass = spPass[10];
    CreateDefaultStages(spD3DPass, 19);
    spD3DPass->m_spVertexShader = spVertexShaders[0];
    spD3DPass->m_spPixelShader = spPixelShaders[4];
    ulVertexConstantFlags[10] = 0x10002;
    ulPixelConstantFlags[10] = 1046;
    spD3DPass = spPass[11];
    CreateDefaultStages(spD3DPass, 19);
    spD3DPass->m_spVertexShader = spVertexShaders[1];
    spD3DPass->m_spPixelShader = spPixelShaders[5];
    ulVertexConstantFlags[11] = 0x10002;
    ulPixelConstantFlags[11] = 1174;
    spD3DPass = spPass[12];
    CreateDefaultStages(spD3DPass, 51);
    spD3DPass->m_spVertexShader = spVertexShaders[0];
    spD3DPass->SetPixelShader(spPixelShaders[8]);
    ulVertexConstantFlags[12] = 0x10002;
    ulPixelConstantFlags[12] = 1046;
    spD3DPass = spPass[13];
    CreateDefaultStages(spD3DPass, 115);
    spD3DPass->SetVertexShader(spVertexShaders[2]);
    spD3DPass->SetPixelShader(spPixelShaders[8]);
    ulVertexConstantFlags[13] = 0x10024;
    ulPixelConstantFlags[13] = 1046;
    spD3DPass = spPass[14];
    CreateDefaultStages(spD3DPass, 67);
    spD3DPass->SetVertexShader(spVertexShaders[0]);
    spD3DPass->SetPixelShader(spPixelShaders[10]);
    ulVertexConstantFlags[14] = 0x10002;
    ulPixelConstantFlags[14] = 1074;
    spD3DPass = spPass[15];
    CreateDefaultStages(spD3DPass, 67);
    spD3DPass->SetVertexShader(spVertexShaders[2]);
    spD3DPass->SetPixelShader(spPixelShaders[10]);
    ulVertexConstantFlags[15] = 0x10024;
    ulPixelConstantFlags[15] = 1074;
    spD3DPass = spPass[16];
    CreateDefaultStages(spD3DPass, 3);
    spD3DPass->SetVertexShader(spVertexShaders[3]);
    spD3DPass->SetPixelShader(spPixelShaders[0]);
    ulVertexConstantFlags[16] = 0x10002;
    ulPixelConstantFlags[16] = 1042;
    spD3DPass = spPass[17];
    CreateDefaultStages(spD3DPass, 3);
    spD3DPass->SetVertexShader(spVertexShaders[5]);
    spD3DPass->SetPixelShader(spPixelShaders[0]);
    ulVertexConstantFlags[17] = 0x10024;
    ulPixelConstantFlags[17] = 1042;
    spD3DPass = spPass[18];
    CreateDefaultStages(spD3DPass, 19);
    spD3DPass->SetVertexShader(spVertexShaders[3]);
    spD3DPass->SetPixelShader(spPixelShaders[4]);
    ulVertexConstantFlags[18] = 0x1000A;
    ulPixelConstantFlags[18] = 1046;
    spD3DPass = spPass[19];
    CreateDefaultStages(spD3DPass, 19);
    spD3DPass->SetVertexShader(spVertexShaders[4]);
    spD3DPass->SetPixelShader(spPixelShaders[5]);
    ulVertexConstantFlags[19] = 0x1000A;
    ulPixelConstantFlags[19] = 1174;
    spD3DPass = spPass[20];
    CreateDefaultStages(spD3DPass, 67);
    spD3DPass->SetVertexShader(spVertexShaders[3]);
    spD3DPass->SetPixelShader(spPixelShaders[10]);
    ulVertexConstantFlags[20] = 0x10002;
    ulPixelConstantFlags[20] = 1074;
    spD3DPass = spPass[21];
    CreateDefaultStages(spD3DPass, 67);
    spD3DPass->SetVertexShader(spVertexShaders[5]);
    spD3DPass->SetPixelShader(spPixelShaders[10]);
    ulVertexConstantFlags[21] = 0x10024;
    ulPixelConstantFlags[21] = 1074;
    spD3DPass = spPass[22];
    CreateDefaultStages(spD3DPass, 3);
    spD3DPass->SetVertexShader(spVertexShaders[0]);
    spD3DPass->SetPixelShader(spPixelShaders[1]);
    ulVertexConstantFlags[22] = 0x10002;
    ulPixelConstantFlags[22] = 1046;
    spD3DPass = spPass[23];
    CreateDefaultStages(spD3DPass, 3);
    spD3DPass->SetVertexShader(spVertexShaders[2]);
    spD3DPass->SetPixelShader(spPixelShaders[1]);
    ulVertexConstantFlags[23] = 0x10024;
    ulPixelConstantFlags[23] = 1046;
    spD3DPass = spPass[24];
    CreateDefaultStages(spD3DPass, 51);
    spD3DPass->SetVertexShader(spVertexShaders[3]);
    spD3DPass->SetPixelShader(spPixelShaders[3]);
    ulVertexConstantFlags[24] = 0x10002;
    ulPixelConstantFlags[24] = 1054;
    spD3DPass = spPass[25];
    CreateDefaultStages(spD3DPass, 19);
    spD3DPass->SetVertexShader(spVertexShaders[0]);
    spD3DPass->SetPixelShader(spPixelShaders[6]);
    ulVertexConstantFlags[25] = 0x10002;
    ulPixelConstantFlags[25] = 1046;
    spD3DPass = spPass[26];
    CreateDefaultStages(spD3DPass, 19);
    spD3DPass->SetVertexShader(spVertexShaders[1]);
    spD3DPass->SetPixelShader(spPixelShaders[7]);
    ulVertexConstantFlags[26] = 0x10002;
    ulPixelConstantFlags[26] = 1174;
    spD3DPass = spPass[27];
    CreateDefaultStages(spD3DPass, 51);
    spD3DPass->SetVertexShader(spVertexShaders[0]);
    spD3DPass->SetPixelShader(spPixelShaders[9]);
    ulVertexConstantFlags[27] = 0x10002;
    ulPixelConstantFlags[27] = 1046;
    spD3DPass = spPass[28];
    CreateDefaultStages(spD3DPass, 115);
    spD3DPass->SetVertexShader(spVertexShaders[2]);
    spD3DPass->SetPixelShader(spPixelShaders[9]);
    ulVertexConstantFlags[28] = 0x10024;
    ulPixelConstantFlags[28] = 1046;
    spD3DPass = spPass[29];
    CreateDefaultStages(spD3DPass, 67);
    spD3DPass->SetVertexShader(spVertexShaders[0]);
    spD3DPass->SetPixelShader(spPixelShaders[11]);
    ulVertexConstantFlags[29] = 0x10002;
    ulPixelConstantFlags[29] = 1078;
    spD3DPass = spPass[30];
    CreateDefaultStages(spD3DPass, 67);
    spD3DPass->SetVertexShader(spVertexShaders[2]);
    spD3DPass->SetPixelShader(spPixelShaders[11]);
    ulVertexConstantFlags[30] = 0x10024;
    ulPixelConstantFlags[30] = 1078;
    spD3DPass = spPass[31];
    CreateDefaultStages(spD3DPass, 3);
    spD3DPass->SetVertexShader(spVertexShaders[3]);
    spD3DPass->SetPixelShader(spPixelShaders[1]);
    ulVertexConstantFlags[31] = 0x10002;
    ulPixelConstantFlags[31] = 1046;
    spD3DPass = spPass[32];
    CreateDefaultStages(spD3DPass, 3);
    spD3DPass->SetVertexShader(spVertexShaders[5]);
    spD3DPass->SetPixelShader(spPixelShaders[1]);
    ulVertexConstantFlags[32] = 0x10024;
    ulPixelConstantFlags[32] = 1046;
    spD3DPass = spPass[33];
    CreateDefaultStages(spD3DPass, 19);
    spD3DPass->SetVertexShader(spVertexShaders[3]);
    spD3DPass->SetPixelShader(spPixelShaders[6]);
    ulVertexConstantFlags[33] = 0x1000A;
    ulPixelConstantFlags[33] = 1046;
    spD3DPass = spPass[34];
    CreateDefaultStages(spD3DPass, 19);
    spD3DPass->SetVertexShader(spVertexShaders[4]);
    spD3DPass->SetPixelShader(spPixelShaders[7]);
    ulVertexConstantFlags[34] = 0x1000A;
    ulPixelConstantFlags[34] = 1174;
    spD3DPass = spPass[35];
    CreateDefaultStages(spD3DPass, 67);
    spD3DPass->SetVertexShader(spVertexShaders[3]);
    spD3DPass->SetPixelShader(spPixelShaders[11]);
    ulVertexConstantFlags[35] = 0x10002;
    ulPixelConstantFlags[35] = 1078;
    spD3DPass = spPass[36];
    CreateDefaultStages(spD3DPass, 67);
    spD3DPass->SetVertexShader(spVertexShaders[5]);
    spD3DPass->SetPixelShader(spPixelShaders[11]);
    ulVertexConstantFlags[36] = 0x10024;
    ulPixelConstantFlags[36] = 1078;
    spD3DPass = spPass[37];
    CreateDefaultStages(spD3DPass, 15);
    spD3DPass->SetVertexShader(spVertexShaders[6]);
    spD3DPass->SetPixelShader(spPixelShaders[12]);
    ulVertexConstantFlags[37] = 0x14002;
    ulPixelConstantFlags[37] = 1042;
    spD3DPass = spPass[38];
    CreateDefaultStages(spD3DPass, 15);
    spD3DPass->SetVertexShader(spVertexShaders[8]);
    spD3DPass->SetPixelShader(spPixelShaders[12]);
    ulVertexConstantFlags[38] = 0x14024;
    ulPixelConstantFlags[38] = 1042;
    spD3DPass = spPass[39];
    CreateDefaultStages(spD3DPass, 63);
    spD3DPass->SetVertexShader(spVertexShaders[9]);
    spD3DPass->SetPixelShader(spPixelShaders[14]);
    ulVertexConstantFlags[39] = 0x14002;
    ulPixelConstantFlags[39] = 1054;
    spD3DPass = spPass[40];
    CreateDefaultStages(spD3DPass, 31);
    spD3DPass->SetVertexShader(spVertexShaders[6]);
    spD3DPass->SetPixelShader(spPixelShaders[16]);
    ulVertexConstantFlags[40] = 0x14002;
    ulPixelConstantFlags[40] = 1046;
    spD3DPass = spPass[41];
    CreateDefaultStages(spD3DPass, 31);
    spD3DPass->SetVertexShader(spVertexShaders[7]);
    spD3DPass->SetPixelShader(spPixelShaders[17]);
    ulVertexConstantFlags[41] = 0x14002;
    ulPixelConstantFlags[41] = 1174;
    spD3DPass = spPass[42];
    CreateDefaultStages(spD3DPass, 63);
    spD3DPass->SetVertexShader(spVertexShaders[6]);
    spD3DPass->SetPixelShader(spPixelShaders[20]);
    ulVertexConstantFlags[42] = 0x14002;
    ulPixelConstantFlags[42] = 1046;
    spD3DPass = spPass[43];
    CreateDefaultStages(spD3DPass, 127);
    spD3DPass->SetVertexShader(spVertexShaders[8]);
    spD3DPass->SetPixelShader(spPixelShaders[20]);
    ulVertexConstantFlags[43] = 0x14024;
    ulPixelConstantFlags[43] = 1046;
    spD3DPass = spPass[44];
    CreateDefaultStages(spD3DPass, 79);
    spD3DPass->SetVertexShader(spVertexShaders[6]);
    spD3DPass->SetPixelShader(spPixelShaders[22]);
    ulVertexConstantFlags[44] = 0x14002;
    ulPixelConstantFlags[44] = 1074;
    spD3DPass = spPass[45];
    CreateDefaultStages(spD3DPass, 79);
    spD3DPass->SetVertexShader(spVertexShaders[8]);
    spD3DPass->SetPixelShader(spPixelShaders[22]);
    ulVertexConstantFlags[45] = 0x14024;
    ulPixelConstantFlags[45] = 1074;
    spD3DPass = spPass[46];
    CreateDefaultStages(spD3DPass, 15);
    spD3DPass->SetVertexShader(spVertexShaders[9]);
    spD3DPass->SetPixelShader(spPixelShaders[12]);
    ulVertexConstantFlags[46] = 0x14002;
    ulPixelConstantFlags[46] = 1042;
    spD3DPass = spPass[47];
    CreateDefaultStages(spD3DPass, 15);
    spD3DPass->SetVertexShader(spVertexShaders[11]);
    spD3DPass->SetPixelShader(spPixelShaders[12]);
    ulVertexConstantFlags[47] = 0x14024;
    ulPixelConstantFlags[47] = 1042;
    spD3DPass = spPass[48];
    CreateDefaultStages(spD3DPass, 31);
    spD3DPass->SetVertexShader(spVertexShaders[9]);
    spD3DPass->SetPixelShader(spPixelShaders[16]);
    ulVertexConstantFlags[48] = 0x14002;
    ulPixelConstantFlags[48] = 1046;
    spD3DPass = spPass[49];
    CreateDefaultStages(spD3DPass, 31);
    spD3DPass->SetVertexShader(spVertexShaders[10]);
    spD3DPass->SetPixelShader(spPixelShaders[17]);
    ulVertexConstantFlags[49] = 0x14002;
    ulPixelConstantFlags[49] = 1174;
    spD3DPass = spPass[50];
    CreateDefaultStages(spD3DPass, 79);
    spD3DPass->SetVertexShader(spVertexShaders[9]);
    spD3DPass->SetPixelShader(spPixelShaders[22]);
    ulVertexConstantFlags[50] = 0x14002;
    ulPixelConstantFlags[50] = 1074;
    spD3DPass = spPass[51];
    CreateDefaultStages(spD3DPass, 79);
    spD3DPass->SetVertexShader(spVertexShaders[11]);
    spD3DPass->SetPixelShader(spPixelShaders[22]);
    ulVertexConstantFlags[51] = 0x14024;
    ulPixelConstantFlags[51] = 1074;
    spD3DPass = spPass[52];
    CreateDefaultStages(spD3DPass, 15);
    spD3DPass->SetVertexShader(spVertexShaders[6]);
    spD3DPass->SetPixelShader(spPixelShaders[13]);
    ulVertexConstantFlags[52] = 0x14002;
    ulPixelConstantFlags[52] = 1046;
    spD3DPass = spPass[53];
    CreateDefaultStages(spD3DPass, 15);
    spD3DPass->SetVertexShader(spVertexShaders[8]);
    spD3DPass->SetPixelShader(spPixelShaders[13]);
    ulVertexConstantFlags[53] = 0x14024;
    ulPixelConstantFlags[53] = 1046;
    spD3DPass = spPass[54];
    CreateDefaultStages(spD3DPass, 63);
    spD3DPass->SetVertexShader(spVertexShaders[9]);
    spD3DPass->SetPixelShader(spPixelShaders[15]);
    ulVertexConstantFlags[54] = 0x14002;
    ulPixelConstantFlags[54] = 1054;
    spD3DPass = spPass[55];
    CreateDefaultStages(spD3DPass, 31);
    spD3DPass->SetVertexShader(spVertexShaders[6]);
    spD3DPass->SetPixelShader(spPixelShaders[18]);
    ulVertexConstantFlags[55] = 0x14002;
    ulPixelConstantFlags[55] = 1046;
    spD3DPass = spPass[56];
    CreateDefaultStages(spD3DPass, 31);
    spD3DPass->SetVertexShader(spVertexShaders[7]);
    spD3DPass->SetPixelShader(spPixelShaders[19]);
    ulVertexConstantFlags[56] = 0x14002;
    ulPixelConstantFlags[56] = 1174;
    spD3DPass = spPass[57];
    CreateDefaultStages(spD3DPass, 63);
    spD3DPass->SetVertexShader(spVertexShaders[6]);
    spD3DPass->SetPixelShader(spPixelShaders[21]);
    ulVertexConstantFlags[57] = 0x14002;
    ulPixelConstantFlags[57] = 1046;
    spD3DPass = spPass[58];
    CreateDefaultStages(spD3DPass, 127);
    spD3DPass->SetVertexShader(spVertexShaders[8]);
    spD3DPass->SetPixelShader(spPixelShaders[21]);
    ulVertexConstantFlags[58] = 0x14024;
    ulPixelConstantFlags[58] = 1046;
    spD3DPass = spPass[59];
    CreateDefaultStages(spD3DPass, 79);
    spD3DPass->SetVertexShader(spVertexShaders[6]);
    spD3DPass->SetPixelShader(spPixelShaders[23]);
    ulVertexConstantFlags[59] = 0x14002;
    ulPixelConstantFlags[59] = 1078;
    spD3DPass = spPass[60];
    CreateDefaultStages(spD3DPass, 79);
    spD3DPass->SetVertexShader(spVertexShaders[8]);
    spD3DPass->SetPixelShader(spPixelShaders[23]);
    ulVertexConstantFlags[60] = 0x14024;
    ulPixelConstantFlags[60] = 1078;
    spD3DPass = spPass[61];
    CreateDefaultStages(spD3DPass, 15);
    spD3DPass->SetVertexShader(spVertexShaders[9]);
    spD3DPass->SetPixelShader(spPixelShaders[13]);
    ulVertexConstantFlags[61] = 0x14002;
    ulPixelConstantFlags[61] = 1046;
    spD3DPass = spPass[62];
    CreateDefaultStages(spD3DPass, 15);
    spD3DPass->SetVertexShader(spVertexShaders[11]);
    spD3DPass->SetPixelShader(spPixelShaders[13]);
    ulVertexConstantFlags[62] = 0x14024;
    ulPixelConstantFlags[62] = 1046;
    spD3DPass = spPass[63];
    CreateDefaultStages(spD3DPass, 31);
    spD3DPass->SetVertexShader(spVertexShaders[9]);
    spD3DPass->SetPixelShader(spPixelShaders[18]);
    ulPixelConstantFlags[63] = 1046;
    ulVertexConstantFlags[63] = 0x14002;
    spD3DPass = spPass[64];
    CreateDefaultStages(spD3DPass, 31);
    spD3DPass->SetVertexShader(spVertexShaders[10]);
    spD3DPass->SetPixelShader(spPixelShaders[19]);
    ulVertexConstantFlags[64] = 0x14002;
    ulPixelConstantFlags[64] = 1174;
    spD3DPass = spPass[65];
    CreateDefaultStages(spD3DPass, 79);
    spD3DPass->SetVertexShader(spVertexShaders[9]);
    spD3DPass->SetPixelShader(spPixelShaders[23]);
    ulVertexConstantFlags[65] = 0x14002;
    ulPixelConstantFlags[65] = 1078;
    spD3DPass = spPass[66];
    CreateDefaultStages(spD3DPass, 79);
    spD3DPass->SetVertexShader(spVertexShaders[11]);
    spD3DPass->SetPixelShader(spPixelShaders[23]);
    ulVertexConstantFlags[66] = 0x14024;
    ulPixelConstantFlags[66] = 1078;
    spD3DPass = spPass[69];
    CreateDefaultStages(spD3DPass, 11);
    spD3DPass->m_kStages.m_pBase[1].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    spD3DPass->SetVertexShader(spVertexShaders[12]);
    spD3DPass->SetPixelShader(spPixelShaders[24]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_ONE, D3DBLEND_ONE, false);
    ulVertexConstantFlags[69] = 0x30082;
    ulPixelConstantFlags[69] = 272;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[70].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[70].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 11);
    spD3DPass->m_kStages.m_pBase[1].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    spD3DPass->SetVertexShader(spVertexShaders[12]);
    spD3DPass->SetPixelShader(spPixelShaders[25]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_ONE, D3DBLEND_ONE, false);
    ulVertexConstantFlags[70] = 0x30082;
    ulPixelConstantFlags[70] = 272;
    // EXE-INLINED 0xBA53C0
    if (spD3DPass.m_pObject != spPass[71].m_pObject) {
        spD3DPass.m_pObject->DecRefCount();
        spD3DPass.m_pObject = spPass[71].m_pObject;
        if (spD3DPass.m_pObject) {
            spD3DPass.m_pObject->IncRefCount();
        }
    }
    CreateDefaultStages(spD3DPass, 11);
    spD3DPass->m_kStages.m_pBase[1].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    spD3DPass->SetVertexShader(spVertexShaders[13]);
    spD3DPass->SetPixelShader(spPixelShaders[24]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_ONE, D3DBLEND_ONE, false);
    ulVertexConstantFlags[71] = 0x30082;
    ulPixelConstantFlags[71] = 272;
    spD3DPass.SetPointer_(&spPass[72]);
    CreateDefaultStages(spD3DPass, 11);
    spD3DPass->m_kStages.m_pBase[1].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    spD3DPass->SetVertexShader(spVertexShaders[13]);
    spD3DPass->SetPixelShader(spPixelShaders[25]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_ONE, D3DBLEND_ONE, false);
    ulVertexConstantFlags[72] = 0x30082;
    ulPixelConstantFlags[72] = 272;
    spD3DPass.SetPointer_(&spPass[73]);
    CreateDefaultStages(spD3DPass, 11);
    spD3DPass->m_kStages.m_pBase[1].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    spD3DPass->SetVertexShader(spVertexShaders[14]);
    spD3DPass->SetPixelShader(spPixelShaders[24]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_ONE, D3DBLEND_ONE, false);
    ulVertexConstantFlags[73] = 0x38024;
    ulPixelConstantFlags[73] = 272;
    spD3DPass.SetPointer_(&spPass[74]);
    CreateDefaultStages(spD3DPass, 11);
    spD3DPass->m_kStages.m_pBase[1].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    spD3DPass->SetVertexShader(spVertexShaders[15]);
    spD3DPass->SetPixelShader(spPixelShaders[24]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_ONE, D3DBLEND_ONE, false);
    ulVertexConstantFlags[74] = 0x38024;
    ulPixelConstantFlags[74] = 272;
    spD3DPass.SetPointer_(&spPass[75]);
    CreateDefaultStages(spD3DPass, 11);
    spD3DPass->m_kStages.m_pBase[1].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    spD3DPass->SetVertexShader(spVertexShaders[16]);
    spD3DPass->SetPixelShader(spPixelShaders[26]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_ONE, D3DBLEND_ONE, false);
    ulVertexConstantFlags[75] = 0x32082;
    ulPixelConstantFlags[75] = 1296;
    spD3DPass.SetPointer_(&spPass[104]);
    spD3DPass->SetVertexShader(spVertexShaders[17]);
    spD3DPass->SetPixelShader(spPixelShaders[27]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[104] = 138;
    ulPixelConstantFlags[104] = 512;
    spD3DPass.SetPointer_(&spPass[105]);
    spD3DPass->SetVertexShader(spVertexShaders[18]);
    spD3DPass->SetPixelShader(spPixelShaders[27]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[105] = 52;
    ulPixelConstantFlags[105] = 512;
    spD3DPass.SetPointer_(&spPass[106]);
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->SetVertexShader(spVertexShaders[19]);
    spD3DPass->SetPixelShader(spPixelShaders[28]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[106] = 138;
    ulPixelConstantFlags[106] = 512;
    spD3DPass.SetPointer_(&spPass[107]);
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->SetVertexShader(spVertexShaders[20]);
    spD3DPass->SetPixelShader(spPixelShaders[28]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[107] = 52;
    ulPixelConstantFlags[107] = 512;
    spD3DPass.SetPointer_(&spPass[108]);
    spD3DPass->SetVertexShader(spVertexShaders[21]);
    spD3DPass->SetPixelShader(spPixelShaders[27]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[108] = 138;
    ulPixelConstantFlags[108] = 512;
    spD3DPass.SetPointer_(&spPass[109]);
    spD3DPass->SetVertexShader(spVertexShaders[22]);
    spD3DPass->SetPixelShader(spPixelShaders[27]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[109] = 52;
    ulPixelConstantFlags[109] = 512;
    spD3DPass.SetPointer_(&spPass[110]);
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->SetVertexShader(spVertexShaders[23]);
    spD3DPass->SetPixelShader(spPixelShaders[28]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[110] = 138;
    ulPixelConstantFlags[110] = 512;
    spD3DPass.SetPointer_(&spPass[111]);
    CreateDefaultStages(spD3DPass, 1);
    spD3DPass->SetVertexShader(spVertexShaders[24]);
    spD3DPass->SetPixelShader(spPixelShaders[28]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 3);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[111] = 52;
    ulPixelConstantFlags[111] = 512;
    spD3DPass.SetPointer_(&spPass[76]);
    CreateDefaultStages(spD3DPass, 5);
    spD3DPass->m_kStages.m_pBase[2].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF4 ||
        BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF9) {
        spD3DPass->m_kStages.m_pBase[2].m_pObject->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
    }
    spD3DPass->SetVertexShader(spVertexShaders[25]);
    spD3DPass->SetPixelShader(spPixelShaders[29]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, false);
    spD3DPass->SetRenderState(D3DRS_CLIPPLANEENABLE, 63);
    ulVertexConstantFlags[76] = 0x10082;
    ulPixelConstantFlags[76] = 1040;
    spD3DPass.SetPointer_(&spPass[77]);
    CreateDefaultStages(spD3DPass, 5);
    spD3DPass->m_kStages.m_pBase[2].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF4 ||
        BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF9) {
        spD3DPass->m_kStages.m_pBase[2].m_pObject->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
    }
    spD3DPass->SetVertexShader(spVertexShaders[26]);
    spD3DPass->SetPixelShader(spPixelShaders[29]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, false);
    spD3DPass->SetRenderState(D3DRS_CLIPPLANEENABLE, 63);
    ulVertexConstantFlags[77] = 0x10082;
    ulPixelConstantFlags[77] = 1040;
    spD3DPass.SetPointer_(&spPass[78]);
    CreateDefaultStages(spD3DPass, 5);
    spD3DPass->m_kStages.m_pBase[2].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF4 ||
        BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF9) {
        spD3DPass->m_kStages.m_pBase[2].m_pObject->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
    }
    spD3DPass->SetVertexShader(spVertexShaders[27]);
    spD3DPass->SetPixelShader(spPixelShaders[29]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, false);
    spD3DPass->SetRenderState(D3DRS_CLIPPLANEENABLE, 63);
    ulVertexConstantFlags[78] = 0x100A4;
    ulPixelConstantFlags[78] = 1040;
    spD3DPass.SetPointer_(&spPass[79]);
    // Only the low byte of the stage mask is kept.
    uint8_t ucStageFlags = C06DC0(1, 0, 1, 0, 0, 0, 0, 0);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->m_kStages.m_pBase[2].m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF4 ||
        BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_PCF9) {
        spD3DPass->m_kStages.m_pBase[2].m_pObject->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
    }
    spD3DPass->SetVertexShader(spVertexShaders[28]);
    spD3DPass->SetPixelShader(spPixelShaders[29]);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, false);
    spD3DPass->SetRenderState(D3DRS_CLIPPLANEENABLE, 63);
    ulVertexConstantFlags[79] = 0x100A4;
    ulPixelConstantFlags[79] = 1040;
    spD3DPass.SetPointer_(&spPass[84]);
    ucStageFlags = C06DC0(1, 0, 0, 0, 0, 0, 0, 0);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[33]);
    spD3DPass->SetPixelShader(spPixelShaders[32]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    ulVertexConstantFlags[84] = 10;
    ulPixelConstantFlags[84] = 64;
    spD3DPass.SetPointer_(&spPass[85]);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[34]);
    spD3DPass->SetPixelShader(spPixelShaders[33]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    ulVertexConstantFlags[85] = 52;
    ulPixelConstantFlags[85] = 64;
    spD3DPass.SetPointer_(&spPass[86]);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[35]);
    spD3DPass->SetPixelShader(spPixelShaders[34]);
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, false);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    ulVertexConstantFlags[86] = 0x140A;
    ulPixelConstantFlags[86] = 64;
    spD3DPass.SetPointer_(&spPass[87]);
    spD3DPass->SetVertexShader(spVertexShaders[36]);
    spD3DPass->SetPixelShader(spPixelShaders[35]);
    spD3DPass->SetRenderState(D3DRS_ZENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    ulVertexConstantFlags[87] = 2;
    ulPixelConstantFlags[87] = 0;
    spD3DPass.SetPointer_(&spPass[88]);
    spD3DPass->SetVertexShader(spVertexShaders[37]);
    spD3DPass->SetPixelShader(spPixelShaders[36]);
    spD3DPass->SetRenderState(D3DRS_ZENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    ulVertexConstantFlags[88] = 36;
    ulPixelConstantFlags[88] = 0;
    spD3DPass.SetPointer_(&spPass[67]);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[0]);
    spD3DPass->SetPixelShader(spPixelShaders[37]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    ulVertexConstantFlags[67] = 0x10002;
    ulPixelConstantFlags[67] = 18;
    spD3DPass.SetPointer_(&spPass[68]);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[3]);
    spD3DPass->SetPixelShader(spPixelShaders[37]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    ulVertexConstantFlags[68] = 0x10002;
    ulPixelConstantFlags[68] = 18;
    spD3DPass.SetPointer_(&spPass[93]);
    spD3DPass->SetVertexShader(spVertexShaders[38]);
    spD3DPass->SetPixelShader(spPixelShaders[38]);
    ulVertexConstantFlags[93] = 2058;
    ulPixelConstantFlags[93] = 0;
    spD3DPass.SetPointer_(&spPass[94]);
    spD3DPass->SetVertexShader(spVertexShaders[39]);
    spD3DPass->SetPixelShader(spPixelShaders[38]);
    ulVertexConstantFlags[94] = 2058;
    ulPixelConstantFlags[94] = 0;
    spD3DPass.SetPointer_(&spPass[95]);
    spD3DPass->SetVertexShader(spVertexShaders[40]);
    spD3DPass->SetPixelShader(spPixelShaders[38]);
    ulVertexConstantFlags[95] = 2100;
    ulPixelConstantFlags[95] = 0;
    spD3DPass.SetPointer_(&spPass[96]);
    spD3DPass->SetVertexShader(spVertexShaders[41]);
    spD3DPass->SetPixelShader(spPixelShaders[38]);
    ulVertexConstantFlags[96] = 2100;
    ulPixelConstantFlags[96] = 0;
    spD3DPass.SetPointer_(&spPass[97]);
    spD3DPass->SetVertexShader(spVertexShaders[42]);
    spD3DPass->SetPixelShader(spPixelShaders[39]);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    spD3DPass->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    ulVertexConstantFlags[97] = 2826;
    ulPixelConstantFlags[97] = 0;
    spD3DPass.SetPointer_(&spPass[98]);
    spD3DPass->SetVertexShader(spVertexShaders[43]);
    spD3DPass->SetPixelShader(spPixelShaders[39]);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    spD3DPass->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    ulVertexConstantFlags[98] = 2932;
    ulPixelConstantFlags[98] = 0;
    spD3DPass.SetPointer_(&spPass[102]);
    ucStageFlags = C06DC0(1, 1, 0, 0, 0, 0, 0, 0);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[47]);
    spD3DPass->SetPixelShader(spPixelShaders[42]);
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, false);
    spD3DPass->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[102] = 0;
    ulPixelConstantFlags[102] = 0;
    spD3DPass.SetPointer_(&spPass[103]);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[48]);
    spD3DPass->SetPixelShader(spPixelShaders[43]);
    spD3DPass->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    spD3DPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    spD3DPass->SetSrcDestBlendRenderState(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, false);
    spD3DPass->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    spD3DPass->SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(fDepthBias));
    ulVertexConstantFlags[103] = 0;
    ulPixelConstantFlags[103] = 0;
    spD3DPass.SetPointer_(&spPass[80]);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[29]);
    spD3DPass->SetPixelShader(spPixelShaders[30]);
    ulVertexConstantFlags[80] = 10;
    ulPixelConstantFlags[80] = 1024;
    spD3DPass.SetPointer_(&spPass[81]);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[30]);
    spD3DPass->SetPixelShader(spPixelShaders[30]);
    ulVertexConstantFlags[81] = 10;
    ulPixelConstantFlags[81] = 1024;
    spD3DPass.SetPointer_(&spPass[82]);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[31]);
    spD3DPass->SetPixelShader(spPixelShaders[30]);
    ulVertexConstantFlags[82] = 52;
    ulPixelConstantFlags[82] = 1024;
    spD3DPass.SetPointer_(&spPass[83]);
    CreateDefaultStages(spD3DPass, ucStageFlags);
    spD3DPass->SetVertexShader(spVertexShaders[32]);
    spD3DPass->SetPixelShader(spPixelShaders[30]);
    ulVertexConstantFlags[83] = 52;
    ulPixelConstantFlags[83] = 1024;
    spD3DPass.SetPointer_(&spPass[99]);
    spD3DPass->SetVertexShader(spVertexShaders[44]);
    spD3DPass->SetPixelShader(spPixelShaders[40]);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    ulVertexConstantFlags[99] = 10;
    ulPixelConstantFlags[99] = 0;
    spD3DPass.SetPointer_(&spPass[100]);
    spD3DPass->SetVertexShader(spVertexShaders[45]);
    spD3DPass->SetPixelShader(spPixelShaders[40]);
    ulVertexConstantFlags[100] = 52;
    ulPixelConstantFlags[100] = 0;
    spD3DPass.SetPointer_(&spPass[101]);
    spD3DPass->SetVertexShader(spVertexShaders[46]);
    spD3DPass->SetPixelShader(spPixelShaders[41]);
    spD3DPass->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    spD3DPass->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    ulVertexConstantFlags[101] = 2;
    ulPixelConstantFlags[101] = 0;
    BuildPerPassConstantMaps();
    // EXE-INLINED 0xB79930
    spD3DPass.m_pObject->DecRefCount();
}

// The local NiPointer<NiD3DPass> is written out by hand, using the pass's own reference count.
void Lighting30Shader::BuildPerPassConstantMaps() {
    NiD3DPass* pPass = nullptr;
    for (int32_t iPass = 0; iPass < 112; ++iPass) {
        // EXE-INLINED 0xBA53C0
        if (pPass != spPass[iPass].m_pObject) {
            if (pPass) {
                pPass->DecRefCount();
            }
            pPass = spPass[iPass].m_pObject;
            if (pPass) {
                pPass->IncRefCount();
            }
        }
        // The texture effect passes take the shared maps below.
        if (iPass == 102 || iPass == 103) {
            continue;
        }
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pVertexMap;
        if (pMemory) {
            pVertexMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        }
        else {
            pVertexMap = nullptr;
        }
        pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pPixelMap;
        if (pMemory) {
            pPixelMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        }
        else {
            pPixelMap = nullptr;
        }
        // The bit is shifted left before each test, so entry j is selected by bit j + 1; it never wraps.
        uint32_t uiBit = 1;
        for (uint32_t i = 0; i < 17; ++i) {
            uiBit <<= 1;
            if ((ulVertexConstantFlags[iPass] & uiBit) != 0) {
                NiShaderConstantMapEntry* pEntry = pVertexConstantMapEntry[i];
                if (pEntry) {
                    if ((pEntry->m_uiFlags & NiShaderConstantMapEntry::SCME_MAP_MASK) ==
                        NiShaderConstantMapEntry::SCME_MAP_DEFINED) {
                        pVertexMap->InsertDefinedEntry(pEntry);
                    }
                    else {
                        pVertexMap->InsertEntry(pEntry);
                    }
                }
            }
        }
        // Walks the 10 pixel entries.
        uiBit = 1;
        for (uint32_t i = 0; i < 10; ++i) {
            uiBit <<= 1;
            if ((ulPixelConstantFlags[iPass] & uiBit) != 0) {
                NiShaderConstantMapEntry* pEntry = pPixelConstantMapEntry[i];
                if (pEntry) {
                    if ((pEntry->m_uiFlags & NiShaderConstantMapEntry::SCME_MAP_MASK) ==
                        NiShaderConstantMapEntry::SCME_MAP_DEFINED) {
                        pPixelMap->InsertDefinedEntry(pEntry);
                    }
                    else {
                        pPixelMap->InsertEntry(pEntry);
                    }
                }
            }
        }
        // SWEEP-INLINE 0xA5F860
        pPass->m_spVertexConstantMap = pVertexMap;
        // SWEEP-INLINE 0xA5F860
        pPass->m_spPixelConstantMap = pPixelMap;
    }
    // SWEEP-INLINE 0xA5F860
    spPass[102].m_pObject->m_spVertexConstantMap = spTexEffectVConstantMap.m_pObject;
    // SWEEP-INLINE 0xA5F860
    spPass[102].m_pObject->m_spPixelConstantMap = spTexEffectPConstantMap.m_pObject;
    // SWEEP-INLINE 0xA5F860
    spPass[103].m_pObject->m_spVertexConstantMap = spTexEffectVConstantMap.m_pObject;
    // SWEEP-INLINE 0xA5F860
    spPass[103].m_pObject->m_spPixelConstantMap = spTexEffectPConstantMap.m_pObject;
    if (pPass) {
        pPass->DecRefCount();
    }
}

// Only Lighting30Shader::PresetStages calls it.
int32_t Lighting30Shader::C06DC0(
    int8_t acBit0,
    int8_t acBit1,
    int8_t acBit2,
    int8_t acBit3,
    int8_t acBit4,
    int8_t acBit5,
    int8_t acBit6,
    int8_t acBit7
) {
    return acBit0 |
        2 * (acBit1 | 2 * (acBit2 | 2 * (acBit3 | 2 * (acBit4 | 2 * (acBit5 | 2 * (acBit6 | 2 * acBit7))))));
}
