#include "pch.h"
#include "e_BotBase.h"

void e_BotBase::Spawn() {}
void e_BotBase::Live(float deltaTime) { (void)deltaTime; }
void e_BotBase::Run(float deltaTime) { (void)deltaTime; }
void e_BotBase::Shoot() {}
uint16_t e_BotBase::GetClassId() const { return ENT_CLASS_NONE; }