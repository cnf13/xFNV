#pragma once

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BSCompoundFrustum;

// The culling process of the Fallout renderer: adds a stack of culling modes and the accumulator that receives the
// visible geometry.
class BSCullingProcess : public NiCullingProcess {
public:
	enum BSCPCullingType : uint32_t {
		BSCP_CULL_NORMAL = 0,
		BSCP_CULL_ALLPASS = 1,
		BSCP_CULL_ALLFAIL = 2,
		BSCP_CULL_IGNOREMULTIBOUNDS = 3,
		BSCP_CULL_FORCEMULTIBOUNDSNOUPDATE = 4,
	};

	// The local the exe constructs in place (FNVD_INLINED_LOCAL + FNVD_RECONSTRUCT): the reconstruct runs the
	// destructor first, which releases spAccumulator, so the default culler is the empty one (no visible set, no
	// accumulator).
	BSCullingProcess() : BSCullingProcess(nullptr) {}
	BSCullingProcess(NiVisibleArray* apVisibleSet);
	~BSCullingProcess() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void Process(NiAVObject* pkObject) override;
	void Process(const NiCamera* pkCamera, NiAVObject* pkScene, NiVisibleArray* pkVisibleSet) override;
	void AppendVirtual(NiGeometry& kVisible) override;

	void SetAccumulator(NiAccumulator* apAccumulator);
	// The PDB's GetAccumulator: spAccumulator's object. The exe's out-of-line copy (thiscall, ret 0, 0x663B40) is
	// folded with TESAmmo::GetShortName.
	BSShaderAccumulator* GetAccumulator() const;
	void SetCullMode(BSCPCullingType abTF);
	void PushCullMode(BSCPCullingType aeType);
	void PopCullMode();
	void ClearSharedMap();
	// Adds apObject to the accumulator's shared map. thiscall, ret 4, 0xC4F320.
	bool AddShared(NiAVObject* apObject);

	static void RegisterRTTI();
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSCPCullingType kCullMode;
	BSCPCullingType eTypeStackA[10];
	uint32_t iCTStackIndex;
	BSCompoundFrustum* pCompoundFrustum;
	NiPointer<NiAccumulator> spAccumulator;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSCullingProcess_vftable{};
