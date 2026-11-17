#include "Fallout/AI/FixedStrings.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"

NiFixedString* FixedStrings::GetBip01_Spine() {
	return reinterpret_cast<
		NiFixedString*>(static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(FixedStrings::Bip01_Spine)));
}

NiFixedString& FixedStrings::QPrn() {
	return *pPrn;
}

NiFixedString& FixedStrings::QAttachLight() {
	return *pAttachLight;
}

NiFixedString& FixedStrings::QGrass() {
	return *pGrass;
}

NiFixedString& FixedStrings::QBip01Yaw() {
	return *pBip01Yaw;
}

NiFixedString& FixedStrings::QRaceHeadBiped() {
	return *pRaceHeadBiped;
}

NiFixedString& FixedStrings::QRaceHeadSkinned() {
	return *pRaceHeadSkinned;
}

NiFixedString& FixedStrings::QHeadAnims() {
	return *pHeadAnims;
}

NiFixedString& FixedStrings::QBip() {
	return *pBip;
}

NiFixedString& FixedStrings::QBip01Looking() {
	return *pBip01Looking;
}

NiFixedString& FixedStrings::QCamera1st() {
	return *pCamera1st;
}

NiFixedString& FixedStrings::QCamera3rd() {
	return *pCamera3rd;
}

NiFixedString& FixedStrings::QMagicNode() {
	return *pMagicNode;
}

NiFixedString* FixedStrings::GetBip01_Head() {
	return FixedStrings::pBip01_Head;
}

NiFixedString* FixedStrings::QBip01LForearm() {
	return FixedStrings::pBip01LForearm;
}

NiFixedString* FixedStrings::GetDecal() {
	return FixedStrings::pDecal;
}

NiFixedString* FixedStrings::QBip01LUpperArm() {
	return FixedStrings::pBip01LUpperArm;
}

NiFixedString* FixedStrings::GetPermanentDecal() {
	return FixedStrings::pPermanentDecal;
}

NiFixedString* FixedStrings::QPinnedLimb() {
	return FixedStrings::pPinnedLimb;
}

NiFixedString* FixedStrings::QBip01RUpperArm() {
	return FixedStrings::pBip01RUpperArm;
}

NiFixedString& FixedStrings::QWeapon() {
	return *Weapon;
}
