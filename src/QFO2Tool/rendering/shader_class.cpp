#include "shader_class.h"

#include <fstream>
#include <iostream>
#include <sstream>

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    // get source code from file
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // exception stuff
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream;
        std::stringstream fShaderStream;
        // read files
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close files
        vShaderFile.close();
        fShaderFile.close();
        // convert stream to string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << '\n';
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // compile shaders
    unsigned int vertex = 0;
    unsigned int fragment = 0;

    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    // print errors
    error_log(&vertex, GL_COMPILE_STATUS);

    // fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    // print errors
    error_log(&fragment, GL_COMPILE_STATUS);

    // shader program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    // print errors
    error_log(&ID, GL_LINK_STATUS);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader() { glDeleteProgram(ID); }

void Shader::use() const { glUseProgram(ID); }

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void error_log(const unsigned int* shader_var, int status_type) {
    int success = 0;
    if (status_type == GL_COMPILE_STATUS) {
        glGetShaderiv(*shader_var, status_type, &success);
    } else if (status_type == GL_LINK_STATUS) {
        glGetProgramiv(*shader_var, status_type, &success);
    }

    char infoLog[512];
    if (success == 0) {
        const char* fail_type = nullptr;
        if (status_type == GL_COMPILE_STATUS) {
            fail_type = "COMPILE";
            glGetShaderInfoLog(*shader_var, 512, nullptr, infoLog);
        } else if (status_type == GL_LINK_STATUS) {
            fail_type = "PROGRAM";
            glGetProgramInfoLog(*shader_var, 512, nullptr, infoLog);
        }
        std::cout << "ERROR::SHADER::" << fail_type << "::COMPILATION_FAILED\n" << infoLog << '\n';
    }
}
