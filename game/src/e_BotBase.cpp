#include "e_BotBase.h"

void e_BotBase::Spawn() {}
void e_BotBase::Live(float deltaTime) { (void)deltaTime; }
void e_BotBase::Run(float deltaTime) { (void)deltaTime; }
void e_BotBase::Shoot() {}
void e_BotBase::Render() const {}

void e_BotBase::RenderModelOrBox() const {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);

    if (m_model.IsLoaded()) {
        constexpr float MODEL_SCALE = 15.0f;

        glScalef(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE);
        m_model.Render();
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor4f(0.35f, 0.05f, 0.05f, 1.0f);

        float hx = halfExtents.x, hy = halfExtents.y, hz = halfExtents.z;

        glBegin(GL_QUADS);
        glVertex3f(hx, -hy, -hz); glVertex3f(hx, -hy, hz); glVertex3f(hx, hy, hz); glVertex3f(hx, hy, -hz);
        glVertex3f(-hx, -hy, hz); glVertex3f(-hx, -hy, -hz); glVertex3f(-hx, hy, -hz); glVertex3f(-hx, hy, hz);
        glVertex3f(-hx, hy, -hz); glVertex3f(hx, hy, -hz); glVertex3f(hx, hy, hz); glVertex3f(-hx, hy, hz);
        glVertex3f(-hx, -hy, hz); glVertex3f(hx, -hy, hz); glVertex3f(hx, -hy, -hz); glVertex3f(-hx, -hy, -hz);
        glVertex3f(-hx, -hy, hz); glVertex3f(-hx, hy, hz); glVertex3f(hx, hy, hz); glVertex3f(hx, -hy, hz);
        glVertex3f(hx, -hy, -hz); glVertex3f(hx, hy, -hz); glVertex3f(-hx, hy, -hz); glVertex3f(-hx, -hy, -hz);
        glEnd();

        glEnable(GL_TEXTURE_2D);
    }

    glPopMatrix();
}