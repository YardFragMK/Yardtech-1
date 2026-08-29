#include"Player.h"

Player g_Player;
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
        death();
    }

}
void Player::attack(int attacknumber) {
    if (attacknumber == 1) {
        Console::Log("1. saldiri yontemi kullanildi");

        if (CurrentWeapon == PISTOL) {
            pistol.Shoot(1);
        }
        else if (CurrentWeapon == SHOTGUN) {
            shotgun.Shoot(1);
        }
    }
    else if (attacknumber == 2) {
        Console::Log("2. saldiri yontemi kullanildi");

        if (CurrentWeapon == SHOTGUN) {
            shotgun.Shoot(2);
        }
    }
}


void Player::death() {
    // [] Kamera durdurulacak ve yere düşecek
    // [] Klavye ve mouse girdisi durduralacak
    // [] Ekrana yazı yazdırılacak
}