#pragma once

#include "Fallout Shared/TESBipedModelForm.h"

// Sets the camera shake strength afMult and the time it has run, afCurrentTime.
void SetCameraShakeData(float afMult, float afCurrentTime);
void ShakeCamera(float afMult, const NiPoint3& arPos);
float GetCameraShake(float afTime, NiMatrix3& apReturnShake);
float GetCameraShakeMult();
float GetCameraShakeCurrentTime();

inline NiNode* ShakeData_pModel;

inline float fCameraShakeMult;

inline float fCameraShakeCurrentTime;
