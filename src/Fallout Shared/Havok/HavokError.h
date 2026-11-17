#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// The game's Havok error reporter: it hands each message to the game's message handler, and can switch off single
// ids and single reports (a file and line).
class HavokError : public hkError {
public:
	// A disabled report: the exe's copy of the file's base name and the line.
	struct hkFileLine {
		const char* m_file;
		int32_t m_line;
	};

	// Switches off the ids and reports the game does not want to hear about; pRootDir starts empty.
	HavokError();
	~HavokError() override;

	int32_t message(hkError::Message m, int32_t id, const char* description, const char* file, int32_t line) override;
	void setEnabled(int32_t id, hkBool enabled) override;
	hkBool* isEnabled(hkBool* arResult, int32_t aiId) override;
	void enableAll() override;
	// The exe passes the report's file as a case-folded base name and keeps a copy of it.
	virtual void disableReport(const char* apFile, int32_t aiLine);
	virtual void enableReport(const char* apFile, int32_t aiLine);
	// The five message handlers take the kind of message as a string ("Report" from message), the id, the
	// description, the file and the line.
	virtual void assertMessage(const char* apWhat, int32_t aiID, const char* apDescription, const char* apFile,
		int32_t aiLine);
	virtual void errorMessage(const char* apWhat, int32_t aiID, const char* apDescription, const char* apFile,
		int32_t aiLine);
	virtual void warningMessage(const char* apWhat, int32_t aiID, const char* apDescription, const char* apFile,
		int32_t aiLine);
	virtual void programFlowMessage(const char* apWhat, int32_t aiID, const char* apDescription, const char* apFile,
		int32_t aiLine);

protected:
	virtual void showString(const char* apStr);
	virtual const char* hkGetFileBaseName(const char* apPath);

	// Frees each disabled report's file name copy.
	void KillArray(hkArray<hkFileLine>& arArray);

public:
	void SetRootDir(const char* apRoot);
	bool isReportEnabled(const char* apFile, int32_t aiLine);

	// The exe's deleting destructor (0x625B10) frees through hkReferencedObject::deleteObject.
	static void operator delete(void* apMemory);

	hkArray<hkFileLine> m_disabledReports;
	// The ids switched off by setEnabled.
	hkPointerMap<int32_t, int32_t> m_disabledAssertIds;
	// The root of the game's source tree (up to "TES4"), cut off the files assertions report.
	char pRootDir[260];
};

inline constexpr fnvd::Vtable HavokError_vftable{};
