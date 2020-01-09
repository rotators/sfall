#if _MSC_VER >= 1920

#pragma once

#include <cstdint>

#include "Module.h"

namespace sfall
{

class Sandbox : public Module {
public:
	const char* name() { return "Sandbox"; }

	void init();
};

}

#endif // _MSC_VER >= 1920 //
