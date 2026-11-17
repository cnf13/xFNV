#include "BSCore/BSCriticalSection.h"

BSCriticalSection::BSCriticalSection() {
	InitializeCriticalSection(&CriticalSection);
}

BSCriticalSection::~BSCriticalSection() {
	DeleteCriticalSection(&CriticalSection);
}

void BSCriticalSection::Lock(const char* apName) {
	EnterCriticalSection(&CriticalSection);
}

void BSCriticalSection::Lock() {
	EnterCriticalSection(&CriticalSection);
}

void BSCriticalSection::Unlock() {
	LeaveCriticalSection(&CriticalSection);
}

bool BSCriticalSection::TryLock() {
	return TryEnterCriticalSection(&CriticalSection) != 0;
}
