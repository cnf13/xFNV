#include "BSMain/BSTestObjects.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "SDK/MSVC/string.h"

#include <cmath>

// The box's 36 triangle indices. Both CreateBox overloads that copy it read this one table.
using BoxIndexArray = const uint16_t[36];
static BoxIndexArray Box{4, 6, 0, 0, 6, 2, 1, 3, 5, 5, 3, 7, 5, 7, 4, 4, 7, 6,
                         0, 2, 1, 1, 2, 3, 5, 4, 1, 1, 4, 0, 6, 7, 2, 2, 7, 3};

// The arrow head's 24 triangle indices used by Create3DArrow.
using HeadIndexArray = const uint16_t[24];
static HeadIndexArray Head{4, 0, 5, 5, 0, 2, 2, 0, 1, 2, 1, 3, 3, 1, 4, 3, 4, 5, 1, 0, 4, 3, 5, 2};

// The unit box's corners used by CreateBox, filled on the first call (its guard is CreateBox_guards, bit 0).
using BoxPointArray = NiPoint3[8];
static BoxPointArray BoxPoints{};

const NiColorA& BSTestObjects::GetColor(uint32_t aiIndex) {
    return BSTestObjects::pColorA[aiIndex % 10];
}

NiAVObject* BSTestObjects::CreateDirArrow(const NiPoint3& arDir, const NiColorA* apColor) {
    NiLines* lines;
    NiColorA* color_2;
    NiPoint3* vertex;
    NiPoint3 dir_2;
    NiPoint3 v70;
    NiPoint3 v71;
    dir_2 = arDir;
    float flength = dir_2.Length();
    if (flength < 8.0) {
        NiPoint3 v72;
        float v21 = 8.0 / flength;
        v72.x = dir_2.x * v21;
        v72.y = dir_2.y * v21;
        v72.z = dir_2.z * v21;
        dir_2 = v72;
        flength = dir_2.Length();
    }
    NiPoint3* v53 = new NiPoint3[5];
    if (v53) {
        vertex = v53;
    }
    else {
        vertex = nullptr;
    }
    float v51 = 0.0f;
    float v52 = 0.0f;
    float v14 = flength * 0.15000000596046448;
    vertex[0].x = 0.0f;
    vertex[0].y = v51;
    vertex[0].z = v52;
    NiPoint3 apResult = dir_2.Perpendicular();
    float v16 = apResult.x * v14;
    float v17 = apResult.y * v14;
    float v18 = apResult.z * v14;
    float v11 = dir_2.x * 0.75f;
    float v12 = dir_2.y * 0.75f;
    float v13 = dir_2.z * 0.75f;
    v71.x = v11 - v16;
    v71.y = v12 - v17;
    v71.z = v13 - v18;
    v70.x = v16 + v11;
    v70.y = v17 + v12;
    v70.z = v18 + v13;
    vertex[1] = dir_2;
    vertex[2] = v70;
    vertex[3] = dir_2;
    vertex[4] = v71;
    if (!apColor) {
        apColor = &BSTestObjects::RED;
    }
    NiColorA* color = new NiColorA[6];
    if (color) {
        color_2 = color;
    }
    else {
        color_2 = nullptr;
    }
    float v35 = 1.0f;
    float v36 = 0.0f;
    float v37 = 1.0f;
    float v31 = 1.0f;
    float v32 = 0.0f;
    float v33 = 1.0f;
    float v27 = 1.0f;
    float v28 = 0.0f;
    float v29 = 1.0f;
    color_2[0] = *apColor;
    color_2[1] = *apColor;
    color_2[2].r = 0.0f;
    color_2[2].g = v35;
    color_2[2].b = v36;
    color_2[2].a = v37;
    color_2[3].r = 0.0f;
    color_2[3].g = v31;
    color_2[3].b = v32;
    color_2[3].a = v33;
    color_2[4].r = 0.0f;
    color_2[4].g = v27;
    color_2[4].b = v28;
    color_2[4].a = v29;
    uint8_t* flag = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(6u));
    flag[0] = 1;
    flag[1] = 0;
    flag[2] = 1;
    flag[3] = 1;
    flag[4] = 0;
    lines = new NiLines(5, vertex, color_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, flag);
    lines->m_kLocal.m_Translate = NiPoint3::ZERO;
    lines->m_kLocal.m_Rotate = NiMatrix3::IDENTITY;
    lines->SetName("BSTestObjects DirArrow");
    return lines;
}

NiAVObject* BSTestObjects::CreateCylinder(
    float afBaseRadius,
    float afTopRadius,
    float afHeight,
    uint16_t aiSlices,
    uint16_t aiStacks,
    const NiColorA* apColor
) {
    NiTriStrips* cylinder;
    NiPoint2* textures;
    NiColorA* colors;
    NiPoint3* normals;
    NiPoint3* vertices;
    uint16_t numVerticesPerStrip = 2 * aiSlices + 2;
    uint32_t arraySize = numVerticesPerStrip * aiStacks;
    NiPoint3* v64 = new NiPoint3[arraySize];
    if (v64) {
        vertices = v64;
    }
    else {
        vertices = nullptr;
    }
    NiPoint3* v61 = new NiPoint3[arraySize];
    if (v61) {
        normals = v61;
    }
    else {
        normals = nullptr;
    }
    if (apColor) {
        NiColorA* v23;
        NiColorA* v58 = new NiColorA[arraySize];
        if (v58) {
            v23 = v58;
        }
        else {
            v23 = nullptr;
        }
        colors = v23;
    }
    else {
        colors = nullptr;
    }
    NiPoint2* v55 = new NiPoint2[arraySize];
    if (v55) {
        textures = v55;
    }
    else {
        textures = nullptr;
    }
    uint16_t* stripLengths = static_cast<uint16_t*>(MemoryManager::Instance().Allocate(
        aiStacks <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * aiStacks : SIZE_MAX
    ));
    uint16_t* stripLists = static_cast<uint16_t*>(MemoryManager::Instance().Allocate(
        arraySize <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * arraySize : SIZE_MAX
    ));
    uint16_t numVertices = 0;
    float da = 3.1415927f * 2.0 / static_cast<double>(aiSlices);
    float dr = (afTopRadius - afBaseRadius) / static_cast<double>(aiStacks);
    float dz = afHeight / static_cast<double>(aiStacks);
    float ds = 1.0 / static_cast<double>(aiSlices);
    float dt = 1.0 / static_cast<double>(aiStacks);
    float t = 1.0f;
    float z = 0.0f;
    float r = afBaseRadius;
    float nz = (afBaseRadius - afTopRadius) / afHeight;
    float nsign = 1.0f;
    for (uint32_t j_2 = 0; j_2 < aiStacks; ++j_2) {
        float s = 0.0f;
        for (uint32_t i_2 = 0; i_2 <= aiSlices; ++i_2) {
            float x;
            float y;
            if (i_2 == aiSlices) {
                x = 0.0f;
                y = 1.0f;
            }
            else {
                float v20 = da * static_cast<double>(i_2);
                float v19 = std::sin(static_cast<double>(v20));
                x = v19;
                float v18 = std::cos(static_cast<double>(v20));
                y = v18;
            }
            float v50 = r * x;
            float v51 = r * y;
            float v47 = nsign * x;
            float v48 = nsign * y;
            float v49 = nsign * nz;
            vertices[numVertices].x = v50;
            vertices[numVertices].y = v51;
            vertices[numVertices].z = z;
            normals[numVertices].x = v47;
            normals[numVertices].y = v48;
            normals[numVertices].z = v49;
            if (apColor) {
                colors[numVertices] = *apColor;
            }
            textures[numVertices].x = s;
            textures[numVertices].y = t;
            stripLists[numVertices] = numVertices;
            ++numVertices;
            float v42 = (dr + r) * x;
            float v43 = (dr + r) * y;
            float v44 = dz + z;
            float v39 = nsign * x;
            float v40 = nsign * y;
            float v41 = nsign * nz;
            float v38 = t - dt;
            vertices[numVertices].x = v42;
            vertices[numVertices].y = v43;
            vertices[numVertices].z = v44;
            normals[numVertices].x = v39;
            normals[numVertices].y = v40;
            normals[numVertices].z = v41;
            if (apColor) {
                colors[numVertices] = *apColor;
            }
            textures[numVertices].x = s;
            textures[numVertices].y = v38;
            stripLists[numVertices] = numVertices;
            ++numVertices;
            s = s + ds;
        }
        r = r + dr;
        t = t - dt;
        z = z + dz;
        stripLengths[j_2] = numVerticesPerStrip;
    }
    cylinder = new NiTriStrips(
        numVertices,
        vertices,
        normals,
        colors,
        textures,
        1,
        0,
        aiSlices * (2 * aiStacks),
        aiStacks,
        stripLengths,
        stripLists
    );
    cylinder->SetName("BSTestObjects Cylinder");
    cylinder->CalculateNormals();
    return cylinder;
}

// One triangle strip per slice: each tube step adds a vertex on this slice's ring and one on the previous slice's ring.
// Every angle, sine and cosine is rounded to float, as the game stores them.
NiAVObject* BSTestObjects::CreateTorus(
    float afInnerRadius,
    float afOuterRadius,
    uint16_t aiSlices,
    uint16_t aiStacks,
    const NiColorA* apColor
) {
    float fSliceStep = static_cast<float>(3.1415927f * 2.0 / static_cast<int32_t>(aiSlices));
    float fStackStep = static_cast<float>(3.1415927f * 2.0 / static_cast<int32_t>(aiStacks));
    uint16_t usStripLength = static_cast<uint16_t>(aiStacks * 2 + 2);
    uint32_t uiVertices = static_cast<uint32_t>(aiSlices) * usStripLength;

    NiPoint3* pkVertex = new NiPoint3[uiVertices];
    NiPoint3* pkNormal = new NiPoint3[uiVertices];
    NiColorA* pkColor = nullptr;
    if (apColor) {
        pkColor = new NiColorA[uiVertices];
    }
    uint16_t* pusStripLengths = new uint16_t[aiSlices];
    uint16_t* pusStripLists = new uint16_t[uiVertices];

    uint16_t usVertex = 0;
    uint16_t usStrip = 0;
    float fPrevAngle = 0.0f;
    float fPrevCos = 1.0f;
    float fPrevSin = 0.0f;
    for (int32_t iSlice = aiSlices - 1; iSlice >= 0; --iSlice) {
        float fAngle = fPrevAngle + fSliceStep;
        float fCos = static_cast<float>(std::cos(static_cast<double>(fAngle)));
        float fSin = static_cast<float>(std::sin(static_cast<double>(fAngle)));
        float fStackAngle = 0.0f;
        for (int32_t iStack = aiStacks; iStack >= 0; --iStack) {
            fStackAngle = fStackAngle + fStackStep;
            float fStackCos = static_cast<float>(std::cos(static_cast<double>(fStackAngle)));
            float fStackSin = static_cast<float>(std::sin(static_cast<double>(fStackAngle)));
            float fRadius = afInnerRadius * fStackCos + afOuterRadius;

            pkVertex[usVertex].x = fRadius * fCos;
            pkVertex[usVertex].y = fRadius * -fSin;
            pkVertex[usVertex].z = fStackSin * afInnerRadius;
            pkNormal[usVertex].x = fStackCos * fCos;
            pkNormal[usVertex].y = fStackCos * -fSin;
            pkNormal[usVertex].z = fStackSin;
            if (apColor) {
                pkColor[usVertex] = *apColor;
            }
            pusStripLists[usVertex] = usVertex;
            ++usVertex;

            pkVertex[usVertex].x = fRadius * fPrevCos;
            pkVertex[usVertex].y = fRadius * -fPrevSin;
            pkVertex[usVertex].z = fStackSin * afInnerRadius;
            pkNormal[usVertex].x = fStackCos * fPrevCos;
            pkNormal[usVertex].y = fStackCos * -fPrevSin;
            pkNormal[usVertex].z = fStackSin;
            if (apColor) {
                pkColor[usVertex] = *apColor;
            }
            pusStripLists[usVertex] = usVertex;
            ++usVertex;
        }
        pusStripLengths[usStrip] = usStripLength;
        ++usStrip;
        fPrevAngle = fAngle;
        fPrevCos = fCos;
        fPrevSin = fSin;
    }

    // The triangle count is pushed as the full product; the callee reads its low word.
    NiTriStrips* pkStrips = new NiTriStrips(
        usVertex,
        pkVertex,
        pkNormal,
        pkColor,
        nullptr,
        0,
        0,
        static_cast<uint16_t>(aiSlices * 2 * aiStacks),
        aiSlices,
        pusStripLengths,
        pusStripLists
    );
    pkStrips->SetName("BSTestObjects Torus");
    NiGeometry* pkGeometry = pkStrips;
    pkGeometry->CalculateNormals();
    return pkStrips;
}

NiAVObject* BSTestObjects::CreateCappedCylinder(
    float afBaseRadius,
    float afTopRadius,
    float afHeight,
    uint16_t aiSlices,
    uint16_t aiStacks,
    const NiColorA* apColor
) {
    NiNode* pNode = new NiNode(0);
    NiAVObject* pCylinder = CreateCylinder(afBaseRadius, afTopRadius, afHeight, aiSlices, aiStacks, apColor);
    NiAVObject* pBottom = CreateCylinder(afBaseRadius, 0.0f, 0.0f, aiSlices, 1, apColor);
    NiMatrix3 kRotate;
    kRotate.MakeXRotation(3.1415927f);
    pBottom->m_kLocal.m_Rotate = kRotate;
    NiAVObject* pTop = CreateCylinder(afBaseRadius, 0.0f, 0.0f, aiSlices, 1, apColor);
    NiPoint3 kTranslate;
    kTranslate.x = 0.0f;
    kTranslate.y = 0.0f;
    kTranslate.z = afHeight;
    pTop->m_kLocal.m_Translate = kTranslate;
    pNode->AttachChild(pCylinder, true);
    pNode->AttachChild(pBottom, true);
    pNode->AttachChild(pTop, true);
    return pNode;
}

NiAVObject* BSTestObjects::CreateBox(
    const NiPoint3& arLeftCorner,
    const NiPoint3& arRightCorner,
    float afForwardDepth,
    float afBackwardDepth,
    float afUpwardHeight,
    float afDownwardHeight,
    const NiColorA* apLeftColor,
    const NiColorA* apRightColor
) {
    NiTriShape* shape;
    NiPoint3* vertices;
    NiPoint3 v76;
    NiPoint3 verticalVector;
    NiPoint3 downwardHeightVector;
    NiPoint3* v119 = new NiPoint3[8];
    if (v119) {
        vertices = v119;
    }
    else {
        vertices = nullptr;
    }
    NiPoint3 horizontalVector = arLeftCorner - arRightCorner;
    verticalVector.x = 0.0f;
    verticalVector.y = 0.0f;
    verticalVector.z = 1.0f;
    NiPoint3 perpVector = horizontalVector.Cross(verticalVector);
    perpVector.z = 0.0f;
    perpVector.Unitize();
    float v68 = perpVector.x * afForwardDepth;
    float v69 = perpVector.y * afForwardDepth;
    float v70 = perpVector.z * afForwardDepth;
    float v64 = -afBackwardDepth;
    float v65 = perpVector.x * v64;
    float v66 = perpVector.y * v64;
    float v67 = perpVector.z * v64;
    float v125 = 0.0f;
    float v126 = 0.0f;
    downwardHeightVector.x = 0.0f;
    downwardHeightVector.y = 0.0f;
    downwardHeightVector.z = -afDownwardHeight;
    float v61 = v65 + arRightCorner.x;
    float v62 = v66 + arRightCorner.y;
    float v63 = v67 + arRightCorner.z;
    float v58 = 0.0f + v61;
    float v59 = 0.0f + v62;
    float v60 = afUpwardHeight + v63;
    vertices[0].x = v58;
    vertices[0].y = v59;
    vertices[0].z = v60;
    float v55 = v68 + arRightCorner.x;
    float v56 = v69 + arRightCorner.y;
    float v57 = v70 + arRightCorner.z;
    float v52 = v125 + v55;
    float v53 = v126 + v56;
    float v54 = afUpwardHeight + v57;
    vertices[1].x = v52;
    vertices[1].y = v53;
    vertices[1].z = v54;
    float v49 = v65 + arRightCorner.x;
    float v50 = v66 + arRightCorner.y;
    float v51 = v67 + arRightCorner.z;
    float v46 = downwardHeightVector.x + v49;
    float v47 = downwardHeightVector.y + v50;
    float v48 = downwardHeightVector.z + v51;
    vertices[2].x = v46;
    vertices[2].y = v47;
    vertices[2].z = v48;
    float v43 = v68 + arRightCorner.x;
    float v44 = v69 + arRightCorner.y;
    float v45 = v70 + arRightCorner.z;
    float v40 = downwardHeightVector.x + v43;
    float v41 = downwardHeightVector.y + v44;
    float v42 = downwardHeightVector.z + v45;
    vertices[3].x = v40;
    vertices[3].y = v41;
    vertices[3].z = v42;
    float v37 = v65 + arLeftCorner.x;
    float v38 = v66 + arLeftCorner.y;
    float v39 = v67 + arLeftCorner.z;
    float v34 = v125 + v37;
    float v35 = v126 + v38;
    float v36 = afUpwardHeight + v39;
    vertices[4].x = v34;
    vertices[4].y = v35;
    vertices[4].z = v36;
    float v31 = v68 + arLeftCorner.x;
    float v32 = v69 + arLeftCorner.y;
    float v33 = v70 + arLeftCorner.z;
    float v28 = v125 + v31;
    float v29 = v126 + v32;
    float v30 = afUpwardHeight + v33;
    vertices[5].x = v28;
    vertices[5].y = v29;
    vertices[5].z = v30;
    float v25 = v65 + arLeftCorner.x;
    float v26 = v66 + arLeftCorner.y;
    float v27 = v67 + arLeftCorner.z;
    float v22 = downwardHeightVector.x + v25;
    float v23 = downwardHeightVector.y + v26;
    float v24 = downwardHeightVector.z + v27;
    vertices[6].x = v22;
    vertices[6].y = v23;
    vertices[6].z = v24;
    float v19 = v68 + arLeftCorner.x;
    float v20 = v69 + arLeftCorner.y;
    float v21 = v70 + arLeftCorner.z;
    v76.x = v19;
    v76.y = v20;
    v76.z = v21;
    vertices[7] = v76 + downwardHeightVector;
    NiColorA* colors_2 = nullptr;
    if (apLeftColor || apRightColor) {
        colors_2 = new NiColorA[8];
        if (!apLeftColor || apRightColor) {
            if (apRightColor && (!apLeftColor)) {
                apLeftColor = apRightColor;
            }
        }
        else {
            apRightColor = apLeftColor;
        }
        for (uint32_t i_2 = 0; i_2 < 4; ++i_2) {
            colors_2[i_2] = *apRightColor;
        }
        for (uint32_t i_3 = 4; i_3 < 8; ++i_3) {
            colors_2[i_3] = *apLeftColor;
        }
    }
    uint16_t* indices = static_cast<uint16_t*>(MemoryManager::Instance().Allocate(36 * sizeof(uint16_t)));
    rt::memcpy(indices, &Box, sizeof(uint16_t) * 36);
    shape = new NiTriShape(8, vertices, nullptr, colors_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 12, indices);
    shape->CalculateNormals();
    return shape;
}

// The two corners differ in y only, by afRadius either side of the centre; both are built from a copy of the centre.
NiAVObject* BSTestObjects::MakeBox(NiPoint3* apCenter, float afRadius, NiColorA* apColor) {
    NiPoint3 kLeftCorner = *apCenter;
    kLeftCorner.y = kLeftCorner.y - afRadius;
    NiPoint3 kRightCorner = *apCenter;
    kRightCorner.y = kRightCorner.y + afRadius;
    return CreateBox(kLeftCorner, kRightCorner, afRadius, afRadius, afRadius, afRadius, apColor, apColor);
}

NiAVObject* BSTestObjects::CreateCoordinateJack(float afScale) {
    NiLines* lines;
    NiColorA* color;
    NiPoint3* vertex;
    NiPoint3* v38 = new NiPoint3[6];
    if (v38) {
        vertex = v38;
    }
    else {
        vertex = nullptr;
    }
    float v36 = 0.0f;
    float v37 = 0.0f;
    float v33 = 0.0f;
    float v34 = 0.0f;
    float v30 = 0.0f;
    float v31 = 0.0f;
    float v28 = 0.0f;
    float v24 = 0.0f;
    float v25 = 0.0f;
    float v21 = 0.0f;
    vertex[0].x = 0.0f;
    vertex[0].y = v36;
    vertex[0].z = v37;
    vertex[1].x = afScale;
    vertex[1].y = v33;
    vertex[1].z = v34;
    vertex[2].x = 0.0f;
    vertex[2].y = v30;
    vertex[2].z = v31;
    vertex[3].x = 0.0f;
    vertex[3].y = afScale;
    vertex[3].z = v28;
    vertex[4].x = 0.0f;
    vertex[4].y = v24;
    vertex[4].z = v25;
    vertex[5].x = 0.0f;
    vertex[5].y = v21;
    vertex[5].z = afScale;
    NiColorA* v18 = new NiColorA[6];
    if (v18) {
        color = v18;
    }
    else {
        color = nullptr;
    }
    color[0] = BSTestObjects::RED;
    color[1] = BSTestObjects::RED;
    color[2] = GREEN;
    color[3] = GREEN;
    color[4] = BLUE;
    color[5] = BLUE;
    uint8_t* flag = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(6u));
    flag[0] = 1;
    flag[1] = 0;
    flag[2] = 1;
    flag[3] = 0;
    flag[4] = 1;
    flag[5] = 0;
    lines = new NiLines(6, vertex, color, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, flag);
    lines->m_kLocal.m_Translate = NiPoint3::ZERO;
    lines->m_kLocal.m_Rotate = NiMatrix3::IDENTITY;
    lines->SetName("BSTestObjects Coordinate Jack");
    return lines;
}

NiAVObject* BSTestObjects::Create3DArrow(
    const NiPoint3& arStart,
    const NiPoint3& arFinish,
    float afSize,
    float afHeight,
    const NiColorA* apStartColor,
    const NiColorA* apEndColor
) {
    NiTriShape* phead;
    NiColorA* colors;
    NiPoint3* vertices;
    NiNode* parrow;
    NiPoint3 v29;
    NiPoint3 verticalVector;
    NiPoint3 shaftEnd;
    NiPoint3 v50;
    NiPoint3 v51;
    NiPoint3 v52;
    NiPoint3 v53;
    NiPoint3 v54;
    parrow = new NiNode(0);
    NiPoint3 arrowVector = arFinish - arStart;
    float flength = arrowVector.UnitizeGetLength();
    float fshaftLength = 0.0f;
    float fheadLength = afSize * 6.0;
    if (!(fheadLength < static_cast<double>(flength))) {
        fheadLength = flength;
    }
    else {
        fshaftLength = flength - fheadLength;
    }
    float v23 = arrowVector.x * fshaftLength;
    float v24 = arrowVector.y * fshaftLength;
    float v25 = arrowVector.z * fshaftLength;
    v29.x = v23;
    v29.y = v24;
    v29.z = v25;
    ::new (&shaftEnd) NiPoint3(arStart + v29);
    if (fshaftLength > 0.0) {
        NiAVObject* pshaft = BSTestObjects::CreateBox(
            arStart,
            shaftEnd,
            afSize,
            afSize,
            afHeight,
            0.0f,
            apStartColor,
            apEndColor
        );
        parrow->AttachChild(pshaft, 1);
    }
    NiPoint3* v28 = new NiPoint3[6];
    if (v28) {
        vertices = v28;
    }
    else {
        vertices = nullptr;
    }
    verticalVector.x = 0.0f;
    verticalVector.y = 0.0f;
    verticalVector.z = 1.0f;
    NiPoint3 perpVector = arrowVector.Cross(verticalVector);
    float v20 = afSize * 4.0;
    perpVector.x = perpVector.x * v20;
    perpVector.y = perpVector.y * v20;
    perpVector.z = perpVector.z * v20;
    float v43 = 0.0f;
    float v44 = 0.0f;
    v54.x = 0.0f + arFinish.x;
    v54.y = 0.0f + arFinish.y;
    v54.z = afHeight + arFinish.z;
    vertices[0] = v54;
    float v17 = perpVector.x + shaftEnd.x;
    float v18 = perpVector.y + shaftEnd.y;
    float v19 = perpVector.z + shaftEnd.z;
    v53.x = v43 + v17;
    v53.y = v44 + v18;
    v53.z = afHeight + v19;
    vertices[1] = v53;
    vertices[2] = arFinish;
    v52.x = perpVector.x + shaftEnd.x;
    v52.y = perpVector.y + shaftEnd.y;
    v52.z = perpVector.z + shaftEnd.z;
    vertices[3] = v52;
    float v14 = shaftEnd.x - perpVector.x;
    float v15 = shaftEnd.y - perpVector.y;
    float v16 = shaftEnd.z - perpVector.z;
    v51.x = v43 + v14;
    v51.y = v44 + v15;
    v51.z = afHeight + v16;
    vertices[4] = v51;
    v50.x = shaftEnd.x - perpVector.x;
    v50.y = shaftEnd.y - perpVector.y;
    v50.z = shaftEnd.z - perpVector.z;
    vertices[5] = v50;
    if (apStartColor && (!apEndColor)) {
        apEndColor = apStartColor;
    }

    if (apEndColor) {
        NiColorA* v9;
        NiColorA* v27 = new NiColorA[6];
        if (v27) {
            v9 = v27;
        }
        else {
            v9 = nullptr;
        }
        colors = v9;
    }
    else {
        colors = nullptr;
    }
    if (colors) {
        for (uint32_t i_2 = 0; i_2 < 6; ++i_2) {
            colors[i_2] = *apEndColor;
        }
    }
    uint16_t* indices = static_cast<uint16_t*>(_NiMalloc(24 * sizeof(uint16_t)));
    rt::memcpy(indices, &Head, 24 * sizeof(uint16_t));
    phead = new NiTriShape(6, vertices, nullptr, colors, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 8, indices);
    phead->CalculateNormals();
    parrow->AttachChild(phead, 1);
    return parrow;
}

NiTriShape* BSTestObjects::MakeTexturedSquare(float afRadius, NiPoint3* apCenter, NiColorA* apColor) {
    NiTriShape* v4;
    NiPoint2* v5;
    NiColorA* v6;
    NiPoint3* v8;
    NiPoint3* v9;
    NiPoint3* v10;
    if (apCenter) {
        v10 = apCenter;
    }
    else {
        v10 = &NiPoint3::ZERO;
    }
    float x = v10->x;
    float y = v10->y;
    NiPoint3* v31 = new NiPoint3[4];
    if (v31) {
        v9 = v31;
    }
    else {
        v9 = nullptr;
    }
    NiPoint3* v30 = new NiPoint3[4];
    if (v30) {
        v8 = v30;
    }
    else {
        v8 = nullptr;
    }
    if (apColor) {
        NiColorA* v7;
        NiColorA* v29 = new NiColorA[4];
        if (v29) {
            v7 = v29;
        }
        else {
            v7 = nullptr;
        }
        v6 = v7;
    }
    else {
        v6 = nullptr;
    }
    NiPoint2* v28 = new NiPoint2[4];
    if (v28) {
        v5 = v28;
    }
    else {
        v5 = nullptr;
    }
    uint16_t* pusTriList = static_cast<uint16_t*>(MemoryManager::Instance().Allocate(6 * sizeof(uint16_t)));
    float v26 = x - afRadius;
    float v27 = afRadius + y;
    float v24 = afRadius + x;
    float v25 = afRadius + y;
    float v22 = x - afRadius;
    float v23 = y - afRadius;
    float v20 = afRadius + x;
    float v21 = y - afRadius;
    v9[0].x = v26;
    v9[0].y = v27;
    v9[0].z = 0.0f;
    v8[0] = NiPoint3::UNIT_Z;
    if (apColor) {
        v6[0] = *apColor;
    }
    v5[0].x = 0.0f;
    v5[0].y = 0.0f;
    v9[1].x = v24;
    v9[1].y = v25;
    v9[1].z = 0.0f;
    v8[1] = NiPoint3::UNIT_Z;
    if (apColor) {
        v6[1] = *apColor;
    }
    v5[1].x = 1.0f;
    v5[1].y = 0.0f;
    v9[2].x = v22;
    v9[2].y = v23;
    v9[2].z = 0.0f;
    v8[2] = NiPoint3::UNIT_Z;
    if (apColor) {
        v6[2] = *apColor;
    }
    v5[2].x = 0.0f;
    v5[2].y = 1.0f;
    v9[3].x = v20;
    v9[3].y = v21;
    v9[3].z = 0.0f;
    v8[3] = NiPoint3::UNIT_Z;
    if (apColor) {
        v6[3] = *apColor;
    }
    v5[3].x = 1.0f;
    v5[3].y = 1.0f;
    pusTriList[0] = 0;
    pusTriList[1] = 1;
    pusTriList[2] = 2;
    pusTriList[3] = 2;
    pusTriList[4] = 1;
    pusTriList[5] = 3;
    v4 = new NiTriShape(4, v9, v8, v6, v5, 0, NiGeometryData::NBT_METHOD_NONE, 2, pusTriList);
    v4->CalculateNormals();
    return v4;
}

NiAVObject* BSTestObjects::CreateBox(const NiPoint3& arExtents, const NiColorA* apColor) {
    NiTriShape* shape;
    NiColorA* colors_2;
    NiPoint3* vertices_2;
    if ((CreateBox_guards & 1) == 0) {
        CreateBox_guards |= 1u;
        BoxPoints[0].x = 0.5f;
        BoxPoints[0].y = 0.5f;
        BoxPoints[0].z = 0.5f;
        BoxPoints[1].x = 0.5f;
        BoxPoints[1].y = 0.5f;
        BoxPoints[1].z = -0.5f;
        BoxPoints[2].x = 0.5f;
        BoxPoints[2].y = -0.5f;
        BoxPoints[2].z = 0.5f;
        BoxPoints[3].x = 0.5f;
        BoxPoints[3].y = -0.5f;
        BoxPoints[3].z = -0.5f;
        BoxPoints[4].x = -0.5f;
        BoxPoints[4].y = 0.5f;
        BoxPoints[4].z = 0.5f;
        BoxPoints[5].x = -0.5f;
        BoxPoints[5].y = 0.5f;
        BoxPoints[5].z = -0.5f;
        BoxPoints[6].x = -0.5f;
        BoxPoints[6].y = -0.5f;
        BoxPoints[6].z = 0.5f;
        BoxPoints[7].x = -0.5f;
        BoxPoints[7].y = -0.5f;
        BoxPoints[7].z = -0.5f;
    }
    NiPoint3* vertices = new NiPoint3[8];
    if (vertices) {
        vertices_2 = vertices;
    }
    else {
        vertices_2 = nullptr;
    }
    for (int32_t j = 0; j < 8; ++j) {
        float v8 = BoxPoints[j].x * arExtents.x;
        vertices_2[j].x = v8;
        float v7 = BoxPoints[j].y * arExtents.y;
        vertices_2[j].y = v7;
        float v6 = BoxPoints[j].z * arExtents.z;
        vertices_2[j].z = v6;
    }
    if (apColor) {
        NiColorA* colors;
        NiColorA* v15 = new NiColorA[8];
        if (v15) {
            colors = v15;
        }
        else {
            colors = nullptr;
        }
        colors_2 = colors;
    }
    else {
        colors_2 = nullptr;
    }
    if (colors_2) {
        for (uint32_t i_2 = 0; i_2 < 8; ++i_2) {
            colors_2[i_2] = *apColor;
        }
    }
    uint16_t* pusTriList = static_cast<uint16_t*>(MemoryManager::Instance().Allocate(36 * sizeof(uint16_t)));
    rt::memcpy(pusTriList, &Box, 36 * sizeof(uint16_t));
    shape = new NiTriShape(
        8,
        vertices_2,
        nullptr,
        colors_2,
        nullptr,
        0,
        NiGeometryData::NBT_METHOD_NONE,
        12,
        pusTriList
    );
    shape->CalculateNormals();
    return shape;
}

NiAVObject* BSTestObjects::CreateSphere(float afRadius, uint16_t aiSlices, uint16_t aiStacks, const NiColorA* apColor) {
    NiTriStrips* strips;
    NiPoint2* textures_2;
    NiColorA* colors_2;
    NiPoint3* normals_2;
    NiPoint3* vertices_2;
    NiTriShape* caps;
    NiPoint2* textures;
    NiColorA* colors;
    NiPoint3* normals;
    NiPoint3* vertices;
    NiNode* sphere;
    sphere = new NiNode(0);
    sphere->SetName("BSTestObjects Sphere");
    float nsign_2 = 1.0f;
    float drho = 3.1415927f / static_cast<double>(aiStacks);
    float dtheta = 3.1415927f * 2.0 / static_cast<double>(aiSlices);
    float ds = 1.0 / static_cast<double>(aiSlices);
    float dt = 1.0 / static_cast<double>(aiStacks);
    NiPoint3* v130 = new NiPoint3[2 * aiSlices + 2];
    if (v130) {
        vertices = v130;
    }
    else {
        vertices = nullptr;
    }
    NiPoint3* vertices_3 = vertices;
    NiPoint3* v129 = new NiPoint3[2 * aiSlices + 2];
    if (v129) {
        normals = v129;
    }
    else {
        normals = nullptr;
    }
    NiPoint3* normals_3 = normals;
    if (apColor) {
        NiColorA* v70;
        NiColorA* v128 = new NiColorA[2 * aiSlices + 2];
        if (v128) {
            v70 = v128;
        }
        else {
            v70 = nullptr;
        }
        colors = v70;
    }
    else {
        colors = nullptr;
    }
    NiColorA* colors_3 = colors;
    NiPoint2* v127 = new NiPoint2[2 * aiSlices + 2];
    if (v127) {
        textures = v127;
    }
    else {
        textures = nullptr;
    }
    NiPoint2* textures_3 = textures;
    uint16_t* indices = static_cast<uint16_t*>(MemoryManager::Instance().Allocate(
        6 * static_cast<uint32_t>(aiSlices) <= SIZE_MAX / sizeof(uint16_t)
            ? sizeof(uint16_t) * (6 * static_cast<uint32_t>(aiSlices))
            : SIZE_MAX
    ));
    uint16_t numVertices = 0;
    uint16_t numIndices = 0;
    uint16_t pole = 0;
    float v126 = afRadius * nsign_2;
    vertices_3->x = 0.0f;
    vertices_3->y = 0.0f;
    vertices_3->z = v126;
    normals_3[numVertices].x = 0.0f;
    normals_3[numVertices].y = 0.0f;
    normals_3[numVertices].z = 1.0f;
    if (apColor) {
        colors_3[numVertices] = *apColor;
    }
    textures_3[numVertices].x = 0.5f;
    textures_3[numVertices].y = 0.0f;
    ++numVertices;
    float s = 0.0f;
    float t = dt;
    for (uint32_t j_2 = 0; j_2 < aiSlices; ++j_2) {
        float theta = static_cast<double>(j_2) * dtheta;
        float v67 = std::sin(static_cast<double>(theta));
        float v66 = std::sin(static_cast<double>(drho));
        float x = v66 * -v67;
        float v65 = std::cos(static_cast<double>(theta));
        float v64 = std::sin(static_cast<double>(drho));
        float y = v64 * v65;
        double v63 = nsign_2;
        float v62 = std::cos(static_cast<double>(drho));
        float z = v62 * v63;
        float v123 = afRadius * x;
        float v124 = afRadius * y;
        float v125 = afRadius * z;
        float v120 = nsign_2 * x;
        float v121 = nsign_2 * y;
        float v122 = nsign_2 * z;
        vertices_3[numVertices].x = v123;
        vertices_3[numVertices].y = v124;
        vertices_3[numVertices].z = v125;
        normals_3[numVertices].x = v120;
        normals_3[numVertices].y = v121;
        normals_3[numVertices].z = v122;
        if (apColor) {
            colors_3[numVertices] = *apColor;
        }
        textures_3[numVertices].x = s;
        textures_3[numVertices].y = t;
        ++numVertices;
        s = s + ds;
        if (j_2) {
            indices[numIndices++] = pole;
            indices[numIndices++] = numVertices - 2;
            indices[numIndices++] = numVertices - 1;
        }
    }
    indices[numIndices++] = pole;
    indices[numIndices++] = numVertices - 1;
    indices[numIndices++] = pole + 1;
    pole = numVertices;
    float v119 = nsign_2 * -afRadius;
    vertices_3[numVertices].x = 0.0f;
    vertices_3[numVertices].y = 0.0f;
    vertices_3[numVertices].z = v119;
    normals_3[numVertices].x = 0.0f;
    normals_3[numVertices].y = 0.0f;
    normals_3[numVertices].z = -1.0f;
    if (apColor) {
        colors_3[numVertices] = *apColor;
    }
    textures_3[numVertices].x = 0.5f;
    textures_3[numVertices].y = 1.0f;
    ++numVertices;
    float rho = 3.1415927f - drho;
    s = 1.0f;
    t = 1.0 - dt;
    for (uint32_t j_3 = aiSlices; j_3; --j_3) {
        double v61;
        if (j_3 == aiSlices) {
            v61 = 0.0;
        }
        else {
            v61 = static_cast<double>(j_3) * dtheta;
        }
        float theta_2 = v61;
        float v60 = std::sin(static_cast<double>(theta_2));
        float v59 = std::sin(static_cast<double>(rho));
        float x_2 = v59 * -v60;
        float v58 = std::cos(static_cast<double>(theta_2));
        float v57 = std::sin(static_cast<double>(rho));
        float y_2 = v57 * v58;
        double v56 = nsign_2;
        float nsign = std::cos(static_cast<double>(rho));
        float z_2 = nsign * v56;
        float v116 = afRadius * x_2;
        float v117 = afRadius * y_2;
        float v118 = afRadius * z_2;
        float v113 = nsign_2 * x_2;
        float v114 = nsign_2 * y_2;
        float v115 = nsign_2 * z_2;
        vertices_3[numVertices].x = v116;
        vertices_3[numVertices].y = v117;
        vertices_3[numVertices].z = v118;
        normals_3[numVertices].x = v113;
        normals_3[numVertices].y = v114;
        normals_3[numVertices].z = v115;
        if (apColor) {
            colors_3[numVertices] = *apColor;
        }
        textures_3[numVertices].x = s;
        textures_3[numVertices].y = t;
        ++numVertices;
        s = s - ds;
        if (j_3 != aiSlices) {
            indices[numIndices++] = pole;
            indices[numIndices++] = numVertices - 2;
            indices[numIndices++] = numVertices - 1;
        }
    }
    indices[numIndices++] = pole;
    indices[numIndices++] = numVertices - 1;
    indices[numIndices++] = pole + 1;
    caps = new NiTriShape(
        numVertices,
        vertices_3,
        normals_3,
        colors_3,
        textures_3,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        numIndices / 3,
        indices
    );
    caps->CalculateNormals();
    sphere->AttachChild(caps, 1);
    t = 0.0f;
    uint16_t numStrips = aiStacks - 2;
    uint16_t numVerticesPerStrip = 2 * aiSlices + 2;
    uint32_t arraySize = numVerticesPerStrip * static_cast<uint16_t>(aiStacks - 2);
    NiPoint3* v110 = new NiPoint3[arraySize];
    if (v110) {
        vertices_2 = v110;
    }
    else {
        vertices_2 = nullptr;
    }
    vertices_3 = vertices_2;
    NiPoint3* v108 = new NiPoint3[arraySize];
    if (v108) {
        normals_2 = v108;
    }
    else {
        normals_2 = nullptr;
    }
    normals_3 = normals_2;
    if (apColor) {
        NiColorA* v51;
        NiColorA* v106 = new NiColorA[arraySize];
        if (v106) {
            v51 = v106;
        }
        else {
            v51 = nullptr;
        }
        colors_2 = v51;
    }
    else {
        colors_2 = nullptr;
    }
    colors_3 = colors_2;
    NiPoint2* v104 = new NiPoint2[arraySize];
    if (v104) {
        textures_2 = v104;
    }
    else {
        textures_2 = nullptr;
    }
    textures_3 = textures_2;
    uint16_t* stripLengths = static_cast<uint16_t*>(MemoryManager::Instance().Allocate(
        numStrips <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * numStrips : SIZE_MAX
    ));
    uint16_t* stripLists = static_cast<uint16_t*>(MemoryManager::Instance().Allocate(
        arraySize <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * arraySize : SIZE_MAX
    ));
    numVertices = 0;
    for (uint32_t i_2 = 0; i_2 < numStrips; ++i_2) {
        rho = static_cast<double>(i_2 + 1) * drho;
        s = 0.0f;
        for (uint32_t j_4 = 0; j_4 <= aiSlices; ++j_4) {
            double v48;
            if (j_4 == aiSlices) {
                v48 = 0.0;
            }
            else {
                v48 = static_cast<double>(j_4) * dtheta;
            }
            float theta_3 = v48;
            float v47 = std::sin(static_cast<double>(theta_3));
            float v46 = std::sin(static_cast<double>(rho));
            float x_3 = v46 * -v47;
            float v45 = std::cos(static_cast<double>(theta_3));
            float v44 = std::sin(static_cast<double>(rho));
            float y_3 = v44 * v45;
            double v43 = nsign_2;
            float v42 = std::cos(static_cast<double>(rho));
            float z_3 = v42 * v43;
            float v101 = afRadius * x_3;
            float v102 = afRadius * y_3;
            float v103 = afRadius * z_3;
            float v98 = nsign_2 * x_3;
            float v99 = nsign_2 * y_3;
            float v100 = nsign_2 * z_3;
            vertices_3[numVertices].x = v101;
            vertices_3[numVertices].y = v102;
            vertices_3[numVertices].z = v103;
            normals_3[numVertices].x = v98;
            normals_3[numVertices].y = v99;
            normals_3[numVertices].z = v100;
            if (apColor) {
                colors_3[numVertices] = *apColor;
            }
            textures_3[numVertices].x = s;
            textures_3[numVertices].y = t;
            stripLists[numVertices] = numVertices;
            ++numVertices;
            float v41 = std::sin(static_cast<double>(theta_3));
            float v40 = drho + rho;
            float v39 = std::sin(static_cast<double>(v40));
            x_3 = v39 * -v41;
            float v38 = std::cos(static_cast<double>(theta_3));
            float v37 = drho + rho;
            float v36 = std::sin(static_cast<double>(v37));
            y_3 = v36 * v38;
            float v35 = drho + rho;
            double v34 = nsign_2;
            float v33 = std::cos(static_cast<double>(v35));
            z_3 = v33 * v34;
            float v95 = afRadius * x_3;
            float v96 = afRadius * y_3;
            float v97 = afRadius * z_3;
            float v92 = nsign_2 * x_3;
            float v93 = nsign_2 * y_3;
            float v94 = nsign_2 * z_3;
            float v91 = dt + t;
            vertices_3[numVertices].x = v95;
            vertices_3[numVertices].y = v96;
            vertices_3[numVertices].z = v97;
            normals_3[numVertices].x = v92;
            normals_3[numVertices].y = v93;
            normals_3[numVertices].z = v94;
            if (apColor) {
                colors_3[numVertices] = *apColor;
            }
            textures_3[numVertices].x = s;
            textures_3[numVertices].y = v91;
            stripLists[numVertices] = numVertices;
            ++numVertices;
            s = s + ds;
        }
        t = t + dt;
        stripLengths[i_2] = numVerticesPerStrip;
    }
    strips = new NiTriStrips(
        numVertices,
        vertices_3,
        normals_3,
        colors_3,
        textures_3,
        1,
        0,
        2 * aiSlices * numStrips,
        numStrips,
        stripLengths,
        stripLists
    );
    strips->CalculateNormals();
    sphere->AttachChild(strips, 1);
    return sphere;
}

NiAVObject* BSTestObjects::CreateCircle(float afRadius, uint16_t asSegments, const NiColorA* apColor) {
    NiColorA* pcolors_2;
    NiPoint3* pvertices_2;
    NiPoint3* pvertices = new NiPoint3[asSegments];
    if (pvertices) {
        pvertices_2 = pvertices;
    }
    else {
        pvertices_2 = nullptr;
    }
    uint8_t* pflags = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(asSegments));
    if (apColor) {
        NiColorA* pcolors;
        NiColorA* v19 = new NiColorA[asSegments];
        if (v19) {
            pcolors = v19;
        }
        else {
            pcolors = nullptr;
        }
        pcolors_2 = pcolors;
    }
    else {
        pcolors_2 = nullptr;
    }
    for (uint32_t i_2 = 0; i_2 < asSegments; ++i_2) {
        if (pcolors_2) {
            pcolors_2[i_2] = *apColor;
        }
        pflags[i_2] = 1;
        float fmult = 6.2831854820251465 / static_cast<double>(asSegments);
        float v8 = static_cast<double>(i_2) * fmult;
        float v7 = std::cos(static_cast<double>(v8));
        float v6 = std::sin(static_cast<double>(v8));
        float v17 = afRadius * v6;
        float stSizeInBytes = afRadius * v7;
        float* p_x = &pvertices_2[i_2].x;
        p_x[0] = v17;
        p_x[1] = stSizeInBytes;
        p_x[2] = 0.0f;
    }
    return new NiLines(asSegments, pvertices_2, pcolors_2, nullptr, 1, NiGeometryData::NBT_METHOD_NONE, pflags);
}

NiAVObject* BSTestObjects::CreateArrow(float afLength, const NiColorA* apColor) {
    NiLines* lines;
    NiColorA* color_2;
    NiPoint3* vertex;
    NiPoint3 v19;
    float fsize = afLength / 6.0;
    NiPoint3* v32 = new NiPoint3[5];
    if (v32) {
        vertex = v32;
    }
    else {
        vertex = nullptr;
    }
    float v30 = 0.0f;
    float v31 = 0.0f;
    float v27 = 0.0f;
    float v24 = 0.0f;
    float v25 = afLength - fsize;
    float v21 = 0.0f;
    v19.x = -fsize;
    v19.y = 0.0f;
    v19.z = afLength - fsize;
    vertex[0].x = 0.0f;
    vertex[0].y = v30;
    vertex[0].z = v31;
    vertex[1].x = 0.0f;
    vertex[1].y = v27;
    vertex[1].z = afLength;
    vertex[2].x = fsize;
    vertex[2].y = v24;
    vertex[2].z = v25;
    vertex[3].x = 0.0f;
    vertex[3].y = v21;
    vertex[3].z = afLength;
    vertex[4] = v19;
    if (!apColor) {
        apColor = &BSTestObjects::RED;
    }
    NiColorA* color = new NiColorA[6];
    if (color) {
        color_2 = color;
    }
    else {
        color_2 = nullptr;
    }
    color_2[0] = *apColor;
    color_2[1] = *apColor;
    color_2[2] = *apColor;
    color_2[3] = *apColor;
    color_2[4] = *apColor;
    uint8_t* flag = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(6u));
    flag[0] = 1;
    flag[1] = 1;
    flag[2] = 0;
    flag[3] = 1;
    flag[4] = 0;
    lines = new NiLines(5, vertex, color_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, flag);
    lines->m_kLocal.m_Translate = NiPoint3::ZERO;
    lines->m_kLocal.m_Rotate = NiMatrix3::IDENTITY;
    lines->SetName("BSTestObjects Arrow");
    return lines;
}
