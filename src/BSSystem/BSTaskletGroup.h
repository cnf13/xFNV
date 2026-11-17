#pragma once

class BSTasklet;
class BSTaskletGroupData;

// A group of tasklets the tasklet manager runs together; a handle to the manager's group data.
class BSTaskletGroup {
public:
    // Closes the group to new tasks and returns the manager's result.
    bool Close();
    // Opens the group for new tasks through the tasklet manager.
    bool Open();
    // Hands aTask to the tasklet manager; false when the group could not take it.
    bool AttachTask(BSTasklet& aTask, bool abLastTask);
    // Waits for the group's tasks unless the group data reports them complete.
    void WaitForCompletion(bool abPoll);

    BSTaskletGroupData* pData;
};
