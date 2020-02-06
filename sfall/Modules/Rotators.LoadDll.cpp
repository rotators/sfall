#include <string>
#include <vector>

#include "..\main.h"
#include "Rotators.LoadDll.h"

#include "Rotators.h"

using namespace rfall;

void LoadDll::init() {
	std::vector<std::string> names = ini.GetStrVec("Main", "LoadDll", ',');

	for (const auto& name : names) {
		if (name.empty())
			continue;

		sfall::dlog_f("Loading %s... ", DL_INIT, name.c_str());

		HMODULE dll = LoadLibraryA(name.c_str());

		if (!dll || dll == INVALID_HANDLE_VALUE)
			sfall::dlogr("ERROR", DL_INIT);
		else
			sfall::dlogr("OK", DL_INIT);
	}
}
