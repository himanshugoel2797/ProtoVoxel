#include "shadersource.h"
#include <string.h>
#include <iostream>
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
    int len = f.tellg();
    f.seekg(0, std::ios::beg);

    char *buf = new char[len];
    memset(buf, 0, len);
    f.read(buf, len);
    f.close();

    glShaderSource(id, 1, &buf, (const GLint *)&len);
    delete[] buf;

    std::cout << "ShaderSource Loaded: " << src << std::endl;
}

void PVG::ShaderSource::Compile()
{
    glCompileShader(id);

    GLint isCompiled = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &isCompiled);

    if (!isCompiled) {
        GLint maxLength = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        char* errorLog = new char[maxLength];
        glGetShaderInfoLog(id, maxLength, &maxLength, errorLog);

        std::cout << "Shader Compilation Error: " << errorLog << std::endl;
        delete[] errorLog;
    }
}