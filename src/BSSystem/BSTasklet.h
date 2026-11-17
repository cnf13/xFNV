#pragma once

class BSTaskletData;

// A handle a tasklet group runs: the data whose Process the tasklet thread calls.
class BSTasklet {
public:
	BSTasklet();
	virtual ~BSTasklet();

	void SetTaskData(BSTaskletData* apData);

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	BSTaskletData* pData;
};
