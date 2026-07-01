#pragma once 
#include<glad/glad.h>

class Mesh {
public:
	Mesh();
	~Mesh();

	bool CreateQuad();
	void Draw() const;
	void Destroy();

private:
	GLuint m_vao = 0;
	GLuint m_vbo = 0;

	GLsizei m_vertexCount = 0;

};
