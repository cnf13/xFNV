#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BGSTextureSet;
class bhkRigidBody;
class COORD_DATA;
class hkpMoppCode;
class NiGeometry;
class NiLines;
class NiNode;
class NiTriStrips;
class QueuedFile;
class TESGrassAreaParam;
class TESLandTexture;
class TESObjectCELL;
class TESObjectLAND;
class TESWorldSpace;

// One texture layer of a land quadrant (a BTXT or ATXT chunk).
struct FILE_TEX_DATA {
	// Swaps the texture ID and the layer index read from a big-endian file.
	void Endian();

	uint32_t iTextureID;
	uint8_t cBlock;
	uint8_t padding05;
	uint16_t sIndex;
};

// One vertex of a texture layer's blend (a VTXT chunk entry): the vertex and how much of the layer shows there.
struct FILE_TEX_VERT_DATA {
	// Swaps the vertex index and the percent read from a big-endian file.
	void Endian();

	uint16_t iVertIndex;
	uint8_t padding02[2];
	float fPercent;
};

// The bits of a land's flags (OBJ_LAND::iFlags).
enum LAND_DATA_FLAGS : uint32_t {
	LAND_DATA_LOADED = 0x8,
};

// A land's flags (the LAND record's DATA chunk).
struct OBJ_LAND {
	// Swaps the flags read from a big-endian file.
	void Endian();

	uint32_t iFlags;
};

// A land's heights (the LAND record's VHGT chunk): the height of the first vertex, then each vertex's signed step from
// the one before it, row by row of 33. Only the first float is byte-swapped.
struct FILE_HEIGHT_DATA {
	// Swaps the first height read from a big-endian file.
	void Endian();

	float fOffset;
	int8_t cHeightData[1089];
	uint8_t padding445[3];
};

// The eight lands around a land (FillLands8), clockwise from the north-west.
struct LANDS8 {
	TESObjectLAND* pNWLand;
	TESObjectLAND* pNLand;
	TESObjectLAND* pNELand;
	TESObjectLAND* pWLand;
	TESObjectLAND* pELand;
	TESObjectLAND* pSWLand;
	TESObjectLAND* pSLand;
	TESObjectLAND* pSELand;
};

// The landscape (LAND) of an exterior cell: heights, normals, vertex colours and texture layers.
class TESObjectLAND : public TESForm, public TESChildCell {
public:
	// The geometry, textures and heights of a loaded land.
	struct LoadedLandData;

	// Grouping of the land's per-vertex arrays.
	struct LandData {
		// The 17 by 17 vertices of a land, as points.
		struct Geometry {
			NiPoint3 kVertices[289];
		};

		// The vertex normals of a land.
		struct Normals {
			NiPoint3 kNormals[289];
		};

		// The vertex colours of a land.
		struct Colors {
			NiColorA kVertColor[17][17];
		};

		// Whether each vertex normal has been computed.
		struct NormalSets {
			bool bNormalSets[289];
		};

		// The texture layers of one quadrant.
		struct Textures {
			TESLandTexture* pTexture[6];
		};
	};

	using pTriStripListBLOCK_t = uint16_t[1021];
	using pYOffsetBLOCK_t = float[4];
	using pXOffsetBLOCK_t = float[4];
	using ppDefVertexBLOCK_t = NiPoint3* [4];

	TESObjectLAND();
	~TESObjectLAND() override;

	bool Load(TESFile* pFile) override;
	void Save() override;
	bool SavesBefore(TESForm* apForm) override;
	bool SavesBefore(FORM* apGroupFORM) override;
	TESForm* CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) override;
	void SetAltered(bool abVal) override;
	void Copy(TESForm* apCopy) override;
	bool BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) override;
	void CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) override;
	TESObjectCELL* GetSaveParentCell() override;
	using TESForm::Save;

	// Attaches the land's 3D to the cell.
	void Attach3D();
	// Loads the land's vertex data.
	bool LoadVertices(bool abLoad3D);
	// Frees the land's vertex data.
	bool UnLoadVertices();
	// Builds the land's geometry from apFrom.
	int8_t CreateLandscape(TESObjectLAND* apFrom);
	void DrawLandBorder(bool bShow);
	// The cell X of the land's loaded data, or its cell's.
	int32_t GetDataX();
	// The cell Y of the land's loaded data, or its cell's.
	int32_t GetDataY();
	// The world X of the land's cell: GetDataX() << 12.
	float GetWorldCellX();
	// Whether the land's vertex data is loaded.
	bool HasVertices();
	// The texture of layer aiLayer of block acBlock, or null.
	TESLandTexture* GetQuadTexture(uint8_t acBlock, uint16_t aiLayer);
	float GetDefaultWorldHeight();
	// Fills cdata for the land point coord.
	bool GetCoordData(COORD_DATA* cdata, const NiPoint3& coord, bool forcetovertex);
	// The land's height at the point cdata describes.
	bool GetLandHeight(const COORD_DATA& cdata, float& height);
	// The geometry of the land under arPoint, or null when the point is off this land.
	NiGeometry* GetGeometry(const NiPoint3& arPoint);
	// The geometry of the land at the point cdata describes.
	NiGeometry* GetGeometry(const COORD_DATA& cdata);
	// Whether any of Data's flags 1, 4 or 2 is set.
	bool IsLandAltered();
	// pParentCell.
	TESObjectCELL* GetParentCell() { return pParentCell; }
	void SetCell(TESObjectCELL* apCell);
	// The land's height under coord's x and y into height; false (with the default world height) when the point is
	// off this land.
	bool GetLandHeight(const NiPoint3& coord, float& height);
	// The land's main texture at aCoord, or null.
	TESLandTexture* GetMainTexture(const NiPoint3& aCoord);
	// The vertex and face normals under aCoord; false when it is off this land.
	bool GetLandNormal(const NiPoint3& aCoord, NiPoint3& aNormal, NiPoint3& aFaceNormal);
	// The vertex and face normals at cdata; false when it is off this land.
	bool GetLandNormal(const COORD_DATA& cdata, NiPoint3& aNormal, NiPoint3& aFaceNormal);
	bool GetLandColor(const NiPoint3& aCoord, NiColorA& aColor);
	// The color at cdata: GetColorA with cdata's dwords at +0x18 and +0x3C into
	// color; returns true.
	bool GetLandColor(const COORD_DATA& cdata, NiColorA& color);
	void InitDistantTextureBlending();
	float GetWorldCellY();
	TESWorldSpace* GetWorldSpace();
	// The default texture of quadrant acBlock, or null when the block is out of range or nothing is loaded.
	TESLandTexture* GetDefaultTexture(uint8_t acBlock);
	float GetBaseHeight();
	// The world offset of the land's block aiBlock: its cell's world X and Y plus half a cell, and the land's base
	// height. aiBlock is never read.
	NiPoint3 GetWorldOffsetForBlock(int32_t aiBlock);
	// The world position of vertex aucVertIndex of the block ausCellX names; apiCellX and apiCellY override the
	// land's own cell coordinates when they are set.
	NiPoint3 GetVertexLocation(uint8_t acBlock, uint16_t asVertIndex, int32_t* apCellX, int32_t* apCellY);
	// The lowest and highest height of the land, computed from its four blocks the first time they are asked for.
	NiPoint2 GetMinMaxLandHeight();
	// The height range of block iblock.
	NiPoint2 GetMinMaxBlockHeight(int32_t iblock);
	bool GetLandRemapped();
	bool HasGoodNormals();
	void SetGoodNormals(bool abVal);
	NiNode* GetQuadLandNode(int32_t aiBlock);
	void MarkLoaded(bool loaded);
	void SetTexture(uint8_t acBlock, uint16_t asTexIndex, TESLandTexture* apTexture);
	float GetDefaultTexturePercent(uint8_t acBlock, uint16_t asVertIndex);
	float GetTexturePercent(uint8_t acBlock, uint16_t asVertIndex, uint16_t asTexIndex);
	void SetTexturePercent(uint8_t acBlock, uint16_t asVertIndex, uint16_t asTexIndex, float afPercent);
	void RemoveTextureFromArrays(uint8_t acBlock, uint16_t asRemoveIndex);
	bool VertexInRect(tagRECT* apRect, const NiPoint3& aPoint);
	bool IsNormalSet(int32_t coord, int32_t a2);
	// Whether the normal of the vertex at coord is set; false when the point is off this land.
	bool IsNormalSet(const NiPoint3& coord);
	void UpdateUseParentWorldspace(char a2);
	void SetHasMopp(uint8_t a2);
	float GetTextureUsePercent(uint8_t acBlock, uint16_t asTexIndex);
	void GetColorA(int32_t iblock, int32_t index, NiColorA& color);
	NiTriStrips* MakeLandTriStrips(uint8_t aiBlock);
	bool InitLandscape();
	void AdjustTextureArrays();
	void MergeSameFileTextures();
	char CalculateGrassParams();
	bool ForceGoodNormals();
	NiPoint3** GetVertexArray();
	void GetVertex(int32_t iblock, int32_t index, NiPoint3& vert);
	void GetNormal(int32_t iblock, int32_t index, NiPoint3& norm);
	bool LoadVerticesIntoArrays();
	void Detach3D();
	bool ReadMoppCodeFromBuffer(char* apBuffer, uint32_t aiBufferSize, hkpMoppCode** apOutCode);
	TESLandTexture* GetMainTexture(uint8_t acBlock, uint16_t asVertIndex);
	void SetNewDefaultTexture(uint8_t acBlock, uint16_t asNewDefaultIndex);
	TESGrassAreaParam** GetGrassParameters(int32_t aiBlock, int32_t aiVert);
	NiPoint3** GetNormalArray();
	void InvalidateNormals(tagRECT* apInvalidateRect);
	void UpdateMesh(bool updatescene, LANDS8* pL8);
	void FillLands8(LANDS8* pL8, bool abForceLoad, bool* apLoaded);
	void SetLandNormals(LANDS8* pL8, tagRECT* apCalcRect);
	void CalculateEdgeNormal(int32_t iblock, int32_t vertidx, NiPoint3& normal, LANDS8* pL8);
	bool IsLoaded();

	static TESLandTexture* GetDefaultTexture();
	static int32_t GetGrassEvalSize();
	// Builds the default land blocks shared by every land.
	static void InitializeStatics();

	OBJ_LAND Data;
	TESObjectCELL* pParentCell;
	NiPointer<QueuedFile> spQueuedTextures;
	LoadedLandData* pLoadedData;

	static inline TESLandTexture* pDefText;
	static int32_t iGrassEvalSize;
	static inline pTriStripListBLOCK_t pTriStripListBLOCK{136, 119, 120, 102, 102, 120, 120, 137, 136, 154, 153, 170,
		170, 137, 137, 137, 154, 138, 155, 155, 68, 68, 85, 86, 102, 103, 120, 104, 121, 122, 121, 138, 120, 137, 137,
		103, 103, 103, 104, 86, 87, 87, 138, 138, 139, 122, 140, 123, 124, 123, 106, 122, 105, 104, 88, 87, 87, 105,
		105, 105, 106, 88, 89, 89, 156, 156, 156, 139, 140, 140, 124, 124, 124, 141, 140, 140, 156, 156, 156, 173, 172,
		172, 188, 188, 188, 171, 172, 172, 205, 205, 205, 204, 188, 187, 188, 170, 171, 154, 172, 155, 156, 138, 139,
		139, 174, 174, 173, 190, 172, 189, 188, 206, 205, 222, 204, 222, 221, 222, 238, 238, 189, 189, 206, 190, 207,
		207, 224, 224, 206, 223, 222, 240, 239, 239, 242, 242, 241, 258, 241, 240, 224, 223, 223, 242, 242, 259, 258,
		258, 222, 222, 238, 239, 256, 240, 257, 258, 274, 258, 275, 258, 276, 259, 260, 260, 257, 257, 257, 274, 256,
		273, 256, 272, 256, 255, 238, 238, 243, 243, 260, 244, 261, 261, 276, 276, 277, 260, 278, 261, 262, 244, 245,
		228, 245, 246, 262, 263, 263, 263, 263, 263, 264, 246, 247, 247, 262, 262, 262, 263, 280, 264, 281, 282, 282,
		280, 280, 279, 262, 278, 278, 228, 228, 229, 246, 230, 247, 248, 264, 265, 282, 266, 283, 266, 284, 284, 266,
		266, 266, 265, 248, 248, 230, 230, 230, 231, 248, 248, 266, 266, 284, 267, 268, 267, 250, 266, 249, 248, 232,
		231, 214, 230, 230, 249, 249, 249, 250, 232, 233, 233, 179, 179, 196, 180, 197, 198, 214, 215, 232, 216, 233,
		234, 250, 250, 216, 216, 216, 215, 198, 198, 216, 216, 216, 217, 234, 234, 180, 180, 180, 181, 198, 198, 252,
		252, 252, 251, 234, 234, 270, 270, 254, 253, 236, 252, 235, 234, 218, 217, 200, 216, 199, 199, 235, 235, 236,
		218, 219, 219, 199, 199, 200, 182, 183, 183, 164, 164, 164, 165, 182, 166, 183, 184, 200, 201, 218, 202, 219,
		220, 236, 237, 254, 254, 201, 201, 202, 184, 185, 185, 220, 220, 203, 202, 186, 185, 168, 184, 167, 166, 150,
		149, 132, 132, 167, 167, 168, 150, 151, 151, 132, 132, 132, 133, 150, 134, 151, 152, 168, 169, 186, 186, 133,
		133, 134, 116, 117, 117, 131, 131, 132, 114, 115, 115, 78, 78, 78, 62, 79, 80, 96, 97, 114, 98, 115, 116, 132,
		133, 133, 97, 97, 97, 98, 80, 81, 81, 95, 95, 96, 78, 79, 79, 152, 152, 135, 134, 118, 117, 100, 116, 99, 98,
		82, 81, 64, 64, 99, 99, 100, 82, 83, 83, 118, 118, 101, 100, 84, 83, 66, 82, 65, 64, 65, 48, 66, 49, 49, 84, 84,
		84, 67, 66, 50, 49, 32, 48, 31, 30, 14, 30, 13, 30, 12, 12, 31, 31, 32, 14, 32, 15, 32, 16, 32, 33, 50, 50, 30,
		30, 12, 29, 28, 46, 45, 62, 44, 61, 60, 61, 78, 62, 62, 27, 27, 27, 44, 28, 45, 45, 59, 59, 59, 42, 60, 43, 44,
		26, 27, 10, 28, 11, 12, 12, 10, 10, 10, 26, 9, 26, 8, 25, 25, 23, 23, 6, 24, 7, 24, 8, 24, 25, 42, 26, 43, 43,
		94, 94, 78, 77, 60, 76, 59, 58, 42, 41, 24, 40, 23, 23, 57, 57, 58, 40, 41, 41, 39, 39, 39, 40, 56, 57, 74, 58,
		75, 76, 92, 93, 110, 94, 94, 93, 93, 93, 94, 76, 77, 77, 91, 91, 92, 74, 75, 75, 127, 127, 128, 110, 111, 111,
		145, 145, 128, 144, 127, 126, 110, 109, 92, 108, 91, 90, 74, 74, 108, 108, 108, 109, 126, 126, 144, 144, 144,
		145, 162, 162, 108, 108, 108, 107, 90, 90, 180, 180, 180, 179, 162, 162, 72, 72, 72, 73, 90, 90, 72, 72, 72, 71,
		54, 54, 36, 36, 53, 54, 70, 71, 88, 72, 89, 90, 106, 107, 124, 124, 36, 36, 36, 37, 54, 54, 36, 36, 36, 35, 18,
		18, 36, 36, 19, 18, 2, 18, 1, 18, 0, 18, 17, 18, 34, 35, 52, 36, 53, 53, 69, 69, 70, 52, 53, 53, 88, 88, 88, 87,
		70, 86, 69, 68, 52, 51, 34, 34, 252, 252, 252, 253, 270, 270, 234, 234, 250, 251, 268, 252, 269, 270, 286, 270,
		287, 270, 288, 270, 271, 270, 254, 254, 269, 269, 269, 286, 268, 285, 284, 284, 242, 242, 242, 225, 226, 226,
		210, 210, 210, 227, 226, 226, 259, 259, 260, 242, 243, 226, 244, 227, 228, 210, 211, 194, 212, 212, 210, 210,
		210, 193, 194, 194, 211, 211, 228, 212, 229, 230, 230, 214, 214, 230, 213, 212, 213, 196, 214, 197, 197, 194,
		194, 194, 212, 195, 196, 195, 178, 194, 177, 176, 160, 159, 142, 159, 158, 158, 177, 177, 178, 160, 161, 161,
		108, 108, 125, 126, 142, 143, 160, 144, 161, 162, 178, 179, 196, 196, 144, 144, 144, 143, 126, 126, 107, 107,
		124, 108, 124, 125, 124, 142, 141, 158, 140, 157, 156, 174, 173, 173, 157, 157, 174, 158, 175, 175, 159, 159,
		158, 176, 175, 192, 174, 191, 190, 208, 207, 224, 206, 206, 191, 191, 208, 192, 209, 209, 194, 194, 194, 193,
		176, 193, 192, 210, 209, 226, 208, 225, 224, 242, 241, 241, 110, 110, 110, 94, 111, 112, 128, 129, 146, 130,
		147, 148, 164, 165, 165, 129, 129, 129, 130, 112, 113, 113, 163, 163, 164, 146, 147, 147, 149, 149, 148, 166,
		165, 165, 149, 149, 132, 148, 131, 130, 114, 113, 96, 112, 95, 94, 78, 78, 216, 216, 199, 198, 182, 181, 164,
		180, 163, 162, 146, 145, 128, 128, 90, 90, 74, 73, 56, 72, 55, 54, 38, 37, 20, 36, 19, 19, 55, 55, 56, 38, 39,
		39, 19, 19, 20, 2, 20, 3, 4, 4, 40, 40, 23, 22, 6, 22, 5, 22, 4, 21, 20, 21, 38, 22, 39, 40, 40, 81, 81, 81, 64,
		80, 63, 62, 63, 46, 64, 47, 48, 47, 30, 46, 29};
	static inline pYOffsetBLOCK_t pYOffsetBLOCK{};
	static inline pXOffsetBLOCK_t pXOffsetBLOCK{};
	static inline ppDefVertexBLOCK_t ppDefVertexBLOCK{};
	static inline uint16_t* pDefTriListBLOCK;
	static inline NiColorA* pDefColorBLOCK;
	static inline NiPoint3* pDefNormalBLOCK;
	static inline NiPoint2* pDefTextureBLOCK;
	static inline bool* pDefNormalSetBLOCK;
	static inline bool bStaticsDefined;
	static inline int32_t iLANDsinuse;
	static inline BGSTextureSet* pDefTextTexSet;
	static NiPointer<NiTexturingProperty> spTexturingProperty;
};

// The land's loaded 3D data: the mesh, per-vertex arrays, border, height range, per-quadrant texture layers, the
// moppcode and rigid body, the grass maps and the cell coordinates.
struct TESObjectLAND::LoadedLandData {
	LoadedLandData();
	~LoadedLandData();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	NiNode** ppMesh;
	// Per quadrant, its 289 vertices.
	NiPoint3** ppVertices;
	// Per quadrant, its 289 vertex normals.
	NiPoint3** ppNormals;
	// Per quadrant, its 17 by 17 vertex colours; MakeLandTriStrips passes an entry to NiTriStrips's constructor as its
	// NiColorA*.
	NiColorA** ppColorsA;
	// Per quadrant, whether each of its 289 vertex normals has been computed.
	bool** ppNormalsSet;
	NiPointer<NiLines> spBorder;
	NiPoint2 HeightExtents;
	TESLandTexture* pDefQuadTexture[4];
	// Per quadrant, its six texture layers.
	TESLandTexture** pQuadTextureArray[4];
	float** ppPercentArrays[4];
	hkpMoppCode* pMoppCode;
	NiTPointerMap<uint32_t, TESGrassAreaParam**> pmGrassMap[4];
	NiPointer<bhkRigidBody> spLandRB;
	int32_t iCellX;
	int32_t iCellY;
	float fBaseHeight;
};

// Where a point lies in the land: its cell, block and tile, the shape, vertex and the triangle it falls in.
class COORD_DATA {
public:
	COORD_DATA();

	NiPoint2 Cellpoint;
	NiPoint2 Blockpoint;
	tagPOINT Block;
	int32_t iBlock;
	NiPoint2 Tilepoint;
	tagPOINT Tile;
	int32_t iShape;
	NiPoint3 Vertcoord;
	int32_t iVertidx;
	int32_t iTrivertidx[3];
	bool bTriright;
	bool bTriupper;
	uint8_t padding4E[2];
};

extern SettingT<INISettingCollection> gs_iLandBorder1R_Landscape;

extern SettingT<INISettingCollection> gs_fLandTextureTilingMult_Landscape;

extern SettingT<INISettingCollection> gs_fTexturePctThreshold_Grass;

extern SettingT<INISettingCollection> gs_iMaxGrassTypesPerTexure_Grass;

extern SettingT<INISettingCollection> gs_sDefaultLandNormalTexture_Landscape;

extern SettingT<INISettingCollection> gs_iLandBorder1G_Landscape;

extern SettingT<INISettingCollection> gs_iLandBorder2B_Landscape;

extern SettingT<INISettingCollection> gs_iGrassDensityEvalSize_Grass;

extern SettingT<INISettingCollection> gs_iLandBorder1B_Landscape;

extern SettingT<INISettingCollection> gs_iLandBorder2R_Landscape;

extern SettingT<INISettingCollection> gs_sDefaultLandDiffuseTexture_Landscape;

extern SettingT<INISettingCollection> gs_iLandBorder2G_Landscape;
