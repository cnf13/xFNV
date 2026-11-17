#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"

// Stock error reporter: it formats each message and hands the text to an error report function, and keeps the ids
// switched off by setEnabled. The game's own reporter (HavokError) derives from hkError directly. isEnabled takes its
// hkBool result as its first argument.
class hkDefaultError : public hkError {
public:
	hkDefaultError(hkErrorReportFunction errorReportFunction, void* errorReportObject);
	~hkDefaultError() override;

	int32_t message(hkError::Message msg, int32_t id, const char* description, const char* file, int32_t line) override;
	void setEnabled(int32_t id, hkBool enabled) override;
	hkBool* isEnabled(hkBool* arResult, int32_t aiId) override;
	void enableAll() override;
	void sectionBegin(int32_t id, const char* sectionName) override;
	void sectionEnd() override;

	// Formats the message and passes it to the error report function.
	void showMessage(const char* what, int32_t id, const char* desc, const char* file, int32_t line, hkBool stackTrace);

	// The ids switched off by setEnabled.
	hkPointerMap<int32_t, int32_t> m_disabledAssertIds;
	// The ids of the sections that are open.
	hkArray<int32_t> m_sectionIds;
	hkErrorReportFunction m_errorFunction;
	void* m_errorObject;
};
