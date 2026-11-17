#include "SDK/MSVC/ehvec.h"
#include "Reconstruction.hpp"
#include <stdint.h>

void rt::eh_vector_constructor_iterator(void* apArray, size_t astSize, int32_t aiCount, rt::EhVectorCallback apfnConstructor) {
	while (--aiCount >= 0) {
		apfnConstructor(apArray);
		apArray = static_cast<char*>(apArray) + astSize;
	}
}

void __ehvec_dtor(void* apArray, uint32_t auiSize, int32_t aiCount, rt::EhVectorCallback apfnDestructor) {
	char* pElement = static_cast<char*>(apArray) + aiCount * auiSize;
	for (int32_t i = aiCount - 1; i >= 0; --i) {
		pElement -= auiSize;
		apfnDestructor(pElement);
	}
}
