#include <algorithm>
#include <string>
#include <vector>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Rotators.h"
#include "Rotators.HTTPD.h"
#include "Rotators.LoadDll.h"
#include "Rotators.Sandbox.h"
#include "Rotators.Script.h"

//
// Remember to wear protective goggles :)
//

// Submodules handling

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

void* rfall::db::fastread(const char* filename)
{
	if (!strlen(filename))
		return nullptr;

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

// Misc stuff

void rfall::misc::CriticalFail(const std::string& message) {
	MessageBoxA(0, message.c_str(), "Error", MB_TASKMODAL | MB_ICONERROR);
	ExitProcess(1);
}

// Taken from sfall::script::FillListVector() Scripting/Handlers/Arrays.cpp
void rfall::misc::FillListVector(FLV type, std::vector<fo::GameObject*>& vec, int8_t elevation /* = -1 */) {
	// current
	if (elevation == -1) {
		if (fo::var::obj_dude == nullptr) {
			sfall::dlogr( "r::FillListVector() : fo::var::obj_dude == nullptr", DL_MAIN);
			return;
		}

		sfall::dlog_f( "r::FillListVector() : fo::var::obj_dude->elevation == %d", DL_MAIN, fo::var::obj_dude->elevation);
		elevation = static_cast<int8_t>(fo::var::obj_dude->elevation);
	}
	// all || specific
	else if( elevation == -2 || (elevation >= 0 && elevation <= 2)) {
		// pass
	}
	// out of range
	else {
		sfall::dlog_f( "r::FillListVector() : invalid elevation == %d", DL_MAIN, elevation);
		return;
	}

	const int8_t elev_min = elevation == -2 ? 0 : elevation;
	const int8_t elev_max = elevation == -2 ? 2 : elevation;

	vec.reserve(100);

	if (type == FLV::SPATIAL) {
		fo::ScriptInstance* scriptPtr;
		fo::GameObject* self_obj;
		fo::Program* programPtr;
		for (int8_t elev = elev_min; elev <= elev_max; elev++) {
			scriptPtr = fo::func::scr_find_first_at(elev);
			while (scriptPtr != nullptr) {
				self_obj = scriptPtr->selfObject;
				if (self_obj == nullptr) {
					programPtr = scriptPtr->program;
					self_obj = fo::func::scr_find_obj_from_program(programPtr);
				}
				vec.push_back(self_obj);
				scriptPtr = fo::func::scr_find_next_at();
			}
		}
	}
	else if (type != FLV::TILES) {
		for (int8_t elev = elev_min; elev <= elev_max; elev++) {
			for (uint16_t tile = 0; tile < 40000; tile++) {
				fo::GameObject* obj = fo::func::obj_find_first_at_tile(elev, tile);
				while (obj) {
					DWORD otype = obj->Type();
					if (type == FLV::ALL ||
					   (type == FLV::CRITTERS && otype == 1) ||
					   (type == FLV::GROUNDITEMS && otype == 0) ||
					   (type >= FLV::SCENERY && type <= FLV::MISC && static_cast<DWORD>(type) == otype)) {
						vec.push_back(obj);
					}
					obj = fo::func::obj_find_next_at_tile();
				}
			}
		}
	}
}


//

void sfall::Rotators::init() {
	SafeWrite8(0x410003, 0xF4);

	SubModules.add<HTTPD>(); // dummy on v140_xp
	SubModules.add<LoadDll>();
	SubModules.add<Script>();

	#if _MSC_VER >= 1920
	SubModules.add<Sandbox>();
	#endif

	SubModules.initAll();
}

void sfall::Rotators::exit() {
	SubModules.exitAll();
}
