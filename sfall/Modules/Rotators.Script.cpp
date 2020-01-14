#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <unordered_map>

#include "../Logging.h"
#include "../FalloutEngine/Fallout2.h"

#include "Scripting/Arrays.h"
#include "Scripting/OpcodeContext.h"
#include "Scripting/ScriptValue.h"

#include "Interface.h"

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


// r_get_ini_*() //
// All .ini files read by scripts are cached and read from memory; speeds up extracting data from large files (like WORLDMAP.TXT) on some hardware

static std::unordered_map<std::string, rfall::Ini> IniCache;

static rfall::Ini& GetCachedIni(const char* name) {
	auto it = IniCache.find(name);
	if (it != IniCache.end())
		return it->second;

	sfall::dlog_f("Adding INI to cache: %s\n", DL_MAIN, name);

	Ini iniFile;
	iniFile.LoadFile(name);
	IniCache.insert(std::make_pair(name, iniFile));

	return IniCache[name];
}

void r_get_ini_string(sfall::script::OpcodeContext& ctx) {
	ctx.setReturn(cstrdup(GetCachedIni(ctx.arg(0).asString()).GetStr(ctx.arg(1).asString(), ctx.arg(2).asString(), ctx.arg(3).asString()).c_str()));
}

// r_message_box() //

// Known flags, with made up names
// Probably needs testing behavior with conflicting flags (YESNO|CLEAN, NORMAL|TINY)
enum DialogOutFlags : uint8_t {
	DIALOGOUT_NORMAL      = 0x01, // uses regular graphic
	DIALOGOUT_SMALL       = 0x02, // uses smaller graphic
	DIALOGOUT_ALIGN_LEFT  = 0x04, // text moved to left
	DIALOGOUT_ALIGN_TOP   = 0x08, // text moved to top
	DIALOGOUT_YESNO       = 0x10, // DONE button replaced with YES/NO; clicking YES returns 1, clicking NO returns 0
	DIALOGOUT_CLEAN       = 0x20  // no buttons
};

static constexpr uint8_t DialogOutColor = 145;

static int32_t __stdcall DialogOut(std::vector<std::string> strings, int32_t flags = DIALOGOUT_NORMAL, int32_t color1 = DialogOutColor, int32_t color23 = DialogOutColor) {
	if (strings.empty())
		return -1;

	// convert first line to char*
	// no length validation here; empty string is allowed, as it doesn't seem to break anything
	// when/if drawing directly on screen will be possible some day, it might be useful for displaying small images instead of text only
	char*    text1  = cstrdup(strings.front().c_str());
	char**   text23 = nullptr;
	uint32_t extra  = 0;

	// convert second/third line to char**
	strings.erase(strings.begin());
	if (!strings.empty()) {
		text23 = new char*[strings.size()];

		for (extra = 0; extra < strings.size(); extra++) {
			text23[extra] = new char[strings[extra].size() + 1];
			strcpy(text23[extra], strings[extra].c_str());
		}
	}

	int32_t result = -1;
	__asm {
		nop;
		xor  eax, eax;     // (mov eax,0)
		push flags;        // render flags, 0 for defaults
		push color23;      // color of second/third line
		push 0;            // "DisplayMsg" what's that for?
		push color1;       // color of first line
		push 116;          // not sure is it really y
		mov  ecx, 192;     // not sure is it really x
		mov  ebx, extra;   // length of extra lines array, 0-2
		mov  edx, text23;  // second/third line
		mov  eax, text1;   // first line
		call fo::funcoffs::dialog_out_;//(text1, text23, extra, x | y, color1, ?=0, color23, flags);
		mov  result, eax;  // always (?) returns 1 on success, unless YESNO flag is set; error values are unknown
	}

	delete text1;
	for (uint32_t idx = 0; idx < extra; idx++) {
		delete[] text23[idx];
	}
	delete[] text23;

	return result;
}

static bool dialogOut = false;
void r_message_box(sfall::script::OpcodeContext& ctx) {
	if (dialogOut)
		return;

	// Converting char* to std::string just to convert them back to char* again is ... questionable design ... i agree
	// However, it allows scripts and dll use exactly same function, so i'll stick to that instead of code duplication, unless someone write better solution
	std::vector<std::string> strings;

	if (ctx.arg(0).isString())
		strings.push_back(ctx.arg(0).strValue());
	else {
		auto id = ctx.arg(0).rawValue();

		if (sfall::script::arrays.find(id) == sfall::script::arrays.end()) { // make sure it IS array
			DialogOut({"ERROR", "array deleted"}, DIALOGOUT_SMALL | DIALOGOUT_CLEAN, 134);
			ctx.setReturn(-1, sfall::script::DataType::INT);
			return;
		}

		for (int32_t idx = 0, size = min(sfall::script::LenArray(id), 3); idx < size; idx++) { // ignore everything after 3rd element
			strings.push_back(sfall::script::GetArrayKey(id, idx).asString());
		}
	}

	// must match defaults used by DialogOut()
	int32_t flags  = ctx.numArgs() >= 2 ? ctx.arg(1).asInt() : DIALOGOUT_NORMAL;
	int32_t color1 = ctx.numArgs() >= 3 ? ctx.arg(2).asInt() : DialogOutColor;
	int32_t color2 = ctx.numArgs() >= 4 ? ctx.arg(3).asInt() : DialogOutColor;

	dialogOut = true;
	int32_t result = DialogOut(strings, flags, color1, color2);
	dialogOut = false;

	ctx.setReturn(result, sfall::script::DataType::INT);
}

void r_set_hotspot_title(sfall::script::OpcodeContext& ctx) {
	int x = ctx.arg(0).asInt();
	int y = ctx.arg(1).asInt();
	const char* msg = ctx.arg(2).asString();

	sfall::UpdateTileTerrainMsg(x, y, msg);
}

void r_tolower(sfall::script::OpcodeContext& ctx) {
	auto str = std::string(ctx.arg(0).asString()); // sue me.
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);

	ctx.setReturn(cstrdup(str.c_str()));
}

void r_toupper(sfall::script::OpcodeContext& ctx) {
	auto str = std::string(ctx.arg(0).asString()); // sue me.
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);

	ctx.setReturn(cstrdup(str.c_str()));
}

// Used by scripts to detect if game is using customized ddraw.dll
void r_otators(sfall::script::OpcodeContext& ctx) {
	ctx.setReturn("Rotators, rotate!");
}

// Module

static const sfall::script::SfallMetarule metarules[] = {
	{ "r_message_box",        r_message_box,           1, 4, -1, {sfall::script::ARG_INTSTR, sfall::script::ARG_INT, sfall::script::ARG_INT, sfall::script::ARG_INT} },
	{ "r_get_ini_string",     r_get_ini_string,        4, 4, -1, {sfall::script::ARG_STRING, sfall::script::ARG_STRING, sfall::script::ARG_STRING, sfall::script::ARG_STRING} },
	{ "r_set_hotspot_title",  r_set_hotspot_title,     3, 3, -1, {sfall::script::ARG_INT, sfall::script::ARG_INT, sfall::script::ARG_STRING} },
	{ "r_tolower",            r_tolower,               1, 1, -1, {sfall::script::ARG_STRING} },
	{ "r_toupper",            r_toupper,               1, 1, -1, {sfall::script::ARG_STRING} },

	{ "rotators",             r_otators,               0, 0 }
};

void sfall::Script::init() {
	for (auto metarule = std::begin(metarules); metarule != std::end(metarules); ++metarule) {
		if (sfall::script::metaruleTable.find(metarule->name) != sfall::script::metaruleTable.end())
			misc::CriticalFail("Metarule name collision: " + std::string(metarule->name));

		std::string name, args;
		if (metarule->minArgs == metarule->maxArgs)
			name = std::to_string(metarule->minArgs);
		else // disallow?
			name = "[" + std::to_string(metarule->minArgs) + "-" + std::to_string(metarule->maxArgs) + "]";

		for (uint8_t arg = 0; arg < metarule->maxArgs; arg++) {
			args += ", ";

			if (arg + 1 > metarule->minArgs)
				args += "[";

			switch (metarule->argValidation[arg]) {
				case sfall::script::ARG_ANY:
					args += "ARG_ANY";
					break;
				case sfall::script::ARG_INT:
					args += "ARG_INT";
					break;
				case sfall::script::ARG_OBJECT:
					args += "ARG_OBJECT";
					break;
				case sfall::script::ARG_STRING:
					args += "ARG_STRING";
					break;
				case sfall::script::ARG_INTSTR:
					args += "ARG_INTSTR";
					break;
				case sfall::script::ARG_NUMBER:
					args += "ARG_NUMBER";
					break;
				default:
					args += "<" + std::to_string(metarule->argValidation[arg]) + ">";
					break;
			}
			if (arg + 1 > metarule->minArgs)
				args += "]";
		}

		sfall::dlog_f("> sfall_func%s(\"%s\"%s)\n", DL_INIT, name.c_str(), metarule->name, args.c_str());
		sfall::script::metaruleTable[metarule->name] = metarule;
	}
}
