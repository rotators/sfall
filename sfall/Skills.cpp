/*
 *    sfall
 *    Copyright (C) 2011  The sfall team
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

#include <math.h>
#include <stdio.h>

#include "main.h"

#include "FalloutEngine.h"
#include "Combat.h"
#include "Objects.h"

#define SKILL_MIN_LIMIT    (-128)

struct SkillInfo {
	const char* name;
	const char* description;
	long attr;
	long image;
	long base;
	long statMulti;
	long statA;
	long statB;
	long skillPointMulti;
	// Default experience for using the skill: 25 for Lockpick, Steal, Traps, and First Aid, 50 for Doctor, and 100 for Outdoorsman.
	long experience;
	// 1 for Lockpick, Steal, Traps; 0 otherwise
	long f;
};

struct SkillModifier {
	long id;
	int maximum;
	int mod;
};

static std::vector<SkillModifier> skillMaxMods;
static SkillModifier baseSkillMax;

static BYTE skillCosts[512 * SKILL_count];
static DWORD basedOnPoints;
static double* multipliers = nullptr;

static std::vector<ChanceModifier> pickpocketMods;
static ChanceModifier basePickpocket;

static int skillNegPoints; // skill raw points (w/o limit)

static void __declspec(naked) item_w_skill_hook() {
	__asm {
		mov  edx, [esp + 4]; // item proto
		test byte ptr [edx + 0x19], 4; // weapon.flags_ext
		jnz  energy;
		mov  edx, ebx;
		jmp  item_w_damage_type_;
energy:
		inc  eax; // DMG_laser
		retn;
	}
}

static int __fastcall PickpocketMod(int base, TGameObj* critter) {
	for (DWORD i = 0; i < pickpocketMods.size(); i++) {
		if (critter->ID == pickpocketMods[i].id) {
			return min(base + pickpocketMods[i].mod, pickpocketMods[i].maximum);
		}
	}
	return min(base + basePickpocket.mod, basePickpocket.maximum);
}

static void __declspec(naked) skill_check_stealing_hack() {
	__asm {
		push edx;
		push ecx;
		mov  edx, esi;          // critter
		mov  ecx, eax;          // base (calculated chance)
		call PickpocketMod;
		pop  ecx;
		pop  edx;
		retn;
	}
}

static int __fastcall CheckSkillMax(TGameObj* critter, int base) {
	for (DWORD i = 0; i < skillMaxMods.size(); i++) {
		if (critter->ID == skillMaxMods[i].id) {
			return min(base, skillMaxMods[i].maximum);
		}
	}
	return min(base, baseSkillMax.maximum);
}

static int __fastcall SkillNegative(TGameObj* critter, int base, int skill) {
	int rawPoints = skillNegPoints;
	if (rawPoints) {
		if (rawPoints < SKILL_MIN_LIMIT) rawPoints = SKILL_MIN_LIMIT;
		SkillInfo *skills = (SkillInfo*)_skill_data;
		rawPoints *= skills[skill].skillPointMulti;
		long tagged;
		__asm {
			mov  eax, skill;
			call skill_is_tagged_;
			mov  tagged, eax;
		}
		if (tagged) rawPoints *= 2;
		base += rawPoints; // add the negative skill points after calculating the skill level
		if (base < 0) return max(-999, base);
	}
	return CheckSkillMax(critter, base);
}

static void __declspec(naked) skill_level_hack() {
	__asm {
		push ebx;           // skill
		mov  edx, esi;      // level skill (base)
		call SkillNegative; // ecx - critter
		mov  edx, 0x4AA64B;
		jmp  edx;
	}
}

static void __declspec(naked) skill_level_hook() {
	__asm {
		mov  skillNegPoints, 0;   // reset value
		call skill_points_;
		test eax, eax;
		jge  notNeg;              // skip if eax >= 0
		mov  skillNegPoints, eax; // save the negative skill points
		xor  eax, eax;            // set skill points to 0
notNeg:
		retn;
	}
}

static const DWORD skill_dec_point_limit_Ret = 0x4AAA91;
static void __declspec(naked) skill_dec_point_hack_limit() {
	__asm {
		cmp edi, SKILL_MIN_LIMIT;
		jle skip; // if raw skill point <= -128
		add esp, 4;
		jmp skill_dec_point_limit_Ret;
skip:
		mov eax, -2;
		retn;
	}
}

static void __declspec(naked) skill_inc_point_force_hack() {
	__asm {
		push ecx;
		push eax;
		mov  edx, 0x7FFFFFFF; // base
		call CheckSkillMax;   // ecx - critter
		pop  edx;             // skill level (from eax)
		cmp  edx, eax;        // eax = max
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) skill_inc_point_hack() {
	__asm {
		push ecx;
		push eax;
		mov  edx, 0x7FFFFFFF; // base
		mov  ecx, esi;        // critter
		call CheckSkillMax;
		pop  edx;             // skill level (from eax)
		cmp  edx, eax;        // eax = max
		mov  eax, edx;        // restore skill level
		pop  ecx;
		retn;
	}
}

static int __fastcall GetStatBonus(TGameObj* critter, const SkillInfo* info, int skill, int points) {
	double result = 0;
	for (int i = 0; i < 7; i++) {
		result += StatLevel(critter, i) * multipliers[skill * 7 + i];
	}
	result += points * info->skillPointMulti;
	result += info->base;
	return static_cast<int>(result);
}

//On input, ebx/edx contains the skill id, ecx contains the critter, edi contains a SkillInfo*, ebp contains the number of skill points
//On exit ebx, ecx, edi, ebp are preserved, esi contains skill base + stat bonus + skillpoints * multiplier
static const DWORD StatBonusHookRet = 0x4AA5D6;
static void __declspec(naked) skill_level_hack_bonus() {
	__asm {
		push ecx;
		push ebp;          // points
		push ebx;          // skill
		mov  edx, edi;     // info
		call GetStatBonus; // ecx - critter
		mov  esi, eax;
		pop  ecx;
		jmp  StatBonusHookRet;
	}
}

static const DWORD SkillIncCostRet = 0x4AA7C1;
static void __declspec(naked) skill_inc_point_hack_cost() {
	__asm { // eax - current skill level, ebx - current skill, ecx - num free skill points
		mov  edx, basedOnPoints;
		test edx, edx;
		jz   next;
		mov  edx, ebx;
		mov  eax, esi;
		call skill_points_;
next:
		mov  edx, ebx;
		shl  edx, 9;
		test eax, eax;
		jle  skip; // if skill level <= 0
		add  edx, eax;
skip:
		movzx eax, skillCosts[edx]; // eax - cost of the skill
		jmp  SkillIncCostRet;
	}
}

static const DWORD SkillDecCostRet = 0x4AA98D;
static void __declspec(naked) skill_dec_point_hack_cost() {
	__asm { // ecx - current skill level, ebx - current skill, esi - num free skill points
		mov  edx, basedOnPoints;
		test edx, edx;
		jz   next;
		mov  edx, ebx;
		mov  eax, edi;
		call skill_points_;
		lea  ecx, [eax - 1];
next:
		mov  edx, ebx;
		shl  edx, 9;
		test ecx, ecx;
		jle  skip; // if skill level <= 0
		add  edx, ecx;
skip:
		movzx eax, skillCosts[edx]; // eax - cost of the skill
		jmp  SkillDecCostRet;
	}
}

static void __declspec(naked) skill_dec_point_hook_cost() {
	__asm {
		mov  edx, ebx;
		shl  edx, 9;
		test eax, eax;
		jle  skip; // if skill level <= 0
		add  edx, eax;
skip:
		movzx eax, skillCosts[edx];
		retn;
	}
}

void _stdcall SetSkillMax(TGameObj* critter, int maximum) {
	if ((DWORD)critter == -1) {
		baseSkillMax.maximum = maximum;
		return;
	}

	long id = SetObjectUniqueID(critter);
	for (DWORD i = 0; i < skillMaxMods.size(); i++) {
		if (id == skillMaxMods[i].id) {
			skillMaxMods[i].maximum = maximum;
			return;
		}
	}
	SkillModifier cm;
	cm.id = id;
	cm.maximum = maximum;
	cm.mod = 0;
	skillMaxMods.push_back(cm);
}

void _stdcall SetPickpocketMax(TGameObj* critter, DWORD maximum, DWORD mod) {
	if ((DWORD)critter == -1) {
		basePickpocket.maximum = maximum;
		basePickpocket.mod = mod;
		return;
	}

	long id = SetObjectUniqueID(critter);
	for (DWORD i = 0; i < pickpocketMods.size(); i++) {
		if (id == pickpocketMods[i].id) {
			pickpocketMods[i].maximum = maximum;
			pickpocketMods[i].mod = mod;
			return;
		}
	}
	ChanceModifier cm;
	cm.id = id;
	cm.maximum = maximum;
	cm.mod = mod;
	pickpocketMods.push_back(cm);
}

void Skills_OnGameLoad() {
	pickpocketMods.clear();
	basePickpocket.maximum = 95;
	basePickpocket.mod = 0;

	skillMaxMods.clear();
	baseSkillMax.maximum = 300;
	baseSkillMax.mod = 0;
}

void SkillsInit() {
	MakeJump(0x4AA63C, skill_level_hack, 1);
	MakeCall(0x4AA847, skill_inc_point_force_hack);
	MakeCall(0x4AA725, skill_inc_point_hack);

	// fix for negative skill points
	HookCall(0x4AA574, skill_level_hook);
	// change the lower limit for negative skill points
	MakeCall(0x4AAA84, skill_dec_point_hack_limit);
	SafeWrite8(0x4AA91B,  SKILL_MIN_LIMIT);
	SafeWrite8(0x4AAA1A,  SKILL_MIN_LIMIT);
	SafeWrite32(0x4AAA23, SKILL_MIN_LIMIT);

	MakeCall(0x4ABC62, skill_check_stealing_hack);  // PickpocketMod
	SafeWrite8(0x4ABC67, 0x89);                     // mov [esp + 0x54], eax
	SafeWrite32(0x4ABC6B, 0x90909090);

	// Add an additional 'Energy Weapon' flag to the weapon flags (offset 0x0018)
	// Weapon Flags:
	// 0x00000400 - Energy Weapon (forces weapon to use Energy Weapons skill)
	HookCall(0x47831E, item_w_skill_hook);

	char buf[512], key[16];
	std::string skillsFile = GetConfigString("Misc", "SkillsFile", "", MAX_PATH);
	if (!skillsFile.empty()) {
		SkillInfo *skills = (SkillInfo*)_skill_data;

		const char* file = skillsFile.insert(0, ".\\").c_str();
		if (GetFileAttributes(file) == INVALID_FILE_ATTRIBUTES) return;

		multipliers = new double[7 * SKILL_count]();

		for (int i = 0; i < SKILL_count; i++) {
			sprintf(key, "Skill%d", i);
			if (iniGetString("Skills", key, "", buf, 64, file)) {
				char* tok = strtok(buf, "|");
				while (tok) {
					if (strlen(tok) >= 2) {
						double m = atof(&tok[1]);
						switch (tok[0]) {
						case 's': multipliers[i * 7 + 0] = m; break;
						case 'p': multipliers[i * 7 + 1] = m; break;
						case 'e': multipliers[i * 7 + 2] = m; break;
						case 'c': multipliers[i * 7 + 3] = m; break;
						case 'i': multipliers[i * 7 + 4] = m; break;
						case 'a': multipliers[i * 7 + 5] = m; break;
						case 'l': multipliers[i * 7 + 6] = m; break;
						default:
							dlogr("Warning : Invalid character for SPECIAL stats in the skills file.", DL_INIT);
						}
					}
					tok = strtok(0, "|");
				}
			} else {
				multipliers[i * 7 + skills[i].statA] = skills[i].statMulti;
				if (skills[i].statB >= 0) multipliers[i * 7 + skills[i].statB] = skills[i].statMulti;
			}
			sprintf(key, "SkillCost%d", i);
			if (iniGetString("Skills", key, "", buf, 512, file)) {
				char* tok = strtok(buf, "|");
				DWORD upto = 0;
				BYTE price = 1;
				while (tok && upto < 512) {
					if (strlen(tok)) {
						DWORD next = atoi(tok);
						while (upto < next && upto < 512) skillCosts[i * 512 + upto++] = price;
						price++;
					}
					tok = strtok(0, "|");
				}
				while (upto < 512) skillCosts[i * 512 + upto++] = price;
			} else { // set default cost values
				for (int j = 0;   j <= 100; j++) skillCosts[i * 512 + j] = 1; // 101
				for (int j = 101; j <= 125; j++) skillCosts[i * 512 + j] = 2; // 25
				for (int j = 126; j <= 150; j++) skillCosts[i * 512 + j] = 3; // 25
				for (int j = 151; j <= 175; j++) skillCosts[i * 512 + j] = 4; // 25
				for (int j = 176; j <= 200; j++) skillCosts[i * 512 + j] = 5; // 25
				for (int j = 201; j <= 511; j++) skillCosts[i * 512 + j] = 6; // 311
			}
			sprintf(key, "SkillBase%d", i);
			skills[i].base = iniGetInt("Skills", key, skills[i].base, file);

			sprintf(key, "SkillMulti%d", i);
			int multi = iniGetInt("Skills", key, skills[i].skillPointMulti, file);
			if (multi < 1) multi = 1; else if (multi > 10) multi = 10;
			skills[i].skillPointMulti = multi;

			sprintf(key, "SkillImage%d", i);
			skills[i].image = iniGetInt("Skills", key, skills[i].image, file);
		}

		MakeJump(0x4AA59D, skill_level_hack_bonus, 1);
		MakeJump(0x4AA738, skill_inc_point_hack_cost);
		MakeJump(0x4AA940, skill_dec_point_hack_cost, 1);
		HookCall(0x4AA9E1, skill_dec_point_hook_cost);
		HookCall(0x4AA9F1, skill_dec_point_hook_cost);

		basedOnPoints = iniGetInt("Skills", "BasedOnPoints", 0, file);
		if (basedOnPoints) HookCall(0x4AA9EC, (void*)skill_points_); // skill_dec_point_
	}
}

void SkillsExit() {
	delete[] multipliers;
}
