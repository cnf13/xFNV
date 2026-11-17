#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

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
	// The personal folder path buffer GetPersonalPath returns. IDA name; not in the PDB.
	using PersonalPathBuffer = char[260];
	// The local application data path buffer GetLocalAppDataPath returns; the callers copy at most 260 bytes into it,
	// and pPersonalPath follows 0x108 bytes on. IDA name; not in the PDB.
	using LocalAppDataPathBuffer = char[260];

	// The file name of a source texture; null when apTexture is null or not a NiSourceTexture.
	static const char* GetTextureName(NiTexture* apTexture);
	// The user's personal folder path (IDA's name; PC only). A writable buffer: TESMain_LoadIniFile (0x876AD0)
	// copies the path into it through the returned pointer.
	static char* GetPersonalPath();
	// A rotation about Z built from the unitized aVector, transposed (cdecl, 0xC4B8A0; the
	// matrix comes back through a hidden pointer, aVector is passed by value).
	static NiMatrix3 GetRotationZToVectorTranspose(NiPoint3 apVector);
	// The rotation about Z that turns the Z axis toward aVector (cdecl, 0xC4B600; the matrix
	// comes back through a hidden pointer, aVector is passed by value).
	static NiMatrix3 GetRotationZToVector(NiPoint3 apVector);
	// The object named apName: the scene's controller manager's object palette first, then a
	// search of the scene graph when bTestScenegraph.
	static NiAVObject* GetObjectByName(NiAVObject* apScene, const char* apName, bool bTestScenegraph);
	static NiAVObject* GetObjectByName(NiAVObject* apScene, const char* apName);
	// GetObjectByName for a fixed string name.
	static NiAVObject* GetObjectByName(NiAVObject* apScene, const NiFixedString& apName, bool bTestScenegraph);
	// GetObjectByName for a fixed string name with the scene graph search.
	static NiAVObject* GetObjectByName(NiAVObject* apScene, const NiFixedString& apName);
	// (IDA's name; not in the PDB). Whether apObj or a node below it has a shader property with
	// the external emittance flag (cdecl, 0x569410).
	// Inferred type: apObj: NiAVObject* from 0x56949a (result of NiNode::GetAt), its members used at 0x569455
	static bool GetExternalEmittanceRecurse(NiAVObject* apObj);
	// The user's local application data path (IDA's name; PC only): returns the buffer below (cdecl, 0x86D480).
	static char* GetLocalAppDataPath();
	// Sets the always-draw flag and FIXED_BOUND on apObject, then on every child of a node.
	static void SetAlwaysDraw(NiAVObject* apObject);
	// Inferred type: a: float: used as one at 0x49e396; b: float: used as one at 0x49e399; afErrorMargin: float: used
	// as one at 0x49e390
	static bool CompareFloat(float a, float b, float afErrorMargin);
	// Inferred type: a: NiPoint3* from 0xc6bc8a (&NiAVObject+0x8c (NiCollisionObject::m_pkSceneObject (this of
	// bhkNiCollisionObject::SetTransform))), its members used at 0x49e308; afMargin: float from 0xc6b642 (parameter 2
	// of CloseTo)
	static bool ComparePoints(NiPoint3* a, NiPoint3* b, float afMargin);
	static void ApplyTransformToVerticesAndNormals(const NiTransform& aTrans, uint32_t aiVertCount, NiPoint3* apInVerts,
		NiPoint3* apOutVerts, NiPoint3* apInNorms, NiPoint3* apOutNorms);
	static const char* GetSequenceFileName(NiControllerSequence* apSequence);
	static void ProcessSelectiveUpdate(NiAVObject* apObject);
	static void SetSequenceFileName(NiControllerSequence* apSequence, const char* apName);
	static void FindAnimatedTextures(NiAVObject* apObject, NiTPrimitiveArray<NiTexture*>& arTextureArray);
	static void ReflectCameraAboutArbitraryPlane(NiCamera* apCamera, NiPlane* apPlane, NiCamera* apReflectedCamera);
	static void GetInverseWorldMatrix(const NiTransform& aWorld, const NiSkinInstance* apSkin, _D3DMATRIX& aOut);

	static inline PersonalPathBuffer pPersonalPath{};
	static inline LocalAppDataPathBuffer pLocalAppDataPath{};
};

using rdata_10C21E0_t = const char[4];
inline rdata_10C21E0_t rdata_10C21E0{"_:"};
