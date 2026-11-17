#pragma once

#include "BSHavok/bhkCharacterPointCollector.h"
#include "BSHavok/bhkSerializable.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct hkStepInfo;
class hkpCharacterProxy;
class hkpShapePhantom;
class hkpSurfaceInfo;

// The Gamebryo wrapper around a Havok character proxy.
class bhkCharacterProxy : public bhkSerializable {
public:
	// Runs the exe's constructor on apMemory and returns it (thiscall, ret 0, 0xC6CE80).
	bhkCharacterProxy();
	~bhkCharacterProxy() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	// folded with bhkWorldObject::IsEqual at 0xC85800
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void Link(bool abAttach) override;

	// The world the proxy's phantom is in, or null.
	const hkpWorld* GethkWorld() const override;
	// Same as GethkWorld.
	hkpWorld* GethkWorldRw() override;

	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override { return 0x80; }
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	// The Havok proxy's shape phantom, null without a proxy.
	hkpShapePhantom* GethkShapePhantom();
	// Drops the reference the proxy holds on its shape phantom's wrapper (thiscall, ret 0, 0xCAC410). Protected in
	// the PDB.
	void RembhkRefs();
	// Fills arCinfo from the proxy.
	void GetCinfo(hkSerializableCinfo* apCinfo);
	// Drops the proxy's shape phantom and the manifold's contact points.
	void CleanUp();

	// The proxy's position and velocity in Havok units; both fall back to the zero vector without a proxy.
	const hkVector4& GethkPosition();
	const hkVector4& GethkLinearVelocity();
	// The exe's shared null-safe wrapped-object getter (0x4AE750, the one bhkWorldObject::operator* declares),
	// typed as the proxy this wrapper holds; null for a null this.
	hkpCharacterProxy* operator*();

	// Gives the proxy a new velocity.
	void SetLinearVelocity(const hkVector4& arVel);
	// Fills arGround with the surface the proxy stands on, probing along arDirection.
	void CheckSupport(const hkVector4& arDirection, hkpSurfaceInfo& arGround);
	// Integrates the proxy over the step arInfo.
	void Integrate(hkStepInfo& arInfo);
	void GetLinearVelocity(NiPoint3& arVel) const;

	bhkCharacterPointCollector m_ignoredCollisionStartCollector;

	static NiRTTI ms_RTTI;
	static inline int32_t iNumObjects;
};
inline constexpr fnvd::Vtable bhkCharacterProxy_vftable{};

using rdata_10C83FC_t = const char[4];
inline rdata_10C83FC_t rdata_10C83FC{"Vel"};

using rdata_10C8400_t = const char[4];
inline rdata_10C8400_t rdata_10C8400{"Pos"};
