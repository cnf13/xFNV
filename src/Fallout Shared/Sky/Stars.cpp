#include "Fallout Shared/Sky/Stars.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSStream.h"
#include "BSMain/Setting.h"
#include "BSShader/SkyShaderProperty.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyObject.h"
#include "Fallout/Misc/Calendar.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <cmath>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> fStarsRotateDays("fStarsRotateDays", 4.0f);
SettingT<GameSettingCollection> fStarsRotateXAxis("fStarsRotateXAxis", 0.0f);
SettingT<GameSettingCollection> fStarsRotateYAxis("fStarsRotateYAxis", 0.0f);
SettingT<GameSettingCollection> fStarsRotateZAxis("fStarsRotateZAxis", 1.0f);

Stars::Stars() : spStars(nullptr), fAlpha(0.0f) {}

Stars::~Stars() {}

// The engine's SkyObject::Initialize has SEH and creates the sky root object.
void Stars::Initialize(NiNode* apSkyRoot) {
    SkyObject::Initialize(apSkyRoot);
}

void Stars::Update(Sky* apSky, float afSecondsPassed) {
    if (apSky->pCurrentWeather) {
        float fHour = apSky->GetCurrentGameHour();
        fAlpha = 0.0f;
        static NiPoint3 colorLastFrame(0.0f, 0.0f, 0.0f);
        SkyObject::Update(apSky, afSecondsPassed);
        if (apSky->eMode == Sky::SM_FULL || apSky->eMode == Sky::SM_FAKEEXTERIOR) {
            float fSunsetBegin;
            float fBeginFadeIn;
            float fSunriseBegin;
            float fSunsetEnd;
            float fEndFadeOut;
            float fSunriseEnd;
            if (spRoot->GetDisplayObject() &&
                ((fSunsetBegin = apSky->GetSunsetBegin(),
                  (fSunsetEnd = apSky->GetEndSunsetColors(),
                   (fSunriseEnd = apSky->GetSunriseEnd(),
                    (fSunriseBegin = apSky->GetBeginSunriseColors(),
                     (fBeginFadeIn = fSunsetEnd - (fSunsetEnd - fSunsetBegin) * 0.5,
                      (fEndFadeOut = fSunriseEnd - (fSunriseEnd - fSunriseBegin) * 0.5,
                       !(fHour > static_cast<double>(fSunriseBegin)))))))) ||
                         !(fEndFadeOut > static_cast<double>(fHour))
                     ? !(fHour >= static_cast<double>(fEndFadeOut)) || !(fBeginFadeIn >= static_cast<double>(fHour))
                         ? !(fHour > static_cast<double>(fBeginFadeIn)) || !(fSunsetEnd > static_cast<double>(fHour))
                             ? fAlpha = 1.0f
                             : fAlpha = (fHour - fBeginFadeIn) / (fSunsetEnd - fBeginFadeIn)
                         : fAlpha = 0.0f
                     : fAlpha = (fEndFadeOut - fHour) / (fEndFadeOut - fSunriseBegin),
                 fAlpha > 0.0)) {
                uint32_t uiCount;
                NiPoint3 pt;
                spRoot->SetAppCulled(false);
                // SkyColor[6] is the stars' color; its three channels are copied into pt.
                const NiColor& kStarsColor = apSky->SkyColor[6];
                pt.x = kStarsColor.r;
                pt.y = kStarsColor.g;
                pt.z = kStarsColor.b;
                if (spStars.m_pObject) {
                    uiCount = static_cast<NiNode*>(spStars.m_pObject)->NiNode::GetChildCount();
                }
                else {
                    uiCount = 0;
                }
                for (uint32_t i = 0; i < uiCount; ++i) {
                    NiAVObject* pChild = static_cast<NiNode*>(spStars.m_pObject)->NiNode::GetAt(i);
                    if (pChild) {
                        NiGeometry* pStarsGeo = pChild->IsGeometry();
                        if (pStarsGeo) {
                            SkyShaderProperty* pStarsProp;
                            int32_t iIsSkyShader;
                            if (pStarsGeo->GetProperty(NiProperty::SHADE)) {
                                pStarsProp = static_cast<SkyShaderProperty*>(pStarsGeo->GetProperty(NiProperty::SHADE));
                                iIsSkyShader = pStarsProp->iShaderPropertyType == 13;
                            }
                            else {
                                iIsSkyShader = 0;
                            }
                            if (iIsSkyShader) {
                                pStarsProp = static_cast<SkyShaderProperty*>(pStarsGeo->GetProperty(NiProperty::SHADE));
                            }
                            else {
                                pStarsProp = nullptr;
                            }
                            if (pStarsProp) {
                                if (colorLastFrame != pt) {
                                    pStarsProp->VertexColor.r = pt.x;
                                    pStarsProp->VertexColor.g = pt.y;
                                    pStarsProp->VertexColor.b = pt.z;
                                }
                                pStarsProp->VertexColor.a = fAlpha;
                            }
                        }
                    }
                }
                colorLastFrame = pt;
                double dDaysPassed = static_cast<double>(appCalendar.GetDaysPassed());
                float fElapsed = apSky->GetCurrentGameHour() / 24.0 + dDaysPassed;
                double dRadians = std::fmodf(fElapsed, fStarsRotateDays.Float()) * 6.2831854820251465;
                float fAngle = dRadians / fStarsRotateDays.Float();
                float fAxisZ = fStarsRotateZAxis.Float();
                float fAxisY = fStarsRotateYAxis.Float();
                NiPoint3 axis(fStarsRotateXAxis.Float(), fAxisY, fAxisZ);
                axis.Unitize();
                if (spStars.m_pObject) {
                    spStars->SetRotate(fAngle, axis.x, axis.y, axis.z);
                }
            }
            else {
                spRoot->SetAppCulled(true);
            }
        }
    }
}

void Stars::UnloadGeometry() {
    // Tests spRoot, not spStars, and clears spStars on both paths.
    if (spRoot) {
        // spRoot is the parent of spStars.
        spRoot->DetachChild(spStars);
    }

    spStars = nullptr;
}

void Stars::LoadGeometry(const char* apFilename) {
    if (apFilename && *apFilename) {
        BSStringT<char> kPath;
        kPath.SPrintF("%s%s%s", "Meshes", "\\", apFilename);
        BSStream kStream;
        if (kStream.BSStream::Load(static_cast<const char*>(kPath.pString), static_cast<NiFile*>(nullptr)) &&
            kStream.GetObjectCount() == 1 &&
            NiObject::IsKindOf_0(&NiNode::ms_RTTI, kStream.GetObjectAt(0))) {
            Model kModel(kPath.pString, &kStream, true, false);
            spRoot->SetDisplayObject(true);
            spRoot->DetachChild(spStars.m_pObject);
            spStars = kModel.GetModel();
            spStars->SetSelectiveUpdate(true);
            spRoot->AttachChild(spStars.m_pObject, 1);
            spStars->UpdateProperties();
        }
        else {
            Warning("DEFAULT: Cannot load the stars.");
            spRoot->SetDisplayObject(false);
        }
    }
    else {
        spRoot->SetDisplayObject(false);
    }
}
