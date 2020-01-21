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
