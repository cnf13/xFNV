#pragma once

#include "Reconstruction.hpp"

class BSTasklet;
class BSTaskletGroupData;

// A group of tasklets the tasklet manager runs together; a handle to the manager's group data.
class BSTaskletGroup {
public:
	// Closes the group to new tasks; the exe leaves the manager's result in eax (thiscall, ret 0, 0x6C09D0).
	bool Close();
	// Opens the group for new tasks through the tasklet manager.
	bool Open();
	// Hands aTask to the tasklet manager; false when the group could not take it.
	bool AttachTask(BSTasklet& aTask, bool abLastTask);
	// Waits for the group's tasks unless the group data reports them complete.
	void WaitForCompletion(bool abPoll);

	BSTaskletGroupData* pData;
};
