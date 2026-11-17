#include "BSSystem/BSContentPackage.h"
#include "Reconstruction.hpp"

#include <stdint.h>

char _10A47B3;

int32_t return_0_2() {
	return 0;
}

BSContentPackage::BSContentPackage(const char16_t* apDisplayName) : m_contentData(apDisplayName) {}

const char16_t* BSContentPackage::DisplayName() const {
	return m_contentData;
}

const char* BSContentPackage::FileName() const {
	return &_10A47B3;
}
