#pragma once
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
private:
	GLuint m_program = 0;
	bool CheckShader(GLuint shader);
	bool CheckProgram(GLuint program);
};