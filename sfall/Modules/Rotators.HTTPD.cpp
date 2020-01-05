#include <cstdint>
#include <string>
#include <thread>
#include <map>

#include "..\FalloutEngine\Fallout2.h"
#include "..\SafeWrite.h"
#include "..\Utils.h"

#include "MainLoopHook.h"

#include "..\Lib\EmbeddableWebServer.h"

#include "Rotators.h"
#include "Rotators.HTTPD.h"

using namespace rfall;

static struct Server server;
std::thread thread;
uint16_t sfall::HTTPD::Port = 0;

std::string DocumentRoot;

int mapIdToLoad = 0;

char* xfopenLoadFile;
char* xfopenLoadedFrom;
char* xf1;
char* xf2;
int _rewind = 0x004F1411;
std::map<char*, char*> xfopened;
bool alreadyHasKey = false;

// TODO reimplement in Rotators.cpp
namespace sfall
{
	namespace script
	{
		// Calls vec.reserve(100) before work
		// Goes through all elevations (!) and tiles (!!!)
		void FillListVector(DWORD type, std::vector<fo::GameObject*>& vec);
	}
}

//

char* HTMLStart = R"HTML_S(<!DOCTYPE html>
	<!DOCTYPE html>
	<html>
	<head>
		<title>sfall UI</title>
		<link rel="stylesheet" href="/style.css" type="text/css" />
	</head>
	<body>)HTML_S";

char* HTMLEnd = R"HTML_S(</body></html>)HTML_S";

//


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

//

static void OnMainLoop() {
	if (mapIdToLoad != 0) {
		loadmap();
		mapIdToLoad = 0;
	}
}

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

template<typename T>
class HTMLTable {
	public:
		int width=0;
		std::vector<T> rowObjects;
		std::vector<std::string> headers;
		std::function<std::string(T, int)> bodyFunc;

	std::string renderHeader() {
		std::string buf("");
		buf += "<thead><tr>";
		for (auto const& h : headers)
		{
			buf += "<th>";
			buf += h;
			buf += "</th>";
		}
		buf += "</tr></thead>";
		return buf;
	}

	static std::string renderCell(std::string val) {
		std::string buf("");
		buf += "<td>";
		buf += val;
		buf += "</td>";
		return buf;
	}

	std::string renderBody() {
		std::string buf("");
		buf += "<tbody>";
		int i = 0;
		for (auto const& obj : rowObjects) {
			buf += this->bodyFunc(obj, i);
			i++;
		}
		buf += "</tbody>";
		return buf;
	}

	std::string render() {
		std::string buf("");
		// TODO: handle attributes better.
		if (width != 0) {
			buf += "<table style='width: ";
			buf += std::to_string(width);
			buf += "px'>";
		}
		else {
			buf += "<table>";
		}
		buf += this->renderHeader();
		buf += this->renderBody();
		buf += "</table>";
		return buf;
	}
};

class HTMLUtils {

	public:

	static std::string URL(std::string href, std::string text) {
		std::string buf("<a href='");
		buf += href;
		buf += "'>";
		buf += text;
		buf += "</a>";
		return buf;
	}

};

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

Response* HTMLDisplayMaps() {
	auto mapsInfo = *(WMMapInfo(*)[])((*(DWORD*)(FO_VAR_wmMapInfoList)));
	std::vector<WMMapInfo> maps;
	
	for (int i = 0; i < fo::var::wmMaxMapNum; i++) {
		maps.push_back(mapsInfo[i]);
	}
	std::string response = std::string(HTMLStart);
	response += "<p>These are all from Maps.txt</p>";
	auto table = new HTMLTable<WMMapInfo>();
	table->width = 400;
	table->rowObjects = maps;
	table->headers =  { "Id", "Name", "Music", "Teleport" };
	table->bodyFunc = [](WMMapInfo map, int idx) -> std::string {
		auto tr = std::string("");
		auto url = std::string("/loadmap/");
		url += std::to_string(idx);
		tr += "<tr>";
		tr += HTMLTable<WMMapInfo>::renderCell(std::to_string(idx));
		tr += HTMLTable<WMMapInfo>::renderCell(map.mapName);
		tr += HTMLTable<WMMapInfo>::renderCell(map.music);
		tr += HTMLTable<WMMapInfo>::renderCell(HTMLUtils::URL(url, "Teleport"));
		tr += "</tr>\n";
		return tr;
	};
	response += table->render();
	response += HTMLEnd;
	return responseAllocHTML(response.c_str());
}



// needs better url handling ASAP
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

	if (0 == strcmp(request->pathDecoded, "/files/scripts"))
		return HTMLDisplayDBFiles("scripts\\*.int");

	if (0 == strcmp(request->pathDecoded, "/files/maps"))
		return HTMLDisplayMaps();

	if (0 == strcmp(request->pathDecoded, "/files/xfopen")) {
		std::string response = std::string(HTMLStart);
		response += "<p>These are all the files loaded via the xfopen function.</p>";
		std::vector<char*> keys;
		for (auto& kvp : xfopened)
			keys.push_back(kvp.first);

		auto table = new HTMLTable<char*>();
		table->width = 400;
		table->headers = { "File", "Source" };
		table->rowObjects = keys;
		table->bodyFunc = [](char* key, int idx) -> std::string {
			auto tr = std::string("");
			tr += "<tr>";
			tr += HTMLTable<void*>::renderCell(key);
			tr += HTMLTable<void*>::renderCell(xfopened[key]);
			tr += "</tr>";
			return tr;
		};
		response += table->render();
		response += HTMLEnd;
		return responseAllocHTML(response.c_str());
	}

	if (0 == strcmp(request->pathDecoded, "/dump/game-objects")) {
		std::vector<fo::GameObject*> vec;
		sfall::script::FillListVector(static_cast<DWORD>(FLV::ALL), vec);

		std::string response = std::string(HTMLStart);

		auto table = new HTMLTable<fo::GameObject*>();
		table->width = 800;
		table->rowObjects = vec;
		table->headers = { "Id", "protoId", "scriptId", "scriptData", "x", "y", "sx", "sy", "rotation", "frm", "artFid" };
		table->bodyFunc = [](fo::GameObject* obj, int idx) -> std::string {
			auto tr = std::string("");
			tr += "<tr>";
			tr += HTMLTable<void*>::renderCell(std::to_string(obj->id));
			tr += HTMLTable<void*>::renderCell(std::to_string(obj->protoId));
			tr += HTMLTable<void*>::renderCell(std::to_string(obj->scriptId));
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
			tr += HTMLTable<void*>::renderCell(scriptData);
			tr += HTMLTable<void*>::renderCell(std::to_string(obj->x));
			tr += HTMLTable<void*>::renderCell(std::to_string(obj->y));
			tr += HTMLTable<void*>::renderCell(std::to_string(obj->sx));
			tr += HTMLTable<void*>::renderCell(std::to_string(obj->sy));
			tr += HTMLTable<void*>::renderCell(std::to_string(obj->rotation));
			tr += HTMLTable<void*>::renderCell(std::to_string(obj->frm));
			tr += HTMLTable<void*>::renderCell(std::to_string(obj->artFid));
			tr += "</tr>\n";
			return tr;
		};
		response += table->render();
		response += HTMLEnd;

		return responseAllocHTML(response.c_str());
	}

	/* Serve files from the current directory */
	if (request->pathDecoded == strstr(request->pathDecoded, "/files")) {
		return responseAllocServeFileFromRequestPath("/files", request->path, request->pathDecoded, ".");
	}
	return responseAlloc404NotFoundHTML("You don't see anything out of the ordinary.");
}

// Module

static void Run() {
	DocumentRoot = Ini::String("HTTPD", "DocumentRoot", ".");

	serverInit(&server);

	struct sockaddr_in localhost = { 0 };
	localhost.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	localhost.sin_family = AF_INET;
	localhost.sin_port = htons(sfall::HTTPD::Port); // maybe pass it as argument?

	xfopenLoadFile = (char*)malloc(200);
	xfopenLoadedFrom = (char*)malloc(200);
	sfall::MakeCall(0x4DEFCD, xfopen_hook);
	sfall::MainLoopHook::OnMainLoop() += OnMainLoop;

	acceptConnectionsUntilStopped(&server, (struct sockaddr*) & localhost, sizeof(localhost));
}

void sfall::HTTPD::init() {
	// Cached in case value changes after init
	Port = Ini::Int( "HTTPD", "Port", 1207);

	if (Port)
		thread = std::thread(Run);
}

void sfall::HTTPD::exit() {
	if (Port) {
		serverDeInit(&server);
		thread.join();
	}
}