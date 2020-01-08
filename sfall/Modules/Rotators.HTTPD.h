#pragma once

#include <cstdint>

#include "Module.h"

namespace sfall
{

class HTTPD : public Module {
public:
	static uint16_t Port;

#if _MSC_VER >= 1920

public:
	const char* name() { return "HTTPD"; }
	void init();
	virtual void exit();

#else // remember to add dummies for v140_xp compilation

public:
	const char* name() { return "HTTPDummy"; }
	void init() {}

#endif

};

}
