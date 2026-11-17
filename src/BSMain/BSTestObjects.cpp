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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "SDK/MSVC/ehvec.h"
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
    FNVD_INLINED_LOCAL(NiFixedString, v22, std::nullopt, std::nullopt);
    NiPoint3 apResult;
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
        float x = v72.x;
        float y = v72.y;
        float z = v72.z;
        dir_2 = v72;
        flength = dir_2.Length();
    }
    NiPoint3* v53 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(5 * sizeof(NiPoint3)));
    if (v53) {
        int32_t v19 = 5;
        for (NiPoint3* i = v53; --v19 >= 0; ++i) {
            ;
        }
        vertex = v53;
    }
    else {
        vertex = nullptr;
    }
    float v50 = 0.0f;
    float v51 = 0.0f;
    float v52 = 0.0f;
    vertex->x = 0.0f;
    vertex->y = v51;
    vertex->z = v52;
    float v14 = flength * 0.15000000596046448;
    NiPoint3* v15 = dir_2.Perpendicular(&apResult);
    float v16 = v15->x * v14;
    float v17 = v15->y * v14;
    float v18 = v15->z * v14;
    float v11 = dir_2.x * 0.75f;
    float v12 = dir_2.y * 0.75f;
    float v13 = dir_2.z * 0.75f;
    v71.x = v11 - v16;
    v71.y = v12 - v17;
    v71.z = v13 - v18;
    float v43 = v71.x;
    float v44 = v71.y;
    float v45 = v71.z;
    v70.x = v16 + v11;
    v70.y = v17 + v12;
    v70.z = v18 + v13;
    float v40 = v70.x;
    float v41 = v70.y;
    float v42 = v70.z;
    vertex[1] = dir_2;
    vertex[2] = v70;
    vertex[3] = dir_2;
    vertex[4] = v71;
    if (!apColor) {
        apColor = &BSTestObjects::RED;
    }
    NiColorA* color = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(6 * sizeof(NiColorA)));
    if (color) {
        int32_t v9 = 6;
        for (float* j = &color->r; --v9 >= 0; j += 4) {
            *j = 0.0f;
            j[1] = 0.0f;
            j[2] = 0.0f;
            j[3] = 0.0f;
        }
        color_2 = color;
    }
    else {
        color_2 = nullptr;
    }
    *color_2 = *apColor;
    color_2[1] = *apColor;
    float v34 = 0.0f;
    float v35 = 1.0f;
    float v36 = 0.0f;
    float v37 = 1.0f;
    NiColorA* v2 = color_2 + 2;
    color_2[2].r = 0.0f;
    v2->g = v35;
    v2->b = v36;
    v2->a = v37;
    float v30 = 0.0f;
    float v31 = 1.0f;
    float v32 = 0.0f;
    float v33 = 1.0f;
    NiColorA* v3 = color_2 + 3;
    color_2[3].r = 0.0f;
    v3->g = v31;
    v3->b = v32;
    v3->a = v33;
    float v26 = 0.0f;
    float v27 = 1.0f;
    float v28 = 0.0f;
    float v29 = 1.0f;
    NiColorA* v4 = color_2 + 4;
    color_2[4].r = 0.0f;
    v4->g = v27;
    v4->b = v28;
    v4->a = v29;
    uint8_t* flag = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(6u));
    *flag = 1;
    flag[1] = 0;
    flag[2] = 1;
    flag[3] = 1;
    flag[4] = 0;
    NiGeometry* v23 = static_cast<NiGeometry*>(NiMemObject::operator new(sizeof(NiLines)));
    if (v23) {
        lines = ::new (v23) NiLines(5, vertex, color_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, flag);
    }
    else {
        lines = nullptr;
    }
    lines->m_kLocal.m_Translate = NiPoint3_ZERO;
    lines->m_kLocal.m_Rotate = NiMatrix3_IDENTITY;
    if ("BSTestObjects DirArrow") {
        v22.m_kHandle = NiGlobalStringTable::AddString("BSTestObjects DirArrow");
    }
    else {
        v22.m_kHandle = NiGlobalStringTable::NULL_STRING;
    }
    lines->SetName(v22);
    // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
    NiGlobalStringTable::ReleaseHandle(v22.GetHandle());
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
    FNVD_INLINED_LOCAL(NiFixedString, v34, std::nullopt, std::nullopt);
    uint16_t numVerticesPerStrip = 2 * aiSlices + 2;
    uint32_t arraySize = numVerticesPerStrip * aiStacks;
    NiPoint3* v64 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
        arraySize <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * arraySize : SIZE_MAX
    ));
    if (v64) {
        uint32_t v32 = arraySize;
        for (NiPoint3* i = v64; (--v32 & 0x80000000u) == 0; ++i) {
            ;
        }
        vertices = v64;
    }
    else {
        vertices = nullptr;
    }
    NiPoint3* v61 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
        arraySize <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * arraySize : SIZE_MAX
    ));
    if (v61) {
        uint32_t v30 = arraySize;
        for (NiPoint3* j = v61; (--v30 & 0x80000000u) == 0; ++j) {
            ;
        }
        normals = v61;
    }
    else {
        normals = nullptr;
    }
    if (apColor) {
        NiColorA* v23;
        NiColorA* v58 = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(
            arraySize <= SIZE_MAX / sizeof(NiColorA) ? sizeof(NiColorA) * arraySize : SIZE_MAX
        ));
        if (v58) {
            uint32_t v28 = arraySize;
            for (NiColorA* k = v58; (--v28 & 0x80000000u) == 0; ++k) {
                k->r = 0.0f;
                k->g = 0.0f;
                k->b = 0.0f;
                k->a = 0.0f;
            }
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
    NiPoint2* v55 = static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(
        arraySize <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * arraySize : SIZE_MAX
    ));
    if (v55) {
        uint32_t v26 = arraySize;
        for (NiPoint2* m = v55; (--v26 & 0x80000000u) == 0; ++m) {
            ;
        }
        textures = v55;
    }
    else {
        textures = nullptr;
    }
    uint16_t* stripLengths = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(
        aiStacks <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * aiStacks : SIZE_MAX
    ));
    uint16_t* stripLists = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(
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
            float v52 = z;
            NiPoint3* v6 = &vertices[numVertices];
            v6->x = v50;
            v6->y = v51;
            v6->z = v52;
            float v47 = nsign * x;
            float v48 = nsign * y;
            float v49 = nsign * nz;
            NiPoint3* v7 = &normals[numVertices];
            v7->x = v47;
            v7->y = v48;
            v7->z = v49;
            if (apColor) {
                colors[numVertices] = *apColor;
            }
            float v45 = s;
            float v46 = t;
            float v8 = t;
            int32_t v9 = numVertices;
            textures[numVertices].x = s;
            textures[v9].y = v8;
            stripLists[numVertices] = numVertices;
            ++numVertices;
            float v42 = (dr + r) * x;
            float v43 = (dr + r) * y;
            float v44 = dz + z;
            NiPoint3* v11 = &vertices[numVertices];
            v11->x = v42;
            v11->y = v43;
            v11->z = v44;
            float v39 = nsign * x;
            float v40 = nsign * y;
            float v41 = nsign * nz;
            NiPoint3* v12 = &normals[numVertices];
            v12->x = v39;
            v12->y = v40;
            v12->z = v41;
            if (apColor) {
                colors[numVertices] = *apColor;
            }
            float v37 = s;
            float v38 = t - dt;
            float v13 = v38;
            int32_t v14 = numVertices;
            textures[numVertices].x = s;
            textures[v14].y = v13;
            stripLists[numVertices] = numVertices;
            ++numVertices;
            s = s + ds;
        }
        r = r + dr;
        t = t - dt;
        z = z + dz;
        stripLengths[j_2] = numVerticesPerStrip;
    }
    void* apMemory = NiMemObject::operator new(sizeof(NiTriStrips));
    if (apMemory) {
        cylinder = ::new (apMemory) NiTriStrips(
            numVertices, vertices, normals, colors, textures, 1, 0, aiSlices * (2 * aiStacks), aiStacks, stripLengths,
            stripLists
        );
    }
    else {
        cylinder = nullptr;
    }
    if ("BSTestObjects Cylinder") {
        v34.m_kHandle = NiGlobalStringTable::AddString("BSTestObjects Cylinder");
    }
    else {
        v34.m_kHandle = NiGlobalStringTable::NULL_STRING;
    }
    cylinder->SetName(v34);
    // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
    NiGlobalStringTable::ReleaseHandle(v34.GetHandle());
    cylinder->CalculateNormals();
    return (NiAVObject*)cylinder;
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

    uint64_t ullSize = static_cast<uint64_t>(uiVertices) * sizeof(NiPoint3);
    NiPoint3* pkVertex = static_cast<NiPoint3*>(
        MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF : static_cast<uint32_t>(ullSize))
    );
    NiPoint3* pkNormal = static_cast<NiPoint3*>(
        MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF : static_cast<uint32_t>(ullSize))
    );
    NiColorA* pkColor = nullptr;
    if (apColor) {
        ullSize = static_cast<uint64_t>(uiVertices) * sizeof(NiColorA);
        pkColor = static_cast<NiColorA*>(
            MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF : static_cast<uint32_t>(ullSize))
        );
        if (pkColor) {
            // EXE-INLINED none: NiColorA's constructor, run on each element of the array new
            for (int32_t i = static_cast<int32_t>(uiVertices) - 1; i >= 0; --i) {
                pkColor[uiVertices - 1 - i].r = 0.0f;
                pkColor[uiVertices - 1 - i].g = 0.0f;
                pkColor[uiVertices - 1 - i].b = 0.0f;
                pkColor[uiVertices - 1 - i].a = 0.0f;
            }
        }
    }
    ullSize = static_cast<uint64_t>(aiSlices) * sizeof(uint16_t);
    uint16_t* pusStripLengths = static_cast<uint16_t*>(
        MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF : static_cast<uint32_t>(ullSize))
    );
    ullSize = static_cast<uint64_t>(uiVertices) * sizeof(uint16_t);
    uint16_t* pusStripLists = static_cast<uint16_t*>(
        MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF : static_cast<uint32_t>(ullSize))
    );

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

    void* pMemory = NiMemObject::operator new(sizeof(NiTriStrips));
    NiTriStrips* pkStrips = nullptr;
    if (pMemory) {
        // The triangle count is pushed as the full product; the callee reads its low word.
        pkStrips = ::new (pMemory) NiTriStrips(
            usVertex, pkVertex, pkNormal, pkColor, nullptr, 0, 0, static_cast<uint16_t>(aiSlices * 2 * aiStacks),
            aiSlices, pusStripLengths, pusStripLists
        );
    }
    // EXE-INLINED none: NiFixedString(const char*), which interns a non-null string
    FNVD_INLINED_LOCAL(NiFixedString, kName, std::nullopt, std::nullopt);
    kName.SetHandle(NiGlobalStringTable::AddString("BSTestObjects Torus"));
    pkStrips->SetName(kName);
    // EXE-INLINED none: ~NiFixedString, which drops the reference unless the handle is the null string. The count is
    // the first word of the interned buffer, 8 bytes before the characters (raw string-table memory).
    NiGlobalStringTable::GlobalStringHandle kHandle = kName.GetHandle();
    if (kHandle != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<uint32_t*>(const_cast<char*>(kHandle) - 8));
    }
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
    void* pMemory = NiMemObject::operator new(sizeof(NiNode));
    NiNode* pNode;
    if (pMemory) {
        pNode = ::new (pMemory) NiNode(0);
    }
    else {
        pNode = nullptr;
    }
    NiAVObject* pCylinder = CreateCylinder(afBaseRadius, afTopRadius, afHeight, aiSlices, aiStacks, apColor);
    NiAVObject* pBottom = CreateCylinder(afBaseRadius, 0.0f, 0.0f, aiSlices, 1, apColor);
    // EXE-INLINED none: the rotation about x by the float pi, an inline NiSinCos.
    float fAngle = 3.1415927f;
    float fCos = std::cos(fAngle);
    float fSin = std::sin(fAngle);
    NiMatrix3 kRotate;
    kRotate.m_pEntry[0][0] = 1.0f;
    kRotate.m_pEntry[0][1] = 0.0f;
    kRotate.m_pEntry[0][2] = 0.0f;
    kRotate.m_pEntry[1][0] = 0.0f;
    kRotate.m_pEntry[1][1] = fCos;
    kRotate.m_pEntry[1][2] = fSin;
    kRotate.m_pEntry[2][0] = 0.0f;
    kRotate.m_pEntry[2][1] = -fSin;
    kRotate.m_pEntry[2][2] = fCos;
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
    NiPoint3* v119 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(8 * sizeof(NiPoint3)));
    if (v119) {
        int32_t v71 = 8;
        for (NiPoint3* i = v119; --v71 >= 0; ++i) {
            ;
        }
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
    vertices->x = v58;
    vertices->y = v59;
    vertices->z = v60;
    float v55 = v68 + arRightCorner.x;
    float v56 = v69 + arRightCorner.y;
    float v57 = v70 + arRightCorner.z;
    float v52 = v125 + v55;
    float v53 = v126 + v56;
    float v54 = afUpwardHeight + v57;
    NiPoint3* v8 = vertices + 1;
    vertices[1].x = v52;
    v8->y = v53;
    v8->z = v54;
    float v49 = v65 + arRightCorner.x;
    float v50 = v66 + arRightCorner.y;
    float v51 = v67 + arRightCorner.z;
    float v46 = downwardHeightVector.x + v49;
    float v47 = downwardHeightVector.y + v50;
    float v48 = downwardHeightVector.z + v51;
    NiPoint3* v9 = vertices + 2;
    vertices[2].x = v46;
    v9->y = v47;
    v9->z = v48;
    float v43 = v68 + arRightCorner.x;
    float v44 = v69 + arRightCorner.y;
    float v45 = v70 + arRightCorner.z;
    float v40 = downwardHeightVector.x + v43;
    float v41 = downwardHeightVector.y + v44;
    float v42 = downwardHeightVector.z + v45;
    NiPoint3* v10 = vertices + 3;
    vertices[3].x = v40;
    v10->y = v41;
    v10->z = v42;
    float v37 = v65 + arLeftCorner.x;
    float v38 = v66 + arLeftCorner.y;
    float v39 = v67 + arLeftCorner.z;
    float v34 = v125 + v37;
    float v35 = v126 + v38;
    float v36 = afUpwardHeight + v39;
    NiPoint3* v11 = vertices + 4;
    vertices[4].x = v34;
    v11->y = v35;
    v11->z = v36;
    float v31 = v68 + arLeftCorner.x;
    float v32 = v69 + arLeftCorner.y;
    float v33 = v70 + arLeftCorner.z;
    float v28 = v125 + v31;
    float v29 = v126 + v32;
    float v30 = afUpwardHeight + v33;
    NiPoint3* v12 = vertices + 5;
    vertices[5].x = v28;
    v12->y = v29;
    v12->z = v30;
    float v25 = v65 + arLeftCorner.x;
    float v26 = v66 + arLeftCorner.y;
    float v27 = v67 + arLeftCorner.z;
    float v22 = downwardHeightVector.x + v25;
    float v23 = downwardHeightVector.y + v26;
    float v24 = downwardHeightVector.z + v27;
    NiPoint3* v13 = vertices + 6;
    vertices[6].x = v22;
    v13->y = v23;
    v13->z = v24;
    float v19 = v68 + arLeftCorner.x;
    float v20 = v69 + arLeftCorner.y;
    float v21 = v70 + arLeftCorner.z;
    v76.x = v19;
    v76.y = v20;
    v76.z = v21;
    vertices[7] = v76 + downwardHeightVector;
    NiColorA* colors_2 = nullptr;
    if (apLeftColor || apRightColor) {
        NiColorA* colors;
        void* ptr = MemoryManager_Singleton.Allocate(8 * sizeof(NiColorA));
        if (ptr) {
            rt::eh_vector_constructor_iterator(ptr, sizeof(NiColorA), 8, &fnvd::constructElement<NiColorA>);
            colors = static_cast<NiColorA*>(ptr);
        }
        else {
            colors = nullptr;
        }
        colors_2 = colors;
        if (!apLeftColor || apRightColor) {
            if ((apRightColor) && (!apLeftColor)) {
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
    uint16_t* indices = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(36 * sizeof(uint16_t)));
    rt::memcpy(indices, &Box, sizeof(uint16_t) * 36);
    NiTriShape* apMemory = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (apMemory) {
        shape = ::new (apMemory)
            NiTriShape(8, vertices, nullptr, colors_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 12, indices);
    }
    else {
        shape = nullptr;
    }
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
    FNVD_INLINED_LOCAL(NiFixedString, v14, std::nullopt, std::nullopt);
    NiPoint3* v38 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(6 * sizeof(NiPoint3)));
    if (v38) {
        int32_t v12 = 6;
        for (NiPoint3* i = v38; --v12 >= 0; ++i) {
            ;
        }
        vertex = v38;
    }
    else {
        vertex = nullptr;
    }
    float v35 = 0.0f;
    float v36 = 0.0f;
    float v37 = 0.0f;
    vertex->x = 0.0f;
    vertex->y = v36;
    vertex->z = v37;
    float v33 = 0.0f;
    float v34 = 0.0f;
    NiPoint3* v1 = vertex + 1;
    vertex[1].x = afScale;
    v1->y = v33;
    v1->z = v34;
    float v29 = 0.0f;
    float v30 = 0.0f;
    float v31 = 0.0f;
    NiPoint3* v2 = vertex + 2;
    vertex[2].x = 0.0f;
    v2->y = v30;
    v2->z = v31;
    float v26 = 0.0f;
    float v28 = 0.0f;
    NiPoint3* v3 = vertex + 3;
    vertex[3].x = 0.0f;
    v3->y = afScale;
    v3->z = v28;
    float v23 = 0.0f;
    float v24 = 0.0f;
    float v25 = 0.0f;
    NiPoint3* v4 = vertex + 4;
    vertex[4].x = 0.0f;
    v4->y = v24;
    v4->z = v25;
    float v20 = 0.0f;
    float v21 = 0.0f;
    NiPoint3* v5 = vertex + 5;
    vertex[5].x = 0.0f;
    v5->y = v21;
    v5->z = afScale;
    NiColorA* v18 = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(6 * sizeof(NiColorA)));
    if (v18) {
        int32_t v10 = 6;
        for (float* j = &v18->r; --v10 >= 0; j += 4) {
            *j = 0.0f;
            j[1] = 0.0f;
            j[2] = 0.0f;
            j[3] = 0.0f;
        }
        color = v18;
    }
    else {
        color = nullptr;
    }
    *color = BSTestObjects::RED;
    color[1] = BSTestObjects::RED;
    color[2] = GREEN;
    color[3] = GREEN;
    color[4] = BLUE;
    color[5] = BLUE;
    uint8_t* flag = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(6u));
    *flag = 1;
    flag[1] = 0;
    flag[2] = 1;
    flag[3] = 0;
    flag[4] = 1;
    flag[5] = 0;
    NiGeometry* v15 = static_cast<NiGeometry*>(NiMemObject::operator new(sizeof(NiLines)));
    if (v15) {
        lines = ::new (v15) NiLines(6, vertex, color, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, flag);
    }
    else {
        lines = nullptr;
    }
    lines->m_kLocal.m_Translate = NiPoint3_ZERO;
    lines->m_kLocal.m_Rotate = NiMatrix3_IDENTITY;
    if ("BSTestObjects Coordinate Jack") {
        v14.m_kHandle = NiGlobalStringTable::AddString("BSTestObjects Coordinate Jack");
    }
    else {
        v14.m_kHandle = NiGlobalStringTable::NULL_STRING;
    }
    lines->SetName(v14);
    // Releases the string handle of v14, as its destructor would.
    // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
    NiGlobalStringTable::ReleaseHandle(v14.GetHandle());
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
    void* apMemory = NiMemObject::operator new(sizeof(NiNode));
    if (apMemory) {
        parrow = ::new (apMemory) NiNode(0);
    }
    else {
        parrow = nullptr;
    }
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
        NiAVObject* pshaft =
            BSTestObjects::CreateBox(arStart, shaftEnd, afSize, afSize, afHeight, 0.0f, apStartColor, apEndColor);
        parrow->AttachChild(pshaft, 1);
    }
    NiPoint3* v28 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(6 * sizeof(NiPoint3)));
    if (v28) {
        int32_t v21 = 6;
        for (NiPoint3* i = v28; --v21 >= 0; ++i) {
            ;
        }
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
    *vertices = v54;
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
    if ((apStartColor) && (!apEndColor)) {
        apEndColor = apStartColor;
    }

    if (apEndColor) {
        NiColorA* v9;
        NiColorA* v27 = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(6 * sizeof(NiColorA)));
        if (v27) {
            int32_t v12 = 6;
            for (NiColorA* j = v27; --v12 >= 0; ++j) {
                j->r = 0.0f;
                j->g = 0.0f;
                j->b = 0.0f;
                j->a = 0.0f;
            }
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
    NiTriShape* v26 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (v26) {
        phead = ::new (v26)
            NiTriShape(6, vertices, nullptr, colors, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 8, indices);
    }
    else {
        phead = nullptr;
    }
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
        v10 = &NiPoint3_ZERO;
    }
    float x = v10->x;
    float y = v10->y;
    float z = v10->z;
    NiPoint3* v31 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(4 * sizeof(NiPoint3)));
    if (v31) {
        int32_t v17 = 4;
        for (NiPoint3* i = v31; --v17 >= 0; ++i) {
            ;
        }
        v9 = v31;
    }
    else {
        v9 = nullptr;
    }
    NiPoint3* v30 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(4 * sizeof(NiPoint3)));
    if (v30) {
        int32_t v15 = 4;
        for (NiPoint3* j = v30; --v15 >= 0; ++j) {
            ;
        }
        v8 = v30;
    }
    else {
        v8 = nullptr;
    }
    if (apColor) {
        NiColorA* v7;
        NiColorA* v29 = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(4 * sizeof(NiColorA)));
        if (v29) {
            int32_t v13 = 4;
            for (NiColorA* k = v29; --v13 >= 0; ++k) {
                k->r = 0.0f;
                k->g = 0.0f;
                k->b = 0.0f;
                k->a = 0.0f;
            }
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
    NiPoint2* v28 = static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(4 * sizeof(NiPoint2)));
    if (v28) {
        int32_t v11 = 4;
        for (NiPoint2* m = v28; --v11 >= 0; ++m) {
            ;
        }
        v5 = v28;
    }
    else {
        v5 = nullptr;
    }
    uint16_t* pusTriList = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(6 * sizeof(uint16_t)));
    float v26 = x - afRadius;
    float v27 = afRadius + y;
    v9->x = v26;
    v9->y = v27;
    v9->z = 0.0f;
    *v8 = NiPoint3_UNIT_Z;
    if (apColor) {
        *v6 = *apColor;
    }
    v5->x = 0.0f;
    v5->y = 0.0f;
    float v24 = afRadius + x;
    float v25 = afRadius + y;
    v9[1].x = v24;
    v9[1].y = v25;
    v9[1].z = 0.0f;
    v8[1] = NiPoint3_UNIT_Z;
    if (apColor) {
        v6[1] = *apColor;
    }
    v5[1].x = 1.0f;
    v5[1].y = 0.0f;
    float v22 = x - afRadius;
    float v23 = y - afRadius;
    v9[2].x = v22;
    v9[2].y = v23;
    v9[2].z = 0.0f;
    v8[2] = NiPoint3_UNIT_Z;
    if (apColor) {
        v6[2] = *apColor;
    }
    v5[2].x = 0.0f;
    v5[2].y = 1.0f;
    float v20 = afRadius + x;
    float v21 = y - afRadius;
    v9[3].x = v20;
    v9[3].y = v21;
    v9[3].z = 0.0f;
    v8[3] = NiPoint3_UNIT_Z;
    if (apColor) {
        v6[3] = *apColor;
    }
    v5[3].x = 1.0f;
    v5[3].y = 1.0f;
    *pusTriList = 0;
    pusTriList[1] = 1;
    pusTriList[2] = 2;
    pusTriList[3] = 2;
    pusTriList[4] = 1;
    pusTriList[5] = 3;
    NiTriShape* apMemory = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (apMemory) {
        v4 = ::new (apMemory) NiTriShape(4, v9, v8, v6, v5, 0, NiGeometryData::NBT_METHOD_NONE, 2, pusTriList);
    }
    else {
        v4 = nullptr;
    }
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
    NiPoint3* vertices = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(8 * sizeof(NiPoint3)));
    if (vertices) {
        int32_t v12 = 8;
        for (NiPoint3* i = vertices; --v12 >= 0; ++i) {
            ;
        }
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
        NiColorA* v15 = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(8 * sizeof(NiColorA)));
        if (v15) {
            int32_t v10 = 8;
            for (NiColorA* k = v15; --v10 >= 0; ++k) {
                k->r = 0.0f;
                k->g = 0.0f;
                k->b = 0.0f;
                k->a = 0.0f;
            }
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
    uint16_t* pusTriList = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(36 * sizeof(uint16_t)));
    rt::memcpy(pusTriList, &Box, 36 * sizeof(uint16_t));
    NiTriShape* apMemory = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (apMemory) {
        shape = ::new (apMemory)
            NiTriShape(8, vertices_2, nullptr, colors_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 12, pusTriList);
    }
    else {
        shape = nullptr;
    }
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
    FNVD_INLINED_LOCAL(NiFixedString, v131, std::nullopt, std::nullopt);
    void* apMemory = NiMemObject::operator new(sizeof(NiNode));
    if (apMemory) {
        sphere = ::new (apMemory) NiNode(0);
    }
    else {
        sphere = nullptr;
    }
    if ("BSTestObjects Sphere") {
        v131.m_kHandle = NiGlobalStringTable::AddString("BSTestObjects Sphere");
    }
    else {
        v131.m_kHandle = NiGlobalStringTable::NULL_STRING;
    }
    sphere->SetName(v131);
    // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
    NiGlobalStringTable::ReleaseHandle(v131.GetHandle());
    float nsign_2 = 1.0f;
    float drho = 3.1415927f / static_cast<double>(aiStacks);
    float dtheta = 3.1415927f * 2.0 / static_cast<double>(aiSlices);
    float ds = 1.0 / static_cast<double>(aiSlices);
    float dt = 1.0 / static_cast<double>(aiStacks);
    NiPoint3* v130 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
        2 * static_cast<uint32_t>(aiSlices) + 2 <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * (2 * aiSlices + 2)
                                                                               : SIZE_MAX
    ));
    if (v130) {
        int32_t v88 = 2 * aiSlices + 2;
        for (NiPoint3* i = v130; --v88 >= 0; ++i) {
            ;
        }
        vertices = v130;
    }
    else {
        vertices = nullptr;
    }
    NiPoint3* vertices_3 = vertices;
    NiPoint3* v129 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
        2 * static_cast<uint32_t>(aiSlices) + 2 <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * (2 * aiSlices + 2)
                                                                               : SIZE_MAX
    ));
    if (v129) {
        int32_t v86 = 2 * aiSlices + 2;
        for (NiPoint3* j = v129; --v86 >= 0; ++j) {
            ;
        }
        normals = v129;
    }
    else {
        normals = nullptr;
    }
    NiPoint3* normals_3 = normals;
    if (apColor) {
        NiColorA* v70;
        NiColorA* v128 = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(
            2 * static_cast<uint32_t>(aiSlices) + 2 <= SIZE_MAX / sizeof(NiColorA)
                ? sizeof(NiColorA) * (2 * aiSlices + 2)
                : SIZE_MAX
        ));
        if (v128) {
            int32_t v84 = 2 * aiSlices + 2;
            for (NiColorA* k = v128; --v84 >= 0; ++k) {
                k->r = 0.0f;
                k->g = 0.0f;
                k->b = 0.0f;
                k->a = 0.0f;
            }
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
    NiPoint2* v127 = static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(
        2 * static_cast<uint32_t>(aiSlices) + 2 <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * (2 * aiSlices + 2)
                                                                               : SIZE_MAX
    ));
    if (v127) {
        int32_t v82 = 2 * aiSlices + 2;
        for (NiPoint2* m = v127; --v82 >= 0; ++m) {
            ;
        }
        textures = v127;
    }
    else {
        textures = nullptr;
    }
    NiPoint2* textures_3 = textures;
    uint16_t* indices = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(
        6 * static_cast<uint32_t>(aiSlices) <= SIZE_MAX / sizeof(uint16_t)
            ? sizeof(uint16_t) * (6 * static_cast<uint32_t>(aiSlices))
            : SIZE_MAX
    ));
    uint16_t numVertices = 0;
    uint16_t numIndices = 0;
    uint16_t pole = 0;
    float v126 = afRadius * nsign_2;
    NiPoint3* v4 = vertices_3;
    vertices_3->x = 0.0f;
    v4->y = 0.0f;
    v4->z = v126;
    NiPoint3* v5 = &normals_3[numVertices];
    v5->x = 0.0f;
    v5->y = 0.0f;
    v5->z = 1.0f;
    if (apColor) {
        colors_3[numVertices] = *apColor;
    }
    int32_t v6 = numVertices;
    NiPoint2* v7 = textures_3;
    textures_3[numVertices].x = 0.5f;
    v7[v6].y = 0.0f;
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
        NiPoint3* v8 = &vertices_3[numVertices];
        v8->x = v123;
        v8->y = v124;
        v8->z = v125;
        float v120 = nsign_2 * x;
        float v121 = nsign_2 * y;
        float v122 = nsign_2 * z;
        NiPoint3* v9 = &normals_3[numVertices];
        v9->x = v120;
        v9->y = v121;
        v9->z = v122;
        if (apColor) {
            colors_3[numVertices] = *apColor;
        }
        float v10 = t;
        int32_t v11 = numVertices;
        NiPoint2* v12 = textures_3;
        textures_3[numVertices].x = s;
        v12[v11].y = v10;
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
    NiPoint3* v13 = &vertices_3[numVertices];
    v13->x = 0.0f;
    v13->y = 0.0f;
    v13->z = v119;
    NiPoint3* v14 = &normals_3[numVertices];
    v14->x = 0.0f;
    v14->y = 0.0f;
    v14->z = -1.0f;
    if (apColor) {
        colors_3[numVertices] = *apColor;
    }
    int32_t v15 = numVertices;
    NiPoint2* v16 = textures_3;
    textures_3[numVertices].x = 0.5f;
    v16[v15].y = 1.0f;
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
        NiPoint3* v17 = &vertices_3[numVertices];
        v17->x = v116;
        v17->y = v117;
        v17->z = v118;
        float v113 = nsign_2 * x_2;
        float v114 = nsign_2 * y_2;
        float v115 = nsign_2 * z_2;
        NiPoint3* v18 = &normals_3[numVertices];
        v18->x = v113;
        v18->y = v114;
        v18->z = v115;
        if (apColor) {
            colors_3[numVertices] = *apColor;
        }
        float v19 = t;
        int32_t v20 = numVertices;
        NiPoint2* v21 = textures_3;
        textures_3[numVertices].x = s;
        v21[v20].y = v19;
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
    NiTriShape* v112 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (v112) {
        caps = ::new (v112) NiTriShape(
            numVertices, vertices_3, normals_3, colors_3, textures_3, 1, NiGeometryData::NBT_METHOD_NONE,
            numIndices / 3, indices
        );
    }
    else {
        caps = nullptr;
    }
    caps->CalculateNormals();
    sphere->AttachChild(caps, 1);
    t = 0.0f;
    uint16_t numStrips = aiStacks - 2;
    uint16_t numVerticesPerStrip = 2 * aiSlices + 2;
    uint32_t arraySize = numVerticesPerStrip * static_cast<uint16_t>(aiStacks - 2);
    NiPoint3* v110 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
        arraySize <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * arraySize : SIZE_MAX
    ));
    if (v110) {
        uint32_t v80 = arraySize;
        for (NiPoint3* n = v110; (--v80 & 0x80000000u) == 0; ++n) {
            ;
        }
        vertices_2 = v110;
    }
    else {
        vertices_2 = nullptr;
    }
    vertices_3 = vertices_2;
    NiPoint3* v108 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
        arraySize <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * arraySize : SIZE_MAX
    ));
    if (v108) {
        uint32_t v78 = arraySize;
        for (NiPoint3* ii = v108; (--v78 & 0x80000000u) == 0; ++ii) {
            ;
        }
        normals_2 = v108;
    }
    else {
        normals_2 = nullptr;
    }
    normals_3 = normals_2;
    if (apColor) {
        NiColorA* v51;
        NiColorA* v106 = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(
            arraySize <= SIZE_MAX / sizeof(NiColorA) ? sizeof(NiColorA) * arraySize : SIZE_MAX
        ));
        if (v106) {
            uint32_t v76 = arraySize;
            for (NiColorA* jj = v106; (--v76 & 0x80000000u) == 0; ++jj) {
                jj->r = 0.0f;
                jj->g = 0.0f;
                jj->b = 0.0f;
                jj->a = 0.0f;
            }
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
    NiPoint2* v104 = static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(
        arraySize <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * arraySize : SIZE_MAX
    ));
    if (v104) {
        uint32_t v74 = arraySize;
        for (NiPoint2* kk = v104; (--v74 & 0x80000000u) == 0; ++kk) {
            ;
        }
        textures_2 = v104;
    }
    else {
        textures_2 = nullptr;
    }
    textures_3 = textures_2;
    uint16_t* stripLengths = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(
        numStrips <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * numStrips : SIZE_MAX
    ));
    uint16_t* stripLists = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(
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
            NiPoint3* v22 = &vertices_3[numVertices];
            v22->x = v101;
            v22->y = v102;
            v22->z = v103;
            float v98 = nsign_2 * x_3;
            float v99 = nsign_2 * y_3;
            float v100 = nsign_2 * z_3;
            NiPoint3* v23 = &normals_3[numVertices];
            v23->x = v98;
            v23->y = v99;
            v23->z = v100;
            if (apColor) {
                colors_3[numVertices] = *apColor;
            }
            float v24 = t;
            int32_t v25 = numVertices;
            textures_3[numVertices].x = s;
            textures_3[v25].y = v24;
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
            NiPoint3* v27 = &vertices_3[numVertices];
            v27->x = v95;
            v27->y = v96;
            v27->z = v97;
            float v92 = nsign_2 * x_3;
            float v93 = nsign_2 * y_3;
            float v94 = nsign_2 * z_3;
            NiPoint3* v28 = &normals_3[numVertices];
            v28->x = v92;
            v28->y = v93;
            v28->z = v94;
            if (apColor) {
                colors_3[numVertices] = *apColor;
            }
            float v91 = dt + t;
            int32_t v29 = numVertices;
            textures_3[numVertices].x = s;
            textures_3[v29].y = v91;
            stripLists[numVertices] = numVertices;
            ++numVertices;
            s = s + ds;
        }
        t = t + dt;
        stripLengths[i_2] = numVerticesPerStrip;
    }
    NiTriStrips* v90 = static_cast<NiTriStrips*>(NiMemObject::operator new(sizeof(NiTriStrips)));
    if (v90) {
        strips = ::new (v90) NiTriStrips(
            numVertices, vertices_3, normals_3, colors_3, textures_3, 1, 0, 2 * aiSlices * numStrips, numStrips,
            stripLengths, stripLists
        );
    }
    else {
        strips = nullptr;
    }
    strips->CalculateNormals();
    sphere->AttachChild(strips, 1);
    return (NiAVObject*)sphere;
}

NiAVObject* BSTestObjects::CreateCircle(float afRadius, uint16_t asSegments, const NiColorA* apColor) {
    NiColorA* pcolors_2;
    NiPoint3* pvertices_2;
    NiPoint3* pvertices = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
        asSegments <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * asSegments : SIZE_MAX
    ));
    if (pvertices) {
        int32_t v14 = asSegments;
        for (NiPoint3* i = pvertices; --v14 >= 0; ++i) {
            ;
        }
        pvertices_2 = pvertices;
    }
    else {
        pvertices_2 = nullptr;
    }
    uint8_t* pflags = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(asSegments));
    if (apColor) {
        NiColorA* pcolors;
        NiColorA* v19 = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(
            asSegments <= SIZE_MAX / sizeof(NiColorA) ? sizeof(NiColorA) * asSegments : SIZE_MAX
        ));
        if (v19) {
            int32_t v12 = asSegments;
            for (NiColorA* j = v19; --v12 >= 0; ++j) {
                j->r = 0.0f;
                j->g = 0.0f;
                j->b = 0.0f;
                j->a = 0.0f;
            }
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
        *p_x = v17;
        p_x[1] = stSizeInBytes;
        p_x[2] = 0.0f;
    }
    NiLines* v16 = static_cast<NiLines*>(NiMemObject::operator new(sizeof(NiLines)));
    if (v16) {
        return (NiAVObject*)::new (v16)
            NiLines(asSegments, pvertices_2, pcolors_2, nullptr, 1, NiGeometryData::NBT_METHOD_NONE, pflags);
    }

    return nullptr;
}

NiAVObject* BSTestObjects::CreateArrow(float afLength, const NiColorA* apColor) {
    NiLines* lines;
    NiColorA* color_2;
    NiPoint3* vertex;
    FNVD_INLINED_LOCAL(NiFixedString, v13, std::nullopt, std::nullopt);
    NiPoint3 v19;
    float fsize = afLength / 6.0;
    NiPoint3* v32 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(5 * sizeof(NiPoint3)));
    if (v32) {
        int32_t v11 = 5;
        for (NiPoint3* i = v32; --v11 >= 0; ++i) {
            ;
        }
        vertex = v32;
    }
    else {
        vertex = nullptr;
    }
    float v29 = 0.0f;
    float v30 = 0.0f;
    float v31 = 0.0f;
    vertex->x = 0.0f;
    vertex->y = v30;
    vertex->z = v31;
    float v26 = 0.0f;
    float v27 = 0.0f;
    NiPoint3* v2 = vertex + 1;
    vertex[1].x = 0.0f;
    v2->y = v27;
    v2->z = afLength;
    float v24 = 0.0f;
    float v25 = afLength - fsize;
    NiPoint3* v3 = vertex + 2;
    vertex[2].x = fsize;
    v3->y = v24;
    v3->z = v25;
    float v20 = 0.0f;
    float v21 = 0.0f;
    NiPoint3* v4 = vertex + 3;
    vertex[3].x = 0.0f;
    v4->y = v21;
    v4->z = afLength;
    v19.x = -fsize;
    v19.y = 0.0f;
    v19.z = afLength - fsize;
    vertex[4] = v19;
    if (!apColor) {
        apColor = &BSTestObjects::RED;
    }
    NiColorA* color = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(6 * sizeof(NiColorA)));
    if (color) {
        int32_t v9 = 6;
        for (float* j = &color->r; --v9 >= 0; j += 4) {
            *j = 0.0f;
            j[1] = 0.0f;
            j[2] = 0.0f;
            j[3] = 0.0f;
        }
        color_2 = color;
    }
    else {
        color_2 = nullptr;
    }
    *color_2 = *apColor;
    color_2[1] = *apColor;
    color_2[2] = *apColor;
    color_2[3] = *apColor;
    color_2[4] = *apColor;
    uint8_t* flag = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(6u));
    *flag = 1;
    flag[1] = 1;
    flag[2] = 0;
    flag[3] = 1;
    flag[4] = 0;
    NiGeometry* v14 = static_cast<NiGeometry*>(NiMemObject::operator new(sizeof(NiLines)));
    if (v14) {
        lines = ::new (v14) NiLines(5, vertex, color_2, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, flag);
    }
    else {
        lines = nullptr;
    }
    lines->m_kLocal.m_Translate = NiPoint3_ZERO;
    lines->m_kLocal.m_Rotate = NiMatrix3_IDENTITY;
    if ("BSTestObjects Arrow") {
        v13.m_kHandle = NiGlobalStringTable::AddString("BSTestObjects Arrow");
    }
    else {
        v13.m_kHandle = NiGlobalStringTable::NULL_STRING;
    }
    lines->SetName(v13);
    // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
    NiGlobalStringTable::ReleaseHandle(v13.GetHandle());
    return lines;
}

NiPoint3* NiPoint3::Perpendicular(NiPoint3* apResult) {
    float v8 = std::fabs(static_cast<double>(x));
    float v7 = std::fabs(static_cast<double>(y));
    if (!(v7 > static_cast<double>(v8))) {
        float v4 = std::fabs(static_cast<double>(z));
        float v3 = std::fabs(static_cast<double>(y));
        if (!(v3 > static_cast<double>(v4))) {
            ::new (apResult) NiPoint3(UnitCross(NiPoint3_UNIT_Y));
        }
        else {
            ::new (apResult) NiPoint3(UnitCross(NiPoint3_UNIT_Z));
        }
        return apResult;
    }

    float v6 = std::fabs(static_cast<double>(z));
    float v5 = std::fabs(static_cast<double>(x));
    if (!(v5 > static_cast<double>(v6))) {
        ::new (apResult) NiPoint3(UnitCross(NiPoint3_UNIT_X));
    }
    else {
        ::new (apResult) NiPoint3(UnitCross(NiPoint3_UNIT_Z));
    }
    return apResult;
}
