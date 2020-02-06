#pragma once

#include "Module.h"

namespace rfall
{

class Script : public sfall::Module {
public:
	const char* name() { return "Script"; }
	void init();
	virtual void exit();
};

}
