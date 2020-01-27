//
// DEPRECATED METARULES
//
// Metarules which has been implemented in vanilla sfall and are no longer required/supported
// This file is NOT compiled, and is left for reference only
//

	{ "r_set_hotspot_title",  r_set_hotspot_title,     3, 3, -1, {sfall::script::ARG_INT, sfall::script::ARG_INT, sfall::script::ARG_STRING} },
	{ "r_tolower",            r_tolower,               1, 1, -1, {sfall::script::ARG_STRING} },
	{ "r_toupper",            r_toupper,               1, 1, -1, {sfall::script::ARG_STRING} },

// r_set_hotspot_title
// implemented in vanilla as set_terrain_name

static char** wmTileTerrains = nullptr;
static int wmWidthInTiles = 7;

char* rfall::wmGetCurrentTerrainName()
{
	char* terrainText;

	if (wmTileTerrains) {
		int zoneX = fo::var::world_xpos / 50;
		int zoneY = fo::var::world_ypos / 50;
		terrainText = wmTileTerrains[zoneX + zoneY * wmWidthInTiles];
		if (!terrainText)
			terrainText = (char*)fo::wmGetCurrentTerrainName();
	}
	else
		terrainText = (char*)fo::wmGetCurrentTerrainName();

	return terrainText;
}

void r_set_hotspot_title(sfall::script::OpcodeContext& ctx) {
	int x = ctx.arg(0).asInt();
	int y = ctx.arg(1).asInt();
	const char* msg = ctx.arg(2).asString();

	if (!wmTileTerrains) {
		wmTileTerrains = new char*[fo::var::wmMaxTileNum * 7 * 6];
		wmWidthInTiles = fo::var::wmNumHorizontalTiles * 7;
		memset(wmTileTerrains, 0, sizeof(char *) * fo::var::wmMaxTileNum * 7 * 6);
	}

	if (wmTileTerrains[x + y * wmWidthInTiles])
		delete wmTileTerrains[x + y * wmWidthInTiles];
	wmTileTerrains[x + y * wmWidthInTiles] = new char[strlen(msg)+1];
	strcpy(wmTileTerrains[x + y * wmWidthInTiles], msg);
}

// r_tolower
// implemented in vanilla as string_to_case
void r_tolower(sfall::script::OpcodeContext& ctx) {
	auto str = std::string(ctx.arg(0).asString()); // sue me.
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);

	ctx.setReturn(cstrdup(str.c_str()));
}

// r_toupper
// implemented in vanilla as string_to_case
void r_toupper(sfall::script::OpcodeContext& ctx) {
	auto str = std::string(ctx.arg(0).asString()); // sue me.
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);

	ctx.setReturn(cstrdup(str.c_str()));
}

// non-blocking r_message_box leftovers
struct DialogOutData
{
	// General data
	uint8_t Type; // dialog_out_ caller type; 0=engine, 1=dll, 2=ssl

	// DLL data
	// TODO

	// SSL data
	fo::Program* Program;
	std::string  ProcName;

	// Result
	uint32_t Input;

	DialogOutData() {
		Clear();
	}

	void Clear()
	{
		Type = Input = 0;
		Program = nullptr;
		ProcName.clear();
		Input = 0;
	};
};
static struct DialogOutData dialogOut;

DialogOut()
	if (!dialogOut.Type)
		dialogOut.Type = 1;

static void __declspec(naked) DialogOut_get_input_hook() { // cache get_input_ return value until engine decides what to do next
	static int input;

	__asm {
		nop;
		call fo::funcoffs::get_input_; // restore
		mov input, eax;
	}

	dialogOut.Input = input;

	__asm {
		nop;
		ret;
	}
}

static void __stdcall DialogOut_message_exit_cpp() {
	if (dialogOut.Type == 0) {
		/* pass without changes */
	}
	else {
		bool result = false;

		switch (dialogOut.Input) {
			case 'Y':
			case 'y':
			case '\r':  // enter
			case 0x1f4: // button
				result = true;
				break;
			case 'N':
			case 'n':
			case 27:    // esc
			case 0x1f5: // button
				result = false;
				break;
			default:
				sfall::dlog_f( "DialogOut : unknown input<%d>\n", DL_MAIN, dialogOut.Input );
				break;
		}

		if (dialogOut.Type == 1) {
			// TODO
		}
		else if (dialogOut.Type == 2 && dialogOut.Program && !dialogOut.ProcName.empty()) {
			fo::Program* program = dialogOut.Program;
			std::string procName = dialogOut.ProcName;
			dialogOut.Clear();

			RunProgramWithFixedParam(program, procName, result ? 1 : 0);
		}
	}

	dialogOut.Clear();
}

static void __declspec(naked) DialogOut_message_exit_hook() {
	__asm {
		nop;
		call fo::funcoffs::message_exit_; // restore
		pushad;
	}

	DialogOut_message_exit_cpp();

	__asm {
		nop;
		popad;
		ret;
	}
}

r_message_box()
	if (ctx.numArgs() >= 5) {
		// callback is stored as global variable - only one can be active at a time
		if (dialogOut.Program) {
			ctx.setReturn(-1, sfall::script::DataType::INT);
			return;
		}

		dialogOut.Type = 2;
		dialogOut.Program = ctx.program();
		dialogOut.ProcName =  ctx.arg(4).asString();
	}

Script::init()
	HookCall(0x41dd84, DialogOut_get_input_hook);    // dialog_out_
	HookCall(0x41de78, DialogOut_message_exit_hook); // dialog_out_
