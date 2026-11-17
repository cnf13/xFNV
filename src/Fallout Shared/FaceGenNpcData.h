#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout Shared/TESActorBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class BGSHeadPart;
class TESEyes;
class TESHair;
class TESTexture;

// Face-generation coordinates and the appearance resources used for an NPC.
class FaceGenNpcData {
public:
	FaceGenNpcData();
	~FaceGenNpcData();

	FR2MatrixVTC<float> FaceCoord[2][2];
	TESHair* pHair;
	int32_t iHairColor;
	float fHairLength;
	TESEyes* pEyes;
	TESActorBase::SEX eSex;
	NiTPrimitiveArray<TESModel*> pModel;
	NiTPrimitiveArray<TESTexture*> pTexture;
	NiTPrimitiveArray<char const*> pHeadPartNodeName;
	NiTObjectArray<NiPointer<NiTexture>> spTexBaseMod;
	bool bFaceGenTexturing;
	bool bPlayer;
	uint8_t paddingD6[2];
	TESModel* pLeftEye;
	TESModel* pRightEye;
	BS_FACEGEN_LOD eLod;
	BSSimpleList<BGSHeadPart*> Accessories;
};
