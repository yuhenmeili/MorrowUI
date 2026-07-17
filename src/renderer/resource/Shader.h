//
// Created by lance on 2022/10/31.
//

#ifndef MORROW_SHADER_H
#define MORROW_SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix3.h"
#include "utils/Log.h"
#include <sys/types.h>
#include <memory>
#include <unordered_map>

#ifdef OPENGL_GLFW
    #include "platform/wgl/OpenglHeader.h"
#else
    #include "platform/egl/GLESHeader.h"
#endif

#define JIM_POSITION "a_position"
#define JIM_NORMAL "a_normal"
#define JIM_TEXCOORD "a_texCoord"
#define JIM_COLOR "a_color"

typedef std::string ShaderString;
typedef std::pair<ShaderString, GLint> ShaderPair;
typedef std::unordered_map<ShaderString, GLint> ShaderMap;

namespace morrow
{
using namespace Math;
/**
 * 暂时废弃
 */
class Shader
{
public:
    Shader(const char* vShaderCode, const char* fShaderCode);

    Shader(const std::string& vertexPath, const std::string& fragmentPath);

    virtual ~Shader();

    void use();

    void unuse();

    void setBool(const ShaderString& name, bool value);

    void setInt(const ShaderString& name, int32_t value);

    void setFloat(const ShaderString& name, float value);

    void setVec2(const ShaderString& name, const Vector2& value);

    void setVec2(const ShaderString& name, float x, float y);

    void setVec3(const ShaderString& name, const Vector3& value);

    void setVec3(const ShaderString& name, float x, float y, float z);

    void setVec4(const ShaderString& name, const Vector4& value);

    void setVec4(const ShaderString& name, float x, float y, float z, float w);

    void setMat3(const ShaderString& name, const Matrix3& mat);

    void setMat4(const ShaderString& name, const Matrix4& mat);

    void setVertexAttribute(const ShaderString& name, int32_t size, int32_t type, bool normalized, int32_t stride, const void* pointer);

    void enableVertexAttribute(const ShaderString& name);

    void enableVertexAttribute(int32_t location);

    void disableVertexAttribute(const ShaderString& name);

    void disableVertexAttribute(int32_t location);

    void bindAttribLocation(uint32_t index, const ShaderString& name);

private:
    void createShader(const char* vShaderCode, const char* fShaderCode);

    void fetchAttributes();

    void fetchUniforms();

    void checkCompileErrors(GLuint shader, ShaderString type);

    GLint fetchUniformLocation(ShaderString name);

    GLint fetchAttributeLocation(ShaderString name);

public:
    GLuint m_ID;

private:
    GLint m_numAttributes = 0;
    GLint m_numUniforms = 0;
    ShaderMap attributes;
    ShaderMap uniforms;
};

using ShaderSharedPtr = std::shared_ptr<Shader>;
}

#endif //MORROW_SHADER_H
