#include <string>
#include <vector>
#include <map>

#include "..\main.h"
#include "..\SafeWrite.h"
#include "..\Utils.h"
#include "..\FalloutEngine\Fallout2.h"
#include "MainLoopHook.h"
#include "FileSystem.h"
#include "LoadGameHook.h"

#include "ScriptExtender.h"

#include "Rotators.h"
#include "Rotators.HTTPD.h"
#include "Rotators.LoadDll.h"
#include "Rotators.Sandbox.h"

// Remember to wear protective goggles :)

void rfall::SubModuleManager::initAll() {
	for (const auto& module : _modules) {
		sfall::dlog_f("Initializing module Rotators->%s...\n", DL_INIT, module->name());
		module->init();
	}
}

void rfall::SubModuleManager::exitAll() {
	for (const auto& module : _modules) {
		sfall::dlog_f("Exiting module Rotators->%s...\n", DL_INIT, module->name());
		module->exit();
	}
}

// Any and all configuration should be read from ddraw.rotators.ini; /artifacts/ddraw.rotators.ini should be updated to reflect code, when possible;
// adds some extra work on PR/merge, but pays off in a long run

const char  rotatorsIni[] = ".\\ddraw.rotators.ini";

std::string rfall::Ini::String(const char* section, const char* setting, const char* defaultValue) {
	return sfall::GetIniString(section, setting, defaultValue, 512, rotatorsIni);
}

int rfall::Ini::Int(const char* section, const char* setting, int defaultValue) {
	return sfall::iniGetInt(section, setting, defaultValue, rotatorsIni);
}

std::vector<std::string> rfall::Ini::List(const char* section, const char* setting, const char* defaultValue, char delim /* = ',' */) {
	return sfall::GetIniList(section, setting, defaultValue, 512, delim, rotatorsIni);
}

// Shortcuts and filling gaps in fo::func::db_*()

void* rfall::db::readfile(char* filename, int len)
{
	auto buffer = malloc(len);
	__asm {
		mov eax, filename
		mov edx, buffer
		call fo::funcoffs::db_read_to_buf_
	}
	return buffer;
}

int rfall::db::filelen(fo::DbFile* dbFile) {
	int len=0;
	__asm {
		mov eax, dbFile
		call fo::funcoffs::xfilelength_
		mov len, eax
	}
	return len;
}

void* rfall::db::fastread(char* filename)
{
	if (fo::func::db_access(filename)) {
		fo::DbFile* file = fo::func::db_fopen(filename, "r");
		int len = 0;

		__asm {
			mov eax, file
			call fo::funcoffs::xfilelength_
			mov len, eax
		}

		auto buffer = malloc(len);

		__asm {
			mov eax, filename
			mov edx, buffer
			call fo::funcoffs::db_read_to_buf_
		}	

		fo::func::db_fclose(file);
		return buffer;
	}

	return nullptr;
}

extern void* LoadGameHookFuncAddress;
int LoadScreenInit = 0x480AF5;
static void MainHook() { }
int LoadSlot;
static __declspec(naked) void LoadScreenInitHook() {
	LoadSlot = rfall::Ini::Int("Debugging", "AutoLoadSlot", -1);
	if (LoadSlot != -1) {
		__asm {
			jmp LoadScreenInit
		}
	}
}

int autoLoadAfter = 0x47CAE5;
// Not totally working, need to change "destination screen" somewhere.
/*static __declspec(naked) void AutoLoadSave() {
	LoadSlot = Ini::Int("Debugging", "AutoLoadSlot", -1);
	if (LoadSlot != -1) {
		fo::var::slot_cursor = LoadSlot;
		__asm {
			mov eax, fo::var::slot_cursor
			call fo::funcoffs::LoadSlot_
			//call fo::funcoffs::getInput_
			//jmp autoLoadAfter
		}
	}
}*/

void sfall::Rotators::init()
{
	SafeWrite8(0x410003, 0xF4);

	SubModules.add<HTTPD>();
	SubModules.add<LoadDll>();
	//SubModules.add<Sandbox>();

	SubModules.initAll();
}

void sfall::Rotators::exit() {
	SubModules.exitAll();
}
