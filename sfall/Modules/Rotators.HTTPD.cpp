#ifdef HTTPD_SERVER // why do we need that, again?

#include <cstdint>
#include <string>
#include <thread>

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

static void loadmap() {
	__asm {
		pushad
		mov eax, [mapIdToLoad]
		call fo::funcoffs::map_load_idx_
		popad
	}
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

//

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
					fo::DbFile* intFile = fo::func::db_fopen(script->program->fileName, "r");
					long intSize = db::filelen(intFile);
					fo::func::db_fclose(intFile);

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


#endif // HTTPD_SERVER //

