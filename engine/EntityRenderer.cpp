#include "EntityRenderer.h"
#include "WeaponEffects.h"
#include "../game/src/GLBModel.h"
#include <windows.h>
#include <GL/gl.h>
#include <glm/glm.hpp>

namespace {
	constexpr const char* STILAR_MODEL_PATH = "nvs1/models/stilar/stilar.glb";
	constexpr float STILAR_MODEL_SCALE = 15.0f;

	GLBModel s_stilarModel;
	bool s_modelLoadAttempted = false;

	void DrawBox(const glm::vec3& half) {
		const float hx = half.x, hy = half.y, hz = half.z;

		glBegin(GL_QUADS);
		glVertex3f(hx, -hy, -hz); glVertex3f(hx, -hy, hz); glVertex3f(hx, hy, hz); glVertex3f(hx, hy, -hz);
		glVertex3f(-hx, -hy, hz); glVertex3f(-hx, -hy, -hz); glVertex3f(-hx, hy, -hz); glVertex3f(-hx, hy, hz);
		glVertex3f(-hx, hy, -hz); glVertex3f(hx, hy, -hz); glVertex3f(hx, hy, hz); glVertex3f(-hx, hy, hz);
		glVertex3f(-hx, -hy, hz); glVertex3f(hx, -hy, hz); glVertex3f(hx, -hy, -hz); glVertex3f(-hx, -hy, -hz);
		glVertex3f(-hx, -hy, hz); glVertex3f(-hx, hy, hz); glVertex3f(hx, hy, hz); glVertex3f(hx, -hy, hz);
		glVertex3f(hx, -hy, -hz); glVertex3f(hx, hy, -hz); glVertex3f(-hx, hy, -hz); glVertex3f(-hx, -hy, -hz);
		glEnd();
	}

	void DrawStilar(const NetEntityState& entity) {
		glPushMatrix();
		glTranslatef(entity.origin.x, entity.origin.y, entity.origin.z);

		if (s_stilarModel.IsLoaded()) {
			glScalef(STILAR_MODEL_SCALE, STILAR_MODEL_SCALE, STILAR_MODEL_SCALE);
			s_stilarModel.Render();
		}
		else {
			// Model yuklenemezse yer tutucu kutu cizilir.
			glDisable(GL_TEXTURE_2D);
			glColor4f(0.35f, 0.05f, 0.05f, 1.0f);
			DrawBox(glm::vec3(16.0f, 32.0f, 16.0f));
			glEnable(GL_TEXTURE_2D);
		}

		glPopMatrix();
	}

	void DrawProjectile(const NetEntityState& entity) {
		glPushMatrix();
		glTranslatef(entity.origin.x, entity.origin.y, entity.origin.z);

		glDisable(GL_TEXTURE_2D);
		glColor4f(1.0f, 0.4f, 0.1f, 1.0f);
		DrawBox(glm::vec3(6.0f));
		glEnable(GL_TEXTURE_2D);

		glPopMatrix();
	}
}

void EntityRenderer_Render(const std::vector<NetEntityState>& entities) {
	if (entities.empty()) return;

	// Model GL kaynagi olusturdugu icin yukleme, GL context'i hazir olduktan
	// sonra, ilk cizimde bir kez denenir.
	if (!s_modelLoadAttempted) {
		s_modelLoadAttempted = true;
		s_stilarModel.Load(STILAR_MODEL_PATH);
	}

	for (const NetEntityState& entity : entities) {
		switch (entity.classId) {
		case ENT_CLASS_BOT_STILAR: DrawStilar(entity); break;
		case ENT_CLASS_PROJECTILE: DrawProjectile(entity); break;
		default: break;
		}
	}
	// Oyuncu silahlarinin mermileri ve etkileri.
	WeaponEffects_Render(entities);
}