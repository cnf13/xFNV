#include "BSCore/BSCoreMessage.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Reconstruction.hpp"

bool get_11F6370(WARNING_TYPES a1) {
	return (bool)BSCoreMessage::bWarningsDisabled[a1];
}

BSCoreMessage::BSCoreMessage() {}

BSCoreMessage::~BSCoreMessage() {}

void BSCoreMessage::Error(const char* apMessage) {}

int32_t BSCoreMessage::Warning(const char* apMessage) {
	return 0;
}

int32_t BSCoreMessage::Assert(const char* apMessage) {
	return 0;
}

// The PC body ignores its arguments and returns 0 (`xor eax, eax; pop ebp; ret`). A virtual can't be a variadic
// template forwarder, so the body is reconstructed instead. Left unregistered: FNVD_REPLACES_VIRTUAL's adapter
// matches a member function pointer with a fixed parameter list, so it has no form for a variadic virtual.
int32_t BSCoreMessage::Assert(const char* apFileName, uint32_t aiLineNumber, const char* apMessage, ...) {
	return 0;
}

BSCoreMessage::MsgReturn BSCoreMessage::Message(MsgStyle aStyle, const char* apcText, const char* apcCaption,
	uint32_t auiTextID, uint32_t auiCaptionID) {
	return MR_NONE;
}

void BSCoreMessage::DisableContextMessages(WARNING_TYPES aWarningContext) {
	BSCoreMessage::SetWarningDisabled(aWarningContext, 1);
}

// Not named in the PDB or IDA; name chosen here. Passes the call on to the handler; the exe calls this same
// function on pHandler (its target is itself), so a set handler recurses.
void BSCoreMessage::PassToHandler5(uint32_t a1, uint32_t a2, float a3, uint32_t a4, uint32_t a5) {
	if (BSCoreMessage::pHandler) {
		BSCoreMessage::pHandler->PassToHandler5(a1, a2, a3, a4, a5);
	}
}

// Not named in the PDB or IDA; name chosen here. As PassToHandler5 with no arguments; the exe's call targets itself.
void BSCoreMessage::StatusOff() {
	if (BSCoreMessage::pHandler) {
		BSCoreMessage::pHandler->StatusOff();
	}
}

bool BSCoreMessage::CheckWarningContexts(WARNING_TYPES aWarningContext) {
	return get_11F6370(aWarningContext);
}

bool BSCoreMessage::HasHandler() {
	return BSCoreMessage::pHandler != 0;
}

BSCoreMessage* BSCoreMessage::GetInstance() {
	return BSCoreMessage::pHandler;
}

void BSCoreMessage::SetWarningDisabled(int32_t aWarningContext, bool abValue) {
	BSCoreMessage::bWarningsDisabled[aWarningContext] = (int8_t)abValue;
}
