//
// Created by lance on 2022/10/31.
//

#include "Shader.h"

namespace morrow
{
Shader::Shader(const char* vShaderCode, const char* fShaderCode)
{
    createShader(vShaderCode, fShaderCode);
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
    const char* vShaderCode = vertexPath.c_str();
    const char* fShaderCode = fragmentPath.c_str();
    createShader(vShaderCode, fShaderCode);
}

void Shader::createShader(const char* vShaderCode, const char* fShaderCode)
{
    uint32_t vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");
    // shader Program
    m_ID = glCreateProgram();
    glAttachShader(m_ID, vertex);
    glAttachShader(m_ID, fragment);
    glLinkProgram(m_ID);
    checkCompileErrors(m_ID, "PROGRAM");
    // delete the shaders as they're linked into our program now and no longer necessery
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    fetchAttributes();
    fetchUniforms();
}


Shader::~Shader()
{
    if(m_ID > 0){
//        RenderingThread::getInstance().deferredRelease.shaderProgramIds.emplace_back(m_ID);
        m_ID = 0;
    }
}

void Shader::use()
{
    glUseProgram(m_ID);
}

void Shader::unuse()
{
    glUseProgram(0);
}

void Shader::setBool(const std::string& name, bool value)
{
    glUniform1i(fetchUniformLocation(name), (int32_t) value);
}

void Shader::setInt(const std::string& name, int32_t value)
{
    glUniform1i(fetchUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value)
{
    glUniform1f(fetchUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const Vector2& value)
{
    glUniform2f(fetchUniformLocation(name), value.x, value.y);
}

void Shader::setVec2(const std::string& name, float x, float y)
{
    glUniform2f(fetchUniformLocation(name), x, y);
}

void Shader::setVec3(const std::string& name, const Vector3& value)
{
    glUniform3f(fetchUniformLocation(name), value.x, value.y, value.z);
}

void Shader::setVec3(const std::string& name, float x, float y, float z)
{
    glUniform3f(fetchUniformLocation(name), x, y, z);
}

void Shader::setVec4(const std::string& name, const Vector4& value)
{
    glUniform4f(fetchUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::setVec4(const std::string& name, float x, float y, float z, float w)
{
    glUniform4f(fetchUniformLocation(name), x, y, z, w);
}

void Shader::setMat3(const std::string& name, const Matrix3& mat)
{
    glUniformMatrix3fv(fetchUniformLocation(name), 1, GL_FALSE,
                       reinterpret_cast<const GLfloat*>(mat.elements));
}

void Shader::setMat4(const std::string& name, const Matrix4& mat)
{
    glUniformMatrix4fv(fetchUniformLocation(name), 1, GL_FALSE,
                       reinterpret_cast<const GLfloat*>(mat.elements));
}

void Shader::fetchAttributes()
{
    glGetProgramiv(m_ID, GL_ACTIVE_ATTRIBUTES, &m_numAttributes);
    for (int32_t i = 0; i < m_numAttributes; i++) {
        GLsizei bufSize = 32;
        GLsizei length;
        GLint size;
        GLenum type;
        GLchar name[bufSize];
        glGetActiveAttrib(m_ID, (GLuint) i, bufSize, &length, &size, &type, name);
//        printf("Attribute #%d Type: %u Name: %s\n", i, type, name);
        GLint location = glGetAttribLocation(m_ID, name);
        attributes.insert(ShaderPair(name, location));
    }
}

void Shader::fetchUniforms()
{
    glGetProgramiv(m_ID, GL_ACTIVE_UNIFORMS, &m_numUniforms);
    for (int32_t i = 0; i < m_numUniforms; i++) {
        GLsizei bufSize = 32;
        GLsizei length;
        GLint size;
        GLenum type;
        GLchar name[bufSize];
        glGetActiveUniform(m_ID, (GLuint) i, bufSize, &length, &size, &type, name);
//        printf("Uniform #%d Type: %u Name: %s\n", i, type, name);
        GLint location = glGetUniformLocation(m_ID, name);
        uniforms.insert(ShaderPair(name, location));
    }
}

GLint Shader::fetchAttributeLocation(ShaderString name)
{
    GLint location = -1;
    ShaderMap::iterator iter = attributes.find(name);
    if (iter != attributes.end()) {
        location = iter->second;
    } else {
        location = glGetAttribLocation(m_ID, (GLchar*) &name);
        if(location != -1)
        {
            attributes.insert(ShaderPair(name, location));
        }
        else
        {
//            printf("no attribute named: %s\n", name.c_str());
        }
    }
    return location;
}

GLint Shader::fetchUniformLocation(ShaderString name)
{
    GLint location = -1;
    ShaderMap::iterator iter = uniforms.find(name);
    if (iter != uniforms.end()) {
        location = iter->second;
    } else {
        location = glGetUniformLocation(m_ID, (GLchar*) &name);
        if(location != -1)
        {
            uniforms.insert(ShaderPair(name, location));
        }
        else
        {
//            printf("no uniform named: %s\n", name.c_str());
        }
    }
    return location;
}


void Shader::setVertexAttribute(const ShaderString& name, int32_t size, int32_t type, bool normalized, int32_t stride,
                                const void* pointer)
{
    GLint location = fetchAttributeLocation(name);
    if (location == -1) return;
    glVertexAttribPointer(location, size, type, normalized, stride, pointer);
    enableVertexAttribute(location);
}

void Shader::checkCompileErrors(GLuint shader, ShaderString type)
{
    GLint success;
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &success);
        if(!success) {
            GLint infoLen = 0;
            glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );
            if (infoLen > 1) {
                char* infoLog = (char*) malloc(sizeof(char) * infoLen);
                glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
                LOG_W("ERROR: {}", infoLog);
                free(infoLog);
            }
//            glDeleteShader ( shader );
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if(!success) {
            GLint infoLen = 0;
            glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen > 1) {
                char* infoLog = (char*) malloc(sizeof(char) * infoLen);
                glGetProgramInfoLog(shader, infoLen, nullptr, infoLog);
                LOG_W("ERROR: {}", infoLog);
                free(infoLog);
            }
//            glDeleteProgram(shader);
        }
    }
}

void Shader::disableVertexAttribute(const ShaderString& name)
{
    int32_t location = fetchAttributeLocation(name);
    if (location == -1) return;
    glDisableVertexAttribArray(location);
}

void Shader::disableVertexAttribute(int32_t location)
{
    glDisableVertexAttribArray(location);
}

void Shader::enableVertexAttribute(const ShaderString& name)
{
    int32_t location = fetchAttributeLocation(name);
    if (location == -1) return;
    glEnableVertexAttribArray(location);
}

void Shader::enableVertexAttribute(int32_t location)
{
    glEnableVertexAttribArray(location);
}

void Shader::bindAttribLocation(uint32_t index, const ShaderString& name)
{
    glBindAttribLocation(m_ID, index, name.c_str());
}
}
