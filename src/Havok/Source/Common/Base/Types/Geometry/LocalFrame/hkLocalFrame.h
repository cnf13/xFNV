#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkLocalFrameCollector;

void cleanupLoadedObjecthkpCollidable(void* p);
void cleanupLoadedObjecthkpCollidableBoundingVolumeData(void* p);
// Havok SDK signature (hkpCollide.lib, tied by bytes).
void finishLoadedObjecthkpCollidableBoundingVolumeData(void* a1, int32_t a2);
// Havok SDK signature (hkpCollide.lib, tied by bytes).
void finishLoadedObjecthkpCollidable(void* a1, int32_t a2);
void cleanupLoadedObjecthkSimpleLocalFrame(void* p);
void cleanupLoadedObjecthkLocalFrameGroup(void* p);
// The class registry's finish-loaded-object callback.
void finishLoadedObjecthkpWorld(void* p, int32_t finishing);

// A named group local frames belong to. The exe's vtable overrides only the destructor (0xDA0800: frees m_name, then
// the hkReferencedObject part); the layout is the exe's hkClass 0x126967C (members 0x10D4578) and the PC PDB's.
class hkLocalFrameGroup : public hkReferencedObject {
public:
	// The PDB's hkLocalFrameGroup::Members: the member table hkLocalFrameGroupClass describes the group with.
	using MemberArray = const hkClassMember[1];

	// The SDK's finish constructor of a loaded object: the vtable, then m_name's finish constructor.
	hkLocalFrameGroup(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_name(aFlag) {}
	~hkLocalFrameGroup() override;

	static const hkClass& staticClass();

	hkStringPtr m_name;

	static MemberArray Members;
};

// A named frame of reference attached to a skeleton or a body, with a parent and children. The exe has no vtable of
// its own for this abstract class; its slots 3 to 13 are the PDB's intro virtuals.
class hkLocalFrame : public hkReferencedObject {
protected:
	// The SDK's finish constructor, which hkSimpleLocalFrame's inlines (the exe stores no vtable of this class).
	hkLocalFrame(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}

public:
	virtual void getLocalTransform(hkTransform& transform) const = 0;
	virtual void setLocalTransform(const hkTransform& transform) = 0;
	// Not pure, as the SDK has it: the base's version takes the translation of getLocalTransform.
	virtual void getLocalPosition(hkVector4& position) const;
	// The distance is hkReal (a float).
	virtual void getNearbyFrames(const hkVector4& target, float maxDistance,
		hkLocalFrameCollector& collector) const = 0;
	virtual const char* getName() const = 0;
	virtual const hkLocalFrame* getParentFrame() const = 0;
	virtual void setParentFrame(const hkLocalFrame* parentFrame) = 0;
	virtual int32_t getNumChildFrames() const = 0;
	virtual hkLocalFrame* getChildFrame(int32_t i) const = 0;
	virtual const hkLocalFrameGroup* getGroup() const = 0;
	virtual void setGroup(const hkLocalFrameGroup* group) = 0;

	// The transform from this frame to the root: the parents' transforms applied over the local one.
	void getTransformToRoot(hkTransform& transform) const;
	// The position in the root frame: the local position transformed by each parent's local transform in turn.
	void getPositionInRoot(hkVector4& position) const;
};

// Collects the frames near a point (the PDB's class: hkReferencedObject's slots, no data members, sizeof 8). Its one
// virtual, addFrame, is at vftable offset 12; hkSimpleLocalFrame::getNearbyFrames (0xDF65D0) calls it with the frame
// and the frame's distance.
class hkLocalFrameCollector : public hkReferencedObject {
public:
	virtual void addFrame(const hkLocalFrame* frame, float distance) = 0;
};

// The local frame that stores its transform, children, parent, group and name. The exe's vtable overrides the
// destructor and slots 3 to 13 (getGroup, slot 12, is a function shared with other classes' one-field getters).
class hkSimpleLocalFrame : public hkLocalFrame {
public:
	// The SDK's finish constructor of a loaded object: the vtable, then m_name's finish constructor.
	hkSimpleLocalFrame(hkFinishLoadedObjectFlag aFlag) : hkLocalFrame(aFlag), m_children(aFlag), m_name(aFlag) {}

	~hkSimpleLocalFrame() override;

	void getLocalTransform(hkTransform& transform) const override;
	void setLocalTransform(const hkTransform& transform) override;
	void getLocalPosition(hkVector4& position) const override;
	void getNearbyFrames(const hkVector4& target, float maxDistance, hkLocalFrameCollector& collector) const override;
	const char* getName() const override { return m_name; }
	const hkLocalFrame* getParentFrame() const override { return m_parentFrame; }
	void setParentFrame(const hkLocalFrame* parentFrame) override { m_parentFrame = parentFrame; }
	int32_t getNumChildFrames() const override;
	hkLocalFrame* getChildFrame(int32_t i) const override;
	const hkLocalFrameGroup* getGroup() const override { return m_group; }
	void setGroup(const hkLocalFrameGroup* group) override;

	static const hkClass& staticClass();

	// hkTransform is 16-aligned, so it starts at +0x10, the next 16-byte boundary after the base's 8 bytes; the PDB
	// has no member at +0x8 and the SDK declares m_transform directly after the base.
	hkTransform m_transform;
	hkArray<hkLocalFrame*> m_children;
	const hkLocalFrame* m_parentFrame;
	const hkLocalFrameGroup* m_group;
	hkStringPtr m_name;
};

extern hkClass hkLocalFrameGroupClass;
extern hkTypeInfo hkLocalFrameGroupTypeInfo;
extern hkClass hkSimpleLocalFrameClass;
extern hkTypeInfo hkSimpleLocalFrameTypeInfo;
inline constexpr fnvd::Vtable hkLocalFrameGroup_vftable{};

// The class's reflection record.
extern hkClass hkLocalFrameClass;
inline constexpr fnvd::Vtable hkSimpleLocalFrame_vftable{};
