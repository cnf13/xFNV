#include "BSSystem/BSHash.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiEndian.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cctype>

BSHash::BSHash(uint8_t aucLast, uint8_t aucLast2, uint8_t aucLength, uint8_t aucFirst, uint32_t aiCRC)
	: cLast(aucLast), cLast2(aucLast2), cLength(aucLength), cFirst(aucFirst), iCRC(aiCRC) {}

BSHash::BSHash(const char* apString, HASHPROCESS aeProcess) {
	const char* pExtension = nullptr;
	char cFile[264];
	char cDrive[16];
	char cDir[264];
	char cExt[16];
	if (aeProcess != HASH_NORMAL) {
		cDrive[0] = 0;
		cDir[0] = 0;
		cFile[0] = 0;
		cExt[0] = 0;
		_splitpath_s(apString, cDrive, 16u, cDir, 260u, cFile, 260u, cExt, 16u);
		if (aeProcess == HASH_DIRECTORY) {
			// The directory: drive and path, "." when both are empty, without a trailing backslash.
			strcpy_s(cFile, 260, cDrive);
			strcat_s(cFile, 260, cDir);
			if (!cFile[0]) {
				strcpy_s(cFile, 260, ".");
			}
			size_t uiLength = std::strlen(cFile);
			if (cFile[uiLength - 1] == '\\') {
				cFile[uiLength - 1] = 0;
			}
		} else {
			pExtension = cExt;
		}
		apString = cFile;
	}
	Create(apString, pExtension, *this);
}

uint32_t BSHash::GetAll() const {
	return static_cast<uint32_t>(static_cast<uint8_t>(cLast)) |
		static_cast<uint32_t>(static_cast<uint8_t>(cLast2)) << 8 |
		static_cast<uint32_t>(static_cast<uint8_t>(cLength)) << 16 |
		static_cast<uint32_t>(static_cast<uint8_t>(cFirst)) << 24;
}

uint8_t BSHash::ExtensionIndex() const {
	return static_cast<uint8_t>(((static_cast<uint8_t>(cLast2) & 0x80) >> 7) +
		((static_cast<uint8_t>(cLast) & 0x80) >> 6) + ((static_cast<uint8_t>(cFirst) & 0x80) >> 5));
}

int32_t BSHash::Compare(const BSHash& arHash) const {
	if (iCRC < arHash.iCRC || (iCRC == arHash.iCRC && GetAll() < arHash.GetAll()))
		return -1;
	if (iCRC > arHash.iCRC || (iCRC == arHash.iCRC && GetAll() > arHash.GetAll()))
		return 1;
	return 0;
}

int32_t BSHash::WildCardCompare(const BSHash& arHash) const {
	if ((static_cast<uint8_t>(cFirst) & 0x7F) != '*') {
		if (iCRC < arHash.iCRC)
			return -1;
		if (iCRC > arHash.iCRC)
			return 1;
		if (cLength != arHash.cLength)
			return 1;
	}
	return ExtensionIndex() != arHash.ExtensionIndex();
}

void BSHash::sub_AFD960() {
	NiEndian::Swap32(reinterpret_cast<char*>(&iCRC), 1u);
}

// KEEP: the exe compares the first dword of its 16-byte extension buffer with each HashFileExtensionStrings entry
// (0xAFD7B9) after writing only the extension's length + 1 bytes there, so an extension shorter than three
// characters reads uninitialised stack bytes.
void BSHash::Create(const char* apString, const char* apExtension, BSHash& arHash) {
	arHash.cFirst = std::tolower(static_cast<uint8_t>(*apString));
	arHash.iCRC = 0;
	uint32_t icrc = 0;
	if (arHash.cFirst) {
		uint32_t c;
		const char* cp = apString + 1;
		// The exe tests cp[0..3] here and then skips a dead branch (xor eax, eax; je at 0xAFD57E): reads only.
		if (cp[0] && cp[1]) {
			uint32_t ilen = 3;
			for (const char* cp2 = cp + 2; *cp2; ++cp2) {
				c = std::tolower(static_cast<uint8_t>(*cp));
				icrc = (icrc << 16) + (c + (icrc << 6)) - icrc;
				++ilen;
				++cp;
			}
			arHash.cLength = ilen;
			arHash.iCRC = icrc;
			arHash.cLast2 = std::tolower(static_cast<uint8_t>(cp[0]));
			arHash.cLast = std::tolower(static_cast<uint8_t>(cp[1]));
		} else {
			if (*cp) {
				arHash.cLast = std::tolower(static_cast<uint8_t>(*cp));
				arHash.cLength = 2;
			} else {
				arHash.cLength = 1;
				arHash.cLast = arHash.cFirst;
			}
			arHash.cLast2 = 0;
		}
		if (apExtension) {
			char acExtension[16];
			char* cpe = acExtension;
			cp = apExtension;
			icrc = 0;
			while (*cp) {
				c = std::tolower(static_cast<uint8_t>(*cp));
				*cpe++ = static_cast<char>(c);
				icrc = (icrc << 16) + (c + (icrc << 6)) - icrc;
				++cp;
			}
			*cpe = 0;
			arHash.iCRC += icrc;
			// The exe reads the buffer's first four bytes as one dword (raw memory, untyped in the exe).
			uint32_t uiExtension = *reinterpret_cast<uint32_t*>(acExtension);
			for (uint32_t i = 0; i < 6; ++i) {
				// The exe reads each entry's first four bytes as one dword (raw memory, untyped in the exe).
				if (*reinterpret_cast<uint32_t*>(HashFileExtensionStrings[i]) == uiExtension) {
					arHash.cFirst += (i << 5) & 0x80;
					arHash.cLast += (i << 6) & 0x80;
					arHash.cLast2 += (i << 7) & 0x80;
					return;
				}
			}
		}
	} else {
		arHash.cLength = 0;
		arHash.cLast2 = 0;
		arHash.cLast = 0;
	}
}
