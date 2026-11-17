#include "Havok/Source/Common/Visualize/hkProcessRegisterUtil.h"

#include "Havok/Source/Common/Visualize/Process/hkDebugDisplayProcess.h"
#include "Havok/Source/Common/Visualize/Process/hkInspectProcess.h"
#include "Havok/Source/Common/Visualize/Process/hkStatisticsProcess.h"

void hkProcessRegisterUtil::registerAllCommonProcesses() {
	hkDebugDisplayProcess::registerProcess();
	hkStatisticsProcess::registerProcess();
	hkInspectProcess::registerProcess();
}
