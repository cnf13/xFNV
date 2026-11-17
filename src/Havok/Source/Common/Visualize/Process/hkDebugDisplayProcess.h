#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkProcessContext;

// The process that forwards the debug display calls to the visual debugger's client stream. Its CreateObject
// (create) allocates 0x24 bytes; hkProcess sits at +0x8 and hkDebugDisplayHandler at +0x20.
class hkDebugDisplayProcess : public hkReferencedObject, public hkProcess, public hkDebugDisplayHandler {
public:
	hkDebugDisplayProcess();
	~hkDebugDisplayProcess() override;

	int32_t getProcessTag() override { return m_tag; }
	hkResult addGeometry(const hkArrayBase<hkDisplayGeometry*>&, const hkTransform&, hkUlong, int32_t,
		hkUlong) override;
	hkResult addGeometryInstance(hkUlong, const hkTransform&, hkUlong, int32_t, hkUlong) override;
	hkResult setGeometryColor(int32_t, hkUlong, int32_t) override;
	hkResult updateGeometry(const hkTransform&, hkUlong, int32_t) override;
	hkResult removeGeometry(hkUlong, int32_t, hkUlong) override;
	// Havok SDK signature (hkVisualize.lib, tied by bytes,vtable).
	hkResult updateCamera(const hkVector4& from, const hkVector4& to, const hkVector4& up, float nearPlane,
		float farPlane, float fov, const char* name) override;
	// Havok SDK signature (hkVisualize.lib, tied by vtable).
	hkResult updateBehavior(const hkArrayBase<int32_t>& wordVarIdx, const hkArrayBase<int32_t>& wordStack,
		const hkArrayBase<int32_t>& quadVarIdx, const hkArrayBase<hkVector4>& quadStack,
		const hkArrayBase<char*>& activeNodes, const hkArrayBase<int32_t>& activeStateIds,
		const hkArrayBase<int32_t>& activeTransitions, const hkArrayBase<hkQsTransform>& transforms) override;
	hkResult displayPoint(const hkVector4&, int32_t, int32_t, int32_t) override;
	hkResult displayLine(const hkVector4&, const hkVector4&, int32_t, int32_t, int32_t) override;
	hkResult displayTriangle(const hkVector4&, const hkVector4&, const hkVector4&, int32_t, int32_t, int32_t) override;
	hkResult displayText(const char*, int32_t, int32_t, int32_t) override;
	hkResult display3dText(const char*, const hkVector4&, int32_t, int32_t, int32_t) override;
	hkResult displayAnnotation(const char*, int32_t, int32_t) override;
	hkResult displayGeometry(const hkArrayBase<hkDisplayGeometry*>&, const hkTransform&, int32_t, int32_t,
		int32_t) override;
	hkResult displayGeometry(const hkArrayBase<hkDisplayGeometry*>&, int32_t, int32_t, int32_t) override;
	// Havok SDK signature (hkVisualize.lib, tied by vtable).
	hkResult sendMemStatsDump(const char* data, int32_t length) override;
	hkResult holdImmediate() override;

	// DeletingDestruct's free (hkReferencedObject's); hkProcess and hkDebugDisplayHandler declare their own.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	// Registers the process with the process factory and keeps its tag.
	static void registerProcess();
	static hkProcess* create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);

	// The tag registerProcess stores.
	static inline int32_t m_tag;
};
inline constexpr fnvd::Vtable hkDebugDisplayProcess_vftable_hkDebugDisplayHandler{};
inline constexpr fnvd::Vtable hkDebugDisplayProcess_vftable_hkProcess{};
inline constexpr fnvd::Vtable hkDebugDisplayProcess_vftable{};

// The process's name, which the exe's inlined getName returns; overlaid so the factory keeps the exe's pointer.
using hkDebugDisplayProcess_Name_t = const char[14];
inline hkDebugDisplayProcess_Name_t hkDebugDisplayProcess_Name{"Debug Display"};
