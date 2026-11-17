#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationSDM.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiActorManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherUpdateTask.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyMatch.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"

#include <stdint.h>

// Bytes no code reads or writes, between NiTAbstractPoolAllocator<NiActorManager::TimelineData>::ms_pkFreeMem and the
// 32-byte-aligned ms_kSpinLock of the same pool; likely the lock's alignment padding.
using TimelinePoolUnusedBytes = uint8_t[0x1C];
static TimelinePoolUnusedBytes data_11F3E84{};

// The timeline pool's _SDMShutdown: frees each block of ms_pkBlockHeader through the node's vector deleting destructor,
// then clears ms_pkFreeMem and ms_pkBlockHeader. The pool is never filled; no code but this function refers to its
// statics.
void ShutdownTimelineDataPool() {
    using Pool = NiTAbstractPoolAllocator<NiActorManager::TimelineData>;
    Pool::AllocNode* v0 = Pool::ms_pkBlockHeader;
    if (Pool::ms_pkBlockHeader) {
        Pool::AllocNode* m_pkNext;
        do {
            m_pkNext = v0->m_pkNext;
            delete[] v0;
            v0 = m_pkNext;
        }
        while (m_pkNext);
    }
    Pool::ms_pkFreeMem = nullptr;
    Pool::ms_pkBlockHeader = nullptr;
}

// The first instance registers the library's Init and Shutdown.
NiAnimationSDM::NiAnimationSDM() {
    if (!NiAnimationSDM::siCounter++) {
        NiStaticDataManager::AddLibrary(NiAnimationSDM::Init, NiAnimationSDM::Shutdown);
    }
}

void NiAnimationSDM::Init() {
    if (!NiAnimationSDM::ms_bInitialized) {
        NiAnimationSDM::ms_bInitialized = 1;
        NiStream::RegisterLoader("NiAlphaController", &NiAlphaController::CreateObject);
        NiStream::RegisterLoader("NiBlendAccumTransformInterpolator", &NiBlendAccumTransformInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBlendBoolInterpolator", &NiBlendBoolInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBlendColorInterpolator", &NiBlendColorInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBlendFloatInterpolator", &NiBlendFloatInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBlendPoint3Interpolator", &NiBlendPoint3Interpolator::CreateObject);
        NiStream::RegisterLoader("NiBlendQuaternionInterpolator", &NiBlendQuaternionInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBlendTransformInterpolator", &NiBlendTransformInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBoneLODController", &NiBoneLODController::CreateObject);
        NiStream::RegisterLoader("NiBoolData", &NiBoolData::CreateObject);
        NiStream::RegisterLoader("NiBoolInterpolator", &NiBoolInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBoolTimelineInterpolator", &NiBoolTimelineInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBSplineBasisData", &NiBSplineBasisData::CreateObject);
        NiStream::RegisterLoader("NiBSplineData", &NiBSplineData::CreateObject);
        NiStream::RegisterLoader("NiBSplineColorInterpolator", &NiBSplineColorInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBSplineCompColorInterpolator", &NiBSplineCompColorInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBSplineCompFloatInterpolator", &NiBSplineCompFloatInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBSplineCompPoint3Interpolator", &NiBSplineCompPoint3Interpolator::CreateObject);
        NiStream::RegisterLoader(
            "NiBSplineCompTransformInterpolator", &NiBSplineCompTransformInterpolator::CreateObject
        );
        NiStream::RegisterLoader("NiBSplineFloatInterpolator", &NiBSplineFloatInterpolator::CreateObject);
        NiStream::RegisterLoader("NiBSplinePoint3Interpolator", &NiBSplinePoint3Interpolator::CreateObject);
        NiStream::RegisterLoader("NiBSplineTransformInterpolator", &NiBSplineTransformInterpolator::CreateObject);
        NiStream::RegisterLoader("NiColorData", &NiColorData::CreateObject);
        NiStream::RegisterLoader("NiColorExtraDataController", &NiColorExtraDataController::CreateObject);
        NiStream::RegisterLoader("NiColorInterpolator", &NiColorInterpolator::CreateObject);
        NiStream::RegisterLoader("NiControllerManager", &NiControllerManager::CreateObject);
        NiStream::RegisterLoader("NiControllerSequence", &NiControllerSequence::CreateObject);
        NiStream::RegisterLoader("NiFlipController", &NiFlipController::CreateObject);
        NiStream::RegisterLoader("NiFloatData", &NiFloatData::CreateObject);
        NiStream::RegisterLoader("NiFloatExtraDataController", &NiFloatExtraDataController::CreateObject);
        NiStream::RegisterLoader("NiFloatInterpolator", &NiFloatInterpolator::CreateObject);
        NiStream::RegisterLoader("NiFloatsExtraDataController", &NiFloatsExtraDataController::CreateObject);
        NiStream::RegisterLoader("NiFloatsExtraDataPoint3Controller", &NiFloatsExtraDataPoint3Controller::CreateObject);
        NiStream::RegisterLoader("NiGeomMorpherController", &NiGeomMorpherController::CreateObject);
        NiStream::RegisterLoader("NiKeyframeController", &NiTransformController::CreateObject);
        NiStream::RegisterLoader("NiKeyframeData", &NiTransformData::CreateObject);
        NiStream::RegisterLoader("NiKeyframeManager", &NiKeyframeManager::CreateObject);
        NiStream::RegisterLoader("NiLightColorController", &NiLightColorController::CreateObject);
        NiStream::RegisterLoader("NiLightDimmerController", &NiLightDimmerController::CreateObject);
        NiStream::RegisterLoader("NiLookAtController", &NiLookAtController::CreateObject);
        NiStream::RegisterLoader("NiLookAtInterpolator", &NiLookAtInterpolator::CreateObject);
        NiStream::RegisterLoader("NiMaterialColorController", &NiMaterialColorController::CreateObject);
        NiStream::RegisterLoader("NiMorphData", &NiMorphData::CreateObject);
        NiStream::RegisterLoader("NiMultiTargetTransformController", &NiMultiTargetTransformController::CreateObject);
        NiStream::RegisterLoader("NiPathController", &NiPathController::CreateObject);
        NiStream::RegisterLoader("NiPathInterpolator", &NiPathInterpolator::CreateObject);
        NiStream::RegisterLoader("NiPoint3Interpolator", &NiPoint3Interpolator::CreateObject);
        NiStream::RegisterLoader("NiPosData", &NiPosData::CreateObject);
        NiStream::RegisterLoader("NiQuaternionInterpolator", &NiQuaternionInterpolator::CreateObject);
        NiStream::RegisterLoader("NiRollController", &NiRollController::CreateObject);
        NiStream::RegisterLoader("NiRotData", &NiRotData::CreateObject);
        NiStream::RegisterLoader("NiSequence", &NiSequence::CreateObject);
        NiStream::RegisterLoader("NiSequenceStreamHelper", &NiSequenceStreamHelper::CreateObject);
        NiStream::RegisterLoader("NiStringPalette", &NiStringPalette::CreateObject);
        NiStream::RegisterLoader("NiTextKeyExtraData", &NiTextKeyExtraData::CreateObject);
        NiStream::RegisterLoader("NiTextureTransformController", &NiTextureTransformController::CreateObject);
        NiStream::RegisterLoader("NiTransformController", &NiTransformController::CreateObject);
        NiStream::RegisterLoader("NiTransformData", &NiTransformData::CreateObject);
        NiStream::RegisterLoader("NiTransformInterpolator", &NiTransformInterpolator::CreateObject);
        NiStream::RegisterLoader("NiUVController", &NiUVController::CreateObject);
        NiStream::RegisterLoader("NiUVData", &NiUVData::CreateObject);
        NiStream::RegisterLoader("NiVisController", &NiVisController::CreateObject);
        NiStream::RegisterLoader("BSAnimNotes", &BSAnimNotes::CreateObject);
        NiStream::RegisterLoader("BSGrabIKNote", &BSGrabIKNote::CreateObject);
        NiStream::RegisterLoader("BSLookIKNote", &BSLookIKNote::CreateObject);
        NiStream::RegisterLoader("BSRotAccumTransfInterpolator", &BSRotAccumTransfInterpolator::CreateObject);
        NiStream::RegisterLoader("BSTreadTransfInterpolator", &BSTreadTransfInterpolator::CreateObject);
        NiStream::RegisterLoader("BSBlendTreadTransfInterpolator", &BSBlendTreadTransfInterpolator::CreateObject);
        NiStream::RegisterLoader(
            "BSMultiTargetTreadTransfController", &BSMultiTargetTreadTransfController::CreateObject
        );
        NiStream::RegisterLoader("BSFrustumFOVController", &BSFrustumFOVController::CreateObject);
        NiStream::RegisterLoader("NiVisData", &NiBoolData::CreateObject);
        NiStream::RegisterPostProcessFunction(&NiOldAnimationConverter::Convert);
        NiActorManager::_SDMInit();
    }
}

void NiAnimationSDM::Shutdown() {
    if (NiAnimationSDM::ms_bInitialized) {
        NiAnimationSDM::ms_bInitialized = 0;
        NiStream::UnregisterLoader("NiAlphaController");
        NiStream::UnregisterLoader("NiBlendAccumTransformInterpolator");
        NiStream::UnregisterLoader("NiBlendBoolInterpolator");
        NiStream::UnregisterLoader("NiBlendColorInterpolator");
        NiStream::UnregisterLoader("NiBlendFloatInterpolator");
        NiStream::UnregisterLoader("NiBlendPoint3Interpolator");
        NiStream::UnregisterLoader("NiBlendQuaternionInterpolator");
        NiStream::UnregisterLoader("NiBlendTransformInterpolator");
        NiStream::UnregisterLoader("NiBoneLODController");
        NiStream::UnregisterLoader("NiBoolData");
        NiStream::UnregisterLoader("NiBoolInterpolator");
        NiStream::UnregisterLoader("NiBoolTimelineInterpolator");
        NiStream::UnregisterLoader("NiBSplineBasisData");
        NiStream::UnregisterLoader("NiBSplineData");
        NiStream::UnregisterLoader("NiBSplineColorInterpolator");
        NiStream::UnregisterLoader("NiBSplineCompColorInterpolator");
        NiStream::UnregisterLoader("NiBSplineCompFloatInterpolator");
        NiStream::UnregisterLoader("NiBSplineCompPoint3Interpolator");
        NiStream::UnregisterLoader("NiBSplineCompTransformInterpolator");
        NiStream::UnregisterLoader("NiBSplineFloatInterpolator");
        NiStream::UnregisterLoader("NiBSplinePoint3Interpolator");
        NiStream::UnregisterLoader("NiBSplineTransformInterpolator");
        NiStream::UnregisterLoader("NiColorData");
        NiStream::UnregisterLoader("NiColorExtraDataController");
        NiStream::UnregisterLoader("NiColorInterpolator");
        NiStream::UnregisterLoader("NiControllerManager");
        NiStream::UnregisterLoader("NiControllerSequence");
        NiStream::UnregisterLoader("NiFlipController");
        NiStream::UnregisterLoader("NiFloatData");
        NiStream::UnregisterLoader("NiFloatExtraDataController");
        NiStream::UnregisterLoader("NiFloatInterpolator");
        NiStream::UnregisterLoader("NiFloatsExtraDataController");
        NiStream::UnregisterLoader("NiFloatsExtraDataPoint3Controller");
        NiStream::UnregisterLoader("NiGeomMorpherController");
        NiStream::UnregisterLoader("NiKeyframeController");
        NiStream::UnregisterLoader("NiKeyframeData");
        NiStream::UnregisterLoader("NiKeyframeManager");
        NiStream::UnregisterLoader("NiLightColorController");
        NiStream::UnregisterLoader("NiLightDimmerController");
        NiStream::UnregisterLoader("NiLookAtController");
        NiStream::UnregisterLoader("NiLookAtInterpolator");
        NiStream::UnregisterLoader("NiMaterialColorController");
        NiStream::UnregisterLoader("NiMorphData");
        NiStream::UnregisterLoader("NiMultiTargetTransformController");
        NiStream::UnregisterLoader("NiPathController");
        NiStream::UnregisterLoader("NiPathInterpolator");
        NiStream::UnregisterLoader("NiPoint3Interpolator");
        NiStream::UnregisterLoader("NiPosData");
        NiStream::UnregisterLoader("NiQuaternionInterpolator");
        NiStream::UnregisterLoader("NiRollController");
        NiStream::UnregisterLoader("NiRotData");
        NiStream::UnregisterLoader("NiSequence");
        NiStream::UnregisterLoader("NiSequenceStreamHelper");
        NiStream::UnregisterLoader("NiStringPalette");
        NiStream::UnregisterLoader("NiTextKeyExtraData");
        NiStream::UnregisterLoader("NiTextureTransformController");
        NiStream::UnregisterLoader("NiTransformController");
        NiStream::UnregisterLoader("NiTransformData");
        NiStream::UnregisterLoader("NiTransformInterpolator");
        NiStream::UnregisterLoader("NiUVController");
        NiStream::UnregisterLoader("NiUVData");
        NiStream::UnregisterLoader("NiVisController");
        NiStream::UnregisterLoader("BSAnimNotes");
        NiStream::UnregisterLoader("BSGrabIKNote");
        NiStream::UnregisterLoader("BSLookIKNote");
        NiStream::UnregisterLoader("BSRotAccumTransfInterpolator");
        NiStream::UnregisterLoader("BSTreadTransfInterpolator");
        NiStream::UnregisterLoader("BSBlendTreadTransfInterpolator");
        NiStream::UnregisterLoader("BSMultiTargetTreadTransfController");
        NiStream::UnregisterLoader("BSFrustumFOVController");
        NiStream::UnregisterLoader("NiVisData");
        NiStream::UnregisterPostProcessFunction(&NiOldAnimationConverter::Convert);
        ShutdownTimelineDataPool();
        NiActorManager::_SDMShutdown();
        NiGeomMorpherUpdateTask::_SDMShutdown();
    }
}

// The members' destructors then release m_kTextKeyMatched and m_spMatchObject.
NiActorManager::TimelineData::~TimelineData() {
    m_spMatchObject = nullptr;
}
