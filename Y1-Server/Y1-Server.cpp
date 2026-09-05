#include <iostream>
#include <enet/enet.h>
#include <cstdio>

void TestEnetInit() {
	if (enet_initialize() != 0) {
		printf("ENet baslatilamadi!\n");
		return;
	}
	printf("ENet basariyla baslatildi.\n");
	enet_deinitialize();
}

int main()
{
	TestEnetInit();
	system("PAUSE");
}

