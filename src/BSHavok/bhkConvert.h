#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"

class hkQsTransform;
class NiMatrix3;

// The length scales between game units and Havok's. Each is a static of every file that includes the header, as in the
// original.
namespace bhkConvert {

    static const float fHkScaleSC = 10.0f;
    // An 8-byte constant divided by a 4-byte 10.0.
    static const float fHk2BSScaleSC = 69.99125671386719 / 10.0f;
    // The file's own fHk2BSScaleSC inverted.
    static const float fBS2HkScaleSC = 1.0f / fHk2BSScaleSC;

} // namespace bhkConvert

// Converts the Havok vector arhkV to game units into arNiP and returns it.
NiPoint3& HK2NI(NiPoint3& arNiP, const hkVector4& arhkV);
// Converts the game-unit point arNiP to Havok units into arhkV and returns it (the body is in
// BSHavok/bhkRagdollController.cpp).
hkVector4& NI2HK(hkVector4& arhkV, const NiPoint3& arNiP);
// Converts the Havok rotation arhkRot into arNiRot and returns it.
NiMatrix3& HK2NI(NiMatrix3& arNiRot, const hkRotation& arhkRot);
// Converts the game transform arNiTran to Havok units into arhkTran and returns it.
hkTransform& NI2HK(hkTransform& arhkTran, const NiTransform& arNiTran);
// Converts the game rotation arNiRot to Havok units into arhkRot and returns it.
hkRotation& NI2HK(hkRotation& arhkRot, const NiMatrix3& arNiRot);
// Converts the game transform arNiQ to Havok units into arhkQ and returns it.
hkQsTransform& NI2HK(hkQsTransform& arhkQ, const NiTransform& arNiQ);
// Copies the game quaternion arNiQ into arhkQ, component by component, and returns arhkQ.
inline hkQuaternion& NI2HK(hkQuaternion& arhkQ, const NiQuaternion& arNiQ) {
    arhkQ.m_vec(0) = arNiQ.m_fX;
    arhkQ.m_vec(1) = arNiQ.m_fY;
    arhkQ.m_vec(2) = arNiQ.m_fZ;
    arhkQ.m_vec(3) = arNiQ.m_fW;
    return arhkQ;
}
// Converts the game rotation arNiQ to a Havok quaternion into arhkQ and returns it.
inline hkQuaternion& NI2HKMIGRATION_QUATERNION(hkQuaternion& arhkQ, const NiQuaternion& arNiQ) {
    float fX = arNiQ.m_fX;
    arhkQ.m_vec(0) = fX;
    float fY = arNiQ.m_fY;
    arhkQ.m_vec(1) = fY;
    float fZ = arNiQ.m_fZ;
    arhkQ.m_vec(2) = fZ;
    float fW = arNiQ.m_fW;
    arhkQ.m_vec(3) = fW;
    return arhkQ;
}

// Converts the Havok vector arhkV to game units into arNiP without the world scale and returns it.
inline NiPoint3& HK2NINS(NiPoint3& arNiP, const hkVector4& arhkV) {
    arNiP.x = arhkV.getSimdAt(0);
    arNiP.y = arhkV.getSimdAt(1);
    arNiP.z = arhkV.getSimdAt(2);
    return arNiP;
}

// Converts the game-unit length afValue to Havok units.
inline float NI2HK(float afValue) {
    return bhkConvert::fBS2HkScaleSC * afValue;
}

// Converts the Havok length afhk to game units.
inline float HK2NI(float afhk) {
    float fResult = bhkConvert::fHk2BSScaleSC * afhk;
    return fResult;
}

// Converts the Havok transform arhkTran into arNiTran and returns it; the scale is left as it
// was.
inline NiTransform& HK2NI(NiTransform& arNiTran, const hkTransform& arhkTran) {
    HK2NI(arNiTran.m_Rotate, arhkTran.m_rotation);
    HK2NI(arNiTran.m_Translate, arhkTran.getTranslation());
    return arNiTran;
}

// Copies the point arNiP into arhkV without the Havok unit scale and returns arhkV.
inline hkVector4& NI2HKNS(hkVector4& arhkV, const NiPoint3& arNiP) {
    arhkV(0) = arNiP.x;
    arhkV(1) = arNiP.y;
    arhkV(2) = arNiP.z;
    arhkV(3) = 0.0;
    return arhkV;
}

// Bethesda's own four-float vector in Havok's layout: the columns of bhkMatrix3 and the translation of bhkTransform.
// Unlike hkVector4, whose default constructor stores nothing, it starts out as the zero vector. Kept on Havok's
// 16-byte alignment so the classes built from it stay laid out like their hk counterparts.
class alignas(16) bhkVector4 {
public:
    bhkVector4();

    void set(float afX, float afY, float afZ, float afW);
    void setZero4();
    // x, y and z set to afValue, w kept.
    void setAll3(float afVal);
    // Passes the four components to arDst's set (Havok's own) and returns arDst.
    hkVector4& ToHK(hkVector4& arDst) const;
    // Copies arSrc's x, y and z, with w 0 (no scaling to Havok units), and returns this.
    bhkVector4& NI2HKNS(const NiPoint3& arSrc);
    // Copies arSrc's x, y and z scaled to Havok units, with w 0, and returns this.
    bhkVector4& NI2HK(const NiPoint3& arSrc);
    // Copies arSrc's four components and returns this.
    bhkVector4& operator=(const hkVector4& arSrc);

    float fX;
    float fY;
    float fZ;
    float fW;
};

inline bhkVector4::bhkVector4() {
    setZero4();
}

inline void bhkVector4::set(float afX, float afY, float afZ, float afW) {
    fX = afX;
    fY = afY;
    fZ = afZ;
    fW = afW;
}

inline void bhkVector4::setZero4() {
    set(0.0f, 0.0f, 0.0f, 0.0f);
}

inline void bhkVector4::setAll3(float afVal) {
    fX = afVal;
    fY = afVal;
    fZ = afVal;
}

inline hkVector4& bhkVector4::ToHK(hkVector4& arDst) const {
    arDst.set(fX, fY, fZ, fW);
    return arDst;
}

inline bhkVector4& bhkVector4::NI2HK(const NiPoint3& arSrc) {
    fX = bhkConvert::fBS2HkScaleSC * arSrc.x;
    fY = bhkConvert::fBS2HkScaleSC * arSrc.y;
    fZ = bhkConvert::fBS2HkScaleSC * arSrc.z;
    fW = 0.0;
    return *this;
}

inline bhkVector4& bhkVector4::operator=(const hkVector4& arSrc) {
    fX = arSrc.getSimdAt(0);
    fY = arSrc.getSimdAt(1);
    fZ = arSrc.getSimdAt(2);
    fW = arSrc.getSimdAt(3);
    return *this;
}

inline bhkVector4& bhkVector4::NI2HKNS(const NiPoint3& arSrc) {
    fX = arSrc.x;
    fY = arSrc.y;
    fZ = arSrc.z;
    fW = 0.0f;
    return *this;
}

// The 3x3 matrix a bhkTransform holds; its columns are bhkVector4s, so a new matrix is all zeros. Like Havok's
// hkMatrix3 the class has no base and no vtable.
class bhkMatrix3 {
public:
    bhkMatrix3();

    // Sets the columns from the NetImmerse matrix apMatrix's columns.
    bhkMatrix3& NI2HKNS(const NiMatrix3& arSrc);

    // Clears all three columns and puts ones on the diagonal.
    void setIdentity();

    // Copies the three columns into arDst's and returns arDst.
    hkRotation& ToHK(hkRotation& arDst) const {
        Col0.ToHK(arDst.m_col0);
        Col1.ToHK(arDst.m_col1);
        Col2.ToHK(arDst.m_col2);
        return arDst;
    }

    void Renormalize();

    bhkVector4 Col0;
    bhkVector4 Col1;
    bhkVector4 Col2;
};

// A rotation and a translation: the placement the collision shapes are built and given to Havok in. Like bhkMatrix3
// it mirrors a Havok class and has no base and no vtable.
class bhkTransform {
public:
    bhkTransform();

    // The rotation becomes the identity and the translation the zero vector.
    void setIdentity();
    // Sets the rotation from arNiTran's matrix (transposed, no scaling) and the translation from its translation,
    // scaled to Havok units; returns this.
    bhkTransform& NI2HK(const NiTransform& arNiTran);

    // Copies the rotation and the translation into arDst's and returns arDst.
    hkTransform& ToHK(hkTransform& arDst) const {
        Rot.ToHK(arDst.getRotation());
        Trans.ToHK(arDst.getTranslation());

        return arDst;
    }

    bhkMatrix3 Rot;
    bhkVector4 Trans;
};

inline bhkTransform& bhkTransform::NI2HK(const NiTransform& arNiTran) {
    Rot.NI2HKNS(arNiTran.m_Rotate);
    Trans.NI2HK(arNiTran.m_Translate);
    return *this;
}

// Each column goes through one NiPoint3.
inline bhkMatrix3& bhkMatrix3::NI2HKNS(const NiMatrix3& arSrc) {
    NiPoint3 kCol;
    arSrc.GetCol(0, kCol);
    Col0.NI2HKNS(kCol);
    arSrc.GetCol(1, kCol);
    Col1.NI2HKNS(kCol);
    arSrc.GetCol(2, kCol);
    Col2.NI2HKNS(kCol);
    return *this;
}
