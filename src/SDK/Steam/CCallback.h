#pragma once

#include <stdint.h>
class SteamAPICallbacks;
struct UserAchievementStored_t;
struct UserStatsReceived_t;
struct UserStatsStored_t;

#include "SDK/Steam/CCallbackBase.h"

// The Steam SDK's callback holder (steam_api.h): registered with steam_api for P::k_iCallback, it calls the handler
// on its object with each result. The PDB has no Steam types; the class name and arguments are the exe's RTTI's
// (CCallback<Steam,UserStatsReceived_t,0> and the UserStatsStored_t and UserAchievementStored_t instances, 0x14
// bytes each, constructed by the Steam object's constructor 0x6FF690).
template <class T, class P, bool bGameServer>
class CCallback : public CCallbackBase {
public:
    // The SDK's `void (T::*)(P*)`. The exe stores the 4-byte code address of the thiscall member (T has single
    // inheritance) and calls it with m_pObj in ecx.
    using func_t = void (*)(T* apObj, P* apParam);

    // Calls the handler with the result (the call-result overload; the exe's instances share 0x6FFAF0, ret 0x10).
    void Run(void* apData, bool abIOFailure, uint64_t aCall) override;
    // Calls the handler with the result (the instances share 0x6FF9D0, ret 4).
    void Run(void* apData) override;
    // sizeof(P): each instance has its own body (or a folded constant getter).
    int32_t GetCallbackSizeBytes() override;
    // apObj and aFunc, then the callback registered when both are set.
    CCallback(T* apObj, func_t aFunc) : m_pObj(apObj), m_Func(aFunc) {
        if (apObj && aFunc) {
            Register(apObj, aFunc);
        }
    }
    // The callback unregistered when it is registered.
    ~CCallback() {
        if ((m_nCallbackFlags & 1) != 0) {
            Unregister();
        }
    }
    // The SDK's Register: unregisters the callback when it is registered, stores apObj and aFunc
    // and registers for P::k_iCallback, when both are set. Each instance has its own body;
    // defined per instance, as an explicit specialization.
    void Register(T* apObj, func_t aFunc);

    T* m_pObj;
    func_t m_Func;
};

// Both overloads call the handler on the object with the result.
template <class T, class P, bool bGameServer>
void CCallback<T, P, bGameServer>::Run(
    void* apData,
    [[maybe_unused]] bool abIOFailure,
    [[maybe_unused]] uint64_t aCall
) {
    m_Func(m_pObj, static_cast<P*>(apData));
}

template <class T, class P, bool bGameServer>
void CCallback<T, P, bGameServer>::Run(void* apData) {
    m_Func(m_pObj, static_cast<P*>(apData));
}

template <class T, class P, bool bGameServer>
int32_t CCallback<T, P, bGameServer>::GetCallbackSizeBytes() {
    return static_cast<int32_t>(sizeof(P));
}

// The SDK's Register with the game-server flag compiled out (bGameServer is false in every instance): unregisters a
// registered callback, stores the object and handler and registers for P::k_iCallback.
template <class T, class P, bool bGameServer>
void CCallback<T, P, bGameServer>::Register(T* apObj, func_t aFunc) {
    if (!apObj || !aFunc) {
        return;
    }
    if ((m_nCallbackFlags & 1) != 0) {
        Unregister();
    }
    m_pObj = apObj;
    m_Func = aFunc;
    SteamAPI_RegisterCallback(this, P::k_iCallback);
}
