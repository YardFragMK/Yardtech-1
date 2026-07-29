#pragma once
#include"e_PolyBase.h"
#include"EntityParser.h"

class PolyBreakable : public e_PolyBase {
public:
    explicit PolyBreakable(const Entity& ent);
    PolyBreakable() = default;

    BreakableMaterial material = BreakableMaterial::Wood;

    void Damage() override;
    void Break() override;

};