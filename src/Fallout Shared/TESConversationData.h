#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"

class TESForm;
class TESTopic;
class TESTopicInfo;

// The links of a dialogue response to the other topics: the topics it comes from and goes to and the responses that may
// follow it. Not polymorphic. The list item types are the form types InitItem resolves the stored form IDs to.
class TESConversationData {
public:
    TESConversationData();
    ~TESConversationData();

    // Resolves the form IDs in the lists to forms of apOwner's file and drops the ones that are not of the right
    // type, with a warning.
    void InitItem(TESForm* apOwner);
    // Returns &m_listLinkFrom.
    BSSimpleList<TESTopic*>* GetLinkFromList();
    // Returns &m_listLinkTo.
    BSSimpleList<TESTopic*>* GetLinkToList();
    // Returns &m_listFollowUpInfos.
    BSSimpleList<TESTopicInfo*>* GetFollowUpInfoList();

protected:
    // Empties the three lists.
    void ClearLists();

public:
    // Copies apDataToCopy's three lists into this one's; apOwner is unused.
    void Copy(TESTopicInfo* apOwner, TESConversationData* apDataToCopy);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BSSimpleList<TESTopic*> m_listLinkFrom;
    BSSimpleList<TESTopic*> m_listLinkTo;
    BSSimpleList<TESTopicInfo*> m_listFollowUpInfos;
};
