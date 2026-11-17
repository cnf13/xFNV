#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTFilter : public ImageSpaceShader {
public:
    ImageSpaceShaderFFTFilter();
    ~ImageSpaceShaderFFTFilter() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
