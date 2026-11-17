#include "BSShader/ShadowLightShader.h"

// The index is compared unsigned. Past the table it returns null.
NiD3DPixelShader* ShadowLightShader::GetPixelShader(int32_t aiWhich) {
	if (static_cast<uint32_t>(aiWhich) > 207) {
		return nullptr;
	}
	return spPixelShaders[aiWhich];
}
