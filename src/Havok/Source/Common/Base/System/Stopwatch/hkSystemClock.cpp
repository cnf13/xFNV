#include "Havok/Source/Common/Base/System/Stopwatch/hkSystemClock.h"

#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/System/Stopwatch/Impl/hkWindowsSystemClock.h"

// Havok SDK signature (hkBase.lib, tied by data).
static hkReferencedObject* hkSystemClockcreate();

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkSystemClock(
    "hkSystemClock",
    &hkSystemClockcreate,
    reinterpret_cast<void**>(&hkSystemClock::s_instance)
);

// The singleton's create function: new hkWindowsSystemClock().
static hkReferencedObject* hkSystemClockcreate() {
    return new hkWindowsSystemClock;
}
