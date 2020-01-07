#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <unordered_map>

#include "../Logging.h"
#include "Scripting/Arrays.h"
#include "Scripting/OpcodeContext.h"
#include "Scripting/ScriptValue.h"

#include "Rotators.h"
#include "Rotators.Script.h"

using namespace rfall;

// Declared in Scripting/Handlers/Metarule.cpp
namespace sfall { namespace script { extern std::unordered_map<std::string, const sfall::script::SfallMetarule*> metaruleTable; }}

// Helpers

static char* cstrdup(const char* str) {
	size_t len = std::strlen(str);
	char* dup = new char[len + 1];
	if (!dup) // ?
		return nullptr;
	if (len)
		memcpy(dup, str, len);
	dup[len] = 0;

	return dup;
}

// Pseudopcodes

void op_tolower(sfall::script::OpcodeContext& ctx) {
	auto str = std::string(ctx.arg(0).asString()); // sue me.
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);

	ctx.setReturn(cstrdup(str.c_str()));
}

void op_toupper(sfall::script::OpcodeContext& ctx) {
	auto str = std::string(ctx.arg(0).asString()); // sue me.
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);

	ctx.setReturn(cstrdup(str.c_str()));
}

void op_rotators(sfall::script::OpcodeContext& ctx) {
	ctx.setReturn("Rotators, rotate!");
}

// Module

static const sfall::script::SfallMetarule metarules[] = {
	{ "r_tolower", op_tolower, 1, 1, -1, {sfall::script::ARG_STRING}},
	{ "r_toupper", op_toupper, 1, 1, -1, {sfall::script::ARG_STRING}},

	{ "rotators",  op_rotators, 0, 0 }
};

void sfall::Script::init() {
	for (auto metarule = std::begin(metarules); metarule != std::end(metarules); ++metarule) {
		std::string name, args;
		if (metarule->minArgs == metarule->maxArgs)
			name = std::to_string(metarule->minArgs);
		else
			name = "[" + std::to_string(metarule->minArgs) + "-" + std::to_string(metarule->maxArgs) + "]";

		if (metarule->minArgs > 0)
			args = ", ..."; // TODO

		sfall::dlog_f("sfall_func%s(\"%s\"%s)\n", DL_INIT, name.c_str(), metarule->name, args.c_str() );

		sfall::script::metaruleTable[metarule->name] = metarule;
	}
}
