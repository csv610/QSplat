#ifndef QSPLAT_SHADER_H
#define QSPLAT_SHADER_H

#include <string>
#include <vector>
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>

class Shader {
public:
    GLuint ID;
    Shader(const char* vertexSource, const char* fragmentSource);
    ~Shader();

    void use();
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;

private:
    void checkCompileErrors(GLuint shader, std::string type);
};

#endif
