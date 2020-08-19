#include "shadersource.h"
#include <string.h>
#include <fstream>

namespace PVG = ProtoVoxel::Graphics;

PVG::ShaderSource::ShaderSource(GLenum shaderType)
{
    this->shaderType = shaderType;
    id = glCreateShader(shaderType);
}

PVG::ShaderSource::~ShaderSource()
{
    glDeleteShader(id);
}

void PVG::ShaderSource::SetSource(const char *src)
{
    const uint32_t len = strnlen(src, 64 * 1024);
    glShaderSource(id, 1, &src, (const GLint *)&len);
}

void PVG::ShaderSource::SetSourceFile(const char *src)
{
    std::ifstream f;
    f.open(src);
    f.seekg(0, std::ios::end);
    auto len = f.tellg();
    char *buf = new char[len];
    f.read(buf, len);
    f.close();

    glShaderSource(id, 1, &buf, (const GLint *)&len);
    delete[] buf;
}

void PVG::ShaderSource::Compile()
{
    glCompileShader(id);
}