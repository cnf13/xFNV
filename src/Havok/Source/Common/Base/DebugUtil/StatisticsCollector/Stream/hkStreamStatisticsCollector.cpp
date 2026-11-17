#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/Stream/hkStreamStatisticsCollector.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/Util/hkStatisticsCollectorUtil.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamAnalyzer.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

// Not named in the PDB or IDA; name chosen here. A file-static helper of hkStreamStatisticsCollector.cpp (its callers
// _sumNodes 0xDDF680 and makeTree 0xDDF740): the child node of apParent named apcName, made when missing, with one
// more sample (usercall: apcName in eax, apNames in ebx, ret 0).
static hkMonitorStreamAnalyzer::Node* _newNode(hkMonitorStreamAnalyzer::Node* apParent, int32_t aiValue0,
	int32_t aiValue1, const char* apcName, hkArray<hkStringPtr, hkContainerHeapAllocator>* apNames);

// Finds the child of apParent named apcName ("?" for none), else makes a directory node for it, first copying the
// name into the string pool apNames when there is one; then counts one more sample and adds aiValue0 and aiValue1
// (aiValue0 again when aiValue1 is 0) to the node's first two timers.
static hkMonitorStreamAnalyzer::Node* _newNode(hkMonitorStreamAnalyzer::Node* apParent, int32_t aiValue0,
	int32_t aiValue1, const char* apcName, hkArray<hkStringPtr, hkContainerHeapAllocator>* apNames) {
	if (!aiValue1) {
		aiValue1 = aiValue0;
	}
	if (!apcName) {
		apcName = "?";
	}
	hkMonitorStreamAnalyzer::Node* pNode;
	int32_t i = 0;
	if (apParent) {
		for (; i < apParent->m_children.m_size; ++i) {
			if (!hkString::strCmp(apParent->m_children.m_data[i]->m_name, apcName)) {
				pNode = apParent->m_children.m_data[i];
				if (pNode) {
					goto found;
				}
				break;
			}
		}
	}
	if (apNames && apcName) {
		{
			FNVD_CONSTRUCTED_LOCAL(hkStringPtr, name, &hkStringPtr::Destruct, apcName);
			// EXE-INLINED none: hkArray<hkStringPtr, hkContainerHeapAllocator>::pushBack
			hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
			if (apNames->m_size == (apNames->m_capacityAndFlags & hkArrayBase<hkStringPtr>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(heap, apNames, sizeof(hkStringPtr));
			}
			hkStringPtr* pSlot = &apNames->m_data[apNames->m_size];
			if (pSlot) {
				::new (pSlot) hkStringPtr(name);
			}
			++apNames->m_size;
		}
		apcName = apNames->m_data[apNames->m_size - 1].cString();
	}
	{
		void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkMonitorStreamAnalyzer::Node));
		pNode = pMemory
			? ::new (pMemory)
				  hkMonitorStreamAnalyzer::Node(apParent, apcName, hkMonitorStreamAnalyzer::Node::NODE_TYPE_DIRECTORY)
			: nullptr;
	}
found:
	// The exe adds the integers on the x87 stack (fild, fadd) and stores the sums as floats.
	++pNode->m_count[0];
	pNode->m_value[0] = static_cast<double>(aiValue0) + pNode->m_value[0];
	++pNode->m_count[1];
	pNode->m_value[1] = static_cast<double>(aiValue1) + pNode->m_value[1];
	return pNode;
}

void hkStreamStatisticsCollector::addReferencedObject(const char* fieldName, const hkReferencedObject* obj,
	int32_t flags) {
	// EXE-INLINED none: hkStreamStatisticsCollector::hasSpace
	if (m_stream.getCapacity() > m_stream.getSize() + static_cast<int32_t>(sizeof(Command)) + 64) {
		// EXE-INLINED none: hkStreamStatisticsCollector::_isKnown
		if (obj && m_knownObjects.getWithDefault(obj, 0) == 0) {
			// EXE-INLINED none: hkStreamStatisticsCollector::_addKnown
			m_knownObjects.insert(obj, 1);
			if ((flags & 1) != 0) {
				obj->calcContentStatistics(this, findClassType(obj));
			} else {
				int32_t iSize = getReferencedObjectSize(obj);
				_startObject(fieldName, obj, iSize);
				obj->calcContentStatistics(this, findClassType(obj));
				_endObject();
			}
		}
	}
}

void hkStreamStatisticsCollector::addObject(const hkClass& cls, const char* fieldName, const void* obj, int32_t flags) {
	// EXE-INLINED none: hkStreamStatisticsCollector::hasSpace
	if (m_stream.getCapacity() > m_stream.getSize() + static_cast<int32_t>(sizeof(Command)) + 64) {
		// EXE-INLINED none: hkStreamStatisticsCollector::_isKnown
		if (obj && m_knownObjects.getWithDefault(obj, 0) == 0) {
			// EXE-INLINED none: hkStreamStatisticsCollector::_addKnown
			m_knownObjects.insert(obj, 1);
			if ((flags & 1) != 0) {
				hkStatisticsCollectorUtil::addClassContentsAll(obj, cls, this);
			} else {
				int32_t iSize = cls.getObjectSize();
				_startObject(fieldName, obj, iSize);
				hkStatisticsCollectorUtil::addClassContentsAll(obj, cls, this);
				_endObject();
			}
		}
	}
}

void hkStreamStatisticsCollector::addChunk(hkStatisticsCollector::MemoryType type, const char* name,
	const void* chunkAddress, int32_t usedSize, int32_t allocSize) {
	// EXE-INLINED none: hkStreamStatisticsCollector::hasSpace
	if (m_stream.getCapacity() > m_stream.getSize() + static_cast<int32_t>(sizeof(Command)) + 64) {
		if (usedSize || allocSize) {
			int32_t aiSize = 0;
			int32_t iPad = 0;
			if (name) {
				aiSize = hkString::strLen(name) + 1;
				iPad = aiSize % 2;
			}
			Command* pCmd = _newCommand(aiSize + iPad, &m_stream);
			pCmd->m_cmd = 'C';
			pCmd->m_slen = iPad + aiSize;
			pCmd->m_memSize = usedSize;
			pCmd->m_allocSize =
				allocSize ? hkMemoryRouter::getInstance().heap().getAllocatedSize(chunkAddress, allocSize) : 0;
			// The name follows the Command in the stream's untyped bytes.
			char* pName = reinterpret_cast<char*>(pCmd) + sizeof(Command);
			if (aiSize > 0) {
				hkString::memCpy(pName, name, aiSize);
				if (iPad) {
					pName[aiSize] = 0;
				}
			}
		}
	}
}

void hkStreamStatisticsCollector::pushDir(const char* dirName) {
	// EXE-INLINED none: hkStreamStatisticsCollector::hasSpace
	if (m_stream.getCapacity() > m_stream.getSize() + static_cast<int32_t>(sizeof(Command)) + 64) {
		int32_t aiSize = 0;
		int32_t iPad = 0;
		if (dirName) {
			aiSize = hkString::strLen(dirName) + 1;
			iPad = aiSize % 2;
		}
		Command* pCmd = _newCommand(aiSize + iPad, &m_stream);
		pCmd->m_cmd = 'P';
		pCmd->m_slen = aiSize + iPad;
		pCmd->m_memSize = 0;
		pCmd->m_allocSize = 0;
		// The name follows the Command in the stream's untyped bytes.
		char* pName = reinterpret_cast<char*>(pCmd) + sizeof(Command);
		if (aiSize > 0) {
			hkString::memCpy(pName, dirName, aiSize);
			if (iPad) {
				pName[aiSize] = 0;
			}
		}
	}
}

void hkStreamStatisticsCollector::popDir() {
	// EXE-INLINED none: hkStreamStatisticsCollector::hasSpace
	if (m_stream.getCapacity() > m_stream.getSize() + static_cast<int32_t>(sizeof(Command)) + 64) {
		Command* pCmd = _newCommand(0, &m_stream);
		pCmd->m_cmd = 'p';
		pCmd->m_slen = 0;
		pCmd->m_memSize = 0;
		pCmd->m_allocSize = 0;
	}
}

// The first node of the list is the root, made as the directory "/" when the list is empty; any other nodes are
// deleted from the back. The stream's commands then build the tree under a stack of open directories: a chunk ('C')
// adds its sizes under the current directory, a push ('P') opens one, a pop ('p') closes it, and any other command
// stops the walk before the totals are summed. The stack array's constructor and destructor are inlined.
void hkStreamStatisticsCollector::makeTree(hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator>&
											   m_nodesOut,
	hkArray<hkStringPtr, hkContainerHeapAllocator>* stringTable) {
	using NodeArray = hkArray<hkMonitorStreamAnalyzer::Node*, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(NodeArray, stack, std::nullopt, std::nullopt);
	stack.m_data = nullptr;
	stack.m_size = 0;
	stack.m_capacityAndFlags = hkArrayBase<hkMonitorStreamAnalyzer::Node*>::DONT_DEALLOCATE_FLAG;
	hkMonitorStreamAnalyzer::Node* pRoot;
	if (m_nodesOut.m_size <= 0) {
		void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkMonitorStreamAnalyzer::Node));
		if (pMemory) {
			pRoot = ::new (pMemory)
				hkMonitorStreamAnalyzer::Node(nullptr, "/", hkMonitorStreamAnalyzer::Node::NODE_TYPE_DIRECTORY);
		} else {
			pRoot = nullptr;
		}
		hkMemoryRouter& router = hkMemoryRouter::getInstance();
		if (m_nodesOut.m_size ==
			(m_nodesOut.m_capacityAndFlags & hkArrayBase<hkMonitorStreamAnalyzer::Node*>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*router.m_heap, &m_nodesOut, sizeof(hkMonitorStreamAnalyzer::Node*));
		}
		m_nodesOut.m_data[m_nodesOut.m_size] = pRoot;
		++m_nodesOut.m_size;
	} else {
		pRoot = m_nodesOut.m_data[0];
		if (m_nodesOut.m_size > 1) {
			do {
				hkMonitorStreamAnalyzer::Node* pNode = m_nodesOut.m_data[m_nodesOut.m_size - 1];
				if (pNode) {
					FNVD_DESTRUCT(hkMonitorStreamAnalyzer::Node)(pNode);
					hkMemoryRouter::getInstance().heap().blockFree(pNode, sizeof(hkMonitorStreamAnalyzer::Node));
				}
				--m_nodesOut.m_size;
			} while (m_nodesOut.m_size > 1);
		}
	}
	{
		hkMemoryRouter& router = hkMemoryRouter::getInstance();
		if (stack.m_size == (stack.m_capacityAndFlags & hkArrayBase<hkMonitorStreamAnalyzer::Node*>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*router.m_heap, &stack, sizeof(hkMonitorStreamAnalyzer::Node*));
		}
		stack.m_data[stack.m_size] = pRoot;
		++stack.m_size;
	}
	for (int32_t iOffset = 0; iOffset < m_stream.m_size;) {
		// The stream is untyped bytes, as the exe reads it: each Command is followed by its name.
		const Command* pCmd = reinterpret_cast<const Command*>(m_stream.m_data + iOffset);
		const char* pcName = pCmd->m_slen ? reinterpret_cast<const char*>(pCmd) + sizeof(Command) : nullptr;
		const int32_t iStep = pCmd->m_slen + static_cast<int32_t>(sizeof(Command));
		if (pCmd->m_cmd == 'C') {
			_newNode(stack.m_data[stack.m_size - 1], pCmd->m_memSize, pCmd->m_allocSize, pcName, stringTable);
		} else if (pCmd->m_cmd == 'P') {
			hkMonitorStreamAnalyzer::Node* pDir =
				_newNode(stack.m_data[stack.m_size - 1], pCmd->m_memSize, pCmd->m_allocSize, pcName, stringTable);
			hkMemoryRouter& router = hkMemoryRouter::getInstance();
			if (stack.m_size ==
				(stack.m_capacityAndFlags & hkArrayBase<hkMonitorStreamAnalyzer::Node*>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*router.m_heap, &stack, sizeof(hkMonitorStreamAnalyzer::Node*));
			}
			stack.m_data[stack.m_size] = pDir;
			++stack.m_size;
		} else if (pCmd->m_cmd == 'p') {
			--stack.m_size;
		} else {
			goto done;
		}
		iOffset += iStep;
	}
	_sumNodes(pRoot, stringTable);
done:
	// EXE-INLINED none: hkArray<hkMonitorStreamAnalyzer::Node*>::~hkArray
	hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
	stack.m_size = 0;
	if (stack.m_capacityAndFlags >= 0) {
		heap.bufFree(stack.m_data,
			(stack.m_capacityAndFlags & hkArrayBase<hkMonitorStreamAnalyzer::Node*>::CAPACITY_MASK) *
				static_cast<int32_t>(sizeof(hkMonitorStreamAnalyzer::Node*)));
	}
}

void hkStreamStatisticsCollector::beginSnapshot(int32_t statisticClasses) {
	m_knownObjects.m_map.clear();
}

void hkStreamStatisticsCollector::endSnapshot() {
	m_knownObjects.m_map.clear();
}

void hkStreamStatisticsCollector::reset() {
	m_knownObjects.m_map.clear();
	m_stream.m_size = 0;
}

// The exe's body builds the node tree and deletes it again: the report output is compiled out, so outstream and
// reportLevel are unused (it still pops both, ret 8). The nodes are deleted from the back, popping each (the size is
// decremented in memory and the data pointer reloaded every pass); delete is the node destructor, then a blockFree of
// sizeof(Node) (0x58) on the heap. The local array's destructor is inlined.
void hkStreamStatisticsCollector::writeStatistics(hkOstream& outstream, int32_t reportLevel) {
	FNVD_INLINED_LOCAL(hkArray<hkMonitorStreamAnalyzer::Node*>, nodes, std::nullopt, std::nullopt);
	nodes.m_data = nullptr;
	nodes.m_size = 0;
	nodes.m_capacityAndFlags = hkArrayBase<hkMonitorStreamAnalyzer::Node*>::DONT_DEALLOCATE_FLAG;
	makeTree(nodes, nullptr);
	for (; nodes.m_size != 0; --nodes.m_size) {
		hkMonitorStreamAnalyzer::Node* pNode = nodes.m_data[nodes.m_size - 1];
		if (pNode) {
			FNVD_DESTRUCT(hkMonitorStreamAnalyzer::Node)(pNode);
			hkMemoryRouter::getInstance().heap().blockFree(pNode, sizeof(hkMonitorStreamAnalyzer::Node));
		}
	}

	// EXE-INLINED none: hkArray<hkMonitorStreamAnalyzer::Node*>::~hkArray
	hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
	const int32_t capacityAndFlags = nodes.m_capacityAndFlags;
	nodes.m_size = 0;
	if (capacityAndFlags >= 0)
		heap.bufFree(nodes.m_data,
			(capacityAndFlags & hkArrayBase<hkMonitorStreamAnalyzer::Node*>::CAPACITY_MASK) *
				sizeof(hkMonitorStreamAnalyzer::Node*));
}

void hkStreamStatisticsCollector::_startObject(const char* name, const void* object, int32_t size) {
	int32_t aiSize = 0;
	int32_t iPad = 0;
	if (name) {
		aiSize = hkString::strLen(name) + 1;
		iPad = aiSize % 2;
	}
	Command* pCmd = _newCommand(aiSize + iPad, &m_stream);
	pCmd->m_cmd = 'P';
	pCmd->m_slen = iPad + aiSize;
	pCmd->m_memSize = size;
	pCmd->m_allocSize = size ? hkMemoryRouter::getInstance().heap().getAllocatedSize(object, size) : 0;
	// The name follows the Command in the stream's untyped bytes.
	char* pName = reinterpret_cast<char*>(pCmd) + sizeof(Command);
	if (aiSize > 0) {
		hkString::memCpy(pName, name, aiSize);
		if (iPad) {
			pName[aiSize] = 0;
		}
	}
}

void hkStreamStatisticsCollector::_endObject() {
	popDir();
}

hkStreamStatisticsCollector::Command* _newCommand(int32_t a1, hkArray<char, hkContainerHeapAllocator>* r_esi) {
	// The stream is a byte array; the commands are laid in it (the SDK's own cast).
	hkStreamStatisticsCollector::Command* pCmd;
	int32_t iSize = r_esi->m_size;
	if (iSize <= 0) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		if (r_esi->m_size == (r_esi->m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*Instance->m_heap, r_esi, sizeof(char));
		}
		int32_t v5 = r_esi->m_size;
		pCmd = reinterpret_cast<hkStreamStatisticsCollector::Command*>(&r_esi->m_data[v5]);
		r_esi->m_size = v5 + 1;
		// The exe's inlined setSize(0) fetches the heap for a shrink its capacity test never takes.
		hkMemoryRouter::getInstance();
		r_esi->m_size = 0;
	} else {
		pCmd = reinterpret_cast<hkStreamStatisticsCollector::Command*>(&r_esi->m_data[iSize]);
	}
	int32_t v8 = a1 + r_esi->m_size + static_cast<int32_t>(sizeof(hkStreamStatisticsCollector::Command));
	hkMemoryAllocator* v9 = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v10 = r_esi->m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
	if (v10 < v8) {
		int32_t v11 = 2 * v10;
		if (v8 >= v11) {
			v11 = v8;
		}
		hkArrayUtil::_reserve(v9, r_esi, v11, sizeof(char));
	}
	r_esi->m_size = v8;
	return pCmd;
}

// Sums the children first. A node with children and its own value gets that value moved into a child "self" (found
// or made by _newNode, which adds the value); a new "self", appended last, is moved to the front. The node's value
// is then the sum of its children's. The values are truncated to ints (_ftol2), the second one first.
void _sumNodes(hkMonitorStreamAnalyzer::Node* a1, hkArray<hkStringPtr, hkContainerHeapAllocator>* a2) {
	if (a1 && a1->m_children.m_size) {
		for (int32_t i = 0; i < a1->m_children.m_size; ++i) {
			_sumNodes(a1->m_children.m_data[i], a2);
		}
		if (a1->m_value[0] != 0.0f) {
			const int32_t iAllocated = static_cast<int32_t>(a1->m_value[1]);
			const int32_t iUsed = static_cast<int32_t>(a1->m_value[0]);
			hkMonitorStreamAnalyzer::Node* pSelf = _newNode(a1, iUsed, iAllocated, "self", a2);
			if (a1->m_children.m_data[a1->m_children.m_size - 1] == pSelf) {
				--a1->m_children.m_size;
				a1->m_children._insertAt(hkMemoryRouter::getInstance().heap(), 0, pSelf);
			}
		}
		a1->m_value[0] = 0.0f;
		a1->m_value[1] = 0.0f;
		for (int32_t i = 0; i < a1->m_children.m_size; ++i) {
			hkMonitorStreamAnalyzer::Node* pChild = a1->m_children.m_data[i];
			a1->m_value[0] = pChild->m_value[0] + a1->m_value[0];
			a1->m_value[1] = pChild->m_value[1] + a1->m_value[1];
		}
	}
}
