#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

class Actor;
class BGSDebris;
class BGSExplosion;
class QueuedFile;
class TESBoundObject;
class TESModel;
class TESModelTextureSwap;
class TESObjectREFR;

enum IO_TASK_PRIORITY : int32_t;

float RandomFloat(float afMin, float afMax);
NiFixedString* get_g_ModelSwapNode();

// The references taking destruction damage.
using DestructibleRefsTakingDamageMap = NiTMap<TESObjectREFR*, uint32_t>;

// One destruction stage as a file stores it (DSTD): health percentage, stage number, model stage and flags, then the
// damage per second and the explosion, debris and debris count.
struct SAVE_DESTRUCTIBLE_OBJECT_STAGE {
	// Byte-swaps the four 32-bit fields read from a big-endian file.
	void Endian();

	uint8_t cHealthPercentage;
	uint8_t cStageNum;
	uint8_t cModelDamageStage;
	uint8_t cFlags;
	int32_t iSelfDamagePerSecond;
	int32_t iExplosionID;
	int32_t iDebrisID;
	int32_t iDebrisCount;
};

// The destruction data as a file stores it (DEST): the health, the stage count and the flags (8 bytes; the two
// trailing bytes are padding the constructor leaves unwritten).
struct SAVE_DESTRUCTIBLE_OBJECT_DATA {
	SAVE_DESTRUCTIBLE_OBJECT_DATA();

	void Endian();

	uint32_t iHealth;
	uint8_t cNumStages;
	uint8_t cFlags;
};

// One destruction stage of a form.
class DestructibleObjectStage {
public:
	// The bits of cFlags.
	enum DESTRUCTIBLE_OBJECT_STAGE_FLAGS : uint8_t {
		FLAG_CAP_DAMAGE = 0x1,
		FLAG_DISABLE_OBJECT = 0x2,
		FLAG_DESTROY_OBJECT = 0x4,
	};

	DestructibleObjectStage();
	~DestructibleObjectStage();

	bool GetDisableObject();
	bool GetCapDamage();
	bool GetDestroyObject();
	// Copies apCopy into this stage.
	void Copy(DestructibleObjectStage* apCopy);
	void CreateReplacementModel();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint8_t cModelDamageStage;
	uint8_t cHealthPercentage;
	uint8_t cFlags;
	int32_t iSelfDamagePerSecond;
	BGSExplosion* pExplosion;
	BGSDebris* pDebris;
	int32_t iDebrisCount;
	TESModelTextureSwap* pReplacementModel;
};

// The destruction data of a form (allocated with ourMalloc, 0x14 bytes).
struct DestructibleObjectData {
	DestructibleObjectData();
	~DestructibleObjectData();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint32_t iHealth;
	uint8_t cNumStages;
	uint8_t cFlags;
	DestructibleObjectStage** pStagesArray;
	int32_t iReplacementModelRefCount;
	// The queued load of the replacement models.
	NiPointer<QueuedFile> spPreloadedReplacementModels;
};

// The destruction stages of a form.
class BGSDestructibleObjectForm : public BaseFormComponent {
public:
	BGSDestructibleObjectForm();
	~BGSDestructibleObjectForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override;
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// Collects the model swap nodes below apObject into apNodes.
	void FindModelSwapNodes(NiAVObject* apRoot, BSSimpleArray<NiNode*, 1024>& arNodeArray);
	// Moves apRef to the damage stage its health calls for; whether it changed.
	// The callee reads abForceUpdate as a byte.
	bool UpdateCurrentDamageStage(TESObjectREFR* apRef, bool abForceUpdate);
	// Applies afDamage to apRef: moves it through the destruction stages its health crosses.
	void DamageObject(TESObjectREFR* apRef, float afDamage, bool abSelfDamage);
	// The maximum health of apRef.
	uint32_t GetMaxHealth(TESObjectREFR* apRef);
	// Spawns apExplosion at the blast nodes of stage aiStageIndex below apRoot; nonzero when
	// it found any. The callee reads aiStageIndex as a byte.
	uint32_t SpawnExplosionsAtBlastNodes(NiAVObject* apRoot, BGSExplosion* apExplosion, uint32_t aiStageIndex,
		TESObjectREFR* apRef);
	// Spawns apDebris at the debris nodes of stage aiStageIndex below apRoot; nonzero when it
	// found any. The callee reads aiStageIndex as a byte.
	uint32_t SpawnDebrisAtDebrisNodes(NiAVObject* apRoot, BGSDebris* apDebris, uint32_t aiStageIndex,
		TESObjectREFR* apRef, uint32_t aiNumParticles);
	// The health damage of the dangerous explosion of the stages below acCurrentHealthPercentage, -1 when there is
	// none; its explosion goes to apOutExplosion.
	float CalculateDangerousExplosionHealth(uint8_t acCurrentHealthPercentage, float afRequiredRadius,
		float afRequiredDamage, BGSExplosion** apOutExplosion);
	// Resolves the forms loaded for apOwner.
	void InitItem(TESForm* apOwner);
	// (KEEP, see the body). Writes the DEST chunk and per stage the DSTD chunk, the replacement model chunks and the
	// DSTF marker to the form being saved.
	void Save();
	// Whether any destruction stage swaps the model, explodes, spawns debris or disables.
	bool HasValidStages();
	// Queues the files of the destruction stages for loading.
	void QueueFiles(TESBoundObject* apObject, IO_TASK_PRIORITY aePriority, QueuedFile* apParent);
	// Allocates pData when it is missing.
	void CreateData();
	DestructibleObjectData* GetData();
	// Allocates acStagesCount stages.
	DestructibleObjectStage** AllocateStages(uint8_t acStagesCount);
	// Puts apRef in its last destruction stage.
	void ApplyTerminalDamage(TESObjectREFR* apRef);
	// Releases the preloaded replacement models of the destruction stages.
	void UnloadReplacementModels();
	// Preloads the replacement models of apObject's destruction stages.
	void PreloadReplacementModels(TESBoundObject* apObject);
	DestructibleObjectStage* GetStage(uint32_t aiIndex);
	float CalculateDangerousExplosionTerminalTime(uint8_t acCurrentHealthPercentage, float afRequiredRadius,
		float afRequiredDamage, BGSExplosion** apOutExplosion);
	bool GetTargetableInVATS();

	// Hides the small debris below apNode.
	static bool HideSmallDebris(NiAVObject* ap3D);
	// Steps the destruction of the objects being destroyed.
	static void UpdateDestructibleObjects();
	// Shows the damage stage aiStageIndex's nodes below apRoot. The callee
	// reads aiStageIndex as a byte.
	static void UpdateDamageStageNodes(NiAVObject* apRoot, uint32_t aiStageIndex);
	// The destruction stage apRef is in.
	static int32_t CalcDestructionStage(TESObjectREFR* apRef);
	// Clears the destruction state of apRef.
	static void ClearDestruction(TESObjectREFR* apRef);
	// Reads a destruction chunk of apFile into apDestruct.
	// KEEP: its DMDL and DMDS branches read the chunk into an __alloca_probe_16 buffer; src cannot place it where the
	// exe's frame has it (see TESModelTextureSwap::LoadModelTextureSwapChunk 0x48A990).
	static void LoadChunk(BGSDestructibleObjectForm* apDestruct, TESFile* apFile);
	// Stores the time of the last destruction update (body in Fallout/Misc/Main.cpp).
	static void SetLastUpdateTime(float afTime);
	// The destructible component of apForm, else null.
	static BGSDestructibleObjectForm* GetDestructionForm(TESForm* apForm);
	// Whether apForm has a destruction stage that does anything.
	static bool IsDestructible(TESForm* apForm);
	static int32_t GetModelSwapIndex(TESForm* apForm, TESModel* apModelSwap);
	static TESModel* GetModelSwap(TESForm* apForm, int32_t aiModelSwapIndex);
	static void RemoveDestructibleObject(TESObjectREFR* apRef);
	static uint32_t GetSelfDamage(TESObjectREFR* apRef, float afHealth);
	static void SetSelfDamage(TESObjectREFR* apRef, uint32_t aiSelfDamage);
	// Returns the address of DestructibleObjects.
	static DestructibleRefsTakingDamageMap* GetDestructibleObjects();

	DestructibleObjectData* pData;

	static inline float fLastTime;
	static DestructibleRefsTakingDamageMap DestructibleObjects;
};
