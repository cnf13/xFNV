#include "SDK/DirectShow/CBSAReader.h"
#include "BSCore/MemoryManager.h"
#include "BSMovie/CAsyncStream.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "SDK/DirectShow/CAsyncReader.h"
#include "SDK/DirectShow/CMediaType.h"

CBSAReader::~CBSAReader() {}

CBSAReader::CBSAReader(CAsyncStream* a1, const AM_MEDIA_TYPE* a2, HRESULT* a3) : CAsyncReader(0, 0, a1, a3) {
    m_mt.CopyFrom(a2);
}

HRESULT STDMETHODCALLTYPE CBSAReader::Register() {
    return 0;
}

HRESULT STDMETHODCALLTYPE CBSAReader::Unregister() {
    return 0;
}
