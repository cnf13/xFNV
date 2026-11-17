#include "Fallout Shared/TESFurniture.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

static SettingT<GameSettingCollection> fFurnitureMarker01DeltaX("fFurnitureMarker01DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker01DeltaY("fFurnitureMarker01DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker01DeltaZ("fFurnitureMarker01DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker02DeltaX("fFurnitureMarker02DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker02DeltaY("fFurnitureMarker02DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker02DeltaZ("fFurnitureMarker02DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker03DeltaX("fFurnitureMarker03DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker03DeltaY("fFurnitureMarker03DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker03DeltaZ("fFurnitureMarker03DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker04DeltaX("fFurnitureMarker04DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker04DeltaY("fFurnitureMarker04DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker04DeltaZ("fFurnitureMarker04DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker05DeltaX("fFurnitureMarker05DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker05DeltaY("fFurnitureMarker05DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker05DeltaZ("fFurnitureMarker05DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker06DeltaX("fFurnitureMarker06DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker06DeltaY("fFurnitureMarker06DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker06DeltaZ("fFurnitureMarker06DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker07DeltaX("fFurnitureMarker07DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker07DeltaY("fFurnitureMarker07DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker07DeltaZ("fFurnitureMarker07DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker08DeltaX("fFurnitureMarker08DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker08DeltaY("fFurnitureMarker08DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker08DeltaZ("fFurnitureMarker08DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker09DeltaX("fFurnitureMarker09DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker09DeltaY("fFurnitureMarker09DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker09DeltaZ("fFurnitureMarker09DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker10DeltaX("fFurnitureMarker10DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker10DeltaY("fFurnitureMarker10DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker10DeltaZ("fFurnitureMarker10DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker11DeltaX("fFurnitureMarker11DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker11DeltaY("fFurnitureMarker11DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker11DeltaZ("fFurnitureMarker11DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker12DeltaX("fFurnitureMarker12DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker12DeltaY("fFurnitureMarker12DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker12DeltaZ("fFurnitureMarker12DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker13DeltaX("fFurnitureMarker13DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker13DeltaY("fFurnitureMarker13DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker13DeltaZ("fFurnitureMarker13DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker14DeltaX("fFurnitureMarker14DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker14DeltaY("fFurnitureMarker14DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker14DeltaZ("fFurnitureMarker14DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker15DeltaX("fFurnitureMarker15DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker15DeltaY("fFurnitureMarker15DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker15DeltaZ("fFurnitureMarker15DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker16DeltaX("fFurnitureMarker16DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker16DeltaY("fFurnitureMarker16DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker16DeltaZ("fFurnitureMarker16DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker17DeltaX("fFurnitureMarker17DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker17DeltaY("fFurnitureMarker17DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker17DeltaZ("fFurnitureMarker17DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker18DeltaX("fFurnitureMarker18DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker18DeltaY("fFurnitureMarker18DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker18DeltaZ("fFurnitureMarker18DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker19DeltaX("fFurnitureMarker19DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker19DeltaY("fFurnitureMarker19DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker19DeltaZ("fFurnitureMarker19DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker20DeltaX("fFurnitureMarker20DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker20DeltaY("fFurnitureMarker20DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker20DeltaZ("fFurnitureMarker20DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker21DeltaX("fFurnitureMarker21DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker21DeltaY("fFurnitureMarker21DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker21DeltaZ("fFurnitureMarker21DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker22DeltaX("fFurnitureMarker22DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker22DeltaY("fFurnitureMarker22DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker22DeltaZ("fFurnitureMarker22DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker23DeltaX("fFurnitureMarker23DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker23DeltaY("fFurnitureMarker23DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker23DeltaZ("fFurnitureMarker23DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker24DeltaX("fFurnitureMarker24DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker24DeltaY("fFurnitureMarker24DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker24DeltaZ("fFurnitureMarker24DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker25DeltaX("fFurnitureMarker25DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker25DeltaY("fFurnitureMarker25DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker25DeltaZ("fFurnitureMarker25DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker26DeltaX("fFurnitureMarker26DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker26DeltaY("fFurnitureMarker26DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker26DeltaZ("fFurnitureMarker26DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker27DeltaX("fFurnitureMarker27DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker27DeltaY("fFurnitureMarker27DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker27DeltaZ("fFurnitureMarker27DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker28DeltaX("fFurnitureMarker28DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker28DeltaY("fFurnitureMarker28DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker28DeltaZ("fFurnitureMarker28DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker29DeltaX("fFurnitureMarker29DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker29DeltaY("fFurnitureMarker29DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker29DeltaZ("fFurnitureMarker29DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker30DeltaX("fFurnitureMarker30DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker30DeltaY("fFurnitureMarker30DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker30DeltaZ("fFurnitureMarker30DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker31DeltaX("fFurnitureMarker31DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker31DeltaY("fFurnitureMarker31DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker31DeltaZ("fFurnitureMarker31DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker32DeltaX("fFurnitureMarker32DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker32DeltaY("fFurnitureMarker32DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker32DeltaZ("fFurnitureMarker32DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker33DeltaX("fFurnitureMarker33DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker33DeltaY("fFurnitureMarker33DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker33DeltaZ("fFurnitureMarker33DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker34DeltaX("fFurnitureMarker34DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker34DeltaY("fFurnitureMarker34DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker34DeltaZ("fFurnitureMarker34DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker35DeltaX("fFurnitureMarker35DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker35DeltaY("fFurnitureMarker35DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker35DeltaZ("fFurnitureMarker35DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker36DeltaX("fFurnitureMarker36DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker36DeltaY("fFurnitureMarker36DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker36DeltaZ("fFurnitureMarker36DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker37DeltaX("fFurnitureMarker37DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker37DeltaY("fFurnitureMarker37DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker37DeltaZ("fFurnitureMarker37DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker38DeltaX("fFurnitureMarker38DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker38DeltaY("fFurnitureMarker38DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker38DeltaZ("fFurnitureMarker38DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker39DeltaX("fFurnitureMarker39DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker39DeltaY("fFurnitureMarker39DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker39DeltaZ("fFurnitureMarker39DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker40DeltaX("fFurnitureMarker40DeltaX", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker40DeltaY("fFurnitureMarker40DeltaY", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker40DeltaZ("fFurnitureMarker40DeltaZ", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker01HeadingDelta("fFurnitureMarker01HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker02HeadingDelta("fFurnitureMarker02HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker03HeadingDelta("fFurnitureMarker03HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker04HeadingDelta("fFurnitureMarker04HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker05HeadingDelta("fFurnitureMarker05HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker06HeadingDelta("fFurnitureMarker06HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker07HeadingDelta("fFurnitureMarker07HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker08HeadingDelta("fFurnitureMarker08HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker09HeadingDelta("fFurnitureMarker09HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker10HeadingDelta("fFurnitureMarker10HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker11HeadingDelta("fFurnitureMarker11HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker12HeadingDelta("fFurnitureMarker12HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker13HeadingDelta("fFurnitureMarker13HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker14HeadingDelta("fFurnitureMarker14HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker15HeadingDelta("fFurnitureMarker15HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker16HeadingDelta("fFurnitureMarker16HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker17HeadingDelta("fFurnitureMarker17HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker18HeadingDelta("fFurnitureMarker18HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker19HeadingDelta("fFurnitureMarker19HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker20HeadingDelta("fFurnitureMarker20HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker21HeadingDelta("fFurnitureMarker21HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker22HeadingDelta("fFurnitureMarker22HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker23HeadingDelta("fFurnitureMarker23HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker24HeadingDelta("fFurnitureMarker24HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker25HeadingDelta("fFurnitureMarker25HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker26HeadingDelta("fFurnitureMarker26HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker27HeadingDelta("fFurnitureMarker27HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker28HeadingDelta("fFurnitureMarker28HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker29HeadingDelta("fFurnitureMarker29HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker30HeadingDelta("fFurnitureMarker30HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker31HeadingDelta("fFurnitureMarker31HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker32HeadingDelta("fFurnitureMarker32HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker33HeadingDelta("fFurnitureMarker33HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker34HeadingDelta("fFurnitureMarker34HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker35HeadingDelta("fFurnitureMarker35HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker36HeadingDelta("fFurnitureMarker36HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker37HeadingDelta("fFurnitureMarker37HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker38HeadingDelta("fFurnitureMarker38HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker39HeadingDelta("fFurnitureMarker39HeadingDelta", 0.0f);
static SettingT<GameSettingCollection> fFurnitureMarker40HeadingDelta("fFurnitureMarker40HeadingDelta", 0.0f);

// Each marker's X, Y and Z offsets and heading delta, by marker.
FurnitureMarkerDeltaSettings pFurnitureMarkerDelta{
    {&fFurnitureMarker01DeltaX, &fFurnitureMarker01DeltaY, &fFurnitureMarker01DeltaZ, &fFurnitureMarker01HeadingDelta},
    {&fFurnitureMarker02DeltaX, &fFurnitureMarker02DeltaY, &fFurnitureMarker02DeltaZ, &fFurnitureMarker02HeadingDelta},
    {&fFurnitureMarker03DeltaX, &fFurnitureMarker03DeltaY, &fFurnitureMarker03DeltaZ, &fFurnitureMarker03HeadingDelta},
    {&fFurnitureMarker04DeltaX, &fFurnitureMarker04DeltaY, &fFurnitureMarker04DeltaZ, &fFurnitureMarker04HeadingDelta},
    {&fFurnitureMarker05DeltaX, &fFurnitureMarker05DeltaY, &fFurnitureMarker05DeltaZ, &fFurnitureMarker05HeadingDelta},
    {&fFurnitureMarker06DeltaX, &fFurnitureMarker06DeltaY, &fFurnitureMarker06DeltaZ, &fFurnitureMarker06HeadingDelta},
    {&fFurnitureMarker07DeltaX, &fFurnitureMarker07DeltaY, &fFurnitureMarker07DeltaZ, &fFurnitureMarker07HeadingDelta},
    {&fFurnitureMarker08DeltaX, &fFurnitureMarker08DeltaY, &fFurnitureMarker08DeltaZ, &fFurnitureMarker08HeadingDelta},
    {&fFurnitureMarker09DeltaX, &fFurnitureMarker09DeltaY, &fFurnitureMarker09DeltaZ, &fFurnitureMarker09HeadingDelta},
    {&fFurnitureMarker10DeltaX, &fFurnitureMarker10DeltaY, &fFurnitureMarker10DeltaZ, &fFurnitureMarker10HeadingDelta},
    {&fFurnitureMarker11DeltaX, &fFurnitureMarker11DeltaY, &fFurnitureMarker11DeltaZ, &fFurnitureMarker11HeadingDelta},
    {&fFurnitureMarker12DeltaX, &fFurnitureMarker12DeltaY, &fFurnitureMarker12DeltaZ, &fFurnitureMarker12HeadingDelta},
    {&fFurnitureMarker13DeltaX, &fFurnitureMarker13DeltaY, &fFurnitureMarker13DeltaZ, &fFurnitureMarker13HeadingDelta},
    {&fFurnitureMarker14DeltaX, &fFurnitureMarker14DeltaY, &fFurnitureMarker14DeltaZ, &fFurnitureMarker14HeadingDelta},
    {&fFurnitureMarker15DeltaX, &fFurnitureMarker15DeltaY, &fFurnitureMarker15DeltaZ, &fFurnitureMarker15HeadingDelta},
    {&fFurnitureMarker16DeltaX, &fFurnitureMarker16DeltaY, &fFurnitureMarker16DeltaZ, &fFurnitureMarker16HeadingDelta},
    {&fFurnitureMarker17DeltaX, &fFurnitureMarker17DeltaY, &fFurnitureMarker17DeltaZ, &fFurnitureMarker17HeadingDelta},
    {&fFurnitureMarker18DeltaX, &fFurnitureMarker18DeltaY, &fFurnitureMarker18DeltaZ, &fFurnitureMarker18HeadingDelta},
    {&fFurnitureMarker19DeltaX, &fFurnitureMarker19DeltaY, &fFurnitureMarker19DeltaZ, &fFurnitureMarker19HeadingDelta},
    {&fFurnitureMarker20DeltaX, &fFurnitureMarker20DeltaY, &fFurnitureMarker20DeltaZ, &fFurnitureMarker20HeadingDelta},
    {&fFurnitureMarker21DeltaX, &fFurnitureMarker21DeltaY, &fFurnitureMarker21DeltaZ, &fFurnitureMarker21HeadingDelta},
    {&fFurnitureMarker22DeltaX, &fFurnitureMarker22DeltaY, &fFurnitureMarker22DeltaZ, &fFurnitureMarker22HeadingDelta},
    {&fFurnitureMarker23DeltaX, &fFurnitureMarker23DeltaY, &fFurnitureMarker23DeltaZ, &fFurnitureMarker23HeadingDelta},
    {&fFurnitureMarker24DeltaX, &fFurnitureMarker24DeltaY, &fFurnitureMarker24DeltaZ, &fFurnitureMarker24HeadingDelta},
    {&fFurnitureMarker25DeltaX, &fFurnitureMarker25DeltaY, &fFurnitureMarker25DeltaZ, &fFurnitureMarker25HeadingDelta},
    {&fFurnitureMarker26DeltaX, &fFurnitureMarker26DeltaY, &fFurnitureMarker26DeltaZ, &fFurnitureMarker26HeadingDelta},
    {&fFurnitureMarker27DeltaX, &fFurnitureMarker27DeltaY, &fFurnitureMarker27DeltaZ, &fFurnitureMarker27HeadingDelta},
    {&fFurnitureMarker28DeltaX, &fFurnitureMarker28DeltaY, &fFurnitureMarker28DeltaZ, &fFurnitureMarker28HeadingDelta},
    {&fFurnitureMarker29DeltaX, &fFurnitureMarker29DeltaY, &fFurnitureMarker29DeltaZ, &fFurnitureMarker29HeadingDelta},
    {&fFurnitureMarker30DeltaX, &fFurnitureMarker30DeltaY, &fFurnitureMarker30DeltaZ, &fFurnitureMarker30HeadingDelta},
    {&fFurnitureMarker31DeltaX, &fFurnitureMarker31DeltaY, &fFurnitureMarker31DeltaZ, &fFurnitureMarker31HeadingDelta},
    {&fFurnitureMarker32DeltaX, &fFurnitureMarker32DeltaY, &fFurnitureMarker32DeltaZ, &fFurnitureMarker32HeadingDelta},
    {&fFurnitureMarker33DeltaX, &fFurnitureMarker33DeltaY, &fFurnitureMarker33DeltaZ, &fFurnitureMarker33HeadingDelta},
    {&fFurnitureMarker34DeltaX, &fFurnitureMarker34DeltaY, &fFurnitureMarker34DeltaZ, &fFurnitureMarker34HeadingDelta},
    {&fFurnitureMarker35DeltaX, &fFurnitureMarker35DeltaY, &fFurnitureMarker35DeltaZ, &fFurnitureMarker35HeadingDelta},
    {&fFurnitureMarker36DeltaX, &fFurnitureMarker36DeltaY, &fFurnitureMarker36DeltaZ, &fFurnitureMarker36HeadingDelta},
    {&fFurnitureMarker37DeltaX, &fFurnitureMarker37DeltaY, &fFurnitureMarker37DeltaZ, &fFurnitureMarker37HeadingDelta},
    {&fFurnitureMarker38DeltaX, &fFurnitureMarker38DeltaY, &fFurnitureMarker38DeltaZ, &fFurnitureMarker38HeadingDelta},
    {&fFurnitureMarker39DeltaX, &fFurnitureMarker39DeltaY, &fFurnitureMarker39DeltaZ, &fFurnitureMarker39HeadingDelta},
    {&fFurnitureMarker40DeltaX, &fFurnitureMarker40DeltaY, &fFurnitureMarker40DeltaZ, &fFurnitureMarker40HeadingDelta}
};

TESFurniture::TESFurniture() : TESObjectACTI() {
    SetFormType(FURN_ID);
    TESFurniture::InitializeData();
    SetInitialized(1);
}

TESFurniture::~TESFurniture() {}

void TESFurniture::InitializeData() {
    iFurnFlags = (uint32_t)0;
    TESObjectACTI::InitializeData();
}

// Reads the furniture's chunks: the editor ID, the bounds, the model and its texture swaps, the name, the marker
// flags, the script and the destructible data.
bool TESFurniture::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case MODL_ID:
            case MODT_ID:
            case MOSD_ID:
            case MODD_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case MODS_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                apFile->GetChunkData(pBuffer, uiChunkSize);
                uint32_t uiVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, uiVersion);
                break;
            }
            case SCRI_ID: {
                uint32_t uiScriptID = 0;
                apFile->GetChunkData(uiScriptID);
                TESScriptableForm* pScriptable = static_cast<TESScriptableForm*>(this);
                // the script's form ID stands in for the script until the forms are resolved
                pScriptable->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
                pScriptable->InitItem(this);
                break;
            }
            case DSTD_ID:
            case DEST_ID:
                BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
                break;
            case MNAM_ID: apFile->GetChunkData(iFurnFlags); break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESFurniture::Save() {
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    static_cast<TESScriptableForm*>(this)->Save();
    static_cast<BGSDestructibleObjectForm*>(this)->Save();
    TESForm::AddChunk(MNAM_ID, iFurnFlags);
    TESForm::CloseForm();
}

void TESFurniture::InitItem() {
    if (!TESForm::GetInitialized()) {
        static_cast<BGSDestructibleObjectForm*>(this)->InitItem(this);
        VerifyMarkers();
        TESForm::SetInitialized(1);
    }
}

void TESFurniture::Copy(TESForm* apCopy) {
    TESFurniture* v3 = dynamic_cast<TESFurniture*>(apCopy);
    if (v3) {
        TESForm::CopyAllComponents(apCopy);
        iFurnFlags = v3->iFurnFlags;
        VerifyMarkers();
    }
}

bool TESFurniture::Compare(TESForm* apCompare) {
    TESFurniture* v4 = dynamic_cast<TESFurniture*>(apCompare);
    if (!v4) {
        return (bool)1;
    }
    if (TESForm::CompareAllComponents(v4)) {
        return (bool)1;
    }
    return (bool)((uint8_t)(iFurnFlags != v4->iFurnFlags));
}

bool TESFurniture::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjectToGet,
    int32_t aicount
) {
    PlayerCharacter* pactor = dynamic_cast<PlayerCharacter*>(apActionRef);
    if (!pactor) {
        return 0;
    }
    if (pactor->GetSitSleepState()) {
        pactor->ForceTemp3rdPerson(1);
        pactor->ClearSitHeadingDelta();
        pactor->InitiateGetUpPackage();
    }
    else {
        uint32_t iunusedMarker = 0;
        FurnitureMark marker(255u);
        const NiPoint3* v7 = &apActionRef->GetLocationOnReference();
        if (!apItemActivated->GetClosestFreeMarker(*v7, 1, 1, marker, iunusedMarker, 1)) {
            return 0;
        }
        uint32_t Type = marker.GetType();
        if (TESFurniture::IsSleepMarker(Type)) {
            if (!apItemActivated->GetOwner() || apItemActivated->IsAnOwner(pactor, 1)) {
                if (pactor == pPlayer && pPlayer->GetParentCell()->GetCantWaitHere()) {
                    const char* v10 = sNoWaitInCell.String();
                    Interface::ShowMessage(
                        v10, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0
                    );
                    return 0;
                }
                if (pPlayer->IsTresPassing()) {
                    const char* v11 = sNoSleepTrespass.String();
                    Interface::ShowMessage(
                        v11, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0
                    );
                }
                else {
                    bool Interior = pPlayer->GetInterior();
                    if (ReferenceProcessLists.AreHostilActorsNear(Interior)) {
                        const char* v13 = sNoSleepHostileActorsNear.String();
                        Interface::ShowMessage(
                            v13, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0
                        );
                    }
                    else {
                        if (ReferenceProcessLists.IsPlayerInRadiationArea()) {
                            const char* v14 = sNoSleepInRadiation.String();
                            Interface::ShowMessage(
                                v14, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0
                            );
                        }
                        else {
                            if (pPlayer->HasDamageHealthEffect()) {
                                const char* v15 = sNoSleepTakingHealthDamage.String();
                                Interface::ShowMessage(
                                    v15, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0
                                );
                            }
                            else {
                                Interface::CreateSleepMenu(1);
                            }
                        }
                    }
                }
            }
            else {
                if (pactor == pPlayer) {
                    const char* v9 = sNoSleepInOwnedBed.String();
                    Interface::ShowMessage(
                        v9, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0
                    );
                    return 0;
                }
            }
        }
        else {
            pactor->ForceTemp3rdPerson(1);
            pactor->ClearSitHeadingDelta();
            pactor->InitiateSitSleepPackage(apItemActivated);
        }
    }
    return 1;
}

bool TESFurniture::CanSleepOn() {
    return ((iFurnFlags & 0x80000000u) != 0);
}

bool TESFurniture::CanSitOn() {
    return ((iFurnFlags & 1073741824) != 0);
}

void TESFurniture::VerifyMarkers() {
    if (!GetTemporary() && !iFurnFlags) {
        NiAVObject* v1 = Clone3D(static_cast<TESObjectREFR*>(0));
        if (BSFurnitureMarker::Find(v1)) {
            Warning("MODELS: You have not selected any FurnitureMarkers for '%s'.", GetFormEditorID());
        }
        else {
            Warning("MODELS: No FurnitureMarkers found in '%s'.", GetModel());
        }
        UnClone3D(0);
    }
}

NiPoint3 TESFurniture::GetMarkerTargetOffset(uint32_t aiIndex, float afScale) {
    NiPoint3 result;
    NiPoint3 v5 = NiPoint3_ZERO;
    if (aiIndex) {
        if (aiIndex <= 40u) {
            v5.x = pFurnitureMarkerDelta[aiIndex - 1][0]->Float();
            v5.y = pFurnitureMarkerDelta[aiIndex - 1][1]->Float();
            v5.z = (afScale - 1.0) * pFurnitureMarkerDelta[aiIndex - 1][2]->Float();
        }
    }
    result = v5;
    return result;
}

float TESFurniture::GetMarkerHeadingOffset(uint32_t aiIndex) {
    float v4 = 0.0;
    if (aiIndex) {
        if (aiIndex <= 40) {
            return pFurnitureMarkerDelta[aiIndex - 1][3]->Float();
        }
    }
    return v4;
}

bool TESFurniture::GetMarkerEnabled(uint32_t aiMarkerIndex) {
    uint8_t v3 = 0;
    if (aiMarkerIndex < 32) {
        return ((((int32_t)(1 << aiMarkerIndex)) & iFurnFlags) != 0);
    }
    return (bool)v3;
}

bool TESFurniture::IsSleepMarker(uint32_t aiNumber) {
    return (bool)((uint8_t)(aiNumber < 10));
}

bool TESFurniture::IsSitMarker(uint32_t aiNumber) {
    uint8_t v2 = 0;
    if (((aiNumber >= 10) && (aiNumber <= 20)) || (aiNumber == 26)) {
        return (bool)1;
    }
    return (bool)v2;
}
