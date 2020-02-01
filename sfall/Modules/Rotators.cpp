#include <algorithm>
#include <string>
#include <vector>

#include "..\main.h"
#include "..\Logging.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Rotators.h"
#include "Rotators.HTTPD.h"
#include "Rotators.LoadDll.h"
#include "Rotators.Sandbox.h"
#include "Rotators.Script.h"

//
// Remember to wear protective goggles :)
//

rfall::Ini rfall::ini;

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

// Fixes the bug that causes the barter button to not animate until after leaving the trade screen. 
// The bug is due to the pointers to the frm graphics not being loaded, causing the button the get default graphics (and no button_down graphic),
// so we insert a hook in gdialog_window_create_ before the button is added to the window. In this hook function we load the graphic.
int dialogInitHook = 0x44A78B;
static void __declspec(naked) DialogButtonFix() {
	__asm {
		push 0
		mov edx, 0x60
		mov eax, 0x6
		xor ecx, ecx
		xor ebx, ebx
		call fo::funcoffs::art_id_
		mov ecx, 0x58F46C
		xor ebx, ebx
		xor edx, edx
		call fo::funcoffs::art_ptr_lock_data_
		mov ds : [0x0058F4AC] , eax  // _dialog_red_button_up_buf
		test eax, eax
		je ret_ // null ptr
		push 0
		mov edx, 0x5F
		mov eax, 0x6
		xor ecx, ecx
		xor ebx, ebx
		call fo::funcoffs::art_id_
		mov ecx, 0x58F4BC
		xor ebx, ebx
		xor edx, edx
		call fo::funcoffs::art_ptr_lock_data_
		mov ds : [0x0058F4A4], eax // _dialog_red_button_down_buf
		mov ebp, eax
	ret_:
		jmp dialogInitHook
	}
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
			sfall::dlogr("r::FillListVector() : fo::var::obj_dude == nullptr", DL_MAIN);
			return;
		}

		sfall::dlog_f("r::FillListVector() : fo::var::obj_dude->elevation == %d", DL_MAIN, fo::var::obj_dude->elevation);
		elevation = static_cast<int8_t>(fo::var::obj_dude->elevation);
	}
	// all || specific
	else if (elevation == -2 || (elevation >= 0 && elevation <= 2)) {
		// pass
	}
	// out of range
	else {
		sfall::dlog_f("r::FillListVector() : invalid elevation == %d", DL_MAIN, elevation);
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
	if (!rfall::ini.LoadFile("ddraw.rotators.ini"))
		dlogr("> configuration not found", DL_INIT);

	SafeWrite8(0x410003, 0xF4);

	MakeJump(0x44A785, DialogButtonFix);

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
