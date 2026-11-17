#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamFrameInfo.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

static void cleanupLoadedObjecthkMonitorStreamFrameInfo(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkMonitorStreamFrameInfoTypeInfo = {
    "hkMonitorStreamFrameInfo",
    nullptr,
    cleanupLoadedObjecthkMonitorStreamFrameInfo,
    nullptr,
    &typeid(hkMonitorStreamFrameInfo),
    sizeof(hkMonitorStreamFrameInfo)
};

// The enum's items and the enum.
using hkMonitorStreamFrameInfoAbsoluteTimeCounterEnumItemArray = const hkClassEnum::Item[3];
static hkMonitorStreamFrameInfoAbsoluteTimeCounterEnumItemArray hkMonitorStreamFrameInfoAbsoluteTimeCounterEnumItems{
    {hkMonitorStreamFrameInfo::ABSOLUTE_TIME_TIMER_0, "ABSOLUTE_TIME_TIMER_0"},
    {hkMonitorStreamFrameInfo::ABSOLUTE_TIME_TIMER_1, "ABSOLUTE_TIME_TIMER_1"},
    {hkMonitorStreamFrameInfo::ABSOLUTE_TIME_NOT_TIMED, "ABSOLUTE_TIME_NOT_TIMED"}
};
using hkMonitorStreamFrameInfoEnumArray = const hkClassEnum[1];
static hkMonitorStreamFrameInfoEnumArray hkMonitorStreamFrameInfoEnums{
    {"AbsoluteTimeCounter", hkMonitorStreamFrameInfoAbsoluteTimeCounterEnumItems, 3, nullptr, 0}
};
static const hkClassEnum* hkMonitorStreamFrameInfoAbsoluteTimeCounterEnum = &hkMonitorStreamFrameInfoEnums[0];

// The class record's member table: the enum pointer is stored in the absoluteTimeCounter entry.
using hkMonitorStreamFrameInfoClassMemberArray = hkClassMember[9];
static hkMonitorStreamFrameInfoClassMemberArray hkMonitorStreamFrameInfoClass_Members{
    {"heading", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"indexOfTimer0", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr},
    {"indexOfTimer1", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr},
    {"absoluteTimeCounter",
     nullptr,
     hkMonitorStreamFrameInfoAbsoluteTimeCounterEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT32},
     0,
     0,
     0xC,
     nullptr},
    {"timerFactor0", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x10, nullptr},
    {"timerFactor1", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0x14, nullptr},
    {"threadId", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x18, nullptr},
    {"frameStreamStart",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x1C,
     nullptr},
    {"frameStreamEnd", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x20, nullptr}
};
hkClass hkMonitorStreamFrameInfoClass(
    "hkMonitorStreamFrameInfo",
    nullptr,
    36,
    nullptr,
    0,
    hkMonitorStreamFrameInfoEnums,
    1,
    hkMonitorStreamFrameInfoClass_Members,
    9,
    nullptr,
    nullptr,
    0,
    0
);

static void cleanupLoadedObjecthkMonitorStreamFrameInfo(void* p) {
    ;
}

const hkClass& hkMonitorStreamFrameInfo::staticClass() {
    return hkMonitorStreamFrameInfoClass;
}
