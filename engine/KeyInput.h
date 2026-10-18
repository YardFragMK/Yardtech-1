#pragma once

// SDL olaylarini frame basina bir kez isler: olaylari client.dll'e iletir,
// menu/konsol/ESC davranisini yonetir. Kamera ve oyun girdisi client.dll'dedir.
class KeyInput {
public:
	static void Update(bool& running);
};