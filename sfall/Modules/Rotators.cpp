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

#ifdef HTTPD_SERVER
	#include <thread>
	#include "..\Lib\EmbeddableWebServer.h"

	void InitHTTPD();
	static struct Server server;
	std::thread thread;
#endif

#include "Rotators.h"

// Remember to wear protective goggles :)

namespace sfall
{
	namespace script
	{
		// Calls vec.reserve(100) before work
		// Goes through all elevations (!) and tiles (!!!)
		void FillListVector(DWORD type, std::vector<fo::GameObject*>& vec);
	}
}

const char  rotatorsIni[] = ".\\ddraw.rotators.ini";
const char* currentTerrainStr;

// DisplayTerrainOnHotspotHover related variables
bool isMouseOverHotspot;
bool displayTerrainOnHotspot;
BYTE terrainOnHotspotTextColor;
BYTE terrainOnHotspotShadowColor;

// Any and all configuration should be read from ddraw.rotators.ini; /artifacts/ddraw.rotators.ini should be updated to reflect code, when possible;
// adds some extra work on PR/merge, but pays off in a long run
/*static*/ struct Ini {
	static std::string String(const char* section, const char* setting, const char* defaultValue) {
		return sfall::GetIniString(section, setting, defaultValue, 512, rotatorsIni);
	}

	static int Int(const char* section, const char* setting, int defaultValue) {
		return sfall::iniGetInt(section, setting, defaultValue, rotatorsIni);
	}

	static std::vector<std::string> List(const char* section, const char* setting, const char* defaultValue, char delim = ',') {
		return sfall::GetIniList(section, setting, defaultValue, 512, delim, rotatorsIni);
	}
};

/*** https://github.com/phobos2077/sfall/pull/273 ***/
static void InitLoadDll() {
	std::vector<std::string> names = Ini::List("Main", "LoadDll", "");

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


// FUNC(xenumfiles_, 0x4DFB3C)
// Function for enumerating files in filesystem and loaded DAT files based on a searchstring.
// Not sure if this is helpful or just a waste of time... this should be rewritten in c++.
// searchstring can be: "PROTO\\CRITTERS\\*.PRO" for example.
// eax = searchstring
// edx = buffer for storing the result
// xenum_files(char* searchstring, void* result);
static void _declspec(naked) _xenum_files()
{
	__asm {
		push ecx
		push esi
		push edi
		push ebp
		sub esp, 0x730
		mov dword ptr ss : [esp + 0x724] , eax // searchstring
		mov dword ptr ss : [esp + 0x728] , edx // result buffer
		mov esi, ebx
		test eax, eax
		jne search_not_null
		mov ecx, 0x34D
		mov ebx, 0x50fc40 // aXfile_c, FILE.C
		mov edx, 0x50fc9c // aFilespec 
		call fo::funcoffs::assert_
	search_not_null:
		cmp dword ptr ss : [esp + 0x728], 0
	jne result_buffer_not_null
		mov ecx, 0x34E
		mov ebx, 0x50FC40 // aXfile_c, FILE.C
		mov edx, 0x50FC88 // aEnumfunc
		xor eax, eax
		call fo::funcoffs::assert_
	result_buffer_not_null:
		lea eax, dword ptr ss : [esp + 0x51C]
		lea ecx, dword ptr ss : [esp + 0x41C]
		lea ebx, dword ptr ss : [esp + 0x61C]
		lea edx, dword ptr ss : [esp + 0x72C] // ?
		push eax
		mov eax, dword ptr ss : [esp + 0x728]
		mov dword ptr ss : [esp + 0x318], esi
		call fo::funcoffs::_splitpath_
		cmp byte ptr ss : [esp + 0x72C], 0
		jne find_first // if ? 
		mov dl, byte ptr ss : [esp + 0x61C]
		cmp dl, 0x5C // '\\'
		je find_first
		cmp dl, 0x2F // '/'
		je find_first
		cmp dl, 0x2E // '.'
		jne read_data_folder
	find_first:
		lea edx, dword ptr ss : [esp + 0x71C] // local path??
		mov eax, dword ptr ss : [esp + 0x724] // searchstring
		call fo::funcoffs::xsys_findfirst_
		test eax, eax
		je ret_close
		mov ebp, 0x50FCA8 // __11
		mov edi, 0x50FCAC // a__
		mov esi, 0x1E
	loop_compare_files:
		xor bh, bh
		mov eax, dword ptr ss : [esp + 0x720]
		mov byte ptr ss : [esp + 0x310] , bh
		test byte ptr ds : [eax + 0x15] , 10
		je make_path
		mov edx, ebp
		add eax, esi
		call fo::funcoffs::strcmp_
		test eax, eax
		je fs_find_next
		mov eax, dword ptr ss : [esp + 0x720]
		mov edx, edi
		add eax, esi
		call fo::funcoffs::strcmp_
		test eax, eax
		je fs_find_next
		mov byte ptr ss : [esp + 0x310] , 1
	make_path:
		push 0
		mov ecx, dword ptr ss : [esp + 0x724]
		lea ebx, dword ptr ss : [esp + 0x620]
		lea edx, dword ptr ss : [esp + 0x730]
		lea eax, dword ptr ss : [esp + 0x210]
		add ecx, esi
		call fo::funcoffs::_makepath_
		lea eax, dword ptr ss : [esp + 0x20C] // current file
		push eax
		call fo::funcoffs::xlistenumfunc_     // call xlistenumfunc_
		add esp, 4
		test eax, eax
		je ret_close
	fs_find_next:
		lea eax, dword ptr ss : [esp + 0x71C]
		call fo::funcoffs::xsys_findnext_
		test eax, eax
		jne loop_compare_files
		jmp ret_close
	read_data_folder:
		mov ebp, dword ptr ds : [FO_VAR_paths] // /data/
		test ebp, ebp
		je split_path
	process_path:
		test byte ptr ss : [ebp + 8], 1       // 1 = is dat
		je search_on_fs                       // jump if not
		mov ebx, dword ptr ss : [esp + 0x724] // search pattern
		mov edx, esp                          // file to look for
		mov eax, dword ptr ss : [ebp + 4]     // dat to look in
		call fo::funcoffs::dbase_findfirst_
		test eax, eax
		je db_close
		mov byte ptr ss : [esp + 0x310] , 2
	db_process_file:
		mov esi, esp
		lea edi, dword ptr ss : [esp + 0x20C]
		push edi
	db_process_file_loop:
		mov al, byte ptr ds : [esi]
		mov byte ptr ds : [edi] , al
		cmp al, 0
		je db_process_file_done
		mov al, byte ptr ds : [esi + 1]
		add esi, 2
		mov byte ptr ds : [edi + 1], al
		add edi, 2
		cmp al, 0
		jne db_process_file_loop
	db_process_file_done:
		pop edi
		lea eax, dword ptr ss : [esp + 0x20C]
		push eax
		// call dword ptr ss : [esp + 0x72C]
		call fo::funcoffs::xlistenumfunc_     // call xlistenumfunc_
		add esp, 4
		test eax, eax
		jne dat_next
		mov edx, esp
	db_close:
		mov eax, dword ptr ss : [ebp + 4]   // dat name
		call fo::funcoffs::dbase_findclose_ // close
		jmp ret_                            // return
	dat_next:
		mov edx, esp                       // current file
		mov eax, dword ptr ss : [ebp + 4]  // dat name
		call fo::funcoffs::dbase_findnext_
		test eax, eax                     
		jne db_process_file
		mov edx, esp
		mov eax, dword ptr ss : [ebp + 4]  // dat name
		call fo::funcoffs::dbase_findclose_
		jmp check_next_path
	search_on_fs:
		mov ecx, dword ptr ss : [esp + 0x724]
		push ecx
		mov esi, dword ptr ss : [ebp]
		push esi
		push 0x50fc64 // SS_14, no idea what this is.
		lea eax, dword ptr ss : [esp + 0x324]
		push eax
		call fo::funcoffs::sprintf_
		add esp, 0x10
		lea edx, dword ptr ss : [esp + 0x71C]
		lea eax, dword ptr ss : [esp + 0x318]
		call fo::funcoffs::xsys_findfirst_
		test eax, eax
		je fs_find_close
		mov edi, 0x50FCAC // a__, no idea what this is.
		mov esi, 0x1E
	fs_compare_files2:
		xor bh, bh
		mov eax, dword ptr ss : [esp + 0x720]
		mov byte ptr ss : [esp + 0x310] , bh
		test byte ptr ds : [eax + 0x15] , 0x10
		je fs_make_path
		mov edx, 0x50FCA8 // __11, no idea what this is.
		add eax, esi
		call fo::funcoffs::strcmp_
		test eax, eax
		je fs_findnext2
		mov eax, dword ptr ss : [esp + 0x720]
		mov edx, edi
		add eax, esi
		call fo::funcoffs::strcmp_
		test eax, eax
		je fs_find_close
		mov byte ptr ss : [esp + 0x310] , 1
	fs_make_path:
		push 0
		mov ecx, dword ptr ss : [esp + 0x724]
		lea ebx, dword ptr ss : [esp + 0x620]
		lea edx, dword ptr ss : [esp + 0x730]
		lea eax, dword ptr ss : [esp + 0x210]
		add ecx, esi
		call fo::funcoffs::_makepath_
		lea eax, dword ptr ss : [esp + 0x20C]
		push eax
		//call dword ptr ss : [esp + 0x72C]
		call fo::funcoffs::xlistenumfunc_     // call xlistenumfunc_
		add esp, 4
		test eax, eax
		je ret_close
	fs_findnext2:
		lea eax, dword ptr ss : [esp + 0x71C]
		call fo::funcoffs::xsys_findnext_
		test eax, eax
		jne fs_compare_files2
	fs_find_close:
		lea eax, dword ptr ss : [esp + 0x71C]
		call fo::funcoffs::xsys_findclose_
	check_next_path:
		mov ebp, dword ptr ss : [ebp + 0xC] // next path
		test ebp, ebp                       // if one exists
		jne process_path
	split_path:
		lea eax, dword ptr ss : [esp + 0x51C] // file extension
		lea ecx, dword ptr ss : [esp + 0x41C] // ?
		lea ebx, dword ptr ss : [esp + 0x61C] // directory
		push eax
		lea edx, dword ptr ss : [esp + 0x730] // ??
		mov eax, dword ptr ss : [esp + 0x728] // searchstring
		call fo::funcoffs::_splitpath_
		lea edx, dword ptr ss : [esp + 0x71C]
		mov eax, dword ptr ss : [esp + 0x724]
		call fo::funcoffs::xsys_findfirst_
		test eax, eax
		je ret_close
		mov ebp, 0x50FCA8 // __11
		mov edi, 0x50FCAC // a__
		mov esi, 0x1E
	fs_compare_files:
		xor dl, dl
		mov eax, dword ptr ss : [esp + 0x720]
		mov byte ptr ss : [esp + 0x310] , dl
		test byte ptr ds : [eax + 0x15] , 10
		je make_path
		mov edx, ebp
		add eax, esi
		call fo::funcoffs::strcmp_
		test eax, eax
		je fs_next_file
		mov eax, dword ptr ss : [esp + 0x720]
		mov edx, edi
		add eax, esi
		call fo::funcoffs::strcmp_
		test eax, eax
		je fs_next_file
		mov byte ptr ss : [esp + 0x310] , 1
		push 0
		mov ecx, dword ptr ss : [esp + 0x724]
		lea ebx, dword ptr ss : [esp + 0x620]
		lea edx, dword ptr ss : [esp + 0x730]
		lea eax, dword ptr ss : [esp + 0x210]
		add ecx, esi
		call fo::funcoffs::_makepath_
		lea eax, dword ptr ss : [esp + 0x20C]
		push eax
		//call dword ptr ss : [esp + 0x72C]
		call fo::funcoffs::xlistenumfunc_     // call xlistenumfunc_
		add esp, 4
		test eax, eax
		je ret_close
	fs_next_file:
		lea eax, dword ptr ss : [esp + 0x71C]
		call fo::funcoffs::xsys_findnext_
		test eax, eax
		jne fs_compare_files
	ret_close:
		lea eax, dword ptr ss : [esp + 0x71C]
		call fo::funcoffs::xsys_findclose_
	ret_:
		add esp, 0x730
		pop ebp
		pop edi
		pop esi
		pop ecx
		ret
	}
}

static void* db_readfile(char* filename, int len)
{
	auto buffer = malloc(len);
	__asm {
		mov eax, filename
		mov edx, buffer
		call fo::funcoffs::db_read_to_buf_
	}
	return buffer;
}

static int db_filelen(int dbFile) {
	int len=0;
	__asm {
		mov eax, dbFile
		call fo::funcoffs::xfilelength_
		mov len, eax
	}
	return len;
}

static void* xenum_files(char* searchstring)
{
	void* result = malloc(12);
	__asm {
		mov eax, searchstring
		mov ecx, result
		mov ebx, result
		mov ebp, result
		mov edx, 0x4E0278 // xlistenumfunc
		call _xenum_files
	}
	return result;
}


extern void* LoadGameHookFuncAddress;
int LoadScreenInit = 0x480AF5;
static void MainHook() { }
int LoadSlot;
static __declspec(naked) void LoadScreenInitHook() {
	LoadSlot = Ini::Int("Debugging", "AutoLoadSlot", -1);
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



char* xfopenLoadFile;
char* xfopenLoadedFrom;
char* xf1;
char* xf2;
int _rewind = 0x004F1411;
std::map<char*, char*> xfopened;

bool alreadyHasKey = false;

void hasKey(char* key)
{
	alreadyHasKey = false;
	for (auto const& x : xfopened)
	{
		if (strcmp(x.first, key) == 0) {
			alreadyHasKey = true;
			return;
		}
	}
	return;
}

static __declspec(naked) void xfopen_hook() {
	__asm {
		pushad
		mov xfopenLoadFile, edi
		mov eax, dword ptr ss : [ecx]
		mov xfopenLoadedFrom, eax

	}
	hasKey(xfopenLoadFile);
	if (!alreadyHasKey) {
		xf1 = (char*)malloc(200);
		xf2 = (char*)malloc(200);
		memmove(xf1, xfopenLoadFile, 200);
		memmove(xf2, xfopenLoadedFrom, 200);
		xfopenLoadFile = "";
		xfopenLoadedFrom = "";
		xfopened[xf1] = xf2;
	}
	__asm {
		popad
		jmp _rewind
	}
}

int mapIdToLoad = 0;
static void loadmap() {
	__asm {
		pushad
		mov eax, [mapIdToLoad]
		call fo::funcoffs::map_load_idx_
		popad
	}
}

static void OnMainLoop() {
	if (mapIdToLoad != 0) {
		loadmap();
		mapIdToLoad = 0;
	}
}

// https://i.imgur.com/0bu4J2l.png
static void InitTerrainHover()
{
	displayTerrainOnHotspot     = Ini::Int("Interface", "DisplayTerrainOnHotspotHover", 0);
	terrainOnHotspotTextColor   = Ini::Int("Interface", "TerrainOnHotspotTextColor", 215);
	terrainOnHotspotShadowColor = Ini::Int("Interface", "TerrainOnHotspotTextShadowColor", 228);
	if(displayTerrainOnHotspot)
		sfall::MakeJump(0x4BFE84, wmDetectHotspotHover);
	currentTerrainStr = "";
}

void sfall::Rotators::init()
{
	SafeWrite8(0x410003,0xF4);
	InitLoadDll();
	InitTerrainHover();
	xfopenLoadFile = (char*)malloc(200);
	xfopenLoadedFrom = (char*)malloc(200);
	sfall::MakeCall(0x4DEFCD, xfopen_hook);
	sfall::MainLoopHook::OnMainLoop() += OnMainLoop;
	//sfall::MakeCall(0x4DFF33, _xenum_files);
	//sfall::MakeJump(0x480A23, LoadScreenInitHook);
	#ifdef HTTPD_SERVER
	thread = std::thread(InitHTTPD);
	#endif
}

void sfall::Rotators::exit()
{
	#ifdef HTTPD_SERVER
	serverDeInit(&server);
	thread.join();
	#endif
}

static void* db_fastread(char* filename)
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

// Move this somewhere else, when/if it grows too much
#ifdef HTTPD_SERVER
std::string DocumentRoot;

void InitHTTPD()
{
	DocumentRoot = Ini::String("HTTP", "DocumentRoot", ".");
	serverInit(&server);
	struct sockaddr_in localhost = { 0 };
	localhost.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	localhost.sin_family = AF_INET;
	localhost.sin_port = htons(1207);

	acceptConnectionsUntilStopped(&server, (struct sockaddr*) & localhost, sizeof(localhost));
}

bool DoesFileExist(const char* filename) {
	struct stat st;
	int result = stat(filename, &st);
	return result == 0;
}

char* HTMLStart = R"HTML_S(<!DOCTYPE html>
	<!DOCTYPE html>
	<html>
	<head>
		<title>sfall UI</title>
		<link rel="stylesheet" href="/style.css" type="text/css" />
	</head>
	<body>)HTML_S";

char* HTMLEnd = R"HTML_S(</body></html>)HTML_S";

Response* HTMLDisplayDBFiles(char* pattern) {
	char** filenames;
	int count = fo::func::db_get_file_list(pattern, &filenames);
	std::string response = std::string(HTMLStart);
	for (auto i = 0; i < count; i++) {
		response += filenames[i];
		response += "<br/>\n";
	}
	fo::func::db_free_file_list(&filenames, 0);
	response += HTMLEnd;
	return responseAllocHTML(response.c_str());
}

Response* HTMLDisplayMaps() {
	auto mapBase = fo::var::wmMapInfoList;
	std::string response = std::string(HTMLStart);
	for (int i = 0; i < fo::var::wmMaxMapNum; i++) {
		char* name = reinterpret_cast<char*>(mapBase + (i * 0x248) + 0x30); // 0x4BFA05 & 4BFA18
		char buf[128];
		sprintf(buf, "<a href='/loadmap/%d'>%s.map</a><br/>\n", i, name);
		response += buf;
	}
	response += HTMLEnd;
	response += HTMLEnd;
	return responseAllocHTML(response.c_str());
}

char* respString;
struct Response* createResponseForRequest(const struct Request* request, struct Connection* connection) {
	// RESERVED
	// /db/*

	if (0 == strcmp(request->pathDecoded, "/")
		|| 0 == strcmp(request->pathDecoded, "/style.css")
		|| 0 == strcmp(request->pathDecoded, "/script.js")) {
		return responseAllocServeFileFromRequestPath("/", request->path, request->pathDecoded, DocumentRoot.c_str());
	}

	if (0 == strncmp(request->pathDecoded, "/loadmap/", 9)) {
		std::vector<std::string> spl = sfall::split(std::string(request->pathDecoded), '/');
		char* cstr = (char*)malloc(32);
		int id = strtol(spl[2].c_str(), NULL, 10);
		mapIdToLoad = id;
		return HTMLDisplayMaps();
	}
	if (0 == strcmp(request->pathDecoded, "/files/scripts")) {	return HTMLDisplayDBFiles("scripts\\*.int"); }
	if (0 == strcmp(request->pathDecoded, "/files/maps")) {
		return HTMLDisplayMaps();
	}

	if (0 == strcmp(request->pathDecoded, "/files/xfopen")) {
		std::string response = std::string(HTMLStart);
		response += "<p>These are all the files loaded via the xfopen function.</p>";
		response += "<table style='width: 400px;'><thead><tr><th>File</th><th>Source</th></thead><tbody>";
		for (auto const& x : xfopened)
		{
			response += "<tr>";
			response += "<td>";
			response += x.first;
			response += "</td>";
			response += "<td>";
			response += x.second;
			response += "</td>";
			response += "</tr>\n";
		}
		response += "</tbody></table>";
		response += HTMLEnd;
		return responseAllocHTML(response.c_str());
	}

	if (0 == strcmp(request->pathDecoded, "/dump/game-objects")) {
		std::vector<fo::GameObject*> vec;
		sfall::script::FillListVector(static_cast<DWORD>(FLV::ALL), vec);

		std::string response = std::string(HTMLStart);
		for (const auto& obj : vec) {
			response += " id=" + std::to_string(obj->id);
			response += " protoId=" + std::to_string(obj->protoId);

			response += " scriptId=" + std::to_string(obj->scriptId) + "=";
			fo::ScriptInstance* script;
			if (fo::func::scr_ptr(obj->scriptId, &script) != -1 && script->program != nullptr) {
				response += std::string(script->program->fileName);

				if (fo::func::db_access(script->program->fileName)) {
					//static constexpr long DB_SEEK_SET = 0;
					//static constexpr long DB_SEEK_CURR = 1;
					//static constexpr long DB_SEEK_END = 2;
					fo::DbFile* intFile = fo::func::db_fopen(script->program->fileName, "r");
					long intSize = db_filelen((int)intFile);
					fo::func::db_fclose(intFile);
					//fo::func::db_fseek(intFile, 0, DB_SEEK_SET);
					//fo::func::db_fclose(intFile);

					response += "=" + std::to_string(intSize) + "b";
				}
				else {
					response += "=?b";
				}
			}
			else
				response += "nullptr";

			response += " x=" + std::to_string(obj->x);
			response += " y=" + std::to_string(obj->y);
			response += " sx=" + std::to_string(obj->sx);
			response += " sy=" + std::to_string(obj->sy);
			response += " rotation=" + std::to_string(obj->rotation);
			response += " frm=" + std::to_string(obj->frm);
			response += " artFid=" + std::to_string(obj->artFid);

			response += "<br/>\n";
		}
		response += HTMLEnd;

		return responseAllocHTML(response.c_str());
	}

	/* Serve files from the current directory */
	if (request->pathDecoded == strstr(request->pathDecoded, "/files")) {
		return responseAllocServeFileFromRequestPath("/files", request->path, request->pathDecoded, ".");
	}
	return responseAlloc404NotFoundHTML("You don't see anything out of the ordinary.");
}
#endif
