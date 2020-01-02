#include <string>
#include <vector>

#include "..\main.h"
#include "..\SafeWrite.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\FalloutEngine\EngineUtils.h"

#include "Rotators.h"

// Remember to wear protective goggles :)

const char rotatorsIni[] = ".\\ddraw.rotators.ini";
const char* currentTerrainStr;

// DisplayTerrainOnHotspotHover related variables
bool displayTerrainOnHotspot;
BYTE terrainOnHotspotTextColor;
BYTE terrainOnHotspotShadowColor;

// Variables related to WorldTravelMarker in Interface.cpp
namespace sfall
{
	extern long dot_color;
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
bool hoveringHotspot = false;
void __declspec(naked) wmDetectHotspotHover() {
	int wmMouseX, wmMouseY;
	int deltaX, deltaY;
	bool hovered;
	__asm {
		pushad
		mov ebp, esp
		sub esp, __LOCAL_SIZE
		mov eax, dword ptr ss : [esp + 60] // if you declare more variables above, increment this and the below esp offset
		mov[ebp - 4], eax
		mov eax, dword ptr ss : [esp + 64]
		mov[ebp - 8], eax
	}
	hovered = hoveringHotspot;
	deltaX = abs((long)fo::var::world_xpos - (wmMouseX - 20 + fo::var::wmWorldOffsetX));
	deltaY = abs((long)fo::var::world_ypos - (wmMouseY - 20 + fo::var::wmWorldOffsetY));

	hoveringHotspot = deltaX < 5 && deltaY < 5;
	if (hoveringHotspot != hovered)
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
	if (hoveringHotspot == 1 && !IsMovingOnWM()) {
		auto x = fo::var::world_xpos - fo::var::wmWorldOffsetX;
		auto y = fo::var::world_ypos - fo::var::wmWorldOffsetY;
		WmDrawText((char*)currentTerrainStr, terrainOnHotspotShadowColor, x, y + 5, 60);  // Shadow
		WmDrawText((char*)currentTerrainStr, terrainOnHotspotTextColor, x - 1, y + 4, 60);
	}
	
	SetFont(oldFont);
}

// https://i.imgur.com/0bu4J2l.png
static void InitTerrainHover()
{
	displayTerrainOnHotspot     = sfall::GetConfigInt("Interface", "DisplayTerrainOnHotspotHover", 0, rotatorsIni);
	terrainOnHotspotTextColor   = sfall::GetConfigInt("Interface", "TerrainOnHotspotTextColor", 215, rotatorsIni);
	terrainOnHotspotShadowColor = sfall::GetConfigInt("Interface", "TerrainOnHotspotTextShadowColor", 228, rotatorsIni);
	if(displayTerrainOnHotspot)
		sfall::MakeJump(0x4BFE84, wmDetectHotspotHover);
	currentTerrainStr = new char[32];
}

static void InitTravelDotSettings()
{
	sfall::dot_color = sfall::GetConfigInt("Interface", "WorldTravelMarkerColor", 133, rotatorsIni);
	sfall::spaceLen  = sfall::GetConfigInt("Interface", "WorldTravelMarkerSpaceLen", 2, rotatorsIni);
	sfall::dotLen    = sfall::GetConfigInt("Interface", "WorldTravelMarkerDotLen", 1, rotatorsIni);
}

void sfall::Rotators::init()
{
    InitCustomDll();
    InitTerrainHover();
	InitTravelDotSettings();
}

void sfall::Rotators::exit()
{
}
