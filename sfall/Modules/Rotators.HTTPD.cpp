#include <cstdint>

#include "Rotators.HTTPD.h"
#include "HTTP\HTML.h"
#include "HTTP\Routing.h"

// Used by other submodules to check if HTTPD is currently enabled, so it must be declared for v140_xp build
uint16_t sfall::HTTPD::Port = 0;

#if _MSC_VER >= 1920

#include <string>
#include <thread>
#include <map>

#include <fstream>

#include "..\main.h"
#include "..\SafeWrite.h"
#include "..\Utils.h"
#include "..\FalloutEngine\Fallout2.h"

#include "MainLoopHook.h"
#include "WorldMap.h"

#include "..\Lib\EmbeddableWebServer.h"

#include "Rotators.h"

using namespace rfall;

static struct Server server;

std::string DocumentRoot;

int mapIdToLoad = 0;

char* xfopenLoadFile;
char* xfopenLoadedFrom;
char* xf1;
char* xf2;
int _rewind = 0x004F1411;
std::map<char*, char*> xfopened;
bool alreadyHasKey = false;

//

char* HTMLStart = R"HTML_S(
	<!DOCTYPE html>
	<html>
	<head>
		<title>sfall UI</title>
		<link rel="stylesheet" href="/style.css" type="text/css" />
	</head>)HTML_S";

char* HTMLEnd = R"HTML_S(
</html>)HTML_S";

static void loadmap() {
	__asm {
		pushad
		mov eax, [mapIdToLoad]
		call fo::funcoffs::map_load_idx_
		popad
	}
}

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




// Thread safe way to manipulate the game
// avoids trashing the game thread from the HTTPD thread
static void OnMainLoop() {
	if (mapIdToLoad != 0) {
		loadmap();
		mapIdToLoad = 0;
	}
}

/*static void ProcessCommonCommands() {

}*/

/*static void OnWMLoop() {
	ProcessCommonCommands();
}

static void OnCombatLoop() {
	ProcessCommonCommands();
}*/

enum MapsEntryFlags : BYTE {
	IsSavable= 0x1,
	DeadBodiesAge = 0x2,
};

// Maps.txt info
// 0x4BFA05 & 4BFA18
// 0x4BFA81 (how flags are handled)
struct WMMapInfo {
	char lookupName[0x30];
	char mapName[0x28];
	char music[0x28];
	MapsEntryFlags mapsFlags;
	char pad[(0x248 - 0x28 - 0x28 - 0x30 - 0x01)]; // rest is unclear for now.
};

bool DoesFileExist(const char* filename) {
	struct stat st;
	int result = stat(filename, &st);
	return result == 0;
}

Response* HTMLDisplayDBFiles(char* pattern) {
	char** filenames;

	int count = fo::func::db_get_file_list(pattern, &filenames);
	std::string response = std::string(HTMLStart);

	for (auto i = 0; i < count; i++) {
		response += std::string(filenames[i]);
		response += "<br/>\n";
	}

	fo::func::db_free_file_list(&filenames, 0);
	response += HTMLEnd;
	return responseAllocHTML(response.c_str());
}

HTMLElement* HTMLDisplayMaps() {
	auto div = HTMLElement::div();
	auto mapsInfo = *(WMMapInfo(*)[])((*(DWORD*)(FO_VAR_wmMapInfoList)));
	std::vector<WMMapInfo> maps;

	for (int i = 0; i < fo::var::wmMaxMapNum; i++) {
		maps.push_back(mapsInfo[i]);
	}
	std::string response = std::string(HTMLStart);
	div->add(HTMLElement::p("These are all from Maps.txt"));
	auto table = new HTMLTable();
	table->width = 400;
	table->headers = { "Id", "Name", "Music", "Teleport" };
	auto tbody = table->createBody();
	int idx = 0;
	for (auto& map : maps) {
		auto href = std::string("/loadmap/");
		href += std::to_string(idx);
		auto tr = HTMLElement::tr();
		tr->add(HTMLElement::td(std::to_string(idx++)));
		tr->add(HTMLElement::td(map.mapName));
		tr->add(HTMLElement::td(map.music));
		tr->add(HTMLElement::td()->add(HTMLUtils::URL(href, "Teleport")));
		tbody->add(tr);
	}

	div->add(table->get());
	return div;
}




std::string RenderBody(HTMLElement* body) {
	std::string response = std::string(HTMLStart);
	response += body->render();
	response += HTMLEnd;
	delete body;
	return response;
}

Router* router;

#define IS_URL(__path) 0 == strcmp(request->pathDecoded, __path)
#define DOC_ROOT responseAllocServeFileFromRequestPath("/", request->path, request->pathDecoded, DocumentRoot.c_str())
#define DOC_INDEX responseAllocServeFileFromRequestPath("/", "/index.html", "/index.html", DocumentRoot.c_str())
void InitRoutes() {
	router = new Router();
}

struct Response* createResponseForRequest(const struct Request* request, struct Connection* connection) {
	// RESERVED
	// /db/*
	auto r = router;
	auto body = HTMLElement::body();
	r->setContext((char*)request->pathDecoded, body);
	
	#define is(__a) r->exactly(__a)

	if (is("/") || is("/style.css") || is("/script.js")) {
		return DOC_ROOT;
	}

	if (is("/files/scripts"))
		return HTMLDisplayDBFiles("scripts\\*.int");

	if (is("/files/maps")) {
		body->add(HTMLDisplayMaps());
	}

	if (is("/cheats/give-xp")) {
		fo::func::stat_pc_add_experience(10000);
		return DOC_INDEX;
	}

	if (is("/files/xfopen")) {
		body->add(HTMLElement::p("These are all the files loaded via the xfopen function."));
		std::vector<char*> keys;
		for (auto& kvp : xfopened)
			keys.push_back(kvp.first);

		auto table = new HTMLTable();
		table->width = 400;
		table->headers = { "File", "Source" };
		auto tbody = table->createBody();
		for (auto& kvp : xfopened) {
			auto tr = HTMLElement::tr();
			tr->add(HTMLElement::td(kvp.first));
			tr->add(HTMLElement::td(kvp.second));
			tbody->add(tr);
		};
		body->add(table->get());
	}

	typedef std::map<std::string, std::string> urlVars;
	#define _ [] (HTMLElement* body, urlVars vars) -> void

	r->on("/loadmap/{id:u8}", _ {
		mapIdToLoad = std::stoi(vars["id"]);
		body->add(HTMLDisplayMaps());
	});

	r->on("/dump/game-objects/?t={type:u8}", _ {
		std::vector<fo::GameObject*> vec;
		rfall::misc::FLV type = (rfall::misc::FLV)std::stoi(vars["type"]);
		misc::FillListVector(type, vec);

		std::string response = std::string(HTMLStart);

		auto table = new HTMLTable();
		auto tbody = table->createBody();
		table->width = 1200;
		table->headers = { "Id", "protoId", "scriptId", "scriptData", "x", "y", "sx", "sy", "rotation", "frm", "artFid", "artName" };
		
		for (auto& obj : vec) {
			auto tr = HTMLElement::tr();
			tr->add(HTMLElement::td(std::to_string(obj->id)));
			tr->add(HTMLElement::td(std::to_string(obj->protoId)));
			tr->add(HTMLElement::td(std::to_string(obj->scriptId)));
			auto scriptData = std::string();
			fo::ScriptInstance* script;
			if (fo::func::scr_ptr(obj->scriptId, &script) != -1 && script->program != nullptr) {
				scriptData += std::string(script->program->fileName);

				if (fo::func::db_access(script->program->fileName)) {
					fo::DbFile* intFile = fo::func::db_fopen(script->program->fileName, "r");
					long intSize = db::filelen(intFile);
					fo::func::db_fclose(intFile);

					scriptData += "=" + std::to_string(intSize) + "b";
				}
				else {
					scriptData += "=?b";
				}
			}
			else
				scriptData += "nullptr";
			tr->add(HTMLElement::td(scriptData));
			tr->add(HTMLElement::td(std::to_string(obj->x)));
			tr->add(HTMLElement::td(std::to_string(obj->y)));
			tr->add(HTMLElement::td(std::to_string(obj->sx)));
			tr->add(HTMLElement::td(std::to_string(obj->sy)));
			tr->add(HTMLElement::td(std::to_string(obj->rotation)));
			tr->add(HTMLElement::td(std::to_string(obj->frm)));
			tr->add(HTMLElement::td(std::to_string(obj->artFid)));
			tr->add(HTMLElement::td(std::string(fo::func::art_get_name(obj->artFid))));
			tbody->add(tr);
		};
		body->add(HTMLUtils::URL("?t=0", type == rfall::misc::FLV::CRITTERS ? "[Critters]" : "Critters"));
		body->add(HTMLUtils::URL("?t=1", type == rfall::misc::FLV::GROUNDITEMS ? "[Items]" : "Items"));
		body->add(HTMLUtils::URL("?t=2", type == rfall::misc::FLV::SCENERY ? "[Scenery]" : "Scenery"));
		body->add(HTMLUtils::URL("?t=3", type == rfall::misc::FLV::WALLS ? "[Walls]" : "Walls"));
		body->add(HTMLUtils::URL("?t=4", type == rfall::misc::FLV::TILES ? "[Tiles]" : "Tiles"));
		body->add(HTMLUtils::URL("?t=5", type == rfall::misc::FLV::MISC ? "[Misc]" : "Misc"));
		body->add(HTMLUtils::URL("?t=6", type == rfall::misc::FLV::SPATIAL ? "[Spatial]" : "Spatial"));
		body->add(HTMLUtils::URL("?t=9", type == rfall::misc::FLV::ALL ? "[All]" : "All"));
		body->add(new HTMLElement(HTMLTag::br));
		body->add(table->get());
	});

	#undef _
	#undef is

	if (body->children.size() != 0) {
		return responseAllocHTML(RenderBody(body).c_str());
	}

	return responseAlloc404NotFoundHTML("You don't see anything out of the ordinary.");
}

// Module

static void Run() {
	std::ofstream log;
	log.open("rfall-httpd.txt", std::ios_base::out | std::ios_base::trunc);

	log << "> config\n";
	log.flush();
	DocumentRoot = Ini::String("HTTPD", "DocumentRoot", ".");

	log << "> serverInit\n";
	log.flush();
	serverInit(&server);

	log << "> localhost\n";
	log.flush();
	struct sockaddr_in localhost = { 0 };
	localhost.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	localhost.sin_family = AF_INET;
	localhost.sin_port = htons(sfall::HTTPD::Port); // maybe pass it as argument?

	log << "> malloc\n";
	log.flush();
	xfopenLoadFile   = (char*)malloc(200);
	xfopenLoadedFrom = (char*)malloc(200);
	log << "> xfopen_hook\n";
	log.flush();
	sfall::MakeCall(0x4DEFCD, xfopen_hook);
	log << "> OnMainLoop\n";
	log.flush();
	sfall::MainLoopHook::OnMainLoop() += OnMainLoop;
	log << "> InitRoutes()\n";
	log.flush();
	InitRoutes();

	// Switching map in Combat crashes the game and from WM it doesn't work, 
	// likely needs the use the "enter map from wm" function instead.
	//sfall::MainLoopHook::OnCombatLoop() += OnCombatLoop;
	//sfall::Worldmap::OnWorldmapLoop() += OnWMLoop;

	log << "> acceptConnectionsUntilStopped()\n";
	log.flush();
	acceptConnectionsUntilStopped(&server, (struct sockaddr*) & localhost, sizeof(localhost));
}

void sfall::HTTPD::init() {
	// Cached in case value changes after init
	Port = Ini::Int( "HTTPD", "Port", 0);
	if (Port) {
		static const std::vector<uint16_t> banned = { 2049, 4045, 6000 };

		if (Port <= 1024 || std::count(banned.begin(), banned.end(), Port))
			misc::CriticalFail("[HTTPD]->Port " + std::to_string(Port) + " invalid");

		dlog_f( "> starting on port %u\n", DL_INIT, Port);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Run, 0, 0, NULL);
		// https://github.com/rotators/sfall/issues/2
	}
}

void sfall::HTTPD::exit() {
	if (Port) {
		serverDeInit(&server);
	}
}

#endif // _MSC_VER >= 1920 //
