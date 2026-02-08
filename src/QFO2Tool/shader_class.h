#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>

void error_log(unsigned int* shader_var, int status_type);

class Shader {
  public:
    // program ID
    unsigned int ID;
    // constructor to read and build shader
    Shader(const char* vertexPath, const char* fragmentPath);
    // destructor to clean up GL program
    ~Shader();
    // use/activate shader
    void use();
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
};

#endif // ! SHADER_H
