#include "Havok/Source/Common/Base/System/StackTracer/hkStackTracer.h"
#include "SDK/MSVC/stdio.h"

// Set once the "cannot find symbol" message has been printed.
static uint8_t s_unknownSymbolReported;

// Prints one line per address through apPrint: the symbol the platform state's dbghelp entry resolves, its source
// line and the address's displacement. A stack that reaches WinMain stops the dump.
void hkStackTracer::dumpStackTrace(const hkUlong* apAddresses, int32_t maxtrace, printFunc apPrint,
	void* aiUserData) const {
	void* pProcess = hkStackTracerImpl::currentProcess();
	hkStackTracerImpl* pImpl = static_cast<hkStackTracerImpl*>(m_impl);
	for (int32_t i = 0; i < maxtrace; ++i) {
		// SymGetSymFromAddr takes the address as a DWORD (the Havok SDK's tSymGetSymFromAddr).
		uint32_t uiAddress = static_cast<uint32_t>(apAddresses[i]);
		int32_t iDisplacement = 0;
		hkStackTracerSymbol symbol;
		symbol.m_sizeOfStruct = 0;
		symbol.m_address = 0;
		symbol.m_size = 0;
		symbol.m_flags = 0;
		symbol.m_maxNameLength = 0;
		*reinterpret_cast<uint32_t*>(symbol.m_name) = 0;
		symbol.m_sizeOfStruct = 0x18;
		symbol.m_maxNameLength = 0x7E8;
		if (pImpl->m_getSymbol(pProcess, uiAddress, &iDisplacement, &symbol) != 0) {
			// hkString::strCmp, which the exe writes out here (0xCB0FA0).
			const char* pName = symbol.m_name;
			const char* pWinMain = "WinMain";
			while (*pName == *pWinMain && *pName != 0) {
				++pName;
				++pWinMain;
			}
			if (*pName == *pWinMain)
				return;
		} else {
			if (s_unknownSymbolReported == 0) {
				apPrint("**************************************************************\n* Cannot find symbol for an "
						"address\n* Either debug information was not found or your version of\n* dbghelp.dll may be "
						"too old to understand the debug format.\n* See the comments in "
						"hkStackTracer::hkStackTracer()\n* "
						"d:\\Projects\\fallout\\code\\Havok\\Source\\Common/Base/System/StackTracer/Impl/"
						"hkStackTracerWin32.cxx\n**************************************************************\n",
					aiUserData);
				s_unknownSymbolReported = 1;
			}
			// "(unknown)" including its zero, the ten bytes the exe copies from 0x10D4E08.
			static const char kUnknown[] = "(unknown)";
			for (int32_t k = 0; k < 10; ++k)
				symbol.m_name[k] = kUnknown[k];
		}
		hkStackTracerLine line;
		line.m_sizeOfStruct = 0;
		line.m_key = 0;
		line.m_lineNumber = 0;
		line.m_fileName = 0;
		line.m_address = 0;
		line.m_sizeOfStruct = sizeof(hkStackTracerLine);
		pImpl->m_getLine(pProcess, apAddresses[i], &iDisplacement, &line);
		char acLine[2048];
		_snprintf(acLine, 2048, "%s(%i):'%s'\n", line.m_fileName, line.m_lineNumber, symbol.m_name);
		apPrint(acLine, aiUserData);
	}
}

// Not registered: the exe's body (xor eax, eax; ret 8) leaves edx intact, and hkStackTracer::CallTree::insertCallStack
// (0xDA2D20) keeps its this in edx across the call (mov ecx, edx right after it), which clang does not guarantee.
int32_t hkStackTracer::getStackTrace([[maybe_unused]] hkUlong* trace, [[maybe_unused]] int32_t maxtrace) {
	return 0;
}
