#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSDM.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/BSStripPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/BSStripParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiMeshPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiMeshParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAgeDeathModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldAirFrictionCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldInheritVelocityCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldSpreadCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysBombModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysBoundUpdateModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysBoxEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysColliderManager.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysColorModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCylinderEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysDragFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysDragModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterCtlrData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterDeclinationCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterDeclinationVarCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterInitialRadiusCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterLifeSpanCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterPlanarAngleCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterPlanarAngleVarCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterSpeedCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldAttenuationCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldMagnitudeCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldMaxDistanceCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysGravityFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysGravityModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysGravityStrengthCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysGrowFadeModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysInitialRotAngleCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysInitialRotAngleVarCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysInitialRotSpeedCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysInitialRotSpeedVarCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysMeshEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysMeshUpdateModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierActiveCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysPlanarCollider.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysPositionModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysRadialFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysResetOnLoopCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysRotationModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSpawnModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSphereEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSphericalCollider.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysTurbulenceFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysUpdateCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysUpdateTask.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVortexFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"

NiParticleSDM::NiParticleSDM() {
    if (!siCounter++) {
        NiStaticDataManager::AddLibrary(&NiParticleSDM::Init, &NiParticleSDM::Shutdown);
    }
}

void NiParticleSDM::Init() {
    if (!NiParticleSDM::ms_bInitialized) {
        NiParticleSDM::ms_bInitialized = 1;
        NiStream::RegisterLoader("NiMeshParticleSystem", &NiMeshParticleSystem::CreateObject);
        NiStream::RegisterLoader("NiMeshPSysData", &NiMeshPSysData::CreateObject);
        NiStream::RegisterLoader("NiParticleSystem", &NiParticleSystem::CreateObject);
        NiStream::RegisterLoader("NiPSysAirFieldAirFrictionCtlr", &NiPSysAirFieldAirFrictionCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysAirFieldInheritVelocityCtlr", &NiPSysAirFieldInheritVelocityCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysAirFieldModifier", &NiPSysAirFieldModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysAirFieldSpreadCtlr", &NiPSysAirFieldSpreadCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysAgeDeathModifier", &NiPSysAgeDeathModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysBombModifier", &NiPSysBombModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysBoundUpdateModifier", &NiPSysBoundUpdateModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysBoxEmitter", &NiPSysBoxEmitter::CreateObject);
        NiStream::RegisterLoader("NiPSysColliderManager", &NiPSysColliderManager::CreateObject);
        NiStream::RegisterLoader("NiPSysColorModifier", &NiPSysColorModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysCylinderEmitter", &NiPSysCylinderEmitter::CreateObject);
        NiStream::RegisterLoader("NiPSysData", &NiPSysData::CreateObject);
        NiStream::RegisterLoader("NiPSysDragFieldModifier", &NiPSysDragFieldModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysDragModifier", &NiPSysDragModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysEmitterCtlr", &NiPSysEmitterCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysEmitterCtlrData", &NiPSysEmitterCtlrData::CreateObject);
        NiStream::RegisterLoader("NiPSysEmitterDeclinationCtlr", &NiPSysEmitterDeclinationCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysEmitterDeclinationVarCtlr", &NiPSysEmitterDeclinationVarCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysEmitterInitialRadiusCtlr", &NiPSysEmitterInitialRadiusCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysEmitterLifeSpanCtlr", &NiPSysEmitterLifeSpanCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysEmitterPlanarAngleCtlr", &NiPSysEmitterPlanarAngleCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysEmitterPlanarAngleVarCtlr", &NiPSysEmitterPlanarAngleVarCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysEmitterSpeedCtlr", &NiPSysEmitterSpeedCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysFieldAttenuationCtlr", &NiPSysFieldAttenuationCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysFieldMagnitudeCtlr", &NiPSysFieldMagnitudeCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysFieldMaxDistanceCtlr", &NiPSysFieldMaxDistanceCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysGravityModifier", &NiPSysGravityModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysGravityFieldModifier", &NiPSysGravityFieldModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysGravityStrengthCtlr", &NiPSysGravityStrengthCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysGrowFadeModifier", &NiPSysGrowFadeModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysInitialRotAngleCtlr", &NiPSysInitialRotAngleCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysInitialRotAngleVarCtlr", &NiPSysInitialRotAngleVarCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysInitialRotSpeedCtlr", &NiPSysInitialRotSpeedCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysInitialRotSpeedVarCtlr", &NiPSysInitialRotSpeedVarCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysMeshEmitter", &NiPSysMeshEmitter::CreateObject);
        NiStream::RegisterLoader("NiPSysMeshUpdateModifier", &NiPSysMeshUpdateModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysModifierActiveCtlr", &NiPSysModifierActiveCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysPlanarCollider", &NiPSysPlanarCollider::CreateObject);
        NiStream::RegisterLoader("NiPSysPositionModifier", &NiPSysPositionModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysRadialFieldModifier", &NiPSysRadialFieldModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysResetOnLoopCtlr", &NiPSysResetOnLoopCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysRotationModifier", &NiPSysRotationModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysSpawnModifier", &NiPSysSpawnModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysSphereEmitter", &NiPSysSphereEmitter::CreateObject);
        NiStream::RegisterLoader("NiPSysSphericalCollider", &NiPSysSphericalCollider::CreateObject);
        NiStream::RegisterLoader("NiPSysTurbulenceFieldModifier", &NiPSysTurbulenceFieldModifier::CreateObject);
        NiStream::RegisterLoader("NiPSysUpdateCtlr", &NiPSysUpdateCtlr::CreateObject);
        NiStream::RegisterLoader("NiPSysVortexFieldModifier", &NiPSysVortexFieldModifier::CreateObject);
        NiStream::RegisterLoader("BSStripParticleSystem", &BSStripParticleSystem::CreateObject);
        NiStream::RegisterLoader("BSStripPSysData", &BSStripPSysData::CreateObject);
        NiParticleSystem::iThreadID = GetCurrentThreadId();
    }
}

// The task pool goes last: NiPSysUpdateTask::_SDMShutdown runs after the loaders are unregistered.
void NiParticleSDM::Shutdown() {
    if (NiParticleSDM::ms_bInitialized) {
        NiParticleSDM::ms_bInitialized = 0;
        NiStream::UnregisterLoader("NiMeshParticleSystem");
        NiStream::UnregisterLoader("NiMeshPSysData");
        NiStream::UnregisterLoader("NiParticleSystem");
        NiStream::UnregisterLoader("NiPSysAirFieldAirFrictionCtlr");
        NiStream::UnregisterLoader("NiPSysAirFieldInheritVelocityCtlr");
        NiStream::UnregisterLoader("NiPSysAirFieldModifier");
        NiStream::UnregisterLoader("NiPSysAirFieldSpreadCtlr");
        NiStream::UnregisterLoader("NiPSysAgeDeathModifier");
        NiStream::UnregisterLoader("NiPSysBombModifier");
        NiStream::UnregisterLoader("NiPSysBoundUpdateModifier");
        NiStream::UnregisterLoader("NiPSysBoxEmitter");
        NiStream::UnregisterLoader("NiPSysColliderManager");
        NiStream::UnregisterLoader("NiPSysColorModifier");
        NiStream::UnregisterLoader("NiPSysCylinderEmitter");
        NiStream::UnregisterLoader("NiPSysData");
        NiStream::UnregisterLoader("NiPSysDragFieldModifier");
        NiStream::UnregisterLoader("NiPSysDragModifier");
        NiStream::UnregisterLoader("NiPSysEmitterCtlr");
        NiStream::UnregisterLoader("NiPSysEmitterCtlrData");
        NiStream::UnregisterLoader("NiPSysEmitterDeclinationCtlr");
        NiStream::UnregisterLoader("NiPSysEmitterDeclinationVarCtlr");
        NiStream::UnregisterLoader("NiPSysEmitterInitialRadiusCtlr");
        NiStream::UnregisterLoader("NiPSysEmitterLifeSpanCtlr");
        NiStream::UnregisterLoader("NiPSysEmitterPlanarAngleCtlr");
        NiStream::UnregisterLoader("NiPSysEmitterPlanarAngleVarCtlr");
        NiStream::UnregisterLoader("NiPSysEmitterSpeedCtlr");
        NiStream::UnregisterLoader("NiPSysFieldAttenuationCtlr");
        NiStream::UnregisterLoader("NiPSysFieldMagnitudeCtlr");
        NiStream::UnregisterLoader("NiPSysFieldMaxDistanceCtlr");
        NiStream::UnregisterLoader("NiPSysGravityModifier");
        NiStream::UnregisterLoader("NiPSysGravityFieldModifier");
        NiStream::UnregisterLoader("NiPSysGravityStrengthCtlr");
        NiStream::UnregisterLoader("NiPSysGrowFadeModifier");
        NiStream::UnregisterLoader("NiPSysInitialRotAngleCtlr");
        NiStream::UnregisterLoader("NiPSysInitialRotAngleVarCtlr");
        NiStream::UnregisterLoader("NiPSysInitialRotSpeedCtlr");
        NiStream::UnregisterLoader("NiPSysInitialRotSpeedVarCtlr");
        NiStream::UnregisterLoader("NiPSysMeshEmitter");
        NiStream::UnregisterLoader("NiPSysMeshUpdateModifier");
        NiStream::UnregisterLoader("NiPSysModifierActiveCtlr");
        NiStream::UnregisterLoader("NiPSysPlanarCollider");
        NiStream::UnregisterLoader("NiPSysPositionModifier");
        NiStream::UnregisterLoader("NiPSysRadialFieldModifier");
        NiStream::UnregisterLoader("NiPSysResetOnLoopCtlr");
        NiStream::UnregisterLoader("NiPSysRotationModifier");
        NiStream::UnregisterLoader("NiPSysSpawnModifier");
        NiStream::UnregisterLoader("NiPSysSphereEmitter");
        NiStream::UnregisterLoader("NiPSysSphericalCollider");
        NiStream::UnregisterLoader("NiPSysTurbulenceFieldModifier");
        NiStream::UnregisterLoader("NiPSysUpdateCtlr");
        NiStream::UnregisterLoader("NiPSysVortexFieldModifier");
        NiPSysUpdateTask::_SDMShutdown();
    }
}
