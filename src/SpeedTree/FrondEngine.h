#pragma once

#include "Reconstruction.hpp"
#include "SpeedTree/CIdvFilename.h"
#include "SpeedTree/CRotTransform.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <__stddef_ptrdiff_t.h>
#include <algorithm>
#include <memory>
#include <vector>

class CIdvBezierSpline;
class CIndexedGeometry;
class CLightingEngine;
class CVec;

// One vertex along a frond guide.
struct SFrondVertex {
	SFrondVertex();

	CVec3 m_cPos;
	CRotTransform m_cTrans;
	float m_fCrossSectionWeight;
	int32_t m_nWindGroup;
};

// A frond's spine: its vertices and the geometry bookkeeping.
struct SFrondGuide {
	SFrondGuide();
	SFrondGuide(const SFrondGuide& arOther);
	~SFrondGuide();

	// operator=(const SFrondGuide&): the vertex vector assigned, then the other fields.
	// Not operator=: the guide stays trivially copyable. The guide vector's fill loops
	// call it.
	static SFrondGuide* Assign(SFrondGuide* apThis, const SFrondGuide& arOther);

	std::vector<SFrondVertex, std::allocator<SFrondVertex>> m_vVertices;
	float m_fLength;
	float m_fRadius;
	uint8_t m_chFrondMapIndex;
	float m_fOffsetAngle;
	float m_fSurfaceArea;
	float m_fFuzzySurfaceArea;
	uint32_t m_nVertexStartIndex;
	uint32_t m_nVerticesPerGuideVertex;
};

// A frond texture: file and the size/angle variation.
struct SFrondTexture {
	// The hybrid's implicit default constructor (the texture vector's insert builds a _Tmp element and reconstructs
	// it): the file name's.
	SFrondTexture() = default;
	SFrondTexture(const SFrondTexture& arOther);
	~SFrondTexture();

	// operator=(const SFrondTexture&): the file name assigned, then the other fields.
	// Not operator=: the texture stays trivially copyable. The texture vector's fill loops call it.
	static SFrondTexture* Assign(SFrondTexture* apThis, const SFrondTexture& arOther);

	CIdvFilename m_strFilename;
	float m_fAspectRatio;
	float m_fSizeScale;
	float m_fMinAngleOffset;
	float m_fMaxAngleOffset;
};

// The string szFormat formats with the arguments (vsprintf into a 1024-byte buffer; cdecl, the string through the
// hidden result pointer, 0xB18860).
std::string IdvFormatString(const char* szFormat, ...);
// Not named in the PDB or IDA; name chosen here. VC9's _Copy_opt over CVec (vector<CVec>'s assign 0xB1D130, its one
// caller, which pushes three tag bytes after the arguments; the body reads none): each element copied whole; returns
// _Dest's end (cdecl, ret 0, 0xB23E80).
CVec* CopyOpt(const CVec* _First, const CVec* _Last, CVec* _Dest);
// VC9's _Copy_backward_opt over SFrondVertex: copies [_First, _Last)
// backward to end at _Dest and returns the destination's start; UncheckedCopyBackward calls it.
SFrondVertex* CopyBackwardOpt(const SFrondVertex* _First, const SFrondVertex* _Last, SFrondVertex* _Dest);
// VC9's _Copy_backward_opt over SFrondGuide: assigns [_First, _Last)
// backward to end at _Dest and returns the destination's start; UncheckedCopyBackward calls it.
SFrondGuide* CopyBackwardOpt(const SFrondGuide* _First, const SFrondGuide* _Last, SFrondGuide* _Dest);
// Not named in the PDB or IDA; name chosen here. VC9's _Copy_opt over SFrondVertex: copies [a1, a2) to a3 and
// returns the destination's end (cdecl, 0xB260C0; vector<SFrondVertex>'s erase and assign).
SFrondVertex* Copy(const SFrondVertex* a1, const SFrondVertex* a2, SFrondVertex* a3);
// Inferred type: a1: SFrondGuide* from 0xb26a17 (parameter 1 of std::_Med3<SFrondGuide, CGuideSorter>); a2:
// SFrondGuide* from 0xb26a17 (parameter 0 of std::_Med3<SFrondGuide, CGuideSorter>)
void iter_swap(SFrondGuide* a1, SFrondGuide* a2);
// Not named in the PDB or IDA; name chosen here. _Adjust_heap over SFrondGuide: _First is the heap's first element
// (_Make_heap 0xB26260 passes _First); _Val by value, _Pred the sorter's byte.
void AdjustHeap(SFrondGuide* _First, int32_t _Hole, int32_t _Bottom, SFrondGuide _Val, uint8_t _Pred);
// Not named in the PDB or IDA; name chosen here. VC9's _Copy_backward_opt of SFrondGuide: assigns [_First, _Last)
// backwards to end at _Dest and returns the destination's start (_Insertion_sort1 0xB262D0 calls it).
SFrondGuide* CopyBackwardOpt(SFrondGuide* _First, SFrondGuide* _Last, SFrondGuide* _Dest);
// Not named in the PDB or IDA; name chosen here. VC9's _Copy_opt over CVec (vector<CVec>'s assign 0xB1D130 and erase
// 0xB1FFD0): each element copied whole; returns _Dest's end (cdecl, ret 0).
CVec* Copy(const CVec* _First, const CVec* _Last, CVec* _Dest);
// VC9's _Copy_opt over SFrondGuide (vector<SFrondGuide>'s erase and
// assign): each element assigned through SFrondGuide::Assign; returns _Dest's end.
SFrondGuide* Copy(const SFrondGuide* _First, const SFrondGuide* _Last, SFrondGuide* _Dest);
// VC9's _Copy_opt over SFrondTexture (vector<SFrondTexture>'s erase):
// each element assigned through SFrondTexture::Assign; returns _Dest's end.
SFrondTexture* CopyOpt(const SFrondTexture* _First, const SFrondTexture* _Last, SFrondTexture* _Dest);
// VC9's _Unchecked_move_backward over SFrondGuide: hands [a1, a2)
// and a3 on to CopyBackwardOpt.
SFrondGuide* UncheckedCopyBackward(SFrondGuide* a1, SFrondGuide* a2, SFrondGuide* a3);
// VC9's _Unchecked_move_backward over SFrondVertex: hands
// [a1, a2) and a3 on to CopyBackwardOpt.
SFrondVertex* UncheckedCopyBackward(SFrondVertex* a1, SFrondVertex* a2, SFrondVertex* a3);
// _Unchecked_move_backward over SFrondTexture: hands the copy to
// _Copy_backward_opt.
SFrondTexture* UncheckedMoveBackward(SFrondTexture* _First, SFrondTexture* _Last, SFrondTexture* _Dest);
// Not named in the PDB or IDA; name chosen here. VC9's _Copy_backward_opt over SFrondTexture (its
// _Unchecked_move_backward 0xB23B00): [_First, _Last) assigned through SFrondTexture::Assign (0xB24A80) to end at
// _Dest, last first; returns the start of the copy (cdecl, ret 0).
SFrondTexture* CopyBackwardOpt(const SFrondTexture* _First, const SFrondTexture* _Last, SFrondTexture* _Dest);
// Not named in the PDB or IDA; name chosen here. VC9's _Copy_opt over SFrondGuide (vector<SFrondGuide>'s assign
// 0xB24F40): each element assigned through SFrondGuide::Assign (0xB249D0); returns _Dest's end (cdecl, ret 0).
SFrondGuide* CopyOpt(const SFrondGuide* _First, const SFrondGuide* _Last, SFrondGuide* _Dest);
// Not named in the PDB or IDA; name chosen here. VC9's _Copy_opt over SFrondVertex: copies [a1, a2) to a3 and
// returns the destination's end (cdecl, 0xB264F0; vector<SFrondVertex>'s assign).
SFrondVertex* CopyOpt(const SFrondVertex* a1, const SFrondVertex* a2, SFrondVertex* a3);
// Not named in the PDB or IDA; name chosen here. _Push_heap of SFrondGuide (_Adjust_heap 0xB26B50 calls it); _Val by
// value (14 dwords), _Pred the sorter's byte.
void PushHeap(SFrondGuide* _First, int32_t _Hole, int32_t _Top, SFrondGuide _Val, uint8_t _Pred);
// Not named in the PDB or IDA; name chosen here. _Pop_heap_0 of SFrondGuide (pop_heap 0xB26CA0 calls it, pushing
// _Val_type's null pointer last).
void _Pop_heap(SFrondGuide* _First, SFrondGuide* _Last, uint8_t _Pred, SFrondGuide* _Valtype);

// Orders guides by descending fuzzy surface area (FrondEngine.cpp's sort predicate; stateless).
class CGuideSorter {
public:
	bool operator()(const SFrondGuide& cLeft, const SFrondGuide& cRight) const {
		return cLeft.m_fFuzzySurfaceArea > cRight.m_fFuzzySurfaceArea;
	}

	// The empty functor's one byte. The exe's algorithm instances pass it on by value (movzx of their own argument
	// byte, pushed as a dword); an empty src class has no byte for clang to copy, so it is modelled as a field.
	uint8_t unk0;
};

class CFrondEngine {
public:
	enum EFrondType : int32_t {
		FROND_BLADES = 0,
		FROND_EXTRUSION = 1,
		NUM_FROND_TYPES = 2,
	};

	CFrondEngine();
	~CFrondEngine();

	// Appends a vertex to the last guide (thiscall, ret 0x38: the arguments by value, 0xB18A60).
	void AddGuideVertex(CVec3 cPos, CRotTransform cTrans, float fCrossSectionWeight, int32_t nWindGroup);
	// Appends an empty guide.
	void StartGuide();
	// Finishes the last guide; fLodSizeScalar sizes its LOD reduction.
	void EndGuide(float fLodSizeScalar);

	// Replaces the profile spline, deleting the one held before unless it is the same.
	void SetProfile(CIdvBezierSpline* pProfile);
	// The branch level the fronds grow on.
	int32_t GetLevel() const;
	// Reads the frond block from cFile.
	void Parse(class CTreeFileAccess& cFile);
	bool Enabled() const;
	// Copies the eight texture coordinates pTexCoords (the odd ones negated when bFlip) to every vertex of
	// pGeometry's frond map nFrondMapIndex.
	void SetTextureCoords(CIndexedGeometry* pGeometry, uint32_t nFrondMapIndex, const float* pTexCoords, bool bFlip);
	// Builds the fronds into pGeometry, lit by pLightingEngine.
	void Compute(CIndexedGeometry* pGeometry, CLightingEngine* pLightingEngine);
	// Private in the PDB. Adds the two vertices of each guide vertex for each of the m_nNumBlades blades of sGuide,
	// then gives the inner vertices texture coordinates along the blade's side lengths (thiscall, ret 4).
	void BuildBladeVertices(SFrondGuide& sGuide);
	// Private in the PDB. The extrusion profile of sGuide (mirrored about the spine) into vProfile and its normals
	// into vNormals (thiscall, ret 0xC).
	void BuildProfileVectors(SFrondGuide& sGuide, std::vector<CVec3, std::allocator<CVec3>>& vProfile,
		std::vector<CVec3, std::allocator<CVec3>>& vNormals);
	// The file name of the frond texture nIndex.
	const char* GetTextureFilename(uint32_t nIndex) const;
	// Private in the PDB. Adds one triangle strip per blade of sGuide to m_pGeometry's LOD nLod, its indices from
	// nStart (thiscall, ret 0xC, 0xB1AED0).
	void ComputeBlade(uint32_t nLod, uint32_t nStart, SFrondGuide& sGuide);
	// Private in the PDB. Extrudes the profile along sGuide's vertices into m_pGeometry
	// (thiscall, ret 4, 0xB1B000; IDA sub_B1B000: it builds the profile vectors of sGuide first, as the SpeedTree
	// source's BuildExtrusionVertices does).
	void BuildExtrusionVertices(SFrondGuide& sGuide);
	void BuildGuideLods();
	// Private in the PDB. Adds sGuide's extrusion strip to m_pGeometry's LOD nLod, its indices from nStart, reduced
	// for the LOD (thiscall, ret 0xC, 0xB1B960).
	void ComputeExtrusion(uint32_t nLod, uint32_t nStart, SFrondGuide& sGuide);

	CIndexedGeometry* m_pGeometry;
	CLightingEngine* m_pLightingEngine;
	std::vector<SFrondGuide, std::allocator<SFrondGuide>> m_vGuides;
	std::vector<std::vector<SFrondGuide, std::allocator<SFrondGuide>>,
		std::allocator<std::vector<SFrondGuide, std::allocator<SFrondGuide>>>>
		m_vGuideLods;
	EFrondType m_eFrondType;
	uint32_t m_nNumBlades;
	CIdvBezierSpline* m_pProfile;
	uint32_t m_nProfileSegments;
	uint32_t m_nLevel;
	bool m_bEnabled;
	uint8_t padding4D[3];
	std::vector<SFrondTexture, std::allocator<SFrondTexture>> m_vTextures;
	uint32_t m_nNumLodLevels;
	float m_fMaxSurfaceAreaPercent;
	float m_fMinSurfaceAreaPercent;
	float m_fReductionFuzziness;
	float m_fLargeRetentionPercent;
	uint32_t m_nMinLengthSegments;
	uint32_t m_nMinCrossSegments;
};
