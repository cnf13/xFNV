#include "Fallout Shared/BGSPrimitive.h"

#include "BSCore/NewOverloads.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

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
    spGeometry = nullptr;
    spFrame = nullptr;
    spNode = nullptr;
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
    BGSPrimitive* pPrimitive = nullptr;
    if (apCopy->eType == BGSPrimitive::PRIMITIVE_PLANE) {
        NiPoint3 kRadii = apCopy->GetRadii();
        float fHeight = kRadii.z + kRadii.z;
        kRadii = apCopy->GetRadii();
        float fWidth = kRadii.x + kRadii.x;
        NiPoint2 pt2Tmp(fWidth, fHeight);
        NiColorA colorTmp = apCopy->GetColor();
        pPrimitive = BGSPrimitive::CreatePlane(pt2Tmp, colorTmp);
    }
    if (apCopy->eType == BGSPrimitive::PRIMITIVE_BOX) {
        NiPoint3 radii = apCopy->GetRadii() * 2.0f;
        NiColorA color = apCopy->GetColor();
        pPrimitive = BGSPrimitive::CreateBox(radii, color);
    }
    if (apCopy->eType == BGSPrimitive::PRIMITIVE_SPHERE) {
        NiColorA color = apCopy->GetColor();
        float fRadius = apCopy->Radii.x;
        return BGSPrimitive::CreateSphere(fRadius, color);
    }
    return pPrimitive;
}

BGSPrimitivePlane* BGSPrimitive::CreatePlane(NiPoint2& aSize, NiColorA& aColor) {
    BGSPrimitivePlane* pPlane = new BGSPrimitivePlane;
    pPlane->SetColor(aColor);
    pPlane->Radii.x = static_cast<float>(aSize.x * 0.5);
    pPlane->Radii.y = 1.0f;
    pPlane->Radii.z = static_cast<float>(aSize.y * 0.5);
    pPlane->eType = BGSPrimitive::PRIMITIVE_PLANE;
    return pPlane;
}

BGSPrimitiveBox* BGSPrimitive::CreateBox(NiPoint3& aSize, NiColorA& aColor) {
    BGSPrimitiveBox* pBox = new BGSPrimitiveBox;
    pBox->SetColor(aColor);
    NiPoint3 kRadii = aSize * 0.5f;
    pBox->SetRadii(kRadii);
    pBox->eType = BGSPrimitive::PRIMITIVE_BOX;
    return pBox;
}

BGSPrimitiveSphere* BGSPrimitive::CreateSphere(float afRadius, NiColorA& aColor) {
    BGSPrimitiveSphere* pSphere = new BGSPrimitiveSphere;
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
    NiFixedString kName;
    BSFadeNode* pNode = new BSFadeNode;
    spNode = pNode;
    NiFixedString* pName = ::new (static_cast<void*>(&kName)) NiFixedString(const_cast<char*>("Plane"));
    spNode->SetName(*pName);
    spNode->UpdateProperties();
}

void BGSPrimitivePlane::SetColor(NiColorA& aColor) {
    Color = aColor;
    if (spGeometry.m_pObject) {
        NiGeometryData* pModelData = spGeometry->GetModelData();
        NiColorA* pColors = pModelData->GetColors();
        pColors[7] = Color;
        pColors[6] = pColors[7];
        pColors[5] = pColors[6];
        pColors[4] = pColors[5];
        pColors[3] = pColors[4];
        pColors[2] = pColors[3];
        pColors[1] = pColors[2];
        pColors[0] = pColors[1];
        pModelData->MarkAsChanged(4u);
    }
    if (spFrame) {
        NiSmartPointerCast<NiAVObject>(spFrame)->SetAppCulled(aColor.a < 0.001);
    }
}

void BGSPrimitivePlane::SetRadii(NiPoint3& aRadii) {
    float fAbs = std::fabs(aRadii[0]);
    aRadii[0] = fAbs;
    aRadii[1] = 0.0f;
    fAbs = std::fabs(aRadii[2]);
    aRadii[2] = fAbs;
    Radii = aRadii;
    if (spGeometry.m_pObject) {
        NiGeometryData* pGeomData = spGeometry->GetModelData();
        NiPoint3* pVertices = pGeomData->m_pkVertex;
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
        pGeomData->MarkAsChanged(1);
    }
    if (spFrame.m_pObject) {
        NiGeometryData* pFrameData = static_cast<NiGeometry*>(spFrame.m_pObject)->GetModelData();
        NiPoint3* pFrameVertices = pFrameData->m_pkVertex;
        float fZ = Radii[2];
        float fY = Radii[1];
        float fX = -Radii[0];
        pFrameVertices[0] = NiPoint3(fX, fY, fZ);
        fZ = Radii[2];
        fY = Radii[1];
        fX = Radii[0];
        pFrameVertices[1] = NiPoint3(fX, fY, fZ);
        fZ = Radii[2];
        fY = -Radii[1];
        fX = Radii[0];
        pFrameVertices[2] = NiPoint3(fX, fY, fZ);
        fZ = Radii[2];
        fY = -Radii[1];
        fX = -Radii[0];
        pFrameVertices[3] = NiPoint3(fX, fY, fZ);
        pFrameVertices[4] = pFrameVertices[0];
        fZ = -Radii[2];
        fY = Radii[1];
        fX = -Radii[0];
        pFrameVertices[5] = NiPoint3(fX, fY, fZ);
        fZ = -Radii[2];
        fY = -Radii[1];
        fX = -Radii[0];
        pFrameVertices[6] = NiPoint3(fX, fY, fZ);
        fZ = -Radii[2];
        fY = -Radii[1];
        fX = Radii[0];
        pFrameVertices[7] = NiPoint3(fX, fY, fZ);
        fZ = -Radii[2];
        fY = Radii[1];
        fX = Radii[0];
        pFrameVertices[8] = NiPoint3(fX, fY, fZ);
        pFrameVertices[9] = pFrameVertices[5];
        pFrameVertices[10] = pFrameVertices[3];
        pFrameVertices[11] = pFrameVertices[6];
        pFrameVertices[12] = pFrameVertices[1];
        pFrameVertices[13] = pFrameVertices[8];
        pFrameVertices[14] = pFrameVertices[2];
        pFrameVertices[15] = pFrameVertices[7];
        pFrameData->MarkAsChanged(1);
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
        kBound.SetCenterAndRadius(NiPoint3::ZERO, fRad);
        spGeometry->SetModelBound(kBound);
        spNode->Update(UpdateData);
        spNode->UpdateWorldData(UpdateData);
        spGeometry->UpdateProperties();
        spNode->UpdateWorldBound();
    }
}

// Whether aPoint lies within the plane's radii on each axis; a NaN coordinate counts as inside.
bool BGSPrimitivePlane::IsInside(NiPoint3& aPoint) {
    return !(aPoint.x < -Radii.x) &&
        !(aPoint.y < -Radii.y) &&
        !(aPoint.z < -Radii.z) &&
        !(Radii.x < aPoint.x) &&
        !(Radii.y < aPoint.y) &&
        !(Radii.z < aPoint.z);
}

BSOcclusionPlane* BGSPrimitivePlane::MakeOcclusionPlane() {
    NiPoint2 kSize;
    return new BSOcclusionPlane(&NiPoint3::ZERO, ::new (&kSize) NiPoint2(Radii.x, Radii.z), &NiMatrix3::IDENTITY);
}

BSPortal* BGSPrimitivePlane::MakePortal() {
    NiPoint2 kSize;
    return new BSPortal(&NiPoint3::ZERO, ::new (&kSize) NiPoint2(Radii.x, Radii.z), &NiMatrix3::IDENTITY);
}

BGSPrimitiveBox::BGSPrimitiveBox() {
    eType = BGSPrimitive::PRIMITIVE_BOX;
}

BGSPrimitiveBox::~BGSPrimitiveBox() {}

void BGSPrimitiveBox::Build() {
    NiFixedString kName;
    if (!spNode.m_pObject) {
        BSFadeNode* pNode = new BSFadeNode;
        spNode = pNode;
        NiFixedString* pName = ::new (static_cast<void*>(&kName)) NiFixedString(const_cast<char*>("Box"));
        spNode->SetName(*pName);
        spNode->UpdateProperties();
    }
}

void BGSPrimitiveBox::SetColor(NiColorA& aColor) {
    // Copies the colour one channel at a time, each read just before its store (a reference that overlaps Color reads
    // the channels already written); C++'s copy assignment may read them all first.
    Color.r = aColor.r;
    Color.g = aColor.g;
    Color.b = aColor.b;
    Color.a = aColor.a;
    if (spGeometry.m_pObject) {
        NiGeometryData* pGeomData = spGeometry->GetModelData();
        NiColorA* pColors = pGeomData->m_pkColor;
        for (int32_t i = 0; i < pGeomData->m_usVertices; ++i) {
            pColors[i] = Color;
        }
        pGeomData->MarkAsChanged(4);
        NiGeometryData* pFrameData = static_cast<NiGeometry*>(spFrame.m_pObject)->GetModelData();
        NiColorA* pFrameColors = pFrameData->m_pkColor;
        for (int32_t i = 0; i < pFrameData->m_usVertices; ++i) {
            pFrameColors[i] = Color;
        }
        pFrameData->MarkAsChanged(4);
        static_cast<NiAVObject*>(spFrame.m_pObject)->NiAVObject::SetAppCulled(aColor.a < 0.001);
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
    if (spGeometry) {
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
    if (spFrame) {
        NiGeometryData* pFrameData = NiSmartPointerCast<NiGeometry>(spFrame)->GetModelData();
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
    if (spNode) {
        float fRadius = aRadii.x;
        if (aRadii.x < aRadii.y && aRadii.z < aRadii.y) {
            fRadius = aRadii.y;
        }
        if (aRadii.y < aRadii.z && aRadii.x < aRadii.z) {
            fRadius = aRadii.z;
        }
        alignas(NiBound) uint8_t aBoundStorage[sizeof(NiBound)];
        NiBound* pBound = ::new (aBoundStorage) NiBound;
        pBound->SetCenterAndRadius(NiPoint3::ZERO, fRadius);
        spGeometry->SetModelBound(*pBound);
        spNode->Update(*pUpdateData);
        spGeometry->UpdateProperties();
        NiAVObject* pNode = spNode;
        pNode->UpdateWorldBound();
    }
}

bool BGSPrimitiveBox::IsInside(NiPoint3& aPoint) {
    return !(aPoint.x < -Radii.x) &&
        !(aPoint.y < -Radii.y) &&
        !(aPoint.z < -Radii.z) &&
        !(Radii.x < static_cast<double>(aPoint.x)) &&
        !(Radii.y < static_cast<double>(aPoint.y)) &&
        !(Radii.z < static_cast<double>(aPoint.z));
}

BSMultiBoundShape* BGSPrimitiveBox::MakeMultiBoundShape(NiPoint3& akAngle) {
    bool bRotated = std::fabs(akAngle.x) > 0.009999999776482582;
    if (std::fabs(akAngle.y) > 0.009999999776482582) {
        bRotated = true;
    }
    if (std::fabs(akAngle.z) > 0.009999999776482582) {
        bRotated = true;
    }
    if (bRotated) {
        NiMatrix3 kRotation;
        BSMultiBoundOBB* pOBB = new BSMultiBoundOBB(&NiPoint3::ZERO, &Radii, &NiMatrix3::IDENTITY);
        kRotation.FromEulerAnglesXYZ(akAngle.x, akAngle.y, akAngle.z);
        pOBB->SetAxis(kRotation);
        return pOBB;
    }

    return new BSMultiBoundAABB(&NiPoint3::ZERO, &Radii);
}

BGSPrimitiveSphere::BGSPrimitiveSphere() {
    eType = BGSPrimitive::PRIMITIVE_SPHERE;
}

BGSPrimitiveSphere::~BGSPrimitiveSphere() {}

void BGSPrimitiveSphere::Build() {
    if (spGeometry.m_pObject) {
        SetRadius(Radii[0]);
        SetColor(Color);
    }
    else {
        NiColorA* pColors = new NiColorA[112];
        NiPoint3* pVerts = new NiPoint3[112];
        uint16_t* pStrip = static_cast<uint16_t*>(ourMalloc(112 * sizeof(uint16_t)));
        for (int32_t i = 0; i < 112; ++i) {
            float fX = pSphereVertices[i][0];
            pVerts[i].x = fX * Radii.x;
            float fY = pSphereVertices[i][1];
            pVerts[i].y = fY * Radii.x;
            float fZ = pSphereVertices[i][2];
            pVerts[i].z = fZ * Radii.x;
            pColors[i] = Color;
            pStrip[i] = i;
        }
        uint16_t* pStripLengths = static_cast<uint16_t*>(ourMalloc(sizeof(uint16_t)));
        *pStripLengths = 112;
        NiTriStrips*
            pTri = new NiTriStrips(112, pVerts, nullptr, pColors, nullptr, 0, 0, 110, 1, pStripLengths, pStrip);
        BSShaderProperty* pShProp = new BSShaderNoLightingProperty;
        pShProp->SetShader(33);
        pTri->AttachProperty(pShProp);
        BSShaderManager::PrepareObject(pTri, 0, 0);
        spGeometry = pTri;
        NiAlphaProperty* pAlphaProperty = new NiAlphaProperty;
        NiPointer<NiAlphaProperty> alpha(pAlphaProperty);
        alpha->SetName("primitive: alpha");
        alpha->SetAlphaBlending(true);
        spGeometry->AttachProperty(alpha.m_pObject);
        BSShaderProperty* pProperty = static_cast<BSShaderProperty*>(spGeometry->GetProperty(3));
        pProperty->SetFlag(BSShaderProperty::BSSP_FLAG_ZBUFFER_WRITE, 0);
        BSFadeNode* pNode = new BSFadeNode;
        spNode = pNode;
        spNode->AttachChild(spGeometry.m_pObject, 1);
        spNode->SetName("Sphere");
        spNode->UpdateProperties();
        spGeometry->UpdateProperties();
    }
    NiUpdateData UpdateData(0.0f, false, false);
    spNode->Update(UpdateData);
}

void BGSPrimitiveSphere::SetColor(NiColorA& aColor) {
    Color = aColor;
    if (spGeometry.m_pObject) {
        NiGeometryData* pModelData = spGeometry->GetModelData();
        NiColorA* pColors = pModelData->GetColors();
        for (int32_t i = 0; i < 112; ++i) {
            pColors[i] = Color;
        }
        pModelData->MarkAsChanged(4u);
    }
}

void BGSPrimitiveSphere::SetRadii(NiPoint3& aRadii) {
    SetRadius(aRadii.x);
}

bool BGSPrimitiveSphere::IsInside(NiPoint3& aPoint) {
    double dRadiusSqr = Radii.x * Radii.x;
    return aPoint.SqrLength() < dRadiusSqr;
}

BSMultiBoundShape* BGSPrimitiveSphere::MakeMultiBoundShape(NiPoint3& akAngle) {
    NiPoint3 kCenter(0.0f, 0.0f, 0.0f);
    return new BSMultiBoundSphere(&kCenter, Radii.x);
}

void BGSPrimitiveSphere::SetRadius(float afRadius) {
    float fRadius = std::fabs(afRadius);
    if (spGeometry.m_pObject) {
        NiGeometryData* pGeomData = spGeometry->GetModelData();
        NiPoint3* pVerts = pGeomData->m_pkVertex;
        for (int32_t i = 0; i < 112; ++i) {
            float fX = pSphereVertices[i][0];
            pVerts[i].x = fX;
            float fY = pSphereVertices[i][1];
            pVerts[i].y = fY;
            float fZ = pSphereVertices[i][2];
            pVerts[i].z = fZ;
            pVerts[i] *= fRadius;
        }
        pGeomData->MarkAsChanged(1);
    }
    Radii.z = fRadius;
    Radii.y = fRadius;
    Radii.x = fRadius;
    NiUpdateData UpdateData(0.0f, false, false);
    if (spNode.m_pObject) {
        NiBound kBound;
        NiPoint3 kCenter(0.0f, 0.0f, 0.0f);
        kBound.SetCenterAndRadius(kCenter, fRadius);
        spGeometry->SetModelBound(kBound);
        spNode->Update(UpdateData);
        spGeometry->UpdateProperties();
        spNode->UpdateWorldBound();
    }
}
