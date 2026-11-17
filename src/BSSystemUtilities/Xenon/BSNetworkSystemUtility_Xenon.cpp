#include "BSSystemUtilities/Xenon/BSNetworkSystemUtility_Xenon.h"

#include "BSSystemUtilities/BSNetworkSystemUtility.h"
#include "BSSystemUtilities/SteamAPICallbacks.h"
#include "Reconstruction.hpp"
#include "SDK/Steam/isteamuser.h"

BSNetworkSystemUtility::~BSNetworkSystemUtility() {}

void BSNetworkSystemUtility::Idle() {
	bool bWasOnline = this->m_online;
	bool bOnline = false;
	if (SteamAPI_IsSteamRunning()) {
		if (SteamUser()) {
			if (SteamUser()->BLoggedOn()) {
				bOnline = true;
			}
		}
	}
	this->m_online = bOnline;
	if (this->m_online != bWasOnline) {
		if (m_callback) {
			m_callback(BSNetworkSystemUtility::NetworkStatusChanged, this->m_online);
		}
	}
}

bool BSNetworkSystemUtility::Initialize() {
	return (bool)1;
}

void BSNetworkSystemUtility::Destroy() {
	;
}
