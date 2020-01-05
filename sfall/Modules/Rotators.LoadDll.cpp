#include <string>
#include <vector>

#include "..\main.h"
#include "Rotators.LoadDll.h"

#include "Rotators.h"
using namespace rfall;

/*** https://github.com/phobos2077/sfall/pull/273 ***/

namespace sfall
{

void LoadDll::init() {
	std::vector<std::string> names = Ini::List("Main", "LoadDll", "");

	for (const auto& name : names) {
		if (name.empty())
			continue;

		dlog_f("Loading %s... ", DL_INIT, name.c_str());

		HMODULE dll = LoadLibraryA(name.c_str());

		if (!dll || dll == INVALID_HANDLE_VALUE)
			dlogr("ERROR", DL_INIT);
		else
			dlogr("OK", DL_INIT);
	}
}

}
