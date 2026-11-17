#include "Fallout Shared/BGSPrimitive.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundOBB.h"
#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/BSMultiBoundSphere.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortal.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "Fallout Shared/BGSPrimitiveSphere.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehvec.h"

#include <cmath>

BGSPrimitive::BGSPrimitive()
  : Color(0.0f, 0.0f, 0.0f, 0.0f), spGeometry(nullptr), spFrame(nullptr), spNode(nullptr), iScaleHandlesEnabled(7) {}

BGSPrimitive::~BGSPrimitive() {
    ClearObjects();
}

// The base primitive makes no multibound shape.
BSMultiBoundShape* BGSPrimitive::MakeMultiBoundShape(NiPoint3& akAngle) {
    return nullptr;
}

void BGSPrimitive::ClearObjects() {
    (&spGeometry)->operator=(nullptr);
    (&spFrame)->operator=(nullptr);
    (&spNode)->operator=(nullptr);
}

BGSPrimitive* BGSPrimitive::CreateAbstract(PRIMITIVE_TYPE eType, NiPoint3& aRadii, NiColorA& aColor) {
    switch (eType) {
        case PRIMITIVE_PLANE: {
            NiPoint2 kSize = NiPoint2(aRadii.x, aRadii.z) * 2.0f;
            return CreatePlane(kSize, aColor);
        }
        case PRIMITIVE_BOX: {
            NiPoint3 kSize = aRadii * 2.0f;
            return CreateBox(kSize, aColor);
        }
        case PRIMITIVE_SPHERE: return CreateSphere(aRadii.x, aColor);
        default: return nullptr;
    }
}

BGSPrimitive* BGSPrimitive::Copy(BGSPrimitive* apCopy) {
    NiPoint3 v10;
    NiPoint3 v11;
    FNVD_INLINED_LOCAL(NiColorA, color_3, std::nullopt, std::nullopt);
    NiPoint3 radii_2;
    FNVD_INLINED_LOCAL(NiColorA, color_2, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiColorA, colorTmp, std::nullopt, std::nullopt);
    BGSPrimitive* pprimitive = nullptr;
    if (apCopy->eType == BGSPrimitive::PRIMITIVE_PLANE) {
        NiPoint3 radii;
        FNVD_INLINED_LOCAL(NiColorA, color, std::nullopt, std::nullopt);
        radii = apCopy->GetRadii() * 2.0f;
        color = apCopy->GetColor();
        NiPoint3* v2 = &(v11 = apCopy->GetRadii());
        float v8 = v2->z + v2->z;
        NiPoint3* v3 = &(v10 = apCopy->GetRadii());
        float v7 = v3->x + v3->x;
        NiPoint2 pt2Tmp(v7, v8);
        colorTmp = apCopy->GetColor();
        pprimitive = BGSPrimitive::CreatePlane(pt2Tmp, colorTmp);
    }
    if (apCopy->eType == BGSPrimitive::PRIMITIVE_BOX) {
        radii_2 = apCopy->GetRadii() * 2.0f;
        color_2 = apCopy->GetColor();
        pprimitive = BGSPrimitive::CreateBox(radii_2, color_2);
    }
    if (apCopy->eType == BGSPrimitive::PRIMITIVE_SPHERE) {
        color_3 = apCopy->GetColor();
        float afRadius = apCopy->Radii.x;
        return BGSPrimitive::CreateSphere(afRadius, color_3);
    }
    return pprimitive;
}

BGSPrimitivePlane* BGSPrimitive::CreatePlane(NiPoint2& aSize, NiColorA& aColor) {
    BGSPrimitivePlane* v3;
    void* v4 = ourMalloc(sizeof(BGSPrimitivePlane));
    if (v4) {
        v3 = ::new (v4) BGSPrimitivePlane;
    }
    else {
        v3 = nullptr;
    }
    v3->SetColor(aColor);
    v3->Radii.x = (float)((&aSize)->x * 0.5);
    v3->Radii.y = (float)1.0;
    v3->Radii.z = (float)((&aSize)->y * 0.5);
    v3->eType = BGSPrimitive::PRIMITIVE_PLANE;
    return v3;
}

BGSPrimitiveBox* BGSPrimitive::CreateBox(NiPoint3& aSize, NiColorA& aColor) {
    BGSPrimitiveBox* v3;
    NiPoint3 result;
    void* v4 = ourMalloc(sizeof(BGSPrimitiveBox));
    int32_t v6 = 0;
    if (v4) {
        v3 = ::new (v4) BGSPrimitiveBox;
    }
    else {
        v3 = nullptr;
    }
    v6 = (-1);
    v3->SetColor(aColor);
    (result = (&aSize)->operator*(0x1p-1f));
    v3->SetRadii(result);
    v3->eType = BGSPrimitive::PRIMITIVE_BOX;
    return v3;
}

BGSPrimitiveSphere* BGSPrimitive::CreateSphere(float afRadius, NiColorA& aColor) {
    BGSPrimitiveSphere* pSphere = static_cast<BGSPrimitiveSphere*>(ourMalloc(sizeof(BGSPrimitiveSphere)));
    if (pSphere) {
        pSphere = ::new (pSphere) BGSPrimitiveSphere;
    }
    else {
        pSphere = nullptr;
    }
    pSphere->SetColor(aColor);
    pSphere->SetRadius(afRadius);
    pSphere->eType = BGSPrimitive::PRIMITIVE_SPHERE;
    return pSphere;
}

BGSPrimitivePlane::BGSPrimitivePlane() {
    eType = BGSPrimitive::PRIMITIVE_PLANE;
}

BGSPrimitivePlane::~BGSPrimitivePlane() {}

void BGSPrimitivePlane::Build() {
    BSFadeNode* v4;
    NiFixedString apString;
    void* v7 = NiMemObject::operator new(sizeof(BSFadeNode));
    int32_t v9 = 0;
    if (v7) {
        v4 = ::new (v7) BSFadeNode;
    }
    else {
        v4 = nullptr;
    }
    NiRefObject* v8 = v4;
    v9 = (-1);
    (&spNode)->operator=(v4);
    NiFixedString* a2 = ::new (static_cast<void*>(&apString)) NiFixedString(const_cast<char*>("Plane"));
    v9 = 1;
    BSFadeNode* v1 = (&spNode)->m_pObject;
    v1->SetName(*a2);
    v9 = (-1);
    BSFadeNode* v2 = (&spNode)->m_pObject;
    v2->UpdateProperties();
}

void BGSPrimitivePlane::SetColor(NiColorA& aColor) {
    NiTriStrips* v2;
    NiAVObject* v3;
    uint8_t v4;
    NiColorA* processLevel;
    NiGeometryData* modelData;
    Color = aColor;
    if ((&spGeometry)->m_pObject) {
        v2 = (&spGeometry)->m_pObject;
        modelData = v2->GetModelData();
        processLevel = modelData->GetColors();
        processLevel[7] = Color;
        processLevel[6] = processLevel[7];
        processLevel[5] = processLevel[6];
        processLevel[4] = processLevel[5];
        processLevel[3] = processLevel[4];
        processLevel[2] = processLevel[3];
        processLevel[1] = processLevel[2];
        *processLevel = processLevel[1];
        modelData->MarkAsChanged(4u);
    }
    NiRefObject* vCondition1;
    vCondition1 = (&spFrame)->operator->();
    if (vCondition1) {
        v4 = (uint8_t)((&aColor)->a < 0.001);
        v3 = static_cast<NiAVObject*>((&spFrame)->operator->());
        v3->SetAppCulled(v4);
    }
}

void BGSPrimitivePlane::SetRadii(NiPoint3& aRadii) {
    NiPoint3 v73;
    NiPoint3 v74;
    NiPoint3 v75;
    NiPoint3 v76;
    NiPoint3 v77;
    NiPoint3 v78;
    NiPoint3 v79;
    NiPoint3 v80;
    NiPoint3 v81;
    NiPoint3 v82;
    NiPoint3 v83;
    NiPoint3 v84;
    NiPoint3 v85;
    NiPoint3 v86;
    NiPoint3 v87;
    NiPoint3 dst;
    float* v2 = &aRadii.operator[](0);
    float v71 = std::fabs(*v2);
    aRadii.operator[](0) = v71;
    aRadii.operator[](1) = 0.0f;
    float* v3 = &aRadii.operator[](2);
    float v70 = std::fabs(*v3);
    aRadii.operator[](2) = v70;
    Radii = aRadii;
    if (spGeometry.m_pObject) {
        NiGeometryData* pgeomdata = spGeometry.m_pObject->GetModelData();
        NiPoint3* pverts = pgeomdata->m_pkVertex;
        float a1 = Radii.operator[](2);
        float y = Radii.operator[](1);
        float x = -Radii.operator[](0);
        NiPoint3* v4 = ::new (&dst) NiPoint3(x, y, a1);
        *pverts = *v4;
        float a1a = Radii.operator[](2);
        float ya = Radii.operator[](1);
        float* v5 = &Radii.operator[](0);
        NiPoint3* v6 = ::new (&v87) NiPoint3(*v5, ya, a1a);
        pverts[1] = *v6;
        float a1b = Radii.operator[](2);
        float yb = -Radii.operator[](1);
        float* v7 = &Radii.operator[](0);
        NiPoint3* v8 = ::new (&v86) NiPoint3(*v7, yb, a1b);
        pverts[2] = *v8;
        float a1c = Radii.operator[](2);
        float yc = -Radii.operator[](1);
        float xa = -Radii.operator[](0);
        NiPoint3* v9 = ::new (&v85) NiPoint3(xa, yc, a1c);
        pverts[3] = *v9;
        float a1d = -Radii.operator[](2);
        float yd = Radii.operator[](1);
        float xb = -Radii.operator[](0);
        NiPoint3* v10 = ::new (&v84) NiPoint3(xb, yd, a1d);
        pverts[4] = *v10;
        float a1e = -Radii.operator[](2);
        float ye = Radii.operator[](1);
        float* v11 = &Radii.operator[](0);
        NiPoint3* v12 = ::new (&v83) NiPoint3(*v11, ye, a1e);
        pverts[5] = *v12;
        float a1f = -Radii.operator[](2);
        float yf = -Radii.operator[](1);
        float* v13 = &Radii.operator[](0);
        NiPoint3* v14 = ::new (&v82) NiPoint3(*v13, yf, a1f);
        pverts[6] = *v14;
        float a1g = -Radii.operator[](2);
        float yg = -Radii.operator[](1);
        float xc = -Radii.operator[](0);
        NiPoint3* v15 = ::new (&v81) NiPoint3(xc, yg, a1g);
        pverts[7] = *v15;
        pgeomdata->MarkAsChanged(1);
    }
    if (spFrame.m_pObject) {
        NiGeometryData* pgeomdata_2 = static_cast<NiGeometry*>(spFrame.m_pObject)->GetModelData();
        NiPoint3* pverts_2 = pgeomdata_2->m_pkVertex;
        float a1h = Radii.operator[](2);
        float yh = Radii.operator[](1);
        float xd = -Radii.operator[](0);
        NiPoint3* v16 = ::new (&v80) NiPoint3(xd, yh, a1h);
        *pverts_2 = *v16;
        float a1i = Radii.operator[](2);
        float yi = Radii.operator[](1);
        float* v17 = &Radii.operator[](0);
        NiPoint3* v18 = ::new (&v79) NiPoint3(*v17, yi, a1i);
        pverts_2[1] = *v18;
        float a1j = Radii.operator[](2);
        float yj = -Radii.operator[](1);
        float* v19 = &Radii.operator[](0);
        NiPoint3* v20 = ::new (&v78) NiPoint3(*v19, yj, a1j);
        pverts_2[2] = *v20;
        float a1k = Radii.operator[](2);
        float yk = -Radii.operator[](1);
        float xe = -Radii.operator[](0);
        NiPoint3* v21 = ::new (&v77) NiPoint3(xe, yk, a1k);
        pverts_2[3] = *v21;
        pverts_2[4] = *pverts_2;
        float a1l = -Radii.operator[](2);
        float yl = Radii.operator[](1);
        float xf = -Radii.operator[](0);
        NiPoint3* v22 = ::new (&v76) NiPoint3(xf, yl, a1l);
        pverts_2[5] = *v22;
        float a1m = -Radii.operator[](2);
        float ym = -Radii.operator[](1);
        float xg = -Radii.operator[](0);
        NiPoint3* v23 = ::new (&v75) NiPoint3(xg, ym, a1m);
        pverts_2[6] = *v23;
        float a1n = -Radii.operator[](2);
        float yn = -Radii.operator[](1);
        float* v24 = &Radii.operator[](0);
        NiPoint3* v25 = ::new (&v74) NiPoint3(*v24, yn, a1n);
        pverts_2[7] = *v25;
        float a1o = -Radii.operator[](2);
        float yo = Radii.operator[](1);
        float* v26 = &Radii.operator[](0);
        NiPoint3* v27 = ::new (&v73) NiPoint3(*v26, yo, a1o);
        pverts_2[8] = *v27;
        pverts_2[9] = pverts_2[5];
        pverts_2[10] = pverts_2[3];
        pverts_2[11] = pverts_2[6];
        pverts_2[12] = pverts_2[1];
        pverts_2[13] = pverts_2[8];
        pverts_2[14] = pverts_2[2];
        pverts_2[15] = pverts_2[7];
        pgeomdata_2->MarkAsChanged(1);
    }
    NiUpdateData UpdateData(0.0f, false, false);
    if (spNode.m_pObject) {
        float fRad = aRadii.x;
        if (aRadii.x < static_cast<double>(aRadii.y) && aRadii.z < static_cast<double>(aRadii.y)) {
            fRad = aRadii.y;
        }
        if (aRadii.y < static_cast<double>(aRadii.z) && aRadii.x < static_cast<double>(aRadii.z)) {
            fRad = aRadii.z;
        }
        NiBound kBound;
        kBound.SetCenterAndRadius(NiPoint3_ZERO, fRad);
        spGeometry.m_pObject->SetModelBound(kBound);
        spNode.m_pObject->Update(UpdateData);
        BSFadeNode* v69 = spNode.m_pObject;
        v69->UpdateWorldData(UpdateData);
        spGeometry.m_pObject->UpdateProperties();
        BSFadeNode* v68 = spNode.m_pObject;
        v68->UpdateWorldBound();
    }
}

// Whether aPoint lies within the plane's radii on each axis; a NaN coordinate counts as inside, as the game does.
bool BGSPrimitivePlane::IsInside(NiPoint3& aPoint) {
    return !(aPoint.x < -Radii.x) && !(aPoint.y < -Radii.y) && !(aPoint.z < -Radii.z) && !(Radii.x < aPoint.x) &&
        !(Radii.y < aPoint.y) && !(Radii.z < aPoint.z);
}

BSOcclusionPlane* BGSPrimitivePlane::MakeOcclusionPlane() {
    NiPoint2 v5;
    void* v6 = NiMemObject::operator new(sizeof(BSOcclusionPlane));
    int32_t v7 = 0;
    if (!v6) {
        return nullptr;
    }
    NiPoint2* v1 = ::new (&v5) NiPoint2(Radii.x, Radii.z);
    return ::new (v6) BSOcclusionPlane(&NiPoint3_ZERO, v1, &NiMatrix3_IDENTITY);
}

BSPortal* BGSPrimitivePlane::MakePortal() {
    NiPoint2 v5;
    void* v6 = NiMemObject::operator new(sizeof(BSPortal));
    int32_t v7 = 0;
    if (!v6) {
        return nullptr;
    }
    NiPoint2* bounds = ::new (&v5) NiPoint2(Radii.x, Radii.z);
    return ::new (v6) BSPortal(&NiPoint3_ZERO, bounds, &NiMatrix3_IDENTITY);
}

BGSPrimitiveBox::BGSPrimitiveBox() {
    eType = BGSPrimitive::PRIMITIVE_BOX;
}

BGSPrimitiveBox::~BGSPrimitiveBox() {}

void BGSPrimitiveBox::Build() {
    BSFadeNode* v1;
    BSFadeNode* v2;
    NiFixedString* a2;
    BSFadeNode* v4;
    NiFixedString apString;
    void* v7;
    NiRefObject* v8;
    int32_t v9;
    if (!(&spNode)->m_pObject) {
        v7 = NiMemObject::operator new(sizeof(BSFadeNode));
        v9 = 0;
        if (v7) {
            v4 = ::new (v7) BSFadeNode;
        }
        else {
            v4 = nullptr;
        }
        v8 = v4;
        v9 = (-1);
        (&spNode)->operator=(v4);
        a2 = ::new (static_cast<void*>(&apString)) NiFixedString(const_cast<char*>("Box"));
        v9 = 1;
        v1 = (&spNode)->m_pObject;
        v1->SetName(*a2);
        v9 = (-1);
        v2 = (&spNode)->m_pObject;
        v2->UpdateProperties();
    }
}

void BGSPrimitiveBox::SetColor(NiColorA& a1) {
    // Copies the colour one channel at a time, each read just before its store (a reference that overlaps Color reads
    // the channels already written); C++'s copy assignment may read them all first.
    Color.r = a1.r;
    Color.g = a1.g;
    Color.b = a1.b;
    Color.a = a1.a;
    if (spGeometry.m_pObject) {
        NiGeometryData* pgeomdata = spGeometry.m_pObject->GetModelData();
        NiColorA* Colors = pgeomdata->m_pkColor;
        for (int32_t i = 0; i < pgeomdata->m_usVertices; ++i) {
            float* p_r = &Colors[i].r;
            *p_r = Color.r;
            p_r[1] = Color.g;
            p_r[2] = Color.b;
            p_r[3] = Color.a;
        }
        pgeomdata->MarkAsChanged(4);
        NiGeometryData* pgeomdata_2 = static_cast<NiGeometry*>(spFrame.m_pObject)->GetModelData();
        NiColorA* v7 = pgeomdata_2->m_pkColor;
        int32_t v11 = 0;
        for (uint16_t j = pgeomdata_2->m_usVertices; v11 < j; j = pgeomdata_2->m_usVertices) {
            float* v4 = &v7[v11].r;
            *v4 = Color.r;
            v4[1] = Color.g;
            v4[2] = Color.b;
            v4[3] = Color.a;
            ++v11;
        }
        pgeomdata_2->MarkAsChanged(4);
        static_cast<NiAVObject*>(spFrame.m_pObject)->NiAVObject::SetAppCulled(a1.a < 0.001);
    }
}

// Makes aRadii positive and stores it, moves the box's eight corners to +-Radii, scales the frame's unit vertices by
// the radii (each at least 0.01) and refits the node's bound to a sphere of the largest radius around the origin. Each
// corner's z, then y, then x is read through NiPoint3::operator[] before constructing it. NiGeometry::GetModelData is
// called on spFrame, so the frame is a geometry.
void BGSPrimitiveBox::SetRadii(NiPoint3& aRadii) {
    float fAbs = std::fabs(aRadii[0]);
    aRadii[0] = fAbs;
    fAbs = std::fabs(aRadii[1]);
    aRadii[1] = fAbs;
    fAbs = std::fabs(aRadii[2]);
    aRadii[2] = fAbs;
    Radii = aRadii;
    if (spGeometry.operator->()) {
        NiGeometryData* pData = spGeometry->GetModelData();
        NiPoint3* pVertices = pData->GetVertices();
        float fZ = Radii[2];
        float fY = Radii[1];
        float fX = -Radii[0];
        pVertices[0] = NiPoint3(fX, fY, fZ);
        fZ = Radii[2];
        fY = Radii[1];
        fX = Radii[0];
        pVertices[1] = NiPoint3(fX, fY, fZ);
        fZ = Radii[2];
        fY = -Radii[1];
        fX = Radii[0];
        pVertices[2] = NiPoint3(fX, fY, fZ);
        fZ = Radii[2];
        fY = -Radii[1];
        fX = -Radii[0];
        pVertices[3] = NiPoint3(fX, fY, fZ);
        fZ = -Radii[2];
        fY = Radii[1];
        fX = -Radii[0];
        pVertices[4] = NiPoint3(fX, fY, fZ);
        fZ = -Radii[2];
        fY = Radii[1];
        fX = Radii[0];
        pVertices[5] = NiPoint3(fX, fY, fZ);
        fZ = -Radii[2];
        fY = -Radii[1];
        fX = Radii[0];
        pVertices[6] = NiPoint3(fX, fY, fZ);
        fZ = -Radii[2];
        fY = -Radii[1];
        fX = -Radii[0];
        pVertices[7] = NiPoint3(fX, fY, fZ);
        pData->MarkAsChanged(NiGeometryData::VERTEX_MASK);
    }
    if (spFrame.operator->()) {
        NiGeometryData* pFrameData = static_cast<NiGeometry*>(spFrame.operator->())->GetModelData();
        NiPoint3* pFrameVertices = pFrameData->GetVertices();
        NiPoint3 kScale = aRadii;
        // Each fabs is compared against the double 0.009999999776482582 and the float 0.01 is stored.
        if (std::fabs(kScale.x) < 0.009999999776482582) {
            kScale.x = 0.01f;
        }
        if (std::fabs(kScale.y) < 0.009999999776482582) {
            kScale.y = 0.01f;
        }
        if (std::fabs(kScale.z) < 0.009999999776482582) {
            kScale.z = 0.01f;
        }
        for (int32_t i = 0; i < pFrameData->GetVertexCount(); ++i) {
            // Each coordinate becomes its sign: -1.0f below the double 0.0, else 1.0f (NaN too).
            float fSign;
            if (pFrameVertices[i].x < 0.0) {
                fSign = -1.0f;
            }
            else {
                fSign = 1.0f;
            }
            pFrameVertices[i].x = fSign;
            if (pFrameVertices[i].y < 0.0) {
                fSign = -1.0f;
            }
            else {
                fSign = 1.0f;
            }
            pFrameVertices[i].y = fSign;
            if (pFrameVertices[i].z < 0.0) {
                fSign = -1.0f;
            }
            else {
                fSign = 1.0f;
            }
            pFrameVertices[i].z = fSign;
            pFrameVertices[i].x = pFrameVertices[i].x * kScale.x;
            pFrameVertices[i].y = pFrameVertices[i].y * kScale.y;
            pFrameVertices[i].z = pFrameVertices[i].z * kScale.z;
            pFrameData->MarkAsChanged(NiGeometryData::VERTEX_MASK);
        }
    }
    alignas(NiUpdateData) uint8_t aUpdateStorage[sizeof(NiUpdateData)];
    NiUpdateData* pUpdateData = ::new (aUpdateStorage) NiUpdateData(0.0f, false, false);
    if (spNode.operator->()) {
        float fRadius = aRadii.x;
        if (aRadii.x < aRadii.y && aRadii.z < aRadii.y) {
            fRadius = aRadii.y;
        }
        if (aRadii.y < aRadii.z && aRadii.x < aRadii.z) {
            fRadius = aRadii.z;
        }
        alignas(NiBound) uint8_t aBoundStorage[sizeof(NiBound)];
        NiBound* pBound = ::new (aBoundStorage) NiBound;
        pBound->SetCenterAndRadius(NiPoint3_ZERO, fRadius);
        spGeometry->SetModelBound(*pBound);
        spNode->Update(*pUpdateData);
        spGeometry->UpdateProperties();
        NiAVObject* pNode = spNode.operator->();
        pNode->UpdateWorldBound();
    }
}

bool BGSPrimitiveBox::IsInside(NiPoint3& a2) {
    return (bool)((uint8_t)((!((&a2)->x < (-Radii.x))) &&
                            ((!((&a2)->y < (-Radii.y))) &&
                             ((!((&a2)->z < (-Radii.z))) &&
                              ((!(Radii.x < ((double)(&a2)->x))) &&
                               ((!(Radii.y < ((double)(&a2)->y))) && (!(Radii.z < ((double)(&a2)->z)))))))));
}

BSMultiBoundShape* BGSPrimitiveBox::MakeMultiBoundShape(NiPoint3& a1) {
    bool v10 = std::fabs(a1.x) > 0.009999999776482582;
    if (std::fabs(a1.y) > 0.009999999776482582) {
        v10 = true;
    }
    if (std::fabs(a1.z) > 0.009999999776482582) {
        v10 = true;
    }
    if (v10) {
        BSMultiBoundOBB* v4;
        NiMatrix3 ecx0;
        BSMultiBoundOBB* apMemory = static_cast<BSMultiBoundOBB*>(NiMemObject::operator new(sizeof(BSMultiBoundOBB)));
        if (apMemory) {
            v4 = ::new (apMemory) BSMultiBoundOBB(&NiPoint3_ZERO, &Radii, &NiMatrix3_IDENTITY);
        }
        else {
            v4 = nullptr;
        }
        ecx0.FromEulerAnglesXYZ(a1.x, a1.y, a1.z);
        v4->SetAxis(ecx0);
        return v4;
    }
    else {
        BSMultiBoundAABB* v6 = static_cast<BSMultiBoundAABB*>(NiMemObject::operator new(sizeof(BSMultiBoundAABB)));
        if (v6) {
            return ::new (v6) BSMultiBoundAABB(&NiPoint3_ZERO, &Radii);
        }
        else {
            return nullptr;
        }
    }
}

BSMultiBoundSphere::~BSMultiBoundSphere() {}

BSOcclusionPlane::~BSOcclusionPlane() {}

BGSPrimitiveSphere::BGSPrimitiveSphere() {
    eType = BGSPrimitive::PRIMITIVE_SPHERE;
}

BGSPrimitiveSphere::~BGSPrimitiveSphere() {}

void BGSPrimitiveSphere::Build() {
    FNVD_INLINED_LOCAL(NiFixedString, v17, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    if (spGeometry.m_pObject) {
        float* v3 = &Radii.operator[](0);
        SetRadius(*v3);
        SetColor(Color);
    }
    else {
        BSFadeNode* BSFadeNode_;
        void* NiAlphaProperty_;
        BSShaderProperty* pshprop;
        NiTriStrips* ptri;
        NiPoint3* pverts;
        NiColorA* pcolors;
        void* ptr = ourMalloc(112 * sizeof(NiColorA));
        if (ptr) {
            rt::eh_vector_constructor_iterator(ptr, sizeof(NiColorA), 112, &fnvd::constructElement<NiColorA>);
            pcolors = static_cast<NiColorA*>(ptr);
        }
        else {
            pcolors = nullptr;
        }
        void* v29 = ourMalloc(112 * sizeof(NiPoint3));
        if (v29) {
            rt::eh_vector_constructor_iterator(v29, sizeof(NiPoint3), 112, &emptyPointConstructor);
            pverts = static_cast<NiPoint3*>(v29);
        }
        else {
            pverts = nullptr;
        }
        uint16_t* pstrip = static_cast<uint16_t*>(ourMalloc(112 * sizeof(uint16_t)));
        for (int32_t i = 0; i < 112; ++i) {
            float v13 = pSphereVertices[i][0];
            pverts[i].x = v13 * Radii.x;
            float v12 = pSphereVertices[i][1];
            pverts[i].y = v12 * Radii.x;
            float v11 = pSphereVertices[i][2];
            pverts[i].z = v11 * Radii.x;
            NiColorA* v1 = &pcolors[i];
            v1->r = Color.r;
            v1->g = Color.g;
            v1->b = Color.b;
            v1->a = Color.a;
            pstrip[i] = i;
        }
        uint16_t* l = static_cast<uint16_t*>(ourMalloc(sizeof(uint16_t)));
        *l = 112;
        NiTriStrips* v25 = static_cast<NiTriStrips*>(NiMemObject::operator new(sizeof(NiTriStrips)));
        if (v25) {
            ptri = ::new (v25) NiTriStrips(112, pverts, nullptr, pcolors, nullptr, 0, 0, 110, 1, l, pstrip);
        }
        else {
            ptri = nullptr;
        }
        BSShaderNoLightingProperty* v23 =
            static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
        if (v23) {
            pshprop = ::new (v23) BSShaderNoLightingProperty;
        }
        else {
            pshprop = nullptr;
        }
        BSShaderNoLightingProperty* v36 = static_cast<BSShaderNoLightingProperty*>(pshprop);
        pshprop->SetShader(33);
        ptri->AttachProperty(pshprop);
        BSShaderManager::PrepareObject(ptri, 0, 0);
        spGeometry.operator=(ptri);
        void* apMemory = NiMemObject::operator new(sizeof(NiAlphaProperty));
        if (apMemory) {
            NiAlphaProperty_ = ::new (apMemory) NiAlphaProperty;
        }
        else {
            NiAlphaProperty_ = nullptr;
        }
        NiRefObject* v22 = static_cast<NiRefObject*>(NiAlphaProperty_);
        NiPointer<NiAlphaProperty> alpha(static_cast<NiAlphaProperty*>(NiAlphaProperty_));
        NiFixedString* a2 = ::new (static_cast<void*>(&apString)) NiFixedString("primitive: alpha");
        alpha.m_pObject->SetName(*a2);
        apString.~NiFixedString();
        alpha.m_pObject->SetAlphaBlending(true);
        spGeometry.m_pObject->AttachProperty(alpha.m_pObject);
        BSShaderProperty* Property = static_cast<BSShaderProperty*>(spGeometry.m_pObject->GetProperty(3));
        Property->SetFlag(32u, 0);
        BSFadeNode* v18 = static_cast<BSFadeNode*>(NiMemObject::operator new(sizeof(BSFadeNode)));
        if (v18) {
            BSFadeNode_ = ::new (v18) BSFadeNode;
        }
        else {
            BSFadeNode_ = nullptr;
        }
        NiRefObject* v19 = BSFadeNode_;
        spNode.operator=(BSFadeNode_);
        BSFadeNode* v5 = spNode.m_pObject;
        v5->AttachChild(spGeometry.m_pObject, 1);
        NiFixedString* kName = ::new (static_cast<void*>(&v17)) NiFixedString("Sphere");
        spNode.m_pObject->SetName(*kName);
        v17.~NiFixedString();
        spNode.m_pObject->UpdateProperties();
        spGeometry.m_pObject->UpdateProperties();
    }
    NiUpdateData UpdateData(0.0f, false, false);
    spNode.m_pObject->Update(UpdateData);
}

void BGSPrimitiveSphere::SetColor(NiColorA& aColor) {
    NiTriStrips* v2;
    int32_t i;
    NiColorA* processLevel;
    NiGeometryData* modelData;
    Color = aColor;
    if ((&spGeometry)->m_pObject) {
        v2 = (&spGeometry)->m_pObject;
        modelData = v2->GetModelData();
        processLevel = modelData->GetColors();
        for (i = 0; i < 112; ++i) {
            processLevel[i] = Color;
        }
        modelData->MarkAsChanged(4u);
    }
}

void BGSPrimitiveSphere::SetRadii(NiPoint3& aRadii) {
    SetRadius((&aRadii)->x);
}

bool BGSPrimitiveSphere::IsInside(NiPoint3& aPoint) {
    double v3 = (double)(Radii.x * Radii.x);
    return (bool)((uint8_t)((&aPoint)->SqrLength() < v3));
}

BSMultiBoundShape* BGSPrimitiveSphere::MakeMultiBoundShape(NiPoint3& akAngle) {
    NiPoint3 dst;
    void* apMemory = NiMemObject::operator new(sizeof(BSMultiBoundSphere));
    if (!apMemory) {
        return nullptr;
    }
    float radius = Radii.x;
    NiPoint3* v2 = ::new (&dst) NiPoint3(0.0f, 0.0f, 0.0f);
    return ::new (apMemory) BSMultiBoundSphere(v2, radius);
}

void BGSPrimitiveSphere::SetRadius(float afRadius) {
    NiPoint3 dst;
    float fRadius = std::fabs(afRadius);
    if (spGeometry.m_pObject) {
        NiGeometryData* pgeomdata = spGeometry.m_pObject->GetModelData();
        NiPoint3* pverts = pgeomdata->m_pkVertex;
        for (int32_t i = 0; i < 112; ++i) {
            float v6 = pSphereVertices[i][0];
            pverts[i].x = v6;
            float v5 = pSphereVertices[i][1];
            pverts[i].y = v5;
            float v4 = pSphereVertices[i][2];
            pverts[i].z = v4;
            pverts[i].operator*=(fRadius);
        }
        pgeomdata->MarkAsChanged(1);
    }
    Radii.z = fRadius;
    Radii.y = fRadius;
    Radii.x = fRadius;
    NiUpdateData UpdateData(0.0f, false, false);
    if (spNode.m_pObject) {
        NiBound kBound;
        NiPoint3* v2 = ::new (&dst) NiPoint3(0.0f, 0.0f, 0.0f);
        kBound.SetCenterAndRadius(*v2, fRadius);
        spGeometry.m_pObject->SetModelBound(kBound);
        spNode.m_pObject->Update(UpdateData);
        spGeometry.m_pObject->UpdateProperties();
        BSFadeNode* v3 = spNode.m_pObject;
        v3->UpdateWorldBound();
    }
}

void NiGeometry::SetModelBound(const NiBound& kBound) {
    m_spModelData.m_pObject->SetBound(kBound);
}

void NiGeometryData::SetBound(const NiBound& kBound) {
    m_kBound.Copy(&kBound);
}

NiColorA* NiGeometryData::GetColors() const {
    return m_pkColor;
}
