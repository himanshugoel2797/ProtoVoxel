#include "shaderprogram.h"
#include <string.h>

namespace PVG = ProtoVoxel::Graphics;

PVG::ShaderProgram::ShaderProgram() {
    id = glCreateProgram();
}

PVG::ShaderProgram::~ShaderProgram() {
    glDeleteProgram(id);
}

void PVG::ShaderProgram::Attach(ShaderSource const& src) {
    glAttachShader(id, src.GetID());
}

void PVG::ShaderProgram::Link() {
    glLinkProgram(id);
}