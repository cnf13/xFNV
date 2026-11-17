#pragma once

#include <cstdint>

using NiMessageBoxFunction = uint32_t (*)(const char* apcText, const char* apcCaption, void* apvExtraData);

// The engine's message box hook.
class NiMessageBox {
public:
	// The default hook: MessageBoxA with no owner window and MB_TOPMOST; the user's answer is
	// dropped and pvExtraData is unused on the PC.
	static uint32_t DefaultMessageBox(const char* pcText, const char* pcCaption, void* pvExtraData);

	// The function that shows engine messages, null when none is set.
	static NiMessageBoxFunction ms_pfnMessageBox;
};
inline NiMessageBoxFunction NiMessageBox::ms_pfnMessageBox{&NiMessageBox::DefaultMessageBox};
