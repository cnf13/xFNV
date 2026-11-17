#pragma once

#include <stdint.h>
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

// The DirectShow automation interfaces the exe uses (the SDK's control.h) and their base IDispatch (oaidl.h). Each
// interface's methods and order are IDA's <Name>Vtbl; FalloutAudioMedia::ThreadProc (0x830B30) obtains all four from
// its IGraphBuilder through QueryInterface and calls them through their vtables. The SDK's LONG is int32_t, LONG_PTR
// intptr_t, BSTR and LPOLESTR are wide strings (uint16_t*), REFTIME is double, OAFilterState is LONG and OAEVENT is
// LONG_PTR.

struct ITypeInfo;
struct tagDISPPARAMS;
struct tagVARIANT;
struct tagEXCEPINFO;

// The automation base of the control interfaces: IUnknown's three methods and four more (IDA's IDispatchVtbl).
struct IDispatch : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(uint32_t* pctinfo) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(uint32_t iTInfo, uint32_t lcid, ITypeInfo** ppTInfo) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, uint16_t** rgszNames, uint32_t cNames, uint32_t lcid,
		int32_t* rgDispId) = 0;
	virtual HRESULT STDMETHODCALLTYPE Invoke(int32_t dispIdMember, REFIID riid, uint32_t lcid, uint16_t wFlags,
		tagDISPPARAMS* pDispParams, tagVARIANT* pVarResult, tagEXCEPINFO* pExcepInfo, uint32_t* puArgErr) = 0;
};

// Runs, pauses and stops the graph (IDA's IMediaControlVtbl, 0x40 bytes, 16 slots; the exe calls Run +0x1C, Pause
// +0x20 and Stop +0x24).
struct IMediaControl : IDispatch {
	virtual HRESULT STDMETHODCALLTYPE Run() = 0;
	virtual HRESULT STDMETHODCALLTYPE Pause() = 0;
	virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetState(int32_t msTimeout, int32_t* pfs) = 0;
	virtual HRESULT STDMETHODCALLTYPE RenderFile(uint16_t* strFilename) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddSourceFilter(uint16_t* strFilename, IDispatch** ppUnk) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_FilterCollection(IDispatch** ppUnk) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_RegFilterCollection(IDispatch** ppUnk) = 0;
	virtual HRESULT STDMETHODCALLTYPE StopWhenReady() = 0;
};

// The graph's event queue (IDA's IMediaEventVtbl, 0x34 bytes, 13 slots).
struct IMediaEvent : IDispatch {
	virtual HRESULT STDMETHODCALLTYPE GetEventHandle(intptr_t* hEvent) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetEvent(int32_t* lEventCode, intptr_t* lParam1, intptr_t* lParam2,
		int32_t msTimeout) = 0;
	virtual HRESULT STDMETHODCALLTYPE WaitForCompletion(int32_t msTimeout, int32_t* pEvCode) = 0;
	virtual HRESULT STDMETHODCALLTYPE CancelDefaultHandling(int32_t lEvCode) = 0;
	virtual HRESULT STDMETHODCALLTYPE RestoreDefaultHandling(int32_t lEvCode) = 0;
	virtual HRESULT STDMETHODCALLTYPE FreeEventParams(int32_t lEvCode, intptr_t lParam1, intptr_t lParam2) = 0;
};

// The stream's position, stop time and rate in seconds (IDA's IMediaPositionVtbl, 0x48 bytes, 18 slots).
struct IMediaPosition : IDispatch {
	virtual HRESULT STDMETHODCALLTYPE get_Duration(double* plength) = 0;
	virtual HRESULT STDMETHODCALLTYPE put_CurrentPosition(double llTime) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_CurrentPosition(double* pllTime) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_StopTime(double* pllTime) = 0;
	virtual HRESULT STDMETHODCALLTYPE put_StopTime(double llTime) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_PrerollTime(double* pllTime) = 0;
	virtual HRESULT STDMETHODCALLTYPE put_PrerollTime(double llTime) = 0;
	virtual HRESULT STDMETHODCALLTYPE put_Rate(double dRate) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_Rate(double* pdRate) = 0;
	virtual HRESULT STDMETHODCALLTYPE CanSeekForward(int32_t* pCanSeekForward) = 0;
	virtual HRESULT STDMETHODCALLTYPE CanSeekBackward(int32_t* pCanSeekBackward) = 0;
};

// The audio renderer's volume and balance in hundredths of a decibel (IDA's IBasicAudioVtbl, 0x2C bytes, 11 slots;
// the exe calls put_Volume +0x1C).
struct IBasicAudio : IDispatch {
	virtual HRESULT STDMETHODCALLTYPE put_Volume(int32_t lVolume) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_Volume(int32_t* plVolume) = 0;
	virtual HRESULT STDMETHODCALLTYPE put_Balance(int32_t lBalance) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_Balance(int32_t* plBalance) = 0;
};
