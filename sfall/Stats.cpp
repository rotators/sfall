/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "main.h"
#include "FalloutEngine.h"
#include "PartyControl.h"

#include "Stats.h"

static DWORD statMaximumsPC[STAT_max_stat];
static DWORD statMinimumsPC[STAT_max_stat];
static DWORD statMaximumsNPC[STAT_max_stat];
static DWORD statMinimumsNPC[STAT_max_stat];

static DWORD xpTable[99];

float ExperienceMod = 1.0f; // set_xp_mod func
DWORD StandardApAcBonus = 4;
DWORD ExtraApAcBonus = 4;

static struct StatFormula {
	long base;
	long min;
	long shift[STAT_lu + 1];
	double multi[STAT_lu + 1];
} statFormulas[STAT_max_derived + 1] = {0};

static TGameObj* cCritter;

static void __declspec(naked) stat_level_hack() {
	static const DWORD StatLevelHack_Ret = 0x4AEF52;
	__asm {
		mov cCritter, eax;
		sub esp, 8;
		mov ebx, eax;
		jmp StatLevelHack_Ret;
	}
}

static int __fastcall check_stat_level(int value, DWORD stat) {
	int valLimit;
	if (cCritter == *ptr_obj_dude) {
		valLimit = statMinimumsPC[stat];
		if (value < valLimit) return valLimit;
		valLimit = statMaximumsPC[stat];
		if (value > valLimit) return valLimit;
	} else {
		valLimit = statMinimumsNPC[stat];
		if (value < valLimit) return valLimit;
		valLimit = statMaximumsNPC[stat];
		if (value > valLimit) return valLimit;
	}
	return value;
}

static void __declspec(naked) stat_level_hack_check() {
	__asm {
		mov  edx, esi;         // stat
		push 0x4AF3D7;         // return address
		jmp  check_stat_level; // ecx - value
	}
}

static void __declspec(naked) stat_set_base_hack_check() {
	static const DWORD StatSetBaseHack_RetMin = 0x4AF57E;
	static const DWORD StatSetBaseHack_RetMax = 0x4AF591;
	static const DWORD StatSetBaseHack_Ret    = 0x4AF59C;
	__asm {
		cmp esi, dword ptr ds:[_obj_dude];
		jz  pc;
		cmp ebx, statMinimumsNPC[eax];
		jl  failMin;
		cmp ebx, statMaximumsNPC[eax];
		jg  failMax;
		jmp StatSetBaseHack_Ret;
pc:
		cmp ebx, statMinimumsPC[eax];
		jl  failMin;
		cmp ebx, statMaximumsPC[eax];
		jg  failMax;
		jmp StatSetBaseHack_Ret;
failMin:
		jmp StatSetBaseHack_RetMin;
failMax:
		jmp StatSetBaseHack_RetMax;
	}
}

static void __declspec(naked) GetLevelXPHook() {
	__asm {
		dec eax;
		mov eax, [xpTable + eax * 4];
		retn;
	}
}

static void __declspec(naked) GetNextLevelXPHook() {
	__asm {
		mov eax, ds:[_Level_];
		jmp GetLevelXPHook;
	}
}

static void __declspec(naked) CalcApToAcBonus() {
	using namespace Fields;
	__asm {
		xor  eax, eax;
		mov  edi, [ebx + movePoints];
		test edi, edi;
		jz   end;
		cmp  [esp + 0x1C - 0x18 + 4], 2; // pc have perk h2hEvade (2 - vanilla bonus)
		jb   standard;
		mov  edx, PERK_hth_evade_perk;
		mov  eax, dword ptr ds:[_obj_dude];
		call perk_level_;
		imul eax, ExtraApAcBonus;        // bonus = perkLvl * ExtraApBonus
		imul eax, edi;                   // perkBonus = bonus * curAP
standard:
		imul edi, StandardApAcBonus;     // stdBonus = curAP * StandardApBonus
		add  eax, edi;                   // bonus = perkBonus + stdBonus
		shr  eax, 2;                     // acBonus = bonus / 4
end:
		retn;
	}
}

static void __declspec(naked) __stdcall ProtoPtr(DWORD pid, int** proto) {
	__asm {
		mov eax, [esp + 4];
		mov edx, [esp + 8];
		call proto_ptr_;
		retn 8;
	}
}

static void __stdcall StatRecalcDerived(TGameObj* critter) {
	int baseStats[7];
	for (int stat = STAT_st; stat <= STAT_lu; stat++) baseStats[stat] = StatLevel(critter, stat);

	int* proto;
	ProtoPtr(critter->protoId, &proto);

	for (int i = STAT_max_hit_points; i <= STAT_poison_resist; i++) {
		if (i >= STAT_dmg_thresh && i <= STAT_dmg_resist_explosion) continue;

		double sum = 0;
		for (int stat = STAT_st; stat <= STAT_lu; stat++) {
			sum += (baseStats[stat] + statFormulas[i].shift[stat]) * statFormulas[i].multi[stat];
		}
		long calcStat = statFormulas[i].base + (int)floor(sum);
		if (calcStat < statFormulas[i].min) calcStat = statFormulas[i].min;
		proto[9 + i] = calcStat; // offset from base_stat_srength
	}
}

static void __declspec(naked) stat_recalc_derived_hack() {
	__asm {
		push edx;
		push ecx;
		push eax;
		call StatRecalcDerived;
		pop  ecx;
		pop  edx;
		retn;
	}
}

static void __declspec(naked) stat_set_base_hack_allow() {
	static const DWORD StatSetBaseRet = 0x4AF559;
	__asm {
		cmp  ecx, STAT_unused;
		je   allow;
		cmp  ecx, STAT_dmg_thresh;
		jl   notAllow;
		cmp  ecx, STAT_dmg_resist_explosion;
		jg   notAllow;
allow:
		pop  eax;      // destroy return address
		jmp  StatSetBaseRet;
notAllow:
		mov  eax, -1;  // overwritten engine code
		retn;
	}
}

static void __declspec(naked) op_set_critter_stat_hack() {
	static const DWORD SetCritterStatRet = 0x455D8A;
	__asm {
		cmp  dword ptr [esp + 0x2C - 0x28 + 4], STAT_unused;
		je   allow;
		mov  ebx, 3;  // overwritten engine code
		retn;
allow:
		add  esp, 4;  // destroy return address
		jmp  SetCritterStatRet;
	}
}

//////////////////////////////// CRITTER POISON ////////////////////////////////

void __fastcall critter_check_poison_fix() {
	if (PartyControl_IsNpcControlled()) {
		// since another critter is being controlled, we can't apply the poison effect to it
		// instead, we add the "poison" event to dude again, which will be triggered when dude returns to the player's control
		QueueClearType(poison_event, nullptr);
		TGameObj* dude = PartyControl_RealDudeObject();
		QueueAdd(10, dude, nullptr, poison_event);
	}
}

static void __declspec(naked) critter_check_poison_hack_fix() {
	using namespace Fields;
	__asm {
		mov  ecx, [eax + protoId]; // critter.pid
		cmp  ecx, PID_Player;
		jnz  notDude;
		retn;
notDude:
		call critter_check_poison_fix;
		or   al, 1; // unset ZF (exit from func)
		retn;
	}
}

void __declspec(naked) critter_adjust_poison_hack_fix() {
	using namespace Fields;
	__asm {
		mov  edx, ds:[_obj_dude];
		mov  ebx, [eax + protoId]; // critter.pid
		mov  ecx, PID_Player;
		retn;
	}
}

static void __declspec(naked) critter_check_rads_hack() {
	using namespace Fields;
	__asm {
		mov  edx, ds:[_obj_dude];
		mov  eax, [eax + protoId]; // critter.pid
		mov  ecx, PID_Player;
		retn;
	}
}

// reduced code from 4.x HOOK_ADJUSTRADS
void __declspec(naked) critter_adjust_rads_hack() {
	using namespace Fields;
	__asm {
		cmp  dword ptr [eax + protoId], PID_Player; // critter.pid
		jne  notDude;
		mov  edx, ds:[_obj_dude];
		xor  eax, eax; // for continue func
notDude:
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

static void StatsReset() {
	for (size_t i = 0; i < STAT_max_stat; i++) {
		statMaximumsPC[i] = statMaximumsNPC[i] = *(DWORD*)(_stat_data + 16 + i * 24);
		statMinimumsPC[i] = statMinimumsNPC[i] = *(DWORD*)(_stat_data + 12 + i * 24);
	}
}

void Stats_OnGameLoad() {
	StatsReset();
	// Reset some settable game values back to the defaults
	StandardApAcBonus = 4;
	ExtraApAcBonus = 4;
	// XP mod set to 100%
	ExperienceMod = 1.0f;
	// HP bonus
	SafeWrite8(0x4AFBC1, 2);
	// Skill points per level mod
	SafeWrite8(0x43C27A, 5);
}

void Stats_Init() {
	StatsReset();

	MakeJump(0x4AEF4D, stat_level_hack);
	MakeJump(0x4AF3AF, stat_level_hack_check, 2);
	MakeJump(0x4AF571, stat_set_base_hack_check);

	MakeCall(0x4AF09C, CalcApToAcBonus, 3); // stat_level_

	// Allow set_critter_stat function to change STAT_unused and STAT_dmg_* stats for the player
	MakeCall(0x4AF54E, stat_set_base_hack_allow);
	MakeCall(0x455D65, op_set_critter_stat_hack); // STAT_unused for other critters

	// Fix/tweak for party control
	MakeCall(0x42D31F, critter_check_poison_hack_fix, 1);
	MakeCall(0x42D21C, critter_adjust_poison_hack_fix, 1);
	SafeWrite8(0x42D223, 0xCB); // cmp eax, edx > cmp ebx, ecx
	// also rads
	MakeCall(0x42D4FE, critter_check_rads_hack, 1);
	SafeWrite8(0x42D505, 0xC8); // cmp eax, edx > cmp eax, ecx
	MakeCall(0x42D3B0, critter_adjust_rads_hack, 1);
	SafeWrite16(0x42D3B6, 0xC085); // test eax, eax

	std::vector<std::string> xpTableList = GetConfigList("Misc", "XPTable", "", 2048);
	size_t numLevels = xpTableList.size();
	if (numLevels > 0) {
		HookCall(0x434AA7, GetNextLevelXPHook);
		HookCall(0x439642, GetNextLevelXPHook);
		HookCall(0x4AFB22, GetNextLevelXPHook);
		HookCall(0x496C8D, GetLevelXPHook);
		HookCall(0x4AFC53, GetLevelXPHook);

		for (size_t i = 0; i < 99; i++) {
			xpTable[i] = (i < numLevels)
				? atoi(xpTableList[i].c_str())
				: -1;
		}
		SafeWrite8(0x4AFB1B, static_cast<BYTE>(numLevels + 1));
	}

	std::string statsFile = GetConfigString("Misc", "DerivedStats", "", MAX_PATH);
	if (!statsFile.empty()) {
		MakeJump(0x4AF6FC, stat_recalc_derived_hack); // overrides function

		// STAT_st + STAT_en * 2 + 15
		statFormulas[STAT_max_hit_points].base            = 15; // max hp
		statFormulas[STAT_max_hit_points].multi[STAT_st]  = 1;
		statFormulas[STAT_max_hit_points].multi[STAT_en]  = 2;
		// STAT_ag / 2 + 5
		statFormulas[STAT_max_move_points].base           = 5;  // max ap
		statFormulas[STAT_max_move_points].multi[STAT_ag] = 0.5;

		statFormulas[STAT_ac].multi[STAT_ag]              = 1;  // ac
		// STAT_st - 5
		statFormulas[STAT_melee_dmg].min                  = 1;  // melee damage
		statFormulas[STAT_melee_dmg].shift[STAT_st]       = -5;
		statFormulas[STAT_melee_dmg].multi[STAT_st]       = 1;
		// STAT_st * 25 + 25
		statFormulas[STAT_carry_amt].base                 = 25; // carry weight
		statFormulas[STAT_carry_amt].multi[STAT_st]       = 25;
		// STAT_pe * 2
		statFormulas[STAT_sequence].multi[STAT_pe]        = 2;  // sequence
		// STAT_en / 3
		statFormulas[STAT_heal_rate].min                  = 1;  // heal rate
		statFormulas[STAT_heal_rate].multi[STAT_en]       = 1.0 / 3.0;

		statFormulas[STAT_crit_chance].multi[STAT_lu]     = 1;  // critical chance
		// STAT_en * 2
		statFormulas[STAT_rad_resist].multi[STAT_en]      = 2;  // rad resist
		// STAT_en * 5
		statFormulas[STAT_poison_resist].multi[STAT_en]   = 5;  // poison resist

		char key[6], buf2[256], buf3[256];
		const char* statFile = statsFile.insert(0, ".\\").c_str();
		if (GetFileAttributes(statFile) == INVALID_FILE_ATTRIBUTES) return;

		for (int i = STAT_max_hit_points; i <= STAT_poison_resist; i++) {
			if (i >= STAT_dmg_thresh && i <= STAT_dmg_resist_explosion) continue;

			_itoa(i, key, 10);
			statFormulas[i].base = iniGetInt(key, "base", statFormulas[i].base, statFile);
			statFormulas[i].min = iniGetInt(key, "min", statFormulas[i].min, statFile);
			for (int j = 0; j < STAT_max_hit_points; j++) {
				sprintf(buf2, "shift%d", j);
				statFormulas[i].shift[j] = iniGetInt(key, buf2, statFormulas[i].shift[j], statFile);
				sprintf(buf2, "multi%d", j);
				_gcvt(statFormulas[i].multi[j], 16, buf3);
				iniGetString(key, buf2, buf3, buf2, 256, statFile);
				statFormulas[i].multi[j] = atof(buf2);
			}
		}
	}
}

long __stdcall GetStatMax(int stat, int isNPC) {
	if (stat >= 0 && stat < STAT_max_stat) {
		return (isNPC) ? statMaximumsNPC[stat] : statMaximumsPC[stat];
	}
	return 0;
}

long __stdcall GetStatMin(int stat, int isNPC) {
	if (stat >= 0 && stat < STAT_max_stat) {
		return (isNPC) ? statMinimumsNPC[stat] : statMinimumsPC[stat];
	}
	return 0;
}

void __stdcall SetPCStatMax(int stat, int value) {
	if (stat >= 0 && stat < STAT_max_stat) {
		statMaximumsPC[stat] = value;
	}
}

void __stdcall SetPCStatMin(int stat, int value) {
	if (stat >= 0 && stat < STAT_max_stat) {
		statMinimumsPC[stat] = value;
	}
}

void __stdcall SetNPCStatMax(int stat, int value) {
	if (stat >= 0 && stat < STAT_max_stat) {
		statMaximumsNPC[stat] = value;
	}
}

void __stdcall SetNPCStatMin(int stat, int value) {
	if (stat >= 0 && stat < STAT_max_stat) {
		statMinimumsNPC[stat] = value;
	}
}
