#pragma once

// Registers the visual debugger's processes with the process factory.
class hkProcessRegisterUtil {
public:
	// The debug display, statistics and inspection processes.
	static void registerAllCommonProcesses();
};
