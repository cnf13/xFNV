#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMenu/IDebugText.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class DebugLine;
class DebugText;
class DebugTextExtraData;
class NiBillboardNode;

// One line of debug text: where it is drawn, its node, the text, when it dies and its colour. The PDB's
// DebugText::DebugTextData (field names IDA's); it is declared here, at namespace scope, as DebugLine so that the
// NiTList and NiTPointerListBase headers can forward declare their DebugLine* specializations, and DebugText names
// it DebugTextData. The exe has one 44-byte type: 0xA11880 constructs both the 200 lines DebugText::Construct builds
// and the floating-text lines AddFloatingText allocates (0x2C bytes), and Print 0xA0F8B0 stores such a line in
// sPrintData::pTextData.
class DebugLine {
public:
	DebugLine();
	~DebugLine();

	float fX;
	float fY;
	int32_t iJustify;
	NiPointer<NiNode> spNode;
	BSStringT<char> pText;
	float fDie;
	NiColorA Color;
};

// The on-screen debug text lines. Only the members the reconstructed functions use are declared.
class DebugText : public IDebugText {
public:
	// Values from the Xbox PDB.
	enum DEBUG_TEXT_PRINT_TYPE : int32_t {
		DTPT_BY_INDEX = 0x0,
		DTPT_SPECIFIED = 0x1,
	};

	// The PDB's name of DebugLine (above).
	using DebugTextData = DebugLine;

	// A print queued from another thread: the line it is for, how it is addressed and its font (IDA's names).
	struct sPrintData {
		DebugTextData* pTextData;
		DEBUG_TEXT_PRINT_TYPE eType;
		int32_t iIndex;
		int32_t iLineNumber;
		float fDie;
		int32_t iFont;
	};

	DebugText();
	~DebugText();

	void Reset() override;
	void SkipLine(DEBUGTEXT_LOCATION aeLocation, int32_t aiSkip) override;
	// Left-justified at the left (0xA0E470; IDA PrintX_Centered).
	int32_t PrintX(int32_t aiX, const char* apFormatString, ...) override;
	int32_t PrintX(int32_t aiX, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify, const char* apFormatString,
		...) override;
	// Thiscall, ret 0x18, 0xA0E4F0.
	int32_t VAPrintX(int32_t aiX, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify,
		const char* apFormatString, va_list aArgs, const NiColorA* apColor) override;
	int32_t PrintLine(DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify, const char* apFormatString,
		...) override;
	// Thiscall, ret 0x14, 0xA0E5C0.
	int32_t VAPrintLine(DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify, const char* apFormatString,
		va_list aArgs, const NiColorA* apColor) override;
	// 0xA0E690 (IDA PrintXC_Centered).
	int32_t PrintXC(int32_t aiX, const NiColorA& aColor, const char* apFormatString, ...) override;
	int32_t PrintXC(int32_t aiX, const NiColorA& aColor, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify,
		const char* apFormatString, ...) override;
	int32_t PrintLineC(const NiColorA& aColor, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify,
		const char* apFormatString, ...) override;
	int32_t GetCurrentLine(DEBUGTEXT_LOCATION aeLocation) override;
	void SetCurrentLine(DEBUGTEXT_LOCATION aeLoc, int32_t aiLine) override;

	// Prints the lines queued from other threads.
	void HandleQueuedPrints();
	// Draws apText at (afX, afY) on line aiLine_num for afDie seconds.
	int32_t Print(const char* apText, float afX, float afY, int32_t aiJustify, int32_t aiLine_num, float afDie,
		int32_t aiFont, const NiColorA* apColor);
	// Removes every debug text line.
	void DeleteAllDebugText();
	const char* GetDebugText(float afX, float afY, int32_t aiJustify);
	// Thiscall, ret 0x18, 0xA0FE20.
	NiNode* CreateDebugText(const char* apText, float afX, float afY, int32_t aiJustify, int32_t aiFont,
		const NiColorA* apColor);
	// Inferred type: apBuf: char* from 0xa10518 (passed to std::strncpy 0)
	bool GetDebugText_0(char* apBuf, float afX, float afY, int32_t aiJustify);
	// Queues apString for the line at index a2.
	// Inferred type: apString: char* from 0xa0f7bd (passed to BSStringT<char>::Set 0)
	int32_t PrintByIndex(char* apString, int32_t a2);

	// The singleton, created when abCreate is set.
	static DebugText* Instance(bool abCreate);
	// The per-frame update of the debug text.
	static void Idle();
	// The extra data of the floating text line on apTarget whose height is aiHeight; apString is unused. Stores the
	// line's old center in aiOldCenter; null when there is none.
	static DebugTextExtraData* GetFloatingText(NiNode* apTarget, const char* apString, int32_t aiHeight,
		int32_t& aiOldCenter);
	// Shows apText over apObject: an existing line for the object only gets the new die time, otherwise the object's
	// billboard is replaced and a line is queued on the floating-text list. False when the font makes no shape.
	static bool AddFloatingText(NiNode* apObject, const char* apText, int32_t aiHeight, float afDie);
	// The PDB's other AddFloatingText: the NiNode overload on apObject when it is a NiNode,
	// else on its parent when that is one; false when neither is (cdecl, 0xA11470).
	static bool AddFloatingText(NiAVObject* apObject, const char* apText, int32_t aiHeight, float afDie);
	// Removes the floating text line(s) of aiHeight from apTarget's node, or from its parent node when apTarget
	// itself is no node; a negative aiHeight removes every line.
	static void ClearFloatingText(NiAVObject* apTarget, int32_t aiHeight);
	// Removes the floating text line(s) of aiHeight from the node apTarget; a negative aiHeight removes every line.
	static void ClearFloatingText(NiNode* apTarget, int32_t aiHeight);
	// IDA name FloatingText::FloatingText (not a constructor); not in the PDB; name chosen here. A billboard node named
	// "FloatingText" holding apText in font aiFont, turned to face the camera, scaled by |afScale| and alpha
	// blended; null when the font makes no shape (cdecl, 0xA10770).
	static NiBillboardNode* CreateBillboardText(const char* apText, float afStartX, float afStartY, float afScale,
		int32_t aiFont, int32_t aiFlags, NiColorA* apColor, bool abPrepareObject);
	static void Lock();
	static void Unlock();
	static void Free();

	// PDB name (IDA kLines): the debug text lines (DeleteAllDebugText 0xA0DE80 walks all 200).
	DebugTextData pDebugTextList[200];
	// PDB xFloatingTextList (IDA NiTListBase kFloatingText): the floating-text lines, whose nodes are separate heap
	// blocks (NiTPointerListBase<DebugLine*>::Remove 0xA11C40 frees them with MemoryManager::Deallocate).
	NiTList<DebugLine*> xFloatingTextList;
	// PDB name (IDA kPrintData): the prints queued from other threads (Print 0xA0F8B0 walks iSize items).
	BSSimpleArray<sPrintData*> xPrintData;
	// PDB names (IDA the same).
	int32_t iFontHeight;
	int32_t iHeight[3];
	int32_t iXLocation[3];

	static inline DebugText* pInstance;
	static _RTL_CRITICAL_SECTION cs;
};

// Extra data that holds a debug text: the billboard node it is drawn on, the string, and its height and center.
class DebugTextExtraData : public NiExtraData {
public:
	DebugTextExtraData(NiNode* apOwner, const char* apText, int32_t aiHeight, int32_t aiCenter);
	~DebugTextExtraData() override;

	const NiRTTI* GetRTTI() const override;

	NiNode* pBillboard;
	BSStringT<char> xString;
	int32_t iHeight;
	int32_t iOldCenter;

	static NiRTTI ms_RTTI;
};

extern SettingT<INISettingCollection> iDebugTextFont;
inline constexpr fnvd::Vtable DebugText_vftable{};
inline constexpr fnvd::Vtable DebugTextExtraData_vftable{};

// The rDebugTextColor:Menu INI setting (PDB xDebugTextColor).
// INISettingCollection: 0xfb35ed calls 0x44f570.
extern SettingT<INISettingCollection> xDebugTextColor;
