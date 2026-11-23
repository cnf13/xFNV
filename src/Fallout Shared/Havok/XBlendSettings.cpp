#include "Fallout Shared/Havok/XBlendSettings.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"

SettingT<BlendSettingCollection> BlendHit::fGetUpTime("fGetUpTime:HIT", 1.0f);
SettingT<BlendSettingCollection> BlendHit::fKnockDownTime("fKnockDownTime:HIT", 0.25f);
SettingT<BlendSettingCollection> BlendHit::fMinVelGain("fMinVelGain:HIT", 0.95f);
SettingT<BlendSettingCollection> BlendHit::fMinHeirGain("fMinHeirGain:HIT", 0.3f);
SettingT<BlendSettingCollection> BlendHit::RHand("sRHand:HIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHit::RForeArm("sRForeArm:HIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHit::RUpperArm("sRUpperArm:HIT", "0.2, 0.5");
SettingT<BlendSettingCollection> BlendHit::LHand("sLHand:HIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHit::LForeArm("sLForeArm:HIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHit::LUpperArm("sLUpperArm:HIT", "0.2, 0.5");
SettingT<BlendSettingCollection> BlendHit::Spine2("sSpine2:HIT", "0.5, 0.7");
SettingT<BlendSettingCollection> BlendHit::Spine1("sSpine1:HIT", "0.6, 0.8");
SettingT<BlendSettingCollection> BlendHit::Body("sBody:HIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHit::Head("sHead:HIT", "0.4, 0.6");
SettingT<BlendSettingCollection> BlendHitQuad::sHitSpot("sHitSpot:QUADHIT", "Bip01 Head");
SettingT<BlendSettingCollection> BlendHitQuad::sHitStart("sHitStart:QUADHIT", "Bip01 Head");
SettingT<BlendSettingCollection> BlendHitQuad::RCalf("sRCalf:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::RThigh("sRThigh:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::RForeArm("sRForeArm:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::RUpperArm("sRUpperArm:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::LCalf("sLCalf:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::LThigh("sLThigh:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::LForeArm("sLForeArm:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::LUpperArm("sLUpperArm:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::Spine2("sSpine2:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::Spine1("sSpine1:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::Body("sBody:QUADHIT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendHitQuad::Head("sHead:QUADHIT", "0.3, 0.9");
SettingT<BlendSettingCollection> BlendDefault::Head("sHead:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::Body("sBody:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::Spine1("sSpine1:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::Spine2("sSpine2:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::LUpperArm("sLUpperArm:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::LForeArm("sLForeArm:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::LHand("sLHand:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::LThigh("sLThigh:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::LCalf("sLCalf:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::LFoot("sLFoot:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::RUpperArm("sRUpperArm:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::RForeArm("sRForeArm:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::RHand("sRHand:DEFAULT", "1.0f, 1.0");
SettingT<BlendSettingCollection> BlendDefault::RThigh("sRThigh:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::RCalf("sRCalf:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::RFoot("sRFoot:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::Tail("sTail:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::Shield("sShield:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::Quiver("sQuiver:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::PonyTail("sPonyTail:DEFAULT", "1.0, 1.0");
SettingT<BlendSettingCollection> BlendDefault::fHighTrans("fHighTrans:DEFAULT", 0.07f);
SettingT<BlendSettingCollection> BlendDefault::fHighRot("fHighRot:DEFAULT", 0.005f);
SettingT<BlendSettingCollection> BlendDefault::fLowTrans("fLowTrans:DEFAULT", 0.7f);
SettingT<BlendSettingCollection> BlendDefault::fLowRot("fLowRot:DEFAULT", 0.2f);
SettingT<BlendSettingCollection> BlendDefault::fPassOutTime("fPassOutTime:DEFAULT", 1.2f);
SettingT<BlendSettingCollection> BlendDefault::fPassOutForce("fPassOutForce:DEFAULT", -10.0f);

BlendSettingCollection::BlendSettingCollection() {}

BlendSettingCollection::~BlendSettingCollection() {}

// The read and the close go through the virtual ReadSettings and Close.
bool SettingCollection::ReadSettingsFromFile(const char* apFileName) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext)
        AutoMemContext(MEM_CONTEXT_GAMESETTING, true, "d:\\_fallout3\\platforms\\common\\code\\bsmain\\Setting.h", 413);
    bool bOpened = OpenPath(apFileName, false);
    if (bOpened) {
        ReadSettings();
        Close();
    }
    pContext->~AutoMemContext();
    return bOpened;
}
