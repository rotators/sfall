#pragma once

#include <cstdint>

#include "Module.h"

namespace sfall
{

	class Sandbox : public Module {
	public:
		static uint16_t Port;

	public:
		const char* name() { return "Sandbox"; }
		void init();
	};

}
