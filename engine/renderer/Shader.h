#pragma once
#include <glm/glm.hpp>
#include<string>
#include<glad/glad.h>


class Shader {
public:
	Shader() = default;
	~Shader();

	bool Load(const char* vertexSource, const char* fragmentSource);
	void Bind() const;
	void Unbind() const;

	GLuint GetProgram() const;

    void SetInt(
        const std::string& name,
        int value);

    void SetFloat(
        const std::string& name,
        float value);

    void SetVec2(
        const std::string& name,
        const glm::vec2& value);

    void SetVec3(
        const std::string& name,
        const glm::vec3& value);

    void SetVec4(
        const std::string& name,
        const glm::vec4& value);

    void SetMat4(
        const std::string& name,
        const glm::mat4& value);
private:
	GLuint m_program = 0;
	bool CheckShader(GLuint shader);
	bool CheckProgram(GLuint program);
    GLint GetUniformLocation( const std::string& name);
};