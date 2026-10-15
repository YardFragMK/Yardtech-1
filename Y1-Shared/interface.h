#pragma once
#include <cstring>

// Moduller (exe, client.dll, server.dll) arasinda nesne paylasimi icin ortak
// mekanizma. Her modul sabit isimli arayuzlerini bir "factory" fonksiyonu
// uzerinden disariya verir; diger moduller arayuzun somut sinifini bilmeden
// yalnizca (isim -> pointer) eslemesiyle erisir.

#define Y_EXPORT extern "C" __declspec(dllexport)

typedef void* (*CreateInterfaceFn)(const char* name, int* returnCode);
typedef void* (*InstantiateInterfaceFn)();

class InterfaceReg {
public:
    InterfaceReg(InstantiateInterfaceFn fn, const char* interfaceName);

    InstantiateInterfaceFn createFn;
    const char* name;
    InterfaceReg* next;
    static InterfaceReg* head;
};

// Bir sinifi, verilen versiyon ismiyle tek (singleton) arayuz olarak kaydeder.
#define EXPOSE_SINGLE_INTERFACE(className, interfaceType, versionString)            \
    static void* Create_##className() {                                              \
        static className instance;                                                   \
        return static_cast<interfaceType*>(&instance);                               \
    }                                                                                \
    static InterfaceReg s_reg_##className(Create_##className, versionString)

// Her modul kendi kopyasini export eder; LoadLibrary + GetProcAddress ile
// modul bazinda cagrildigi icin modulller arasinda isim cakismasi olmaz.
Y_EXPORT void* CreateInterface(const char* name, int* returnCode);

template <typename T>
T* GetInterface(CreateInterfaceFn factory, const char* version) {
    if (factory == nullptr) return nullptr;
    int code = 0;
    return static_cast<T*>(factory(version, &code));
}