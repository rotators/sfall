#pragma once

#include <cstdint>

#include "Module.h"

namespace sfall
{

class Rotators : public Module {
public:
	const char* name() { return "Rotators"; }
	void init();
	void exit() override;

	static void OnWmRefresh();
};

}

// For sfall::script::FillListVector(DWORD type, std::vector<fo::GameObject*>& vec)
enum class FLV : uint8_t {
	CRITTERS = 0,
	GROUNDITEMS,
	SCENERY,
	WALLS,
	TILES, // Disabled
	MISC,
	SPATIAL,
	ALL = 9
};
