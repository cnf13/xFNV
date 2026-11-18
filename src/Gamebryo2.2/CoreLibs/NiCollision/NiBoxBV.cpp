#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxBV.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBox.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxBVMacros.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxCapsuleIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxSphereIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSegment.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cmath>
#include <cstring>

NiBoxBV::NiBoxBV(
    const NiPoint3& kExtent,
    const NiPoint3& kCenter,
    const NiPoint3& kXAvis,
    const NiPoint3& kYAvis,
    const NiPoint3& kZAxis
) {
    m_kBox.m_kCenter = kCenter;
    m_kBox.m_afExtent[0] = kExtent.x;
    m_kBox.m_afExtent[1] = kExtent.y;
    m_kBox.m_afExtent[2] = kExtent.z;
    m_kBox.m_akAxis[0] = kXAvis;
    m_kBox.m_akAxis[1] = kYAvis;
    m_kBox.m_akAxis[2] = kZAxis;
}

NiBoxBV::NiBoxBV(const NiBoxBV& kABV) : m_kBox(kABV.m_kBox) {}

void NiBoxBV::LoadBinary(NiStream& kStream) {
    NiBoundingVolume::LoadBinary(kStream);
    m_kBox.LoadBinary(kStream);
}

void NiBoxBV::SaveBinary(NiStream& kStream) {
    NiBoundingVolume::SaveBinary(kStream);
    m_kBox.SaveBinary(kStream);
}

// Rotates, scales and translates the centre; rotates the axes and scales the extents. It keeps the scale in the dead
// arModelABV argument slot (a local here) and reloads it for each extent after the stores.
void NiBoxBV::UpdateWorldData(const NiBoundingVolume& kModelABV, const NiTransform& kWorld) {
    const NiBox& kModel = static_cast<const NiBoxBV&>(kModelABV).m_kBox;
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
    m_kBox.m_kCenter.x = fX;
    m_kBox.m_kCenter.y = fY;
    m_kBox.m_kCenter.z = fZ;

    for (int32_t i = 0; i < 3; i++) {
        const NiPoint3& a = kModel.m_akAxis[i];
        float fAxisX = m[0][0] * a.x + m[0][1] * a.y + m[0][2] * a.z;
        float fAxisY = m[1][1] * a.y + m[1][0] * a.x + m[1][2] * a.z;
        float fAxisZ = m[2][0] * a.x + m[2][1] * a.y + m[2][2] * a.z;
        m_kBox.m_akAxis[i].x = fAxisX;
        m_kBox.m_akAxis[i].y = fAxisY;
        m_kBox.m_akAxis[i].z = fAxisZ;
        m_kBox.m_afExtent[i] = kModel.m_afExtent[i] * kWorld.m_fScale;
    }
}

// A unit box at the origin, on the world axes.
NiBoundingVolume* NiBoxBV::Create() const {
    void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(NiBoxBV));
    if (!pvMemory) {
        return nullptr;
    }
    return ::new (pvMemory)
        NiBoxBV(NiPoint3_UNIT_ALL, NiPoint3_ZERO, NiPoint3_UNIT_X, NiPoint3_UNIT_Y, NiPoint3_UNIT_Z);
}

NiBoundingVolume* NiBoxBV::Clone() const {
    NiBoxBV* pkClone = static_cast<NiBoxBV*>(MemoryManager_Singleton.Allocate(sizeof(NiBoxBV)));
    if (!pkClone) {
        return nullptr;
    }
    return ::new (pkClone) NiBoxBV(*this);
}

void NiBoxBV::Copy(const NiBoundingVolume& kABV) {
    m_kBox = static_cast<const NiBoxBV&>(kABV).m_kBox;
}

bool NiBoxBV::operator==(const NiBoundingVolume& kABV) const {
    return kABV.Type() == BOX_BV && static_cast<const NiBoxBV&>(kABV).m_kBox == m_kBox;
}

// Shared by every bounding volume class.
bool NiBoxBV::operator!=(const NiBoundingVolume& kABV) const {
    return !operator==(kABV);
}

void NiBoxBV::AddViewerStrings(const char* pcPrefix, NiTPrimitiveArray<char*>* pkStrings) const {
    size_t stLen = strlen(pcPrefix) + 10;
    char* pcString = static_cast<char*>(_NiMalloc(stLen));
    NiSprintf(pcString, stLen, "%s = BOX_BV", pcPrefix);
    pkStrings->Add(pcString);
}

NiPoint3 NiBoxBV::GetNormal(int32_t iSign0, int32_t iSign1, int32_t iSign2, const NiPoint3 kVelocity) const {
    double v12;
    double v14;
    float apResultc;
    float apResultd;
    float apResulte;
    float apResultf;
    double x = kVelocity.x;
    NiPoint3 kNormal = NiPoint3_ZERO;
    int32_t v8 = 0;
    double z = kVelocity.z;
    double y = kVelocity.y;
    bool v11 = NiPoint3_ZERO.x == kVelocity.x && (NiPoint3_ZERO.y == y && NiPoint3_ZERO.z == z);
    if (!v11) {
        float apResulta = m_kBox.m_akAxis[0].y * y + m_kBox.m_akAxis[0].x * x + m_kBox.m_akAxis[0].z * z;
        float apResultb = std::fabs((double)apResulta);
        if (!(apResultb > 1.1920929e-07f)) {
            goto LABEL_14;
        }
    }
    if (iSign0 == 1) {
        kNormal.x = kNormal.x + m_kBox.m_akAxis[0].x;
        kNormal.y = m_kBox.m_akAxis[0].y + kNormal.y;
        v12 = m_kBox.m_akAxis[0].z + kNormal.z;
    }
    else {
        if (iSign0 != -1) {
            goto LABEL_13;
        }
        kNormal.x = kNormal.x - m_kBox.m_akAxis[0].x;
        kNormal.y = kNormal.y - m_kBox.m_akAxis[0].y;
        v12 = kNormal.z - m_kBox.m_akAxis[0].z;
    }
    kNormal.z = v12;
    v8 = 1;
LABEL_13:;
    if (v11) {
        goto LABEL_15;
    }
LABEL_14:;
    apResultc = m_kBox.m_akAxis[1].y * y + x * m_kBox.m_akAxis[1].x + m_kBox.m_akAxis[1].z * z;
    apResultd = std::fabs((double)apResultc);
    if (apResultd > 1.1920929e-07f) {
        double v13;
    LABEL_15:;
        if (iSign1 == 1) {
            kNormal.x = kNormal.x + m_kBox.m_akAxis[1].x;
            kNormal.y = m_kBox.m_akAxis[1].y + kNormal.y;
            v13 = m_kBox.m_akAxis[1].z + kNormal.z;
        }
        else {
            if (iSign1 != -1) {
                goto LABEL_20;
            }
            kNormal.x = kNormal.x - m_kBox.m_akAxis[1].x;
            kNormal.y = kNormal.y - m_kBox.m_akAxis[1].y;
            v13 = kNormal.z - m_kBox.m_akAxis[1].z;
        }
        kNormal.z = v13;
        ++v8;
    LABEL_20:;
        if (v11) {
            goto LABEL_22;
        }
    }
    apResulte = z * m_kBox.m_akAxis[2].z + (x * m_kBox.m_akAxis[2].x + y * m_kBox.m_akAxis[2].y);
    apResultf = std::fabs((double)apResulte);
    if (!(apResultf > 1.1920929e-07f)) {
        goto LABEL_27;
    }
LABEL_22:;
    if (iSign2 == 1) {
        kNormal.x = kNormal.x + m_kBox.m_akAxis[2].x;
        kNormal.y = m_kBox.m_akAxis[2].y + kNormal.y;
        v14 = m_kBox.m_akAxis[2].z + kNormal.z;
    }
    else {
        if (iSign2 != -1) {
            goto LABEL_27;
        }
        kNormal.x = kNormal.x - m_kBox.m_akAxis[2].x;
        kNormal.y = kNormal.y - m_kBox.m_akAxis[2].y;
        v14 = kNormal.z - m_kBox.m_akAxis[2].z;
    }
    kNormal.z = v14;
    ++v8;
LABEL_27:;
    if (v8 != 1) {
        kNormal.Unitize();
    }
    return kNormal;
}

NiBoundingVolume* NiBoxBV::CreateFromStream(NiStream& kStream) {
    NiBoxBV* v1 = static_cast<NiBoxBV*>(MemoryManager_Singleton.Allocate(sizeof(NiBoxBV)));
    // LoadBinary is called on the new NiBoxBV without a null test, as the game does: on a failed allocation it is
    // called through a null object.
    NiBoxBV* v2 = nullptr;
    if (v1) {
        v2 = ::new (v1) NiBoxBV(NiPoint3_UNIT_ALL, NiPoint3_ZERO, NiPoint3_UNIT_X, NiPoint3_UNIT_Y, NiPoint3_UNIT_Z);
    }
    v2->LoadBinary(kStream);
    return v2;
}

// The separating-axis test of a triangle swept from kV0 to kV1 against the box: the triangle's normal, the three box
// axes, then the nine axis x edge crosses. The compares follow the C++ conditions of the macros exactly, NaN included.
bool NiBoxBV::BoxTriTestIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kV0,
    const NiPoint3& kVert0,
    const NiPoint3& kVert1,
    const NiPoint3& kVert2,
    const NiPoint3& kV1
) {
    const NiBoxBV& kBBV = static_cast<const NiBoxBV&>(kBV0);
    const NiPoint3* A = kBBV.m_kBox.m_akAxis;
    const float* extA = kBBV.m_kBox.m_afExtent;

    // EXE-INLINED 0x439EF0
    NiPoint3 W;
    W.x = kV1.x - kV0.x;
    W.y = kV1.y - kV0.y;
    W.z = kV1.z - kV0.z;
    // EXE-INLINED 0x439EF0
    NiPoint3 E[2];
    E[0].x = kVert1.x - kVert0.x;
    E[0].y = kVert1.y - kVert0.y;
    E[0].z = kVert1.z - kVert0.z;
    E[1].x = kVert2.x - kVert0.x;
    E[1].y = kVert2.y - kVert0.y;
    E[1].z = kVert2.z - kVert0.z;
    // EXE-INLINED 0x4B3800
    NiPoint3 N;
    N.x = E[0].y * E[1].z - E[0].z * E[1].y;
    N.y = E[0].z * E[1].x - E[0].x * E[1].z;
    N.z = E[0].x * E[1].y - E[0].y * E[1].x;
    const NiPoint3& kCenter = kBBV.m_kBox.m_kCenter;
    // EXE-INLINED 0x439EF0
    NiPoint3 D;
    D.x = kVert0.x - kCenter.x;
    D.y = kVert0.y - kCenter.y;
    D.z = kVert0.z - kCenter.z;

    // EXE-INLINED 0x4B6190
    float A0dN = A[0].x * N.x + A[0].y * N.y + A[0].z * N.z;
    float A1dN = A[1].x * N.x + A[1].y * N.y + A[1].z * N.z;
    float A2dN = A[2].x * N.x + A[2].y * N.y + A[2].z * N.z;
    // EXE-INLINED 0x408820
    float R = __builtin_fabsf(extA[0] * A0dN) + __builtin_fabsf(extA[1] * A1dN) + __builtin_fabsf(extA[2] * A2dN);
    // EXE-INLINED 0x4B6190
    float NdD = N.x * D.x + N.y * D.y + N.z * D.z;
    // BT_TEST0(NdD, R, fTime, N, W), written out with N.Dot(W) expanded.
    if (NdD > R) {
        // EXE-INLINED 0x4B6190
        if (NdD + (fTime * (N.x * W.x + N.y * W.y + N.z * W.z)) > R) {
            return false;
        }
    }
    else if ((NdD < -R) && (NdD + (fTime * (N.x * W.x + N.y * W.y + N.z * W.z)) < -R))
    // EXE-INLINED 0x4B6190
    {
        return false;
    }

    // EXE-INLINED 0x4B6190
    float A0dD = A[0].x * D.x + A[0].y * D.y + A[0].z * D.z;
    float A0dE0 = A[0].x * E[0].x + A[0].y * E[0].y + A[0].z * E[0].z;
    float A0dE1 = A[0].x * E[1].x + A[0].y * E[1].y + A[0].z * E[1].z;
    float A0dW = A[0].x * W.x + A[0].y * W.y + A[0].z * W.z;
    BT_TEST1(A0dD, fTime, A0dW, A0dE0, A0dE1, extA[0]);

    // EXE-INLINED 0x4B6190
    float A1dD = A[1].x * D.x + A[1].y * D.y + A[1].z * D.z;
    float A1dE0 = A[1].x * E[0].x + A[1].y * E[0].y + A[1].z * E[0].z;
    float A1dE1 = A[1].x * E[1].x + A[1].y * E[1].y + A[1].z * E[1].z;
    float A1dW = A[1].x * W.x + A[1].y * W.y + A[1].z * W.z;
    BT_TEST1(A1dD, fTime, A1dW, A1dE0, A1dE1, extA[1]);

    // EXE-INLINED 0x4B6190
    float A2dD = A[2].x * D.x + A[2].y * D.y + A[2].z * D.z;
    float A2dE0 = A[2].x * E[0].x + A[2].y * E[0].y + A[2].z * E[0].z;
    float A2dE1 = A[2].x * E[1].x + A[2].y * E[1].y + A[2].z * E[1].z;
    float A2dW = A[2].x * W.x + A[2].y * W.y + A[2].z * W.z;
    BT_TEST1(A2dD, fTime, A2dW, A2dE0, A2dE1, extA[2]);

    // EXE-INLINED 0x4B3800
    NiPoint3 A0xE0;
    A0xE0.x = A[0].y * E[0].z - A[0].z * E[0].y;
    A0xE0.y = A[0].z * E[0].x - A[0].x * E[0].z;
    A0xE0.z = A[0].x * E[0].y - A[0].y * E[0].x;
    // EXE-INLINED 0x4B6190
    float A0xE0dD = A0xE0.x * D.x + A0xE0.y * D.y + A0xE0.z * D.z;
    float A0xE0dW = A0xE0.x * W.x + A0xE0.y * W.y + A0xE0.z * W.z;
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(extA[1] * A2dE0) + __builtin_fabsf(extA[2] * A1dE0);
    BT_TEST2(A0xE0dD, fTime, A0xE0dW, A0dN, R);

    // EXE-INLINED 0x4B3800
    NiPoint3 A0xE1;
    A0xE1.x = A[0].y * E[1].z - A[0].z * E[1].y;
    A0xE1.y = A[0].z * E[1].x - A[0].x * E[1].z;
    A0xE1.z = A[0].x * E[1].y - A[0].y * E[1].x;
    // EXE-INLINED 0x4B6190
    float A0xE1dD = A0xE1.x * D.x + A0xE1.y * D.y + A0xE1.z * D.z;
    float A0xE1dW = A0xE1.x * W.x + A0xE1.y * W.y + A0xE1.z * W.z;
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(extA[1] * A2dE1) + __builtin_fabsf(extA[2] * A1dE1);
    BT_TEST2(A0xE1dD, fTime, A0xE1dW, -A0dN, R);

    float A1dE2 = A1dE1 - A1dE0;
    float A2dE2 = A2dE1 - A2dE0;
    float A0xE2dD = A0xE1dD - A0xE0dD;
    float A0xE2dW = A0xE1dW - A0xE0dW;
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(extA[1] * A2dE2) + __builtin_fabsf(extA[2] * A1dE2);
    BT_TEST2(A0xE2dD, fTime, A0xE2dW, -A0dN, R);

    NiPoint3 A1xE0 = A[1].Cross(E[0]);
    // EXE-INLINED 0x4B6190
    float A1xE0dD = A1xE0.x * D.x + A1xE0.y * D.y + A1xE0.z * D.z;
    float A1xE0dW = A1xE0.x * W.x + A1xE0.y * W.y + A1xE0.z * W.z;
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(extA[0] * A2dE0) + __builtin_fabsf(extA[2] * A0dE0);
    BT_TEST2(A1xE0dD, fTime, A1xE0dW, A1dN, R);

    NiPoint3 A1xE1 = A[1].Cross(E[1]);
    // EXE-INLINED 0x4B6190
    float A1xE1dD = A1xE1.x * D.x + A1xE1.y * D.y + A1xE1.z * D.z;
    float A1xE1dW = A1xE1.x * W.x + A1xE1.y * W.y + A1xE1.z * W.z;
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(extA[0] * A2dE1) + __builtin_fabsf(extA[2] * A0dE1);
    BT_TEST2(A1xE1dD, fTime, A1xE1dW, -A1dN, R);

    float A0dE2 = A0dE1 - A0dE0;
    float A1xE2dD = A1xE1dD - A1xE0dD;
    float A1xE2dW = A1xE1dW - A1xE0dW;
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(extA[0] * A2dE2) + __builtin_fabsf(extA[2] * A0dE2);
    BT_TEST2(A1xE2dD, fTime, A1xE2dW, -A1dN, R);

    NiPoint3 A2xE0 = A[2].Cross(E[0]);
    // EXE-INLINED 0x4B6190
    float A2xE0dD = A2xE0.x * D.x + A2xE0.y * D.y + A2xE0.z * D.z;
    float A2xE0dW = A2xE0.x * W.x + A2xE0.y * W.y + A2xE0.z * W.z;
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(extA[0] * A1dE0) + __builtin_fabsf(extA[1] * A0dE0);
    BT_TEST2(A2xE0dD, fTime, A2xE0dW, A2dN, R);

    NiPoint3 A2xE1 = A[2].Cross(E[1]);
    float A2xE1dD = A2xE1.Dot(D);
    float A2xE1dW = A2xE1.Dot(W);
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(extA[0] * A1dE1) + __builtin_fabsf(extA[1] * A0dE1);
    BT_TEST2(A2xE1dD, fTime, A2xE1dW, -A2dN, R);

    float A2xE2dD = A2xE1dD - A2xE0dD;
    float A2xE2dW = A2xE1dW - A2xE0dW;
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(extA[0] * A1dE2) + __builtin_fabsf(extA[1] * A0dE2);
    BT_TEST2(A2xE2dD, fTime, A2xE2dW, -A2dN, R);

    return true;
}

// The separating-axis test of box kBV0 moving by kV0 and box kBV1 moving by kV1 over fTime: the three axes of each box
// and the nine axis crosses, each tested at D0 (the centres' offset now) and D1 (at fTime), then the six axes W x A[i]
// and W x B[i]. Every stored intermediate is a float; the D1 sides of the cross tests stay unrounded, as the game does.
bool NiBoxBV::BoxBoxTestIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kBV1,
    const NiPoint3& kV1
) {
    const NiBoxBV& kBBV0 = static_cast<const NiBoxBV&>(kBV0);
    const NiBoxBV& kBBV1 = static_cast<const NiBoxBV&>(kBV1);
    const NiPoint3* A = kBBV0.m_kBox.m_akAxis;
    const NiPoint3* B = kBBV1.m_kBox.m_akAxis;
    const float* extA = kBBV0.m_kBox.m_afExtent;
    const float* extB = kBBV1.m_kBox.m_afExtent;

    float AB[3][3];
    float fAB[3][3];
    float AD0[3];
    float AD1[3];
    float R0;
    float R1;
    float R;
    float R01;

    // EXE-INLINED 0x439EF0
    NiPoint3 W;
    W.x = kV1.x - kV0.x;
    W.y = kV1.y - kV0.y;
    W.z = kV1.z - kV0.z;
    // EXE-INLINED 0x439EF0
    NiPoint3 D0;
    D0.x = kBBV1.m_kBox.m_kCenter.x - kBBV0.m_kBox.m_kCenter.x;
    D0.y = kBBV1.m_kBox.m_kCenter.y - kBBV0.m_kBox.m_kCenter.y;
    D0.z = kBBV1.m_kBox.m_kCenter.z - kBBV0.m_kBox.m_kCenter.z;
    // D1 = D0 + fTime * W: the product is rounded to float before the sum.
    // EXE-INLINED 0x4A3760
    // EXE-INLINED 0x439E90
    NiPoint3 D1;
    D1.x = D0.x + (fTime * W.x);
    D1.y = D0.y + (fTime * W.y);
    D1.z = D0.z + (fTime * W.z);

    // The axes A[0], A[1], A[2].
    for (int32_t i = 0; i < 3; i++) {
        // EXE-INLINED 0x4B6190
        AB[i][0] = A[i].x * B[0].x + A[i].y * B[0].y + A[i].z * B[0].z;
        AB[i][1] = A[i].x * B[1].x + A[i].y * B[1].y + A[i].z * B[1].z;
        AB[i][2] = A[i].x * B[2].x + A[i].y * B[2].y + A[i].z * B[2].z;
        AD0[i] = A[i].x * D0.x + A[i].y * D0.y + A[i].z * D0.z;
        AD1[i] = A[i].x * D1.x + A[i].y * D1.y + A[i].z * D1.z;
        // EXE-INLINED 0x408820
        fAB[i][0] = __builtin_fabsf(AB[i][0]);
        fAB[i][1] = __builtin_fabsf(AB[i][1]);
        fAB[i][2] = __builtin_fabsf(AB[i][2]);
        R1 = extB[0] * fAB[i][0] + extB[1] * fAB[i][1] + extB[2] * fAB[i][2];
        R01 = extA[i] + R1;
        if (AD0[i] > R01) {
            if (AD1[i] > R01) {
                return false;
            }
        }
        else if ((AD0[i] < -R01) && (AD1[i] < -R01)) {
            return false;
        }
    }

    // The axis B[0]: both of its D1 dots inline.
    float fBD1;
    R0 = extA[0] * fAB[0][0] + extA[1] * fAB[1][0] + extA[2] * fAB[2][0];
    // EXE-INLINED 0x4B6190
    R = B[0].x * D0.x + B[0].y * D0.y + B[0].z * D0.z;
    R01 = R0 + extB[0];
    if (R > R01) {
        // EXE-INLINED 0x4B6190
        fBD1 = B[0].x * D1.x + B[0].y * D1.y + B[0].z * D1.z;
        if (fBD1 > R01) {
            return false;
        }
    }
    else if (R < -R01) {
        // EXE-INLINED 0x4B6190
        fBD1 = B[0].x * D1.x + B[0].y * D1.y + B[0].z * D1.z;
        if (fBD1 < -R01) {
            return false;
        }
    }

    // The axis B[1]: the "below" side calls Dot.
    R0 = extA[0] * fAB[0][1] + extA[1] * fAB[1][1] + extA[2] * fAB[2][1];
    // EXE-INLINED 0x4B6190
    R = B[1].x * D0.x + B[1].y * D0.y + B[1].z * D0.z;
    R01 = R0 + extB[1];
    if (R > R01) {
        // EXE-INLINED 0x4B6190
        fBD1 = B[1].x * D1.x + B[1].y * D1.y + B[1].z * D1.z;
        if (fBD1 > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (B[1].Dot(D1) < -R01)) {
        return false;
    }

    // The axis B[2]: the "below" side calls Dot.
    R0 = extA[0] * fAB[0][2] + extA[1] * fAB[1][2] + extA[2] * fAB[2][2];
    // EXE-INLINED 0x4B6190
    R = B[2].x * D0.x + B[2].y * D0.y + B[2].z * D0.z;
    R01 = R0 + extB[2];
    if (R > R01) {
        // EXE-INLINED 0x4B6190
        fBD1 = B[2].x * D1.x + B[2].y * D1.y + B[2].z * D1.z;
        if (fBD1 > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (B[2].Dot(D1) < -R01)) {
        return false;
    }

    // A[0] x B[0]
    R0 = extA[1] * fAB[2][0] + extA[2] * fAB[1][0];
    R1 = extB[1] * fAB[0][2] + extB[2] * fAB[0][1];
    R = AD0[2] * AB[1][0] - AD0[1] * AB[2][0];
    R01 = R0 + R1;
    if (R > R01) {
        if (AD1[2] * AB[1][0] - AD1[1] * AB[2][0] > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (AD1[2] * AB[1][0] - AD1[1] * AB[2][0] < -R01)) {
        return false;
    }

    // A[0] x B[1]
    R0 = extA[1] * fAB[2][1] + extA[2] * fAB[1][1];
    R1 = extB[0] * fAB[0][2] + extB[2] * fAB[0][0];
    R = AD0[2] * AB[1][1] - AD0[1] * AB[2][1];
    R01 = R0 + R1;
    if (R > R01) {
        if (AD1[2] * AB[1][1] - AD1[1] * AB[2][1] > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (AD1[2] * AB[1][1] - AD1[1] * AB[2][1] < -R01)) {
        return false;
    }

    // A[0] x B[2]
    R0 = extA[1] * fAB[2][2] + extA[2] * fAB[1][2];
    R1 = extB[0] * fAB[0][1] + extB[1] * fAB[0][0];
    R = AD0[2] * AB[1][2] - AD0[1] * AB[2][2];
    R01 = R0 + R1;
    if (R > R01) {
        if (AD1[2] * AB[1][2] - AD1[1] * AB[2][2] > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (AD1[2] * AB[1][2] - AD1[1] * AB[2][2] < -R01)) {
        return false;
    }

    // A[1] x B[0]
    R0 = extA[0] * fAB[2][0] + extA[2] * fAB[0][0];
    R1 = extB[1] * fAB[1][2] + extB[2] * fAB[1][1];
    R = AD0[0] * AB[2][0] - AD0[2] * AB[0][0];
    R01 = R0 + R1;
    if (R > R01) {
        if (AD1[0] * AB[2][0] - AD1[2] * AB[0][0] > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (AD1[0] * AB[2][0] - AD1[2] * AB[0][0] < -R01)) {
        return false;
    }

    // A[1] x B[1]
    R0 = extA[0] * fAB[2][1] + extA[2] * fAB[0][1];
    R1 = extB[0] * fAB[1][2] + extB[2] * fAB[1][0];
    R = AD0[0] * AB[2][1] - AD0[2] * AB[0][1];
    R01 = R0 + R1;
    if (R > R01) {
        if (AD1[0] * AB[2][1] - AD1[2] * AB[0][1] > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (AD1[0] * AB[2][1] - AD1[2] * AB[0][1] < -R01)) {
        return false;
    }

    // A[1] x B[2]
    R0 = extA[0] * fAB[2][2] + extA[2] * fAB[0][2];
    R1 = extB[0] * fAB[1][1] + extB[1] * fAB[1][0];
    R = AD0[0] * AB[2][2] - AD0[2] * AB[0][2];
    R01 = R0 + R1;
    if (R > R01) {
        if (AD1[0] * AB[2][2] - AD1[2] * AB[0][2] > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (AD1[0] * AB[2][2] - AD1[2] * AB[0][2] < -R01)) {
        return false;
    }

    // A[2] x B[0]
    R0 = extA[0] * fAB[1][0] + extA[1] * fAB[0][0];
    R1 = extB[1] * fAB[2][2] + extB[2] * fAB[2][1];
    R = AD0[1] * AB[0][0] - AD0[0] * AB[1][0];
    R01 = R0 + R1;
    if (R > R01) {
        if (AD1[1] * AB[0][0] - AD1[0] * AB[1][0] > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (AD1[1] * AB[0][0] - AD1[0] * AB[1][0] < -R01)) {
        return false;
    }

    // A[2] x B[1]
    R0 = extA[0] * fAB[1][1] + extA[1] * fAB[0][1];
    R1 = extB[0] * fAB[2][2] + extB[2] * fAB[2][0];
    R = AD0[1] * AB[0][1] - AD0[0] * AB[1][1];
    R01 = R0 + R1;
    if (R > R01) {
        if (AD1[1] * AB[0][1] - AD1[0] * AB[1][1] > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (AD1[1] * AB[0][1] - AD1[0] * AB[1][1] < -R01)) {
        return false;
    }

    // A[2] x B[2]
    R0 = extA[0] * fAB[1][2] + extA[1] * fAB[0][2];
    R1 = extB[0] * fAB[2][1] + extB[1] * fAB[2][0];
    R = AD0[1] * AB[0][2] - AD0[0] * AB[1][2];
    R01 = R0 + R1;
    if (R > R01) {
        if (AD1[1] * AB[0][2] - AD1[0] * AB[1][2] > R01) {
            return false;
        }
    }
    else if ((R < -R01) && (AD1[1] * AB[0][2] - AD1[0] * AB[1][2] < -R01)) {
        return false;
    }

    // The axes W x A[i] and W x B[i], tested against kWxD0 = W x D0 alone.
    NiPoint3 kWxD0 = W.Cross(D0);
    float afWA[3];
    float afWB[3];

    // W x A[0]
    afWA[1] = W.Dot(A[1]);
    afWA[2] = W.Dot(A[2]);
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(A[0].Dot(kWxD0));
    R0 = extA[1] * __builtin_fabsf(afWA[2]) + extA[2] * __builtin_fabsf(afWA[1]);
    R1 = extB[0] * __builtin_fabsf(AB[1][0] * afWA[2] - AB[2][0] * afWA[1]) +
        extB[1] * __builtin_fabsf(AB[1][1] * afWA[2] - AB[2][1] * afWA[1]) +
        extB[2] * __builtin_fabsf(AB[1][2] * afWA[2] - AB[2][2] * afWA[1]);
    R01 = R0 + R1;
    if (R > R01) {
        return false;
    }

    // W x A[1]
    afWA[0] = W.Dot(A[0]);
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(A[1].Dot(kWxD0));
    R0 = extA[2] * __builtin_fabsf(afWA[0]) + extA[0] * __builtin_fabsf(afWA[2]);
    R1 = extB[0] * __builtin_fabsf(AB[2][0] * afWA[0] - AB[0][0] * afWA[2]) +
        extB[1] * __builtin_fabsf(AB[2][1] * afWA[0] - AB[0][1] * afWA[2]) +
        extB[2] * __builtin_fabsf(AB[2][2] * afWA[0] - AB[0][2] * afWA[2]);
    R01 = R0 + R1;
    if (R > R01) {
        return false;
    }

    // W x A[2]
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(A[2].Dot(kWxD0));
    R0 = extA[0] * __builtin_fabsf(afWA[1]) + extA[1] * __builtin_fabsf(afWA[0]);
    R1 = extB[0] * __builtin_fabsf(AB[0][0] * afWA[1] - AB[1][0] * afWA[0]) +
        extB[1] * __builtin_fabsf(AB[0][1] * afWA[1] - AB[1][1] * afWA[0]) +
        extB[2] * __builtin_fabsf(AB[0][2] * afWA[1] - AB[1][2] * afWA[0]);
    R01 = R0 + R1;
    if (R > R01) {
        return false;
    }

    // W x B[0]
    afWB[1] = W.Dot(B[1]);
    afWB[2] = W.Dot(B[2]);
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(B[0].Dot(kWxD0));
    R0 = extA[0] * __builtin_fabsf(AB[0][1] * afWB[2] - AB[0][2] * afWB[1]) +
        extA[1] * __builtin_fabsf(AB[1][1] * afWB[2] - AB[1][2] * afWB[1]) +
        extA[2] * __builtin_fabsf(AB[2][1] * afWB[2] - AB[2][2] * afWB[1]);
    R1 = extB[1] * __builtin_fabsf(afWB[2]) + extB[2] * __builtin_fabsf(afWB[1]);
    R01 = R0 + R1;
    if (R > R01) {
        return false;
    }

    // W x B[1]
    afWB[0] = W.Dot(B[0]);
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(B[1].Dot(kWxD0));
    R0 = extA[0] * __builtin_fabsf(AB[0][2] * afWB[0] - AB[0][0] * afWB[2]) +
        extA[1] * __builtin_fabsf(AB[1][2] * afWB[0] - AB[1][0] * afWB[2]) +
        extA[2] * __builtin_fabsf(AB[2][2] * afWB[0] - AB[2][0] * afWB[2]);
    R1 = extB[2] * __builtin_fabsf(afWB[0]) + extB[0] * __builtin_fabsf(afWB[2]);
    R01 = R0 + R1;
    if (R > R01) {
        return false;
    }

    // W x B[2]
    // EXE-INLINED 0x408820
    R = __builtin_fabsf(B[2].Dot(kWxD0));
    R0 = extA[0] * __builtin_fabsf(AB[0][0] * afWB[1] - AB[0][1] * afWB[0]) +
        extA[1] * __builtin_fabsf(AB[1][0] * afWB[1] - AB[1][1] * afWB[0]) +
        extA[2] * __builtin_fabsf(AB[2][0] * afWB[1] - AB[2][1] * afWB[0]);
    R1 = extB[0] * __builtin_fabsf(afWB[1]) + extB[1] * __builtin_fabsf(afWB[0]);
    R01 = R0 + R1;
    // Inverting the float test into `return R <= R01` would change the NaN result.
    if (R > R01) {
        return false; // NOLINT(readability-simplify-boolean-expr)
    }

    return true;
}

bool NiBoxBV::BoxSphereTestIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kBV1,
    const NiPoint3& kV1
) {
    const NiBoxBV& kBBV = static_cast<const NiBoxBV&>(kBV0);
    const NiSphereBV& kSBV = static_cast<const NiSphereBV&>(kBV1);
    const NiPoint3* A = kBBV.m_kBox.m_akAxis;
    const float* extA = kBBV.m_kBox.m_afExtent;

    // EXE-INLINED 0x439EF0
    NiPoint3 kDeltaC;
    kDeltaC.x = kSBV.m_kSphere.m_kCenter.x - kBBV.m_kBox.m_kCenter.x;
    kDeltaC.y = kSBV.m_kSphere.m_kCenter.y - kBBV.m_kBox.m_kCenter.y;
    kDeltaC.z = kSBV.m_kSphere.m_kCenter.z - kBBV.m_kBox.m_kCenter.z;
    // EXE-INLINED 0x4B6190
    float fCx = kDeltaC.x * A[0].x + kDeltaC.y * A[0].y + kDeltaC.z * A[0].z;
    // EXE-INLINED 0x408820
    if (__builtin_fabsf(fCx) <= extA[0]) {
        // EXE-INLINED 0x4B6190
        float fCy = kDeltaC.x * A[1].x + kDeltaC.y * A[1].y + kDeltaC.z * A[1].z;
        // EXE-INLINED 0x408820
        if (__builtin_fabsf(fCy) <= extA[1]) {
            // EXE-INLINED 0x4B6190
            float fCz = kDeltaC.x * A[2].x + kDeltaC.y * A[2].y + kDeltaC.z * A[2].z;
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(fCz) <= extA[2]) {
                return true;
            }
        }
    }

    // EXE-INLINED 0x439EF0
    NiPoint3 kDeltaV;
    kDeltaV.x = kV1.x - kV0.x;
    kDeltaV.y = kV1.y - kV0.y;
    kDeltaV.z = kV1.z - kV0.z;

    float fSqrDist;
    float fR0;
    float fR1;
    float fR2;
    // EXE-INLINED 0x439090
    if (kDeltaV.x == NiPoint3_ZERO.x && kDeltaV.y == NiPoint3_ZERO.y && kDeltaV.z == NiPoint3_ZERO.z) {
        fSqrDist = NiSqrDistance::Compute(kSBV.m_kSphere.m_kCenter, kBBV.m_kBox, fR0, fR1, fR2);
    }
    else {
        NiSegment kSeg;
        kSeg.m_kOrigin = kSBV.m_kSphere.m_kCenter;
        // EXE-INLINED 0x4A3760
        kSeg.m_kDirection.x = fTime * kDeltaV.x;
        kSeg.m_kDirection.y = fTime * kDeltaV.y;
        kSeg.m_kDirection.z = fTime * kDeltaV.z;

        float fS;
        fSqrDist = NiSqrDistance::Compute(kSeg, kBBV.m_kBox, fS, fR0, fR1, fR2);
    }

    return fSqrDist <= kSBV.m_kSphere.m_fRadius * kSBV.m_kSphere.m_fRadius;
}

bool NiBoxBV::BoxCapsuleTestIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kBV1,
    const NiPoint3& kV1
) {
    const NiBoxBV& kBBV = static_cast<const NiBoxBV&>(kBV0);
    const NiCapsuleBV& kCBV = static_cast<const NiCapsuleBV&>(kBV1);

    // EXE-INLINED 0x439EF0
    NiPoint3 kDeltaC;
    kDeltaC.x = kCBV.m_kCenter.x - kBBV.m_kBox.m_kCenter.x;
    kDeltaC.y = kCBV.m_kCenter.y - kBBV.m_kBox.m_kCenter.y;
    kDeltaC.z = kCBV.m_kCenter.z - kBBV.m_kBox.m_kCenter.z;
    const NiPoint3* A = kBBV.m_kBox.m_akAxis;
    // EXE-INLINED 0x4B6190
    float fCx = kDeltaC.x * A[0].x + kDeltaC.y * A[0].y + kDeltaC.z * A[0].z;
    // EXE-INLINED 0x408820
    if (__builtin_fabsf(fCx) <= kBBV.m_kBox.m_afExtent[0]) {
        // EXE-INLINED 0x4B6190
        float fCy = kDeltaC.x * A[1].x + kDeltaC.y * A[1].y + kDeltaC.z * A[1].z;
        // EXE-INLINED 0x408820
        if (__builtin_fabsf(fCy) <= kBBV.m_kBox.m_afExtent[1]) {
            // EXE-INLINED 0x4B6190
            float fCz = kDeltaC.x * A[2].x + kDeltaC.y * A[2].y + kDeltaC.z * A[2].z;
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(fCz) <= kBBV.m_kBox.m_afExtent[2]) {
                return true;
            }
        }
    }

    NiBoxCapsuleIntersector kIntersector(kBBV, kCBV, fTime, 0.001f, 0.0001f, 32);
    kIntersector.NiIntersector::Test(kV0, kV1);
    bool bResult = kIntersector.m_eIntersectionType == NiIntersector::IT_OVERLAP ||
        kIntersector.m_eIntersectionType == NiIntersector::IT_CONTACT;
    return bResult;
}

bool NiBoxBV::BoxSphereFindIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kBV1,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    bool bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    // The intersector holds references: the native side constructs it in its declaration.
    NiBoxSphereIntersector kIntersector(
        static_cast<const NiBoxBV&>(kBV0), static_cast<const NiSphereBV&>(kBV1), fTime, 0.001f, 0.0001f, 32
    );
    kIntersector.NiIntersector::Find(kV0, kV1);
    fIntrTime = kIntersector.GetContactTime();
    if (kIntersector.m_eIntersectionType == NiIntersector::IT_OVERLAP ||
        kIntersector.m_eIntersectionType == NiIntersector::IT_CONTACT) {
        kIntrPt = kIntersector.GetContactPoint();
        if (bCalcNormals) {
            kNormal0 = kIntersector.GetContactNormal();
            // EXE-INLINED 0x4A0BD0
            NiPoint3 kNegated;
            kNegated.x = -kNormal0.x;
            kNegated.y = -kNormal0.y;
            kNegated.z = -kNormal0.z;
            kNormal1 = kNegated;
        }
        return true;
    }
    return false;
}

bool NiBoxBV::BoxCapsuleFindIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kBV1,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    bool bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    // The intersector holds references: the native side constructs it in its declaration.
    NiBoxCapsuleIntersector kIntersector(
        static_cast<const NiBoxBV&>(kBV0), static_cast<const NiCapsuleBV&>(kBV1), fTime, 0.001f, 0.0001f, 32
    );
    kIntersector.NiIntersector::Find(kV0, kV1);
    fIntrTime = kIntersector.GetContactTime();
    if (kIntersector.m_eIntersectionType == NiIntersector::IT_OVERLAP ||
        kIntersector.m_eIntersectionType == NiIntersector::IT_CONTACT) {
        kIntrPt = kIntersector.GetContactPoint();
        if (bCalcNormals) {
            kNormal0 = kIntersector.GetContactNormal();
            // EXE-INLINED 0x4A0BD0
            NiPoint3 kNegated;
            kNegated.x = -kNormal0.x;
            kNegated.y = -kNormal0.y;
            kNegated.z = -kNormal0.z;
            kNormal1 = kNegated;
        }
        return true;
    }
    return false;
}

// The sweep test of BoxTriTestIntersect, keeping the latest time of first contact with its axis (uiType), the triangle
// vertex that meets the box first (uiExtr) and the box side (iSide); then the contact point on that axis and, when
// asked, the triangle's normals. Each edge case computes its four dots with NDE last. Compares follow the C++
// conditions of the macros, NaN included. Case 2-4 adds kV1, not kV0, deliberately, as Gamebryo's source does.
bool NiBoxBV::BoxTriFindIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
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
    const NiBoxBV& kBBV = static_cast<const NiBoxBV&>(kBV0);
    const NiPoint3* A = kBBV.m_kBox.m_akAxis;
    const float* extA = kBBV.m_kBox.m_afExtent;

    // EXE-INLINED 0x439EF0
    NiPoint3 W;
    W.x = kV1.x - kV0.x;
    W.y = kV1.y - kV0.y;
    W.z = kV1.z - kV0.z;

    NiPoint3 D;
    NiPoint3 E[3];
    NiPoint3 N;
    // EXE-INLINED 0x439EF0
    E[0].x = kVert1.x - kVert0.x;
    E[0].y = kVert1.y - kVert0.y;
    E[0].z = kVert1.z - kVert0.z;
    // EXE-INLINED 0x439EF0
    E[1].x = kVert2.x - kVert0.x;
    E[1].y = kVert2.y - kVert0.y;
    E[1].z = kVert2.z - kVert0.z;
    // EXE-INLINED 0x4B3800
    N.x = E[0].y * E[1].z - E[0].z * E[1].y;
    N.y = E[0].z * E[1].x - E[0].x * E[1].z;
    N.z = E[0].x * E[1].y - E[0].y * E[1].x;
    const NiPoint3& kCenter = kBBV.m_kBox.m_kCenter;
    // EXE-INLINED 0x439EF0
    D.x = kVert0.x - kCenter.x;
    D.y = kVert0.y - kCenter.y;
    D.z = kVert0.z - kCenter.z;

    uint32_t uiType = 0;
    uint32_t uiExtr = 0;
    int32_t iSide = 0;
    fIntrTime = 0.0f;

    float AN[3];
    // EXE-INLINED 0x4B6190
    AN[0] = A[0].x * N.x + A[0].y * N.y + A[0].z * N.z;
    AN[1] = A[1].x * N.x + A[1].y * N.y + A[1].z * N.z;
    AN[2] = A[2].x * N.x + A[2].y * N.y + A[2].z * N.z;
    // EXE-INLINED 0x408820
    float R = __builtin_fabsf(AN[0]) * extA[0] + __builtin_fabsf(AN[1]) * extA[1] + __builtin_fabsf(AN[2]) * extA[2];
    // EXE-INLINED 0x4B6190
    float NdD = N.x * D.x + N.y * D.y + N.z * D.z;
    // BT_FIND0(NdD, R, fTime, N, W, fIntrTime, uiType, iSide, 1), written out, with N.Dot(W) expanded.
    if (NdD > R) {
        // EXE-INLINED 0x4B6190
        float NdW = N.x * W.x + N.y * W.y + N.z * W.z;
        if (NdD + (fTime * NdW) > R) {
            return false;
        }
        float fTmp = (R - NdD) / NdW;
        if (fTmp > fIntrTime) {
            fIntrTime = fTmp;
            uiType = 1;
            iSide = 1;
        }
    }
    else if (NdD < -R) {
        // EXE-INLINED 0x4B6190
        float NdW = N.x * W.x + N.y * W.y + N.z * W.z;
        if (NdD + (fTime * NdW) < -R) {
            return false;
        }
        float fTmp = -(R + NdD) / NdW;
        if (fTmp > fIntrTime) {
            fIntrTime = fTmp;
            uiType = 1;
            iSide = -1;
        }
    }

    float AD[3];
    float AE[3][3];
    for (int32_t i = 0; i < 3; i++) {
        // EXE-INLINED 0x4B6190
        AD[i] = A[i].x * D.x + A[i].y * D.y + A[i].z * D.z;
        AE[i][0] = A[i].x * E[0].x + A[i].y * E[0].y + A[i].z * E[0].z;
        AE[i][1] = A[i].x * E[1].x + A[i].y * E[1].y + A[i].z * E[1].z;
        float AidW = A[i].x * W.x + A[i].y * W.y + A[i].z * W.z;
        BT_FIND1(AD[i], fTime, AidW, AE[i][0], AE[i][1], extA[i], fIntrTime, uiType, uiExtr, iSide, i + 2);
    }

    // EXE-INLINED 0x4B3800
    NiPoint3 A0xE0;
    A0xE0.x = A[0].y * E[0].z - A[0].z * E[0].y;
    A0xE0.y = A[0].z * E[0].x - A[0].x * E[0].z;
    A0xE0.z = A[0].x * E[0].y - A[0].y * E[0].x;
    // EXE-INLINED 0x4B6190
    float A0xE0dD = A0xE0.x * D.x + A0xE0.y * D.y + A0xE0.z * D.z;
    float A0xE0dW = A0xE0.x * W.x + A0xE0.y * W.y + A0xE0.z * W.z;
    // EXE-INLINED 0x408820
    R = extA[1] * __builtin_fabsf(AE[2][0]) + extA[2] * __builtin_fabsf(AE[1][0]);
    BT_FIND2(A0xE0dD, fTime, A0xE0dW, AN[0], R, fIntrTime, uiType, uiExtr, iSide, 5);

    // EXE-INLINED 0x4B3800
    NiPoint3 A0xE1;
    A0xE1.x = A[0].y * E[1].z - A[0].z * E[1].y;
    A0xE1.y = A[0].z * E[1].x - A[0].x * E[1].z;
    A0xE1.z = A[0].x * E[1].y - A[0].y * E[1].x;
    // EXE-INLINED 0x4B6190
    float A0xE1dD = A0xE1.x * D.x + A0xE1.y * D.y + A0xE1.z * D.z;
    float A0xE1dW = A0xE1.x * W.x + A0xE1.y * W.y + A0xE1.z * W.z;
    // EXE-INLINED 0x408820
    R = extA[1] * __builtin_fabsf(AE[2][1]) + extA[2] * __builtin_fabsf(AE[1][1]);
    BT_FIND2(A0xE1dD, fTime, A0xE1dW, -AN[0], R, fIntrTime, uiType, uiExtr, iSide, 6);

    AE[1][2] = AE[1][1] - AE[1][0];
    AE[2][2] = AE[2][1] - AE[2][0];
    float A0xE2dD = A0xE1dD - A0xE0dD;
    float A0xE2dW = A0xE1dW - A0xE0dW;
    // EXE-INLINED 0x408820
    R = extA[1] * __builtin_fabsf(AE[2][2]) + extA[2] * __builtin_fabsf(AE[1][2]);
    BT_FIND2(A0xE2dD, fTime, A0xE2dW, -AN[0], R, fIntrTime, uiType, uiExtr, iSide, 7);

    // EXE-INLINED 0x4B3800
    NiPoint3 A1xE0;
    A1xE0.x = A[1].y * E[0].z - A[1].z * E[0].y;
    A1xE0.y = A[1].z * E[0].x - A[1].x * E[0].z;
    A1xE0.z = A[1].x * E[0].y - A[1].y * E[0].x;
    // EXE-INLINED 0x4B6190
    float A1xE0dD = A1xE0.x * D.x + A1xE0.y * D.y + A1xE0.z * D.z;
    float A1xE0dW = A1xE0.x * W.x + A1xE0.y * W.y + A1xE0.z * W.z;
    // EXE-INLINED 0x408820
    R = extA[0] * __builtin_fabsf(AE[2][0]) + extA[2] * __builtin_fabsf(AE[0][0]);
    BT_FIND2(A1xE0dD, fTime, A1xE0dW, AN[1], R, fIntrTime, uiType, uiExtr, iSide, 8);

    // EXE-INLINED 0x4B3800
    NiPoint3 A1xE1;
    A1xE1.x = A[1].y * E[1].z - A[1].z * E[1].y;
    A1xE1.y = A[1].z * E[1].x - A[1].x * E[1].z;
    A1xE1.z = A[1].x * E[1].y - A[1].y * E[1].x;
    // EXE-INLINED 0x4B6190
    float A1xE1dD = A1xE1.x * D.x + A1xE1.y * D.y + A1xE1.z * D.z;
    float A1xE1dW = A1xE1.x * W.x + A1xE1.y * W.y + A1xE1.z * W.z;
    // EXE-INLINED 0x408820
    R = extA[0] * __builtin_fabsf(AE[2][1]) + extA[2] * __builtin_fabsf(AE[0][1]);
    BT_FIND2(A1xE1dD, fTime, A1xE1dW, -AN[1], R, fIntrTime, uiType, uiExtr, iSide, 9);

    AE[0][2] = AE[0][1] - AE[0][0];
    float A1xE2dD = A1xE1dD - A1xE0dD;
    float A1xE2dW = A1xE1dW - A1xE0dW;
    // EXE-INLINED 0x408820
    R = extA[0] * __builtin_fabsf(AE[2][2]) + extA[2] * __builtin_fabsf(AE[0][2]);
    BT_FIND2(A1xE2dD, fTime, A1xE2dW, -AN[1], R, fIntrTime, uiType, uiExtr, iSide, 10);

    // EXE-INLINED 0x4B3800
    NiPoint3 A2xE0;
    A2xE0.x = A[2].y * E[0].z - A[2].z * E[0].y;
    A2xE0.y = A[2].z * E[0].x - A[2].x * E[0].z;
    A2xE0.z = A[2].x * E[0].y - A[2].y * E[0].x;
    // EXE-INLINED 0x4B6190
    float A2xE0dD = A2xE0.x * D.x + A2xE0.y * D.y + A2xE0.z * D.z;
    float A2xE0dW = A2xE0.x * W.x + A2xE0.y * W.y + A2xE0.z * W.z;
    // EXE-INLINED 0x408820
    R = extA[0] * __builtin_fabsf(AE[1][0]) + extA[1] * __builtin_fabsf(AE[0][0]);
    BT_FIND2(A2xE0dD, fTime, A2xE0dW, AN[2], R, fIntrTime, uiType, uiExtr, iSide, 11);

    // EXE-INLINED 0x4B3800
    NiPoint3 A2xE1;
    A2xE1.x = A[2].y * E[1].z - A[2].z * E[1].y;
    A2xE1.y = A[2].z * E[1].x - A[2].x * E[1].z;
    A2xE1.z = A[2].x * E[1].y - A[2].y * E[1].x;
    // EXE-INLINED 0x4B6190
    float A2xE1dD = A2xE1.x * D.x + A2xE1.y * D.y + A2xE1.z * D.z;
    float A2xE1dW = A2xE1.x * W.x + A2xE1.y * W.y + A2xE1.z * W.z;
    // EXE-INLINED 0x408820
    R = extA[0] * __builtin_fabsf(AE[1][1]) + extA[1] * __builtin_fabsf(AE[0][1]);
    BT_FIND2(A2xE1dD, fTime, A2xE1dW, -AN[2], R, fIntrTime, uiType, uiExtr, iSide, 12);

    float A2xE2dD = A2xE1dD - A2xE0dD;
    float A2xE2dW = A2xE1dW - A2xE0dW;
    // EXE-INLINED 0x408820
    R = extA[0] * __builtin_fabsf(AE[1][2]) + extA[1] * __builtin_fabsf(AE[0][2]);
    BT_FIND2(A2xE2dD, fTime, A2xE2dW, -AN[2], R, fIntrTime, uiType, uiExtr, iSide, 13);

    float x[3];
    float fNumer;
    float NDE[3];
    float NAE[3][3];
    NiPoint3 DxE[3];
    NiPoint3 A0xE2;
    NiPoint3 A1xE2;
    NiPoint3 A2xE2;

    switch (uiType) {
        case 0: {
            kIntrPt = 0.5f * (fOneThird * (kVert0 + kVert1 + kVert2) + kBBV.m_kBox.m_kCenter);
            break;
        }
        case 1: {
            BT_GET_COEFF_PLUS_MINUS(x[0], iSide, AN[0], extA[0]);
            BT_GET_COEFF_PLUS_MINUS(x[1], iSide, AN[1], extA[1]);
            BT_GET_COEFF_PLUS_MINUS(x[2], iSide, AN[2], extA[2]);
            BT_GET_POINT(kIntrPt, kBBV, fIntrTime, kV0, x);
            break;
        }
        case 2:
        case 3:
        case 4: {
            if (uiExtr == 0) {
                kIntrPt = kVert0 + (fIntrTime * kV1);
            }
            else if (uiExtr == 1) {
                NiPoint3 kMoved = kVert0 + (fIntrTime * kV1);
                // EXE-INLINED 0x439E90
                NiPoint3 kPt;
                kPt.x = kMoved.x + E[0].x;
                kPt.y = kMoved.y + E[0].y;
                kPt.z = kMoved.z + E[0].z;
                kIntrPt = kPt;
            }
            else {
                kIntrPt = kVert0 + (fIntrTime * kV1) + E[1];
            }
            break;
        }
        case 5: {
            BT_GET_COEFF_MINUS_PLUS(x[1], iSide, AE[2][0], extA[1]);
            BT_GET_COEFF_PLUS_MINUS(x[2], iSide, AE[1][0], extA[2]);
            D += (fIntrTime * W);
            DxE[0] = D.Cross(E[0]);
            NAE[0][0] = A0xE0.Dot(A0xE0);
            NAE[1][0] = A0xE0.Dot(A1xE0);
            NAE[2][0] = A0xE0.Dot(A2xE0);
            NDE[0] = A0xE0.Dot(DxE[0]);
            fNumer = NDE[0] - (NAE[1][0] * x[1]) - (NAE[2][0] * x[2]);
            if (uiExtr == 0) {
                x[0] = fNumer / NAE[0][0];
            }
            else {
                x[0] = (fNumer - A0xE0.Dot(N)) / NAE[0][0];
            }
            BT_GET_POINT(kIntrPt, kBBV, fIntrTime, kV0, x);
            break;
        }
        case 6: {
            BT_GET_COEFF_MINUS_PLUS(x[1], iSide, AE[2][1], extA[1]);
            BT_GET_COEFF_PLUS_MINUS(x[2], iSide, AE[1][1], extA[2]);
            D += (fIntrTime * W);
            DxE[1] = D.Cross(E[1]);
            NAE[0][1] = A0xE1.Dot(A0xE1);
            NAE[1][1] = A0xE1.Dot(A1xE1);
            NAE[2][1] = A0xE1.Dot(A2xE1);
            NDE[1] = A0xE1.Dot(DxE[1]);
            fNumer = NDE[1] - (NAE[1][1] * x[1]) - (NAE[2][1] * x[2]);
            if (uiExtr == 0) {
                x[0] = fNumer / NAE[0][1];
            }
            else {
                x[0] = (fNumer + A0xE1.Dot(N)) / NAE[0][1];
            }
            BT_GET_POINT(kIntrPt, kBBV, fIntrTime, kV0, x);
            break;
        }
        case 7: {
            // EXE-INLINED 0x439EF0
            E[2].x = E[1].x - E[0].x;
            E[2].y = E[1].y - E[0].y;
            E[2].z = E[1].z - E[0].z;
            A0xE2 = A[0].Cross(E[2]);
            A1xE2 = A[1].Cross(E[2]);
            A2xE2 = A[2].Cross(E[2]);

            BT_GET_COEFF_MINUS_PLUS(x[1], iSide, AE[2][2], extA[1]);
            BT_GET_COEFF_PLUS_MINUS(x[2], iSide, AE[1][2], extA[2]);
            D += (fIntrTime * W);
            DxE[2] = D.Cross(E[2]);
            NAE[0][2] = A0xE2.Dot(A0xE2);
            NAE[1][2] = A0xE2.Dot(A1xE2);
            NAE[2][2] = A0xE2.Dot(A2xE2);
            NDE[2] = A0xE2.Dot(DxE[2]);
            fNumer = NDE[2] - (NAE[1][2] * x[1]) - (NAE[2][2] * x[2]);
            if (uiExtr == 0) {
                x[0] = fNumer / NAE[0][2];
            }
            else {
                x[0] = (fNumer + A0xE2.Dot(N)) / NAE[0][2];
            }
            BT_GET_POINT(kIntrPt, kBBV, fIntrTime, kV0, x);
            break;
        }
        case 8: {
            BT_GET_COEFF_PLUS_MINUS(x[0], iSide, AE[2][0], extA[0]);
            BT_GET_COEFF_MINUS_PLUS(x[2], iSide, AE[0][0], extA[2]);
            D += (fIntrTime * W);
            DxE[0] = D.Cross(E[0]);
            NAE[0][0] = A1xE0.Dot(A0xE0);
            NAE[1][0] = A1xE0.Dot(A1xE0);
            NAE[2][0] = A1xE0.Dot(A2xE0);
            NDE[0] = A1xE0.Dot(DxE[0]);
            fNumer = NDE[0] - (NAE[0][0] * x[0]) - (NAE[2][0] * x[2]);
            if (uiExtr == 0) {
                x[1] = fNumer / NAE[1][0];
            }
            else {
                x[1] = (fNumer - A1xE0.Dot(N)) / NAE[1][0];
            }
            BT_GET_POINT(kIntrPt, kBBV, fIntrTime, kV0, x);
            break;
        }
        case 9: {
            BT_GET_COEFF_PLUS_MINUS(x[0], iSide, AE[2][1], extA[0]);
            BT_GET_COEFF_MINUS_PLUS(x[2], iSide, AE[0][1], extA[2]);
            D += (fIntrTime * W);
            DxE[1] = D.Cross(E[1]);
            NAE[0][1] = A1xE1.Dot(A0xE1);
            NAE[1][1] = A1xE1.Dot(A1xE1);
            NAE[2][1] = A1xE1.Dot(A2xE1);
            NDE[1] = A1xE1.Dot(DxE[1]);
            fNumer = NDE[1] - (NAE[0][1] * x[0]) - (NAE[2][1] * x[2]);
            if (uiExtr == 0) {
                x[1] = fNumer / NAE[1][1];
            }
            else {
                x[1] = (fNumer + A1xE1.Dot(N)) / NAE[1][1];
            }
            BT_GET_POINT(kIntrPt, kBBV, fIntrTime, kV0, x);
            break;
        }
        case 10: {
            // EXE-INLINED 0x439EF0
            E[2].x = E[1].x - E[0].x;
            E[2].y = E[1].y - E[0].y;
            E[2].z = E[1].z - E[0].z;
            A0xE2 = A[0].Cross(E[2]);
            A1xE2 = A[1].Cross(E[2]);
            A2xE2 = A[2].Cross(E[2]);

            BT_GET_COEFF_PLUS_MINUS(x[0], iSide, AE[2][2], extA[0]);
            BT_GET_COEFF_MINUS_PLUS(x[2], iSide, AE[0][2], extA[2]);
            D += (fIntrTime * W);
            DxE[2] = D.Cross(E[2]);
            NAE[0][2] = A1xE2.Dot(A0xE2);
            NAE[1][2] = A1xE2.Dot(A1xE2);
            NAE[2][2] = A1xE2.Dot(A2xE2);
            NDE[2] = A1xE2.Dot(DxE[2]);
            fNumer = NDE[2] - (NAE[0][2] * x[0]) - (NAE[2][2] * x[2]);
            if (uiExtr == 0) {
                x[1] = fNumer / NAE[1][2];
            }
            else {
                x[1] = (fNumer + A1xE2.Dot(N)) / NAE[1][2];
            }
            BT_GET_POINT(kIntrPt, kBBV, fIntrTime, kV0, x);
            break;
        }
        case 11: {
            BT_GET_COEFF_MINUS_PLUS(x[0], iSide, AE[1][0], extA[0]);
            BT_GET_COEFF_PLUS_MINUS(x[1], iSide, AE[0][0], extA[1]);
            D += (fIntrTime * W);
            DxE[0] = D.Cross(E[0]);
            NAE[0][0] = A2xE0.Dot(A0xE0);
            NAE[1][0] = A2xE0.Dot(A1xE0);
            NAE[2][0] = A2xE0.Dot(A2xE0);
            NDE[0] = A2xE0.Dot(DxE[0]);
            fNumer = NDE[0] - (NAE[0][0] * x[0]) - (NAE[1][0] * x[1]);
            if (uiExtr == 0) {
                x[2] = fNumer / NAE[2][0];
            }
            else {
                x[2] = (fNumer - A2xE0.Dot(N)) / NAE[2][0];
            }
            BT_GET_POINT(kIntrPt, kBBV, fIntrTime, kV0, x);
            break;
        }
        case 12: {
            BT_GET_COEFF_MINUS_PLUS(x[0], iSide, AE[1][1], extA[0]);
            BT_GET_COEFF_PLUS_MINUS(x[1], iSide, AE[0][1], extA[1]);
            D += (fIntrTime * W);
            DxE[1] = D.Cross(E[1]);
            NAE[0][1] = A2xE1.Dot(A0xE1);
            NAE[1][1] = A2xE1.Dot(A1xE1);
            NAE[2][1] = A2xE1.Dot(A2xE1);
            NDE[1] = A2xE1.Dot(DxE[1]);
            fNumer = NDE[1] - (NAE[0][1] * x[0]) - (NAE[1][1] * x[1]);
            if (uiExtr == 0) {
                x[2] = fNumer / NAE[2][1];
            }
            else {
                x[2] = (fNumer + A2xE1.Dot(N)) / NAE[2][1];
            }
            BT_GET_POINT(kIntrPt, kBBV, fIntrTime, kV0, x);
            break;
        }
        case 13: {
            // EXE-INLINED 0x439EF0
            E[2].x = E[1].x - E[0].x;
            E[2].y = E[1].y - E[0].y;
            E[2].z = E[1].z - E[0].z;
            A0xE2 = A[0].Cross(E[2]);
            A1xE2 = A[1].Cross(E[2]);
            A2xE2 = A[2].Cross(E[2]);

            BT_GET_COEFF_MINUS_PLUS(x[0], iSide, AE[1][2], extA[0]);
            BT_GET_COEFF_PLUS_MINUS(x[1], iSide, AE[0][2], extA[1]);
            D += (fIntrTime * W);
            DxE[2] = D.Cross(E[2]);
            NAE[0][2] = A2xE2.Dot(A0xE2);
            NAE[1][2] = A2xE2.Dot(A1xE2);
            NAE[2][2] = A2xE2.Dot(A2xE2);
            NDE[2] = A2xE2.Dot(DxE[2]);
            fNumer = NDE[2] - (NAE[0][2] * x[0]) - (NAE[1][2] * x[1]);
            if (uiExtr == 0) {
                x[2] = fNumer / NAE[2][2];
            }
            else {
                x[2] = (fNumer + A2xE2.Dot(N)) / NAE[2][2];
            }
            BT_GET_POINT(kIntrPt, kBBV, fIntrTime, kV0, x);
            break;
        }
    }

    if (bCalcNormals) {
        kNormal1 = N;
        kNormal1.Unitize();
        // EXE-INLINED 0x4A0BD0
        NiPoint3 kNegated;
        kNegated.x = -kNormal1.x;
        kNegated.y = -kNormal1.y;
        kNegated.z = -kNormal1.z;
        kNormal0 = kNegated;
    }

    return true;
}

// The sweep of box kBV0 moving by kV0 against box kBV1 moving by kV1 over fTime: the fifteen axes of
// BoxBoxTestIntersect, each giving the time the boxes first touch along it; the latest is fIntrTime, and its axis
// (uiType) and side (iSide) place kIntrPt and the normals.
bool NiBoxBV::BoxBoxFindIntersect(
    float fTime,
    const NiBoundingVolume& kBV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kBV1,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    bool bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    const NiBoxBV& kBBV0 = static_cast<const NiBoxBV&>(kBV0);
    const NiBoxBV& kBBV1 = static_cast<const NiBoxBV&>(kBV1);
    const NiPoint3* A = kBBV0.m_kBox.m_akAxis;
    const NiPoint3* B = kBBV1.m_kBox.m_akAxis;
    const float* extA = kBBV0.m_kBox.m_afExtent;
    const float* extB = kBBV1.m_kBox.m_afExtent;

    float AB[3][3];
    float fAB[3][3];
    float AD0[3];
    float AD1[3];
    float BD0[3];
    float BD1[3];
    float AR1[3];
    float BR0[3];
    float R0;
    float R1;
    float R;
    float Rsum;

    // EXE-INLINED 0x439EF0
    NiPoint3 W;
    W.x = kV1.x - kV0.x;
    W.y = kV1.y - kV0.y;
    W.z = kV1.z - kV0.z;
    // EXE-INLINED 0x439EF0
    NiPoint3 D0;
    D0.x = kBBV1.m_kBox.m_kCenter.x - kBBV0.m_kBox.m_kCenter.x;
    D0.y = kBBV1.m_kBox.m_kCenter.y - kBBV0.m_kBox.m_kCenter.y;
    D0.z = kBBV1.m_kBox.m_kCenter.z - kBBV0.m_kBox.m_kCenter.z;
    // D1 = D0 + fTime * W: the product is rounded to float before the sum.
    // EXE-INLINED 0x4A3760
    // EXE-INLINED 0x439E90
    NiPoint3 D1;
    D1.x = D0.x + (fTime * W.x);
    D1.y = D0.y + (fTime * W.y);
    D1.z = D0.z + (fTime * W.z);

    uint32_t uiType = 0;
    int32_t iSide = 0;
    fIntrTime = 0.0f;

    // The axes A[0], A[1], A[2].
    for (int32_t k = 0; k < 3; k++) {
        // EXE-INLINED 0x4B6190
        AB[k][0] = A[k].x * B[0].x + A[k].y * B[0].y + A[k].z * B[0].z;
        AB[k][1] = A[k].x * B[1].x + A[k].y * B[1].y + A[k].z * B[1].z;
        AB[k][2] = A[k].x * B[2].x + A[k].y * B[2].y + A[k].z * B[2].z;
        AD0[k] = A[k].x * D0.x + A[k].y * D0.y + A[k].z * D0.z;
        AD1[k] = A[k].x * D1.x + A[k].y * D1.y + A[k].z * D1.z;
        // EXE-INLINED 0x408820
        fAB[k][0] = __builtin_fabsf(AB[k][0]);
        fAB[k][1] = __builtin_fabsf(AB[k][1]);
        fAB[k][2] = __builtin_fabsf(AB[k][2]);
        AR1[k] = extB[0] * fAB[k][0] + extB[1] * fAB[k][1] + extB[2] * fAB[k][2];
        Rsum = extA[k] + AR1[k];
        BB_FIND0(AD0[k], AD1[k], Rsum, fTime, fIntrTime, uiType, iSide, k + 1);
    }

    // The axes B[0], B[1], B[2].
    for (int32_t k = 0; k < 3; k++) {
        // EXE-INLINED 0x4B6190
        BD0[k] = B[k].x * D0.x + B[k].y * D0.y + B[k].z * D0.z;
        BD1[k] = B[k].x * D1.x + B[k].y * D1.y + B[k].z * D1.z;
        BR0[k] = extA[0] * fAB[0][k] + extA[1] * fAB[1][k] + extA[2] * fAB[2][k];
        Rsum = BR0[k] + extB[k];
        BB_FIND0(BD0[k], BD1[k], Rsum, fTime, fIntrTime, uiType, iSide, k + 4);
    }

    // A[0] x B[0]
    R0 = extA[1] * fAB[2][0] + extA[2] * fAB[1][0];
    R1 = extB[1] * fAB[0][2] + extB[2] * fAB[0][1];
    R = AD0[2] * AB[1][0] - AD0[1] * AB[2][0];
    Rsum = R0 + R1;
    BB_FIND1(R, AD1[2], AB[1][0], AD1[1], AB[2][0], Rsum, fTime, fIntrTime, uiType, iSide, 7);

    // A[0] x B[1]
    R0 = extA[1] * fAB[2][1] + extA[2] * fAB[1][1];
    R1 = extB[0] * fAB[0][2] + extB[2] * fAB[0][0];
    R = AD0[2] * AB[1][1] - AD0[1] * AB[2][1];
    Rsum = R0 + R1;
    BB_FIND1(R, AD1[2], AB[1][1], AD1[1], AB[2][1], Rsum, fTime, fIntrTime, uiType, iSide, 8);

    // A[0] x B[2]
    R0 = extA[1] * fAB[2][2] + extA[2] * fAB[1][2];
    R1 = extB[0] * fAB[0][1] + extB[1] * fAB[0][0];
    R = AD0[2] * AB[1][2] - AD0[1] * AB[2][2];
    Rsum = R0 + R1;
    BB_FIND1(R, AD1[2], AB[1][2], AD1[1], AB[2][2], Rsum, fTime, fIntrTime, uiType, iSide, 9);

    // A[1] x B[0]
    R0 = extA[0] * fAB[2][0] + extA[2] * fAB[0][0];
    R1 = extB[1] * fAB[1][2] + extB[2] * fAB[1][1];
    R = AD0[0] * AB[2][0] - AD0[2] * AB[0][0];
    Rsum = R0 + R1;
    BB_FIND1(R, AD1[0], AB[2][0], AD1[2], AB[0][0], Rsum, fTime, fIntrTime, uiType, iSide, 10);

    // A[1] x B[1]
    R0 = extA[0] * fAB[2][1] + extA[2] * fAB[0][1];
    R1 = extB[0] * fAB[1][2] + extB[2] * fAB[1][0];
    R = AD0[0] * AB[2][1] - AD0[2] * AB[0][1];
    Rsum = R0 + R1;
    BB_FIND1(R, AD1[0], AB[2][1], AD1[2], AB[0][1], Rsum, fTime, fIntrTime, uiType, iSide, 11);

    // A[1] x B[2]
    R0 = extA[0] * fAB[2][2] + extA[2] * fAB[0][2];
    R1 = extB[0] * fAB[1][1] + extB[1] * fAB[1][0];
    R = AD0[0] * AB[2][2] - AD0[2] * AB[0][2];
    Rsum = R0 + R1;
    BB_FIND1(R, AD1[0], AB[2][2], AD1[2], AB[0][2], Rsum, fTime, fIntrTime, uiType, iSide, 12);

    // A[2] x B[0]
    R0 = extA[0] * fAB[1][0] + extA[1] * fAB[0][0];
    R1 = extB[1] * fAB[2][2] + extB[2] * fAB[2][1];
    R = AD0[1] * AB[0][0] - AD0[0] * AB[1][0];
    Rsum = R0 + R1;
    BB_FIND1(R, AD1[1], AB[0][0], AD1[0], AB[1][0], Rsum, fTime, fIntrTime, uiType, iSide, 13);

    // A[2] x B[1]
    R0 = extA[0] * fAB[1][1] + extA[1] * fAB[0][1];
    R1 = extB[0] * fAB[2][2] + extB[2] * fAB[2][0];
    R = AD0[1] * AB[0][1] - AD0[0] * AB[1][1];
    Rsum = R0 + R1;
    BB_FIND1(R, AD1[1], AB[0][1], AD1[0], AB[1][1], Rsum, fTime, fIntrTime, uiType, iSide, 14);

    // A[2] x B[2]
    R0 = extA[0] * fAB[1][2] + extA[1] * fAB[0][2];
    R1 = extB[0] * fAB[2][1] + extB[1] * fAB[2][0];
    R = AD0[1] * AB[0][2] - AD0[0] * AB[1][2];
    Rsum = R0 + R1;
    BB_FIND1(R, AD1[1], AB[0][2], AD1[0], AB[1][2], Rsum, fTime, fIntrTime, uiType, iSide, 15);

    int32_t i;
    int32_t j;
    float x[3];
    float y[3];
    float div;
    float tmp;
    NiPoint3 D;
    NiPoint3 kTW;
    NiPoint3 kCross;

    switch (uiType) {
        case 0: {
            // The boxes overlap at the start: the point halfway between the centres.
            kIntrPt = 0.5f * (kBBV0.m_kBox.m_kCenter + kBBV1.m_kBox.m_kCenter);
            if (bCalcNormals) {
                kNormal0 = kIntrPt - kBBV0.m_kBox.m_kCenter;
            }
            break;
        }
        case 1:
        case 2:
        case 3: {
            i = uiType - 1;
            BB_GET_COEFF_MINUS_PLUS(y[0], iSide, AB[i][0], extB[0]);
            BB_GET_COEFF_MINUS_PLUS(y[1], iSide, AB[i][1], extB[1]);
            BB_GET_COEFF_MINUS_PLUS(y[2], iSide, AB[i][2], extB[2]);
            BB_GET_POINT(kIntrPt, kBBV1, fIntrTime, kV1, y);
            RefinePoint(fTime, &kBBV1, &kV1, &kBBV0, &kV0, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -A[i].x;
                    kNormal0.y = -A[i].y;
                    kNormal0.z = -A[i].z;
                }
                else {
                    kNormal0 = A[i];
                }
                // EXE-INLINED 0x4A0BD0
                kNormal1.x = -kNormal0.x;
                kNormal1.y = -kNormal0.y;
                kNormal1.z = -kNormal0.z;
            }
            return true;
        }
        case 4:
        case 5:
        case 6: {
            j = uiType - 4;
            BB_GET_COEFF_PLUS_MINUS(x[0], iSide, AB[0][j], extA[0]);
            BB_GET_COEFF_PLUS_MINUS(x[1], iSide, AB[1][j], extA[1]);
            BB_GET_COEFF_PLUS_MINUS(x[2], iSide, AB[2][j], extA[2]);
            BB_GET_POINT(kIntrPt, kBBV0, fIntrTime, kV0, x);
            RefinePoint(fTime, &kBBV0, &kV0, &kBBV1, &kV1, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -B[j].x;
                    kNormal0.y = -B[j].y;
                    kNormal0.z = -B[j].z;
                }
                else {
                    kNormal0 = B[j];
                }
                // EXE-INLINED 0x4A0BD0
                kNormal1.x = -kNormal0.x;
                kNormal1.y = -kNormal0.y;
                kNormal1.z = -kNormal0.z;
            }
            return true;
        }
        case 7: {
            BB_GET_COEFF_MINUS_PLUS(x[1], iSide, AB[2][0], extA[1]);
            BB_GET_COEFF_PLUS_MINUS(x[2], iSide, AB[1][0], extA[2]);
            BB_GET_COEFF_MINUS_PLUS(y[1], iSide, AB[0][2], extB[1]);
            BB_GET_COEFF_PLUS_MINUS(y[2], iSide, AB[0][1], extB[2]);
            div = 1.0f - AB[0][0] * AB[0][0];
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(div) > 1e-06f) {
                kTW = fIntrTime * W;
                // EXE-INLINED 0x439E90
                D.x = D0.x + kTW.x;
                D.y = D0.y + kTW.y;
                D.z = D0.z + kTW.z;
                tmp = AB[1][0] * x[1] + AB[2][0] * x[2] - B[0].Dot(D);
                x[0] = (A[0].Dot(D) + AB[0][0] * tmp + AB[0][1] * y[1] + AB[0][2] * y[2]) / div;
            }
            else {
                x[0] = 0.0f;
            }
            BB_GET_POINT(kIntrPt, kBBV0, fIntrTime, kV0, x);
            RefinePoint(fTime, &kBBV0, &kV0, &kBBV1, &kV1, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    kCross = A[0].Cross(B[0]);
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -kCross.x;
                    kNormal0.y = -kCross.y;
                    kNormal0.z = -kCross.z;
                }
                else {
                    kNormal0 = A[0].Cross(B[0]);
                }
            }
            break;
        }
        case 8: {
            BB_GET_COEFF_MINUS_PLUS(x[1], iSide, AB[2][1], extA[1]);
            BB_GET_COEFF_PLUS_MINUS(x[2], iSide, AB[1][1], extA[2]);
            BB_GET_COEFF_PLUS_MINUS(y[0], iSide, AB[0][2], extB[0]);
            BB_GET_COEFF_MINUS_PLUS(y[2], iSide, AB[0][0], extB[2]);
            div = 1.0f - AB[0][1] * AB[0][1];
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(div) > 1e-06f) {
                kTW = fIntrTime * W;
                // EXE-INLINED 0x439E90
                D.x = D0.x + kTW.x;
                D.y = D0.y + kTW.y;
                D.z = D0.z + kTW.z;
                tmp = AB[1][1] * x[1] + AB[2][1] * x[2] - B[1].Dot(D);
                x[0] = (A[0].Dot(D) + AB[0][1] * tmp + AB[0][0] * y[0] + AB[0][2] * y[2]) / div;
            }
            else {
                x[0] = 0.0f;
            }
            BB_GET_POINT(kIntrPt, kBBV0, fIntrTime, kV0, x);
            RefinePoint(fTime, &kBBV0, &kV0, &kBBV1, &kV1, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    kCross = A[0].Cross(B[1]);
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -kCross.x;
                    kNormal0.y = -kCross.y;
                    kNormal0.z = -kCross.z;
                }
                else {
                    kNormal0 = A[0].Cross(B[1]);
                }
            }
            break;
        }
        case 9: {
            BB_GET_COEFF_MINUS_PLUS(x[1], iSide, AB[2][2], extA[1]);
            BB_GET_COEFF_PLUS_MINUS(x[2], iSide, AB[1][2], extA[2]);
            BB_GET_COEFF_MINUS_PLUS(y[0], iSide, AB[0][1], extB[0]);
            BB_GET_COEFF_PLUS_MINUS(y[1], iSide, AB[0][0], extB[1]);
            div = 1.0f - AB[0][2] * AB[0][2];
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(div) > 1e-06f) {
                kTW = fIntrTime * W;
                // EXE-INLINED 0x439E90
                D.x = D0.x + kTW.x;
                D.y = D0.y + kTW.y;
                D.z = D0.z + kTW.z;
                tmp = AB[1][2] * x[1] + AB[2][2] * x[2] - B[2].Dot(D);
                x[0] = (A[0].Dot(D) + AB[0][2] * tmp + AB[0][0] * y[0] + AB[0][1] * y[1]) / div;
            }
            else {
                x[0] = 0.0f;
            }
            BB_GET_POINT(kIntrPt, kBBV0, fIntrTime, kV0, x);
            RefinePoint(fTime, &kBBV0, &kV0, &kBBV1, &kV1, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    kCross = A[0].Cross(B[2]);
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -kCross.x;
                    kNormal0.y = -kCross.y;
                    kNormal0.z = -kCross.z;
                }
                else {
                    kNormal0 = A[0].Cross(B[2]);
                }
            }
            break;
        }
        case 10: {
            BB_GET_COEFF_PLUS_MINUS(x[0], iSide, AB[2][0], extA[0]);
            BB_GET_COEFF_MINUS_PLUS(x[2], iSide, AB[0][0], extA[2]);
            BB_GET_COEFF_MINUS_PLUS(y[1], iSide, AB[1][2], extB[1]);
            BB_GET_COEFF_PLUS_MINUS(y[2], iSide, AB[1][1], extB[2]);
            div = 1.0f - AB[1][0] * AB[1][0];
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(div) > 1e-06f) {
                kTW = fIntrTime * W;
                // EXE-INLINED 0x439E90
                D.x = D0.x + kTW.x;
                D.y = D0.y + kTW.y;
                D.z = D0.z + kTW.z;
                tmp = AB[0][0] * x[0] + AB[2][0] * x[2] - B[0].Dot(D);
                x[1] = (A[1].Dot(D) + AB[1][0] * tmp + AB[1][1] * y[1] + AB[1][2] * y[2]) / div;
            }
            else {
                x[1] = 0.0f;
            }
            BB_GET_POINT(kIntrPt, kBBV0, fIntrTime, kV0, x);
            RefinePoint(fTime, &kBBV0, &kV0, &kBBV1, &kV1, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    kCross = A[1].Cross(B[0]);
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -kCross.x;
                    kNormal0.y = -kCross.y;
                    kNormal0.z = -kCross.z;
                }
                else {
                    kNormal0 = A[1].Cross(B[0]);
                }
            }
            break;
        }
        case 11: {
            BB_GET_COEFF_PLUS_MINUS(x[0], iSide, AB[2][1], extA[0]);
            BB_GET_COEFF_MINUS_PLUS(x[2], iSide, AB[0][1], extA[2]);
            BB_GET_COEFF_PLUS_MINUS(y[0], iSide, AB[1][2], extB[0]);
            BB_GET_COEFF_MINUS_PLUS(y[2], iSide, AB[1][0], extB[2]);
            div = 1.0f - AB[1][1] * AB[1][1];
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(div) > 1e-06f) {
                kTW = fIntrTime * W;
                // EXE-INLINED 0x439E90
                D.x = D0.x + kTW.x;
                D.y = D0.y + kTW.y;
                D.z = D0.z + kTW.z;
                tmp = AB[0][1] * x[0] + AB[2][1] * x[2] - B[1].Dot(D);
                x[1] = (A[1].Dot(D) + AB[1][1] * tmp + AB[1][0] * y[0] + AB[1][2] * y[2]) / div;
            }
            else {
                x[1] = 0.0f;
            }
            BB_GET_POINT(kIntrPt, kBBV0, fIntrTime, kV0, x);
            RefinePoint(fTime, &kBBV0, &kV0, &kBBV1, &kV1, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    kCross = A[1].Cross(B[1]);
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -kCross.x;
                    kNormal0.y = -kCross.y;
                    kNormal0.z = -kCross.z;
                }
                else {
                    kNormal0 = A[1].Cross(B[1]);
                }
            }
            break;
        }
        case 12: {
            BB_GET_COEFF_PLUS_MINUS(x[0], iSide, AB[2][2], extA[0]);
            BB_GET_COEFF_MINUS_PLUS(x[2], iSide, AB[0][2], extA[2]);
            BB_GET_COEFF_MINUS_PLUS(y[0], iSide, AB[1][1], extB[0]);
            BB_GET_COEFF_PLUS_MINUS(y[1], iSide, AB[1][0], extB[1]);
            div = 1.0f - AB[1][2] * AB[1][2];
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(div) > 1e-06f) {
                kTW = fIntrTime * W;
                // EXE-INLINED 0x439E90
                D.x = D0.x + kTW.x;
                D.y = D0.y + kTW.y;
                D.z = D0.z + kTW.z;
                tmp = AB[0][2] * x[0] + AB[2][2] * x[2] - B[2].Dot(D);
                x[1] = (A[1].Dot(D) + AB[1][2] * tmp + AB[1][0] * y[0] + AB[1][1] * y[1]) / div;
            }
            else {
                x[1] = 0.0f;
            }
            BB_GET_POINT(kIntrPt, kBBV0, fIntrTime, kV0, x);
            RefinePoint(fTime, &kBBV0, &kV0, &kBBV1, &kV1, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    kCross = A[1].Cross(B[2]);
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -kCross.x;
                    kNormal0.y = -kCross.y;
                    kNormal0.z = -kCross.z;
                }
                else {
                    kNormal0 = A[1].Cross(B[2]);
                }
            }
            break;
        }
        case 13: {
            BB_GET_COEFF_MINUS_PLUS(x[0], iSide, AB[1][0], extA[0]);
            BB_GET_COEFF_PLUS_MINUS(x[1], iSide, AB[0][0], extA[1]);
            BB_GET_COEFF_MINUS_PLUS(y[1], iSide, AB[2][2], extB[1]);
            BB_GET_COEFF_PLUS_MINUS(y[2], iSide, AB[2][1], extB[2]);
            div = 1.0f - AB[2][0] * AB[2][0];
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(div) > 1e-06f) {
                kTW = fIntrTime * W;
                // EXE-INLINED 0x439E90
                D.x = D0.x + kTW.x;
                D.y = D0.y + kTW.y;
                D.z = D0.z + kTW.z;
                tmp = AB[0][0] * x[0] + AB[1][0] * x[1] - B[0].Dot(D);
                x[2] = (A[2].Dot(D) + AB[2][0] * tmp + AB[2][1] * y[1] + AB[2][2] * y[2]) / div;
            }
            else {
                x[2] = 0.0f;
            }
            BB_GET_POINT(kIntrPt, kBBV0, fIntrTime, kV0, x);
            RefinePoint(fTime, &kBBV0, &kV0, &kBBV1, &kV1, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    kCross = A[2].Cross(B[0]);
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -kCross.x;
                    kNormal0.y = -kCross.y;
                    kNormal0.z = -kCross.z;
                }
                else {
                    kNormal0 = A[2].Cross(B[0]);
                }
            }
            break;
        }
        case 14: {
            BB_GET_COEFF_MINUS_PLUS(x[0], iSide, AB[1][1], extA[0]);
            BB_GET_COEFF_PLUS_MINUS(x[1], iSide, AB[0][1], extA[1]);
            BB_GET_COEFF_PLUS_MINUS(y[0], iSide, AB[2][2], extB[0]);
            BB_GET_COEFF_MINUS_PLUS(y[2], iSide, AB[2][0], extB[2]);
            div = 1.0f - AB[2][1] * AB[2][1];
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(div) > 1e-06f) {
                kTW = fIntrTime * W;
                // EXE-INLINED 0x439E90
                D.x = D0.x + kTW.x;
                D.y = D0.y + kTW.y;
                D.z = D0.z + kTW.z;
                tmp = AB[0][1] * x[0] + AB[1][1] * x[1] - B[1].Dot(D);
                x[2] = (A[2].Dot(D) + AB[2][1] * tmp + AB[2][0] * y[0] + AB[2][2] * y[2]) / div;
            }
            else {
                x[2] = 0.0f;
            }
            BB_GET_POINT(kIntrPt, kBBV0, fIntrTime, kV0, x);
            RefinePoint(fTime, &kBBV0, &kV0, &kBBV1, &kV1, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    kCross = A[2].Cross(B[1]);
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -kCross.x;
                    kNormal0.y = -kCross.y;
                    kNormal0.z = -kCross.z;
                }
                else {
                    kNormal0 = A[2].Cross(B[1]);
                }
            }
            break;
        }
        case 15: {
            BB_GET_COEFF_MINUS_PLUS(x[0], iSide, AB[1][2], extA[0]);
            BB_GET_COEFF_PLUS_MINUS(x[1], iSide, AB[0][2], extA[1]);
            BB_GET_COEFF_MINUS_PLUS(y[0], iSide, AB[2][1], extB[0]);
            BB_GET_COEFF_PLUS_MINUS(y[1], iSide, AB[2][0], extB[1]);
            div = 1.0f - AB[2][2] * AB[2][2];
            // EXE-INLINED 0x408820
            if (__builtin_fabsf(div) > 1e-06f) {
                kTW = fIntrTime * W;
                // EXE-INLINED 0x439E90
                D.x = D0.x + kTW.x;
                D.y = D0.y + kTW.y;
                D.z = D0.z + kTW.z;
                tmp = AB[0][2] * x[0] + AB[1][2] * x[1] - B[2].Dot(D);
                x[2] = (A[2].Dot(D) + AB[2][2] * tmp + AB[2][0] * y[0] + AB[2][1] * y[1]) / div;
            }
            else {
                x[2] = 0.0f;
            }
            BB_GET_POINT(kIntrPt, kBBV0, fIntrTime, kV0, x);
            RefinePoint(fTime, &kBBV0, &kV0, &kBBV1, &kV1, fIntrTime, kIntrPt);
            if (bCalcNormals) {
                if (iSide < 0) {
                    kCross = A[2].Cross(B[2]);
                    // EXE-INLINED 0x4A0BD0
                    kNormal0.x = -kCross.x;
                    kNormal0.y = -kCross.y;
                    kNormal0.z = -kCross.z;
                }
                else {
                    kNormal0 = A[2].Cross(B[2]);
                }
            }
            break;
        }
    }

    if (bCalcNormals) {
        // Make the normal unit length (UNIT_X when it is too short to unitize).
        const float fNormalSqrLength = kNormal0.SqrLength();
        // EXE-INLINED 0x408820
        if (__builtin_fabsf(fNormalSqrLength - 1.0) > 1.1920929e-07f) {
            if (fNormalSqrLength > 1.1920929e-07f) {
                kNormal0.Unitize();
            }
            else {
                kNormal0 = NiPoint3_UNIT_X;
            }
        }
        // EXE-INLINED 0x4A0BD0
        kNormal1.x = -kNormal0.x;
        kNormal1.y = -kNormal0.y;
        kNormal1.z = -kNormal0.z;
    }

    return true;
}

// For each box in turn, unless a sphere of radius 0.01 at kIntrPt0 already meets it, projects kIntrPt0 onto the box's
// axes from its center at fIntrTime, snaps each projection within 1% of an extent to that extent, and rebuilds the
// point from the projections. Every intermediate is stored as a float, as the float locals here do.
void NiBoxBV::RefinePoint(
    float fTime,
    const NiBoxBV* pkBBV0,
    const NiPoint3* pkV0,
    const NiBoxBV* pkBBV1,
    const NiPoint3* pkV1,
    float& fIntrTime,
    NiPoint3& kIntrPt0
) {
    // The sphere is constructed and never destroyed, as the game does.
    NiSphereBV kSBV(1.0f, NiPoint3_ZERO);
    kSBV.m_kSphere.m_fRadius = 0.01f;
    kSBV.m_kSphere.m_kCenter.x = kIntrPt0.x;
    kSBV.m_kSphere.m_kCenter.y = kIntrPt0.y;
    kSBV.m_kSphere.m_kCenter.z = kIntrPt0.z;
    const double fClampPoint = 0.9900000095367432;

    // The source writes the same steps out twice, for pkBBV0 and then pkBBV1.
    const NiBoxBV* apkBBV[2] = {pkBBV0, pkBBV1};
    const NiPoint3* apkV[2] = {pkV0, pkV1};
    for (int32_t iBox = 0; iBox < 2; ++iBox) {
        if (BoxSphereTestIntersect(fTime, *apkBBV[iBox], *apkV[iBox], kSBV, NiPoint3_ZERO)) {
            continue;
        }
        const NiBox& kBox = apkBBV[iBox]->m_kBox;
        const NiPoint3& kV = *apkV[iBox];

        // kNewCenter = kBox.m_kCenter + fIntrTime * kV
        // EXE-INLINED 0x4A3760
        const float fMoveX = fIntrTime * kV.x;
        const float fMoveY = fIntrTime * kV.y;
        const float fMoveZ = fIntrTime * kV.z;
        // EXE-INLINED 0x439E90
        const float fNewCenterX = kBox.m_kCenter.x + fMoveX;
        const float fNewCenterY = kBox.m_kCenter.y + fMoveY;
        const float fNewCenterZ = kBox.m_kCenter.z + fMoveZ;

        // kFromCenter = kIntrPt0 - kNewCenter
        // EXE-INLINED 0x439EF0
        const float fFromCenterX = kIntrPt0.x - fNewCenterX;
        const float fFromCenterY = kIntrPt0.y - fNewCenterY;
        const float fFromCenterZ = kIntrPt0.z - fNewCenterZ;

        // EXE-INLINED 0x4B6190
        float afNrmlAxisProj[3];
        for (int32_t i = 0; i < 3; ++i) {
            afNrmlAxisProj[i] = fFromCenterX * kBox.m_akAxis[i].x + fFromCenterY * kBox.m_akAxis[i].y +
                fFromCenterZ * kBox.m_akAxis[i].z;
        }

        for (int32_t i = 0; i < 3; ++i) {
            if (afNrmlAxisProj[i] > kBox.m_afExtent[i] * fClampPoint) {
                afNrmlAxisProj[i] = kBox.m_afExtent[i];
            }
            else if (afNrmlAxisProj[i] < -kBox.m_afExtent[i] * fClampPoint) {
                afNrmlAxisProj[i] = -kBox.m_afExtent[i];
            }
        }

        // kIntrPt0 = kNewCenter + afNrmlAxisProj[0] * axis 0 + afNrmlAxisProj[1] * axis 1 + afNrmlAxisProj[2] * axis 2,
        // summed left to right, each product and sum a float.
        // EXE-INLINED 0x4A3760
        // EXE-INLINED 0x439E90
        float fX = fNewCenterX;
        float fY = fNewCenterY;
        float fZ = fNewCenterZ;
        for (int32_t i = 0; i < 3; ++i) {
            const float fTermX = afNrmlAxisProj[i] * kBox.m_akAxis[i].x;
            const float fTermY = afNrmlAxisProj[i] * kBox.m_akAxis[i].y;
            const float fTermZ = afNrmlAxisProj[i] * kBox.m_akAxis[i].z;
            fX = fX + fTermX;
            fY = fY + fTermY;
            fZ = fZ + fTermZ;
        }
        kIntrPt0.x = fX;
        kIntrPt0.y = fY;
        kIntrPt0.z = fZ;
    }
}
