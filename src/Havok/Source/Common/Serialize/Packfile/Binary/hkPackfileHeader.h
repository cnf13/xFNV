#pragma once

#include <cstdint>

class hkClass;
class hkTypeInfo;

// The header at the start of a binary packfile: the magic, the version and layout it was written with, and where its
// contents start.
class hkPackfileHeader {
public:
	// The reflection class hkPackfileHeaderClass.
	static const hkClass& staticClass();

	int32_t m_magic[2];
	int32_t m_userTag;
	int32_t m_fileVersion;
	uint8_t m_layoutRules[4];
	int32_t m_numSections;
	int32_t m_contentsSectionIndex;
	int32_t m_contentsSectionOffset;
	int32_t m_contentsClassNameSectionIndex;
	int32_t m_contentsClassNameSectionOffset;
	char m_contentsVersion[16];
	int32_t m_pad[2];
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkPackfileHeaderTypeInfo;
