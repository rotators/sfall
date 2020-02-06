#pragma once

#include <cstdint>

#include "Module.h"

namespace rfall
{

class HTTPD : public sfall::Module {
public:
	static uint16_t Port;

public:
	const char* name() { return "HTTPD"; }
	void init();
	virtual void exit();

};

}
