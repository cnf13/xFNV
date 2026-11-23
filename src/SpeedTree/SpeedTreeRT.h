#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class CFrondEngine;
class CIndexedGeometry;
class CLeafGeometry;
class CLightingEngine;
class CProjectedShadow;
// An axis-aligned box (LibVector_Source/IdvVector.h).
class CRegion;
class CRotTransform;
class CSimpleBillboard;
class CSpeedTreeRT;
class CTreeEngine;
class CTreeFileAccess;
class CWindEngine;
struct SCollisionObjects;
// The instances of a tree (Instances.h).
struct SInstanceList;
// The tree an instance copies, where the instance stands and its LOD level (defined in Instances.h).
struct STreeInstanceData;

// The texture coordinates of the leaf, billboard, frond and shadow images embedded in a tree's composite texture.
struct SEmbeddedTexCoords {
    SEmbeddedTexCoords();
    ~SEmbeddedTexCoords();

    int32_t m_nNumLeafMaps;
    float* m_pLeafTexCoords;
    int32_t m_nNumBillboards;
    float* m_pBillboardTexCoords;
    int32_t m_nNumFrondMaps;
    float* m_pFrondTexCoords;
    std::string m_strCompositeFilename;
    float m_afShadowTexCoords[8];
};

// A 4x4 matrix; a vector is transformed as a row (the last row is the translation).
class CTransform {
public:
    // The result of inverting a matrix.
    enum EInversionCodeType : int32_t {
        OK = 0,
        SINGULAR = 1,
    };

    // Sets the identity matrix.
    CTransform* B3CCD0();
    // The constructor: sets the identity matrix through B3CCD0.
    CTransform* B3CCB0();

    float m_afData[4][4];
};

class CVec3 {
public:
    // The components left unset, as by the implicit constructor the declared one below would hide.
    CVec3() = default;
    CVec3(float fX, float fY, float fZ);
    // Allows the aggregate-style initialisation `CVec3{{fX, fY, fZ}}` (the components array), which the declared
    // constructors would otherwise forbid.
    CVec3(const float (&afData)[3]) : m_afData{afData[0], afData[1], afData[2]} {}

    // The distance to cVec, through the fast square root approximation on the float's bits.
    float Distance(const CVec3& cVec) const;
    // Scales the vector to unit length.
    void Normalize();
    // The angle to cVec in radians, from their dot product clamped to [-1, 1].
    float AngleBetween(const CVec3& cVec) const;
    // The dot product.
    float operator*(const CVec3& cVec) const;
    // The vector transformed by cTransform.
    CVec3 operator*(const CTransform& cTransform) const;
    // The vector rotated by cTransform.
    CVec3 operator*(const CRotTransform& cTransform) const;
    // This scaled by afScale, through apResult; only reads this.
    CVec3* Scale(CVec3* apResult, float afScale) const;
    // This minus apOther, written through apResult; only this and apOther are read.
    CVec3* Diff(CVec3* apResult, const CVec3* apOther) const;
    // Writes the sum with apOther through apResult.
    CVec3* Add(CVec3* apResult, const CVec3* apOther) const;
    CVec3& operator*=(float fScale);
    CVec3 Cross(const CVec3& cVec) const;
    // Stores the three components.
    void Set(float fX, float fY, float fZ);
    // The length, through the fast square root approximation on the float's bits of the sum of squares.
    float Magnitude() const;
    CVec3& operator+=(const CVec3& cVec);

    float m_afData[3];
};

enum ESpeedTreeInternalEvent : int32_t {
    STIE_CLIENT_CHANGED_WIND = 0,
    STIE_CLIENT_CHANGED_CAMERA = 1,
    STIE_UNKNOWN = 2,
};

enum ESpeedTreeSizes : int32_t {
    STS_MIN_BOX = 0,
    STS_MAX_BOX = 3,
    STS_BB_SIZE = 6,
    STS_COUNT = 7,
};

class CSpeedTreeRT {
public:
    enum ELodMethod : int32_t {
        LOD_POP = 0,
        LOD_SMOOTH = 1,
        LOD_NONE = 3,
    };

    enum EWindMethod : int32_t {
        WIND_GPU = 0,
        WIND_CPU = 1,
        WIND_NONE = 2,
    };

    enum ELightingMethod : int32_t {
        LIGHT_DYNAMIC = 0,
        LIGHT_STATIC = 1,
    };

    enum EStaticLightingStyle : int32_t {
        SLS_BASIC = 0,
        SLS_USE_LIGHT_SOURCES = 1,
        SLS_SIMULATE_SHADOWS = 2,
    };

    enum ECollisionObjectType : int32_t {
        CO_SPHERE = 0,
        CO_CYLINDER = 1,
        CO_BOX = 2,
    };

    // The geometry CSpeedTreeRT::GetGeometry fills in.
    struct SGeometry {
        // An indexed (strip-based) geometry block: LOD level, strips and per-vertex arrays.
        struct SIndexed {
            SIndexed();
            ~SIndexed();

            int32_t m_nDiscreteLodLevel;
            uint16_t m_usNumStrips;
            const uint16_t* m_pStripLengths;
            const uint16_t** m_pStrips;
            uint16_t m_usVertexCount;
            const uint32_t* m_pColors;
            const float* m_pNormals;
            const float* m_pBinormals;
            const float* m_pTangents;
            const float* m_pCoords;
            const float* m_pTexCoords0;
            const float* m_pTexCoords1;
            const float* m_pWindWeights;
            const uint8_t* m_pWindMatrixIndices;
        };

        struct SLeaf {
            ~SLeaf();

            CSpeedTreeRT::SGeometry::SLeaf* Init();

            bool m_bIsActive;
            float m_fAlphaTestValue;
            int32_t m_nDiscreteLodLevel;
            uint16_t m_usLeafCount;
            const uint8_t* m_pLeafMapIndices;
            const uint8_t* m_pLeafClusterIndices;
            const float* m_pCenterCoords;
            const float** m_pLeafMapTexCoords;
            const float** m_pLeafMapCoords;
            const uint32_t* m_pColors;
            const float* m_pNormals;
            const float* m_pBinormals;
            const float* m_pTangents;
            const float* m_pWindWeights;
            const uint8_t* m_pWindMatrixIndices;
        };

        // One speed tree billboard: whether it is active, its texture and vertex coordinates and its alpha test value.
        // speedtree-2's SBillboard ends at m_fAlphaTestValue; this one has m_usWhich after it.
        struct SBillboard {
            SBillboard();
            ~SBillboard();

            bool m_bIsActive;
            const float* m_pTexCoords;
            const float* m_pCoords;
            float m_fAlphaTestValue;
            uint16_t m_usWhich;
        };

        SGeometry();
        // SGeometry's destructor.
        void Destroy();

        SIndexed m_sBranches;
        float m_fBranchAlphaTestValue;
        SIndexed m_sFronds;
        float m_fFrondAlphaTestValue;
        SLeaf m_sLeaves0;
        SLeaf m_sLeaves1;
        SBillboard m_sBillboard0;
        SBillboard m_sBillboard1;
        SBillboard m_sHorizontalBillboard;
    };

    // The texture file names GetTextures reports.
    struct STextures {
        STextures();
        ~STextures();

        const char* m_pBranchTextureFilename;
        uint32_t m_uiLeafTextureCount;
        const char** m_pLeafTextureFilenames;
        uint32_t m_uiFrondTextureCount;
        const char** m_pFrondTextureFilenames;
        const char* m_pCompositeFilename;
        const char* m_pSelfShadowFilename;
    };

    CSpeedTreeRT();
    CSpeedTreeRT(const CSpeedTreeRT* pOrig);
    ~CSpeedTreeRT();

    // The instance's LOD level, else the tree engine's.
    float GetLodLevel() const;
    uint16_t GetNumBranchLodLevels();
    uint16_t GetNumLeafLodLevels();
    uint16_t GetNumFrondLodLevels() const;
    // Clears the branch geometry's colours, if there is branch geometry.
    void FreeBranchColors();
    // Clears the branch geometry's wind weights and wind matrix indices, if there is branch geometry.
    void FreeBranchWindData();
    // The branch and frond LOD level fLodLevel (-1: the tree's own) selects, the last level for a full LOD of 0.
    int16_t GetDiscreteBranchLodLevel(float fLodLevel);
    int16_t GetDiscreteFrondLodLevel(float fLodLevel);
    // The tree this one is an instance of; null for a tree that is not an instance.
    const CSpeedTreeRT* InstanceOf() const;
    void GetTreeSize(float& fSize, float& fVariance) const;
    uint16_t GetDiscreteLeafLodLevel(float fLodLevel);
    // Moves the instance, else the tree.
    void SetTreePosition(float x, float y, float z);
    // Sets the instance's LOD level, else the tree's; a level outside [0, 1] sets the error string instead.
    void SetLodLevel(float fLevel);
    // Copies the six floats of the tree's bounding box (min, then max) to pBounds.
    void GetBoundingBox(float* pBounds) const;
    void SetLeafRockingState(bool bFlag);
    CSpeedTreeRT::EWindMethod GetLeafWindMethod() const;
    CSpeedTreeRT::EWindMethod GetBranchWindMethod() const;
    CSpeedTreeRT::EWindMethod GetFrondWindMethod() const;
    // Fills sGeometry with the simple billboard's geometry.
    void GetSimpleBillboardGeometry(CSpeedTreeRT::SGeometry& sGeometry);
    const char* GetUserData() const;
    void SetBranchDimming(float fScalar);
    void SetLeafDimming(float fScalar);
    void SetBudAngleMin(float fAngle);
    void SetBudAngleMax(float fAngle);
    void FreeLeafLODDataArrays();
    // Frees the engine's transient data, or reports an error when it is already gone.
    void DeleteTransientData();
    // The branch geometry's trunk width and length, 0 without an engine or branch geometry.
    float GetTrunkWidth() const;
    float GetTrunkLength() const;
    void SetLodLimits(float fNear, float fFar);
    void ComputeSelfShadowTexCoords();
    // Deletes the projected shadow and clears the branch and frond geometry's second texture coordinates.
    void FreeProjectedShadowData();
    // Sets the tree engine's size and variance; an error while the transient data is gone or for a size not above 0.
    void SetTreeSize(float fNewSize, float fNewVariance);
    // The tree engine's seed, 0 (and an error) once the transient data is gone.
    uint32_t GetSeed() const;
    // The lighting and wind setters: an error once the tree is computed, else the engine's method and the
    // geometry's manual-lighting or vertex-weighting flag.
    void SetBranchLightingMethod(ELightingMethod eMethod);
    void SetLeafLightingMethod(ELightingMethod eMethod);
    void SetFrondLightingMethod(ELightingMethod eMethod);
    // Sets the leaves' rocking group count (0 counts as 1); an error once the tree is computed.
    void SetNumLeafRockingGroups(uint32_t nRockingGroups);
    void SetLeafWindMethod(EWindMethod eMethod);
    void SetBranchWindMethod(EWindMethod eMethod);
    void SetFrondWindMethod(EWindMethod eMethod);
    // Sets the wind engine's strength (-1: keep the old strength or time shift) and invalidates the geometry the CPU
    // winds; returns the new time shift (0 and an error for a negative strength).
    float SetWindStrength(float fNewStrength, float fOldStrength, float fFrequencyTimeOffset);
    // Sets the wind engine's local matrices; an error once the tree is computed or past the wind matrices.
    void SetLocalMatrices(uint32_t nStartingMatrix, uint32_t nMatrixSpan);
    // Builds the tree once: the branches, fronds and leaf tables, the embedded texture coordinates, the optional
    // transform, the extents and the billboard size; an error when the tree is already computed.
    bool Compute(const float* pTransform, uint32_t nSeed, bool bCompositeStrips);
    // A new instance sharing this tree's geometry (CSpeedTreeRT(pOrig)); null after an exception.
    CSpeedTreeRT* MakeInstance();
    // Computes the LOD of the tree, or of the instance at its own position (the engine's own state restored).
    void ComputeLodLevel();
    // Bethesda's LoadTree(pFilename): the file read through FileFinder (a BSFile) into a block for LoadTree(pBlock);
    // false and the error string when the file is missing, empty or short.
    bool LoadTree(const char* pFilename);
    // Reads the tree from the nNumBytes bytes at pBlock; false and the error string on a bad file.
    bool LoadTree(const uint8_t* pBlock, uint32_t nNumBytes);

protected:
    // Reads the collision shapes up to File_EndCollisionInfo.
    void ParseCollisionObjects(CTreeFileAccess* pFile);
    // Creates the projected shadow and reads it from pFile.
    void ParseShadowProjectionInfo(CTreeFileAccess* pFile);
    // Fills sGeometry's fronds: the frond geometry's arrays at the LOD level (sOverrideLodValue, else the tree's) and
    // the alpha test value. The LOD values below are signed shorts and are compared signed with -1.
    void GetFrondGeometry(SGeometry& sGeometry, int16_t sOverrideLodValue);
    // Fills sGeometry's leaves at the LOD sOverrideLeafLodValue, else the tree's, after updating every LOD.
    void GetLeafGeometry(SGeometry& sGeometry, int16_t sOverrideLeafLodValue);
    // Sets up the horizontal billboard's coordinates.
    void SetupHorizontalBillboard();

public:
    // Bethesda's form of GetLeafGeometry: the leaves and the billboard alpha at the LOD
    // level fLodLevel.
    void GetBSGeometryLODData(SGeometry& sGeometry, float afLODlevel);

protected:
    // Lights the leaf LODs statically around the bounding box's centre.
    void ComputeLeafStaticLighting();
    // Reads the user data string up to File_EndUserData.
    void ParseUserData(CTreeFileAccess* pFile);
    // Parse the LOD, wind, texture coordinate and supplemental texture coordinate blocks of a tree file.
    void ParseLodInfo(CTreeFileAccess* pFile);
    void ParseWindInfo(CTreeFileAccess* pFile);
    void ParseTextureCoordInfo(CTreeFileAccess* pFile);
    void ParseSupplementalTexCoordInfo(CTreeFileAccess* pFile);

public:
    // Deletes the branch or frond geometry of a computed tree that is not an instance and is the last reference.
    void DeleteBranchGeometry();
    void DeleteFrondGeometry();
    // The leaf geometry's billboard table and its entry count.
    const float* GetLeafBillboardTable(uint32_t& nEntryCount) const;
    // The collision object nIndex's type, position and dimensions; an error without collision objects or past them.
    void GetCollisionObject(uint32_t nIndex, ECollisionObjectType& eType, float* pPosition, float* pDimensions);
    // The number of collision objects, 0 without any. Unsigned: BSTreeNode::MakeHavokLeaves compares it unsigned.
    uint32_t GetCollisionObjectCount();
    // Fills sGeometry with the geometry ulBitVector selects (1 branches, 2 fronds, 4 leaves, 8 billboards).
    void GetGeometry(
        SGeometry& sGeometry,
        uint32_t ulBitVector,
        int16_t sOverrideBranchLodValue,
        int16_t sOverrideFrondLodValue,
        int16_t sOverrideLeafLodValue
    );
    // Fills sGeometry's branches.
    void GetBranchGeometry(SGeometry& sGeometry, int16_t sOverrideLodValue);
    // Fills sGeometry's 360-degree billboards.
    void Get360BillboardGeometry(SGeometry& sGeometry, uint32_t ulBitVector);
    // The per-leaf-LOD size factors, built on first use (1 + level * the size increase factor).
    const float* GetLeafLodSizeAdjustments();
    // The texture file names, in arrays allocated here.
    void GetTextures(STextures& sTextures);

    // Copies apError into the engine's error string.
    static void SetError(const char* pError);
    // Whether SpeedTree textures are flipped.
    static bool GetTextureFlip();
    static void SetTextureFlip(bool bFlag);
    // The LOD transition's high and low alpha values and LOD levels at fLodLevel.
    static void GetTransitionValues(
        float fLodLevel,
        uint16_t usLodCount,
        float fOverlapRadius,
        float fTransitionFactor,
        float fCurveExponent,
        float fTargetAlphaValue,
        float& fHighValue,
        float& fLowValue,
        int16_t& sHighLod,
        int16_t& sLowLod
    );
    static void SetDropToBillboard(bool bFlag);
    // Copies the camera position and direction to the two three-float arrays.
    static void GetCamera(float* pPosition, float* pDirection);
    // Frees through ourFree.
    static void operator delete(void* apMemory);
    // Resizes the shared wind matrices.
    static void SetNumWindMatrices(int32_t nNumMatrices);
    // Moves the shared camera; when it changed, invalidates every tree's camera-dependent geometry, recomputes the
    // unit billboard and the horizontal billboard's fade.
    static void SetCamera(const float* pPosition, const float* pDirection);
    // Sets the shared wind time and invalidates every tree's CPU-wind geometry.
    static void SetTime(float fTime);

protected:
    // Invalidates every unique tree's CPU-wind geometry (STIE_CLIENT_CHANGED_WIND) or its leaves and billboard
    // (STIE_CLIENT_CHANGED_CAMERA), under the unique trees' critical section.
    static void NotifyAllTreesOfEvent(int32_t nMessage);
    // A new copy of pUserData.
    static char* CopyUserData(const char* pOrig);

public:
    CTreeEngine* m_pEngine;
    CIndexedGeometry* m_pBranchGeometry;
    CLeafGeometry* m_pLeafGeometry;
    CLightingEngine* m_pLightingEngine;
    CWindEngine* m_pWindEngine;
    CSimpleBillboard* m_pSimpleBillboard;
    ELodMethod m_eLeafLodMethod;
    float m_fLeafLodTransitionRadius;
    float m_fLeafLodCurveExponent;
    float m_fLeafSizeIncreaseFactor;
    float m_fLeafTransitionFactor;
    float* m_pLeafLodSizeFactors;
    uint32_t* m_pInstanceRefCount;
    STreeInstanceData* m_pInstanceData;
    SInstanceList* m_pInstanceList;
    int32_t m_nFrondLevel;
    float* m_pTreeSizes;
    uint8_t m_ucTargetAlphaValue;
    bool m_bTreeComputed;
    int32_t m_nBranchWindLevel;
    SEmbeddedTexCoords* m_pEmbeddedTexCoords;
    CProjectedShadow* m_pProjectedShadow;
    // The game's SpeedTree version has this member; speedtree-2 does not.
    uint16_t m_usNum360Billboards;
    SCollisionObjects* m_pCollisionObjects;
    CFrondEngine* m_pFrondEngine;
    CIndexedGeometry* m_pFrondGeometry;
    uint16_t m_usNumFrondLodLevels;
    char* m_pUserData;
    bool m_b360Billboard;
    bool m_bHorizontalBillboard;
    float m_afHorizontalCoords[12];

    static inline bool m_bDropToBillboard;
    static inline bool m_bTextureFlip;
    static inline uint32_t m_uiAllRefCount;
    // Statics that speedtree-2 lacks, used by SetCamera.
    static inline float m_fHorizontalFadeStartAngle = 30.0f;
    static inline float m_fHorizontalFadeEndAngle = 60.0f;
    static inline float m_fHorizontalFadeValue = 84.0f;
};

// One collision shape of a tree: its type, position and dimensions. Cpp-local in speedtree-2's SpeedTreeRT.cpp.
struct SShape {
    CSpeedTreeRT::ECollisionObjectType m_eType;
    float m_afPosition[3];
    float m_afDimensions[3];
};

// The azimuth of the camera direction pCameraDir in degrees.
float ComputeAzimuth(const float* pCameraDir);
SShape* UncheckedCopyBackward(SShape* _First, SShape* _Last, SShape* _Dest);
// Copies the 28-byte SShape elements of [_First, _Last) backward to end at
// _Dest and returns the destination's start; UncheckedCopyBackward calls it.
SShape* CopyBackwardOpt(SShape* _First, SShape* _Last, SShape* _Dest);
uint8_t* UninitFillN(uint8_t* _First, int32_t _Count, uint8_t* _Val);
// Copies the bytes [_First, _Last) to _Dest with memmove_s when the range is not empty, returning
// _Dest + (_Last - _First); its one caller is the byte vector's erase.
uint8_t* CopyOpt(const uint8_t* _First, const uint8_t* _Last, uint8_t* _Dest);
// Copies the bytes [_First, _Last) with memmove_s so that they end at _DestEnd, when the range is not empty, and
// returns the destination's start _DestEnd - (_Last - _First). The byte vector's _Insert_n calls it.
uint8_t* CopyBackwardOpt(uint8_t* _First, uint8_t* _Last, uint8_t* _DestEnd);
// Copies bytes; the allocator _Al is never read.
uint8_t* UninitCopy(uint8_t* _First, uint8_t* _Last, uint8_t* _Dest, std::allocator<uint8_t>& _Al);
// Fills _Count bytes at _First with *_Val.
char _Fill_n(uint8_t* _First, int32_t _Count, const uint8_t* _Val);

// A tree's collision shapes. Cpp-local in speedtree-2's SpeedTreeRT.cpp.
struct SCollisionObjects {
    // Transforms every shape's position by cTransform and scales its dimensions.
    void TransformAll(CTransform& cTransform);

    std::vector<SShape, std::allocator<SShape>> m_vObjects;
};

using SShapeVector = std::vector<SShape, std::allocator<SShape>>;

// The engine's last error, which SetError writes.
extern std::string g_strError;
