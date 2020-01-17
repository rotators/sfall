#pragma once

#include "Module.h"

namespace sfall
{

class Script : public Module {
public:
	const char* name() { return "Script"; }
	void init();
	virtual void exit();
};

}

namespace rfall
{

char* wmGetCurrentTerrainName();

}
