#pragma once

#include <stdint.h>
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

// The DirectShow SDK's strmif.h types the exe's filter registration (AMovieSetupRegisterFilterWithMapper, AMovieSetupRegisterFilter) uses.

struct IEnumRegFilters;

// A pin's media type (the DirectShow SDK's REGPINTYPES, AMOVIESETUP_MEDIATYPE): its major type and subtype.
// AMovieSetupRegisterFilterWithMapper reads the pair at 8-byte steps (0x9FB3BC lea eax,[ecx+eax*8]; the subtype at +4).
struct REGPINTYPES {
	const GUID* clsMajorType;
	const GUID* clsMinorType;
};

// A filter's pin (the DirectShow SDK's REGFILTERPINS, AMOVIESETUP_PIN; IDA's struct). AMovieSetupRegisterFilterWithMapper steps through the
// pins 0x24 bytes at a time (0x9FB452) and reads each field at these offsets. The wide strings are the SDK's
// LPWSTR/WCHAR, the flags its BOOL.
struct REGFILTERPINS {
	uint16_t* strName;
	int32_t bRendered;
	int32_t bOutput;
	int32_t bZero;
	int32_t bMany;
	const GUID* clsConnectsToFilter;
	const uint16_t* strConnectsToPin;
	uint32_t nMediaTypes;
	const REGPINTYPES* lpMediaType;
};

// A filter's setup data (the DirectShow SDK's AMOVIESETUP_FILTER): its CLSID, name, merit and pins. AMovieSetupRegisterFilterWithMapper reads
// clsID at +0, strName at +4, dwMerit at +8, nPins at +0xC and lpPin at +0x10.
struct AMOVIESETUP_FILTER {
	const GUID* clsID;
	const uint16_t* strName;
	uint32_t dwMerit;
	uint32_t nPins;
	const REGFILTERPINS* lpPin;
};

// The filter mapper (the DirectShow SDK's IFilterMapper; IDA's IFilterMapperVtbl gives the methods and their order).
// AMovieSetupRegisterFilterWithMapper calls RegisterFilter at +0xC, RegisterPin at +0x14, RegisterPinType at +0x18 and UnregisterFilter at
// +0x1C, each CLSID pushed by value (16 bytes). The wide strings are the SDK's LPCWSTR, the flags its BOOL.
struct IFilterMapper : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE RegisterFilter(GUID clsid, const uint16_t* Name, uint32_t dwMerit) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterFilterInstance(GUID clsid, const uint16_t* Name, GUID* MRId) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterPin(GUID Filter, const uint16_t* Name, int32_t bRendered, int32_t bOutput,
		int32_t bZero, int32_t bMany, GUID ConnectsToFilter, const uint16_t* ConnectsToPin) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterPinType(GUID clsFilter, const uint16_t* strName, GUID clsMajorType,
		GUID clsSubType) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterFilter(GUID Filter) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterFilterInstance(GUID MRId) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterPin(GUID Filter, const uint16_t* Name) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumMatchingFilters(IEnumRegFilters** ppEnum, uint32_t dwMerit,
		int32_t bInputNeeded, GUID clsInMaj, GUID clsInSub, int32_t bRender, int32_t bOututNeeded, GUID clsOutMaj,
		GUID clsOutSub) = 0;
};
