// D1-N1 only: no game process, version detection, pattern scan or hook calls.
#include <cstddef>
#include <iostream>
#include "CPool.h"

static_assert(sizeof(void*) == 4, "Plugin-SDK must remain in the private x86 target");
static_assert(sizeof(CPool<int>) == 0x14);
static_assert(offsetof(CPool<int>, m_nSize) == 0x08);
static_assert(offsetof(CPool<int>, m_bOwnsAllocations) == 0x10);

int main() {
    // This exact source function returns a constant; GetGameVersion accesses GTA.
    if (plugin::Core::GetVersion() != 0x10) return 1;
    std::cout << "SAEX SDK compile/link probe: x86, CPool layout, SDK version 0x10; GTA not accessed\n";
    return 0;
}
