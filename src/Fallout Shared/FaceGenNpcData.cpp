#include "Fallout Shared/FaceGenNpcData.h"

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BGSHeadPart.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "SDK/MSVC/ehvec.h"

FaceGenNpcData::FaceGenNpcData()
  : pHair(nullptr), iHairColor(0), fHairLength(0.0f), pEyes(nullptr), eSex(TESActorBase::SEX_MALE), pModel(0, 1),
    pTexture(0, 1), pHeadPartNodeName(0, 1), spTexBaseMod(0, 1), bFaceGenTexturing(false), bPlayer(false),
    eLod(BS_FACEGEN_LOD_NONE) {
    pModel.RemoveAll();
    pTexture.RemoveAll();
    pHeadPartNodeName.RemoveAll();
}

FaceGenNpcData::~FaceGenNpcData() {
    Accessories.RemoveAll();
}
