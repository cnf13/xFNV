#pragma once

#include <cstdint>

class TESTopic;

class TopicData {
public:
	TESTopic* pTopic;
	uint32_t uiFormID;
	const char* pcText;
};
