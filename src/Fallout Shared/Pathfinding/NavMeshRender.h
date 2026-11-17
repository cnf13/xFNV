#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NavMesh;
struct NavMeshTriangle;
class NiMaterialProperty;
class NiTriShape;
class TESObjectCELL;

// The two node maps' out-of-line constructors: the table of auiHashSize zeroed buckets. Defined in NavMeshRender.cpp.
using NiTMapBase_NavMesh_NiPointer_NiNode =
	NiTMapBase<DFALL<NiTMapItem<NavMesh*, NiPointer<NiNode>>>, NavMesh*, NiPointer<NiNode>>;
using NiTMapBase_TESObjectCELL_NiPointer_NiNode =
	NiTMapBase<DFALL<NiTMapItem<TESObjectCELL*, NiPointer<NiNode>>>, TESObjectCELL*, NiPointer<NiNode>>;

// The debug renderer of the navmeshes. All of its data members are named after the PDB's.
class NavMeshRender {
public:
	// A navmesh debug draw colour (PDB values): the index into ColorValues and the material arrays.
	enum NAVMESH_RENDER_COLOR_TYPE : int32_t {
		NAVMESH_RENDER_COLOR_TYPE_NORMAL = 0,
		NAVMESH_RENDER_COLOR_TYPE_HOVER = 1,
		NAVMESH_RENDER_COLOR_TYPE_GREEN = 2,
		NAVMESH_RENDER_COLOR_TYPE_SELECTED_HOVER = 3,
		NAVMESH_RENDER_COLOR_TYPE_BLUE_GREEN = 4,
		NAVMESH_RENDER_COLOR_TYPE_DARK_BLUE_GREEN = 5,
		NAVMESH_RENDER_COLOR_TYPE_YELLOW = 6,
		NAVMESH_RENDER_COLOR_TYPE_BLUE = 7,
		NAVMESH_RENDER_COLOR_TYPE_CYAN = 8,
		NAVMESH_RENDER_COLOR_TYPE_WHITE = 9,
		NAVMESH_RENDER_COLOR_TYPE_BLACK = 10,
		NAVMESH_RENDER_COLOR_TYPE_OBSTACLE = 11,
		NAVMESH_RENDER_COLOR_TYPE_OBSTACLE_BOUNDS = 12,
		NAVMESH_RENDER_COLOR_TYPE_ORANGE = 13,
		NAVMESH_RENDER_COLOR_TYPE_NOLARGECREATURE = 14,
		NAVMESH_RENDER_COLOR_TYPE_PURPLE = 15,
		NAVMESH_RENDER_COLOR_TYPE_DARK_RED = 16,
		NAVMESH_RENDER_COLOR_SIZE = 17,
	};

	NavMeshRender();
	~NavMeshRender();

	bool GetDraw();
	// Adds the draw-only navmeshes of apCell.
	void AddDrawOnlyNavMeshes(TESObjectCELL* apCell, bool abUpdate);
	void UpdateRenderOnlyNavMeshes();
	// Updates the navmesh debug draw.
	void Update();
	void SetDraw(bool abDraw);
	void SetTransparentOverLayMode(bool abTransparent);
	void RemoveCell(TESObjectCELL* apCell);
	NAVMESH_RENDER_COLOR_TYPE GetTriangleColor(const NavMeshTriangle* apTri);

	static void operator delete(void* apMemory);
	static NavMeshRender* GetInstance();
	static void InitSingleton();
	static void Destroy();

	// The debug geometry, the materials and the NavMeshNodes map.
	NiPointer<NiNode> sp3DNode;
	NiPointer<NiNode> spSelectedPathGeometry;
	// 0x6A61B0 detaches and releases it like spSelectedPathGeometry.
	NiPointer<NiNode> spLOSMapGeometry;
	NiPointer<NiTriShape> spVertPoints[17];
	NiPointer<NiMaterialProperty> spEdgeAndVertexMaterialArray[17];
	// The destructor 0x6A3580 releases it.
	NiPointer<NiMaterialProperty> spTriangleMaterialArray[17];
	// The draw colour of each navmesh triangle colour type (the PDB's NAVMESH_RENDER_COLOR_TYPE, 17 entries);
	// the constructor 0x6A2A20 initialises all of them.
	NiColorA ColorValues[17];
	NiPointer<NiAlphaProperty> spAlphaProp;
	NiPointer<NiMaterialProperty> spLocalMapMaterial;
	NiTMap<NavMesh*, NiPointer<NiNode>> NavMeshNodes;
	// The draw node of each cell; the exe's SetAt (0x6C6920) stores NiPointer values.
	NiTMap<TESObjectCELL*, NiPointer<NiNode>> NavMeshCellMap;
	bool bDraw;
	uint8_t padding211[0x3];

	static inline NavMeshRender* TheRender;
	static NiPointer<NiNode> spRootDrawNode;
	static inline int32_t iNavMeshDrawMode;
	static inline bool bDrawCover;
	static inline bool bDrawConnections;
	static inline bool bDrawTriangles;
};

// The game settings NavMeshRender.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bSelectedPathDrawAvoidNodes;
inline float fPointSize = 5.0f;
inline float fedgeHalfWidth = 7.0f;
inline float fedgeAdditionalHeight = 2.0f;
inline float fcoverBoxHalfWidth = 5.0f;
