#include "BSSystemUtilities/BSNetworkSystemUtility.h"

#include "BSCore/MemoryManager.h"

#include <new>

BSNetworkSystemUtility::BSNetworkSystemUtility() : m_online(false), m_callback(nullptr) {}

bool BSNetworkSystemUtility::QOnline() {
    return m_online;
}

void BSNetworkSystemUtility::RegisterCallback(NetCallback cb) {
    m_callback = cb;
    if (cb) {
        cb(BSNetworkSystemUtility::NetworkStatusChanged, m_online);
    }
}

BSNetworkSystemUtility& BSNetworkSystemUtility::QInstance() {
    if (!BSNetworkSystemUtility::pSingleton) {
        BSNetworkSystemUtility::pSingleton = new BSNetworkSystemUtility;
    }
    return *BSNetworkSystemUtility::pSingleton;
}
