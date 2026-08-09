#pragma once

class e_NPCBase {
public:
	virtual void Spawn();
	virtual void Live(); //Botun ana düşünme fonksiyonu
	virtual void Run(); //Botun harita içerisindeki tüm hareketlerinin fonksiyonu
	virtual void Shoot(); //Botun dünya ile etkileşim kurma fonksiyonu
};