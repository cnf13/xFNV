#pragma once

#include "Reconstruction.hpp"
#include "SDK/Windows/unknwn.h"
#include "SDK/Windows/windef.h"

// The Windows SDK v6.0A <dinput.h> declarations the exe uses (DIRECTINPUT_VERSION 0x0800), with the SDK's names and
// 32-bit layouts, so no Windows header is needed. Global, as Windows declares them.

// One object of a data format: its type GUID (null: any), its offset in the data, its type and flags.
typedef struct _DIOBJECTDATAFORMAT {
	const GUID* pguid;
	uint32_t dwOfs;
	uint32_t dwType;
	uint32_t dwFlags;
} DIOBJECTDATAFORMAT, *LPDIOBJECTDATAFORMAT;

// The data format IDirectInputDevice8::SetDataFormat takes: dwSize and dwObjSize are the 32-bit sizes of this and
// of an object entry (0x18, 0x10).
typedef struct _DIDATAFORMAT {
	uint32_t dwSize;
	uint32_t dwObjSize;
	uint32_t dwFlags;
	uint32_t dwDataSize;
	uint32_t dwNumObjs;
	LPDIOBJECTDATAFORMAT rgodf;
} DIDATAFORMAT, *LPDIDATAFORMAT;

// dinput8.lib's predefined data formats (the SDK's `extern const DIDATAFORMAT c_dfDI...`), which the linker placed in
// the exe with their object tables: the library's objects, so src declares them and never defines them. The hybrid
// reads the exe's copies; natively they are dinput8.lib's C symbols, declared as the SDK's <dinput.h> does.
extern "C" const DIDATAFORMAT c_dfDIMouse2;
extern "C" const DIDATAFORMAT c_dfDIKeyboard;
extern "C" const DIDATAFORMAT c_dfDIJoystick;

// DirectInput8Create, dinput8.dll: the exe's import slot. IDA name; not in the PDB.
using imp_DirectInput8Create_t = HRESULT(WINAPI*)(HINSTANCE__* hinst, uint32_t dwVersion,
	const GUID* riidltf, void** ppvOut, IUnknown* punkOuter);
namespace xfnv::imports {
// import slot 0xFDF02C
extern "C" DECLSPEC_IMPORT HRESULT WINAPI DirectInput8Create(HINSTANCE__* hinst, uint32_t dwVersion,
	const GUID* riidltf, void** ppvOut, IUnknown* punkOuter);
} // namespace xfnv::imports
inline HRESULT imp_DirectInput8Create(HINSTANCE__* hinst, uint32_t dwVersion,
	const GUID* riidltf, void** ppvOut, IUnknown* punkOuter) {
	return xfnv::imports::DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

// DirectInput 8 structures the PC Controls embeds (the SDK's dinput.h layouts and field names).
struct IDirectInputDevice8A;
struct DIDEVICEINSTANCEA;
struct DIDEVICEOBJECTINSTANCEW;

// The enumeration callbacks IDirectInput8A::EnumDevices and IDirectInputDevice8A::EnumObjects call (the SDK's
// LPDIENUMDEVICESCALLBACKA and LPDIENUMDEVICEOBJECTSCALLBACK): the reference is the caller's pvRef.
using LPDIENUMDEVICESCALLBACKA = int32_t(CALLBACK*)(const DIDEVICEINSTANCEA* lpddi, void* pvRef);
using LPDIENUMDEVICEOBJECTSCALLBACKW = int32_t(CALLBACK*)(const DIDEVICEOBJECTINSTANCEW* lpddoi, void* pvRef);

// The SDK's IDirectInput8A (dinput.h), the DirectInput object Controls::Construct creates with DirectInput8Create.
// The exe calls CreateDevice (slot 3, +0xC), EnumDevices (slot 4, +0x10) and Release (slot 2, +0x8); the other slots
// keep the SDK's order, with the types src does not model as void*.
struct IDirectInput8A : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE CreateDevice(const GUID* rguid, IDirectInputDevice8A** ppDevice,
		IUnknown* punkOuter) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumDevices(uint32_t dwDevType, LPDIENUMDEVICESCALLBACKA lpCallback, void* pvRef,
		uint32_t dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDeviceStatus(const GUID* rguidInstance) = 0;
	virtual HRESULT STDMETHODCALLTYPE RunControlPanel(HWND__* hwndOwner, uint32_t dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE Initialize(HINSTANCE__* hinst, uint32_t dwVersion) = 0;
	virtual HRESULT STDMETHODCALLTYPE FindDevice(const GUID* rguidClass, const char* ptszName, GUID* pguidInstance) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumDevicesBySemantics(const char* ptszUserName, void* lpdiActionFormat,
		int32_t (CALLBACK* lpCallback)(const DIDEVICEINSTANCEA*, IDirectInputDevice8A*, uint32_t, uint32_t,
			void*),
		void* pvRef, uint32_t dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE ConfigureDevices(void* lpdiCallback, void* lpdiCDParams, uint32_t dwFlags,
		void* pvRef) = 0;
};

struct DIJOYSTATE {
	int32_t lX;
	int32_t lY;
	int32_t lZ;
	int32_t lRx;
	int32_t lRy;
	int32_t lRz;
	int32_t rglSlider[2];
	uint32_t rgdwPOV[4];
	uint8_t rgbButtons[32];
};

struct DIDEVICEINSTANCEA {
	uint32_t dwSize;
	GUID guidInstance;
	GUID guidProduct;
	uint32_t dwDevType;
	char tszInstanceName[260];
	char tszProductName[260];
	GUID guidFFDriver;
	uint16_t wUsagePage;
	uint16_t wUsage;
};

// The SDK's DIDEVICEOBJECTINSTANCEW (dinput.h, 0x240 bytes): what EnumObjects hands its callback for each axis or
// button. DIEnumDeviceObjectsCallback compares guidType (+0x4, 16 bytes) with the axis GUIDs and tests
// dwType (+0x18); the callback's buffer is 0x240 bytes (Controls.cpp:1208, gen::Blob<576>). tszName is a wide string,
// uint16_t here so it stays 2 bytes in the x64 build too.
struct DIDEVICEOBJECTINSTANCEW {
	uint32_t dwSize;
	GUID guidType;
	uint32_t dwOfs;
	uint32_t dwType;
	uint32_t dwFlags;
	uint16_t tszName[260];
	uint32_t dwFFMaxForce;
	uint32_t dwFFForceResolution;
	uint16_t wCollectionNumber;
	uint16_t wDesignatorIndex;
	uint16_t wUsagePage;
	uint16_t wUsage;
	uint32_t dwDimension;
	uint16_t wExponent;
	uint16_t wReportId;
};

struct DIDEVCAPS {
	uint32_t dwSize;
	uint32_t dwFlags;
	uint32_t dwDevType;
	uint32_t dwAxes;
	uint32_t dwButtons;
	uint32_t dwPOVs;
	uint32_t dwFFSamplePeriod;
	uint32_t dwFFMinTimeResolution;
	uint32_t dwFirmwareRevision;
	uint32_t dwHardwareRevision;
	uint32_t dwFFDriverVersion;
};

struct DIMOUSESTATE2 {
	int32_t lX;
	int32_t lY;
	int32_t lZ;
	uint8_t rgbButtons[8];
};

struct DIPROPHEADER {
	uint32_t dwSize;
	uint32_t dwHeaderSize;
	uint32_t dwObj;
	uint32_t dwHow;
};

struct DIPROPRANGE {
	DIPROPHEADER diph;
	int32_t lMin;
	int32_t lMax;
};

struct DIPROPDWORD {
	DIPROPHEADER diph;
	uint32_t dwData;
};

// The SDK's DIDEVICEOBJECTDATA: uAppData is a UINT_PTR (4 bytes on the exe's x86, 8 on x64, where DirectInput takes
// only the 24-byte size).
struct DIDEVICEOBJECTDATA {
	uint32_t dwOfs;
	uint32_t dwData;
	uint32_t dwTimeStamp;
	uint32_t dwSequence;
	uintptr_t uAppData;
};

// The SDK's IDirectInputDevice8A, through slot 25 (Poll); the slots src does not call take untyped pointers.
struct IDirectInputDevice8A {
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(const void* riid, void** ppvObj) = 0;
	virtual uint32_t STDMETHODCALLTYPE AddRef() = 0;
	virtual uint32_t STDMETHODCALLTYPE Release() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCapabilities(DIDEVCAPS* lpDIDevCaps) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback, void* pvRef,
		uint32_t dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetProperty(const void* rguidProp, DIPROPHEADER* pdiph) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetProperty(const void* rguidProp, const DIPROPHEADER* pdiph) = 0;
	virtual HRESULT STDMETHODCALLTYPE Acquire() = 0;
	virtual HRESULT STDMETHODCALLTYPE Unacquire() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDeviceState(uint32_t cbData, void* lpvData) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDeviceData(uint32_t cbObjectData, DIDEVICEOBJECTDATA* rgdod,
		uint32_t* pdwInOut, uint32_t dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetDataFormat(const void* lpdf) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetEventNotification(void* hEvent) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCooperativeLevel(void* hwnd, uint32_t dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetObjectInfo(void* pdidoi, uint32_t dwObj, uint32_t dwHow) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDeviceInfo(DIDEVICEINSTANCEA* pdidi) = 0;
	virtual HRESULT STDMETHODCALLTYPE RunControlPanel(void* hwndOwner, uint32_t dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE Initialize(void* hinst, uint32_t dwVersion, const void* rguid) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateEffect(const void* rguid, const void* lpeff, void** ppdeff,
		void* punkOuter) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumEffects(void* lpCallback, void* pvRef, uint32_t dwEffType) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetEffectInfo(void* pdei, const void* rguid) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetForceFeedbackState(uint32_t* pdwOut) = 0;
	virtual HRESULT STDMETHODCALLTYPE SendForceFeedbackCommand(uint32_t dwFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumCreatedEffectObjects(void* lpCallback, void* pvRef, uint32_t fl) = 0;
	virtual HRESULT STDMETHODCALLTYPE Escape(void* pesc) = 0;
	virtual HRESULT STDMETHODCALLTYPE Poll() = 0;
};

