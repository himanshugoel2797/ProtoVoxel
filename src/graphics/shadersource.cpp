#include "shadersource.h"
#include <string.h>

namespace PVG = ProtoVoxel::Graphics;

PVG::ShaderSource::ShaderSource(GLenum shaderType) {
    this->shaderType = shaderType;
    id = glCreateShader(shaderType);
}

PVG::ShaderSource::~ShaderSource() {
    glDeleteShader(id);
}

void PVG::ShaderSource::SetSource(const char* src) {
    const uint32_t len = strnlen(src, 16384);
    glShaderSource(id, 1, &src, (const GLint*)&len);
}

void PVG::ShaderSource::Compile() {
    glCompileShader(id);
}