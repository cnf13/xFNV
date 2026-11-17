#include "Gamebryo2.2/CoreLibs/NiSystem/NiMessageBox.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/System.h"

// The default hook: MessageBoxA with no owner window and MB_TOPMOST (0x40000), the user's
// answer dropped and 0 returned. pvExtraData is unused on the PC.
uint32_t NiMessageBox::DefaultMessageBox(const char* pcText, const char* pcCaption, void* pvExtraData) {
	platform::ShowMessageBox(pcText, pcCaption, 0x40000u);
	return 0;
}
