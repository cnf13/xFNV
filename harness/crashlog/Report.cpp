// The parts of a thread's section of the log that both platforms print alike: registers, the stack scan, the thread's
// game object, and the time.

#include "crashlog/CrashLog.hpp"

namespace fnvd::crashlog {

void registers(Out& arOut, const Register* apRegisters, size_t auiCount, uintptr_t auiStackLow,
	uintptr_t auiStackHigh)
{
	for (size_t i = 0; i < auiCount; ++i) {
		arOut.str("  ").str(apRegisters[i].pName).chr(' ').hex(apRegisters[i].uiValue);
		annotateValue(arOut, apRegisters[i].uiValue, auiStackLow, auiStackHigh);
		arOut.nl();
	}
}

void scan(Out& arOut, uintptr_t auiStackLow, uintptr_t auiStackHigh, uint32_t auiMaxWords)
{
	const uintptr_t uiEnd =
		auiStackHigh - auiStackLow > kWord * auiMaxWords ? auiStackLow + kWord * auiMaxWords : auiStackHigh;
	arOut.str(" Stack ").hex(auiStackLow).str("..").hex(auiStackHigh).str(" (scanned to ").hex(uiEnd).str("):").nl();
	static uintptr_t auiWords[1024];
	for (uintptr_t uiChunk = auiStackLow & ~(kWord - 1); uiChunk < uiEnd; uiChunk += sizeof(auiWords)) {
		const uint32_t uiCount = static_cast<uint32_t>(
			(uiEnd - uiChunk < sizeof(auiWords) ? uiEnd - uiChunk : sizeof(auiWords)) / kWord);
		if (!readMemory(auiWords, uiChunk, kWord * uiCount)) {
			arOut.str("  ").hex(uiChunk).str(" unreadable").nl();
			return;
		}
		for (uint32_t i = 0; i < uiCount; ++i) {
			const uintptr_t uiValue = auiWords[i];
			uint8_t cProbe;
			if (uiValue < 0x10000 || !(isCode(uiValue) || readMemory(&cProbe, uiValue, 1))) {
				continue;
			}
			arOut.str("  ").hex(uiChunk + kWord * i).str(": ").hex(uiValue);
			annotateValue(arOut, uiValue, auiStackLow, auiStackHigh);
			arOut.nl();
		}
	}
}

void threadClass(Out& arOut, uintptr_t auiStackLow, uintptr_t auiStackHigh, uint32_t auiThreadId)
{
	// a BSThread on the stack's first words (its start routine's argument) whose thread id is this one names the
	// thread by its class
	for (uintptr_t uiWord = auiStackHigh - kWord; uiWord >= auiStackLow && uiWord + 4096 > auiStackHigh;
		uiWord -= kWord) {
		uintptr_t uiValue = 0;
		if (readMemory(&uiValue, uiWord, kWord) && uiValue >= 0x10000 && threadObject(arOut, uiValue, auiThreadId)) {
			return;
		}
	}
}

void printTime(Out& arOut, const DateTime& arTime)
{
	arOut.dec(arTime.uiYear).chr('-');
	const uint16_t auiParts[] = {arTime.uiMonth, arTime.uiDay, arTime.uiHour, arTime.uiMinute, arTime.uiSecond};
	const char acSeparators[] = {'-', ' ', ':', ':', '\0'};
	for (int32_t i = 0; i < 5; ++i) {
		arOut.chr(static_cast<char>('0' + auiParts[i] / 10)).chr(static_cast<char>('0' + auiParts[i] % 10));
		if (acSeparators[i]) {
			arOut.chr(acSeparators[i]);
		}
	}
}

} // namespace fnvd::crashlog
