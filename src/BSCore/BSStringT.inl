template <class Char>
BSStringT<Char>::BSStringT() {
    Init();
}

template <class Char>
BSStringT<Char>::BSStringT(const Char* apString) {
    Init();
    Set(apString, 0);
}

template <class Char>
BSStringT<Char>::BSStringT(const BSStringT& aSrc) {
    Init();
    Set(aSrc.pString, 0);
}

// Releases the buffer.
template <class Char>
BSStringT<Char>::~BSStringT() {
    Set(nullptr, 0);
}

template <class Char>
BSStringT<Char>& BSStringT<Char>::operator=(const Char* apString) {
    Set(apString, 0);
    return *this;
}

// Appends apString: an empty string takes a copy, otherwise the buffer grows to the total length through Set.
template <class Char>
BSStringT<Char>& BSStringT<Char>::operator+=(const Char* apString) {
    if (apString) {
        if (pString) {
            uint32_t uiSrcLen = static_cast<uint32_t>(std::char_traits<Char>::length(apString));
            uint32_t uiTotalLen = uiSrcLen + GetLength();
            if (uiTotalLen > MaxLength()) {
                Set(pString, uiTotalLen);
            }
            std::char_traits<Char>::copy(&pString[GetLength()], apString, uiSrcLen + 1);
            SetLength(uiTotalLen);
        }
        else {
            Set(apString, 0);
        }
    }
    return *this;
}

// Appends aSrc's text, as operator+=(const Char*) does.
template <class Char>
BSStringT<Char>& BSStringT<Char>::operator+=(const BSStringT& aSrc) {
    if (aSrc.pString) {
        if (pString) {
            uint32_t uiTotalLen = aSrc.GetLength() + GetLength();
            if (uiTotalLen > MaxLength()) {
                Set(pString, uiTotalLen);
            }
            uint32_t uiCopyLen = aSrc.GetLength() + 1;
            std::char_traits<Char>::copy(&pString[GetLength()], aSrc.pString, uiCopyLen);
            SetLength(uiTotalLen);
        }
        else {
            Set(aSrc);
        }
    }
    return *this;
}

// Copies apString (null: empty) into a buffer of at least aiMaxLen characters. A longer text gets a new buffer; an
// empty text with no reserve frees the buffer. Returns whether the text is not empty.
template <class Char>
bool BSStringT<Char>::Set(const Char* apString, uint32_t aiMaxLen) {
    uint32_t uiStrLen = apString ? static_cast<uint32_t>(std::char_traits<Char>::length(apString)) : 0;
    uint32_t uiNewLen = uiStrLen > aiMaxLen ? uiStrLen : aiMaxLen;
    if (uiNewLen > MaxLength()) {
        Char* pOld = pString;
        pString = static_cast<Char*>(ourMalloc((uiNewLen + 1) * sizeof(Char)));
        if (apString) {
            std::char_traits<Char>::copy(pString, apString, uiStrLen + 1);
        }
        else {
            *pString = 0;
        }
        if (pOld) {
            ourFree(pOld);
        }
        SetMaxLength(uiNewLen);
    }
    else if (uiNewLen) {
        if (apString) {
            std::char_traits<Char>::copy(pString, apString, uiStrLen + 1);
        }
        else {
            *pString = 0;
        }
    }
    else {
        if (pString) {
            ourFree(pString);
        }
        pString = nullptr;
        SetMaxLength(0);
    }
    SetLength(uiStrLen);
    return uiStrLen != 0;
}

// Copies aSrc's text, as Set(const Char*, 0) does.
template <class Char>
bool BSStringT<Char>::Set(const BSStringT& aSrc) {
    uint32_t uiNewLen = aSrc.GetLength();
    if (uiNewLen > MaxLength()) {
        Char* pOld = pString;
        pString = static_cast<Char*>(ourMalloc((uiNewLen + 1) * sizeof(Char)));
        std::char_traits<Char>::copy(pString, aSrc.pString, uiNewLen + 1);
        if (pOld) {
            ourFree(pOld);
        }
        SetMaxLength(uiNewLen);
    }
    else if (uiNewLen) {
        std::char_traits<Char>::copy(pString, aSrc.pString, uiNewLen + 1);
    }
    else {
        if (pString) {
            ourFree(pString);
        }
        pString = nullptr;
        SetMaxLength(0);
    }
    SetLength(uiNewLen);
    return uiNewLen != 0;
}

// The stored length, or the counted one when it overflowed the 16-bit field (0xFFFF); the count is the character
// type's own, so that BSStringT<char16_t> compiles too.
template <class Char>
uint32_t BSStringT<Char>::Length() const {
    return sLen == 0xFFFF ? static_cast<uint32_t>(std::char_traits<Char>::length(pString)) : sLen;
}

// The stored length; a string of 0xFFFF characters or more is measured.
template <class Char>
uint32_t BSStringT<Char>::GetLength() const {
    return sLen == 0xFFFF ? static_cast<uint32_t>(std::char_traits<Char>::length(pString)) : sLen;
}

// Upper-cases the letters A-Z of the text in place, as _strupr_s does in the C locale.
template <class Char>
void BSStringT<Char>::MakeUpper() {
    if (pString) {
        for (Char* pChar = pString; *pChar; ++pChar) {
            if (*pChar >= 'a' && *pChar <= 'z') {
                *pChar = static_cast<Char>(*pChar - 'a' + 'A');
            }
        }
    }
}

// Compares the text with apString as BSstrcmp (strcmp: -1, 0 or 1) or, ignoring case, BSstricmp (_stricmp in the C
// locale: the difference of the first differing characters, lower-cased). A null string ranks below any text; two null
// strings are equal.
template <class Char>
int32_t BSStringT<Char>::StrCmp(const Char* apString, bool abIgnoreCase) const {
    using UChar = std::make_unsigned_t<Char>;
    if (!apString || !pString) {
        if (apString || pString) {
            return 2 * (apString == nullptr) - 1;
        }
        return 0;
    }
    for (uint32_t i = 0;; ++i) {
        int32_t iMine = static_cast<UChar>(pString[i]);
        int32_t iOther = static_cast<UChar>(apString[i]);
        if (abIgnoreCase) {
            if (iMine >= 'A' && iMine <= 'Z') {
                iMine += 'a' - 'A';
            }
            if (iOther >= 'A' && iOther <= 'Z') {
                iOther += 'a' - 'A';
            }
            if (iMine != iOther || !iMine) {
                return iMine - iOther;
            }
        }
        else if (iMine != iOther || !iMine) {
            return iMine == iOther ? 0 : (iMine < iOther ? -1 : 1);
        }
    }
}

template <class Char>
uint32_t BSStringT<Char>::MaxLength() const {
    return sMaxLen;
}

// Lengths do not wrap: a value above 0xFFFF is stored as 0xFFFF.
template <class Char>
void BSStringT<Char>::SetLength(uint32_t aiLen) {
    sLen = aiLen > 0xFFFF ? 0xFFFF : static_cast<uint16_t>(aiLen);
}

template <class Char>
void BSStringT<Char>::SetMaxLength(uint32_t aiLen) {
    sMaxLen = aiLen > 0xFFFF ? 0xFFFF : static_cast<uint16_t>(aiLen);
}

// No buffer, zero length and capacity.
template <class Char>
void BSStringT<Char>::Init() {
    pString = nullptr;
    SetLength(0);
    SetMaxLength(0);
}

// The length in bytes: GetLength, in characters of the character type, times the character size.
template <class Char>
uint32_t BSStringT<Char>::ByteLength() const {
    return GetLength() * sizeof(Char);
}

// The length plus one, through GetLength.
template <class Char>
uint32_t BSStringT<Char>::GetLenPlusOne() const {
    return GetLength() + 1;
}

template <class Char>
const Char* BSStringT<Char>::String() const {
    return pString;
}

// StrCmp on arString's text.
template <class Char>
int32_t BSStringT<Char>::StrCmp(const BSStringT& arString, bool abIgnoreCase) const {
    return StrCmp(arString.pString, abIgnoreCase);
}

// Whether the text equals arString's, ignoring case.
template <class Char>
bool BSStringT<Char>::operator==(const BSStringT& arString) const {
    return StrCmp(arString, true) == 0;
}

// Whether the text differs from arString's, ignoring case.
template <class Char>
bool BSStringT<Char>::operator!=(const BSStringT& arString) const {
    return IsNotEqual(arString) != 0;
}

// Whether the text equals apString, ignoring case.
template <class Char>
bool BSStringT<Char>::operator==(const Char* apString) const {
    return StrCmp(apString, true) == 0;
}

// Whether the text differs from apString, ignoring case.
template <class Char>
bool BSStringT<Char>::operator!=(const Char* apString) const {
    return !operator==(apString);
}

// Whether the text differs from arString's, ignoring case, as 0 or 1.
template <class Char>
int32_t BSStringT<Char>::IsNotEqual(const BSStringT& arString) const {
    return (*this == arString) ? 0 : 1;
}

// The character at auiIndex; a null string yields the character its null pointer names.
template <class Char>
Char& BSStringT<Char>::operator[](uint32_t aiIndex) {
    return pString[pString ? aiIndex : 0];
}

// Builds in apResult a copy of the text without its {comments}: text between braces (nested braces counted) and
// every closing brace are dropped.
template <class Char>
BSStringT<Char>* BSStringT<Char>::RemoveCurlyContent(BSStringT* apResult) {
    uint32_t uiOut = 0;
    uint32_t uiDepth = 0;
    BSStringT sText;
    sText.Set(String(), 0);
    for (uint32_t i = 0; i < GetLength(); ++i) {
        if (String()[i] == '{') {
            ++uiDepth;
        }
        else if (String()[i] == '}') {
            uiDepth = uiDepth ? uiDepth - 1 : 0;
        }
        if (!uiDepth && String()[i] != '}') {
            sText[uiOut] = String()[i];
            ++uiOut;
        }
    }
    if (uiOut < sText.GetLength()) {
        sText[uiOut] = 0;
    }
    sText.SetLength(uiOut);
    new (apResult) BSStringT(sText);
    return apResult;
}
