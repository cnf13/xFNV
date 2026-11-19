#include "Fallout Shared/Sky/Stars.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSStream.h"
#include "BSMain/Setting.h"
#include "BSShader/SkyShaderProperty.h"
#include "Fallout Shared/ModelLoader.h"
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
        float fhour = apSky->GetCurrentGameHour();
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
            if (spRoot.m_pObject->GetDisplayObject() &&
                ((fSunsetBegin = apSky->GetSunsetBegin(),
                  (fSunsetEnd = apSky->GetEndSunsetColors(),
                   (fSunriseEnd = apSky->GetSunriseEnd(),
                    (fSunriseBegin = apSky->GetBeginSunriseColors(),
                     (fBeginFadeIn = fSunsetEnd - (fSunsetEnd - fSunsetBegin) * 0.5,
                      (fEndFadeOut = fSunriseEnd - (fSunriseEnd - fSunriseBegin) * 0.5,
                       !(fhour > static_cast<double>(fSunriseBegin)))))))) ||
                         !(fEndFadeOut > static_cast<double>(fhour))
                     ? !(fhour >= static_cast<double>(fEndFadeOut)) || !(fBeginFadeIn >= static_cast<double>(fhour))
                         ? !(fhour > static_cast<double>(fBeginFadeIn)) || !(fSunsetEnd > static_cast<double>(fhour))
                             ? fAlpha = 1.0f
                             : fAlpha = (fhour - fBeginFadeIn) / (fSunsetEnd - fBeginFadeIn)
                         : fAlpha = 0.0f
                     : fAlpha = (fEndFadeOut - fhour) / (fEndFadeOut - fSunriseBegin),
                 fAlpha > 0.0)) {
                uint32_t icount;
                NiPoint3 pt;
                spRoot.m_pObject->SetAppCulled(false);
                // SkyColor[6] is the stars' color; its three channels are copied into pt.
                const NiColor& kStarsColor = apSky->SkyColor[6];
                pt.x = kStarsColor.r;
                pt.y = kStarsColor.g;
                pt.z = kStarsColor.b;
                NiNode* v22 = nullptr;
                int32_t v17 = 0;
                if (spStars.m_pObject) {
                    icount = static_cast<NiNode*>(spStars.m_pObject)->NiNode::GetChildCount();
                }
                else {
                    icount = 0;
                }
                for (uint32_t i = 0; i < icount; ++i) {
                    NiAVObject* pchild = static_cast<NiNode*>(spStars.m_pObject)->NiNode::GetAt(i);
                    if (pchild) {
                        NiGeometry* pstarsgeo = pchild->IsGeometry();
                        if (pstarsgeo) {
                            SkyShaderProperty* pStarsProp_2;
                            int32_t v12;
                            if (pstarsgeo->GetProperty(3)) {
                                SkyShaderProperty* pStarsProp =
                                    static_cast<SkyShaderProperty*>(pstarsgeo->GetProperty(3));
                                v12 = pStarsProp->iShaderPropertyType == 13;
                            }
                            else {
                                v12 = 0;
                            }
                            if (v12) {
                                pStarsProp_2 = static_cast<SkyShaderProperty*>(pstarsgeo->GetProperty(3));
                            }
                            else {
                                pStarsProp_2 = nullptr;
                            }
                            if (pStarsProp_2) {
                                if (colorLastFrame != pt) {
                                    pStarsProp_2->VertexColor.r = pt.x;
                                    pStarsProp_2->VertexColor.g = pt.y;
                                    pStarsProp_2->VertexColor.b = pt.z;
                                }
                                pStarsProp_2->VertexColor.a = fAlpha;
                            }
                        }
                    }
                }
                colorLastFrame = pt;
                double DaysPassed = static_cast<double>(appCalendar.GetDaysPassed());
                float felapsed = apSky->GetCurrentGameHour() / 24.0 + DaysPassed;
                float* v5 = &fStarsRotateDays.Float();
                double v9 = std::fmodf(felapsed, *v5) * 6.2831854820251465;
                float fangle = v9 / fStarsRotateDays.Float();
                float Y = fStarsRotateZAxis.Float();
                float X = fStarsRotateYAxis.Float();
                float* v6 = &fStarsRotateXAxis.Float();
                NiPoint3 axis(*v6, X, Y);
                axis.Unitize();
                if (spStars.m_pObject) {
                    spStars.m_pObject->SetRotate(fangle, axis.x, axis.y, axis.z);
                }
            }
            else {
                spRoot.m_pObject->SetAppCulled(true);
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
        NiObject* ObjectAt;
        BSStringT<char> v8;
        v8.SPrintF("%s%s%s", "Meshes", "\\", apFilename);
        BSStream apMemory_16;
        if (apMemory_16.BSStream::Load(static_cast<const char*>(v8.pString), static_cast<NiFile*>(nullptr)) &&
            (apMemory_16.GetObjectCount() == 1 &&
             (ObjectAt = apMemory_16.GetObjectAt(0), NiObject::IsKindOf_0(&NiNode::ms_RTTI, ObjectAt)))) {
            Model v6(v8.pString, &apMemory_16, true, false);
            spRoot.m_pObject->SetDisplayObject(true);
            NiNode* v4 = spRoot.m_pObject;
            v4->DetachChild(spStars.m_pObject);
            spStars = v6.GetModel();
            spStars.m_pObject->SetSelectiveUpdate(true);
            NiNode* v3 = spRoot.m_pObject;
            v3->AttachChild(spStars.m_pObject, 1);
            spStars.m_pObject->UpdateProperties();
        }
        else {
            Warning("DEFAULT: Cannot load the stars.");
            spRoot.m_pObject->SetDisplayObject(false);
        }
    }
    else {
        spRoot.m_pObject->SetDisplayObject(false);
    }
}
