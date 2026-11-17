#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpMouseSpringAction;
class hkpWorld;

// The viewer that lets the visual debugger's client pick a body and drag it with a mouse spring.
class hkpMousePickingViewer : public hkpWorldViewerBase {
public:
	enum Command {
		HK_PICK_OBJECT = 0xB0,
		HK_DRAG_OBJECT = 0xB1,
		HK_RELEASE_OBJECT = 0xB2,
	};

	hkpMousePickingViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkpMousePickingViewer() override;

	int32_t getProcessTag() override { return m_tag; }
	void getConsumableCommands(uint8_t*& commands, int32_t& numCommands) override;
	void consumeCommand(uint8_t command) override;
	void worldRemovedCallback(hkpWorld* world) override;

	void dragObject(const hkVector4& newWorldSpacePoint);
	hkBool* pickObject(hkBool* apResult, uint64_t id, const hkVector4& worldPosition);
	// Removes the mouse spring from the world, wakes its body and releases it.
	void releaseObject();

	static hkProcess* create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	// Registers the viewer under its name with the process factory.
	static void registerViewer();

	hkpWorld* m_currentWorld;
	hkpMouseSpringAction* m_mouseSpring;
	float m_mouseSpringMaxRelativeForce;

	// The viewer's process tag, given by the process factory when the viewer is registered.
	static inline int32_t m_tag;
};

// The three command bytes getConsumableCommands hands out with a count of 3.
using hkpMousePickingViewer_cmdsArray = uint8_t[3];
inline hkpMousePickingViewer_cmdsArray hkpMousePickingViewer_cmds{176, 177, 178};
inline constexpr fnvd::Vtable hkpMousePickingViewer_vftable{};
inline constexpr fnvd::Vtable hkpMousePickingViewer_vftable_hkProcess{};
inline constexpr fnvd::Vtable hkpMousePickingViewer_vftable_hkpPhysicsContextWorldListener{};
