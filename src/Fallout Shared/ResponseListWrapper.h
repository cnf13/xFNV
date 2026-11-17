#pragma once

#include <cstdint>

class TESResponse;

// The responses of a topic info as a singly linked list: the head of a chain of TESResponse.
class ResponseListWrapper {
public:
	ResponseListWrapper();
	~ResponseListWrapper();

	// Appends apResponse to the list.
	void AddTail(TESResponse* apRes);
	// The head response's response ID, 0 without a list.
	int32_t GetResponseID();
	void ClearResponseList();
	// Clears this list and fills it with copies of the chain starting at apHead.
	void CopyResponseList(TESResponse* apHead);
	// The first response.
	TESResponse* QResponseList();

	TESResponse* pHead;
};

// The response list TESTopicInfo::ClearResponseList frees.
extern ResponseListWrapper kResponseList;
