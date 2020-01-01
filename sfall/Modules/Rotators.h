#pragma once

#include "Module.h"

namespace sfall
{

    class Rotators : public Module
    {
    public:
        const char* name() { return "Rotators"; }
        void init();
        void exit() override;
		static void OnWmRefresh();
    };
}
