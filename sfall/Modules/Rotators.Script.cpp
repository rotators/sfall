#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <unordered_map>

#include "../Logging.h"
#include "../SafeWrite.h"
#include "../Utils.h"
#include "../FalloutEngine/Fallout2.h"

#include "Scripting/Arrays.h"
#include "Scripting/OpcodeContext.h"
#include "Scripting/ScriptValue.h"

#include "Interface.h"
#include "ScriptExtender.h"

#include "Rotators.h"
#include "Rotators.Script.h"

using namespace rfall;

// Declared in Scripting/Handlers/Metarule.cpp
namespace sfall { namespace script { extern std::unordered_map<std::string, const sfall::script::SfallMetarule*> metaruleTable; }}

// Declared in ScriptExtender.cpp
namespace sfall { extern long overrideScriptStructFixedParam; }

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

static void RunProgramWithFixedParam(fo::Program* program, std::string procName, long fixedParam) {
	const char* proc = procName.c_str();
	int procNum = fo::func::interpretFindProcedure(program, proc);
	if (procNum != -1) {
		sfall::overrideScriptStructFixedParam = fixedParam;
		fo::func::executeProcedure(program, procNum);
		sfall::overrideScriptStructFixedParam = 0;
	}
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
	DIALOGOUT_YESNO       = 0x10, // DONE button replaced with YES/NO -- WIP, useless in scripts in current state
	DIALOGOUT_CLEAN       = 0x20  // no buttons
};

// default color index for text used by most dialogs; character creation screen uses green in few cases
static constexpr uint8_t DialogOutColor = 145;

struct DialogOutData
{
	// General data
	uint8_t Type; // dialog_out_ caller type; 0=engine, 1=dll, 2=ssl

	// DLL data
	// TODO

	// SSL data
	fo::Program* Program;
	std::string  ProcName;

	// Result
	uint32_t Input;

	DialogOutData() {
		Clear();
	}

	void Clear()
	{
		Type = Input = 0;
		Program = nullptr;
		ProcName.clear();
		Input = 0;
	};
};
static struct DialogOutData dialogOut;

// NOTE: none of strings[N].c_str() pointers can be invalidated before they reach engine
static bool __stdcall DialogOut(const std::vector<std::string> strings, int32_t flags = DIALOGOUT_NORMAL, int32_t color1 = DialogOutColor, int32_t color23 = DialogOutColor) {
	if (strings.empty())
		return false;

	// most likely should be `min(strings.size(), 3) - 1` for public use
	// not applying limit here in case someone wants to experiment with it
	const uint32_t size23 = strings.size() - 1;

	// convert first line to char*
	// no length validation here; empty string is allowed, as it doesn't seem to break anything
	// when/if drawing directly on screen will be possible some day, it might be useful for displaying small images instead of text only
	const char* text1 = strings.front().c_str();

	// convert second/third line to char**
	char** text23 = nullptr;
	if (size23) {
		text23 = new char*[size23];

		for (uint32_t idx = 0; idx < size23; idx++) {
			text23[idx] = (char*)strings[idx+1].c_str();
		}
	}

	if (!dialogOut.Type)
		dialogOut.Type = 1;

	__asm {
		nop;
		push flags;        // render flags, see DialogOutFlags; if 0, engine sets DIALOGOUT_NORMAL or DIALOGOUT_SMALL (depends on text width)
		push color23;      // color index for second/third line
		push 0;            // "DisplayMsg" what's that for?
		push color1;       // color index for first line
		push 116;          // not sure is it really y
		mov  ecx, 192;     // not sure is it really x
		mov  ebx, size23;  // size of extra lines array, 0-2
		mov  edx, text23;  // second/third line
		mov  eax, text1;   // first line
		call fo::funcoffs::dialog_out_;//(text1, text23, size23, x | y, color1, ?=0, color23, flags);
	}

	if (size23)
		delete[] text23;

	return true;
}

static void __declspec(naked) DialogOut_get_input_hook() { // cache get_input_ return value until engine decides what to do next
	static int input;

	__asm {
		nop;
		call fo::funcoffs::get_input_; // restore
		mov input, eax;
	}

	dialogOut.Input = input;

	__asm {
		nop;
		ret;
	}
}

static void __stdcall DialogOut_message_exit_cpp() {
	if (dialogOut.Type == 0) {
		/* pass without changes */
	}
	else {
		bool result = false;

		switch (dialogOut.Input) {
			case 'Y':
			case 'y':
			case '\r':  // enter
			case 0x1f4: // button
				result = true;
				break;
			case 'N':
			case 'n':
			case 27:    // esc
			case 0x1f5: // button
				result = false;
				break;
			default:
				sfall::dlog_f( "DialogOut : unknown input<%d>\n", DL_MAIN, dialogOut.Input );
				break;
		}

		if (dialogOut.Type == 1) {
			// TODO
		}
		else if (dialogOut.Type == 2 && dialogOut.Program && !dialogOut.ProcName.empty()) {
			RunProgramWithFixedParam(dialogOut.Program, dialogOut.ProcName, result ? 1 : 0);
		}
	}

	dialogOut.Clear();
}

static void __declspec(naked) DialogOut_message_exit_hook() {
	__asm {
		nop;
		call fo::funcoffs::message_exit_; // restore
		pushad;
	}

	DialogOut_message_exit_cpp();

	__asm {
		nop;
		popad;
		ret;
	}
}

void r_message_box(sfall::script::OpcodeContext& ctx) {
	if (dialogOut.Program) {
		ctx.setReturn(-1, sfall::script::DataType::INT);
		return;
	}

	// Converting char* to std::string just to convert them back to char* again is ... questionable design ... i agree
	// However, it allows scripts and dll use exactly same function, so i'll stick to that instead of code duplication, unless someone write better solution
	std::vector<std::string> strings = sfall::split(ctx.arg(0).asString(), '|');

	if (strings.size() > 3)
		strings.resize(3);

	// Must match defaults used by DialogOut()
	int32_t flags  = ctx.numArgs() >= 2 ? ctx.arg(1).asInt() : DIALOGOUT_NORMAL;
	int32_t color1 = ctx.numArgs() >= 3 ? ctx.arg(2).asInt() : DialogOutColor;
	int32_t color2 = ctx.numArgs() >= 4 ? ctx.arg(3).asInt() : DialogOutColor;

	if (ctx.numArgs() >= 5) {
		dialogOut.Type = 2;
		dialogOut.Program = ctx.program();
		dialogOut.ProcName =  ctx.arg(4).asString();
	}

	if( DialogOut(strings,flags, color1, color2))
		ctx.setReturn(0, sfall::script::DataType::INT);
	else
		ctx.setReturn(-1, sfall::script::DataType::INT);
}

// Used by scripts to detect if game is using customized ddraw.dll
void r_otators(sfall::script::OpcodeContext& ctx) {
	ctx.setReturn("Rotators, rotate!");
}

// Module

static const sfall::script::SfallMetarule metarules[] = {
	{ "r_get_ini_string",     r_get_ini_string,        4, 4, -1, {sfall::script::ARG_STRING, sfall::script::ARG_STRING, sfall::script::ARG_STRING, sfall::script::ARG_STRING} },
	{ "r_message_box",        r_message_box,           1, 5, -1, {sfall::script::ARG_STRING, sfall::script::ARG_INT, sfall::script::ARG_INT, sfall::script::ARG_INT, sfall::script::ARG_STRING} },

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

	HookCall(0x41dd84, DialogOut_get_input_hook);    // dialog_out_
	HookCall(0x41de78, DialogOut_message_exit_hook); // dialog_out_
}

void sfall::Script::exit() {
}
