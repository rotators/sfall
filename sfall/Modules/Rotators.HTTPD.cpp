#include <cstdint>
#include <string>
#include <thread>
#include <map>

#include "..\FalloutEngine\Fallout2.h"
#include "..\SafeWrite.h"
#include "..\Utils.h"

#include "MainLoopHook.h"
#include "WorldMap.h"

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

enum HTMLTag {
	a,
	br,
	body,
	div_,
	html,
	td,
	tr,
	th,
	table,
	tbody,
	thead,
	p,
};

// Start indentation to be under <body>
BYTE HTMLIdentation = 2;

// https://developer.mozilla.org/en-US/docs/Web/HTML/Element
class HTMLElement {
	public:
		HTMLTag tag;
		std::map<std::string, std::string> attributes;
		std::string innerHTML;
		std::vector<HTMLElement*> children;

		HTMLElement::~HTMLElement() {
			for (auto ch : children) {
				delete(ch);
			}
		}

		HTMLElement::HTMLElement(HTMLTag tag) {
			this->tag = tag;
		}

		HTMLElement::HTMLElement(HTMLTag tag, std::string innerHTML) {
			this->tag = tag;
			this->innerHTML = innerHTML;
		}

		HTMLElement* add(HTMLElement* child) {
			this->children.push_back(child);
			return this;
		}

		std::string getTagStr() {
			switch (tag) {
				case HTMLTag::a:     return "a";
				case HTMLTag::br:    return "br";
				case HTMLTag::div_:  return "div";
				case HTMLTag::body:  return "body";
				case HTMLTag::td:    return "td";
				case HTMLTag::tr:    return "tr";
				case HTMLTag::th:    return "th";
				case HTMLTag::table: return "table";
				case HTMLTag::tbody: return "tbody";
				case HTMLTag::thead: return "thead";
				case HTMLTag::p:     return "p";
				default: throw "invalid tag";
			}
		}
		
		bool selfClosing() {
			switch (tag) {
				case HTMLTag::br: return true;
				default: return false;
			}
		}

		std::string render() {
			auto tag = this->getTagStr();
			std::string buf("");
			buf += '\n';
			// make sure that stuff is properly indented in the generated source.
			for (auto i = 0; i < HTMLIdentation;i++) {
				buf += '\t';
			}

			buf += '<';
			buf += tag;
			if (attributes.size() > 0)
				buf += ' ';

			bool first = true;
			for (auto attr : attributes) {
				buf += attr.first;
				buf += '=';
				buf += '"';
				buf += attr.second;
				buf += '"';
				if (!first) {
					buf += ' ';
				}
				first = false;
			}
			if (selfClosing()) {
				buf += "/>";
				return buf;
			}
			else {
				buf += '>';
			}
			HTMLIdentation++;
			for (auto ch : children) {
				buf += ch->render();
			}
			HTMLIdentation--;
			buf += innerHTML;
			if (children.size() != 0) {
				buf += '\n';
				for (auto i = 0; i < HTMLIdentation; i++) {
					buf += '\t';
				}
			}
			buf += "</";
			buf += tag;
			buf += ">";
			return buf;
		}

		static HTMLElement* a(std::string innerHTML) { return new HTMLElement(HTMLTag::a, innerHTML); }
		static HTMLElement* div() { return new HTMLElement(HTMLTag::div_); }
		static HTMLElement* tbody() { return new HTMLElement(HTMLTag::tbody); }
		static HTMLElement* tbody(std::string innerHTML) { return new HTMLElement(HTMLTag::tbody, innerHTML); }
		static HTMLElement* thead() { return new HTMLElement(HTMLTag::thead); }
		static HTMLElement* td(std::string innerHTML) { return new HTMLElement(HTMLTag::td, innerHTML); }
		static HTMLElement* td() { return new HTMLElement(HTMLTag::td); }
		static HTMLElement* th(std::string innerHTML) { return new HTMLElement(HTMLTag::th, innerHTML); }
		static HTMLElement* tr() { return new HTMLElement(HTMLTag::tr); }
		static HTMLElement* p() { return new HTMLElement(HTMLTag::p); }
		static HTMLElement* p(std::string innerHTML) { return new HTMLElement(HTMLTag::p, innerHTML); }
};

enum RouteParserType {
	u8,
	u16,
	u32,
	i8,
	i16,
	i32,
	str
};

class Route {
	public:
		virtual Response* match() = 0;
		virtual void setContext(HTMLElement* body, char* decoded) = 0;
};

#define CHECK_TYPE(__type) if (spl[1].compare(#__type) == 0) tagtype = RouteParserType::__type
template<typename T>
class RouteParser : public Route {
	public:
		RouteParser(std::string _template, std::function<Response*(RouteParser<T>*, HTMLElement*)> handler) {
			this->_template = _template;
			this->handler = handler;
		}
		std::map<std::string, std::string> variables;
		char* decoded;
		HTMLElement* body;
		std::function<Response*(RouteParser<T>*, HTMLElement*)> handler;
		std::string _template;
		void setContext(HTMLElement* body, char* decoded) {
			this->body = body;
			this->decoded = decoded;
		}
		Response* match() {
			if (matchTemplate(decoded, _template)) {
				return this->handler(this, body);
			}
			return NULL;
		}
		virtual void parseVariables(){};

		// /loadmap/{id:u8}/{s:str}
	private:
		// Validate templates
		// {id:u8} = variable is named id and the type is uint8
		// Datatypes: u8, u16, u32, i8, i16, i32, str (char*)
		bool matchTemplate(char decoded[1024], std::string _template) {
			bool parsingTag = false;
			std::string tagname("");
			RouteParserType tagtype;
			std::string tagbuf("");
			std::string matchbuf(""); // The decoded buffer that we compare with our tagbuf.
			char parseUntil = ' '; // What character is the next character after the tag ends?

			int pd=0; // pos of decoded
			int pt=0; // pos of template

			int maxLen = strlen(decoded);
			int parseState=0;

			bool done = false;
			while(!done) {
				switch (parseState) {
					case 0: {
						if (_template[pt] == '{') {
							parseState = 1;
							continue;
						}
						if (decoded[pd++] != _template[pt++])
							return false;
					} break;
					case 1: { // Parse tag contents (everything in {})
						if (_template[pt] == '}') {
							parseState = 2;

							// Validate tag contents
							if (strstr(tagbuf.c_str(), ":") == NULL) {
								throw "no seperator : in tag";
							}
							auto spl = sfall::split(tagbuf, ':');
							tagname = spl[0].substr(1);
							matchbuf = "";
							
							CHECK_TYPE(u8); CHECK_TYPE(u16); CHECK_TYPE(u32); //CHECK_TYPE(u64);
							CHECK_TYPE(i8); CHECK_TYPE(i16); CHECK_TYPE(i32); //CHECK_TYPE(i64);
							CHECK_TYPE(str);

							if (pt + 1 == _template.size()) {
								parseUntil = '\0'; // Parse until the end
							}
							else {
								parseUntil = _template[pt + 1];
							}
							continue;
						}

						tagbuf += _template[pt++];
					} break;
					case 2: { // Compare with decoded string.
						if (decoded[pd] == parseUntil || pd == 1024) {
							try {
								if (tagtype != RouteParserType::str) {
									auto i = std::stoi(matchbuf);
									auto ui = std::stoul(matchbuf);
									switch (tagtype) {
										case u8: if (i < 0 || i > 255) return false;
											break;
										case u16: if (i < 0 || i > 65535) return false;
											break;
										case u32: if (i < 0) return false;
											break;
										case i8:  if (i < -128 || i > 127) return false;
											break;
										case i16: if (i < 0 || i > 65535) return false;
											break;
										case i32: if (ui > 2147483647) return false;
											break;
									}
								}
							}
							catch (...) { return false; } // Since value couldn't be parsed, it's not a matching route.

							// Add the captured variable
							variables[tagname] = matchbuf;

							// Are we done?
							if (pd < maxLen) {
								parseState = 0;
							}
							else {
								done = true;
							}
							continue;
						}

						matchbuf += decoded[pd++];
					} break;
				}
			}
			parseVariables();
			return true;
		}
};

// Not nice...
class IdRoute : public RouteParser<IdRoute> {
	  public: 
		  unsigned long id;
	  IdRoute(std::string __template, std::function<Response*(RouteParser<IdRoute>*, HTMLElement*)> handler) : RouteParser(__template, handler) {};
	  void parseVariables() {
		 id = std::stoul(this->variables["id"]);
	  }
};

class GameObjectRoute : public RouteParser<GameObjectRoute> {
public:
	rfall::misc::FLV type;
	GameObjectRoute(std::string __template, std::function<Response * (RouteParser<GameObjectRoute >*, HTMLElement*)> handler) : RouteParser(__template, handler) {};
	void parseVariables() {
		type = (rfall::misc::FLV)std::stoul(this->variables["type"]);
	}
};

template<typename T>
class HTMLTable {
	public:
		int width=0;
		std::vector<T> rowObjects;
		std::vector<std::string> headers;
		std::function<HTMLElement*(T, int)> bodyFunc;

	HTMLElement* createHeader() {
		auto thead = new HTMLElement(HTMLTag::thead);
		auto tr = HTMLElement::tr();
		thead->children.push_back(tr);
		for (auto const& text : headers)
			tr->children.push_back(HTMLElement::th(text));
		return thead;
	}

	static std::string renderCell(std::string val) {
		return HTMLElement::td(val)->render();
	}

	HTMLElement* createBody() {
		auto tbody = HTMLElement::tbody();
		int i = 0;
		for (auto const& obj : rowObjects) {
			tbody->add(this->bodyFunc(obj, i++));
		}
		return tbody;
	}

	HTMLElement* get() {
		auto table = new HTMLElement(HTMLTag::table);
		if (width != 0)
			table->attributes["style"] = "width: " + std::to_string(width) + "px";
		table->add(this->createHeader());
		table->add(this->createBody());
		return table;
	}
};

class HTMLUtils {

	public:

	static HTMLElement* URL(std::string href, std::string text) { 
		auto a = HTMLElement::a(text); 
		a->attributes["href"] = href;
		return a;
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

HTMLElement* HTMLDisplayMaps() {
	auto div = HTMLElement::div();
	auto mapsInfo = *(WMMapInfo(*)[])((*(DWORD*)(FO_VAR_wmMapInfoList)));
	std::vector<WMMapInfo> maps;
	
	for (int i = 0; i < fo::var::wmMaxMapNum; i++) {
		maps.push_back(mapsInfo[i]);
	}
	std::string response = std::string(HTMLStart);
	div->add(HTMLElement::p("These are all from Maps.txt"));
	auto table = new HTMLTable<WMMapInfo>();
	table->width = 400;
	table->rowObjects = maps;
	table->headers =  { "Id", "Name", "Music", "Teleport" };
	table->bodyFunc = [](WMMapInfo map, int idx) -> HTMLElement* {
		auto href = std::string("/loadmap/");
		href += std::to_string(idx);
		auto tr = HTMLElement::tr();
		tr->add(HTMLElement::td(std::to_string(idx)));
		tr->add(HTMLElement::td(map.mapName));
		tr->add(HTMLElement::td(map.music));
		tr->add(HTMLElement::td()->add(HTMLUtils::URL(href, "Teleport")));
		return tr;
	};
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

// :)
#define ADD_ROUTE(__class, __template) routes.push_back(new __class(std::string(__template), ([](RouteParser<__class>* ctx, HTMLElement* body) -> Response*
#define IS_URL(__path) 0 == strcmp(request->pathDecoded, __path)
#define DOC_ROOT responseAllocServeFileFromRequestPath("/", request->path, request->pathDecoded, DocumentRoot.c_str())
#define DOC_INDEX responseAllocServeFileFromRequestPath("/", "/index.html", "/index.html", DocumentRoot.c_str())
std::vector<Route*> routes;
void InitRoutes() {
	ADD_ROUTE(IdRoute, "/loadmap/{id:u8}") {
		mapIdToLoad = ((IdRoute*)ctx)->id;
		body->add(HTMLDisplayMaps());
		return responseAllocHTML(RenderBody(body).c_str());
	})));

	ADD_ROUTE(GameObjectRoute, "/dump/game-objects/?t={type:u8}") {
		std::vector<fo::GameObject*> vec;
		rfall::misc::FLV type = ((GameObjectRoute*)ctx)->type;
		misc::FillListVector(type, vec);

		std::string response = std::string(HTMLStart);

		auto table = new HTMLTable<fo::GameObject*>();
		table->width = 1200;
		table->rowObjects = vec;
		table->headers = { "Id", "protoId", "scriptId", "scriptData", "x", "y", "sx", "sy", "rotation", "frm", "artFid", "artName" };
		table->bodyFunc = [](fo::GameObject* obj, int idx) -> HTMLElement* {
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
			return tr;
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
		return responseAllocHTML(RenderBody(body).c_str());
	})));
}

struct Response* createResponseForRequest(const struct Request* request, struct Connection* connection) {
	// RESERVED
	// /db/*
	auto body = new HTMLElement(HTMLTag::body);
	auto d = (char*)request->pathDecoded;

	if (IS_URL("/") || IS_URL("/style.css") || IS_URL("/script.js")) {
		return DOC_ROOT;
	}
	// Resolve all routes
	for (auto& route : routes) {
		route->setContext(body, (char*)request->pathDecoded);
		auto m = route->match();
		if (m != NULL)
			return m;
	}

	if (IS_URL("/files/scripts"))
		return HTMLDisplayDBFiles("scripts\\*.int");

	if (IS_URL("/files/maps")) {
		body->add(HTMLDisplayMaps());
		return responseAllocHTML(RenderBody(body).c_str());
	}

	if (IS_URL("/cheats/give-xp")) {
		fo::func::stat_pc_add_experience(10000);
		return DOC_INDEX;
	}

	if (IS_URL("/files/xfopen")) {
		body->add(HTMLElement::p("These are all the files loaded via the xfopen function."));
		std::vector<char*> keys;
		for (auto& kvp : xfopened)
			keys.push_back(kvp.first);

		auto table = new HTMLTable<char*>();
		table->width = 400;
		table->headers = { "File", "Source" };
		table->rowObjects = keys;
		table->bodyFunc = [](char* key, int idx) -> HTMLElement* {
			auto tr = HTMLElement::tr();
			tr->add(HTMLElement::td(key));
			tr->add(HTMLElement::td(xfopened[key]));
			return tr;
		};
		body->add(table->get());
		return responseAllocHTML(RenderBody(body).c_str());
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

	xfopenLoadFile   = (char*)malloc(200);
	xfopenLoadedFrom = (char*)malloc(200);
	sfall::MakeCall(0x4DEFCD, xfopen_hook);
	sfall::MainLoopHook::OnMainLoop() += OnMainLoop;
	InitRoutes();
	// Switching map in Combat crashes the game and from WM it doesn't work, 
	// likely needs the use the "enter map from wm" function instead.
	//sfall::MainLoopHook::OnCombatLoop() += OnCombatLoop;
	//sfall::Worldmap::OnWorldmapLoop() += OnWMLoop;

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