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
// "Conflicting" flags (YESNO|CLEAN, NORMAL|TINY) seems to prioritize flag with smaller value
enum DialogOutFlags : uint8_t {
	DIALOGOUT_NORMAL      = 0x01, // uses regular graphic
	DIALOGOUT_SMALL       = 0x02, // uses smaller graphic
	DIALOGOUT_ALIGN_LEFT  = 0x04, // text moved to left
	DIALOGOUT_ALIGN_TOP   = 0x08, // text moved to top
	DIALOGOUT_YESNO       = 0x10, // DONE button replaced with YES/NO
	DIALOGOUT_CLEAN       = 0x20  // no buttons
};

// default color index for text used by most dialogs; character creation screen uses green in few cases
static constexpr uint8_t DialogOutColor = 145;

// NOTE: none of strings[N].c_str() pointers can be invalidated before they reach engine
static bool DialogOut(const std::vector<std::string> strings, int32_t flags = DIALOGOUT_NORMAL, int32_t color1 = DialogOutColor, int32_t color23 = DialogOutColor) {
	if (strings.empty())
		return false;

	// always freeze scripts engine, even if function has been called by dll
	*(DWORD*)FO_VAR_script_engine_running = 0;

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

	int32_t result = 0;
	__asm {
		nop;
		push flags;        // render flags, see DialogOutFlags; if 0, engine sets DIALOGOUT_NORMAL or DIALOGOUT_SMALL (depends on text size)
		push color23;      // color index for second/third line
		push 0;            // "DisplayMsg" what's that for?
		push color1;       // color index for first line
		push 116;          // not sure is it really y
		mov  ecx, 192;     // not sure is it really x
		mov  ebx, size23;  // size of extra lines array, 0-2
		mov  edx, text23;  // second/third line
		mov  eax, text1;   // first line
		call fo::funcoffs::dialog_out_;//(text1, text23, size23, x | y, color1, ?=0, color23, flags);
		mov result, eax;   // returns 0 or 1 if DIALOGOUT_YESNO is set, 1 otherwise
	}

	if (size23)
		delete[] text23;

	*(DWORD*)FO_VAR_script_engine_running = 1;

	return result ? true : false;
}

void r_message_box(sfall::script::OpcodeContext& ctx) {
	// Converting char* to std::string just to convert them back to char* again is ... questionable design ... i agree
	// However, it allows scripts and dll use exactly same function, so i'll stick to that instead of code duplication, unless someone write better solution
	// Changing DialogOut() to accept char*, char** is not one of them
	std::vector<std::string> strings = sfall::split(ctx.arg(0).asString(), '|');

	if (strings.size() > 3)
		strings.resize(3);

	// Must match defaults used by DialogOut()
	int32_t flags  = ctx.numArgs() >= 2 ? ctx.arg(1).asInt() : DIALOGOUT_NORMAL;
	int32_t color1 = ctx.numArgs() >= 3 ? ctx.arg(2).asInt() : DialogOutColor;
	int32_t color2 = ctx.numArgs() >= 4 ? ctx.arg(3).asInt() : DialogOutColor;

	bool result = DialogOut(strings, flags, color1, color2);
	ctx.setReturn( result ? 1 : 0, sfall::script::DataType::INT);
}

// Used by scripts to detect if game is using customized ddraw.dll
void r_otators(sfall::script::OpcodeContext& ctx) {
	ctx.setReturn("Rotators, rotate!");
}

// Voodoo

/*
#define r_call_offset_push(val)   sfall_func("r_call_offset_push", val)
#define r_call_offset(addr)       sfall_func("r_call_offset", addr)
#define r_hrp                     sfall_func("r_hrp")
#define r_hrp_offset(addr)        sfall_func("r_hrp_offset", addr)
#define r_write_byte(addr,val)    sfall_func("r_write", 0, addr, val)
#define r_write_short(addr,val)   sfall_func("r_write", 1, addr, val)
#define r_write_int(addr,val)     sfall_func("r_write", 2, addr, val)
#define r_write_string(addr,val)  sfall_func("r_write", 3, addr, val)
*/

static int32_t call_offset_buff[16];
static int32_t call_offset_args;

void r_call_offset_push(sfall::script::OpcodeContext& ctx) {
	int32_t value = ctx.arg(0).asInt();

	if(call_offset_args >= sizeof(call_offset_buff) / sizeof(int32_t)) {
		ctx.printOpcodeError("maximum %d arguments allowed", sizeof(call_offset_buff) / sizeof(int32_t));

		ctx.setReturn(-1, sfall::script::DataType::INT);
		return;
	}

	call_offset_buff[call_offset_args++] = value;
}

void r_call_offset(sfall::script::OpcodeContext& ctx) { // watcom
	static int addr, arg, result;

	addr = ctx.arg(0).asInt();
	arg = 0;

	// LTR -> RTL
	if(call_offset_args >= 6)
		std::reverse(std::begin(call_offset_buff) + 4, std::begin(call_offset_buff) + call_offset_args);

	__asm {
		nop;
		push esi;
		jmp _check;
	_loop:
		mov esi, arg;
	//_eax:
		cmp arg, 0;
		jne _edx;
		mov eax, call_offset_buff[esi*4];
		jmp _inc;
	_edx:
		cmp arg, 1;
		jne _ebx;
		mov edx, call_offset_buff[esi*4];
		jmp _inc;
	_ebx:
		cmp arg, 2;
		jne _ecx;
		mov ebx, call_offset_buff[esi*4];
		jmp _inc;
	_ecx:
		cmp arg, 3;
		jne _push;
		mov ecx, call_offset_buff[esi*4];
		jmp _inc;
	_push:
		push call_offset_buff[esi*4];
	_inc:
		inc arg;
	_check:
		mov esi, call_offset_args;
		cmp arg, esi;
		jne _loop;
	//_call:
		call addr;
		mov result, eax;
		pop esi;
	}

	std::memset(call_offset_buff, 0, sizeof(call_offset_buff));
	call_offset_args = 0;

	ctx.setReturn(result, sfall::script::DataType::INT);
}

void r_call_offset_cdecl(sfall::script::OpcodeContext& ctx) {
	static int addr, arg, result;

	addr = ctx.arg(0).asInt();
	arg = 0;

	// LTR -> RTL
	if(call_offset_args >= 2)
		std::reverse(std::begin(call_offset_buff), std::begin(call_offset_buff) + call_offset_args);

	__asm {
		nop;
		push esi;
		jmp _check;
		hlt;
	_loop:
		mov esi, arg;
		push call_offset_buff[esi*4];
	//_inc:
		inc arg;
	_check:
		mov esi, call_offset_args;
		cmp arg, esi;
		jne _loop;
	//_call:
		call addr;
		mov result, eax;
		mov eax, call_offset_args;
		mov esi, 4;
		mul esi;
		add esp, eax;
		pop esi;
	}

	std::memset(call_offset_buff, 0, sizeof(call_offset_buff));
	call_offset_args = 0;

	ctx.setReturn(result, sfall::script::DataType::INT);
}

void r_hrp(sfall::script::OpcodeContext& ctx) {
	ctx.setReturn(HRPOK ? 1 : 0, sfall::script::DataType::INT);
}

void r_hrp_offset(sfall::script::OpcodeContext& ctx) {
	ctx.setReturn(sfall::HRPAddress(ctx.arg(0).asInt()), sfall::script::DataType::INT);
}

void r_write(sfall::script::OpcodeContext& ctx) {
	int32_t type = ctx.arg(0).asInt();
	int32_t addr = ctx.arg(1).asInt();

	// type 0-2 must be int
	if((type >= 0 && type <= 2) && !ctx.arg(2).isInt()) {
		ctx.printOpcodeError("invalid value, expected INT");

		ctx.setReturn(-1, sfall::script::DataType::INT);
		return;
	}
	// type 3 must be string
	else if(type == 3 && !ctx.arg(2).isString()) {
		ctx.printOpcodeError("invalid value, expected STRING");

		ctx.setReturn(-1, sfall::script::DataType::INT);
		return;
	}

	switch(type) {
		case 0: // r_write_byte
			sfall::SafeWrite8(addr, static_cast<BYTE>(ctx.arg(2).asInt()));
			break;
		case 1: // r_write_short
			sfall::SafeWrite16(addr, static_cast<WORD>(ctx.arg(2).asInt()));
			break;
		case 2: // r_write_int
			sfall::SafeWrite32(addr, static_cast<DWORD>(ctx.arg(2).asInt()));
			break;
		case 3: // r_write_string
			sfall::SafeWriteStr(addr, ctx.arg(2).strValue());
			break;
		default:
			ctx.printOpcodeError("invalid write type<%d>", type);
			ctx.setReturn(-1, sfall::script::DataType::INT);
			return;
	}

	ctx.setReturn(0, sfall::script::DataType::INT);
}

// Module

static const sfall::script::SfallMetarule metarules[] = {
	{ "r_get_ini_string",     r_get_ini_string,        4, 4, -1, {sfall::script::ARG_STRING, sfall::script::ARG_STRING, sfall::script::ARG_STRING, sfall::script::ARG_STRING} },
	{ "r_message_box",        r_message_box,           1, 4, -1, {sfall::script::ARG_STRING, sfall::script::ARG_INT, sfall::script::ARG_INT, sfall::script::ARG_INT} },

	{ "rotators",             r_otators,               0, 0 }
};

static const sfall::script::SfallMetarule unsafe_metarules[] = {
	{ "r_call_offset_push",   r_call_offset_push,      1, 1, -1, {sfall::script::ARG_INT} },
	{ "r_call_offset",        r_call_offset,           1, 1, -1, {sfall::script::ARG_INT} },
	{ "r_call_offset_cdecl",  r_call_offset_cdecl,     1, 1, -1, {sfall::script::ARG_INT} },
	{ "r_hrp",                r_hrp,                   0, 0,  0 },
	{ "r_hrp_offset",         r_hrp_offset,            1, 1,  0, {sfall::script::ARG_INT} },
	{ "r_write",              r_write,                 3, 3, -1, {sfall::script::ARG_INT, sfall::script::ARG_INT, sfall::script::ARG_INTSTR} },

	{ "voodoo",               r_otators,               0, 0 }
};

void AddMetarule(const sfall::script::SfallMetarule* metarule, std::string info = std::string()) {
	if (sfall::script::metaruleTable.find(metarule->name) != sfall::script::metaruleTable.end())
		misc::CriticalFail("Metarule name collision: " + std::string(metarule->name));

	if(!info.empty())
		info = " (" + info + ")";

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

	sfall::dlog_f(">%s sfall_func%s(\"%s\"%s)\n", DL_INIT, info.c_str(), name.c_str(), metarule->name, args.c_str());
	sfall::script::metaruleTable[metarule->name] = metarule;
}

void Script::init() {
	for (auto metarule = std::begin(metarules); metarule != std::end(metarules); ++metarule) {
		AddMetarule(metarule);
	}

	Ini ddraw_ini;
	if( !ddraw_ini.LoadFile("ddraw.ini") || !ddraw_ini.GetBool("Debugging", "AllowUnsafeScripting", false))
		return;

	for (auto metarule = std::begin(unsafe_metarules); metarule != std::end(unsafe_metarules); ++metarule) {
		AddMetarule(metarule, "unsafe");
	}

	std::memset(call_offset_buff, 0, sizeof(call_offset_buff));
	call_offset_args = 0;
}

void Script::exit() {
}
