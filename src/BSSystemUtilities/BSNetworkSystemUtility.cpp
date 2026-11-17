#include "BSSystemUtilities/BSNetworkSystemUtility.h"

#include "BSCore/MemoryManager.h"

BSNetworkSystemUtility::BSNetworkSystemUtility() : m_online(false), m_callback(nullptr) {}

bool BSNetworkSystemUtility::QOnline() {
	return (bool)((uint8_t)m_online);
}

void BSNetworkSystemUtility::RegisterCallback(NetCallback cb) {
	m_callback = cb;
	if (cb) {
		cb(BSNetworkSystemUtility::NetworkStatusChanged, m_online);
	}
}

BSNetworkSystemUtility& BSNetworkSystemUtility::QInstance() {
	BSNetworkSystemUtility* v1;
	void* v2;
	if (!BSNetworkSystemUtility::pSingleton) {
		v2 = (&MemoryManager_Singleton)->Allocate(sizeof(BSNetworkSystemUtility));
		if (v2) {
			v1 = ::new (v2) BSNetworkSystemUtility;
		} else {
			v1 = nullptr;
		}
		BSNetworkSystemUtility::pSingleton = v1;
	}
	return *BSNetworkSystemUtility::pSingleton;
}
