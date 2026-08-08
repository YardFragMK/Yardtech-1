#include"Player.h"

void Player::takeDamage(int enemyDamage) {
    if (Armor > 0){
        int armorDamage = enemyDamage * 70 / 100;
        int healthDamage = enemyDamage * 30 / 100;

        if (Armor >= armorDamage){
            Armor -= armorDamage;
            Health -= healthDamage;
        }
        else{
            int kalan = armorDamage - Armor;
            Armor = 0;
            Health -= healthDamage + kalan;
        }
    }
    else{
        Health -= enemyDamage;
    }
    if (Health < 0) {
        Health = 0;
    }

}
void Player::attack(int attacknumber) {
    Console::Log("Ates edildi");
    if (attacknumber == 1) {
        if (CurrentWeapon = PISTOL) {
            pistol.Shoot();
        }
        if (CurrentWeapon = SHOTGUN) {
            shotgun.Shoot(1);
        }
    }
    else if (attacknumber == 2) {
        if (CurrentWeapon = SHOTGUN) {
            shotgun.Shoot(2);
        }
    }

}