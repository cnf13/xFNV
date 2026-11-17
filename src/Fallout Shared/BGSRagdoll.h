#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"

#include <stdint.h>

class BGSBodyPartData;
class TESActorBase;

// The ragdoll's bone counts and enabled systems (DATA).
struct RagdollSaveStruct {
	// Swaps the four bone counts read from a big-endian file.
	void Endian();

	uint16_t usFeedbackDynamicBoneCount;
	uint16_t usLegCount;
	uint16_t usHeadCount;
	uint16_t usArmCount;
	bool bFeedbackEnabled;
	bool bFootIKEnabled;
	bool bLookIKEnabled;
	bool bGrabIKEnabled;
	bool bPoseMatchEnabled;
};

// The ragdoll's animation feedback settings and the bones it drives dynamically.
struct RagdollFeedbackData {
	// The feedback gains and limits (RAFD).
	struct RagdollFeedbackSystemData {
		RagdollFeedbackSystemData();

		// Swaps every field read from a big-endian file.
		void Endian();

		float fKeyDynIntrp;
		float fHeirGain;
		float fPosGain;
		float fVelGain;
		float fAccGain;
		float fSnapGain;
		float fVelDamp;
		float fSnapMaxLinVel;
		float fSnapMaxAngVel;
		float fSnapMaxLinDist;
		float fSnapMaxAngDist;
		float fPosMaxLinVel;
		float fPosMaxAngVel;
		uint32_t uiImpulseMult1;
		uint32_t uiImpulseMult2;
	};

	// Built through Init.
	RagdollFeedbackData();
	~RagdollFeedbackData();

	void Init();
	bool Compare(RagdollFeedbackData* a1);
	void Copy(const RagdollFeedbackData& arData);

	bool bEnabled;
	RagdollFeedbackSystemData SystemData;
	// The game builds and destroys it as BSSimpleArray<short, 1024>.
	BSSimpleArray<int16_t> DynamicBoneIdxA;
};

// The ragdoll's death pose matching settings.
struct PoseMatching_DATA {
	// The matched bones and the matching motor settings (RAPS).
	struct PoseMatching_System_DATA {
		PoseMatching_System_DATA();

		// Swaps the bones and the motor settings read from a big-endian file.
		void Endian();

		int16_t sRootMatchingBone;
		int16_t sMatchingBone2;
		int16_t sMatchingBone3;
		bool bDisableOnActivate;
		uint8_t padding07;
		float fMotorStrength;
		float fDelayTime;
		float fErrorAllowance;
		float fDisableDisplacement;
	};

	PoseMatching_DATA();
	~PoseMatching_DATA();

	bool Compare(PoseMatching_DATA* a1);
	void Init();
	void Copy(const PoseMatching_DATA& arData);

	bool bEnabled;
	BSStringT<char> PSAFilename;
	PoseMatching_System_DATA SystemData;
};

// The ragdoll's look IK settings: the names of the head systems and their data.
struct LookIK_DATA {
	// A head system's neck and head bones and look settings. The two vectors are written as float[3] because they are
	// never constructed or copied as NiPoint3; the constructor leaves them untouched.
	struct LookIK_Head_DATA {
		LookIK_Head_DATA();

		bool operator==(const LookIK_DATA::LookIK_Head_DATA& arData) const;
		bool operator!=(const LookIK_DATA::LookIK_Head_DATA& arData) const;

		int16_t sNeckIdx;
		int16_t sHeadIdx;
		float fForwardHeadLS[3];
		float fForwardNeckMS[3];
		float fLimitConeAngle;
		float fLookAtTargetGain;
		float fLookAtGain;
	};

	LookIK_DATA();

	char Compare(LookIK_DATA* a1);
	void Copy(const LookIK_DATA& arData);

	bool bEnabled;
	BSSimpleArray<const char*> HeadSystemsNameA;
	BSSimpleArray<LookIK_Head_DATA> HeadSystemsA;
};

// The ragdoll's grab IK settings: the names of the arm systems and their data.
struct GrabIK_DATA {
	// An arm system's shoulder, elbow and hand bones.
	struct GrabIK_Arm_DATA {
		GrabIK_Arm_DATA();

		bool operator==(const GrabIK_DATA::GrabIK_Arm_DATA& arData);
		bool operator!=(const GrabIK_DATA::GrabIK_Arm_DATA& arData);

		int16_t sShoulderIndex;
		int16_t sElbowIndex;
		int16_t sHandIndex;
	};

	GrabIK_DATA();

	char Compare(GrabIK_DATA* a1);
	void Copy(const GrabIK_DATA& arData);

	bool bEnabled;
	BSSimpleArray<const char*> ArmSystemsNameA;
	BSSimpleArray<GrabIK_Arm_DATA> ArmSystemsA;
};

// The ragdoll's foot IK settings: the gains and ray cast distances, and the leg systems.
struct FootIK_DATA {
	// The gains and ray cast distances shared by the legs.
	struct FootIKSystem_DATA {
		FootIKSystem_DATA();

		float fOnOffGain;
		float fGroundAscendingGain;
		float fGroundDescendingGain;
		float fFootRaisedGain;
		float fFootPlantedGain;
		float fRaycastDistUp;
		float fRaycastDistDown;
	};

	// A leg system's hip, knee and ankle bones and its ankle heights and knee limits. The two vectors are written as
	// float[3]: the constructor leaves them untouched.
	struct FootIK_Leg_DATA {
		FootIK_Leg_DATA();

		bool operator==(const FootIK_DATA::FootIK_Leg_DATA& arData);
		bool operator!=(const FootIK_DATA::FootIK_Leg_DATA& arData);

		int16_t sHipIndex;
		int16_t sKneeIndex;
		int16_t sAnkleIndex;
		bool bAlignFootToTerrain;
		float fMinAnkleHeightMS;
		float fMaxAnkleHeightMS;
		float fFootPlantedAnkleHeightMS;
		float fFootRaisedAnkleHeightMS;
		float fCosMaxKneeAngle;
		float fCosMinKneeAngle;
		float fKneeAxis[3];
		float fFootEndLocLS[3];
	};

	FootIK_DATA();
	~FootIK_DATA();

	char Compare(FootIK_DATA* a1);
	void Copy(const FootIK_DATA& arData);

	bool bEnabled;
	FootIKSystem_DATA SystemData;
	BSSimpleArray<const char*> LegSystemsNameA;
	BSSimpleArray<FootIK_Leg_DATA> LegSystemsA;
};

// A ragdoll form (RGDL): its save struct and the feedback, foot, look and grab IK and pose matching settings.
class BGSRagdoll : public TESForm, public TESModel {
public:
	BGSRagdoll();
	~BGSRagdoll() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// The feedback data.
	RagdollFeedbackData& QFeedbackData() { return FeedbackData; }

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	RagdollSaveStruct SaveStruct;
	uint8_t padding3E[2];
	RagdollFeedbackData FeedbackData;
	FootIK_DATA FootIKData;
	LookIK_DATA LookIKData;
	GrabIK_DATA GrabIKData;
	PoseMatching_DATA PoseMatchingData;
	uint32_t iSkeletonID;
	BGSBodyPartData* pBodyPartData;
	TESActorBase* pPreviewActor;
};
