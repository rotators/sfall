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
int refreshwm = 0x4C3830;
int hoveringHotspot = 0;
void __declspec(naked) wmDetectHotspotHover() {
	__asm {
		push edi
		push ebp
		push eax
		push edx
		push ecx
		mov ecx, hoveringHotspot // to check if hovering changed
		mov edi, dword ptr ss : [esp + 28] // [esp + 8], since we pushed some registers
		mov ebp, dword ptr ds : [0x51DE2C]
		mov eax, dword ptr ds : [0x51DE30]
		add edi, ebp // edi = wmMouseX
		mov ebp, dword ptr ss : [esp + 24] // [esp + 4]
		add ebp, eax // ebp = wmMouseY
		sub edi, 20
		sub ebp, 20
		// check if the cursor is inside
		mov eax, dword ptr ds : [0x672E0C]
		sub eax, edi
		cdq
		xor eax, edx
		sub eax, edx
		cmp eax, 5
		jge unset
		mov eax, dword ptr ds : [0x672E10]
		sub eax, ebp
		cdq
		xor eax, edx
		sub eax, edx
		cmp eax, 5
		jge unset
		mov hoveringHotspot, 1
		jmp change_check
	unset:
		mov hoveringHotspot, 0
	change_check:
		cmp ecx, hoveringHotspot
		je cleanup
		call refreshwm // if value has changed, refresh wm
	cleanup:
		pop ecx
		pop edx
		pop eax
		pop ebp
		pop edi
		mov eax, dword ptr ds : [0x51DE30] // we overwrote it in jmp, so it's here
		jmp jmpBack
	}
}
//extern fo::MessageList* msgWorldmap;
//fo::MessageList*& msgWorldmap = *reinterpret_cast<fo::MessageList**>(0x672FB0);

int*& msgWorldmap = *reinterpret_cast<int**>(0x672FB0);
const char* GetWorldmapMsg(int msgId) {
	//memset(msg, 0, 255);
	return fo::GetMessageStr((fo::MessageList*)0x672FB0, msgId);
}

void GetCurrentTerrain() {
	int*& terrainId = *reinterpret_cast<int**>(0x672E14);
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
	
	auto oldFont = GetFont();
	GetCurrentTerrain();
	SetFont(0x65);
	if (hoveringHotspot == 1 && !IsMovingOnWM()) {
		WmDrawText((char*)currentTerrainStr, 228, fo::var::world_xpos - fo::var::wmWorldOffsetX, (fo::var::world_ypos - fo::var::wmWorldOffsetY) + 5, 60);  // Shadow
		WmDrawText((char*)currentTerrainStr, 215, fo::var::world_xpos - fo::var::wmWorldOffsetX - 1, (fo::var::world_ypos - fo::var::wmWorldOffsetY) + 4, 60);
	}
	
	SetFont(oldFont);
}

// https://i.imgur.com/0bu4J2l.png
static void InitTerrainHover()
{
	sfall::MakeJump(0x4BFE84, wmDetectHotspotHover);
	currentTerrainStr = new char[32];
}

void sfall::Rotators::init()
{
    InitCustomDll();
    InitTerrainHover();
}

void sfall::Rotators::exit()
{
}
