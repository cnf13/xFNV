#pragma once

// The classes whose CreateObject the lines below name (none of these headers includes this one).
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSBlendTreadTransfInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSFrustumFOVController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSGrabIKNote.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSLookIKNote.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSMultiTargetTreadTransfController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSRotAccumTransfInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSTreadTransfInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAlphaController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendAccumTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendQuaternionInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoneLODController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolTimelineInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineBasisData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineCompColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineCompFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineCompPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineCompTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplinePoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorExtraDataController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFlipController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatExtraDataController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatsExtraDataController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatsExtraDataPoint3Controller.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyframeManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLightColorController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLightDimmerController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLookAtController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLookAtInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMaterialColorController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMorphData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiOldAnimationConverter.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPathController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPathInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuaternionInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRollController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequenceStreamHelper.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStringPalette.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextureTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiUVController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiUVData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiVisController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"

#include <stdint.h>

void ShutdownTimelineDataPool();

// The animation library's static initialization and shutdown interface; it has no instance data.
class NiAnimationSDM {
public:
	NiAnimationSDM();

	static void Init();
	static void Shutdown();

	static inline bool ms_bInitialized;
	static inline int32_t siCounter;
};

// NiActorManager::_SDMInit's "start": its NiFixedString assignment compares the handle with this address.
using aStart_1_t = const char[6];
inline aStart_1_t aStart_1{"start"};

// NiActorManager::_SDMInit's "end", compared by address as aStart_1 is.
using aEnd_t = const char[4];
inline aEnd_t aEnd{"end"};

// The library's static object: one per file that includes this header; its constructor runs NiAnimationSDM's Init the
// first time.
static NiAnimationSDM NiAnimationSDMObject{};
