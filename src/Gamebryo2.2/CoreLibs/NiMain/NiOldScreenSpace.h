#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiPoint2;
class NiScreenTexture;

// The old screen-space API: screen geometry, screen polygons and the camera that draws them.

// A triangle-list mesh in screen space.
class NiScreenGeometry : public NiTriShape {
public:
	NiScreenGeometry();
	~NiScreenGeometry() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};

// The geometry data of NiScreenGeometry: a list of screen elements (2D vertices, colors, texture coordinates).
class NiScreenGeometryData : public NiTriShapeData {
public:
	// An element's vertices, colors and texture coordinates. The exe's vtable (0x109F814) holds only the destructor.
	struct ScreenElement {
		ScreenElement();
		virtual ~ScreenElement();

		// The deleting destructors free through MemoryManager::Deallocate, an element or a new[] array.
		static void operator delete(void* apMemory);
		static void operator delete[](void* apMemory);

		uint16_t m_usNumVerts;
		uint16_t m_usNumTexCoordSets;
		NiPoint2* m_pkVerts;
		NiColorA* m_pkColors;
		NiPoint2* m_pkTexCoords;
	};

	// The pool the screen elements come from; _SDMInit makes it and _SDMShutdown frees it.
	typedef NiTObjectPool<ScreenElement> ScreenElementPool;

	NiScreenGeometryData();
	~NiScreenGeometryData() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

	static NiObject* CreateObject();
	static void _SDMInit();
	static void _SDMShutdown();

	bool m_bPixelAccurate;
	bool m_bUnorderedElements;
	uint32_t m_usActiveVerts;
	NiTPrimitiveArray<ScreenElement*> m_kScreenElements;

	static NiRTTI ms_RTTI;
	static inline ScreenElementPool* ms_pkScreenElementPool;
};

// The exe's new[] of uiCount screen elements (cdecl): the count in a cookie dword before the array, each element
// built by its constructor 0xA8FF50.
NiScreenGeometryData::ScreenElement* AllocateScreenElements(uint32_t uiCount);

// A screen-space polygon: vertices, texture coordinates and colors drawn with its own property state.
class NiScreenPolygon : public NiObject {
public:
	NiScreenPolygon();
	~NiScreenPolygon() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& stream) override;
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

	static NiObject* CreateObject();

	// a smart pointer in the original (src's NiPropertyState has no reference count)
	NiPropertyState* m_spPropertyState;
	uint16_t m_usVertices;
	NiPoint3* m_pkVertex;
	NiPoint2* m_pkTexture;
	NiColorA* m_pkColor;

	static NiRTTI ms_RTTI;
};

// A camera that also draws lists of screen polygons and screen textures.
class NiScreenSpaceCamera : public NiCamera {
public:
	NiScreenSpaceCamera();
	~NiScreenSpaceCamera() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;

	// NiCamera's members, then the screen polygon and texture arrays.
	void CopyMembers(NiScreenSpaceCamera* pkDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	NiTObjectArray<NiPointer<NiScreenPolygon>> m_kScreenPolygons;
	NiTObjectArray<NiPointer<NiScreenTexture>> m_kScreenTextures;

	static NiRTTI ms_RTTI;
};

// The vtable NiScreenGeometry::CreateObject writes.
inline constexpr fnvd::Vtable NiScreenGeometry_vftable{};
inline constexpr fnvd::Vtable NiScreenGeometryData_ScreenElement_vftable{};
inline constexpr fnvd::Vtable NiScreenGeometryData_vftable{};

// The vtable NiScreenPolygon::CreateObject writes.
inline constexpr fnvd::Vtable NiScreenPolygon_vftable{};
inline constexpr fnvd::Vtable NiScreenSpaceCamera_vftable{};
