#include <cstdint>
#include <string>
#include <thread>
#include <map>

#include "..\main.h"
#include "..\SafeWrite.h"
#include "..\Utils.h"
#include "..\FalloutEngine\Fallout2.h"

#include "MainLoopHook.h"
#include "WorldMap.h"

#include "..\Lib\EmbeddableWebServer.h"

#include "Rotators.h"
#include "Rotators.HTTPD.h"
#include "HTTP\HTML.h"
#include "HTTP\Routing.h"

using namespace rfall;

// Used by other submodules to check if HTTPD is currently enabled
uint16_t HTTPD::Port = 0;

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
		mov esi, xfopenLoadFile
		mov xfopenLoadFile, edi
		cmp ecx, 0
		jne open // ptr is valid
		mov xfopenLoadFile, esi
		jmp end
	open:
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
	end:
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

#define h(__type) HTMLElement::__type()
#define h(__type, __arg1) HTMLElement::__type(__arg1)
#define _s(__arg) std::to_string(__arg)
#define _url(href, text) HTMLUtils::URL(href, text)

void HTMLDisplayDBFiles(char* pattern, HTMLElement* parent) {
	char** filenames;

	int count = fo::func::db_get_file_list(pattern, &filenames);

	for (auto i = 0; i < count; i++) {
		parent->_(h(span, std::string(filenames[i])));
		parent->_(h(br));
	}
	fo::func::db_free_file_list(&filenames, 0);
}

HTMLElement* HTMLDisplayMaps() {
	auto div = HTMLElement::div();
	auto mapsInfo = *(WMMapInfo(*)[])((*(DWORD*)(FO_VAR_wmMapInfoList)));
	std::vector<WMMapInfo> maps;

	for (int i = 0; i < fo::var::wmMaxMapNum; i++) {
		maps.push_back(mapsInfo[i]);
	}
	div->add(h(p, "These are all from Maps.txt"));
	auto table = new HTMLTable();
	table->width = 400;
	table->headers = { "Id", "Name", "Music", "Teleport" };
	auto tbody = table->createBody();
	int idx = 0;
	for (auto& map : maps) {
		auto href = std::string("/loadmap/");
		href += _s(idx);
		auto tr = h(tr);
		tr->_(h(td, _s(idx++)));
		tr->_(h(td, map.mapName));
		tr->_(h(td, map.music));
		tr->_(h(td)->_(HTMLUtils::URL(href, "Teleport")));
		tbody->_(tr);
	}

	div->_(table->get());
	return div;
}

Router* router;
#define DOC_ROOT responseAllocServeFileFromRequestPath("/", request->path, request->pathDecoded, DocumentRoot.c_str())
#define DOC_INDEX renderIndex(body); html->_(body); title->text = "sfall"; return renderHtml(html);

Response* renderHtml(HTMLElement* h) {
	auto r = responseAllocHTML(h->render().c_str());
	delete h;
	return r;
}

void renderIndex(HTMLElement* body) {
	body->_(h(h1, "Database files"));
	auto ul = h(ul);
	ul->_(h(li)->_(_url("/files/scripts", "Loaded scripts")))
	  ->_(h(li)->_(_url("/files/maps", "Loaded maps")))
	  ->_(h(li)->_(_url("/files/xfopen", "XFOpened files")));
	body->_(ul);
	body->_(h(h1, "Game state"));
	ul = h(ul);
	ul->_(h(li)->_(_url("/dump/game-objects/?t=0", "Game objects")));
	body->_(ul);
	body->_(h(h1, "Cheats"));
	ul = h(ul);
	ul->_(h(li)->_(_url("/cheats/give-xp", "Gain 10 000 XP")));
	body->_(ul);
}

struct Response* createResponseForRequest(const struct Request* request, struct Connection* connection) {
	// RESERVED
	// /db/*
	auto html = new HTMLElement(HTMLTag::html);
	auto head = HTMLElement::head();
	auto title = HTMLElement::title("sfall UI");
	head->add(title);
	head->add(HTMLUtils::CSS("/style.css"));
	html->add(head);

	auto r = router;
	auto body = HTMLElement::body();
	r->setContext((char*)request->pathDecoded, body, title);
	
	#define is(__a) r->exactly(__a)

	if (is("/")) {
		DOC_INDEX;
	}

	if (is("/style.css") || is("/script.js")) {
		return DOC_ROOT;
	}

	if (is("/files/scripts")) {
		HTMLDisplayDBFiles("scripts\\*.int", body);
		title->text = "sfall - loaded scripts";
	}

	if (is("/files/maps")) {
		body->add(HTMLDisplayMaps());
		title->text = "sfall - loaded maps";
	}

	if (is("/cheats/give-xp")) {
		fo::func::stat_pc_add_experience(10000);
		DOC_INDEX;
	}

	if (is("/files/xfopen")) {
		title->text = "sfall - xfopened files";
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
			tr->_(h(td, kvp.first));
			tr->_(h(td, kvp.second));
			tbody->add(tr);
		};
		body->add(table->get());
	}

	typedef std::map<std::string, std::string> urlVars;
	#define __ [] (HTMLElement* body, HTMLElement* title, urlVars vars) -> void

	r->on("/loadmap/{id:u8}", __ {
		mapIdToLoad = std::stoi(vars["id"]);
		body->add(HTMLDisplayMaps());
	});

	r->on("/dump/game-objects/?t={type:u8}", __ {
		title->text = "sfall - game objects";
		std::vector<fo::GameObject*> vec;
		rfall::misc::FLV type = (rfall::misc::FLV)std::stoi(vars["type"]);
		misc::FillListVector(type, vec);

		auto table = new HTMLTable();
		auto tbody = table->createBody();
		table->width = 1200;
		table->headers = { "Id", "protoId", "scriptId", "scriptData", "tile", "x", "y", "sx", "sy", "rotation", "frm", "artFid", "artName" };
		
		for (auto& obj : vec) {
			auto tr = HTMLElement::tr();
			tr->_(h(td, _s(obj->id)));
			tr->_(h(td, _s(obj->protoId)));
			tr->_(h(td, _s(obj->scriptId)));
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
			tr->_(h(td, scriptData))
			  ->_(h(td, _s(obj->tile)))
			  ->_(h(td, _s(obj->x)))
			  ->_(h(td, _s(obj->y)))
			  ->_(h(td, _s(obj->sx)))
			  ->_(h(td, _s(obj->sy)))
			  ->_(h(td, _s(obj->rotation)))
			  ->_(h(td, _s(obj->frm)))
			  ->_(h(td, _s(obj->artFid)))
			  ->_(h(td, std::string(fo::func::art_get_name(obj->artFid))));
			tbody->_(tr);
		};
		body->_(_url("?t=0", type == rfall::misc::FLV::CRITTERS ? "[Critters]" : "Critters"));
		body->_(_url("?t=1", type == rfall::misc::FLV::GROUNDITEMS ? "[Items]" : "Items"));
		body->_(_url("?t=2", type == rfall::misc::FLV::SCENERY ? "[Scenery]" : "Scenery"));
		body->_(_url("?t=3", type == rfall::misc::FLV::WALLS ? "[Walls]" : "Walls"));
		body->_(_url("?t=4", type == rfall::misc::FLV::TILES ? "[Tiles]" : "Tiles"));
		body->_(_url("?t=5", type == rfall::misc::FLV::MISC ? "[Misc]" : "Misc"));
		body->_(_url("?t=6", type == rfall::misc::FLV::SPATIAL ? "[Spatial]" : "Spatial"));
		body->_(_url("?t=9", type == rfall::misc::FLV::ALL ? "[All]" : "All"));
		body->_(h(br));
		body->_(table->get());
	});

	#undef h
	#undef _
	#undef is

	if (body->children.size() != 0) {
		html->add(body);
		return renderHtml(html);
	}
	delete html;
	return responseAlloc404NotFoundHTML("You don't see anything out of the ordinary.");
}

// Module

static void Run() {
	DocumentRoot = ini.GetStr("HTTPD", "DocumentRoot", ".");

	serverInit(&server);

	struct sockaddr_in localhost = { 0 };
	localhost.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	localhost.sin_family = AF_INET;
	localhost.sin_port = htons(HTTPD::Port); // maybe pass it as argument?

	xfopenLoadFile   = (char*)malloc(200);
	xfopenLoadedFrom = (char*)malloc(200);
	sfall::MakeCall(0x4DEFCD, xfopen_hook);
	sfall::MainLoopHook::OnMainLoop() += OnMainLoop;
	router = new Router();

	// Switching map in Combat crashes the game and from WM it doesn't work, 
	// likely needs the use the "enter map from wm" function instead.
	//sfall::MainLoopHook::OnCombatLoop() += OnCombatLoop;
	//sfall::Worldmap::OnWorldmapLoop() += OnWMLoop;

	acceptConnectionsUntilStopped(&server, (struct sockaddr*) & localhost, sizeof(localhost));
}

void HTTPD::init() {
	// Cached in case value changes after init
	Port = ini.GetInt("HTTPD", "Port", 0);
	if (Port) {
		static const std::vector<uint16_t> banned = { 2049, 4045, 6000 };

		if (Port <= 1024 || std::count(banned.begin(), banned.end(), Port))
			misc::CriticalFail("[HTTPD]->Port " + std::to_string(Port) + " invalid");

		sfall::dlog_f( "> starting on port %u\n", DL_INIT, Port);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Run, 0, 0, NULL);
		// https://github.com/rotators/sfall/issues/2
	}
}

void HTTPD::exit() {
	if (Port) {
		serverDeInit(&server);
	}
}

//#endif // _MSC_VER >= 1920 //
