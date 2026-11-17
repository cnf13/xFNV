#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSStringT.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/FanLinCtlGrpS.h"
#include "Fallout Shared/FaceGen/FR2MatrixCTC.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSemaphore.h"
#include "SDK/MSVC/ehvec.h"

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

class Actor;
class BSFaceGenModelExtraData;
class BSFaceGenMorphDataHair;
class BSFaceGenNiNode;
class BSFile;
class FaceGenNpcData;
class FutBinaryFileC;
class NiCamera;
class NiGeometryDataIteratorNiPoint3;
class NiTriBasedGeom;
class QueuedFile;
class TESNPC;

enum IO_TASK_PRIORITY : int32_t;

// Which hair model a FaceGen file name is built for.
enum HAIR_PART : int32_t {
	HAIR_PART_NONE = -1,
	HAIR_PART_NORMAL = 0,
	HAIR_PART_HAT = 1,
	HAIR_PART_COUNT = 2,
};

// The head parts of a race; RACE_HAIR stands for the hair, which has a model per HAIR_PART.
enum RACE_HEAD_PART : int32_t {
	RHP_HEAD = 0,
	RHP_EAR = 1,
	RHP_MOUTH = 2,
	RHP_TEETH_LOWER = 3,
	RHP_TEETH_UPPER = 4,
	RHP_TONGUE = 5,
	RHP_EYE_LEFT = 6,
	RHP_EYE_RIGHT = 7,
	RACE_HEAD_PART_COUNT = 8,
	RACE_HAIR = 9,
};

// FaceGen's two coordinate spaces: geometry and texture.
enum FanTypeE : int32_t {
	FANTYPE_GEO = 0,
	FANTYPE_TEX = 1,
	FANTYPE_SIZE = 2,
};

// FaceGen's attributes: the first index of a control group's attr array (getAttribute).
enum FanAttribE : int32_t {
	FANATT_AGE = 0,
	FANATT_GENDER = 1,
	FANATT_SIZE = 2,
};

// FaceGen's symmetry: the second index of a control group's linear controls (see GetFaceCoordValue).
enum FanSymmE : int32_t {
	FANSYMM_SYMM = 0,
	FANSYMM_ASYM = 1,
	FANSYMM_SIZE = 2,
};

// FaceGen's race groups.
enum FanRaceE : int32_t {
	FANRACE_ALL = 0,
	FANRACE_AFRO = 1,
	FANRACE_ASIA = 2,
	FANRACE_EIND = 3,
	FANRACE_EURO = 4,
	FANRACE_SIZE = 5,
};

// The cache of loaded FaceGen models, by file name.
class BSFaceGenModelMap {
public:
	// The two caches of morph data: the geometry morphs (EGM) and the texture morphs (EGT), one cap each in
	// uiMaxDataSize. FreeLRUData switches on it: case 0 takes BSFaceGenModel::GetEGMDataSize, case 1 GetEGTDataSize.
	enum DataType : int32_t {
		DATA_TYPE_EGM = 0,
		DATA_TYPE_EGT = 1,
		NUM_DATA_TYPES = 2,
	};

	// A cached model, with the time it was last used.
	class Entry : public NiRefObject {
	public:
		Entry();
		~Entry() override;

		NiPointer<BSFaceGenModel> spModel;
		uint32_t uiTime;
	};

	BSFaceGenModelMap();
	virtual ~BSFaceGenModelMap();

	// Frees cached data until the entry's model data fits under each cap.
	void VerifyData(Entry* aspNewEntry);

	// Looks up apcKey; on a hit, holds the model and its mesh data in aspModel and
	// aspModelMeshData and returns true.
	bool GetAt(const char* apcKey, NiPointer<BSFaceGenModel>& aspModel,
		NiPointer<BSFaceGenModelMeshData>& aspModelMeshData);
	// Drops the texture data of the cached model apKey; true when it was cached.
	bool KillModelTextureData(const char* apKey);
	// Drops every cached model nothing else holds.
	void RemoveAllUnusedModels();
	// Unloads the texture (EGT) data of every cached model.
	void ClearEGTData();
	// Loads apModel's geometry morph data.
	bool LoadEGMData(BSFaceGenModel* apModel);
	// Loads apModel's texture morph data.
	bool LoadEGTData(BSFaceGenModel* apModel);
	// The cap on the cached data of type aDataType (0 the geometry morphs, 1 the texture morphs).
	uint32_t GetMaxDataSize(BSFaceGenModelMap::DataType aDataType);
	// Sets the cap on the cached data of type aDataType, then frees cached data down to it.
	void SetMaxDataSize(BSFaceGenModelMap::DataType aDataType, uint32_t auiSize);
	// The total size of the cached data of type aDataType (0 the geometry morphs, 1 the texture morphs).
	uint32_t GetTotalDataSize(BSFaceGenModelMap::DataType aDataType);
	// Unloads the texture (EGT) data of every cached model; abForce locks the access instead of trying to.
	void UnloadAllEGMAndEGTData(bool abForce);
	void OutputModelMapContents(BSFile* apFile);
	void SetAt(const char* apcKey, BSFaceGenModel* apModel);
	bool GetAt(const char* apcKey, NiPointer<BSFaceGenModel>& aspModel);
	// Unloads the least recently used model data of type aDataType, other than aspNewEntry's.
	void FreeLRUData(BSFaceGenModelMap::DataType aDataType, BSFaceGenModelMap::Entry* aspNewEntry);

	static int32_t GetCSEnterCounter();
	// Deleting frees through ourFree (BSCore/MemoryManager.h).
	static void operator delete(void* apMemory);

	BSTCaseInsensitiveStringMap<NiPointer<Entry>> Map;
	uint32_t uiMaxDataSize[2];
	bool bHasFaceGenData;
};

using BSTCaseInsensitiveStringMapFaceGenEntry = BSTCaseInsensitiveStringMap<NiPointer<BSFaceGenModelMap::Entry>>;
using NiTStringMapFaceGenEntry = NiTStringMap<NiPointer<BSFaceGenModelMap::Entry>>;

// FaceGen's linear control: a control matrix and its label.
struct FanLinCtlS {
	FanLinCtlS();
	// The copy constructor: the control matrix's copy, then the label's.
	FanLinCtlS(const FanLinCtlS& _Right);
	~FanLinCtlS();

	// Deleting frees through ourFree (BSCore/MemoryManager.h).
	static void operator delete(void* apMemory);

	FR2MatrixVTC<float> ctl;
	std::string label;
};

// FaceGen's face residual: an offset, a scale and four control matrices.
struct FanFgResidualS {
	~FanFgResidualS();

	// Deleting frees through ourFree (BSCore/MemoryManager.h).
	static void operator delete(void* apMemory);

	float resOffset;
	float resScale;
	FR2MatrixVTC<float> resMode[2][2];
};

// _Destroy_range(_First, _Last, _Al) of FanFgResidualS: passes the elements on with their category tag.
void DestroyRange(FanFgResidualS* _First, FanFgResidualS* _Last, std::allocator<FanFgResidualS>& _Al);
// _Destroy_range(_First, _Last, _Al, _Nonscalar_ptr_iterator_tag) of FanFgResidualS: destroys the elements one by one.
// _Cat is the tag's byte.
void DestroyRange(FanFgResidualS* _First, FanFgResidualS* _Last, std::allocator<FanFgResidualS>& _Al, uint8_t _Cat);
std::vector<float, std::allocator<float>>::const_iterator*
VectorIteratorPreIncrement(std::vector<float, std::allocator<float>>::const_iterator* apThis,
	[[maybe_unused]] void* apEdx);
float fsqr(float a1);
// Destroys the FanLinCtlS elements in [_First, _Last) one by one through the allocator's destroy. _Cat is the iterator
// category tag; the body does not read it.
void DestroyRange(FanLinCtlS* _First, FanLinCtlS* _Last, std::allocator<FanLinCtlS>& _Al, uint8_t _Cat);
// Destroys the element through DestroyFanLinCtl, which hands a1 to FanLinCtlS's scalar deleting destructor with flags
// 0, and returns that call's result.
FanLinCtlS* Destroy(FanLinCtlS* a1);
void* Destroy(void* a1);
// Destroys *apThis without freeing it, through its scalar deleting destructor with flags 0.
FanLinCtlS* DestroyFanLinCtl(FanLinCtlS* apThis);
// The vector's element destroyer Destroy calls it. It destroys apThis without freeing it.
FanFgResidualS* ExceptionPtrDestroy_0(FanFgResidualS* apThis);
FR2MatrixVTC<float>* ConcatRows(FR2MatrixVTC<float>* a1, FR2MatrixVTC<float>* hProcess, FR2MatrixVTC<float>* arg8);
// Assigns the controls [_First, _Last) to the ones from _Dest, each control's matrix then its label, and returns the
// end of the copy. None of the three tag bytes are read.
FanLinCtlS* CopyOpt(FanLinCtlS* _First, FanLinCtlS* _Last, FanLinCtlS* _Dest, uint8_t _IterCat, uint8_t _PtrCat,
	uint8_t _Checked);

// FanLinCtlGrpS::grp: one list of linear controls.
using FanLinCtlSVector = std::vector<FanLinCtlS, std::allocator<FanLinCtlS>>;

// FaceGen's distribution of one race: the means and the eigen bases, 0xA0 bytes (the constructor's element stride).
struct FanRaceDistS {
	FanRaceDistS();
	~FanRaceDistS();

	FR2MatrixVTC<float> mean[2];
	FR2MatrixVTC<float> eigBasis;
	FR2MatrixVTC<float> invEigBasis[2];
};

// FaceGen's offset linear control on two matrices (geometry and texture): 0x48 bytes. setRace scales by scale.
struct FanOffLinCtlS {
	FanOffLinCtlS();
	~FanOffLinCtlS();

	FR2MatrixVTC<float> ctl[2];
	float scale;
	float off;
};

// FaceGen's offset linear control on one matrix: 0x28 bytes. getAttribute adds off to the control's product.
struct FanPrtOffLinCtlS {
	FanPrtOffLinCtlS();
	~FanPrtOffLinCtlS();

	FR2MatrixVTC<float> ctl;
	float scale;
	float off;
};

// The FaceGen SDK's control groups (SDK\FaceGen\FanControls.cpp), 0x106C bytes in the manager. The layout is the
// constructor's: 5 race distributions, 20 attribute controls indexed [race][att][type] by getAttribute,
// 25 race-to-race controls indexed [race][race] by the race functions, 20 basis matrices and the 5 x 2 inverse
// attribute matrices setAttribute writes.
class FanGenCtlsGrpC {
public:
	FanGenCtlsGrpC();
	~FanGenCtlsGrpC();

	// Attribute att of type type for race, measured on coord.
	float getAttribute(FanRaceE race, FanAttribE att, FanTypeE type, const FR2MatrixVTC<float> (*coord)[2]);
	// Sets the type attributes of coord for race from val.
	void setAttributes(FanRaceE race, FanTypeE type, const float* val, FR2MatrixVTC<float> (*coord)[2]);
	bool readFromFile(FutBinaryFileC& file, uint32_t (*dims)[2]);
	// The caricature of coord's type and symmetry against race's distribution. The result is kept as a double. The
	// enums' int32_t base gives signed compares.
	double getCaricature(FanRaceE race, FanTypeE type, FanSymmE symm, const FR2MatrixVTC<float> (*coord)[2]);
	// Scales coord's type and symmetry to caricature val against race's distribution.
	void setCaricature(FanRaceE race, FanTypeE type, FanSymmE symm, float val, FR2MatrixVTC<float> (*coord)[2]);
	double getRace(int32_t race1, int32_t race2, const FR2MatrixVTC<float> (*coord)[2]);
	void setRace(int32_t race1, int32_t race2, float val, FR2MatrixVTC<float> (*coord)[2]);
	void random(int32_t a1, FR2MatrixVTC<float>* a2);
	// Sizes each of outCoord's 2 x 2 matrices as currCoord's and fills it with currCoord's values plus Gaussian noise
	// scaled by randomness. Both coordinates are indexed as FR2MatrixVTC<float>[2][2], reading currCoord and writing
	// outCoord.
	void geneticRandom(float randomness, const FR2MatrixVTC<float> (*currCoord)[2], FR2MatrixVTC<float> (*outCoord)[2]);
	void geneticRandom(float afGeneticVariation, FR2MatrixVTC<float> (*apSourceCoord)[2][2],
		FR2MatrixVTC<float> (*apOutCoord)[2][2]);
	// Reflects coord about race's mean: the symmetric part of each type becomes twice the mean less itself, the
	// asymmetric part its negation.
	void reflect(FanRaceE race, FR2MatrixVTC<float> (*coord)[2]);
	// Blends parent1 and parent2's type and symmetry by ratio into child, then sets child's caricature to the parents'
	// caricatures blended the same way, ratio clamped to [0, 1].
	void tween(FanTypeE type, FanSymmE symm, float ratio, const FR2MatrixVTC<float> (*parent1)[2],
		const FR2MatrixVTC<float> (*parent2)[2], FR2MatrixVTC<float> (*child)[2]);
	void setAttribute(FanRaceE race, FanAttribE att, FanTypeE type, const float val, FR2MatrixVTC<float> (*coord)[2]);

	bool initialized;
	FanRaceDistS distrib[5];
	FanPrtOffLinCtlS attr[5][2][2];
	FanOffLinCtlS race[5][5];
	FR2MatrixVTC<float> basis[20];
	// The 2 x 2 inverse of each attribute matrix, indexed [race][type]: setAttribute writes the 16-byte
	// invAttCov[race][type] at this + 0xFCC + race * 0x20 + type * 0x10 and readFromFile
	// clears and fills it.
	FR2MatrixCTC<float, 2, 2> invAttCov[5][2];
};

// The FaceGen SDK's per-face data.
struct FanFgFileDataS {
	FanFgFileDataS();
	~FanFgFileDataS();

	uint32_t egmKey;
	uint32_t egtKey;
	// The face coordinates, geometry first, then texture (CopyFaceGenCoord's abGeometryOnly copies coord[0]).
	FR2MatrixVTC<float> coord[2][2];
	std::vector<FanFgResidualS, std::allocator<FanFgResidualS>> residualCtrls;
	// The detail texture's JPEG bytes.
	std::vector<uint8_t, std::allocator<uint8_t>> detailTexJpeg;
};

// The FaceGen system: head morph data, the model cache and the FaceGen file names.
class BSFaceGenManager {
public:
	// The expression, modifier and phoneme names: 15 expressions "Anger".."CombatAnger", 17 modifiers
	// "BlinkLeft".."HeadYaw", 16 phonemes "Aah".."W". The pointer to "VampireMorph" after the modifiers is
	// pFaceGenCustomMorphText.
	using ExpressionTextArray = const char* [15];
	using ModifierTextArray = const char* [17];
	using PhonemeTextArray = const char* [16];

	BSFaceGenManager();
	~BSFaceGenManager();

	// apGeom's BSFaceGenModelExtraData, or null.
	static BSFaceGenModelExtraData* GetFaceGenModelExtraData(NiTriBasedGeom* apGeom);
	// The FaceGen model hung on apGeom, or null.
	static BSFaceGenModel* GetFaceGenModel(NiTriBasedGeom* apGeom);
	// Whether faces show emotions.
	static bool GetUseEmotions();
	// Queues the FaceGen head part files of apNPC under apParent.
	static void QueueHeadParts(TESNPC* apNPC, IO_TASK_PRIORITY aePriority, QueuedFile* apParent);
	// Sets up apNode's head geometry for the shaders from arNpcData.
	static void PrepareHeadForShaders(BSFaceGenNiNode* apNode, FaceGenNpcData& arNpcData);
	// Writes the detail texture name nearest acAge for apModel to aOutFilename and returns it, or null when there is
	// none.
	static const char* GetAsNearestDetailDDSFile(BSStringT<char>& aOutFilename, int32_t aeSex, uint8_t acAge,
		const char* apModel);
	// Sets the aeFanType attributes of apCoord from afValues (race 0). The coordinate parameters below and in the other
	// coord functions are an FR2MatrixVTC<float>[2][2]* written as a row pointer: the 2x2 arrays are inline
	// (FanFgFileDataS::coord), so both spell the same address.
	static void SetFaceCoordAttributes(FR2MatrixVTC<float> (*apCoord)[2], FanTypeE aeFanType, float* afValues);
	// Sets attribute aeFanAttribute of type aeFanType of apCoord to afValue, keeping the other attribute.
	static void SetFaceCoordAttribute(FR2MatrixVTC<float> (*apCoord)[2], FanAttribE aeFanAttribute, FanTypeE aeFanType,
		float afValue);
	// True when both coordinates are null or any of their matrices differ; false when only one is null or all match.
	static bool CompareFaceGenCoord(const FR2MatrixVTC<float> (*apCoord1)[2][2],
		const FR2MatrixVTC<float> (*apCoord2)[2][2]);
	// Deleting frees through ourFree (BSCore/MemoryManager.h).
	static void operator delete(void* apMemory);
	// Deletes the manager and clears pInstance.
	static void DestroySystem();
	// Sets aOutFilename to apModel's .egm file name for aeHairPart; returns the name, or null when apModel is null or
	// has no extension.
	static const char* GetAsEGMFile(BSStringT<char>& aOutFilename, const char* apModel, HAIR_PART aeHairPart);
	// Sets aOutFilename to apModel's .tri file name for aeHairPart; returns the name, or null when apModel is null or
	// has no extension.
	static const char* GetAsTRIFile(BSStringT<char>& aOutFilename, const char* apModel, HAIR_PART aeHairPart);
	// The manager's model cache; null when there is no manager.
	static BSFaceGenModelMap* GetModelCache();
	// The manager's current face coordinates (the whole coord member).
	static auto GetFaceGenCoord() -> FR2MatrixVTC<float> (*)[2][2];
	// Sizes apCoord's matrices for a face (50, 30 and 50 rows of one column).
	static void InitFaceGenCoord(FR2MatrixVTC<float> (*apCoord)[2][2]);
	// Copies apCopyFromCoord into apCopyToCoord, only the geometry or only the texture coordinates when asked.
	static void CopyFaceGenCoord(const FR2MatrixVTC<float> (*apCopyFromCoord)[2][2],
		FR2MatrixVTC<float> (*apCopyToCoord)[2][2], bool abGeometryOnly, bool abTextureOnly);
	// Merges apCopyFromCoord1 and apCopyFromCoord2 into apCopyToCoord, leaving the texture coordinates alone when
	// abNoTextureMerge; afClampVal limits the result.
	static void MergeFaceGenCoord(const FR2MatrixVTC<float> (*apCopyFromCoord1)[2][2],
		const FR2MatrixVTC<float> (*apCopyFromCoord2)[2][2], FR2MatrixVTC<float> (*apCopyToCoord)[2][2],
		bool abNoTextureMerge, float afClampVal);
	// Sets aOutFilename to apModel's .egt file name; returns the name, or null when apModel
	// is null.
	static const char* GetAsEGTFile(BSStringT<char>& aOutFilename, const char* apModel);
	// Whether FaceGen heads load on the background threads.
	static bool GetUseMultiThreading();
	// Writes the name of the hair node for aeHairPart into apHairName and returns it.
	static char* GetHairName(char* apHairName, HAIR_PART aeHairPart);
	// Attaches the hair models aNpcData selects to the biped and skinned head nodes.
	static void AttachHairToHead(BSFaceGenNiNode*& apOutNodeBiped, BSFaceGenNiNode*& apOutNodeSkinned,
		FaceGenNpcData& aNpcData, bool abForceNoSkinning);
	// Resets quantities to zero or the largest finite float.
	static void ResetQuantityArray(float* apQuantities, uint32_t aiNumQuantities, bool abForceZero);
	// Whether FaceGen heads are loaded at all.
	static bool GetUseFaceGenHeads();
	// The cached FaceGen model of apEGMFilename, created when abAllowCreate.
	static BSFaceGenModel* GetFaceGenModel(const char* apEGMFilename, const char* apNIFFilename,
		const char* apTRIFilename, const char* apEGTFilename, bool abAllowCreate, HAIR_PART aeHairPart,
		bool abIsAccessory);

	// The text of expression aeWhich.
	static const char* GetFaceGenExpressionText(BS_FACEGEN_EXPRESSION aeWhich);
	// The text of modifier aeWhich.
	static const char* GetFaceGenModifierText(BS_FACEGEN_MODIFIER aeWhich);
	// The text of phoneme aeWhich.
	static const char* GetFaceGenPhonemeText(BS_FACEGEN_PHONEME aeWhich);
	// The manager's default base modulation texture; null without a manager.
	static NiTexture* GetDefaultBaseModulationTexture();
	// The manager's hair and teeth alpha property; null without a manager.
	static NiAlphaProperty* GetHairTeethAlphaProperty();
	// Readies apObj's tri-based geometry for rendering: its data gets aiCompressFlags and becomes mutable, and an
	// unskinned geometry with a shader is precached with that shader's declaration.
	static void PrecacheFaceGeometry(NiAVObject* apObj, uint32_t aiShaderIndex, uint32_t aiCompressFlags);
	// The manager's default detail modulation texture; null without a manager.
	static NiTexture* GetDefaultDetailModulationTexture();
	// Whether eyes blink procedurally.
	static bool GetUseProceduralEyeBlinking();
	// The manager, created on first use.
	static BSFaceGenManager* GetInstance();
	// Creates the manager in pInstance when there is none.
	static void InitializeSystem();
	// Drops the cached models nothing holds.
	static void ClearModelCache();
	// Unloads the cached models' texture data.
	static void ClearEGTCache();
	// Loads a model's EGM data when both the manager and model map exist.
	static bool LoadEGMData(BSFaceGenModel* apModel);
	// Loads a model's EGT data when both the manager and model map exist.
	static bool LoadEGTData(BSFaceGenModel* apModel);
	// Takes the FaceGen lock when it is free and counts it; true when taken.
	static bool TryLockFaceGenAccess();
	// Takes the FaceGen lock and counts it; returns the new count.
	static int32_t LockFaceGenAccess();
	// Releases the FaceGen lock and uncounts it.
	static void UnlockFaceGenAccess();
	// The FaceGen expression (BS_FACEGEN_EXPRESSION) aeWhich shows, or -1.
	static BS_FACEGEN_EXPRESSION GetExpressionFromEmotion(DIALOGUE_EMOTION aeWhich);
	// FaceGen attribute aeFanAttribute of type aeFanType measured on apCoord (race 0).
	static float GetFaceCoordAttribute(const FR2MatrixVTC<float> (*apCoord)[2], FanAttribE aeFanAttribute,
		FanTypeE aeFanType);
	// Splits afPos - afNeg into its negative and positive parts.
	static void FixExclusiveValues(float afNeg, float afPos, float& afOutNeg, float& afOutPos);
	// The distance at which FaceGen heads switch to their LOD.
	static float GetLodDistance();
	// The LOD distance for talking heads.
	static float GetTalkingLodDistance();
	// The world camera's position, or the origin without one.
	static const NiPoint3& GetCameraPosition();
	// Sets the world camera.
	static void SetCamera(NiCamera* apCamera);
	// Whether FaceGen heads switch to their LOD models.
	static bool GetUseLOD();
	// Resets apGeom's vertices, walked by arIterator, to the base morph its extra data holds.
	static void RefreshMeshFromBaseMorphExtraData(NiTriBasedGeom* apGeom, NiGeometryDataIteratorNiPoint3& arIterator);
	// Steps apActor's face coordinates through the FaceGen values.
	static void CycleFaceGenCoordValue(Actor* apActor);
	static void SetUseProceduralEyeBlinking(bool abUse);
	static void SetToZero(float* apQuantities, uint32_t aiNumQuantities);
	static uint32_t GetEGTClampSize();
	static void SetUseEmotions(bool abUseEmotions);
	static bool GetFixNormals();
	static BSFaceGenMorphData* GetFaceGenMorphData(NiTriBasedGeom* apGeom);
	// The head morph data of apGeom's FaceGen model, or null.
	static BSFaceGenMorphDataHead* GetFaceGenMorphDataHead(NiTriBasedGeom* apGeom);
	// The hair morph data of apGeom's FaceGen model, or null.
	static BSFaceGenMorphDataHair* GetFaceGenMorphDataHair(NiTriBasedGeom* apGeom);
	// Applies apGeom's hair morph data to its base vertices, scaled by afModifierValue; nothing when the value is
	// negative or the geometry has no hair morph.
	static void ApplyHairMorph(NiTriBasedGeom* apGeom, float afModifierValue);
	static void ReplaceFaceMeshLOD(BSFaceGenNiNode* apNode, int32_t aDetail);
	static BS_FACEGEN_LOD GetForcedLod();
	static void ClearBodyTexturesFromPalette(TESNPC* apNPC);
	static NiSourceTexture* GetBaseTexture();
	static void OffsetFaceGenCoord(const FR2MatrixVTC<float> (*apCopyCoordOld)[2][2],
		const FR2MatrixVTC<float> (*apCopyCoordNew)[2][2], FR2MatrixVTC<float> (*apCopyToCoord)[2][2],
		bool abNoTextureOffset);
	static void ScaleFaceCoord(float afScaleAmount, FR2MatrixVTC<float> (*apCoordToScale)[2][2], bool abNoTextureScale);
	static const char* GetAsNIFFile(BSStringT<char>& aOutFilename, const char* apModel);
	static const char* GetAsNIFFileLowLOD(BSStringT<char>& aOutFilename, const char* apModel);
	static const char* GetAsNIFFileHighLOD(BSStringT<char>& aOutFilename, const char* apModel);
	static bool CreateFaceGenHead(BSFaceGenNiNode*& apOutNodeBiped, BSFaceGenNiNode*& apOutNodeSkinned,
		FaceGenNpcData& arNpcData, bool abForceNoSkinning);
	static void AttachEyesToHead(BSFaceGenNiNode*& apOutNodeBiped, BSFaceGenNiNode*& apOutNodeSkinned,
		FaceGenNpcData& aNpcData, bool abForceNoSkinning);
	static void ComputeModelBound(NiBound& arBound, NiGeometryDataIteratorNiPoint3& arIterator, int32_t aiVertices);
	static char* GetCustomMorphText(int32_t a1);
	static float GetFaceCoordValue(const FR2MatrixVTC<float> (*apCoord)[2][2], FanTypeE aeFanType, FanSymmE aeFanSymm,
		uint32_t aiCtrlID);
	static void SetFaceCoordValue(FR2MatrixVTC<float> (*apCoord)[2][2], FanTypeE aeFanType, FanSymmE aeFanSymm,
		uint32_t aiCtrlID, float afValue);
	static int32_t GetMaxControlID(int32_t a1, int32_t a2);
	static void RandomizeFaceGenetic(float afGeneticVariation, FR2MatrixVTC<float> (*apSourceCoord)[2][2],
		FR2MatrixVTC<float> (*apOutCoord)[2][2]);

	// The size of the manager is not pinned.
	FanFgFileDataS FgData;
	// The linear controls, built by the manager's constructor.
	FanLinCtlGrpS LinCtrls;
	// The FaceGen SDK controls, invAttCov included: their 0x106C bytes end at 0x1184.
	FanGenCtlsGrpC GenCtrls;
	// The FaceGen basis sizes fanReadCtlFile reads with the controls.
	uint32_t iBasisSize[2][2];
	BSFaceGenModelMap* pModelMap;
	// The alpha property of hair and teeth.
	NiPointer<NiAlphaProperty> spHairTeethAlphaProp;
	// The default base modulation texture.
	NiPointer<NiSourceTexture> spDefaultBaseMod;
	// The default detail modulation texture.
	NiPointer<NiSourceTexture> spDefaultDetailMod;

	static inline ExpressionTextArray pFaceGenExpressionText{"Anger", "Fear", "Happy", "Sad", "Surprise", "MoodNeutral",
		"MoodAfraid", "MoodAnnoyed", "MoodCocky", "MoodDrugged", "MoodPleasant", "MoodAngry", "MoodSad", "Pained",
		"CombatAnger"};
	static inline ModifierTextArray pFaceGenModifierText{"BlinkLeft", "BlinkRight", "BrowDownLeft", "BrowDownRight",
		"BrowInLeft", "BrowInRight", "BrowUpLeft", "BrowUpRight", "LookDown", "LookLeft", "LookRight", "LookUp",
		"SquintLeft", "SquintRight", "HeadPitch", "HeadRoll", "HeadYaw"};
	static inline PhonemeTextArray pFaceGenPhonemeText{"Aah", "BigAah", "BMP", "ChJSh", "DST", "Eee", "Eh", "FV", "I",
		"K", "N", "Oh", "OohQ", "R", "Th", "W"};
	static inline bool bEmotions = true;
	// The LOD every face is forced to; -1 forces none. GetForcedLod returns it as BS_FACEGEN_LOD.
	static inline int32_t iForcedLod = -1;
	static inline int32_t iLockCount;
	static inline bool bUseProceduralEyeBlinking;
	static inline BSFaceGenManager* pInstance;
	static inline NiCamera* pWorldCamera;
	// The face coordinate CycleFaceGenCoordValue steps; set by the ModifyFaceGen console command.
	static inline uint32_t iCycleCtrlID;
	// CycleFaceGenCoordValue steps the value up while it is set and turns it at +-10.
	static inline bool bCyclePositive;
	static inline char* pFaceGenCustomMorphText = const_cast<char*>("VampireMorph");
	static SettingT<INISettingCollection> bUseMultiThreadedFaceGen;
	static SettingT<INISettingCollection> bUseFaceGenHeads;
	// uiEGTClampSize:General: the size the EGT textures are clamped to.
	static SettingT<INISettingCollection> uiEGTClampSize;
	static SettingT<INISettingCollection> bUseFaceGenLOD;
	// bLoadFaceGenHeadEGTFiles:General: when on, NPCs get no face-mod textures.
	static SettingT<INISettingCollection> bLoadFaceGenHeadEGTFiles;
	// The lock on FaceGen data.
	static BSCriticalSection FGAccessCrit;
	static BSCriticalSection QueueCritical;
	// The semaphore the FaceGen loading waits on (built with a count and maximum of 1).
	static NiSemaphore LoadingSema;
	static SettingT<INISettingCollection> fLodDistance;
	static SettingT<INISettingCollection> fTalkingDistance;
	static SettingT<INISettingCollection> bUseEyeEnvMapping;
	static SettingT<INISettingCollection> bLoadBackgroundFaceGen;
};

// The hair part suffixes of the FaceGen file and node names ("NoHat", "Hat"), by HAIR_PART.
using HairPartNameArray = char* [HAIR_PART_COUNT];
inline HairPartNameArray pHairPartName{const_cast<char*>("NoHat"), const_cast<char*>("Hat")};

extern SettingT<INISettingCollection> gs_uiFaceGenMaxEGMDataSize_General;

extern SettingT<INISettingCollection> gs_uiFaceGenMaxEGTDataSize_General;
