#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <string.h>

// An axis-aligned rectangle given by its four edges.
template <class T>
class NiRect {
public:
    // A new viewer string describing the rectangle.
    char* GetViewerString(const char* pPrefix) const;

    T m_left;
    T m_right;
    T m_top;
    T m_bottom;
};

// The prefix and the four edges in a new string from the Gamebryo heap.
template <class T>
char* NiRect<T>::GetViewerString(const char* pPrefix) const {
    size_t stLength = strlen(pPrefix) + 64;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString,
        stLength,
        "%s = (L=%g,R=%g,T=%g,B=%g)",
        pPrefix,
        static_cast<double>(m_left),
        static_cast<double>(m_right),
        static_cast<double>(m_top),
        static_cast<double>(m_bottom)
    );
    return pcString;
}
