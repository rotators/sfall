#include <string>
#include <vector>
#include <thread>

#include "..\main.h"
#include "..\SafeWrite.h"
#include "..\FalloutEngine\Fallout2.h"
#ifdef HTTPD_SERVER
  #include "..\Lib\EmbeddableWebServer.h"
  void InitHTTPD();
  static struct Server server;
#endif

#include "Rotators.h"

// Remember to wear protective goggles :)

const char rotatorsIni[] = ".\\ddraw.rotators.ini";
const char* currentTerrainStr;

// DisplayTerrainOnHotspotHover related variables
bool isMouseOverHotspot;
bool displayTerrainOnHotspot;
BYTE terrainOnHotspotTextColor;
BYTE terrainOnHotspotShadowColor;

std::thread thread;

// Variables related to WorldTravelMarker in Interface.cpp
namespace sfall
{
	extern BYTE dot_color;
	extern long spaceLen;
	extern long dotLen;
}

static void InitCustomDll()
{
    std::vector<std::string> names = sfall::GetIniList( "Debugging", "CustomDll", "", 512, ',', rotatorsIni );

    for( const auto& name : names )
    {
        if( name.empty() )
            continue;

        sfall::dlog_f( "Loading %s... ", DL_MAIN, name.c_str() );

        HMODULE dll = LoadLibraryA( name.c_str() );

        if( !dll || dll == INVALID_HANDLE_VALUE )
            sfall::dlogr( "ERROR", DL_MAIN );
        else
            sfall::dlogr( "OK", DL_MAIN );
    }
}

int jmpBack = 0x4BFE89;
void __declspec(naked) wmDetectHotspotHover() {
	int wmMouseX, wmMouseY;
	int deltaX, deltaY;
	bool oldIsMouseOverHotspot;
	__asm {
		pushad
		mov ebp, esp
		sub esp, __LOCAL_SIZE
		mov eax, dword ptr ss : [esp + 60] // if you declare more variables above, increment this and the below esp offset
		mov[ebp - 4], eax
		mov eax, dword ptr ss : [esp + 64]
		mov[ebp - 8], eax
	}
	oldIsMouseOverHotspot = isMouseOverHotspot;
	deltaX = abs((long)fo::var::world_xpos - (wmMouseX - 20 + fo::var::wmWorldOffsetX));
	deltaY = abs((long)fo::var::world_ypos - (wmMouseY - 20 + fo::var::wmWorldOffsetY));

	isMouseOverHotspot = deltaX < 5 && deltaY < 5;
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

// Called from Interface.cpp
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

static int GetConfigInt(const char* section, const char* setting, int defaultValue) {
	return sfall::iniGetInt(section, setting, defaultValue, rotatorsIni);
}

// https://i.imgur.com/0bu4J2l.png
static void InitTerrainHover()
{
	displayTerrainOnHotspot     = GetConfigInt("Interface", "DisplayTerrainOnHotspotHover", 0);
	terrainOnHotspotTextColor   = GetConfigInt("Interface", "TerrainOnHotspotTextColor", 215);
	terrainOnHotspotShadowColor = GetConfigInt("Interface", "TerrainOnHotspotTextShadowColor", 228);
	if(displayTerrainOnHotspot)
		sfall::MakeJump(0x4BFE84, wmDetectHotspotHover);
	currentTerrainStr = "";
}

void sfall::Rotators::init()
{
    InitCustomDll();
    InitTerrainHover();
	#ifdef HTTPD_SERVER
	  thread = std::thread(InitHTTPD);
	#endif
}

void sfall::Rotators::exit()
{
	#ifdef HTTPD_SERVER
	  thread.detach();
	#endif
}

// Move this somewhere else?
#ifdef HTTPD_SERVER
void InitHTTPD()
{
	serverInit(&server);
	struct sockaddr_in localhost = { 0 };
	localhost.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	localhost.sin_family = AF_INET;
	localhost.sin_port = htons(1207);

	acceptConnectionsUntilStopped(&server, (struct sockaddr*) & localhost, sizeof(localhost));
}

char* respString;
struct Response* createResponseForRequest(const struct Request* request, struct Connection* connection) {
	if (0 == strcmp(request->pathDecoded, "/welcome")) {
		char response[1024];

		sprintf(response, "<html><body><h1>Hello from ddraw.dll</h1><p>wmPosX: %d<br/>wmPosY: %d<br/>Current Terrain: %s</p></body></html>",
			fo::var::world_xpos,
			fo::var::world_ypos,
			currentTerrainStr
		);

		return responseAllocHTML(response);
	}
	if (0 == strcmp(request->pathDecoded, "/status/json")) {
		static const char* statuses[] = { ":-)", ":-(", ":-|" };
		int status = rand() % (sizeof(statuses) / sizeof(*statuses));
		/* There is also a family of responseAllocJSON functions */
		return responseAllocWithFormat(200, "OK", "application/json", "{ \"status\" : \"%s\" }", statuses[status]);
	}

	/* Serve files from the current directory */
	if (request->pathDecoded == strstr(request->pathDecoded, "/files")) {
		return responseAllocServeFileFromRequestPath("/files", request->path, request->pathDecoded, ".");
	}
	return responseAlloc404NotFoundHTML("Nope");
}
#endif