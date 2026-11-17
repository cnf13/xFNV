#pragma once

#include <cstdint>

// The network status utility: whether the player is online, and the callback told when that changes.
class BSNetworkSystemUtility {
public:
	// The events the registered callback receives.
	enum NetEvent : int32_t {
		NetworkStatusChanged = 0,
	};

	// Called with the event and, for NetworkStatusChanged, whether the player is online. The second argument is a
	// uint32_t because StartMenu's callback and RegisterCallback's parameter take it so.
	using NetCallback = void (*)(NetEvent, uint32_t);

	BSNetworkSystemUtility();
	~BSNetworkSystemUtility();

	bool QOnline();
	// Stores the callback and, when it is set, calls it at once with the current status.
	void RegisterCallback(NetCallback cb);
	// Polls the online status once a frame.
	void Idle();

	// The utility, allocated on first use.
	static BSNetworkSystemUtility& QInstance();
	static bool Initialize();
	static void Destroy();

	bool m_online;
	uint8_t padding01[3];
	NetCallback m_callback;

	static inline BSNetworkSystemUtility* pSingleton;
};
