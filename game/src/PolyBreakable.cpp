#include"PolyBreakable.h"
#include <cstdio>

PolyBreakable::PolyBreakable(const Entity& ent)
{
    if (const std::string* targetname = ent.Get(EntityKeys::Targetname))
        name = *targetname;

    health = ent.GetInt(EntityKeys::Health, 100);

    material = static_cast<BreakableMaterial>(ent.GetInt(EntityKeys::Material));
}

void PolyBreakable::Damage()
{
    health -= 25; // Geçici
    Console::Log("Hasar aldı");

    if (health <= 0)
        Break();
}

void PolyBreakable::Break()
{
    switch (material)
    {
        case BreakableMaterial::Wood:
            Console::Log("Odun kırıldı");
            break;

        case BreakableMaterial::Glass:
            Console::Log("Cam kırıldı");
            break;
        case BreakableMaterial::UnbreakableGlass:
            break;

        case BreakableMaterial::Metal:
            Console::Log("Metal kırıldı");
            break;

        default:
            Console::Log("kırıldı");
            break;
    }
}