#pragma once

#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"

class TESForm;
class TESTopic;
class TESTopicInfo;

// The links of a dialogue response to the other topics: the topics it comes from and goes to and the responses that
// may follow it. Not polymorphic. IDA types every list as BSSimpleList<void*> and names the members kLinkFrom, kLinkTo
// and kFollowUp and ClearLists Reset; the item types are the form types InitItem resolves the stored form IDs to.
class TESConversationData {
public:
	TESConversationData();
	~TESConversationData();

	// Resolves the form IDs in the lists to forms of apOwner's file and drops the ones that are not of the right
	// type, with a warning.
	void InitItem(TESForm* apOwner);
	// &m_listLinkFrom, through the exe's shared identity getter (thiscall, ret 0, 0x6815C0).
	BSSimpleList<TESTopic*>* GetLinkFromList();
	// &m_listLinkTo, through the exe's shared "this + 8" getter (thiscall, ret 0, 0x413F40).
	BSSimpleList<TESTopic*>* GetLinkToList();
	// &m_listFollowUpInfos, through the exe's shared "this + 0x10" getter (thiscall, ret 0,
	// 0x460140).
	BSSimpleList<TESTopicInfo*>* GetFollowUpInfoList();

protected:
	// Empties the three lists.
	void ClearLists();

public:
	// Copies apDataToCopy's three lists into this one's; apOwner is unused.
	void Copy(TESTopicInfo* apOwner, TESConversationData* apDataToCopy);

	// The exe's deleting destructor (0x61D110) frees through ourFree.
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSSimpleList<TESTopic*> m_listLinkFrom;
	BSSimpleList<TESTopic*> m_listLinkTo;
	BSSimpleList<TESTopicInfo*> m_listFollowUpInfos;
};
