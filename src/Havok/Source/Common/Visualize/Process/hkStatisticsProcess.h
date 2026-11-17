#pragma once

#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Reconstruction.hpp"

class hkProcessContext;

// The process that sends the monitor statistics to the visual debugger. Only the members the reconstructed code
// uses are declared.
class hkStatisticsProcess : public hkReferencedObject, public hkProcess {
public:
	// The command it sends (the SDK's value).
	enum Command : int32_t {
		HK_SEND_STATISTICS_DUMP = 10,
	};

	hkStatisticsProcess(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkStatisticsProcess() override;

	int32_t getProcessTag() override;
	// Sends the monitor statistics to the visual debugger. The exe's entry in the hkProcess table takes the hkProcess
	// subobject as this.
	void step(float frameTimeInMs) override;

	// Registers the process with the process factory and keeps its tag.
	static void registerProcess();
	// DeletingDestruct's free (hkReferencedObject's); hkProcess declares its own.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	// Allocates and constructs the process on the contexts.
	static hkProcess* create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);

	hkArray<hkProcessContext*> m_contexts;
	hkArray<char> m_infoBuffer;
	hkArray<char> m_strBuffer;

	// The tag registerProcess stores.
	static inline int32_t m_tag;
};
inline constexpr fnvd::Vtable hkStatisticsProcess_vftable_hkProcess{};
inline constexpr fnvd::Vtable hkStatisticsProcess_vftable{};

// The process's name, which the exe's inlined getName returns; overlaid so the factory keeps the exe's pointer.
using hkStatisticsProcess_Name_t = const char[11];
inline hkStatisticsProcess_Name_t hkStatisticsProcess_Name{"Statistics"};
