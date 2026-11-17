#include "Gamebryo2.2/CoreLibs/NiSystem/NiThread.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

int32_t NiThread::Suspend() {
	return SystemSuspend();
}

// SuspendThread has no standard form: BSSystemThread::Suspend stops the thread at its next wait (a listed
// departure). Nothing in src creates a NiThread (NiStream's m_pkThread stays null).
int32_t NiThread::SystemSuspend() {
	if (!m_hThread) {
		return -1;
	}
	int32_t iResult = static_cast<int32_t>(m_hThread->Suspend());
	if (iResult != -1) {
		m_eStatus = SUSPENDED;
	}
	return iResult;
}
