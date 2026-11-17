#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"

// Node and animation names the game looks up by interned string, created once at start-up.
// Each static points at the NiFixedString the accessor hands out by reference.
class FixedStrings {
public:
	// The node name Bip01 Spine.
	static NiFixedString* GetBip01_Spine();
	// The laser-sight node name.
	static NiFixedString& QLaserSightsName();
	// The aim-sight node name.
	static NiFixedString& QAimSightsName();
	static NiFixedString& QPrn();
	static NiFixedString& QAttachLight();
	static NiFixedString& QGrass();
	static NiFixedString& QBip01Yaw();
	// The sequence name Unequip.
	static NiFixedString& QUnequip();
	static NiFixedString& QRaceHeadBiped();
	static NiFixedString& QRaceHeadSkinned();
	static NiFixedString& QHeadAnims();
	static NiFixedString& QBip();
	static NiFixedString& QBip01Looking();
	static NiFixedString& QCamera1st();
	static NiFixedString& QCamera3rd();
	static NiFixedString& QMagicNode();
	static NiFixedString& QBip01();
	// The node names of skinned and unskinned decal nodes.
	static NiFixedString& QSkinnedDecalNode();
	static NiFixedString& QDecalNode();
	// The sequence names Open and Close.
	static NiFixedString& QOpenString();
	static NiFixedString& QCloseString();
	static NiFixedString* GetBip01_Head();
	// Returns pBip01LForearm, the node name Bip01 L Forearm.
	static NiFixedString* QBip01LForearm();
	static NiFixedString* GetDecal();
	static NiFixedString* QBip01LUpperArm();
	static NiFixedString* GetPermanentDecal();
	static NiFixedString& QBip01Pitch();
	static NiFixedString* QPinnedLimb();
	static void InitSDM();
	static void KillSDM();
	static NiFixedString& QAttachSound();
	static NiFixedString& QArrowQuiver();
	static NiFixedString& QSpecialIdle_AreaEffect();
	static NiFixedString& QGrabLeft();
	static NiFixedString& QGrabRight();
	// Returns pBip01RUpperArm.
	static NiFixedString* QBip01RUpperArm();
	static NiFixedString& QBip01Neck();
	static NiFixedString& QBip01Spine1();
	static NiFixedString& QTalking();
	static NiFixedString& QArrowBone();
	static NiFixedString& QHeadMeshExp();
	// The node name Weapon.
	static NiFixedString& QWeapon();
	static NiFixedString* QSkin();

	static inline NiFixedString* pPrn;
	static inline NiFixedString* pAttachLight;
	static inline NiFixedString* pGrass;
	static inline NiFixedString* pBip01Yaw;
	static inline NiFixedString* pRaceHeadBiped;
	static inline NiFixedString* pRaceHeadSkinned;
	static inline NiFixedString* pHeadAnims;
	static inline NiFixedString* pBip;
	static inline NiFixedString* pBip01Looking;
	static inline NiFixedString* pCamera1st;
	static inline NiFixedString* pCamera3rd;
	static inline NiFixedString* pMagicNode;
	static inline NiFixedString* pBip01;
	static inline NiFixedString* AttachSound;
	static inline NiFixedString* ArrowQuiver;
	static inline NiFixedString* SpecialIdle_AreaEffect;
	static inline NiFixedString* pPermanentDecal;
	static inline NiFixedString* pBip01Pitch;
	static inline NiFixedString* Weapon;
	static inline NiFixedString* pGrabLeft;
	static inline NiFixedString* pBip01_Head;
	static inline NiFixedString* pBip01_Neck;
	static inline NiFixedString* Bip01_Spine1;
	static inline NiFixedString* pTalking;
	static inline NiFixedString* pArrowBone;
	static inline NiFixedString* pHeadMeshExp;
	static inline NiFixedString* pPinnedLimb;
	static inline NiFixedString* Bip01_Spine;
	static inline NiFixedString* pDecal;
	static inline NiFixedString* pGrabRight;
	static inline NiFixedString* SpecialIdle_Cast;
	static inline NiFixedString* Sound_Marker;
	static inline NiFixedString* Skinned_Decal_Node;
	static inline NiFixedString* Decal_Node;
	static inline NiFixedString* ModelSwapNode;
	static inline NiFixedString* Open;
	static inline NiFixedString* Close;
	static inline NiFixedString* EditorMarker;
	static inline NiFixedString* EditorMarker0;
	static inline NiFixedString* EditorMarker1;
	static inline NiFixedString* EditorMarker2;
	static inline NiFixedString* MarkerSource;
	static inline NiFixedString* MarkerTarget;
	static inline NiFixedString* pSkin;
	static inline NiFixedString* pFaceGenEars;
	static inline NiFixedString* pUnequip;
	static inline NiFixedString* pLaserSight;
	static inline NiFixedString* pAimSight;
	static inline NiFixedString* pArrow0;
	static inline NiFixedString* pEntryPoint;
	static inline NiFixedString* pBip01LUpperArm;
	static inline NiFixedString* pBip01LForearm;
	static inline NiFixedString* pBip01RUpperArm;
};
