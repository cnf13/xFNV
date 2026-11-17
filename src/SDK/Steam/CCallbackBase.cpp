#include "SDK/Steam/CCallbackBase.h"
#include "SDK/Steam/steamclientpublic.h"

CCallbackBase::CCallbackBase() : m_nCallbackFlags(0), m_iCallback(0) {}

void CCallbackBase::Unregister() {
    SteamAPI_UnregisterCallback(this);
}

// The SDK's CSteamID(): account id, account type (k_EAccountTypeInvalid), universe (k_EUniverseInvalid) and
// instance all 0.
CSteamID::CSteamID() : m_unAccountID(0), m_unAccountInstance(0), m_EAccountType(0), m_EUniverse(0) {}

// The SDK's CSteamID(unAccountID, unAccountInstance, eUniverse, eAccountType).
CSteamID::CSteamID(uint32_t unAccountID, uint32_t unAccountInstance, EUniverse eUniverse, EAccountType eAccountType) {
    Set(unAccountID, unAccountInstance, eUniverse, eAccountType);
}

// The exe stores the account id as the whole first dword, then the universe's low byte, the account type's low 4
// bits and the instance's low 20 bits into the high dword.
void CSteamID::Set(uint32_t unAccountID, uint32_t unAccountInstance, EUniverse eUniverse, EAccountType eAccountType) {
    m_unAccountID = unAccountID;
    m_EUniverse = eUniverse;
    m_EAccountType = eAccountType;
    m_unAccountInstance = unAccountInstance;
}
