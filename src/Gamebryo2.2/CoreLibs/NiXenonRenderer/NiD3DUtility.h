#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

class NiMatrix3;
class NiPoint3;
class NiTransform;

// Conversions between Gamebryo and Direct3D types. Only some of the members are declared.
class NiD3DUtility {
public:
    // The Direct3D world matrix of the rotation, translation and uniform scale.
    static void GetD3DFromNi(D3DMATRIX& arD3D, const NiMatrix3& arNiRot, const NiPoint3& arNiTrans, float afNiScale);
    // The Direct3D world matrix of a transform (the overload above with its rotation, translation and scale).
    static void GetD3DFromNi(D3DMATRIX& arD3D, const NiTransform& arNi);
    // GetD3DFromNi's matrix transposed.
    static void
    GetD3DTransposeFromNi(D3DMATRIX& kD3D, const NiMatrix3& kNiRot, const NiPoint3& kNiTrans, float fNiScale);
    // Splits pcFilename into its drive, directory, name and extension buffers; always true.
    static bool GetSplitPath(const char* pcFilename, char* pcDrive, char* pcDir, char* pcFname, char* pcExt);
    // Copies the extension of pcFilename (without the dot) into pcExt, which holds uiMaxLen characters; always true.
    static bool GetFileExtension(const char* pcFilename, char* pcExt, size_t uiMaxLen);
};
