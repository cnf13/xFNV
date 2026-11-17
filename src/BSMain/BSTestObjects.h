#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiAVObject;
class NiPoint3;
class NiTriShape;

// Debug geometry builders and the debug palette; everything here is static.
class BSTestObjects {
public:
	static constexpr uint32_t COLOR_COUNT = 10;
	using ColorTable = NiColorA[COLOR_COUNT];

	// The debug colour aiIndex of the palette.
	static const NiColorA& GetColor(uint32_t aiIndex);
	// A debug arrow along arDir in apColor.
	static NiAVObject* CreateDirArrow(const NiPoint3& arDir, const NiColorA* apColor);
	// A debug cylinder of aiSlices sides and aiStacks rings along z.
	static NiAVObject* CreateCylinder(float afBaseRadius, float afTopRadius, float afHeight, uint16_t aiSlices,
		uint16_t aiStacks, const NiColorA* apColor);
	// A debug torus of aiSlices strips around z, each of aiStacks + 1 quads around the tube, in apColor when given.
	static NiAVObject* CreateTorus(float afInnerRadius, float afOuterRadius, uint16_t aiSlices, uint16_t aiStacks,
		const NiColorA* apColor);
	// A node holding a cylinder and its two caps: zero-height cylinders, the bottom one turned over by a half turn
	// about x and the top one moved up by afHeight.
	static NiAVObject* CreateCappedCylinder(float afBaseRadius, float afTopRadius, float afHeight, uint16_t aiSlices,
		uint16_t aiStacks, const NiColorA* apColor);

	// A debug box from its two corners: the depths below and above them, and the colours of
	// the left and right halves.
	static NiAVObject* CreateBox(const NiPoint3& arLeftCorner, const NiPoint3& arRightCorner, float afForwardDepth,
		float afBackwardDepth, float afUpwardHeight, float afDownwardHeight, const NiColorA* apLeftColor,
		const NiColorA* apRightColor);
	// IDA name `au_re_BSTestObjects::MakeBox`; not in the PDB. A box of half-extent afRadius in y around apCenter,
	// built from the corners apCenter.y - afRadius and apCenter.y + afRadius (cdecl, ret 0, 0xC55CD0).
	static NiAVObject* MakeBox(NiPoint3* apCenter, float afRadius, NiColorA* apColor);
	static NiAVObject* CreateCoordinateJack(float afScale);
	static NiAVObject* Create3DArrow(const NiPoint3& arStart, const NiPoint3& arFinish, float afSize, float afHeight,
		const NiColorA* apStartColor, const NiColorA* apEndColor);
	// Inferred type: afRadius: float: used as one at 0xc555da
	static NiTriShape* MakeTexturedSquare(float afRadius, NiPoint3* apCenter, NiColorA* apColor);
	static NiAVObject* CreateBox(const NiPoint3& arExtents, const NiColorA* apColor);
	static NiAVObject* CreateSphere(float afRadius, uint16_t aiSlices, uint16_t aiStacks, const NiColorA* apColor);
	static NiAVObject* CreateCircle(float afRadius, uint16_t asSegments, const NiColorA* apColor);
	static NiAVObject* CreateArrow(float afLength, const NiColorA* apColor);

	// The debug palette; GetColor indexes it modulo COLOR_COUNT.
	static inline ColorTable pColorA = {
		NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
		NiColorA(1.0f, 0.0f, 0.0f, 1.0f),
		NiColorA(1.0f, 0.62f, 0.16f, 1.0f),
		NiColorA(1.0f, 1.0f, 0.0f, 1.0f),
		NiColorA(0.0f, 1.0f, 0.0f, 1.0f),
		NiColorA(0.0f, 0.0f, 1.0f, 1.0f),
		NiColorA(1.0f, 0.0f, 1.0f, 1.0f),
		NiColorA(0.8f, 0.3f, 0.5f, 1.0f),
		NiColorA(0.5f, 0.5f, 0.5f, 1.0f),
		NiColorA(0.0f, 0.0f, 0.0f, 1.0f),
	};
	static inline NiColorA RED{1.0f, 0.0f, 0.0f, 1.0f};
	static inline NiColorA GREEN{0.0f, 1.0f, 0.0f, 1.0f};
	static inline NiColorA BLUE{0.0f, 0.0f, 1.0f, 1.0f};
};

inline int32_t CreateBox_guards;
