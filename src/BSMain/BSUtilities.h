#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class NiAVObject;
class NiCamera;
class NiControllerSequence;
class NiFixedString;
class NiMatrix3;
class NiPlane;
class NiPoint3;
class NiSkinInstance;
class NiTexture;
class NiTransform;

// Static helpers over scene graph objects.
class BSUtilities {
public:
    // The personal folder path buffer GetPersonalPath returns.
    using PersonalPathBuffer = char[260];
    // The local application data path buffer GetLocalAppDataPath returns; the callers copy at most 260 bytes into it,
    // and pPersonalPath follows 0x108 bytes on.
    using LocalAppDataPathBuffer = char[260];

    // The file name of a source texture; null when apTexture is null or not a NiSourceTexture.
    static const char* GetTextureName(NiTexture* apTexture);
    // The user's personal folder path (PC only). Returns a writable buffer: Main_LoadIniFile copies the path into it
    // through the returned pointer.
    static char* GetPersonalPath();
    // A rotation about Z built from the unitized aVector, transposed.
    static NiMatrix3 GetRotationZToVectorTranspose(NiPoint3 apVector);
    // The rotation about Z that turns the Z axis toward aVector.
    static NiMatrix3 GetRotationZToVector(NiPoint3 apVector);
    // The object named apName: the scene's controller manager's object palette first, then a
    // search of the scene graph when bTestScenegraph.
    static NiAVObject* GetObjectByName(NiAVObject* apScene, const char* apName, bool bTestScenegraph);
    static NiAVObject* GetObjectByName(NiAVObject* apScene, const char* apName);
    // GetObjectByName for a fixed string name.
    static NiAVObject* GetObjectByName(NiAVObject* apScene, const NiFixedString& apName, bool bTestScenegraph);
    // GetObjectByName for a fixed string name with the scene graph search.
    static NiAVObject* GetObjectByName(NiAVObject* apScene, const NiFixedString& apName);
    // Whether apObj or a node below it has a shader property with the external emittance flag.
    static bool GetExternalEmittanceRecurse(NiAVObject* apObj);
    // The user's local application data path (PC only): returns the buffer below.
    static char* GetLocalAppDataPath();
    // Sets the always-draw flag and FIXED_BOUND on apObject, then on every child of a node.
    static void SetAlwaysDraw(NiAVObject* apObject);
    static bool CompareFloat(float a, float b, float afErrorMargin);
    static bool ComparePoints(NiPoint3* a, NiPoint3* b, float afMargin);
    static void ApplyTransformToVerticesAndNormals(
        const NiTransform& aTrans,
        uint32_t aiVertCount,
        NiPoint3* apInVerts,
        NiPoint3* apOutVerts,
        NiPoint3* apInNorms,
        NiPoint3* apOutNorms
    );
    static const char* GetSequenceFileName(NiControllerSequence* apSequence);
    static void ProcessSelectiveUpdate(NiAVObject* apObject);
    static void SetSequenceFileName(NiControllerSequence* apSequence, const char* apName);
    static void FindAnimatedTextures(NiAVObject* apObject, NiTPrimitiveArray<NiTexture*>& arTextureArray);
    static void ReflectCameraAboutArbitraryPlane(NiCamera* apCamera, NiPlane* apPlane, NiCamera* apReflectedCamera);
    static void GetInverseWorldMatrix(const NiTransform& aWorld, const NiSkinInstance* apSkin, _D3DMATRIX& aOut);

    static inline PersonalPathBuffer pPersonalPath{};
    static inline LocalAppDataPathBuffer pLocalAppDataPath{};
};

// The value on the line through (y1, x1) and (y2, x2) at value: x1 + (value - y1) / (y2 - y1) * (x2 - x1), rounded
// to float.
inline float FLerp(float x1, float x2, float y1, float y2, float value) {
    float fResult = (value - y1) / (y2 - y1) * (x2 - x1) + x1;
    return fResult;
}

inline double fFloor(float afValue) {
    return static_cast<int32_t>(afValue);
}

// The game's floor of a float: the truncation fFloor gives, less 1 when aIn lies below it, with the result rounded to
// float. Not the CRT's floor.
inline float FloatFloor(float aIn) {
    float fFloored = fFloor(aIn);
    if (aIn - fFloored >= 0.0) {
        return fFloored;
    }

    return fFloored - 1.0;
}

inline float FloatCeil(float aIn) {
    float fFloored = fFloor(aIn);
    if (!(fFloored - aIn >= 0.0)) {
        return fFloored + 1.0;
    }

    return fFloored;
}

inline bool FloatEQ(float aX, float aY, float afPrecision) {
    if (afPrecision < 0.0) {
        return false;
    }
    if (!(aY < static_cast<double>(aX))) {
        return afPrecision > (aY - aX);
    }
    return afPrecision > (aX - aY);
}

// The float rounded to an integer in the FPU's current rounding mode.
// Rounds in the current floating-point rounding mode, not by truncation.
inline int32_t FloatToInt(float someFloat) {
    return NiFastFloatToInt(someFloat);
}
