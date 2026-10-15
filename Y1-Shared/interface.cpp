#include "interface.h"

InterfaceReg* InterfaceReg::head = nullptr;

InterfaceReg::InterfaceReg(InstantiateInterfaceFn fn, const char* interfaceName)
    : createFn(fn), name(interfaceName), next(head) {
    head = this;
}

Y_EXPORT void* CreateInterface(const char* name, int* returnCode) {
    for (InterfaceReg* reg = InterfaceReg::head; reg != nullptr; reg = reg->next) {
        if (std::strcmp(reg->name, name) == 0) {
            if (returnCode) *returnCode = 0;
            return reg->createFn();
        }
    }
    if (returnCode) *returnCode = 1;
    return nullptr;
}