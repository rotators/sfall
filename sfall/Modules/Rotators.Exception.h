#pragma once

#include <string>

#include "Module.h"

namespace rfall
{

class Exception : public sfall::Module {
public:
	static std::string String;

public:
	const char* name() { return "Exception"; }
	void init();
};

}