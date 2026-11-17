#pragma once

#include "BSMain/XRegSetting.h"
#include "Fallout Shared/TESFile.h"
#include "Reconstruction.hpp"

// INISettingCollection: 0xf425e2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fWaterGroupHeightRange_Water;

// INISettingCollection: 0xf4263a calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_uMaxInteriorWaterReflections_Water;

// INISettingCollection: 0xf42582 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fWadingWaterQuadSize_Water;

// INISettingCollection: 0xf4260a calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_uMaxExteriorWaterReflections_Water;

// INISettingCollection: 0xf42ac2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fRefractionWaterPlaneBias_Water;

// RendererSettingCollection: 0xf4242a calls 0x4a4a40.
extern SettingT<RendererSettingCollection> gs_iWaterBlurAmount_Water;

// RendererSettingCollection: 0xf423fa calls 0x4de2f0.
extern SettingT<RendererSettingCollection> gs_bUseWaterReflectionBlur_Water;

// INISettingCollection: 0xf4273a calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bUseBulletWaterDisplacements_Water;

// INISettingCollection: 0xf4254a calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bForceHighDetailLandReflections_Water;
