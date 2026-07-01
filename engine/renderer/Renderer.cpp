#include <glad/glad.h>
#include "Renderer.h"



bool Renderer::Init() {
    glViewport(0, 0, 1280, 720);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    //SHADERS
    const char* vertexSource = R"(
                #version 330 core
                layout(location = 0) in vec3 aPos;
                void main(){
                     gl_Position = vec4(aPos, 1.0);
                }
                )";

    const char* fragmentSource = R"(
                #version 330 core
                out vec4 FragColor;
                void main(){
                    FragColor = vec4(
                                    1.0,
                                    1.0,
                                    1.0,
                                    1.0);
                }
                )";

    if (!s_testShader.Load(vertexSource,fragmentSource)){
        return false;
    }

    return true;
}
void Renderer::BeginFrame(){
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame(){
}

void Renderer::Shutdown(){
}

Shader Renderer::s_testShader;