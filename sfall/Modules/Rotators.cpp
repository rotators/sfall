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

const char* currentTerrainStr;

// DisplayTerrainOnHotspotHover related variables
bool isMouseOverHotspot;
bool displayTerrainOnHotspot;
BYTE terrainOnHotspotTextColor;
BYTE terrainOnHotspotShadowColor;

//

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

//

int jmpBack = 0x4BFE89;
void __declspec(naked) wmDetectHotspotHover() {
	int wmMouseX, wmMouseY;
	int deltaX, deltaY;
	bool oldIsMouseOverHotspot;
	__asm {
		pushad
		mov eax, dword ptr ss : [esp + 36] // 4+32 (because of pushad)
		mov edx, dword ptr ss : [esp + 40]
		mov ebp, esp
		sub esp, __LOCAL_SIZE
		mov wmMouseY, eax
		mov wmMouseX, edx
	}
	oldIsMouseOverHotspot = isMouseOverHotspot;
	deltaX = abs((long)fo::var::world_xpos - (wmMouseX - 20 + fo::var::wmWorldOffsetX));
	deltaY = abs((long)fo::var::world_ypos - (wmMouseY - 20 + fo::var::wmWorldOffsetY));

	isMouseOverHotspot = deltaX < 8 && deltaY < 5;
	if (isMouseOverHotspot != oldIsMouseOverHotspot)
		fo::func::wmInterfaceRefresh();

	__asm {
		mov esp, ebp; // restore stack
		popad;
		mov eax, dword ptr ds : [0x51DE30] // we overwrote it in jmp, so it's here
		jmp jmpBack
	}
}

const char* GetWorldmapMsg(int msgId) {
	return fo::GetMessageStr((fo::MessageList*)MSG_FILE_WORLDMAP, msgId);
}

// player is on a green circle.
bool IsOnLocation() {
	return fo::var::WorldMapCurrArea != -1;
}

void GetCurrentTerrain() {
	int*& terrainId = *reinterpret_cast<int**>(FO_VAR_world_subtile);
	if (terrainId == NULL)
		currentTerrainStr = "";
	else
		currentTerrainStr = GetWorldmapMsg(1000 + *terrainId);
	// TODO: Handle special terrain areas
}

static void SetFont(long ref) {
	fo::func::text_font(ref);
}

static long GetFont() {
	return fo::var::curr_font_num;
}

bool IsMovingOnWM() {
	return fo::var::target_xpos + fo::var::target_ypos > 0;
}

void FMTextToBuffer(void* buffer, char* text, BYTE colorIndex, DWORD x, DWORD y, DWORD txtWidth, DWORD bufferWidth)
{
	DWORD posOffset = y * bufferWidth + x;
	__asm {
		xor eax, eax
		mov al, colorIndex
		push eax
		mov eax, buffer
		add eax, posOffset
		mov edx, text
		mov ebx, txtWidth
		mov ecx, bufferWidth
		call fo::funcoffs::FMtext_to_buf_
	}
}

void WmDrawText(char* text, BYTE colorIndex, DWORD x, DWORD y, DWORD txtWidth) {
	int*& buf = *reinterpret_cast<int**>(FO_VAR_wmBkWinBuf);
	FMTextToBuffer((void*)buf, text, colorIndex, x, y, txtWidth, 890); // 890=width of wm buffer
}

// Called from Interface.cpp wmInterfaceRefresh_hook()
void sfall::Rotators::OnWmRefresh() {
	if (!displayTerrainOnHotspot)
		return;

	auto oldFont = GetFont();
	GetCurrentTerrain();
	SetFont(0x65);
	if (isMouseOverHotspot == 1 && !IsMovingOnWM() && !IsOnLocation()) {
		auto x = fo::var::world_xpos - fo::var::wmWorldOffsetX;
		auto y = fo::var::world_ypos - fo::var::wmWorldOffsetY;
		WmDrawText((char*)currentTerrainStr, terrainOnHotspotShadowColor, x, y + 5, 60);  // Shadow
		WmDrawText((char*)currentTerrainStr, terrainOnHotspotTextColor, x - 1, y + 4, 60);
	}
	SetFont(oldFont);
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

// https://i.imgur.com/0bu4J2l.png
static void InitTerrainHover()
{
	using namespace rfall;

	displayTerrainOnHotspot     = Ini::Int("Interface", "DisplayTerrainOnHotspotHover", 0);
	terrainOnHotspotTextColor   = Ini::Int("Interface", "TerrainOnHotspotTextColor", 215);
	terrainOnHotspotShadowColor = Ini::Int("Interface", "TerrainOnHotspotTextShadowColor", 228);
	if(displayTerrainOnHotspot)
		sfall::MakeJump(0x4BFE84, wmDetectHotspotHover);
	currentTerrainStr = "";
}

void sfall::Rotators::init()
{
	SafeWrite8(0x410003, 0xF4);

	SubModules.add<HTTPD>();
	SubModules.add<LoadDll>();
	//SubModules.add<Sandbox>();

	SubModules.initAll();

	InitTerrainHover();
}

void sfall::Rotators::exit() {
	SubModules.exitAll();
}
