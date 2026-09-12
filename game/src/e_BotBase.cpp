#include "e_BotBase.h"

// Taban sinifin varsayilan (bos) davranislari. Botlar her zaman turetilmis
// bir sinif (orn. BotStilar) uzerinden olusturuldugu icin bu fonksiyonlar
// pratikte hicbir zaman dogrudan cagrilmaz, ama virtual oldugu icin bir
// tanimlarinin olmasi gerekir.
void e_BotBase::Spawn() {}
void e_BotBase::Live(float deltaTime) { (void)deltaTime; }
void e_BotBase::Run(float deltaTime) { (void)deltaTime; }
void e_BotBase::Shoot() {}
void e_BotBase::Render() const {}