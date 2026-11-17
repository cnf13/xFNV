#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphere.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereTriIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cmath>
#include <cstring>

NiSphereBV::NiSphereBV(float fRadius, const NiPoint3& kCenter) {
    m_kSphere.m_kCenter = kCenter;
    m_kSphere.m_fRadius = fRadius;
}

NiSphereBV::NiSphereBV(const NiSphereBV& kABV) : m_kSphere(kABV.m_kSphere) {}

void NiSphereBV::LoadBinary(NiStream& kStream) {
    NiBoundingVolume::LoadBinary(kStream);
    m_kSphere.LoadBinary(kStream);
}

void NiSphereBV::SaveBinary(NiStream& kStream) {
    NiBoundingVolume::SaveBinary(kStream);
    m_kSphere.SaveBinary(kStream);
}

// Rotates, scales and translates the centre; scales the radius. It keeps the scale in the dead arModelABV argument slot
// (a local here) and reloads it for the radius after the stores.
void NiSphereBV::UpdateWorldData(const NiBoundingVolume& kModelABV, const NiTransform& kWorld) {
    const NiSphere& kModel = static_cast<const NiSphereBV&>(kModelABV).m_kSphere;
    const float (*m)[3] = kWorld.m_Rotate.m_pEntry;
    const NiPoint3& c = kModel.m_kCenter;

    float fRotX = m[0][1] * c.y + m[0][0] * c.x + m[0][2] * c.z;
    float fRotY = m[1][1] * c.y + m[1][0] * c.x + m[1][2] * c.z;
    float fRotZ = m[2][1] * c.y + m[2][0] * c.x + m[2][2] * c.z;
    float fScale = kWorld.m_fScale;
    float fScaledX = fRotX * fScale;
    float fScaledY = fRotY * fScale;
    float fScaledZ = fScale * fRotZ;
    float fX = kWorld.m_Translate.x + fScaledX;
    float fY = kWorld.m_Translate.y + fScaledY;
    float fZ = kWorld.m_Translate.z + fScaledZ;
    m_kSphere.m_kCenter.x = fX;
    m_kSphere.m_kCenter.y = fY;
    m_kSphere.m_kCenter.z = fZ;
    m_kSphere.m_fRadius = kModel.m_fRadius * kWorld.m_fScale;
}

// Gamebryo's default arguments: the unit sphere at NiPoint3::ZERO.
NiBoundingVolume* NiSphereBV::Create() const {
    NiSphereBV* pkSphere = static_cast<NiSphereBV*>(MemoryManager_Singleton.Allocate(sizeof(NiSphereBV)));
    if (!pkSphere) {
        return nullptr;
    }
    return ::new (pkSphere) NiSphereBV(1.0f, NiPoint3_ZERO);
}

NiBoundingVolume* NiSphereBV::Clone() const {
    NiSphereBV* pkClone = static_cast<NiSphereBV*>(MemoryManager_Singleton.Allocate(sizeof(NiSphereBV)));
    if (!pkClone) {
        return nullptr;
    }
    return ::new (pkClone) NiSphereBV(*this);
}

void NiSphereBV::Copy(const NiBoundingVolume& kABV) {
    m_kSphere = static_cast<const NiSphereBV&>(kABV).m_kSphere;
}

bool NiSphereBV::operator==(const NiBoundingVolume& kABV) const {
    return kABV.Type() == SPHERE_BV && static_cast<const NiSphereBV&>(kABV).m_kSphere == m_kSphere;
}

bool NiSphereBV::operator!=(const NiBoundingVolume& kABV) const {
    return !operator==(kABV);
}

// The type, the centre, and the radius cut to two decimals.
void NiSphereBV::AddViewerStrings(const char* pcPrefix, NiTPrimitiveArray<char*>* pkStrings) const {
    size_t stLen = strlen(pcPrefix) + 13;
    char* pcString = static_cast<char*>(_NiMalloc(stLen));
    NiSprintf(pcString, stLen, "%s = SPHERE_BV", pcPrefix);
    pkStrings->Add(pcString);

    pkStrings->Add(m_kSphere.m_kCenter.GetViewerString("     center"));

    stLen = strlen(pcPrefix) + 30;
    pcString = static_cast<char*>(_NiMalloc(stLen));
    NiSprintf(pcString, stLen, "     radius = %g", static_cast<int32_t>(m_kSphere.m_fRadius * 100.0) / 100.0);
    pkStrings->Add(pcString);
}

// LoadBinary is called on the allocation even when it is null, deliberately, as the engine's source does. The default
// arguments are the unit sphere at NiPoint3::ZERO.
NiBoundingVolume* NiSphereBV::CreateFromStream(NiStream& kStream) {
    NiSphereBV* pkSphABV = static_cast<NiSphereBV*>(MemoryManager_Singleton.Allocate(sizeof(NiSphereBV)));
    if (pkSphABV) {
        ::new (pkSphABV) NiSphereBV(1.0f, NiPoint3_ZERO);
    }
    pkSphABV->LoadBinary(kStream);
    return pkSphABV;
}

bool NiSphereBV::SphereTriTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiPoint3& kVert0,
    const NiPoint3& kVert1,
    const NiPoint3& kVert2,
    const NiPoint3& kV1
) {
    const NiSphereBV& kSphABV = static_cast<const NiSphereBV&>(kABV0);

    float fT0, fT1, fSqrDist;
    NiTrigon kTri;
    kTri.m_kOrigin = kVert0;
    // EXE-INLINED 0x439EF0
    kTri.m_kEdge0.x = kVert1.x - kVert0.x;
    kTri.m_kEdge0.y = kVert1.y - kVert0.y;
    kTri.m_kEdge0.z = kVert1.z - kVert0.z;
    // EXE-INLINED 0x439EF0
    kTri.m_kEdge1.x = kVert2.x - kVert0.x;
    kTri.m_kEdge1.y = kVert2.y - kVert0.y;
    kTri.m_kEdge1.z = kVert2.z - kVert0.z;

    // EXE-INLINED 0x439EF0
    NiPoint3 kDeltaV;
    kDeltaV.x = kV0.x - kV1.x;
    kDeltaV.y = kV0.y - kV1.y;
    kDeltaV.z = kV0.z - kV1.z;

    // EXE-INLINED 0x439090
    if (NiPoint3_ZERO.x != kDeltaV.x || NiPoint3_ZERO.y != kDeltaV.y || NiPoint3_ZERO.z != kDeltaV.z) {
        float fS0;
        NiSegment kSeg;
        kSeg.m_kOrigin = kSphABV.m_kSphere.m_kCenter;
        // EXE-INLINED 0x4A3760
        kSeg.m_kDirection.x = fTime * kDeltaV.x;
        kSeg.m_kDirection.y = fTime * kDeltaV.y;
        kSeg.m_kDirection.z = fTime * kDeltaV.z;

        fSqrDist = NiSqrDistance::Compute(kSeg, kTri, fS0, fT0, fT1);
    }
    else {
        fSqrDist = NiSqrDistance::Compute(kSphABV.m_kSphere.m_kCenter, kTri, fT0, fT1);
    }

    return fSqrDist <= kSphABV.m_kSphere.m_fRadius * kSphABV.m_kSphere.m_fRadius;
}

bool NiSphereBV::SphereTriFindIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiPoint3& kVert0,
    const NiPoint3& kVert1,
    const NiPoint3& kVert2,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    bool bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    NiPoint3 v40;
    NiPoint3 kTriNormal;
    NiPoint3 kPnt;
    NiPoint3 v66;
    NiTrigon kTri;
    float fTriParam0;
    float fTriParam1;
    NiPoint3 kSurfacePt;
    NiPoint3 result;
    double v12 = static_cast<const NiSphereBV*>(&kABV0)->m_kSphere.m_fRadius *
        static_cast<const NiSphereBV*>(&kABV0)->m_kSphere.m_fRadius;
    float y = static_cast<const NiSphereBV*>(&kABV0)->m_kSphere.m_kCenter.y;
    float z = static_cast<const NiSphereBV*>(&kABV0)->m_kSphere.m_kCenter.z;
    kPnt.x = static_cast<const NiSphereBV*>(&kABV0)->m_kSphere.m_kCenter.x;
    float v68 = v12;
    double x = kVert1.x;
    kTri.m_kOrigin.x = kVert0.x;
    double v16 = x - kVert0.x;
    kPnt.y = y;
    kTri.m_kOrigin.y = kVert0.y;
    float v37 = v16;
    double v17 = kVert1.y;
    kPnt.z = z;
    double v18 = v17 - kVert0.y;
    float v19 = kVert0.z;
    kTri.m_kEdge0.x = v37;
    kTri.m_kOrigin.z = v19;
    float v41 = v18;
    double v20 = kVert1.z;
    kTri.m_kEdge0.y = v41;
    float v44 = v20 - kVert0.z;
    double v21 = kVert2.x;
    kTri.m_kEdge0.z = v44;
    float v38 = v21 - kVert0.x;
    double v22 = kVert2.y;
    kTri.m_kEdge1.x = v38;
    float v42 = v22 - kVert0.y;
    double v23 = kVert2.z;
    kTri.m_kEdge1.y = v42;
    float v45 = v23 - kVert0.z;
    kTri.m_kEdge1.z = v45;
    double v24 = NiSqrDistance::Compute(kPnt, kTri, fTriParam0, fTriParam1);
    float v53 = v24 - v68;
    if (v53 <= 0.0f) {
        NiPoint3 v74;
        fIntrTime = 0.0f;
        float v54 = kTri.m_kEdge1.x * fTriParam1;
        float v59 = kTri.m_kEdge1.y * fTriParam1;
        float v61 = fTriParam1 * kTri.m_kEdge1.z;
        float v39 = kTri.m_kEdge0.x * fTriParam0;
        float v43 = kTri.m_kEdge0.y * fTriParam0;
        float v46 = fTriParam0 * kTri.m_kEdge0.z;
        kTriNormal.x = kTri.m_kOrigin.x + v39;
        kTriNormal.y = kTri.m_kOrigin.y + v43;
        kTriNormal.z = kTri.m_kOrigin.z + v46;
        v74.x = kTriNormal.x + v54;
        double v25 = kTriNormal.y;
        kIntrPt.x = v74.x;
        v74.y = v25 + v59;
        double v26 = kTriNormal.z;
        kIntrPt.y = v74.y;
        v74.z = v26 + v61;
        kIntrPt.z = v74.z;
        if (bCalcNormals) {
            kNormal1 = *kTri.GetNormal(&kSurfacePt);
            // EXE-INLINED 0x4A0BD0
            v74.x = -kNormal1.x;
            v74.y = -kNormal1.y;
            v74.z = -kNormal1.z;
            kNormal0 = v74;
            return true;
        }
        return true;
    }
    v40.x = kV0.x - kV1.x;
    double v29 = kV0.y;
    v66.x = v40.x;
    v40.y = v29 - kV1.y;
    double v30 = kV0.z - kV1.z;
    v66.y = v40.y;
    v40.z = v30;
    v66.z = v40.z;
    float v55 = v40.x * v40.x + v40.y * v40.y + v40.z * v40.z;
    if (v55 * fTime <= 9.999999747378752e-06) {
        return false;
    }
    float v56 = kTri.m_kEdge0.y * kTri.m_kEdge1.z - kTri.m_kEdge0.z * kTri.m_kEdge1.y;
    float v60 = kTri.m_kEdge0.z * kTri.m_kEdge1.x - kTri.m_kEdge1.z * kTri.m_kEdge0.x;
    float v62 = kTri.m_kEdge1.y * kTri.m_kEdge0.x - kTri.m_kEdge0.y * kTri.m_kEdge1.x;
    kTriNormal.x = -v56;
    kTriNormal.y = -v60;
    kTriNormal.z = -v62;
    float v57 = kPnt.z * kTriNormal.z + (kPnt.y * kTriNormal.y + kTriNormal.x * kPnt.x);
    float v64 = kTriNormal.x * kVert0.x + kVert0.y * kTriNormal.y + kTriNormal.z * kVert0.z;
    float v47 = v57 - v64;
    double v31 = v47 * v47;
    float v48 = kTriNormal.x * kTriNormal.x + kTriNormal.y * kTriNormal.y + kTriNormal.z * kTriNormal.z;
    float v49 = v68 * v48;
    if (v49 < v31) {
        kTriNormal.Unitize();
        double v32 = kTriNormal.x * v40.x + kTriNormal.y * v40.y + kTriNormal.z * v40.z;
        if (!(v64 < static_cast<double>(v57))) {
            NiPoint3 v76;
            float v51 = v32;
            if (v51 <= 0.0f) {
                return false;
            }
            NiPoint3* v34 =
                ::new (&kSurfacePt) NiPoint3(static_cast<const NiSphereBV*>(&kABV0)->m_kSphere.m_fRadius * kTriNormal);
            v76.x = v34->x + kPnt.x;
            v76.y = v34->y + kPnt.y;
            v76.z = v34->z + kPnt.z;
            v40 = v76;
        }
        else {
            NiPoint3 v75;
            float v50 = v32;
            if (v50 >= 0.0f) {
                return false;
            }
            NiPoint3* v33 =
                ::new (&kSurfacePt) NiPoint3(static_cast<const NiSphereBV*>(&kABV0)->m_kSphere.m_fRadius * kTriNormal);
            v75.x = kPnt.x - v33->x;
            v75.y = kPnt.y - v33->y;
            v75.z = kPnt.z - v33->z;
            v40 = v75;
        }
        v64 = kVert0.y * kTriNormal.y + kTriNormal.x * kVert0.x + kTriNormal.z * kVert0.z;
        double v58 = v64 + 0.0010000000474974513;
        float v52 = kTriNormal.y * v40.y + kTriNormal.x * v40.x + kTriNormal.z * v40.z;
        if (v52 > v58) {
            NiPoint3* v35 = ::new (&result) NiPoint3(fTime * v66);
            kSurfacePt.x = v35->x + v40.x;
            kSurfacePt.y = v35->y + v40.y;
            kSurfacePt.z = v35->z + v40.z;
            if (kTriNormal.Dot(kSurfacePt) > v58) {
                return false;
            }
        }
    }
    // The source's downcast: kABV0 is a sphere here.
    NiSphereTriIntersector kIntersector(
        static_cast<const NiSphereBV*>(&kABV0), &kVert0, &kVert1, &kVert2, fTime, 0.001f, 0.0001f, 32
    );
    kIntersector.NiIntersector::Find(kV0, kV1);
    fIntrTime = kIntersector.GetContactTime();
    if (kIntersector.m_eIntersectionType != NiIntersector::IT_OVERLAP &&
        kIntersector.m_eIntersectionType != NiIntersector::IT_CONTACT) {
        return false;
    }
    kIntrPt = kIntersector.GetContactPoint();
    if (bCalcNormals) {
        kNormal0 = kIntersector.GetContactNormal();
        // EXE-INLINED 0x4A0BD0
        v40.x = -kNormal0.x;
        v40.y = -kNormal0.y;
        v40.z = -kNormal0.z;
        kNormal1 = v40;
    }
    return true;
}

bool NiSphereBV::SphereSphereTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1
) {
    const NiSphereBV& kSphABV0 = static_cast<const NiSphereBV&>(kABV0);
    const NiSphereBV& kSphABV1 = static_cast<const NiSphereBV&>(kABV1);
    double v8;
    double v9;
    double v10;
    double v11;
    double v12;
    float kABV0a;
    float v15 = kV1.x - kV0.x;
    float v17 = kV1.y - kV0.y;
    float v19 = kV1.z - kV0.z;
    double v5 = v17;
    double v6 = v15;
    double v7 = v19;
    float v14 = v19 * v19 + (v17 * v17 + v15 * v15);
    float v16 = kSphABV1.m_kSphere.m_kCenter.x - kSphABV0.m_kSphere.m_kCenter.x;
    float v18 = kSphABV1.m_kSphere.m_kCenter.y - kSphABV0.m_kSphere.m_kCenter.y;
    float v20 = kSphABV1.m_kSphere.m_kCenter.z - kSphABV0.m_kSphere.m_kCenter.z;
    float kV1a = v18 * v18 + v16 * v16 + v20 * v20;
    float kV0b = kSphABV0.m_kSphere.m_fRadius + kSphABV1.m_kSphere.m_fRadius;
    float kV0a = kV0b * kV0b;
    if (!(v14 > 0.0f) ||
        (v8 = v7 * v20 + (v6 * v16 + v5 * v18), (v9 = v14, (kABV0a = v8, (v10 = kABV0a, !(kABV0a <= 0.0f)))))) {
        v11 = kV1a;
        goto LABEL_7;
    }
    if (!(-fTime * v9 <= v10)) {
        v11 = fTime * (v9 * fTime + (v10 + v10)) + kV1a;
    LABEL_7:;
        v12 = kV0a;
        return v12 >= v11;
    }
    v11 = kV1a * v9 - v10 * v10;
    v12 = v9 * kV0a;
    return v12 >= v11;
}

bool NiSphereBV::SphereSphereFindIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    bool bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    const NiSphereBV& kSphABV0 = static_cast<const NiSphereBV&>(kABV0);
    const NiSphereBV& kSphABV1 = static_cast<const NiSphereBV&>(kABV1);
    double v14;
    NiPoint3* v30;
    double z;
    NiPoint3 v33;
    float v51;
    float v53;
    float v37 = kV1.x - kV0.x;
    float v40 = kV1.y - kV0.y;
    float v42 = kV1.z - kV0.z;
    float kV0a = v42 * v42 + (v40 * v40 + v37 * v37);
    v33.x = kSphABV1.m_kSphere.m_kCenter.x - kSphABV0.m_kSphere.m_kCenter.x;
    v33.y = kSphABV1.m_kSphere.m_kCenter.y - kSphABV0.m_kSphere.m_kCenter.y;
    v33.z = kSphABV1.m_kSphere.m_kCenter.z - kSphABV0.m_kSphere.m_kCenter.z;
    float kABV1a = v33.x * v33.x + v33.y * v33.y + v33.z * v33.z;
    float kABV0a = kSphABV1.m_kSphere.m_fRadius + kSphABV0.m_kSphere.m_fRadius;
    float v32 = kABV0a * kABV0a;
    if (!(kV0a > 0.0f)) {
        v14 = 0.0;
    }
    else {
        v14 = 0.0;
        double v15 = kV0a;
        float kV1a = v42 * v33.z + (v37 * v33.x + v40 * v33.y);
        double v16 = kV1a;
        if (kV1a <= 0.0f) {
            double v21;
            double v17 = -fTime * v15;
            bool v18 = !(v17 >= v16);
            bool v19 = v17 == v16;
            double v20 = kABV1a;
            if (v18 || v19) {
                v21 = v32;
            }
            else {
                v21 = v32;
                if (!(v32 + 9.999999747378752e-06 >= fTime * (v15 * fTime + (v16 + v16)) + v20)) {
                    return false;
                }
            }
            double v23 = kABV1a;
            float kABV1c = v20 - v21;
            double v24 = v16 * v16;
            double v25 = kABV1c;
            double v26 = v24 - v15 * kABV1c;
            float kABV1b = v26;
            if (!(kABV1b >= 0.0f)) {
                return false;
            }
            if (!(v25 <= 0.0)) {
                NiPoint3 v39;
                NiPoint3 v72;
                float kABV1d = std::sqrt((double)kABV1b);
                float kV1b = -((kABV1d + kV1a) / kV0a);
                fIntrTime = kV1b;
                if (!(kV1b < 0.0f)) {
                    if (fTime < static_cast<double>(kV1b)) {
                        fIntrTime = fTime;
                    }
                }
                else {
                    fIntrTime = 0.0f;
                }
                float kV1c = fIntrTime;
                float v44 = kV1c * v37;
                float v46 = v40 * kV1c;
                float v48 = kV1c * v42;
                v39.x = v44 + v33.x;
                v39.y = v46 + v33.y;
                v39.z = v48 + v33.z;
                float kV1d = kSphABV0.m_kSphere.m_fRadius / kABV0a;
                float v50 = v39.x * kV1d;
                float v52 = v39.y * kV1d;
                float v54 = kV1d * v39.z;
                float kV1e = fIntrTime;
                float v45 = kV1e * kV0.x;
                float v47 = kV1e * kV0.y;
                float v49 = kV1e * kV0.z;
                float v34 = kSphABV0.m_kSphere.m_kCenter.x + v45;
                float v35 = kSphABV0.m_kSphere.m_kCenter.y + v47;
                float v36 = kSphABV0.m_kSphere.m_kCenter.z + v49;
                v72.x = v34 + v50;
                v72.y = v35 + v52;
                v72.z = v36 + v54;
                kIntrPt = v72;
                if (bCalcNormals) {
                    NiPoint3 v75;
                    kNormal0 = v39;
                    kNormal0.Unitize();
                    v75.x = -kNormal0.x;
                    v75.y = -kNormal0.y;
                    v75.z = -kNormal0.z;
                    kNormal1 = v75;
                }
                return true;
            }
            else {
                NiPoint3 v70;
                fIntrTime = 0.0f;
                float v38 = kSphABV0.m_kSphere.m_kCenter.x + kSphABV1.m_kSphere.m_kCenter.x;
                float v41 = kSphABV0.m_kSphere.m_kCenter.y + kSphABV1.m_kSphere.m_kCenter.y;
                float v43 = kSphABV0.m_kSphere.m_kCenter.z + kSphABV1.m_kSphere.m_kCenter.z;
                v70.x = v38 * 0.5;
                v70.y = v41 * 0.5;
                v70.z = 0.5 * v43;
                kIntrPt = v70;
                if (bCalcNormals) {
                    NiPoint3 v71;
                    if (!(v23 > 1.1920929e-07f)) {
                        kNormal0 = NiPoint3_UNIT_X;
                    }
                    else {
                        kNormal0 = v33;
                        kNormal0.Unitize();
                    }
                    v71.x = -kNormal0.x;
                    v71.y = -kNormal0.y;
                    v71.z = -kNormal0.z;
                    kNormal1 = v71;
                    return true;
                }
                else {
                    return true;
                }
            }
        }
    }
    if (!(v32 >= static_cast<double>(kABV1a))) {
        return false;
    }
    fIntrTime = v14;
    float kV1f = kV1.z * kV1.z + (kV1.x * kV1.x + kV1.y * kV1.y);
    double v28 = kV1f;
    float kV1g = kV0.z * kV0.z + (kV0.x * kV0.x + kV0.y * kV0.y);
    if (!(kV1g < v28)) {
        NiPoint3 v74;
        NiPoint3 v77;
        v77.x = kSphABV0.m_kSphere.m_kCenter.x - kSphABV1.m_kSphere.m_kCenter.x;
        v77.y = kSphABV0.m_kSphere.m_kCenter.y - kSphABV1.m_kSphere.m_kCenter.y;
        v77.z = kSphABV0.m_kSphere.m_kCenter.z - kSphABV1.m_kSphere.m_kCenter.z;
        kNormal1 = v77;
        kNormal1.Unitize();
        float kV1h = kSphABV1.m_kSphere.m_fRadius;
        v77.x = kV1h * kNormal1.x;
        v77.y = kNormal1.y * kV1h;
        v77.z = kV1h * kNormal1.z;
        v74.x = v77.x + kSphABV1.m_kSphere.m_kCenter.x;
        v74.y = kSphABV1.m_kSphere.m_kCenter.y + v77.y;
        v74.z = kSphABV1.m_kSphere.m_kCenter.z + v77.z;
        kIntrPt = v74;
        v30 = &kNormal0;
        v51 = -kNormal1.x;
        v53 = -kNormal1.y;
        z = kNormal1.z;
    }
    else {
        NiPoint3 v73;
        NiPoint3 v76;
        v76.x = kSphABV1.m_kSphere.m_kCenter.x - kSphABV0.m_kSphere.m_kCenter.x;
        v76.y = kSphABV1.m_kSphere.m_kCenter.y - kSphABV0.m_kSphere.m_kCenter.y;
        v76.z = kSphABV1.m_kSphere.m_kCenter.z - kSphABV0.m_kSphere.m_kCenter.z;
        kNormal0 = v76;
        kNormal0.Unitize();
        double v29 = kSphABV0.m_kSphere.m_fRadius;
        v76.x = kNormal0.x * v29;
        v76.y = kNormal0.y * v29;
        v76.z = v29 * kNormal0.z;
        v73.x = kSphABV0.m_kSphere.m_kCenter.x + v76.x;
        v73.y = kSphABV0.m_kSphere.m_kCenter.y + v76.y;
        v73.z = kSphABV0.m_kSphere.m_kCenter.z + v76.z;
        kIntrPt = v73;
        v30 = &kNormal1;
        v51 = -kNormal0.x;
        v53 = -kNormal0.y;
        z = kNormal0.z;
    }
    float v55 = -z;
    v30->x = v51;
    v30->y = v53;
    v30->z = v55;
    return true;
}

// KEEP: niread: the stream reads fill locals nothing initialised: a short read (malformed data) leaves kCenter or
// fRadius unwritten, and the exe stores them into the new sphere. The axes and the two extents are read and dropped;
// the third float is the radius.
NiBoundingVolume* NiSphereBV::ConvertLozengeToSphereBoundingVolume(NiStream& kStream) {
    NiPoint3 kCenter;
    NiPoint3 kAxis0;
    NiPoint3 kAxis1;
    kCenter.LoadBinary(kStream);
    kAxis0.LoadBinary(kStream);
    kAxis1.LoadBinary(kStream);
    float fExtent0;
    float fExtent1;
    float fRadius;
    NiStreamLoadBinary(kStream, fExtent0);
    NiStreamLoadBinary(kStream, fExtent1);
    NiStreamLoadBinary(kStream, fRadius);
    NiSphereBV* pkSphABV = static_cast<NiSphereBV*>(MemoryManager_Singleton.Allocate(sizeof(NiSphereBV)));
    if (!pkSphABV) {
        return nullptr;
    }
    return ::new (pkSphABV) NiSphereBV(fRadius, kCenter);
}
