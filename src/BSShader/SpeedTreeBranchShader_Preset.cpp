#include "BSShader/ShadowLightShader.h"

// The exe compares the index unsigned. Past the table it returns the pointer of a null NiPointer temporary, whose
// release does nothing.
NiD3DPixelShader* ShadowLightShader::GetPixelShader(int32_t aiWhich) {
	if (static_cast<uint32_t>(aiWhich) > 207) {
		return nullptr;
	}
	return spPixelShaders[aiWhich];
}
