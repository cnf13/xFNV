// What a value in a register or on a stack points to: code, the thread's stack, an object's dynamic class by its
// vtable's RTTI (with its identity, Identity.cpp), else a string or the first words. Every read goes through
// readMemory into a copy, so a bad pointer never faults.

#include "xfnv/crashlog/CrashLog.hpp"

namespace xfnv::crashlog {

    Image kImage = {};

    namespace {

        void words(Out& arOut, uintptr_t auiValue) {
            uint32_t auiWords[4];
            int32_t iCount = 4;
            while (iCount && !readMemory(auiWords, auiValue, 4 * iCount)) {
                --iCount;
            }
            arOut.str(" [");
            for (int32_t i = 0; i < iCount; ++i) {
                arOut.str(i ? " " : "").hex(auiWords[i]);
            }
            arOut.chr(']');
        }

        // Whether the code at auiReturn follows a call (E8 rel32, or FF /2 in its 2, 3, 6 and 7-byte forms): a return
        // address, not a function pointer or a stale code address.
        bool followsCall(uintptr_t auiReturn) {
            uint8_t acBefore[7];
            if (!readMemory(acBefore, auiReturn - 7, 7)) {
                return false;
            }
            auto callFF = [&](int32_t aiLength) {
                return acBefore[7 - aiLength] == 0xFF && (acBefore[8 - aiLength] & 0x38) == 0x10;
            };
            return acBefore[2] == 0xE8 || callFF(2) || callFF(3) || callFF(6) || callFF(7);
        }

    } // namespace

    void annotateData(Out& arOut, uintptr_t auiValue) {
        uint8_t acProbe[32];
        if (!readMemory(acProbe, auiValue, 4)) {
            return;
        }
        Dynamic kObject;
        if (dynamicType(auiValue, kObject)) {
            arOut.str(" -> ");
            className(arOut, kObject);
            if (kObject.uiComplete != auiValue) {
                arOut.str(" (complete object ").hex(kObject.uiComplete).chr(')');
            }
            identity(arOut, kObject);
            words(arOut, auiValue);
            return;
        }
        if (vtableType(auiValue, kObject)) {
            arOut.str(" -> vftable of ");
            className(arOut, kObject);
            return;
        }
        // a string: at least 4 printable characters, then its end (or more of them)
        uint32_t uiProbe = 4096 - static_cast<uint32_t>(auiValue % 4096);
        uiProbe = uiProbe < sizeof(acProbe) ? uiProbe : sizeof(acProbe);
        if (readMemory(acProbe, auiValue, uiProbe)) {
            uint32_t uiText = 0;
            while (uiText < uiProbe && acProbe[uiText] >= ' ' && acProbe[uiText] < 0x7F) {
                ++uiText;
            }
            if (uiText >= 4 && (uiText == uiProbe || acProbe[uiText] == 0)) {
                arOut.str(" -> \"").guardedStr(auiValue, 80).chr('"');
                return;
            }
        }
        arOut.str(" ->");
        words(arOut, auiValue);
    }

    void annotateValue(Out& arOut, uintptr_t auiValue, uintptr_t auiStackLow, uintptr_t auiStackHigh) {
        if (auiValue < 0x10000) {
            return;
        }
        if (isCode(auiValue)) {
            arOut.str(followsCall(auiValue) ? " ret  " : " code ");
            codeAddress(arOut, auiValue);
        }
        else if (auiValue >= auiStackLow && auiValue < auiStackHigh) {
            arOut.str(" -> stack");
        }
        else {
            annotateData(arOut, auiValue);
        }
    }

} // namespace xfnv::crashlog
