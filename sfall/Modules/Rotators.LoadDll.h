#pragma once

#include "Module.h"

namespace rfall
{

class LoadDll : public sfall::Module {
public:
    const char* name() { return "LoadDll"; }
    void init();
};

}
